#include "tui_game.h"

#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

namespace {

struct AuxSeed {
    std::string unitId;
    std::string name;
    int attack = 0;
    int shield = 0;
    int turns = -1;
};

struct Setup {
    std::string shipId = "vanguard";
    std::string shipName = "Smoke Ship";
    int hp = 60;
    int maxHp = 60;
    int energy = 6;
    int maxEnergy = 6;
    int heat = 0;
    int maxHeat = 10;
    int form = 0;
    int enemyHp = 120;
    int enemyMaxHp = 120;
    int enemyShield = 0;
    int enemyHeat = 0;
    int enemyMaxHeat = 12;
    int battleTurn = 1;
    int heatGainedThisTurn = 0;
    int heatReducedThisTurn = 0;
    int enemyHeatRaisedThisTurn = 0;
    int formChangesThisTurn = 0;
    bool enteredOverloadThisTurn = false;
    bool exitedDefenseThisTurn = false;
    bool exitedFormThisTurn = false;
    std::vector<std::string> hand;
    std::vector<std::string> drawPile;
    std::vector<AuxSeed> aux;
};

struct CaseDef {
    std::string id;
    Setup setup;
    std::function<void(GameEngine&)> action;
    std::function<bool(const RunState&, const std::vector<std::string>&, std::string&)> verify;
};

std::string SavePathFor(const std::string& id) {
    return (std::filesystem::temp_directory_path() / ("smoke_" + id + ".sav")).string();
}

void WriteScenarioSave(const Setup& s, const std::string& path) {
    std::ofstream out(path, std::ios::trunc);
    out << "VERSION|16\n";
    out << "PAGE|Battle\n";
    out << "DIFFICULTY|1\n";
    out << "SECTOR|1\n";
    out << "DEPTH|0\n";
    out << "NODES|5\n";
    out << "BATTLETURN|" << s.battleTurn << "\n";
    out << "MARKETUPGRADES|0\n";
    out << "MARKETPURGEUSED|0\n";
    out << "FIRSTTURN|0\n";
    out << "PENDINGSECTOR|0\n";
    out << "PENDINGVICTORY|0\n";
    out << "ACTIVE|battle|Smoke\n";
    out << "STATUS|smoke\n";
    out << "PLAYER|" << s.shipId << "|" << s.shipName << "|"
        << s.hp << "|" << s.maxHp << "|0|"
        << s.energy << "|" << s.maxEnergy << "|"
        << s.heat << "|" << s.maxHeat << "|0|0|0|0|1|0|0|0|0|" << s.form << "\n";
    out << "AUX|" << s.aux.size() << "\n";
    for (const auto& a : s.aux) {
        out << "AUXUNIT|" << a.unitId << "|" << a.name << "|"
            << a.attack << "|" << a.shield << "|" << a.turns << "\n";
    }
    out << "RELICS|0\n";
    out << "DRAW|" << s.drawPile.size() << "\n";
    for (const std::string& c : s.drawPile) {
        out << "CARD|" << c << "\n";
    }
    out << "HAND|" << s.hand.size() << "\n";
    for (const std::string& c : s.hand) {
        out << "CARD|" << c << "\n";
    }
    out << "DISCARD|0\n";
    out << "EXILE|0\n";
    out << "BLACKBOX||0\n";
    out << "BLACKBOXPENDING|0|0\n";
    out << "COND|0|" << s.formChangesThisTurn << "|0|0|0|0|0|0|0|0|0\n";
    out << "COND2|" << s.heatGainedThisTurn << "|"
        << s.heatReducedThisTurn << "|"
        << s.enemyHeatRaisedThisTurn << "|"
        << "0|0|0|0|0|0|0|0|0|0|0|0|"
        << (s.enteredOverloadThisTurn ? "1" : "0") << "|"
        << (s.exitedDefenseThisTurn ? "1" : "0") << "|"
        << (s.exitedFormThisTurn ? "1" : "0") << "\n";
    out << "ENEMY|pirate_scout|Pirate Scout|aggressive|"
        << s.enemyHp << "|" << s.enemyMaxHp << "|" << s.enemyShield
        << "|attack|Attack|0|0|0|"
        << s.enemyHeat << "|" << s.enemyMaxHeat << "|0|0\n";
    out << "ROUTE|0\n";
    out << "MARKETOFFERS|0\n";
    out << "BATTLELOG|0\n";
}

bool LogsContain(const std::vector<std::string>& logs, const std::string& text) {
    for (const std::string& line : logs) {
        if (line.find(text) != std::string::npos) {
            return true;
        }
    }
    return false;
}

}  // namespace

