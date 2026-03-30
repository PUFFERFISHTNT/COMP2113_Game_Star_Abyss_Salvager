#include "engine/game_content.h"

#include <algorithm>

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
