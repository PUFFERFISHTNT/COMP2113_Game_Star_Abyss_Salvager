#include "engine/game_content.h"
#include "engine/game_card_json.h"

#include <algorithm>
#include <iostream>

namespace {

template <typename T>
void ShuffleVector(std::vector<T>& values, std::mt19937& rng) {
    std::shuffle(values.begin(), values.end(), rng);
}

bool HasString(const std::vector<std::string>& values, const std::string& needle) {
    return std::find(values.begin(), values.end(), needle) != values.end();
}

}  // namespace

void GameEngine::initializeData() {
    std::string error;
    if (!LoadCardsFromJsonFile("data/cards.json", &cardPool_, &error)) {
        std::cerr << "Failed to load card data from data/cards.json: " << error << "\n";
        cardPool_.clear();
    } else {
        std::sort(cardPool_.begin(), cardPool_.end(), [](const CardDef& lhs, const CardDef& rhs) {
            return lhs.id < rhs.id;
        });
        const auto duplicate = std::adjacent_find(cardPool_.begin(), cardPool_.end(), [](const CardDef& lhs, const CardDef& rhs) {
            return lhs.id == rhs.id;
        });
        if (duplicate != cardPool_.end()) {
            std::cerr << "Duplicate card id in data/cards.json: " << duplicate->id << "\n";
            cardPool_.clear();
        }
    }

    enemyPool_ = {
        {"pirate_scout", "Pirate Scout", "aggressive", 20, false, false, {{"attack", "Twin Lasers", 6}, {"attack", "Afterburner Strike", 7}, {"shield", "Brace", 4}}},
        {"raider_frigate", "Raider Frigate", "balanced", 26, false, false, {{"attack", "Cannon Burst", 7}, {"shield", "Scrap Plating", 5}, {"attack", "Heavy Salvo", 10}}},
        {"scrap_drone", "Scrap Drone", "defensive", 22, false, false, {{"shield", "Shunt Shield", 6}, {"attack", "Needle Volley", 4}, {"attack", "Saw Drone", 5}}},
        {"rogue_miner", "Rogue Miner", "heavy", 28, false, false, {{"charge", "Charge Drill", 3}, {"attack", "Mining Beam", 12}, {"attack", "Shrapnel Burst", 6}}},
        {"signal_leech", "Signal Leech", "control", 24, false, false, {{"emp", "EMP Ripple", 1}, {"heat", "Heat Spike", 3}, {"attack", "Signal Lash", 7}}},
        {"war_corvette", "War Corvette", "elite", 38, true, false, {{"attack", "Barrage", 9}, {"shield", "Shield Wall", 8}, {"attack", "Heavy Salvo", 14}}},
        {"black_market_hunter", "Black Market Hunter", "elite", 36, true, false, {{"emp", "Jam Pulse", 1}, {"attack", "Hunter Volley", 11}, {"heat", "Tracer Burn", 4}}},
        {"corrupted_ai_ship", "Corrupted AI Ship", "chaotic", 40, true, false, {{"attack", "Glitch Strike", 8}, {"shield", "Phase Barrier", 8}, {"heat", "Entropy Pulse", 4}}},
        {"dread_frigate_omega", "Dread Frigate Omega", "boss", 56, false, true, {{"shield", "Aegis Drum", 10}, {"attack", "Broadside", 13}, {"attack", "Missile Rain", 16}}},
        {"furnace_carrier", "The Furnace Carrier", "boss", 60, false, true, {{"heat", "Heat Surge", 4}, {"attack", "Forge Cannon", 10}, {"attack", "Furnace Lash", 14}}},
        {"abyss_core_sentinel", "Abyss Core Sentinel", "boss", 72, false, true, {{"shield", "Core Barrier", 12}, {"attack", "Beam Lance", 16}, {"emp", "Reality Static", 1}}},
    };

    ships_ = {
        {"vanguard", "Vanguard", 40, 3, 10, {}, {"reactor_core"}},
        {"drift_fox", "Drift Fox", 34, 4, 9, {"scan_pulse"}, {}},
        {"iron_bastion", "Iron Bastion", 46, 3, 11, {"emergency_barrier"}, {"shield_matrix"}},
    };

    relicPool_ = {
        {"auto_cooler", "Auto-Cooler", "Reduce 1 heat at end of each turn"},
        {"reactor_core", "Reactor Core", "Gain +1 energy on the first turn of each battle"},
        {"salvage_scanner", "Salvage Scanner", "Increase credit gains from salvage and battle"},
        {"emergency_battery", "Emergency Battery", "Survive one fatal hit with 1 HP once per run"},
        {"reinforced_hull", "Reinforced Hull", "Increase max HP by 8"},
        {"shield_matrix", "Shield Matrix", "Start each battle with 4 shield"},
        {"heat_regulator", "Heat Regulator", "Increase max heat by 2"},
    };
}

void GameEngine::setupPlayerForShip(const ShipTemplate& ship, int difficulty) {
    PlayerState player;
    player.shipId = ship.id;
    player.shipName = ship.name;
    player.maxHp = ship.maxHp;
    player.maxEnergy = ship.maxEnergy;
    player.maxHeat = ship.maxHeat;

    if (difficulty == 0) {
        player.maxHp += 6;
        player.maxHeat += 2;
    } else if (difficulty == 2) {
        player.maxHp = std::max(24, player.maxHp - 5);
        player.maxHeat = std::max(7, player.maxHeat - 1);
    }

    player.hp = player.maxHp;
    player.energy = player.maxEnergy;
    player.drawPile.clear();
    for (const CardDef& card : cardPool_) {
        for (int copy = 0; copy < card.starterCopies; ++copy) {
            player.drawPile.push_back(card.id);
        }
    }
    for (const std::string& card : ship.extraCards) {
        player.drawPile.push_back(card);
    }
    ShuffleVector(player.drawPile, rng_);

    state_.player = player;
    for (const std::string& relic : ship.relics) {
        grantRelic(relic);
    }
}

void GameEngine::grantRelic(const std::string& relicId) {
    if (relicId.empty() || hasRelic(relicId)) {
        return;
    }
    state_.player.relics.push_back(relicId);
    if (relicId == "reinforced_hull") {
        state_.player.maxHp += 8;
        state_.player.hp += 8;
    } else if (relicId == "heat_regulator") {
        state_.player.maxHeat += 2;
    }
}

bool GameEngine::hasRelic(const std::string& relicId) const {
    return HasString(state_.player.relics, relicId);
}
