#include "engine/game_reward.h"

#include <algorithm>
#include <random>

namespace {

// what it does: Randomly shuffles a vector using the supplied random generator.
// what the inputs are: values - vector to shuffle; rng - random number generator used for ordering.
// what the outputs are: No return value (void).
template <typename T>
void ShuffleVector(std::vector<T>& values, std::mt19937& rng) {
    std::shuffle(values.begin(), values.end(), rng);
}

// what it does: Checks whether a card id belongs to the colorless reward pool.
// what the inputs are: card_id - card identifier to inspect.
// what the outputs are: Returns true for colorless cards; otherwise false (bool).
bool IsColorlessCardId(const std::string& card_id) {
    return card_id.rfind("cl_", 0) == 0;
}

// what it does: Checks whether a card id belongs to a ship-specific card pool.
// what the inputs are: card_id - card identifier to inspect; ship_id - current ship identifier.
// what the outputs are: Returns true when the card matches the ship pool; otherwise false (bool).
bool MatchesShipCardPool(const std::string& card_id, const std::string& ship_id) {
    if (ship_id == "vanguard") {
        return card_id.rfind("vg_", 0) == 0 || card_id.rfind("vgh_", 0) == 0;
    }
    if (ship_id == "drift_fox") {
        return card_id.rfind("df_", 0) == 0 || card_id.rfind("dfh_", 0) == 0;
    }
    if (ship_id == "iron_bastion") {
        return card_id.rfind("ib_", 0) == 0 || card_id.rfind("ibh_", 0) == 0;
    }
    return false;
}

// what it does: Checks whether a card id is present in a card-id list.
// what the inputs are: card_ids - list to search; card_id - card identifier to find.
// what the outputs are: Returns true when the id is present; otherwise false (bool).
bool ContainsCardId(const std::vector<std::string>& card_ids, const std::string& card_id) {
    return std::find(card_ids.begin(), card_ids.end(), card_id) != card_ids.end();
}

// what it does: Extracts the base card id from a reward or temporary card reference.
// what the inputs are: card_ref - card reference to normalize.
// what the outputs are: Returns the base reward card identifier (std::string).
std::string BaseRewardCardId(const std::string& card_ref) {
    constexpr const char* kTemporaryCopyPrefix = "tmp|";
    std::string ref = card_ref;
    if (ref.rfind(kTemporaryCopyPrefix, 0) == 0) {
        ref = ref.substr(std::char_traits<char>::length(kTemporaryCopyPrefix));
    }
    const size_t upgrade_marker = ref.find('@');
    if (upgrade_marker != std::string::npos) {
        ref.resize(upgrade_marker);
    }
    return ref;
}

// what it does: Adds base ids from a card pile to reward exclusion list.
// what the inputs are: excluded - output exclusion list; pile - card references to inspect.
// what the outputs are: No return value (void).
void AppendOwnedRewardExclusions(std::vector<std::string>* excluded, const std::vector<std::string>& pile) {
    if (excluded == nullptr) {
        return;
    }
    for (const std::string& ref : pile) {
        const std::string base_id = BaseRewardCardId(ref);
        if (!base_id.empty() && !ContainsCardId(*excluded, base_id)) {
            excluded->push_back(base_id);
        }
    }
}

// what it does: Builds a reward card pool for ship, rarity, and exclusion filters.
// what the inputs are: card_pool - available card definitions; ship_id - current ship identifier; excluded - card ids to skip; rarity - optional rarity filter.
// what the outputs are: Returns eligible reward card identifiers (std::vector<std::string>).
std::vector<std::string> BuildRewardPool(const std::vector<CardDef>& card_pool,
                                         const std::string& ship_id,
                                         const std::vector<std::string>& excluded,
                                         const CardRarity* rarity) {
    std::vector<std::string> pool;
    for (const CardDef& card : card_pool) {
        if (card.starterCopies != 0 || IsColorlessCardId(card.id)) {
            continue;
        }
        if (rarity != nullptr && card.rarity != *rarity) {
            continue;
        }
        if (!MatchesShipCardPool(card.id, ship_id) || ContainsCardId(excluded, card.id)) {
            continue;
        }
        pool.push_back(card.id);
    }
    return pool;
}

// what it does: Builds a list of currently owned base card ids to reduce duplicate rewards.
// what the inputs are: state - current run state.
// what the outputs are: Returns excluded base card identifiers (std::vector<std::string>).
std::vector<std::string> CurrentOwnedRewardExclusions(const RunState& state) {
    std::vector<std::string> excluded;
    excluded.reserve(state.player.drawPile.size() + state.player.hand.size() + state.player.discardPile.size());
    AppendOwnedRewardExclusions(&excluded, state.player.drawPile);
    AppendOwnedRewardExclusions(&excluded, state.player.hand);
    AppendOwnedRewardExclusions(&excluded, state.player.discardPile);
    AppendOwnedRewardExclusions(&excluded, state.player.exilePile);
    for (const ActiveAbilityState& ability : state.player.activeAbilities) {
        const std::string base_id = BaseRewardCardId(ability.cardRef);
        if (!base_id.empty() && !ContainsCardId(excluded, base_id)) {
            excluded.push_back(base_id);
        }
    }
    return excluded;
}

// what it does: Rolls the rarity for a battle reward based on sector.
// what the inputs are: sector - current sector number; rng - random number generator.
// what the outputs are: Returns the selected card rarity (CardRarity).
CardRarity RollBattleRewardRarity(int sector, std::mt19937& rng) {
    const int clamped_sector = std::max(1, std::min(3, sector));
    const int uncommon_percent = 25 + (clamped_sector - 1) * 5;
    const int rare_percent = 5 + (clamped_sector - 1) * 5;
    const int common_percent = std::max(0, 100 - uncommon_percent - rare_percent);
    std::uniform_int_distribution<int> roll(1, 100);
    const int value = roll(rng);
    if (value <= common_percent) {
        return CardRarity::Common;
    }
    if (value <= common_percent + uncommon_percent) {
        return CardRarity::Uncommon;
    }
    return CardRarity::Rare;
}

}  // namespace

