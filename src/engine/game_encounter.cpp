#include "engine/game_encounter.h"

#include <algorithm>

namespace {

void SetEpilogue_A(std::string& out, std::mt19937& rng) {
    std::uniform_int_distribution<int> pick(0, 2);
    switch (pick(rng)) {
        case 0: out = "The distress signal fades into deep space. You don't know if you did the right thing."; break;
        case 1: out = "Another wrecked ship left behind."; break;
        default: out = "People die in the Star Abyss every day. One more or less makes no difference."; break;
    }
}

void SetEpilogue_C(std::string& out, std::mt19937& rng) {
    std::uniform_int_distribution<int> pick(0, 2);
    switch (pick(rng)) {
        case 0: out = "Malfunction temporarily resolved. But the ship won't hold forever."; break;
        case 1: out = "System alert cleared. The next breakdown could be the next jump away."; break;
        default: out = "You barely stabilized the hull. But danger never truly fades."; break;
    }
}

void SetEpilogue_D(std::string& out, std::mt19937& rng) {
    std::uniform_int_distribution<int> pick(0, 2);
    switch (pick(rng)) {
        case 0: out = "The pirate tracking signal vanishes. You're safe for now."; break;
        case 1: out = "The shadow of black market forces always looms over you."; break;
        default: out = "You took what wasn't yours. Someday you'll pay it back."; break;
    }
}

}  // namespace

// Event state variables defined in game_route.cpp
extern int g_sa_beacon_activated;
extern int g_sa_escape_stage;
extern int g_sa_truth_stage;
extern bool g_sa_truth_post_battle;
extern int g_sa_skip_segments;
extern bool g_sa_next_elite;
extern bool g_sa_heat_accel;
extern bool g_sa_no_boss;
extern int g_sa_permanent_shield;
extern bool g_sa_escape_battle_pending;
extern bool g_sa_escape_intro_pending;
extern bool g_sa_game_intro_pending;
extern std::string g_sa_event_epilogue;

