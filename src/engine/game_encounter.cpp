#include "engine/game_encounter.h"

#include <algorithm>

namespace {

constexpr int kBlackMarketPurgePrice = 20;
constexpr size_t kBlackMarketPurgePageSize = 4;

}  // namespace

void GameEngine::resolveEncounter(size_t index) {
    if (state_.page != AppPage::Encounter || index >= state_.encounter.choices.size()) {
        return;
    }

    const std::string typeId = state_.encounter.typeId;
    if (typeId == "repair") {
        if (index == 0) {
            state_.player.hp = std::min(state_.player.maxHp, state_.player.hp + 10);
            state_.statusLine = "Repair crews patched the hull for 10 HP.";
        } else if (index == 1) {
            changeHeat(-6);
            state_.player.openingShieldBonus += 4;
            state_.statusLine = "The reactor vents cleanly. Your next battle starts with +4 shield.";
        } else {
            state_.player.maxHp += 4;
            state_.player.hp += 4;
            state_.statusLine = "New plating installed. Max HP increased by 4.";
        }
        finishCurrentNode();
        return;
    }

    if (typeId == "salvage") {
        if (index == 0) {
            const int scrap = 14 + (hasRelic("salvage_scanner") ? 4 : 0);
            state_.player.scrap += scrap;
            changeHeat(1);
            state_.statusLine = "You strip the wreck for " + std::to_string(scrap) + " credits.";
            finishCurrentNode();
            return;
        }
        if (index == 1) {
            state_.reward = {};
            state_.reward.banner = "Recovered salvage module";
            state_.reward.scrap = 6;
            state_.player.scrap += 6;
            state_.reward.cardIds = randomCardChoices(3);
            state_.page = AppPage::Reward;
            resumePage_ = AppPage::Reward;
            state_.statusLine = "A surviving module rack can be integrated into your deck.";
            return;
        }
        state_.statusLine = "You leave the unstable hulk drifting in the dark.";
        finishCurrentNode();
        return;
    }

    if (typeId == "market") {
        const std::string payload = index < state_.encounter.payloadIds.size() ? state_.encounter.payloadIds[index] : "";
        const int price = index < state_.encounter.payloadValues.size() ? state_.encounter.payloadValues[index] : 0;

        if (payload.empty() || payload == "leave") {
            state_.statusLine = "You break contact with the black market broker.";
            finishCurrentNode();
            return;
        }

        if (state_.encounter.encounterId == "market_purge") {
            if (payload == "market_purge_cycle") {
                enterMarketPurge(static_cast<size_t>(std::max(0, price)));
                return;
            }
            if (state_.player.scrap < price) {
                state_.statusLine = "Not enough credits for that transaction.";
                return;
            }
            if (purgeCardFromDeck(payload)) {
                state_.player.scrap -= price;
                const CardDef* card = cardById(payload);
                const std::string name = card != nullptr ? card->name : "module";
                state_.statusLine = name + " has been purged from your deck.";
                finishCurrentNode();
            } else {
                state_.statusLine = "The broker cannot find that module in your active deck.";
            }
            return;
        }

        if (payload == "purge") {
            if (state_.player.scrap < price) {
                state_.statusLine = "Not enough credits for that transaction.";
                return;
            }
            enterMarketPurge(0);
            state_.statusLine = "The broker opens a purge console. Choose one module to erase.";
            return;
        }

        if (state_.player.scrap < price) {
            state_.statusLine = "Not enough credits for that transaction.";
            return;
        }

        state_.player.scrap -= price;
        if (payload.rfind("card:", 0) == 0) {
            state_.player.discardPile.push_back(payload.substr(5));
            state_.statusLine = "Module purchased and queued into your discard pile.";
        } else {
            state_.statusLine = "The broker closes the channel.";
        }
        finishCurrentNode();
        return;
    }

    if (typeId == "event") {
        if (state_.encounter.encounterId == "distress_beacon") {
            if (index == 0) {
                state_.player.hp = std::min(state_.player.maxHp, state_.player.hp + 6);
                state_.player.scrap += 8;
                state_.statusLine = "The stranded crew transfers 8 credits and patch gel.";
            } else if (index == 1) {
                state_.player.scrap += 18;
                changeHeat(2);
                state_.statusLine = "You strip the beacon ship. The haul is rich, but the core runs hot.";
            } else {
                state_.statusLine = "You leave the signal fading behind you.";
            }
            finishCurrentNode();
            return;
        }

        if (state_.encounter.encounterId == "strange_signal") {
            if (index == 0) {
                state_.player.scrap += 10;
                state_.reward = {};
                state_.reward.banner = "Signal blueprint recovered";
                state_.reward.cardIds = randomCardChoices(3);
                state_.page = AppPage::Reward;
                resumePage_ = AppPage::Reward;
                state_.statusLine = "Signal decryption exposes a hidden module cache.";
                return;
            }
            if (index == 1) {
                state_.player.openingEnergyBonus += 1;
                state_.statusLine = "You align with the signal and bank +1 energy for your next battle.";
            } else {
                state_.statusLine = "You cut the feed before it can warp the route.";
            }
            finishCurrentNode();
            return;
        }

        if (state_.encounter.encounterId == "reactor_leak") {
            if (index == 0) {
                changeHeat(-3);
                state_.player.scrap += 8;
                state_.statusLine = "You vent the leak safely and salvage intact reactor plates.";
            } else if (index == 1) {
                state_.player.scrap += 18;
                changeHeat(3);
                state_.statusLine = "You harvest unstable fuel. Profit rises, temperature spikes.";
            } else {
                state_.player.hp = std::min(state_.player.maxHp, state_.player.hp + 4);
                state_.statusLine = "Containment seals hold. Your crew stabilizes the ship.";
            }
            finishCurrentNode();
            return;
        }

        if (state_.encounter.encounterId == "smuggler_cache") {
            if (index == 0) {
                const std::string relicId = randomRelic();
                if (!relicId.empty()) {
                    grantRelic(relicId);
                }
                applyDamageToPlayer(4);
                state_.statusLine = "The cache is trapped, but a relic survives the blast.";
                finishCurrentNode();
                return;
            }
            if (index == 1) {
                if (state_.player.scrap >= 10) {
                    state_.player.scrap -= 10;
                    state_.reward = {};
                    state_.reward.banner = "Smuggler wares";
                    state_.reward.cardIds = randomCardChoices(3);
                    state_.page = AppPage::Reward;
                    resumePage_ = AppPage::Reward;
                    state_.statusLine = "A hidden crate opens into a module auction.";
                    return;
                }
                state_.statusLine = "You do not have the 10 credits needed to bribe the lock.";
                return;
            }
            state_.statusLine = "You mark the cache and move on.";
            finishCurrentNode();
        }
    }
}