// what it does: Applies a selected card reward or skips it, then finishes the node.
// what the inputs are: index - zero-based reward index, or negative to skip.
// what the outputs are: No return value (void).
void GameEngine::chooseReward(int index) {
    if (state_.page != AppPage::Reward) {
        return;
    }

    if (index >= 0 && static_cast<size_t>(index) < state_.reward.cardIds.size()) {
        const std::string& cardId = state_.reward.cardIds[static_cast<size_t>(index)];
        queueOwnedCardToDiscard(cardId);
        const std::string cardName = displayCardName(cardId);
        state_.statusLine = cardName + " has been integrated into your deck.";
    } else {
        state_.statusLine = "You skip the module reward and keep moving.";
    }

    finishCurrentNode();
}

// what it does: Generates random ship-appropriate card choices while preferring new cards.
// what the inputs are: count - number of card identifiers to choose.
// what the outputs are: Returns selected card identifiers (std::vector<std::string>).
std::vector<std::string> GameEngine::randomCardChoices(size_t count) {
    std::vector<std::string> choices;
    const std::vector<std::string> owned_excluded = CurrentOwnedRewardExclusions(state_);
    auto add_random_card = [&](bool prefer_new) {
        std::vector<std::string> pool = BuildRewardPool(cardPool_, state_.player.shipId, choices, nullptr);
        if (prefer_new) {
            pool.erase(std::remove_if(pool.begin(),
                                      pool.end(),
                                      [&](const std::string& card_id) {
                                          return ContainsCardId(owned_excluded, card_id);
                                      }),
                       pool.end());
        }
        ShuffleVector(pool, rng_);
        if (!pool.empty()) {
            choices.push_back(pool.front());
        }
    };

    while (choices.size() < count) {
        const size_t before = choices.size();
        add_random_card(true);
        if (choices.size() == before) {
            add_random_card(false);
        }
        if (choices.size() == before) {
            break;
        }
    }
    return choices;
}

