#include "tui_game.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <utility>

namespace {

constexpr int kBlackMarketCardPrice = 28;
constexpr int kBlackMarketPurgePrice = 20;
constexpr size_t kBlackMarketPurgePageSize = 4;

std::vector<std::string> Split(const std::string& line, char delimiter) {
    std::vector<std::string> parts;
    std::stringstream stream(line);
    std::string item;
    while (std::getline(stream, item, delimiter)) {
        parts.push_back(item);
    }
    return parts;
}

int ToInt(const std::string& value, int fallback = 0) {
    try {
        return std::stoi(value);
    } catch (...) {
        return fallback;
    }
}

std::string BoolToken(bool value) {
    return value ? "1" : "0";
}

bool HasString(const std::vector<std::string>& values, const std::string& needle) {
    return std::find(values.begin(), values.end(), needle) != values.end();
}

std::string PageToString(AppPage page) {
    switch (page) {
    case AppPage::MainMenu:
        return "MainMenu";
    case AppPage::Setup:
        return "Setup";
    case AppPage::Map:
        return "Map";
    case AppPage::Encounter:
        return "Encounter";
    case AppPage::Battle:
        return "Battle";
    case AppPage::Reward:
        return "Reward";
    case AppPage::Summary:
        return "Summary";
    case AppPage::Info:
        return "Info";
    }
    return "MainMenu";
}

AppPage PageFromString(const std::string& value) {
    if (value == "Setup") {
        return AppPage::Setup;
    }
    if (value == "Map") {
        return AppPage::Map;
    }
    if (value == "Encounter") {
        return AppPage::Encounter;
    }
    if (value == "Battle") {
        return AppPage::Battle;
    }
    if (value == "Reward") {
        return AppPage::Reward;
    }
    if (value == "Summary") {
        return AppPage::Summary;
    }
    if (value == "Info") {
        return AppPage::Info;
    }
    return AppPage::MainMenu;
}

template <typename T>
void ShuffleVector(std::vector<T>& values, std::mt19937& rng) {
    std::shuffle(values.begin(), values.end(), rng);
}

std::string DifficultyLabel(int difficulty) {
    if (difficulty == 0) {
        return "Easy";
    }
    if (difficulty == 2) {
        return "Hard";
    }
    return "Normal";
}

}  // namespace

GameEngine::GameEngine() : rng_(std::random_device{}()) {
    initializeData();
    resetRunState();
    fillHelpInfo();
}

const RunState& GameEngine::state() const {
    return state_;
}