int main() {
    std::vector<CaseDef> cases;

    cases.push_back({
        "vg_03_high_heat_block",
        Setup{.shipId = "vanguard", .hand = {"vg_03"}, .heat = 4, .enemyHp = 120},
        [](GameEngine& e) { e.playCard(0); },
        [](const RunState& st, const std::vector<std::string>& logs, std::string& why) {
            if (st.player.shield != 9) {
                why = "expected shield=9";
                return false;
            }
            if (!LogsContain(logs, "Shield rises by 9")) {
                why = "missing shield log";
                return false;
            }
            return true;
        }});

    cases.push_back({
        "vg_07_heat_gain_bonus_block",
        Setup{.shipId = "vanguard", .hand = {"vg_07"}, .heatGainedThisTurn = 1},
        [](GameEngine& e) { e.playCard(0); },
        [](const RunState& st, const std::vector<std::string>& logs, std::string& why) {
            if (st.player.shield != 9) {
                why = "expected shield=9";
                return false;
            }
            if (!LogsContain(logs, "Shield rises by 9")) {
                why = "missing shield log";
                return false;
            }
            return true;
        }});

    cases.push_back({
        "vg_09_high_heat_bonus_damage",
        Setup{.shipId = "vanguard", .hand = {"vg_09"}, .heat = 6},
        [](GameEngine& e) { e.playCard(0); },
        [](const RunState&, const std::vector<std::string>& logs, std::string& why) {
            if (!LogsContain(logs, "deals 9")) {
                why = "missing damage 9 log";
                return false;
            }
            return true;
        }});

    cases.push_back({
        "vg_11_high_heat_self_damage_branch",
        Setup{.shipId = "vanguard", .hand = {"vg_11"}, .heat = 6},
        [](GameEngine& e) { e.playCard(0); },
        [](const RunState& st, const std::vector<std::string>& logs, std::string& why) {
            if (!LogsContain(logs, "deals 13")) {
                why = "missing damage 13 log";
                return false;
            }
            if (!LogsContain(logs, "Feedback lashes your own hull for 2")) {
                why = "missing self-damage log";
                return false;
            }
            if (st.player.hp != 58) {
                why = "expected hp=58";
                return false;
            }
            return true;
        }});

    cases.push_back({
        "vg_13_reduce_next_heat_gain",
        Setup{.shipId = "vanguard", .hand = {"vg_13", "vg_08"}, .heat = 0},
        [](GameEngine& e) {
            e.playCard(0);
            e.playCard(0);
        },
        [](const RunState& st, const std::vector<std::string>& logs, std::string& why) {
            if (st.player.heat != 0) {
                why = "expected heat remains 0";
                return false;
            }
            if (LogsContain(logs, "Heat climbs")) {
                why = "unexpected heat gain log";
                return false;
            }
            return true;
        }});

    cases.push_back({
        "vg_17_high_heat_override_damage",
        Setup{.shipId = "vanguard", .hand = {"vg_17"}, .heat = 6},
        [](GameEngine& e) { e.playCard(0); },
        [](const RunState&, const std::vector<std::string>& logs, std::string& why) {
            if (!LogsContain(logs, "deals 25")) {
                why = "missing damage 25 log";
                return false;
            }
            return true;
        }});

    cases.push_back({
        "vg_20_extra_hit_after_heat_reduce",
        Setup{.shipId = "vanguard", .hand = {"vg_20"}, .heatReducedThisTurn = 1},
        [](GameEngine& e) { e.playCard(0); },
        [](const RunState&, const std::vector<std::string>& logs, std::string& why) {
            if (!LogsContain(logs, "x 3")) {
                why = "missing multi-hit x3 log";
                return false;
            }
            return true;
        }});

    cases.push_back({
        "vg_28_heat_gain_to_attack_bonus",
        Setup{.shipId = "vanguard", .hand = {"vg_28", "vg_01"}, .heat = 0, .enemyHp = 200},
        [](GameEngine& e) {
            e.playCard(0);
            e.playCard(0);
        },
        [](const RunState& st, const std::vector<std::string>& logs, std::string& why) {
            if (st.player.attackBonusFromHeatGainThisBattle < 3) {
                why = "expected attack bonus from heat gain >=3";
                return false;
            }
            if (!LogsContain(logs, "Heat climbs by 3")) {
                why = "missing immediate heat gain log";
                return false;
            }
            return true;
        }});

    cases.push_back({
        "df_08_block_if_has_aux",
        Setup{.shipId = "drift_fox", .hand = {"df_08"}, .aux = {{"aux_interceptor", "Interceptor Drone", 3, 0, -1}}},
        [](GameEngine& e) { e.playCard(0); },
        [](const RunState& st, const std::vector<std::string>& logs, std::string& why) {
            if (st.player.shield != 3) {
                why = "expected shield=3";
                return false;
            }
            if (!LogsContain(logs, "Shield rises by 3")) {
                why = "missing shield+3 log";
                return false;
            }
            return true;
        }});

    cases.push_back({
        "df_09_summon_when_no_aux",
        Setup{.shipId = "drift_fox", .hand = {"df_09"}},
        [](GameEngine& e) { e.playCard(0); },
        [](const RunState& st, const std::vector<std::string>& logs, std::string& why) {
            if (st.auxiliaryShips.empty()) {
                why = "expected one summoned aux";
                return false;
            }
            if (!LogsContain(logs, "deployed")) {
                why = "missing deploy log";
                return false;
            }
            return true;
        }});

    cases.push_back({
        "df_10_energy_if_has_aux",
        Setup{.shipId = "drift_fox", .hand = {"df_10"}, .aux = {{"aux_guardian", "Guardian Drone", 0, 3, -1}}, .energy = 3},
        [](GameEngine& e) { e.playCard(0); },
        [](const RunState& st, const std::vector<std::string>& logs, std::string& why) {
            if (st.player.energy != 3) {
                why = "expected net energy unchanged at 3";
                return false;
            }
            if (!LogsContain(logs, "Reactor gain: +1 energy")) {
                why = "missing +1 energy log";
                return false;
            }
            return true;
        }});

    cases.push_back({
        "df_13_extra_draw_with_pulse",
        Setup{.shipId = "drift_fox",
              .hand = {"df_13"},
              .drawPile = {"basic_strike", "basic_strike", "basic_defense"},
              .aux = {{"aux_pulse", "Pulse Drone", 2, 1, -1}}},
        [](GameEngine& e) { e.playCard(0); },
        [](const RunState& st, const std::vector<std::string>&, std::string& why) {
            if (st.player.hand.size() != 3) {
                why = "expected draw 3 cards";
                return false;
            }
            return true;
        }});

    cases.push_back({
        "df_26_three_types_bonus",
        Setup{.shipId = "drift_fox",
              .hand = {"df_26"},
              .drawPile = {"basic_strike"},
              .energy = 4,
              .aux = {{"aux_interceptor", "Interceptor Drone", 3, 0, -1},
                      {"aux_guardian", "Guardian Drone", 0, 3, -1},
                      {"aux_pulse", "Pulse Drone", 2, 1, -1}}},
        [](GameEngine& e) { e.playCard(0); },
        [](const RunState& st, const std::vector<std::string>& logs, std::string& why) {
            if (st.player.energy < 5) {
                why = "expected +2 energy effect";
                return false;
            }
            if (!LogsContain(logs, "Reactor gain: +2 energy")) {
                why = "missing +2 energy log";
                return false;
            }
            return true;
        }});

    cases.push_back({
        "dfh_03_bonus_damage_with_aux",
        Setup{.shipId = "drift_fox",
              .hand = {"dfh_03"},
              .aux = {{"aux_interceptor", "Interceptor Drone", 3, 0, -1}}},
        [](GameEngine& e) { e.playCard(0); },
        [](const RunState&, const std::vector<std::string>& logs, std::string& why) {
            if (!LogsContain(logs, "deals 5")) {
                why = "missing damage 5 log";
                return false;
            }
            if (!LogsContain(logs, "Enemy heat climbs by 4")) {
                why = "missing enemy heat +4 log";
                return false;
            }
            return true;
        }});

    cases.push_back({
        "dfh_04_pulse_draw_bonus",
        Setup{.shipId = "drift_fox",
              .hand = {"dfh_04"},
              .drawPile = {"basic_strike"},
              .aux = {{"aux_pulse", "Pulse Drone", 2, 1, -1}}},
        [](GameEngine& e) { e.playCard(0); },
        [](const RunState& st, const std::vector<std::string>& logs, std::string& why) {
            if (st.player.hand.size() != 1) {
                why = "expected draw 1 card";
                return false;
            }
            if (!LogsContain(logs, "Enemy heat climbs by 5")) {
                why = "missing enemy heat +5 log";
                return false;
            }
            return true;
        }});

    cases.push_back({
        "dfh_06_energy_if_two_aux",
        Setup{.shipId = "drift_fox",
              .hand = {"dfh_06"},
              .energy = 3,
              .aux = {{"aux_interceptor", "Interceptor Drone", 3, 0, -1},
                      {"aux_guardian", "Guardian Drone", 0, 3, -1}}},
        [](GameEngine& e) { e.playCard(0); },
        [](const RunState&, const std::vector<std::string>& logs, std::string& why) {
            if (!LogsContain(logs, "Enemy heat climbs by 8")) {
                why = "missing enemy heat +8 log";
                return false;
            }
            if (!LogsContain(logs, "Reactor gain: +1 energy")) {
                why = "missing +1 energy log";
                return false;
            }
            return true;
        }});

    cases.push_back({
        "ib_03_bonus_after_form_exit",
        Setup{.shipId = "iron_bastion", .hand = {"ib_03"}, .exitedFormThisTurn = true},
        [](GameEngine& e) { e.playCard(0); },
        [](const RunState&, const std::vector<std::string>& logs, std::string& why) {
            if (!LogsContain(logs, "deals 11")) {
                why = "missing damage 11 log";
                return false;
            }
            return true;
        }});

    cases.push_back({
        "ib_04_draw_only_in_defense",
        Setup{.shipId = "iron_bastion", .hand = {"ib_04"}, .drawPile = {"basic_strike"}, .form = 2},
        [](GameEngine& e) { e.playCard(0); },
        [](const RunState& st, const std::vector<std::string>& logs, std::string& why) {
            if (st.player.hand.size() != 1) {
                why = "expected draw 1 card";
                return false;
            }
            if (!LogsContain(logs, "Shield rises by 8")) {
                why = "missing shield 8 log";
                return false;
            }
            return true;
        }});

    cases.push_back({
        "ib_07_bonus_block_after_leave_defense",
        Setup{.shipId = "iron_bastion", .hand = {"ib_07"}, .exitedDefenseThisTurn = true},
        [](GameEngine& e) { e.playCard(0); },
        [](const RunState& st, const std::vector<std::string>& logs, std::string& why) {
            if (st.player.shield != 9) {
                why = "expected shield=9";
                return false;
            }
            if (!LogsContain(logs, "Shield rises by 9")) {
                why = "missing shield 9 log";
                return false;
            }
            return true;
        }});

    cases.push_back({
        "ib_12_defense_gets_energy",
        Setup{.shipId = "iron_bastion", .hand = {"ib_12"}, .energy = 3, .form = 2},
        [](GameEngine& e) { e.playCard(0); },
        [](const RunState& st, const std::vector<std::string>& logs, std::string& why) {
            if (st.player.energy != 4) {
                why = "expected net energy=4";
                return false;
            }
            if (!LogsContain(logs, "Reactor gain: +2 energy")) {
                why = "missing +2 energy log";
                return false;
            }
            return true;
        }});

    cases.push_back({
        "ib_13_overload_multiplier3",
        Setup{.shipId = "iron_bastion", .hand = {"ib_13"}, .form = 1},
        [](GameEngine& e) { e.playCard(0); },
        [](const RunState& st, const std::vector<std::string>& logs, std::string& why) {
            if (st.player.nextAttackMultiplier != 3) {
                why = "expected nextAttackMultiplier=3";
                return false;
            }
            if (!LogsContain(logs, "next attack will strike harder")) {
                why = "missing multiplier setup log";
                return false;
            }
            return true;
        }});

    cases.push_back({
        "ib_17_overload_bonus_damage",
        Setup{.shipId = "iron_bastion", .hand = {"ib_17"}, .form = 1},
        [](GameEngine& e) { e.playCard(0); },
        [](const RunState&, const std::vector<std::string>& logs, std::string& why) {
            if (!LogsContain(logs, "deals 42")) {
                why = "missing damage 42 log";
                return false;
            }
            return true;
        }});

    cases.push_back({
        "ib_23_dual_condition_26_damage",
        Setup{.shipId = "iron_bastion", .hand = {"ib_23"}, .enteredOverloadThisTurn = true, .exitedDefenseThisTurn = true},
        [](GameEngine& e) { e.playCard(0); },
        [](const RunState&, const std::vector<std::string>& logs, std::string& why) {
            if (!LogsContain(logs, "deals 26")) {
                why = "missing damage 26 log";
                return false;
            }
            return true;
        }});

    cases.push_back({
        "ib_26_exit_defense_reward",
        Setup{.shipId = "iron_bastion", .hand = {"ib_26"}, .form = 2, .energy = 3},
        [](GameEngine& e) { e.playCard(0); },
        [](const RunState& st, const std::vector<std::string>& logs, std::string& why) {
            if (st.player.form != 0) {
                why = "expected form becomes neutral";
                return false;
            }
            if (st.player.energy != 3) {
                why = "expected net energy stays 3";
                return false;
            }
            if (!LogsContain(logs, "Combat form shifted to Neutral")) {
                why = "missing form shift log";
                return false;
            }
            return true;
        }});

    int passCount = 0;
    for (const CaseDef& c : cases) {
        const std::string path = SavePathFor(c.id);
        WriteScenarioSave(c.setup, path);

        GameEngine engine;
        std::string error;
        if (!engine.loadFromFile(path, &error)) {
            std::cout << "[FAIL] " << c.id << " | load error: " << error << "\n";
            continue;
        }
        const size_t logStart = engine.state().battleLog.size();
        c.action(engine);
        const auto& battleLog = engine.state().battleLog;
        std::vector<std::string> newLogs;
        for (size_t i = logStart; i < battleLog.size(); ++i) {
            newLogs.push_back(battleLog[i]);
        }

        std::string why;
        const bool ok = c.verify(engine.state(), newLogs, why);
        if (ok) {
            ++passCount;
            std::cout << "[PASS] " << c.id << "\n";
        } else {
            std::cout << "[FAIL] " << c.id << " | " << why << "\n";
        }
        for (const std::string& line : newLogs) {
            std::cout << "  - " << line << "\n";
        }
    }

    std::cout << "\nRESULT: " << passCount << "/" << static_cast<int>(cases.size()) << " passed.\n";
    return passCount == static_cast<int>(cases.size()) ? 0 : 1;
}