// what it does: Generates battle reward card choices using node-type rarity bias.
// what the inputs are: count - number of card identifiers to choose; nodeType - active battle node type.
// what the outputs are: Returns selected card identifiers (std::vector<std::string>).
std::vector<std::string> GameEngine::randomBattleCardChoices(size_t count, const std::string& nodeType) {
    std::vector<std::string> choices;
    auto add_card_of_rarity = [&](CardRarity rarity) {
        std::vector<std::string> pool = BuildRewardPool(cardPool_, state_.player.shipId, choices, &rarity);
        if (pool.empty()) {
            pool = BuildRewardPool(cardPool_, state_.player.shipId, choices, nullptr);
        }
        ShuffleVector(pool, rng_);
        if (!pool.empty()) {
            choices.push_back(pool.front());
        }
    };

    if (nodeType == "boss") {
        add_card_of_rarity(CardRarity::Rare);
    } else if (nodeType == "elite") {
        add_card_of_rarity(CardRarity::Uncommon);
    }

    while (choices.size() < count) {
        add_card_of_rarity(RollBattleRewardRarity(state_.sector, rng_));
        if (choices.size() >= cardPool_.size()) {
            break;
        }
        if (BuildRewardPool(cardPool_, state_.player.shipId, choices, nullptr).empty()) {
            break;
        }
    }

    return choices;
}

// what it does: Generates random ship-appropriate card choices from one rarity.
// what the inputs are: count - number of card identifiers to choose; rarity - rarity to sample.
// what the outputs are: Returns selected card identifiers (std::vector<std::string>).
std::vector<std::string> GameEngine::randomCardChoicesByRarity(size_t count, CardRarity rarity) {
    std::vector<std::string> choices;
    const std::vector<std::string> owned_excluded = CurrentOwnedRewardExclusions(state_);
    auto add_random_card = [&](bool prefer_new) {
        std::vector<std::string> pool = BuildRewardPool(cardPool_, state_.player.shipId, choices, &rarity);
        if (prefer_new) {
            pool.erase(std::remove_if(pool.begin(),
                                      pool.end(),
                                      [&](const std::string& card_id) {
                                          return ContainsCardId(owned_excluded, card_id);
                                      }),
                       pool.end());
        }
        ShuffleVector(pool, rng_);
        if (!pool.empty()) {
            choices.push_back(pool.front());
        }
    };

    while (choices.size() < count) {
        const size_t before = choices.size();
        add_random_card(true);
        if (choices.size() == before) {
            add_random_card(false);
        }
        if (choices.size() == before) {
            break;
        }
    }
    return choices;
}

// what it does: Generates random market card choices from colorless cards.
// what the inputs are: count - number of card identifiers to choose.
// what the outputs are: Returns selected market card identifiers (std::vector<std::string>).
std::vector<std::string> GameEngine::randomMarketCardChoices(size_t count) {
    std::vector<std::string> pool;
    for (const CardDef& card : cardPool_) {
        if (card.starterCopies == 0 && IsColorlessCardId(card.id)) {
            pool.push_back(card.id);
        }
    }
    ShuffleVector(pool, rng_);
    if (pool.size() > count) {
        pool.resize(count);
    }
    return pool;
}

// what it does: Selects a random relic that the player does not already own.
// what the inputs are: None.
// what the outputs are: Returns the selected relic identifier, or empty string if none (std::string).
std::string GameEngine::randomRelic() {
    std::vector<std::string> candidates;
    for (const RelicDef& relic : relicPool_) {
        if (relic.id != "origin" && !hasRelic(relic.id)) {
            candidates.push_back(relic.id);
        }
    }
    if (candidates.empty()) {
        return "";
    }
    ShuffleVector(candidates, rng_);
    return candidates.front();
}
