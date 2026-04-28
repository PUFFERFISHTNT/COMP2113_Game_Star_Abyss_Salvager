#include "engine/game_reward.h"

#include <algorithm>
#include <random>

namespace {

// Shuffles the given vector with the supplied random generator and returns nothing.
template <typename T>
void ShuffleVector(std::vector<T>& values, std::mt19937& rng) {
    std::shuffle(values.begin(), values.end(), rng);
}

// Checks whether the card id belongs to the colorless pool and returns the result.
bool IsColorlessCardId(const std::string& card_id) {
    return card_id.rfind("cl_", 0) == 0;
}

// Checks whether the card id belongs to the current ship pool and returns the result.
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

// Searches a list for the given card id and returns whether it is present.
bool ContainsCardId(const std::vector<std::string>& card_ids, const std::string& card_id) {
    return std::find(card_ids.begin(), card_ids.end(), card_id) != card_ids.end();
}

// Removes temporary or upgrade markers from a reward card reference and returns the base id.
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

// Adds base ids from the pile into the exclusion list and returns nothing.
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

// Builds an eligible reward pool from card data, ship id, exclusions, and optional rarity and returns it.
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

// Collects currently owned base card ids from the run state and returns them for reward filtering.
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

// Rolls a battle reward rarity from sector and random generator and returns the chosen rarity.
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

// Applies the selected reward or skips when requested, then returns nothing.
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

// Samples general ship-appropriate card rewards by count and returns their ids.
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

// Samples battle reward cards using node type and count and returns their ids.
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

// Samples cards of one rarity by count and returns their ids.
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

// Samples market cards by count and returns their ids.
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

// Chooses a random unowned relic without input and returns its id.
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
