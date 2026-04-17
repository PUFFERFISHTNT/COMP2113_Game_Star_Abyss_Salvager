#include "engine/game_encounter.h"

void GameEngine::resolveEncounter(size_t index) {
    if (state_.page != AppPage::Encounter || index >= state_.encounter.choices.size()) {
        return;
    }

    const std::string typeId = state_.encounter.typeId;
    if (typeId == "repair") {
        if (state_.encounter.encounterId == "repair_upgrade") {
            resolveUpgradeSelection(index);
            return;
        }
        if (index == 0) {
            state_.player.hp = std::min(state_.player.maxHp, state_.player.hp + 10);
            state_.statusLine = "Repair crews patched the hull for 10 HP.";
        } else if (index == 1) {
            changeHeat(-6);
            state_.player.openingShieldBonus += 4;
            state_.statusLine = "The reactor vents cleanly. Your next battle starts with +4 shield.";
        } else if (index == 2) {
            state_.player.maxHp += 4;
            state_.player.hp += 4;
            state_.statusLine = "New plating installed. Max HP increased by 4.";
        } else {
            enterRepairUpgrade(0);
            state_.statusLine = "The station opens a retrofit console. Choose one card to upgrade for free.";
            return;
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
        if (state_.encounter.encounterId == "market_purge") {
            resolvePurgeSelection(index);
            return;
        }
        if (state_.encounter.encounterId == "market_upgrade") {
            resolveUpgradeSelection(index);
            return;
        }

        const std::string payload = index < state_.encounter.payloadIds.size() ? state_.encounter.payloadIds[index] : "";
        const int price = index < state_.encounter.payloadValues.size() ? state_.encounter.payloadValues[index] : 0;

        if (payload.empty() || payload == "leave") {
            state_.statusLine = "You break contact with the black market broker.";
            finishCurrentNode();
            return;
        }

        if (payload == "purge") {
            enterMarketPurge(0);
            state_.statusLine = "The broker opens a purge console. Choose one module to erase.";
            return;
        }
        if (payload == "upgrade") {
            enterMarketUpgrade(0);
            state_.statusLine = "The broker opens an upgrade console. Choose one module to improve.";
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
        if (state_.encounter.encounterId == "event_purge") {
            resolvePurgeSelection(index);
            return;
        }
        if (state_.encounter.encounterId == "event_upgrade") {
            resolveUpgradeSelection(index);
            return;
        }

        if (state_.encounter.encounterId == "distress_beacon") {
            if (index == 0) {
                state_.player.hp = std::min(state_.player.maxHp, state_.player.hp + 6);
                state_.player.scrap += 8;
                std::string upgradedCardName;
                std::string error;
                if (upgradeRandomCard(CardUpgradeSource::EventReward, &upgradedCardName, &error)) {
                    state_.statusLine = "The stranded crew transfers 8 credits, patch gel, and upgrades " +
                                        upgradedCardName + ".";
                } else {
                    state_.statusLine = "The stranded crew transfers 8 credits and patch gel.";
                }
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
                enterEventUpgrade(0);
                state_.statusLine = "Signal decryption reveals a precision tuning pattern. Choose one card to upgrade.";
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
                    enterEventPurge(0);
                    state_.statusLine = "The bribe exposes a silent purge rig. Remove one card from your deck.";
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