void GameEngine::resolveEncounter(size_t index) {
    if (state_.page != AppPage::Encounter || index >= state_.encounter.choices.size()) {
        return;
    }

    const std::string typeId = state_.encounter.typeId;

    // ==================== Repair ====================
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

    // ==================== Salvage ====================
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

    // ==================== Market ====================
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
            queueOwnedCardToDiscard(payload.substr(5));
            state_.statusLine = "Module purchased and queued into your discard pile.";
        } else {
            state_.statusLine = "The broker closes the channel.";
        }
        finishCurrentNode();
        return;
    }

    // ==================== Events ====================
    if (typeId == "event") {
        // Re-entry for purge / upgrade screens
        if (state_.encounter.encounterId == "event_purge") {
            resolvePurgeSelection(index);
            return;
        }
        if (state_.encounter.encounterId == "event_upgrade") {
            resolveUpgradeSelection(index);
            return;
        }

        const std::string eid = state_.encounter.encounterId;

        // ====== Category A: Rescue & Exploration ======

        if (eid == "event_a1") {  // Distress Beacon
            if (index == 0) {
                state_.player.hp = std::min(state_.player.maxHp, state_.player.hp + 10);
                state_.player.scrap += 12;
                state_.statusLine = "Rescued the stranded crew. Gained 10 HP and 12 credits.";
            } else if (index == 1) {
                changeHeat(-3);
                auto cards = randomCardChoices(1);
                if (!cards.empty()) queueOwnedCardToDiscard(cards[0]);
                state_.statusLine = "Relayed the coordinates. -3 heat and gained a card.";
            } else {
                state_.statusLine = "You leave the signal fading behind you.";
            }
            SetEpilogue_A(g_sa_event_epilogue, rng_);
            finishCurrentNode();
            return;
        }

        if (eid == "event_a2") {  // Ghost Ship
            if (index == 0) {
                applyDamageToPlayer(5);
                const std::string relicId = randomRelic();
                if (!relicId.empty()) grantRelic(relicId);
                state_.statusLine = "Lost 5 HP exploring, but found a rare relic aboard the ghost ship.";
                finishCurrentNode();
            } else if (index == 1) {
                state_.player.hp = std::min(state_.player.maxHp, state_.player.hp + 8);
                changeHeat(2);
                state_.statusLine = "The ghost ship responds with supplies. +8 HP, +2 heat.";
                finishCurrentNode();
            } else {
                // Forced normal battle
                state_.activeNodeType = "battle";
                startBattle("battle");
                state_.statusLine = "The ghost ship's defenses activate! Battle!";
            }
            return;
        }

        // ====== Category B: Anomalous Signals ======

        if (eid == "event_b1") {  // Strange Signal
            if (index == 0) {
                g_sa_next_elite = true;
                auto cards = randomCardChoices(1);
                if (!cards.empty()) queueOwnedCardToDiscard(cards[0]);
                state_.statusLine = "Locked onto the signal. Gained a card. Elite encounter chance increased next.";
            } else if (index == 1) {
                changeHeat(-4);
                state_.statusLine = "Blocked the interference. -4 heat.";
            } else {
                state_.player.scrap += 8;
                state_.statusLine = "Recorded the spectrum. +8 credits.";
            }
            finishCurrentNode();
            return;
        }

        if (eid == "event_b2") {  // Space Fold
            if (index == 0) {
                g_sa_skip_segments = 1;
                changeHeat(5);
                auto cards = randomCardChoicesByRarity(1, CardRarity::Rare);
                if (!cards.empty()) queueOwnedCardToDiscard(cards[0]);
                state_.statusLine = "Entered the fold. Skip 1 segment. +5 heat. Gained a rare card.";
            } else if (index == 1) {
                if (state_.player.scrap < 10) {
                    state_.statusLine = "Not enough credits. The space fold collapses.";
                    finishCurrentNode();
                    return;
                }
                state_.player.scrap -= 10;
                changeHeat(-4);
                g_sa_permanent_shield += 2;
                state_.statusLine = "Stabilized the space. -4 heat, +2 permanent shield. Costs 10 credits.";
            } else {
                state_.player.nextBattleFirstAttackMultiplier = 2;
                state_.statusLine = "Steered clear. Next battle's first attack enhanced.";
            }
            finishCurrentNode();
            return;
        }

        if (eid == "event_b3") {  // Ark Beacon
            if (index == 0) {
                auto cards = randomCardChoicesByRarity(1, CardRarity::Rare);
                if (!cards.empty()) {
                    queueOwnedCardToDiscard(cards[0]);
                }
                changeHeat(8);
                g_sa_beacon_activated = 1;
                state_.statusLine = "The Ark's beacon activates. Gained a rare card. +8 heat. Something awakens.";
            } else if (index == 1) {
                state_.player.scrap += 25;
                const std::string relicId = randomRelic();
                if (!relicId.empty()) grantRelic(relicId);
                applyDamageToPlayer(6);
                state_.statusLine = "Dismantled the beacon. +25 credits, gained a relic, -6 HP.";
            } else {
                // Trigger escape route
                g_sa_escape_stage = 1;
                g_sa_no_boss = true;
                state_.statusLine = "Blocked the beacon. Escape route initiated. No more boss encounters.";
            }
            finishCurrentNode();
            return;
        }

        if (eid == "event_story_boss") {
            if (index == 0) {
                const std::string payload = index < state_.encounter.payloadIds.size()
                                                ? state_.encounter.payloadIds[index]
                                                : "";
                state_.activeNodeType = "boss";
                startBattle("boss", payload);
                state_.statusLine = "Signal lock confirmed. Story boss battle engaged.";
            } else {
                state_.statusLine = "You break lock and leave the anomalous signature behind.";
                finishCurrentNode();
            }
            return;
        }

        // ====== Category C: Ship Malfunctions ======

        if (eid == "event_c1") {  // Reactor Leak
            if (index == 0) {
                applyDamageToPlayer(3);
                changeHeat(-4);
                state_.statusLine = "Manual stabilization. -3 HP, -4 heat.";
            } else if (index == 1) {
                if (state_.player.scrap >= 8) {
                    state_.player.scrap -= 8;
                    state_.statusLine = "Emergency cooling activated. -8 credits.";
                } else {
                    state_.statusLine = "Not enough credits for emergency cooling.";
                    return;
                }
            } else {
                g_sa_heat_accel = true;
                state_.statusLine = "Alarm silenced. Heat will build faster next segment.";
            }
            SetEpilogue_C(g_sa_event_epilogue, rng_);
            finishCurrentNode();
            return;
        }

        if (eid == "event_c2") {  // Hull Breach
            if (index == 0) {
                if (state_.player.scrap < 10) {
                    state_.statusLine = "Not enough credits for welding supplies.";
                    return;
                }
                state_.player.scrap -= 10;
                applyDamageToPlayer(4);
                changeHeat(3);
                state_.statusLine = "Emergency welding complete. -4 HP, +3 heat, -10 credits.";
            } else if (index == 1) {
                if (state_.player.scrap >= 8) {
                    state_.player.scrap -= 8;
                    state_.statusLine = "Dumped cargo to seal the breach. -8 credits.";
                } else {
                    state_.statusLine = "Not enough credits to dump cargo.";
                    return;
                }
            } else {
                applyDamageToPlayer(8);
                state_.statusLine = "Left the breach open. Lost 8 HP from atmosphere venting.";
            }
            SetEpilogue_C(g_sa_event_epilogue, rng_);
            finishCurrentNode();
            return;
        }

        if (eid == "event_c3") {  // Deck Chaos
            if (index == 0) {
                // Upgrade 2 random cards, lose 4 HP
                std::string upgName, upgErr;
                upgradeRandomCard(CardUpgradeSource::EventReward, &upgName, &upgErr);
                upgradeRandomCard(CardUpgradeSource::EventReward, &upgName, &upgErr);
                applyDamageToPlayer(4);
                state_.statusLine = "Manual sorting complete. 2 cards upgraded, -4 HP.";
            } else if (index == 1) {
                // -5 heat, delete 1 low-quality card, costs 10 credits
                if (state_.player.scrap >= 10) {
                    state_.player.scrap -= 10;
                    changeHeat(-5);
                    // Purge a random common-rarity card
                    std::vector<std::string> lowCards;
                    for (const auto& cr : state_.player.drawPile) {
                        const CardDef* def = cardById(baseCardId(cr));
                        if (def && def->rarity == CardRarity::Common) lowCards.push_back(cr);
                    }
                    for (const auto& cr : state_.player.discardPile) {
                        const CardDef* def = cardById(baseCardId(cr));
                        if (def && def->rarity == CardRarity::Common) lowCards.push_back(cr);
                    }
                    if (!lowCards.empty()) {
                        std::uniform_int_distribution<size_t> pick(0, lowCards.size() - 1);
                        purgeCardFromDeck(lowCards[pick(rng_)]);
                    }
                    state_.statusLine = "System rebooted. -5 heat, removed a low-quality card. -10 credits.";
                } else {
                    state_.statusLine = "Not enough credits to reboot the system.";
                    return;
                }
            } else {
                // Force format: costs 15 credits, clear all low-quality cards
                if (state_.player.scrap >= 15) {
                    state_.player.scrap -= 15;
                    std::vector<std::string> toPurge;
                    for (const auto& cr : state_.player.drawPile) {
                        const CardDef* def = cardById(baseCardId(cr));
                        if (def && def->rarity == CardRarity::Common) toPurge.push_back(cr);
                    }
                    for (const auto& cr : state_.player.discardPile) {
                        const CardDef* def = cardById(baseCardId(cr));
                        if (def && def->rarity == CardRarity::Common) toPurge.push_back(cr);
                    }
                    for (const auto& cid : toPurge) purgeCardFromDeck(cid);
                    state_.statusLine = "Force format complete. All low-quality cards purged. -15 credits.";
                } else {
                    state_.statusLine = "Not enough credits for force format.";
                    return;
                }
            }
            SetEpilogue_C(g_sa_event_epilogue, rng_);
            finishCurrentNode();
            return;
        }

        // ====== Category D: Black Market & Underworld ======

        if (eid == "event_d1") {  // Smuggler Cache
            if (index == 0) {
                state_.player.scrap += 16;
                auto cards = randomCardChoices(1);
                if (!cards.empty()) queueOwnedCardToDiscard(cards[0]);
                state_.statusLine = "Took everything. +16 credits and gained a card.";
            } else if (index == 1) {
                state_.player.hp = std::min(state_.player.maxHp, state_.player.hp + 6);
                state_.statusLine = "Took only essentials. +6 HP.";
            } else {
                state_.statusLine = "Left the cache alone. No risk, no reward.";
            }
            SetEpilogue_D(g_sa_event_epilogue, rng_);
            finishCurrentNode();
            return;
        }

        if (eid == "event_d2") {  // Wandering Trader
            if (index == 0) {
                if (state_.player.scrap >= 18) {
                    state_.player.scrap -= 18;
                    auto cards = randomCardChoices(2);
                    queueOwnedCardsToDiscard(cards);
                    state_.statusLine = "Bought 2 cards. -18 credits.";
                } else {
                    state_.statusLine = "Not enough credits to buy cards.";
                    return;
                }
            } else if (index == 1) {
                // Delete 1 random card, gain 10 credits
                std::vector<std::string> allCards;
                allCards.insert(allCards.end(), state_.player.drawPile.begin(), state_.player.drawPile.end());
                allCards.insert(allCards.end(), state_.player.discardPile.begin(), state_.player.discardPile.end());
                if (!allCards.empty()) {
                    std::uniform_int_distribution<size_t> pick(0, allCards.size() - 1);
                    purgeCardFromDeck(allCards[pick(rng_)]);
                }
                state_.player.scrap += 10;
                state_.statusLine = "Sold a junk card. +10 credits.";
            } else {
                state_.statusLine = "Declined the trader's offer.";
            }
            SetEpilogue_D(g_sa_event_epilogue, rng_);
            finishCurrentNode();
            return;
        }

        if (eid == "event_d3") {  // Pirate Ambush
            if (index == 0) {
                if (state_.player.scrap >= 12) {
                    state_.player.scrap -= 12;
                    state_.statusLine = "Paid the ransom. -12 credits.";
                    finishCurrentNode();
                } else {
                    state_.statusLine = "Not enough credits to pay the ransom.";
                    return;
                }
            } else if (index == 1) {
                // Forced elite battle
                state_.activeNodeType = "elite";
                startBattle("elite");
                state_.statusLine = "You fight back against the pirates! Elite battle!";
            } else {
                changeHeat(6);
                state_.statusLine = "Escaped at full speed. +6 heat.";
                finishCurrentNode();
            }
            SetEpilogue_D(g_sa_event_epilogue, rng_);
            return;
        }

        if (eid == "event_d4") {  // Cryo Pod
            if (index == 0) {
                // Cards enhanced + higher elite encounter chance next
                std::string upgName, upgErr;
                upgradeRandomCard(CardUpgradeSource::EventReward, &upgName, &upgErr);
                upgradeRandomCard(CardUpgradeSource::EventReward, &upgName, &upgErr);
                g_sa_next_elite = true;
                state_.statusLine = "Woke the sleeper. Cards enhanced. Elite encounter chance increased next.";
            } else if (index == 1) {
                if (state_.player.scrap < 8) {
                    state_.statusLine = "Not enough credits to drain cryo energy.";
                    return;
                }
                state_.player.scrap -= 8;
                changeHeat(5);
                state_.player.hp = std::min(state_.player.maxHp, state_.player.hp + 6);
                state_.statusLine = "Drained cryo energy. +5 heat, +6 HP, -8 credits.";
            } else {
                state_.player.scrap += 18;
                auto cards = randomCardChoices(1);
                if (!cards.empty()) queueOwnedCardToDiscard(cards[0]);
                applyDamageToPlayer(8);
                state_.statusLine = "Scavenged supplies. +18 credits, gained a card, -8 HP.";
            }
            SetEpilogue_D(g_sa_event_epilogue, rng_);
            finishCurrentNode();
            return;
        }

        // ====== Game Opening Story ======

        if (eid == "game_intro") {
            g_sa_game_intro_pending = false;
            state_.encounter = {};
            state_.statusLine = "The Abyss awaits. Chart your first jump.";
            state_.page = AppPage::Map;
            resumePage_ = AppPage::Map;
            return;
        }

        // ====== Escape Route Prologue ======

        if (eid == "escape_prologue") {
            g_sa_escape_intro_pending = false;
            state_.statusLine = "Engines engaged. The escape corridor opens ahead.";
            finishCurrentNode();
            return;
        }

        // ====== Escape Route Events ======

        if (eid == "escape_1") {  // Federal Patrol Outpost
            if (index == 0) {
                if (state_.player.scrap >= 12) {
                    state_.player.scrap -= 12;
                    state_.statusLine = "Paid the transit fee. -12 credits.";
                    finishCurrentNode();
                } else {
                    state_.statusLine = "Not enough credits for the transit fee.";
                    return;
                }
            } else if (index == 1) {
                changeHeat(4);
                state_.statusLine = "Powered down and drifted through. +4 heat.";
                finishCurrentNode();
            } else {
                state_.activeNodeType = "battle";
                startBattle("battle");
                state_.statusLine = "Forced through the checkpoint! Battle!";
            }
            return;
        }

        if (eid == "escape_2") {  // Abandoned Smuggling Port
            if (index == 0) {
                state_.player.hp = std::min(state_.player.maxHp, state_.player.hp + 10);
                state_.statusLine = "Emergency repairs. +10 HP.";
            } else if (index == 1) {
                changeHeat(-6);
                state_.statusLine = "Cleaned heat exchangers. -6 heat.";
            } else {
                state_.player.scrap += 14;
                state_.statusLine = "Scavenged supplies. +14 credits.";
            }
            finishCurrentNode();
            return;
        }

        if (eid == "escape_3") {  // Deep Space Radiation Belt
            if (index == 0) {
                g_sa_skip_segments = 1;
                applyDamageToPlayer(6);
                state_.statusLine = "Full speed through the radiation belt. Skip 1 segment, -6 HP.";
            } else if (index == 1) {
                changeHeat(3);
                state_.statusLine = "Slow detour around the belt. +3 heat.";
            } else {
                if (state_.player.scrap >= 10) {
                    state_.player.scrap -= 10;
                    state_.player.maxHp += 2;
                    state_.player.hp += 2;
                    state_.statusLine = "Anti-rad shield active. +2 permanent max HP, -10 credits.";
                } else {
                    state_.statusLine = "Not enough credits for the anti-rad shield.";
                    return;
                }
            }
            finishCurrentNode();
            return;
        }

        if (eid == "escape_4") {  // Pirate Pursuit Ship
            if (index == 0) {
                if (state_.player.scrap >= 15) {
                    state_.player.scrap -= 15;
                    state_.statusLine = "Paid off the pirates. -15 credits.";
                    finishCurrentNode();
                } else {
                    state_.statusLine = "Not enough credits to pay off the pirates.";
                    return;
                }
            } else if (index == 1) {
                // Jettison cargo: delete 1 random card
                std::vector<std::string> allCards;
                allCards.insert(allCards.end(), state_.player.drawPile.begin(), state_.player.drawPile.end());
                allCards.insert(allCards.end(), state_.player.discardPile.begin(), state_.player.discardPile.end());
                if (!allCards.empty()) {
                    std::uniform_int_distribution<size_t> pick(0, allCards.size() - 1);
                    purgeCardFromDeck(allCards[pick(rng_)]);
                }
                state_.statusLine = "Jettisoned cargo. Lost 1 random card.";
                finishCurrentNode();
            } else {
                state_.activeNodeType = "elite";
                startBattle("elite");
                state_.statusLine = "Counter-attacking the pirates! Elite battle!";
            }
            return;
        }

        if (eid == "escape_5") {  // Drifting Escape Pod
            if (index == 0) {
                state_.player.hp = std::min(state_.player.maxHp, state_.player.hp + 8);
                state_.statusLine = "Took the medical kit. +8 HP.";
            } else if (index == 1) {
                state_.player.openingEnergyBonus += 2;
                state_.statusLine = "Drained fuel. +2 initial energy next battle.";
            } else {
                state_.player.nextBattleHeal += 3;
                state_.statusLine = "Took the emergency chip. +3 HP recovery next battle.";
            }
            finishCurrentNode();
            return;
        }

        if (eid == "escape_6") {  // Unstable Jump Point
            if (index == 0) {
                g_sa_skip_segments = 2;
                changeHeat(7);
                state_.statusLine = "Forced the jump. Skip 2 segments. +7 heat.";
            } else if (index == 1) {
                if (state_.player.scrap >= 8) {
                    state_.player.scrap -= 8;
                    changeHeat(-5);
                    state_.statusLine = "Stabilized coordinates. -5 heat, -8 credits.";
                } else {
                    state_.statusLine = "Not enough credits to stabilize.";
                    return;
                }
            } else {
                auto cards = randomCardChoices(1);
                if (!cards.empty()) queueOwnedCardToDiscard(cards[0]);
                applyDamageToPlayer(5);
                state_.statusLine = "Abandoned the jump. Gained a card, -5 HP.";
            }
            finishCurrentNode();
            return;
        }

        if (eid == "escape_7") {  // Star Abyss Edge Barrier
            if (index == 0) {
                changeHeat(6);
                state_.statusLine = "Energy breach through the barrier. +6 heat.";
            } else if (index == 1) {
                if (state_.player.scrap >= 18) {
                    state_.player.scrap -= 18;
                    state_.statusLine = "Spent supplies to breach the barrier. -18 credits.";
                } else {
                    state_.statusLine = "Not enough credits to breach.";
                    return;
                }
            } else {
                applyDamageToPlayer(5);
                state_.statusLine = "Found a gap. Lost 5 HP squeezing through.";
            }
            finishCurrentNode();
            return;
        }

        if (eid == "escape_8") {  // Temporary Safe Station
            if (index == 0) {
                if (state_.player.scrap >= 12) {
                    state_.player.scrap -= 12;
                    state_.player.hp = std::min(state_.player.maxHp, state_.player.hp + 5);
                    state_.statusLine = "Repaired hull. +5 HP, -12 credits.";
                } else {
                    state_.statusLine = "Not enough credits for repairs.";
                    return;
                }
            } else if (index == 1) {
                state_.player.openingEnergyBonus += 1;
                changeHeat(2);
                state_.statusLine = "Optimized engines. +1 initial energy, +2 heat.";
            } else {
                // Delete 1 low-quality card, lose 4 HP
                std::vector<std::string> lowCards;
                for (const auto& cr : state_.player.drawPile) {
                    const CardDef* def = cardById(baseCardId(cr));
                    if (def && def->rarity == CardRarity::Common) lowCards.push_back(cr);
                }
                for (const auto& cr : state_.player.discardPile) {
                    const CardDef* def = cardById(baseCardId(cr));
                    if (def && def->rarity == CardRarity::Common) lowCards.push_back(cr);
                }
                if (!lowCards.empty()) {
                    std::uniform_int_distribution<size_t> pick(0, lowCards.size() - 1);
                    purgeCardFromDeck(lowCards[pick(rng_)]);
                }
                applyDamageToPlayer(4);
                state_.statusLine = "Removed junk cards. -4 HP.";
            }
            finishCurrentNode();
            return;
        }

        // ====== Truth Route Events (post-battle narrative) ======

        if (eid == "truth_1") {  // Creation Memory Fragment
            if (index == 0) {
                std::string upgName, upgErr;
                upgradeRandomCard(CardUpgradeSource::EventReward, &upgName, &upgErr);
                upgradeRandomCard(CardUpgradeSource::EventReward, &upgName, &upgErr);
                state_.statusLine = "Merged memories. 2 cards upgraded.";
            } else if (index == 1) {
                state_.player.maxHp += 6;
                state_.player.hp += 6;
                state_.statusLine = "Extracted energy. Permanent max HP +6.";
            } else {
                auto cards = randomCardChoicesByRarity(1, CardRarity::Rare);
                if (!cards.empty()) {
                    queueOwnedCardToDiscard(cards[0]);
                }
                state_.statusLine = "Saved the log. Gained a rare card.";
            }
            finishCurrentNode();
            return;
        }

        if (eid == "truth_2") {  // Rogue Guardian AI
            if (index == 0) {
                const std::string relicId = randomRelic();
                if (!relicId.empty()) grantRelic(relicId);
                applyDamageToPlayer(8);
                state_.statusLine = "Force breakthrough. Gained a rare relic. -8 HP.";
            } else if (index == 1) {
                state_.player.openingEnergyBonus += 2;
                changeHeat(5);
                state_.statusLine = "Override permissions. +2 initial energy next battle, +5 heat.";
            } else {
                if (state_.player.scrap >= 12) {
                    state_.player.scrap -= 12;
                    state_.statusLine = "Sneaked around the AI. -12 credits.";
                } else {
                    state_.statusLine = "Not enough credits to bypass.";
                    return;
                }
            }
            finishCurrentNode();
            return;
        }

        if (eid == "truth_3") {  // First Captain's Consciousness
            if (index == 0) {
                auto cards = randomCardChoicesByRarity(1, CardRarity::Rare);
                if (!cards.empty()) {
                    queueOwnedCardToDiscard(cards[0]);
                }
                state_.statusLine = "Listened to the truth. Gained a rare card.";
            } else if (index == 1) {
                g_sa_permanent_shield += 4;
                state_.statusLine = "Gained ship permissions. +4 permanent opening shield.";
            } else {
                changeHeat(-10);
                state_.player.hp = std::min(state_.player.maxHp, state_.player.hp + 10);
                state_.statusLine = "Drained core energy. -10 heat, +10 HP.";
            }
            finishCurrentNode();
            return;
        }

        if (eid == "truth_4") {  // Spatial Collapse Point
            if (index == 0) {
                changeHeat(8);
                state_.statusLine = "Engines full. +8 heat.";
            } else if (index == 1) {
                if (state_.player.scrap >= 20) {
                    state_.player.scrap -= 20;
                    state_.statusLine = "Dumped supplies to brace. -20 credits.";
                } else {
                    state_.statusLine = "Not enough credits.";
                    return;
                }
            } else {
                applyDamageToPlayer(9);
                state_.statusLine = "Emergency dodge. -9 HP.";
            }
            finishCurrentNode();
            return;
        }

        if (eid == "truth_5") {  // Star Abyss Birth Truth
            if (index == 0) {
                // All cards permanently enhanced (upgrade all non-upgraded)
                std::vector<std::string> allCards;
                allCards.insert(allCards.end(), state_.player.drawPile.begin(), state_.player.drawPile.end());
                allCards.insert(allCards.end(), state_.player.discardPile.begin(), state_.player.discardPile.end());
                for (const auto& cr : allCards) {
                    if (cardUpgradeLevel(cr) == 0) {
                        std::string err;
                        upgradeCard(cr, CardUpgradeSource::EventReward, &err);
                    }
                }
                state_.statusLine = "Accepted the truth. All cards permanently enhanced.";
            } else {
                // Gain ultimate relic, lose 10 HP
                const std::string relicId = randomRelic();
                if (!relicId.empty()) grantRelic(relicId);
                applyDamageToPlayer(10);
                state_.statusLine = "Extracted the origin. Gained ultimate relic, -10 HP.";
            }
            finishCurrentNode();
            return;
        }

        // Fallback: unknown event
        state_.statusLine = "The signal fades. Nothing to be done.";
        finishCurrentNode();
    }
}