void GameEngine::enterMarketPurge(size_t offset) {
    const std::vector<std::pair<std::string, int>> candidates = purgeableCards();
    const size_t total_pages = std::max<size_t>(1, (candidates.size() + kBlackMarketPurgePageSize - 1) / kBlackMarketPurgePageSize);
    const size_t safe_offset = candidates.empty() ? 0 : std::min(offset, ((candidates.size() - 1) / kBlackMarketPurgePageSize) * kBlackMarketPurgePageSize);
    const size_t current_page = candidates.empty() ? 1 : (safe_offset / kBlackMarketPurgePageSize) + 1;
    const size_t upper = std::min(candidates.size(), safe_offset + kBlackMarketPurgePageSize);

    state_.encounter.encounterId = "market_purge";
    state_.encounter.title = "Black Market Purge";
    state_.encounter.description =
        "Select one card to erase from your deck. The black market visit ends immediately after the purge. Page " +
        std::to_string(current_page) + "/" + std::to_string(total_pages) +
        ". Starter cards may appear on later pages. Scroll with mouse wheel, [ ], or PgUp/PgDn if needed.";
    state_.encounter.choices.clear();
    state_.encounter.details.clear();
    state_.encounter.payloadIds.clear();
    state_.encounter.payloadValues.clear();
    for (size_t i = safe_offset; i < upper; ++i) {
        const CardDef* card = cardById(candidates[i].first);
        if (card == nullptr) {
            continue;
        }
        state_.encounter.choices.push_back(card->name);
        state_.encounter.details.push_back("Purge 1 copy from the deck. Copies owned: " + std::to_string(candidates[i].second) + ".");
        state_.encounter.payloadIds.push_back(card->id);
        state_.encounter.payloadValues.push_back(kBlackMarketPurgePrice);
    }

    if (upper < candidates.size()) {
        state_.encounter.choices.push_back("More purge targets");
        state_.encounter.details.push_back("Cycle to the next page of cards that can be removed. Next page: " +
                                           std::to_string(current_page + 1) + "/" + std::to_string(total_pages) +
                                           ". Scroll controls stay available while browsing.");
        state_.encounter.payloadIds.push_back("market_purge_cycle");
        state_.encounter.payloadValues.push_back(static_cast<int>(upper));
    } else {
        state_.encounter.choices.push_back("Leave the broker");
        state_.encounter.details.push_back("Abort the purge and close the channel.");
        state_.encounter.payloadIds.push_back("leave");
        state_.encounter.payloadValues.push_back(0);
    }
}

std::vector<std::pair<std::string, int>> GameEngine::purgeableCards() const {
    std::vector<std::string> all_cards;
    all_cards.insert(all_cards.end(), state_.player.drawPile.begin(), state_.player.drawPile.end());
    all_cards.insert(all_cards.end(), state_.player.hand.begin(), state_.player.hand.end());
    all_cards.insert(all_cards.end(), state_.player.discardPile.begin(), state_.player.discardPile.end());

    std::sort(all_cards.begin(), all_cards.end(), [&](const std::string& lhs, const std::string& rhs) {
        const CardDef* left = cardById(lhs);
        const CardDef* right = cardById(rhs);
        const std::string left_name = left != nullptr ? left->name : lhs;
        const std::string right_name = right != nullptr ? right->name : rhs;
        if (left_name != right_name) {
            return left_name < right_name;
        }
        return lhs < rhs;
    });

    std::vector<std::pair<std::string, int>> result;
    for (const std::string& card_id : all_cards) {
        if (!result.empty() && result.back().first == card_id) {
            ++result.back().second;
        } else {
            result.push_back({card_id, 1});
        }
    }
    return result;
}

bool GameEngine::purgeCardFromDeck(const std::string& cardId) {
    std::vector<std::vector<std::string>*> piles = {
        &state_.player.hand,
        &state_.player.drawPile,
        &state_.player.discardPile,
    };
    for (std::vector<std::string>* pile : piles) {
        std::vector<std::string>::iterator it = std::find(pile->begin(), pile->end(), cardId);
        if (it != pile->end()) {
            pile->erase(it);
            return true;
        }
    }
    return false;
}
