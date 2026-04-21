#include "engine/game_content.h"
#include "engine/game_card_json.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace {

constexpr const char* kObtainedCardsRegistryPath = "save/obtained_cards.txt";

template <typename T>
void ShuffleVector(std::vector<T>& values, std::mt19937& rng) {
    std::shuffle(values.begin(), values.end(), rng);
}

bool HasString(const std::vector<std::string>& values, const std::string& needle) {
    return std::find(values.begin(), values.end(), needle) != values.end();
}

std::string TrimAscii(const std::string& value) {
    size_t begin = 0;
    while (begin < value.size() && (value[begin] == ' ' || value[begin] == '\t' || value[begin] == '\r')) {
        ++begin;
    }
    size_t end = value.size();
    while (end > begin && (value[end - 1] == ' ' || value[end - 1] == '\t' || value[end - 1] == '\r')) {
        --end;
    }
    return value.substr(begin, end - begin);
}

}  // namespace

void GameEngine::initializeData() {
    std::string error;
    const std::string cards_path = ResolveCardsJsonPath();
    if (!LoadCardsFromJsonFile(cards_path, &cardPool_, &configuredUpgradePool_, &error)) {
        std::cerr << "Failed to load card data from " << cards_path << ": " << error << "\n";
        cardPool_.clear();
        configuredUpgradePool_.clear();
    } else {
        std::sort(cardPool_.begin(), cardPool_.end(), [](const CardDef& lhs, const CardDef& rhs) {
            return lhs.id < rhs.id;
        });
        const auto duplicate = std::adjacent_find(cardPool_.begin(), cardPool_.end(), [](const CardDef& lhs, const CardDef& rhs) {
            return lhs.id == rhs.id;
        });
        if (duplicate != cardPool_.end()) {
            std::cerr << "Duplicate card id in " << cards_path << ": " << duplicate->id << "\n";
            cardPool_.clear();
            configuredUpgradePool_.clear();
        }
    }

    enemyPool_ = {
        {"pirate_needle", "Pirate Needle", "aggressive", 34, false, false,
         {{"attack", "Needle Burst", 10}, {"heat", "Tracer Burn", 4}, {"attack", "Afterburn Dash", 11}}, 9, 1, 1},
        {"scrapbreaker_tug", "Scrapbreaker Tug", "balanced", 40, false, false,
         {{"shield", "Scrap Bulkhead", 8}, {"attack", "Tow Ram", 12}, {"charge", "Brace Thrusters", 6}}, 10, 1, 1},
        {"signal_leech_mk2", "Signal Leech Mk-II", "control", 36, false, false,
         {{"emp", "Pulse Jam", 1}, {"heat", "Heat Spike", 4}, {"attack", "Signal Lash", 9}}, 8, 1, 1},
        {"rogue_miner_colossus", "Rogue Miner Colossus", "heavy", 48, true, false,
         {{"charge", "Core Drill Spin-Up", 8}, {"attack", "Seismic Beam", 16}, {"attack", "Shrapnel Wash", 11}, {"shield", "Ore Plating", 8}}, 11, 1, 1},
        {"dread_frigate_omega", "Dread Frigate Omega", "boss", 72, false, true,
         {{"shield", "Aegis Drum", 10}, {"attack", "Broadside", 14}, {"heat", "Reactor Flare", 4}, {"attack", "Missile Rain", 18}}, 12, 1, 1},

        {"cinder_hauler", "Cinder Hauler", "balanced", 62, false, false,
         {{"heat", "Slag Exhaust", 4}, {"attack", "Cargo Swing", 14}, {"shield", "Insulated Plating", 8}}, 11, 2, 2},
        {"furnace_tender", "Furnace Tender", "defensive", 70, false, false,
         {{"shield", "Thermal Screen", 10}, {"heat", "Boiler Surge", 4}, {"shield", "Coolant Wall", 9}, {"attack", "Pipe Lash", 13}}, 13, 2, 2},
        {"ember_rack_skiff", "Ember Rack Skiff", "aggressive", 64, false, false,
         {{"attack", "Incendiary Burst", 13}, {"heat", "Ash Injection", 5}, {"attack", "Rack Detonation", 15}}, 10, 2, 2},
        {"slag_tribunal", "Slag Tribunal", "elite", 82, true, false,
         {{"shield", "Verdict Barrier", 10}, {"heat", "Molten Sentence", 5}, {"attack", "Execution Lance", 18}, {"emp", "Suppression Pulse", 1}}, 12, 2, 2},
        {"furnace_carrier", "The Furnace Carrier", "boss", 118, false, true,
         {{"heat", "Heat Surge", 5}, {"attack", "Forge Cannon", 16}, {"shield", "Smelter Shell", 12}, {"attack", "Furnace Lash", 20}}, 14, 2, 2},

        {"phase_skewer", "Phase Skewer", "aggressive", 88, false, false,
         {{"attack", "Phase Jab", 18}, {"heat", "Warp Friction", 4}, {"attack", "Skewer Drive", 20}}, 14, 3, 3},
        {"null_choir", "Null Choir", "control", 96, false, false,
         {{"emp", "Silence Vector", 1}, {"heat", "Null Resonance", 5}, {"attack", "Echo Collapse", 17}, {"emp", "Static Break", 1}}, 15, 3, 3},
        {"core_warder", "Core Warder", "defensive", 108, false, false,
         {{"shield", "Core Guard", 12}, {"attack", "Beam Sweep", 16}, {"shield", "Anchor Field", 10}, {"heat", "Containment Leak", 4}}, 16, 3, 3},
        {"reality_harrower", "Reality Harrower", "chaotic", 116, true, false,
         {{"attack", "Rift Slash", 19}, {"heat", "Entropy Bloom", 5}, {"shield", "Fracture Veil", 12}, {"emp", "Static Snare", 1}}, 15, 3, 3},
        {"abyss_core_sentinel", "Abyss Core Sentinel", "boss", 166, false, true,
         {{"shield", "Core Barrier", 14}, {"attack", "Beam Lance", 21}, {"emp", "Reality Static", 1}, {"heat", "Abyss Pressure", 5}, {"attack", "Collapse Verdict", 24}}, 16, 3, 3},

        {"the_pale_broker", "The Pale Broker", "boss", 86, false, true,
         {{"emp", "Contract Lock", 1}, {"shield", "Broker Screen", 10}, {"attack", "Asset Seizure", 15}, {"heat", "Salvage Burn", 4}, {"attack", "Liquidation Shot", 18}}, 11, 1, 1},
        {"saint_of_cinders", "Saint of Cinders", "boss", 132, false, true,
         {{"shield", "Kiln Mantle", 12}, {"heat", "Censer Exhaust", 5}, {"charge", "Sanctified Pressure", 8}, {"attack", "Foundry Verdict", 21}, {"attack", "Ashen Choir", 17}}, 15, 2, 2},
        {"the_choir_beneath", "The Choir Beneath", "chaotic", 178, false, true,
         {{"emp", "Mute Pulse", 1}, {"heat", "Resonance Fever", 5}, {"shield", "Echo Veil", 13}, {"attack", "Hymn of Collapse", 22}, {"emp", "Silence Cascade", 1}}, 17, 3, 3},
        {"salvager_zero", "Salvager Zero", "boss", 196, false, true,
         {{"shield", "Prototype Shell", 14}, {"heat", "Reactor Overwrite", 5}, {"attack", "Salvage Breaker", 23}, {"emp", "System Refit Pulse", 1}, {"charge", "Zero-State Windup", 10}, {"attack", "Origin Collapse", 28}}, 18, 3, 3},
    };

    ships_ = {
        {"vanguard", "Vanguard", 50, 3, 10, {"vg_02", "vg_04"}, {"reactor_core"}},
        {"drift_fox", "Drift Fox", 47, 3, 10, {"df_01", "df_05"}, {}},
        {"iron_bastion", "Iron Bastion", 58, 3, 10, {"ib_01", "ib_02"}, {"shield_matrix"}},
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

bool GameEngine::hasObtainedCard(const std::string& cardId) const {
    return HasString(obtainedCardIds_, baseCardId(cardId));
}

void GameEngine::recordObtainedCard(const std::string& cardRef) {
    const std::string baseId = baseCardId(cardRef);
    if (baseId.empty() || cardById(baseId) == nullptr || hasObtainedCard(baseId)) {
        return;
    }
    obtainedCardIds_.push_back(baseId);
    std::sort(obtainedCardIds_.begin(), obtainedCardIds_.end());
    const auto unique_end = std::unique(obtainedCardIds_.begin(), obtainedCardIds_.end());
    obtainedCardIds_.erase(unique_end, obtainedCardIds_.end());
    saveObtainedCardRegistry(nullptr);
}

void GameEngine::recordCurrentOwnedCards() {
    bool changed = false;
    const auto collect = [&](const std::vector<std::string>& pile) {
        for (const std::string& ref : pile) {
            const std::string baseId = baseCardId(ref);
            if (baseId.empty() || cardById(baseId) == nullptr || hasObtainedCard(baseId)) {
                continue;
            }
            obtainedCardIds_.push_back(baseId);
            changed = true;
        }
    };
    collect(state_.player.drawPile);
    collect(state_.player.hand);
    collect(state_.player.discardPile);
    if (!changed) {
        return;
    }
    std::sort(obtainedCardIds_.begin(), obtainedCardIds_.end());
    const auto unique_end = std::unique(obtainedCardIds_.begin(), obtainedCardIds_.end());
    obtainedCardIds_.erase(unique_end, obtainedCardIds_.end());
    saveObtainedCardRegistry(nullptr);
}

void GameEngine::queueOwnedCardToDiscard(const std::string& cardRef) {
    if (cardRef.empty()) {
        return;
    }
    recordObtainedCard(cardRef);
    state_.player.discardPile.push_back(cardRef);
}

void GameEngine::queueOwnedCardsToDiscard(const std::vector<std::string>& cardRefs) {
    for (const std::string& cardRef : cardRefs) {
        queueOwnedCardToDiscard(cardRef);
    }
}

void GameEngine::loadObtainedCardRegistry() {
    obtainedCardIds_.clear();
    std::ifstream input(kObtainedCardsRegistryPath);
    if (!input) {
        return;
    }

    std::string line;
    while (std::getline(input, line)) {
        const std::string cardId = TrimAscii(line);
        if (cardId.empty() || cardById(cardId) == nullptr || hasObtainedCard(cardId)) {
            continue;
        }
        obtainedCardIds_.push_back(cardId);
    }
    std::sort(obtainedCardIds_.begin(), obtainedCardIds_.end());
    const auto unique_end = std::unique(obtainedCardIds_.begin(), obtainedCardIds_.end());
    obtainedCardIds_.erase(unique_end, obtainedCardIds_.end());
}

bool GameEngine::saveObtainedCardRegistry(std::string* error) const {
    namespace fs = std::filesystem;
    std::error_code ec;
    fs::create_directories("save", ec);

    std::ofstream output(kObtainedCardsRegistryPath, std::ios::trunc);
    if (!output) {
        if (error != nullptr) {
            *error = "Unable to write obtained card registry.";
        }
        return false;
    }
    for (const std::string& cardId : obtainedCardIds_) {
        output << cardId << "\n";
    }
    if (error != nullptr) {
        *error = "";
    }
    return true;
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
            recordObtainedCard(card.id);
        }
    }
    for (const std::string& card : ship.extraCards) {
        player.drawPile.push_back(card);
        recordObtainedCard(card);
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