void GameEngine::initializeData() {
    cardPool_.clear();
    auto addCard = [&](const std::string& id,
                       const std::string& name,
                       const std::string& type,
                       const std::string& description,
                       int cost) -> CardDef& {
        cardPool_.push_back(CardDef{});
        CardDef& card = cardPool_.back();
        card.id = id;
        card.name = name;
        card.type = type;
        card.description = description;
        card.cost = cost;
        return card;
    };

    addCard("laser_burst", "Laser Burst", "Attack", "Deal 6 damage.", 1).damage = 6;
    addCard("raise_shield", "Raise Shield", "Defense", "Gain 5 shield.", 1).shield = 5;
    addCard("coolant_flush", "Coolant Flush", "Utility", "Reduce 3 heat.", 1).heat = -3;
    addCard("quick_patch", "Quick Patch", "Utility", "Recover 4 HP.", 1).heal = 4;
    {
        CardDef& card = addCard("overclock", "Overclock", "Utility", "Gain 1 energy. Gain 1 heat.", 0);
        card.energy = 1;
        card.heat = 1;
    }
    addCard("scan_pulse", "Scan Pulse", "Utility", "Draw 2 cards.", 1).draw = 2;
    addCard("missile_pod", "Missile Pod", "Attack", "Deal 11 damage.", 2).damage = 11;
    {
        CardDef& card = addCard("piercing_shot", "Piercing Shot", "Attack", "Deal 5 damage. Ignore shield.", 1);
        card.damage = 5;
        card.ignoreShield = true;
    }
    {
        CardDef& card = addCard("burst_fire", "Burst Fire", "Attack", "Deal 4 damage twice.", 2);
        card.damage = 4;
        card.multiHit = 2;
    }
    addCard("emergency_barrier", "Emergency Barrier", "Defense", "Gain 10 shield.", 2).shield = 10;
    {
        CardDef& card = addCard("reactor_boost", "Reactor Boost", "Utility", "Next turn, gain 2 extra energy.", 1);
        card.nextTurnEnergy = 2;
    }
    {
        CardDef& card = addCard("reroute_power", "Reroute Power", "Utility", "Draw 1 card and gain 1 shield.", 1);
        card.draw = 1;
        card.shield = 1;
    }
    {
        CardDef& card = addCard("stabilizer", "Stabilizer", "Utility", "Reduce 4 heat. Draw 1 card.", 1);
        card.draw = 1;
        card.heat = -4;
    }
    addCard("salvage_drone", "Salvage Drone", "Utility", "If you win this battle, gain +6 credits.", 1).salvageBonus = 6;
    {
        CardDef& card = addCard("plasma_lance", "Plasma Lance", "Attack", "Deal 14 damage. Gain 2 heat.", 2);
        card.damage = 14;
        card.heat = 2;
    }
    {
        CardDef& card = addCard("emp_pulse", "EMP Pulse", "Control", "Deal 4 damage. Enemy's next 2 attacks lose 2 damage.", 2);
        card.damage = 4;
        card.weakenTurns = 2;
    }
    {
        CardDef& card = addCard("hack_jammer", "Hack Jammer", "Control", "Remove enemy shield. They cannot gain shield for 2 turns.", 2);
        card.removeEnemyShield = 999;
        card.shieldLockTurns = 2;
    }
    {
        CardDef& card = addCard("chain_pulse", "Chain Pulse", "Attack", "Deal 5 damage. Deal +3 if another attack was played this turn.", 1);
        card.damage = 5;
        card.comboBonus = 3;
    }
    addCard("heat_sink", "Heat Sink", "Utility", "Cut current heat in half.", 1).halveHeat = true;
    {
        CardDef& card = addCard("system_flush", "System Flush", "Utility", "Reduce 5 heat. Draw 2 cards.", 2);
        card.draw = 2;
        card.heat = -5;
    }
    addCard("shield_converter", "Shield Converter", "Utility", "Deal damage equal to your current shield, then lose that shield.", 1).shieldToDamage = true;
    {
        CardDef& card = addCard("overload_cannon", "Overload Cannon", "Attack", "Deal 22 damage. Gain 4 heat.", 3);
        card.damage = 22;
        card.heat = 4;
    }
    {
        CardDef& card = addCard("reactor_surge", "Reactor Surge", "Risk", "Gain 2 energy. Gain 3 heat.", 0);
        card.energy = 2;
        card.heat = 3;
    }
    addCard("nova_shield", "Nova Shield", "Defense", "Gain 14 shield.", 2).shield = 14;
    addCard("target_lock", "Target Lock", "Control", "Your next attack card deals double damage.", 1).attackMultiplier = 2;
    {
        CardDef& card = addCard("emergency_vent", "Emergency Vent", "Utility", "Remove all heat. Lose 2 HP.", 0);
        card.clearHeat = true;
        card.selfDamage = 2;
    }
    addCard("blind_jump_protocol", "Blind Jump Protocol", "Risk", "Trigger one strong but unpredictable effect.", 2).randomEffect = true;

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

void GameEngine::resetRunState() {
    state_ = RunState{};
    state_.difficulty = 1;
    state_.page = AppPage::MainMenu;
    state_.statusLine = "Abyss link stable. Choose a route.";
    resumePage_ = AppPage::Map;
}

void GameEngine::startNewRun(int difficulty, const std::string& shipId) {
    resetRunState();
    const ShipTemplate* ship = shipTemplateById(shipId);
    const ShipTemplate& chosenShip = ship ? *ship : ships_.front();

    state_.hasRun = true;
    state_.difficulty = difficulty;
    state_.sector = 1;
    state_.depth = 0;
    state_.nodesPerSector = 3;
    setupPlayerForShip(chosenShip, difficulty);
    generateRouteOptions();
    state_.page = AppPage::Map;
    resumePage_ = AppPage::Map;
    state_.statusLine = "Run initialized. Chart your first jump.";
}

bool GameEngine::saveToFile(const std::string& path, std::string* error) const {
    if (!state_.hasRun && state_.page != AppPage::Summary) {
        if (error != nullptr) {
            *error = "No active run to save.";
        }
        return false;
    }

    std::ofstream output(path.c_str(), std::ios::trunc);
    if (!output) {
        if (error != nullptr) {
            *error = "Unable to open save file for writing.";
        }
        return false;
    }

    output << "VERSION|3\n";
    output << "PAGE|" << PageToString(state_.page) << "\n";
    output << "DIFFICULTY|" << state_.difficulty << "\n";
    output << "SECTOR|" << state_.sector << "\n";
    output << "DEPTH|" << state_.depth << "\n";
    output << "NODES|" << state_.nodesPerSector << "\n";
    output << "BATTLETURN|" << state_.battleTurn << "\n";
    output << "FIRSTTURN|" << BoolToken(state_.firstTurnOfBattle) << "\n";
    output << "PENDINGSECTOR|" << BoolToken(state_.pendingSectorAdvance) << "\n";
    output << "PENDINGVICTORY|" << BoolToken(state_.pendingVictory) << "\n";
    output << "ACTIVE|" << state_.activeNodeType << "|" << state_.activeNodeName << "\n";
    output << "STATUS|" << state_.statusLine << "\n";

    const PlayerState& player = state_.player;
    output << "PLAYER|" << player.shipId << "|" << player.shipName << "|"
           << player.hp << "|" << player.maxHp << "|" << player.shield << "|"
           << player.energy << "|" << player.maxEnergy << "|" << player.heat << "|"
           << player.maxHeat << "|" << player.scrap << "|" << player.nextTurnEnergy << "|"
           << player.openingEnergyBonus << "|" << player.openingShieldBonus << "|"
           << player.nextAttackMultiplier << "|" << player.salvageBonus << "|"
           << player.attacksPlayedThisTurn << "|" << BoolToken(player.emergencyBatteryUsed) << "\n";

    output << "RELICS|" << player.relics.size() << "\n";
    for (const std::string& relic : player.relics) {
        output << "RELIC|" << relic << "\n";
    }

    output << "DRAW|" << player.drawPile.size() << "\n";
    for (const std::string& card : player.drawPile) {
        output << "CARD|" << card << "\n";
    }
    output << "HAND|" << player.hand.size() << "\n";
    for (const std::string& card : player.hand) {
        output << "CARD|" << card << "\n";
    }
    output << "DISCARD|" << player.discardPile.size() << "\n";
    for (const std::string& card : player.discardPile) {
        output << "CARD|" << card << "\n";
    }

    const EnemyState& enemy = state_.enemy;
    output << "ENEMY|" << enemy.enemyId << "|" << enemy.name << "|" << enemy.behavior << "|"
           << enemy.hp << "|" << enemy.maxHp << "|" << enemy.shield << "|"
           << enemy.intentKind << "|" << enemy.intentLabel << "|" << enemy.intentValue << "|"
           << enemy.patternIndex << "|" << enemy.weakenTurns << "|" << enemy.shieldLockTurns << "\n";

    output << "ROUTE|" << state_.routeOptions.size() << "\n";
    for (const NodeOption& option : state_.routeOptions) {
        output << "NODE|" << option.typeId << "|" << option.name << "|" << option.detail
               << "|" << option.danger << "\n";
    }

    output << "ENCOUNTER|" << state_.encounter.typeId << "|" << state_.encounter.encounterId
           << "|" << state_.encounter.title << "|" << state_.encounter.description << "|"
           << state_.encounter.choices.size() << "\n";
    for (size_t i = 0; i < state_.encounter.choices.size(); ++i) {
        const std::string detail = i < state_.encounter.details.size() ? state_.encounter.details[i] : "";
        const std::string payloadId = i < state_.encounter.payloadIds.size() ? state_.encounter.payloadIds[i] : "";
        const int payloadValue = i < state_.encounter.payloadValues.size() ? state_.encounter.payloadValues[i] : 0;
        output << "CHOICE|" << state_.encounter.choices[i] << "|" << detail << "|" << payloadId
               << "|" << payloadValue << "\n";
    }

    output << "REWARD|" << state_.reward.banner << "|" << state_.reward.scrap << "|"
           << state_.reward.relicId << "|" << state_.reward.cardIds.size() << "\n";
    for (const std::string& card : state_.reward.cardIds) {
        output << "REWARDCARD|" << card << "\n";
    }

    output << "INFO|" << state_.info.title << "|" << state_.info.footer << "|"
           << state_.info.lines.size() << "\n";
    for (const std::string& line : state_.info.lines) {
        output << "INFOLINE|" << line << "\n";
    }

    output << "BATTLELOG|" << state_.battleLog.size() << "\n";
    for (const std::string& line : state_.battleLog) {
        output << "LOG|" << line << "\n";
    }

    output << "ANIMATION|" << state_.animation.kind << "|" << state_.animation.label << "|"
           << state_.animation.magnitude << "|" << BoolToken(state_.animation.playerToEnemy) << "|"
           << state_.animation.tick << "|" << state_.animation.duration << "|"
           << BoolToken(state_.animation.active) << "\n";

    if (error != nullptr) {
        *error = "";
    }
    return true;
}

bool GameEngine::loadFromFile(const std::string& path, std::string* error) {
    std::ifstream input(path.c_str());
    if (!input) {
        if (error != nullptr) {
            *error = "Save file not found.";
        }
        return false;
    }

    RunState loaded;
    loaded.page = AppPage::Map;
    std::string line;
    std::vector<std::string>* activePile = nullptr;

    while (std::getline(input, line)) {
        const std::vector<std::string> parts = Split(line, '|');
        if (parts.empty()) {
            continue;
        }

        const std::string& key = parts[0];
        if (key == "PAGE" && parts.size() >= 2) {
            loaded.page = PageFromString(parts[1]);
        } else if (key == "DIFFICULTY" && parts.size() >= 2) {
            loaded.difficulty = ToInt(parts[1], 1);
        } else if (key == "SECTOR" && parts.size() >= 2) {
            loaded.sector = ToInt(parts[1], 1);
        } else if (key == "DEPTH" && parts.size() >= 2) {
            loaded.depth = ToInt(parts[1], 0);
        } else if (key == "NODES" && parts.size() >= 2) {
            loaded.nodesPerSector = ToInt(parts[1], 3);
        } else if (key == "BATTLETURN" && parts.size() >= 2) {
            loaded.battleTurn = ToInt(parts[1], 0);
        } else if (key == "FIRSTTURN" && parts.size() >= 2) {
            loaded.firstTurnOfBattle = parts[1] == "1";
        } else if (key == "PENDINGSECTOR" && parts.size() >= 2) {
            loaded.pendingSectorAdvance = parts[1] == "1";
        } else if (key == "PENDINGVICTORY" && parts.size() >= 2) {
            loaded.pendingVictory = parts[1] == "1";
        } else if (key == "ACTIVE" && parts.size() >= 3) {
            loaded.activeNodeType = parts[1];
            loaded.activeNodeName = parts[2];
        } else if (key == "STATUS" && parts.size() >= 2) {
            loaded.statusLine = parts[1];
        } else if (key == "PLAYER" && parts.size() >= 16) {
            loaded.player.shipId = parts[1];
            loaded.player.shipName = parts[2];
            loaded.player.hp = ToInt(parts[3], 40);
            loaded.player.maxHp = ToInt(parts[4], 40);
            loaded.player.shield = ToInt(parts[5], 0);
            loaded.player.energy = ToInt(parts[6], 3);
            loaded.player.maxEnergy = ToInt(parts[7], 3);
            loaded.player.heat = ToInt(parts[8], 0);
            loaded.player.maxHeat = ToInt(parts[9], 10);
            loaded.player.scrap = ToInt(parts[10], 0);
            loaded.player.nextTurnEnergy = ToInt(parts[11], 0);
            if (parts.size() >= 18) {
                loaded.player.openingEnergyBonus = ToInt(parts[12], 0);
                loaded.player.openingShieldBonus = ToInt(parts[13], 0);
                loaded.player.nextAttackMultiplier = ToInt(parts[14], 1);
                loaded.player.salvageBonus = ToInt(parts[15], 0);
                loaded.player.attacksPlayedThisTurn = ToInt(parts[16], 0);
                loaded.player.emergencyBatteryUsed = parts[17] == "1";
            } else {
                loaded.player.nextAttackMultiplier = ToInt(parts[12], 1);
                loaded.player.salvageBonus = ToInt(parts[13], 0);
                loaded.player.attacksPlayedThisTurn = ToInt(parts[14], 0);
                loaded.player.emergencyBatteryUsed = parts[15] == "1";
            }
        } else if (key == "RELIC" && parts.size() >= 2) {
            loaded.player.relics.push_back(parts[1]);
        } else if (key == "DRAW") {
            loaded.player.drawPile.clear();
            activePile = &loaded.player.drawPile;
        } else if (key == "HAND") {
            loaded.player.hand.clear();
            activePile = &loaded.player.hand;
        } else if (key == "DISCARD") {
            loaded.player.discardPile.clear();
            activePile = &loaded.player.discardPile;
        } else if (key == "CARD" && parts.size() >= 2 && activePile != nullptr) {
            activePile->push_back(parts[1]);
        } else if (key == "ENEMY" && parts.size() >= 12) {
            loaded.enemy.enemyId = parts[1];
            loaded.enemy.name = parts[2];
            loaded.enemy.behavior = parts[3];
            loaded.enemy.hp = ToInt(parts[4], 0);
            loaded.enemy.maxHp = ToInt(parts[5], 0);
            loaded.enemy.shield = ToInt(parts[6], 0);
            loaded.enemy.intentKind = parts[7];
            loaded.enemy.intentLabel = parts[8];
            loaded.enemy.intentValue = ToInt(parts[9], 0);
            loaded.enemy.patternIndex = ToInt(parts[10], 0);
            loaded.enemy.weakenTurns = ToInt(parts[11], 0);
            if (parts.size() >= 13) {
                loaded.enemy.shieldLockTurns = ToInt(parts[12], 0);
            }
        } else if (key == "NODE" && parts.size() >= 5) {
            loaded.routeOptions.push_back({parts[1], parts[2], parts[3], ToInt(parts[4], 1)});
        } else if (key == "ENCOUNTER" && parts.size() >= 5) {
            loaded.encounter.typeId = parts[1];
            loaded.encounter.encounterId = parts[2];
            loaded.encounter.title = parts[3];
            loaded.encounter.description = parts[4];
        } else if (key == "CHOICE" && parts.size() >= 5) {
            loaded.encounter.choices.push_back(parts[1]);
            loaded.encounter.details.push_back(parts[2]);
            loaded.encounter.payloadIds.push_back(parts[3]);
            loaded.encounter.payloadValues.push_back(ToInt(parts[4], 0));
        } else if (key == "REWARD" && parts.size() >= 4) {
            loaded.reward.banner = parts[1];
            loaded.reward.scrap = ToInt(parts[2], 0);
            loaded.reward.relicId = parts[3];
        } else if (key == "REWARDCARD" && parts.size() >= 2) {
            loaded.reward.cardIds.push_back(parts[1]);
        } else if (key == "INFO" && parts.size() >= 3) {
            loaded.info.title = parts[1];
            loaded.info.footer = parts[2];
        } else if (key == "INFOLINE" && parts.size() >= 2) {
            loaded.info.lines.push_back(parts[1]);
        } else if (key == "LOG" && parts.size() >= 2) {
            loaded.battleLog.push_back(parts[1]);
        } else if (key == "ANIMATION" && parts.size() >= 7) {
            loaded.animation.kind = parts[1];
            loaded.animation.label = parts[2];
            loaded.animation.magnitude = ToInt(parts[3], 0);
            loaded.animation.playerToEnemy = parts[4] == "1";
            loaded.animation.tick = ToInt(parts[5], 0);
            loaded.animation.duration = ToInt(parts[6], 0);
            if (parts.size() >= 8) {
                loaded.animation.active = parts[7] == "1";
            }
        }
    }

    loaded.hasRun = true;
    loaded.sector = std::max(1, loaded.sector);
    loaded.nodesPerSector = std::max(1, loaded.nodesPerSector);
    loaded.depth = std::max(0, loaded.depth);
    loaded.battleTurn = std::max(0, loaded.battleTurn);
    loaded.player.maxHp = std::max(1, loaded.player.maxHp);
    loaded.player.hp = std::max(0, std::min(loaded.player.hp, loaded.player.maxHp));
    loaded.player.maxEnergy = std::max(1, loaded.player.maxEnergy);
    loaded.player.energy = std::max(0, loaded.player.energy);
    loaded.player.maxHeat = std::max(1, loaded.player.maxHeat);
    loaded.player.heat = std::max(0, std::min(loaded.player.heat, loaded.player.maxHeat));
    loaded.player.shield = std::max(0, loaded.player.shield);
    loaded.player.nextAttackMultiplier = std::max(1, loaded.player.nextAttackMultiplier);
    loaded.enemy.hp = std::max(0, std::min(loaded.enemy.hp, loaded.enemy.maxHp));
    loaded.enemy.shield = std::max(0, loaded.enemy.shield);
    loaded.enemy.weakenTurns = std::max(0, loaded.enemy.weakenTurns);
    loaded.enemy.shieldLockTurns = std::max(0, loaded.enemy.shieldLockTurns);

    if (shipTemplateById(loaded.player.shipId) == nullptr) {
        if (error != nullptr) {
            *error = "Save file references an unknown ship.";
        }
        return false;
    }
    for (const std::string& relicId : loaded.player.relics) {
        if (relicById(relicId) == nullptr) {
            if (error != nullptr) {
                *error = "Save file references an unknown relic.";
            }
            return false;
        }
    }
    const std::vector<std::vector<std::string>*> piles = {
        &loaded.player.drawPile, &loaded.player.hand, &loaded.player.discardPile, &loaded.reward.cardIds,
    };
    for (const std::vector<std::string>* pile : piles) {
        for (const std::string& cardId : *pile) {
            if (cardById(cardId) == nullptr) {
                if (error != nullptr) {
                    *error = "Save file references an unknown card.";
                }
                return false;
            }
        }
    }
    if (!loaded.enemy.enemyId.empty() && enemyTemplateById(loaded.enemy.enemyId) == nullptr) {
        if (error != nullptr) {
            *error = "Save file references an unknown enemy.";
        }
        return false;
    }

    if (loaded.page == AppPage::MainMenu || loaded.page == AppPage::Setup || loaded.page == AppPage::Info) {
        loaded.page = AppPage::Map;
    }
    if (loaded.statusLine.empty()) {
        loaded.statusLine = "Save loaded. Navigation restored.";
    }

    state_ = loaded;
    resumePage_ = (state_.page == AppPage::MainMenu) ? AppPage::Map : state_.page;
    if (error != nullptr) {
        *error = "";
    }
    return true;
}

void GameEngine::setSeedForTesting(unsigned int seed) {
    rng_.seed(seed);
}

void GameEngine::showMainMenu() {
    if (state_.hasRun && state_.page != AppPage::MainMenu && state_.page != AppPage::Setup && state_.page != AppPage::Info) {
        resumePage_ = state_.page;
    }
    state_.page = AppPage::MainMenu;
}

void GameEngine::resumeRun() {
    if (!state_.hasRun) {
        return;
    }
    state_.page = resumePage_;
}

void GameEngine::showHelp() {
    fillHelpInfo();
    state_.page = AppPage::Info;
}

void GameEngine::showInfo(const std::string& title,
                          const std::vector<std::string>& lines,
                          const std::string& footer) {
    state_.info = {};
    state_.info.title = title;
    state_.info.lines = lines;
    state_.info.footer = footer;
    state_.page = AppPage::Info;
}

void GameEngine::showLeaderboard(const std::string& path) {
    state_.info = {};
    state_.info.title = "Run Ledger";
    state_.info.footer = "Press Esc or click Back to return.";
    std::ifstream input(path.c_str());
    if (!input) {
        state_.info.lines.push_back("No recorded runs yet. Win or lose a few voyages first.");
    } else {
        std::string entry;
        while (std::getline(input, entry)) {
            if (!entry.empty()) {
                state_.info.lines.push_back(entry);
            }
        }
        if (state_.info.lines.empty()) {
            state_.info.lines.push_back("Ledger is empty.");
        }
    }
    state_.page = AppPage::Info;
}

void GameEngine::chooseNode(size_t index) {
    if (state_.page != AppPage::Map || index >= state_.routeOptions.size()) {
        return;
    }

    const NodeOption node = state_.routeOptions[index];
    state_.activeNodeType = node.typeId;
    state_.activeNodeName = node.name;
    state_.statusLine = "Jump vector locked for " + node.name + ".";

    if (node.typeId == "battle" || node.typeId == "elite" || node.typeId == "boss") {
        startBattle(node.typeId);
    } else {
        startEncounterForNode(node);
    }
}

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

bool GameEngine::canPlayCard(size_t index) const {
    if (state_.page != AppPage::Battle) {
        return false;
    }
    if (index >= state_.player.hand.size()) {
        return false;
    }
    const CardDef* card = cardById(state_.player.hand[index]);
    return card != nullptr && card->cost <= state_.player.energy;
}

void GameEngine::playCard(size_t index) {
    if (!canPlayCard(index)) {
        state_.statusLine = "That module cannot be fired right now.";
        return;
    }

    const std::string cardId = state_.player.hand[index];
    const CardDef* card = cardById(cardId);
    if (card == nullptr) {
        return;
    }

    state_.player.energy -= card->cost;
    state_.player.discardPile.push_back(cardId);
    state_.player.hand.erase(state_.player.hand.begin() + static_cast<long long>(index));

    int outgoingDamage = card->damage;
    if (card->comboBonus > 0 && state_.player.attacksPlayedThisTurn > 0) {
        outgoingDamage += card->comboBonus;
    }
    if (card->shieldToDamage) {
        outgoingDamage += state_.player.shield;
        state_.player.shield = 0;
    }
    if (outgoingDamage > 0 && state_.player.nextAttackMultiplier > 1) {
        outgoingDamage *= state_.player.nextAttackMultiplier;
        state_.player.nextAttackMultiplier = 1;
    }

    log("You route " + card->name + ".");

    if (card->attackMultiplier > 1) {
        state_.player.nextAttackMultiplier *= card->attackMultiplier;
        log("Target lock acquired. The next attack will strike harder.");
    }
    if (card->shield > 0) {
        state_.player.shield += card->shield;
        log("Shield rises by " + std::to_string(card->shield) + ".");
    }
    if (card->heal > 0) {
        state_.player.hp = std::min(state_.player.maxHp, state_.player.hp + card->heal);
        log("Hull restores " + std::to_string(card->heal) + " HP.");
    }
    if (card->draw > 0) {
        drawCards(card->draw);
    }
    if (card->energy > 0) {
        state_.player.energy += card->energy;
        log("Reactor gain: +" + std::to_string(card->energy) + " energy.");
    }
    if (card->nextTurnEnergy != 0) {
        state_.player.nextTurnEnergy += card->nextTurnEnergy;
        log("Next turn energy buffered: +" + std::to_string(card->nextTurnEnergy) + ".");
    }
    if (card->halveHeat) {
        state_.player.heat /= 2;
        log("Heat is cut in half.");
    }
    if (card->clearHeat) {
        state_.player.heat = 0;
        log("All heat is vented from the reactor.");
    }
    if (card->heat != 0) {
        changeHeat(card->heat);
        if (card->heat > 0) {
            log("Heat climbs by " + std::to_string(card->heat) + ".");
        } else {
            log("Heat drops by " + std::to_string(-card->heat) + ".");
        }
    }
    if (card->selfDamage > 0) {
        applyDamageToPlayer(card->selfDamage);
        log("Feedback lashes your own hull for " + std::to_string(card->selfDamage) + ".");
    }
    if (card->weakenTurns > 0) {
        state_.enemy.weakenTurns = std::max(state_.enemy.weakenTurns, card->weakenTurns);
        log("Enemy fire-control is weakened.");
    }
    if (card->removeEnemyShield > 0) {
        const int removed = std::min(state_.enemy.shield, card->removeEnemyShield);
        state_.enemy.shield = std::max(0, state_.enemy.shield - card->removeEnemyShield);
        if (removed > 0) {
            log("Enemy shield stripped by " + std::to_string(removed) + ".");
        }
    }
    if (card->shieldLockTurns > 0) {
        state_.enemy.shieldLockTurns = std::max(state_.enemy.shieldLockTurns, card->shieldLockTurns);
        log("Enemy shield emitters are jammed.");
    }
    if (card->salvageBonus > 0) {
        state_.player.salvageBonus += card->salvageBonus;
        log("Battle credit payout increased by " + std::to_string(card->salvageBonus) + ".");
    }

    if (card->randomEffect) {
        std::uniform_int_distribution<int> outcome(0, 3);
        const int roll = outcome(rng_);
        if (roll == 0) {
            outgoingDamage += 18;
            changeHeat(2);
            log("Blind jump locks on target. Massive burst incoming.");
        } else if (roll == 1) {
            state_.player.shield += 12;
            drawCards(1);
            log("Blind jump returns with a mirrored shield bloom.");
        } else if (roll == 2) {
            state_.player.energy += 2;
            changeHeat(3);
            log("The jump spikes the reactor. Power surges through the ship.");
        } else {
            applyDamageToPlayer(5);
            drawCards(3);
            log("The jump tears plating free but hands you a fresh tactical line.");
        }
    }

    if (outgoingDamage > 0) {
        applyDamageToEnemy(outgoingDamage, card->ignoreShield, std::max(1, card->multiHit));
        startAnimation("attack", card->name, outgoingDamage, true, 7);
        if (card->multiHit > 1) {
            log(card->name + " deals " + std::to_string(outgoingDamage) + " x " +
                std::to_string(card->multiHit) + ".");
        } else {
            log(card->name + " deals " + std::to_string(outgoingDamage) + ".");
        }
    } else if (card->shield > 0) {
        startAnimation("shield", card->name, card->shield, true, 5);
    } else {
        startAnimation("support", card->name, std::max(card->heal, std::abs(card->heat)), true, 5);
    }

    if (card->type == "Attack" || outgoingDamage > 0) {
        ++state_.player.attacksPlayedThisTurn;
    }

    if (state_.enemy.hp <= 0) {
        handleBattleVictory();
    } else if (state_.player.hp <= 0) {
        handleDefeat();
    } else {
        state_.statusLine = card->name + " resolved. " + std::to_string(state_.player.energy) + " energy remains.";
    }
}

void GameEngine::endTurn() {
    if (state_.page != AppPage::Battle) {
        return;
    }

    for (const std::string& card : state_.player.hand) {
        state_.player.discardPile.push_back(card);
    }
    state_.player.hand.clear();
    log("Your remaining hand is discarded.");

    if (hasRelic("auto_cooler") && state_.player.heat > 0) {
        state_.player.heat = std::max(0, state_.player.heat - 1);
        log("Auto-Cooler bleeds 1 heat from the reactor.");
    }

    resolveEnemyIntent();
    if (state_.player.hp <= 0) {
        handleDefeat();
        return;
    }
    if (state_.enemy.hp <= 0) {
        handleBattleVictory();
        return;
    }

    chooseEnemyIntent();
    startPlayerTurn();
    state_.statusLine = "Enemy cycle resolved. Your turn begins.";
}

void GameEngine::chooseReward(int index) {
    if (state_.page != AppPage::Reward) {
        return;
    }

    if (index >= 0 && static_cast<size_t>(index) < state_.reward.cardIds.size()) {
        const std::string& cardId = state_.reward.cardIds[static_cast<size_t>(index)];
        state_.player.discardPile.push_back(cardId);
        const CardDef* card = cardById(cardId);
        const std::string cardName = card != nullptr ? card->name : "module";
        state_.statusLine = cardName + " has been integrated into your deck.";
    } else {
        state_.statusLine = "You skip the module reward and keep moving.";
    }

    finishCurrentNode();
}

void GameEngine::enterMarketPurge(size_t offset) {
    state_.encounter.encounterId = "market_purge";
    state_.encounter.title = "Black Market Purge";
    state_.encounter.description = "Select one card to erase from your deck. The black market visit ends immediately after the purge.";
    state_.encounter.choices.clear();
    state_.encounter.details.clear();
    state_.encounter.payloadIds.clear();
    state_.encounter.payloadValues.clear();

    const std::vector<std::pair<std::string, int>> candidates = purgeableCards();
    const size_t safe_offset = candidates.empty() ? 0 : std::min(offset, ((candidates.size() - 1) / kBlackMarketPurgePageSize) * kBlackMarketPurgePageSize);
    const size_t upper = std::min(candidates.size(), safe_offset + kBlackMarketPurgePageSize);
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
        state_.encounter.details.push_back("Cycle to the next page of cards that can be removed.");
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

void GameEngine::returnToMenu() {
    showMainMenu();
}

void GameEngine::startAnotherRun() {
    state_.page = AppPage::Setup;
}

void GameEngine::tickAnimation() {
    if (!state_.animation.active) {
        return;
    }
    ++state_.animation.tick;
    if (state_.animation.tick >= state_.animation.duration) {
        state_.animation.active = false;
        state_.animation.tick = 0;
    }
}

const CardDef* GameEngine::cardById(const std::string& id) const {
    const auto found = std::find_if(cardPool_.begin(), cardPool_.end(), [&](const CardDef& card) {
        return card.id == id;
    });
    return found == cardPool_.end() ? nullptr : &(*found);
}

std::vector<const CardDef*> GameEngine::rewardCards() const {
    std::vector<const CardDef*> result;
    for (const std::string& id : state_.reward.cardIds) {
        const CardDef* card = cardById(id);
        if (card != nullptr) {
            result.push_back(card);
        }
    }
    return result;
}

std::vector<const CardDef*> GameEngine::handCards() const {
    std::vector<const CardDef*> result;
    for (const std::string& id : state_.player.hand) {
        const CardDef* card = cardById(id);
        if (card != nullptr) {
            result.push_back(card);
        }
    }
    return result;
}

std::string GameEngine::difficultyName() const {
    return DifficultyLabel(state_.difficulty);
}

std::string GameEngine::shipNameForId(const std::string& shipId) const {
    const ShipTemplate* ship = shipTemplateById(shipId);
    return ship != nullptr ? ship->name : ships_.front().name;
}

std::string GameEngine::relicNameForId(const std::string& relicId) const {
    const RelicDef* relic = relicById(relicId);
    return relic != nullptr ? relic->name : relicId;
}

std::string GameEngine::relicDescriptionForId(const std::string& relicId) const {
    const RelicDef* relic = relicById(relicId);
    return relic != nullptr ? relic->description : "";
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
    player.drawPile = {
        "laser_burst", "laser_burst", "laser_burst", "laser_burst",
        "raise_shield", "raise_shield", "raise_shield", "raise_shield",
        "coolant_flush", "quick_patch", "overclock", "scan_pulse",
    };
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

void GameEngine::generateRouteOptions() {
    state_.routeOptions.clear();
    if (state_.depth >= state_.nodesPerSector) {
        if (state_.sector == 1) {
            state_.routeOptions.push_back({"boss", "Dread Frigate Omega", "A warship blocks the sector jump.", 4});
        } else if (state_.sector == 2) {
            state_.routeOptions.push_back({"boss", "The Furnace Carrier", "Heat wakes ripple through a carrier hull.", 5});
        } else {
            state_.routeOptions.push_back({"boss", "Abyss Core Sentinel", "The final guardian unfolds in the void.", 6});
        }
        return;
    }

    std::vector<std::string> types = {"battle", "salvage", "repair", "event"};
    if (state_.sector >= 2) {
        types.push_back("elite");
        types.push_back("market");
    }
    ShuffleVector(types, rng_);
    types.resize(std::min<size_t>(3, types.size()));

    for (const std::string& type : types) {
        if (type == "battle") {
            state_.routeOptions.push_back({"battle", "Pirate Ambush", "Fast raiders are sweeping the lane ahead.", 2});
        } else if (type == "elite") {
            state_.routeOptions.push_back({"elite", "Elite Intercept", "A heavier hostile signature holds position.", 3});
        } else if (type == "salvage") {
            state_.routeOptions.push_back({"salvage", "Derelict Wreck", "Broken hulls drift in a silent salvage field.", 1});
        } else if (type == "repair") {
            state_.routeOptions.push_back({"repair", "Repair Station", "A maintenance dock answers your ping.", 1});
        } else if (type == "market") {
            state_.routeOptions.push_back({"market", "Black Market", "Encrypted trade beacons flare nearby.", 2});
        } else {
            state_.routeOptions.push_back({"event", "Strange Signal", "Anomalous data bleeds across the sensor grid.", 2});
        }
    }

    if (state_.player.hp < state_.player.maxHp / 2) {
        const bool hasRepair = std::any_of(state_.routeOptions.begin(), state_.routeOptions.end(), [](const NodeOption& option) {
            return option.typeId == "repair";
        });
        if (!hasRepair && !state_.routeOptions.empty()) {
            state_.routeOptions.back() = {"repair", "Emergency Drydock", "A lightly armed station can patch your hull.", 1};
        }
    }
}

void GameEngine::startEncounterForNode(const NodeOption& node) {
    state_.encounter = {};
    state_.encounter.typeId = node.typeId;
    state_.encounter.title = node.name;
    state_.encounter.description = node.detail;
    state_.page = AppPage::Encounter;
    resumePage_ = AppPage::Encounter;

    if (node.typeId == "repair") {
        state_.encounter.choices = {
            "Patch Hull (+10 HP)",
            "Vent Reactor (-6 Heat, +4 Shield)",
            "Install Plating (+4 Max HP)",
        };
        state_.encounter.details = {
            "Reliable, clean repairs for a bruised ship.",
            "Stabilize the core and harden your next engagement.",
            "Trade time for permanent hull reinforcement.",
        };
        return;
    }

    if (node.typeId == "salvage") {
        state_.encounter.choices = {
            "Strip Credits",
            "Recover Module Rack",
            "Leave It",
        };
        state_.encounter.details = {
            "Gain immediate credits, but risk a little reactor heat.",
            "Turn the wreck into a fresh card reward.",
            "Avoid the unknown and conserve resources.",
        };
        return;
    }

    if (node.typeId == "market") {
        state_.encounter.encounterId = "market_shop";
        state_.encounter.description = "A broker crackles through the static with contraband cards and a purge service.";
        const std::vector<std::string> offers = randomCardChoices(3);
        state_.encounter.choices.clear();
        state_.encounter.details.clear();
        state_.encounter.payloadIds.clear();
        state_.encounter.payloadValues.clear();

        for (const std::string& cardId : offers) {
            const CardDef* card = cardById(cardId);
            if (card != nullptr) {
                state_.encounter.choices.push_back(card->name);
                state_.encounter.details.push_back(card->description);
                state_.encounter.payloadIds.push_back("card:" + card->id);
                state_.encounter.payloadValues.push_back(kBlackMarketCardPrice);
            }
        }
        state_.encounter.choices.push_back("Purge 1 card");
        state_.encounter.details.push_back("Pay 20 credits to permanently remove one card from your deck.");
        state_.encounter.payloadIds.push_back("purge");
        state_.encounter.payloadValues.push_back(kBlackMarketPurgePrice);
        state_.encounter.choices.push_back("Leave the broker");
        state_.encounter.details.push_back("Cut the transmission and keep moving.");
        state_.encounter.payloadIds.push_back("leave");
        state_.encounter.payloadValues.push_back(0);
        return;
    }

    std::uniform_int_distribution<int> pick(0, 3);
    const int eventIndex = pick(rng_);
    if (eventIndex == 0) {
        state_.encounter.encounterId = "distress_beacon";
        state_.encounter.title = "Distress Beacon";
        state_.encounter.description = "A damaged salvage tug is limping between asteroids.";
        state_.encounter.choices = {"Aid the tug", "Strip the derelict", "Ignore the call"};
        state_.encounter.details = {
            "Gain a modest reward and some goodwill.",
            "Take a bigger haul, but risk heat from a stressed reactor.",
            "Keep your run clean and your conscience murky.",
        };
    } else if (eventIndex == 1) {
        state_.encounter.encounterId = "strange_signal";
        state_.encounter.title = "Strange Signal";
        state_.encounter.description = "A harmonic transmission keeps syncing to your nav core.";
        state_.encounter.choices = {"Investigate", "Ride the signal", "Mute it"};
        state_.encounter.details = {
            "Dig for blueprints and possible reward cards.",
            "Bank an extra energy spike for the next battle.",
            "Stay safe and keep moving.",
        };
    } else if (eventIndex == 2) {
        state_.encounter.encounterId = "reactor_leak";
        state_.encounter.title = "Reactor Leak";
        state_.encounter.description = "A floating refinery vents unstable coolant into the lane.";
        state_.encounter.choices = {"Vent it clean", "Harvest unstable fuel", "Seal and patch"};
        state_.encounter.details = {
            "Cool the ship and earn a few credits.",
            "Cash out hard, but pay in reactor heat.",
            "A safe choice that restores a little hull.",
        };
    } else {
        state_.encounter.encounterId = "smuggler_cache";
        state_.encounter.title = "Smuggler Cache";
        state_.encounter.description = "A hidden cargo pod blinks with black market encryption.";
        state_.encounter.choices = {"Force it open", "Bribe the lock (10 credits)", "Leave it"};
        state_.encounter.details = {
            "High-risk relic grab with self-inflicted damage.",
            "Trade 10 credits for a controlled card reward.",
            "No risk, no gain.",
        };
    }
}

void GameEngine::startBattle(const std::string& nodeType) {
    const EnemyTemplate& enemy = pickEnemy(nodeType);
    state_.enemy.enemyId = enemy.id;
    state_.enemy.name = enemy.name;
    state_.enemy.behavior = enemy.behavior;
    state_.enemy.maxHp = enemy.baseHp + (state_.sector - 1) * (enemy.boss ? 8 : (enemy.elite ? 5 : 3));
    state_.enemy.hp = state_.enemy.maxHp;
    state_.enemy.shield = 0;
    state_.enemy.patternIndex = 0;
    state_.enemy.weakenTurns = 0;
    state_.enemy.shieldLockTurns = 0;
    state_.battleLog.clear();
    state_.battleTurn = 0;
    state_.firstTurnOfBattle = true;
    state_.player.nextAttackMultiplier = 1;
    state_.player.salvageBonus = 0;
    state_.player.attacksPlayedThisTurn = 0;
    resetBattleDeck();
    chooseEnemyIntent();
    startPlayerTurn();
    state_.page = AppPage::Battle;
    resumePage_ = AppPage::Battle;
    log("Hostile contact acquired: " + state_.enemy.name + ".");
}

void GameEngine::resetBattleDeck() {
    for (const std::string& card : state_.player.hand) {
        state_.player.drawPile.push_back(card);
    }
    for (const std::string& card : state_.player.discardPile) {
        state_.player.drawPile.push_back(card);
    }
    state_.player.hand.clear();
    state_.player.discardPile.clear();
    ShuffleVector(state_.player.drawPile, rng_);
    state_.player.shield = 0;
}

void GameEngine::startPlayerTurn() {
    if (state_.battleTurn > 0 && state_.player.shield > 0) {
        log("Your temporary shield field dissipates.");
    }
    state_.player.shield = 0;

    const int drawTarget = state_.player.heat >= state_.player.maxHeat ? (kBattleDrawCount - 1) : kBattleDrawCount;
    int energy = state_.player.maxEnergy + state_.player.nextTurnEnergy;
    if (state_.firstTurnOfBattle && hasRelic("reactor_core")) {
        energy += 1;
    }
    if (state_.firstTurnOfBattle && state_.player.openingEnergyBonus > 0) {
        energy += state_.player.openingEnergyBonus;
        log("Stored anomaly charge grants +" + std::to_string(state_.player.openingEnergyBonus) + " energy.");
        state_.player.openingEnergyBonus = 0;
    }
    state_.player.energy = std::max(0, energy);
    state_.player.nextTurnEnergy = 0;
    state_.player.attacksPlayedThisTurn = 0;
    ++state_.battleTurn;

    const int startShield = (hasRelic("shield_matrix") && state_.battleTurn == 1 ? 4 : 0) + state_.player.openingShieldBonus;
    if (startShield > 0) {
        state_.player.shield += startShield;
        log("Deflector envelope stabilizes for +" + std::to_string(startShield) + " shield.");
        state_.player.openingShieldBonus = 0;
    }

    drawCards(drawTarget - static_cast<int>(state_.player.hand.size()));
    state_.firstTurnOfBattle = false;
    log("Turn " + std::to_string(state_.battleTurn) + " begins with " + std::to_string(state_.player.energy) +
        " energy and " + std::to_string(static_cast<int>(state_.player.hand.size())) + " cards in hand.");
}

void GameEngine::chooseEnemyIntent() {
    const EnemyTemplate* enemy = enemyTemplateById(state_.enemy.enemyId);
    if (enemy == nullptr || enemy->pattern.empty()) {
        state_.enemy.intentKind = "attack";
        state_.enemy.intentLabel = "Attack";
        state_.enemy.intentValue = 6;
        return;
    }

    int index = state_.enemy.patternIndex % static_cast<int>(enemy->pattern.size());
    if (enemy->behavior == "chaotic") {
        std::uniform_int_distribution<int> randomIndex(0, static_cast<int>(enemy->pattern.size()) - 1);
        index = randomIndex(rng_);
    }

    const IntentDef& intent = enemy->pattern[static_cast<size_t>(index)];
    state_.enemy.intentKind = intent.kind;
    state_.enemy.intentLabel = intent.label;
    state_.enemy.intentValue = intent.value;
}

void GameEngine::resolveEnemyIntent() {
    if (state_.enemy.shield > 0) {
        log(state_.enemy.name + "'s temporary shield collapses.");
        state_.enemy.shield = 0;
    }

    log(state_.enemy.name + " executes " + state_.enemy.intentLabel + ".");

    if (state_.enemy.intentKind == "attack") {
        int damage = state_.enemy.intentValue;
        if (state_.enemy.weakenTurns > 0) {
            damage = std::max(0, damage - 2);
            --state_.enemy.weakenTurns;
        }
        applyDamageToPlayer(damage);
        startAnimation("enemy_attack", state_.enemy.intentLabel, damage, false, 7);
        log("You absorb " + std::to_string(damage) + " damage.");
    } else if (state_.enemy.intentKind == "shield") {
        if (state_.enemy.shieldLockTurns > 0) {
            log("Shield jam active. Enemy plating fails to form.");
        } else {
            state_.enemy.shield += state_.enemy.intentValue;
            startAnimation("shield", state_.enemy.intentLabel, state_.enemy.intentValue, false, 5);
            log(state_.enemy.name + " gains " + std::to_string(state_.enemy.intentValue) + " shield.");
        }
    } else if (state_.enemy.intentKind == "heat") {
        changeHeat(state_.enemy.intentValue);
        startAnimation("heat", state_.enemy.intentLabel, state_.enemy.intentValue, false, 6);
        log("Your reactor spikes by " + std::to_string(state_.enemy.intentValue) + " heat.");
    } else if (state_.enemy.intentKind == "emp") {
        state_.player.nextTurnEnergy -= state_.enemy.intentValue;
        startAnimation("support", state_.enemy.intentLabel, state_.enemy.intentValue, false, 5);
        log("The EMP will cut " + std::to_string(state_.enemy.intentValue) + " energy from your next turn.");
    } else if (state_.enemy.intentKind == "charge") {
        state_.enemy.shield += state_.enemy.intentValue;
        startAnimation("support", state_.enemy.intentLabel, state_.enemy.intentValue, false, 5);
        log(state_.enemy.name + " charges for a heavier strike.");
    }

    if (state_.enemy.shieldLockTurns > 0) {
        --state_.enemy.shieldLockTurns;
    }
    ++state_.enemy.patternIndex;
}

void GameEngine::handleBattleVictory() {
    state_.animation.active = false;
    state_.reward = {};
    state_.reward.banner = (state_.activeNodeType == "elite")
                               ? "Elite salvage secured"
                               : (state_.activeNodeType == "boss" ? "Boss cache recovered" : "Battle reward");

    int scrapReward = 12 + state_.sector * 4;
    if (state_.activeNodeType == "elite") {
        scrapReward += 10;
    }
    if (state_.activeNodeType == "boss") {
        scrapReward += 16;
    }
    if (hasRelic("salvage_scanner")) {
        scrapReward += 6;
    }
    scrapReward += state_.player.salvageBonus;
    state_.player.scrap += scrapReward;
    state_.reward.scrap = scrapReward;
    state_.reward.cardIds = randomCardChoices(3);

    if (state_.activeNodeType == "elite" || state_.activeNodeType == "boss") {
        const std::string relicId = randomRelic();
        state_.reward.relicId = relicId;
        if (!relicId.empty()) {
            grantRelic(relicId);
        }
    }

    state_.player.shield = 0;
    state_.player.energy = state_.player.maxEnergy;
    state_.player.nextTurnEnergy = 0;
    state_.battleTurn = 0;
    state_.pendingSectorAdvance = state_.activeNodeType == "boss" && state_.sector < 3;
    state_.pendingVictory = state_.activeNodeType == "boss" && state_.sector >= 3;
    state_.page = AppPage::Reward;
    resumePage_ = AppPage::Reward;
    state_.statusLine = "Enemy destroyed. Choose your salvage reward.";
}

void GameEngine::handleDefeat() {
    fillSummary(false);
}

void GameEngine::finishCurrentNode() {
    state_.encounter = {};
    state_.reward = {};
    state_.animation = {};
    state_.battleLog.clear();
    state_.player.shield = 0;
    state_.player.nextTurnEnergy = 0;
    state_.player.nextAttackMultiplier = 1;
    state_.player.attacksPlayedThisTurn = 0;
    state_.battleTurn = 0;

    if (state_.pendingVictory) {
        fillSummary(true);
        return;
    }

    if (state_.pendingSectorAdvance) {
        state_.pendingSectorAdvance = false;
        ++state_.sector;
        state_.depth = 0;
        state_.player.hp = std::min(state_.player.maxHp, state_.player.hp + 6);
        state_.statusLine = "Sector cleared. Your crew patches 6 HP while plotting the next route.";
    } else if (state_.activeNodeType != "boss") {
        ++state_.depth;
    }

    state_.activeNodeType.clear();
    state_.activeNodeName.clear();

    generateRouteOptions();
    state_.page = AppPage::Map;
    resumePage_ = AppPage::Map;
}

void GameEngine::applyDamageToPlayer(int amount) {
    int remaining = amount;
    if (state_.player.shield > 0) {
        const int blocked = std::min(state_.player.shield, remaining);
        state_.player.shield -= blocked;
        remaining -= blocked;
    }
    state_.player.hp -= remaining;
    if (state_.player.hp <= 0 && hasRelic("emergency_battery") && !state_.player.emergencyBatteryUsed) {
        state_.player.hp = 1;
        state_.player.emergencyBatteryUsed = true;
        log("Emergency Battery triggers and keeps the ship alive at 1 HP.");
    }
    state_.player.hp = std::max(0, state_.player.hp);
}

void GameEngine::applyDamageToEnemy(int amount, bool ignoreShield, int hits) {
    for (int hit = 0; hit < hits; ++hit) {
        int remaining = amount;
        if (!ignoreShield && state_.enemy.shield > 0) {
            const int blocked = std::min(state_.enemy.shield, remaining);
            state_.enemy.shield -= blocked;
            remaining -= blocked;
        }
        state_.enemy.hp -= remaining;
        state_.enemy.hp = std::max(0, state_.enemy.hp);
        if (state_.enemy.hp == 0) {
            break;
        }
    }
}

void GameEngine::drawCards(int count) {
    for (int i = 0; i < count; ++i) {
        if (state_.player.drawPile.empty()) {
            reshuffleDiscardIntoDraw();
        }
        if (state_.player.drawPile.empty()) {
            return;
        }
        const std::string nextCard = state_.player.drawPile.back();
        state_.player.drawPile.pop_back();
        if (state_.player.hand.size() >= kBattleHandLimit) {
            state_.player.discardPile.push_back(nextCard);
            const CardDef* card = cardById(nextCard);
            const std::string cardName = card != nullptr ? card->name : "card";
            log("Hand full. " + cardName + " is routed to discard.");
            continue;
        }
        state_.player.hand.push_back(nextCard);
    }
}

void GameEngine::reshuffleDiscardIntoDraw() {
    if (state_.player.discardPile.empty()) {
        return;
    }
    for (const std::string& card : state_.player.discardPile) {
        state_.player.drawPile.push_back(card);
    }
    state_.player.discardPile.clear();
    ShuffleVector(state_.player.drawPile, rng_);
    if (state_.page == AppPage::Battle) {
        log("Discard pile reshuffled into the draw pile.");
    }
}

void GameEngine::changeHeat(int delta) {
    state_.player.heat += delta;
    if (state_.player.heat < 0) {
        state_.player.heat = 0;
    }
    if (state_.player.heat > state_.player.maxHeat) {
        const int overflow = state_.player.heat - state_.player.maxHeat;
        state_.player.heat = state_.player.maxHeat;
        applyDamageToPlayer(overflow + 1);
        log("Overheat! Feedback burns through the hull for " + std::to_string(overflow + 1) + ".");
    }
}

void GameEngine::log(const std::string& line) {
    state_.battleLog.push_back(line);
    while (state_.battleLog.size() > 7) {
        state_.battleLog.erase(state_.battleLog.begin());
    }
}

void GameEngine::startAnimation(const std::string& kind,
                                const std::string& label,
                                int magnitude,
                                bool playerToEnemy,
                                int duration) {
    state_.animation.kind = kind;
    state_.animation.label = label;
    state_.animation.magnitude = magnitude;
    state_.animation.playerToEnemy = playerToEnemy;
    state_.animation.tick = 0;
    state_.animation.duration = duration;
    state_.animation.active = true;
}

std::vector<std::string> GameEngine::randomCardChoices(size_t count) {
    std::vector<std::string> pool;
    for (const CardDef& card : cardPool_) {
        if (card.id != "laser_burst" && card.id != "raise_shield" && card.id != "coolant_flush" &&
            card.id != "quick_patch" && card.id != "overclock" && card.id != "scan_pulse") {
            pool.push_back(card.id);
        }
    }
    ShuffleVector(pool, rng_);
    if (pool.size() > count) {
        pool.resize(count);
    }
    return pool;
}

std::string GameEngine::randomRelic() {
    std::vector<std::string> candidates;
    for (const RelicDef& relic : relicPool_) {
        if (!hasRelic(relic.id)) {
            candidates.push_back(relic.id);
        }
    }
    if (candidates.empty()) {
        return "";
    }
    ShuffleVector(candidates, rng_);
    return candidates.front();
}

const GameEngine::EnemyTemplate& GameEngine::pickEnemy(const std::string& nodeType) {
    std::vector<const EnemyTemplate*> candidates;
    for (const EnemyTemplate& enemy : enemyPool_) {
        if (nodeType == "boss" && enemy.boss) {
            if ((state_.sector == 1 && enemy.id == "dread_frigate_omega") ||
                (state_.sector == 2 && enemy.id == "furnace_carrier") ||
                (state_.sector >= 3 && enemy.id == "abyss_core_sentinel")) {
                return enemy;
            }
        }
        if (nodeType == "elite" && enemy.elite) {
            candidates.push_back(&enemy);
        } else if (nodeType == "battle" && !enemy.elite && !enemy.boss) {
            candidates.push_back(&enemy);
        }
    }

    ShuffleVector(candidates, rng_);
    return *(candidates.empty() ? &enemyPool_.front() : candidates.front());
}

const GameEngine::EnemyTemplate* GameEngine::enemyTemplateById(const std::string& id) const {
    const auto found = std::find_if(enemyPool_.begin(), enemyPool_.end(), [&](const EnemyTemplate& enemy) {
        return enemy.id == id;
    });
    return found == enemyPool_.end() ? nullptr : &(*found);
}

const GameEngine::ShipTemplate* GameEngine::shipTemplateById(const std::string& id) const {
    const auto found = std::find_if(ships_.begin(), ships_.end(), [&](const ShipTemplate& ship) {
        return ship.id == id;
    });
    return found == ships_.end() ? nullptr : &(*found);
}

const GameEngine::RelicDef* GameEngine::relicById(const std::string& id) const {
    const auto found = std::find_if(relicPool_.begin(), relicPool_.end(), [&](const RelicDef& relic) {
        return relic.id == id;
    });
    return found == relicPool_.end() ? nullptr : &(*found);
}

void GameEngine::fillHelpInfo() {
    state_.info = {};
    state_.info.title = "Flight Manual";
    state_.info.footer = "Keyboard: 1-9 and 0 choose cards, E ends turn, S saves, L loads, Esc returns.";
    state_.info.lines = {
        "Star Abyss Salvager now runs through the FTXUI gameplay flow instead of the old placeholder help screen.",
        "Map flow: choose one node per jump, clear three jumps, then defeat a sector boss.",
        "Battle flow: start each turn with up to 5 cards, spend energy to play modules, then end turn with E.",
        "Cards can draw extra modules, but your hand caps at 10 cards just like a deckbuilder battle lane.",
        "Shield is temporary: it protects you through the enemy turn, then collapses at the start of your next turn.",
        "Heat is dangerous: overheat damages your hull and can reduce your next draw to 4 cards.",
        "Mouse is enabled. You can click menu items, route nodes, cards, and reward choices directly.",
        "Save files now keep deck piles, relics, route state, enemy state, rewards, and encounter choices.",
    };
}

void GameEngine::fillSummary(bool victory) {
    state_.hasRun = false;
    state_.info = {};
    state_.info.title = victory ? "Run Complete" : "Ship Lost";
    state_.info.footer = "Choose a next step below.";
    state_.info.lines = {
        std::string("Result: ") + (victory ? "Victory" : "Defeat"),
        "Ship: " + state_.player.shipName,
        "Difficulty: " + difficultyName(),
        "Final sector: " + std::to_string(state_.sector),
        "Credits banked: " + std::to_string(state_.player.scrap),
        "Relics recovered: " + std::to_string(state_.player.relics.size()),
    };
    state_.page = AppPage::Summary;
    resumePage_ = AppPage::Summary;

    std::ofstream output("data/leaderboard.txt", std::ios::app);
    if (output) {
        output << (victory ? "Victory" : "Defeat")
               << " | Ship " << state_.player.shipName
               << " | Difficulty " << difficultyName()
               << " | Sector " << state_.sector
               << " | Credits " << state_.player.scrap << "\n";
    }
}
