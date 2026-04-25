#include "engine/game_save.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>

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

namespace {

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

std::string ValueAfterKey(const std::string& line, const char* key_with_delimiter) {
    const std::string prefix(key_with_delimiter);
    if (line.rfind(prefix, 0) != 0) {
        return "";
    }
    return line.substr(prefix.size());
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

std::string UpgradedCardsJsonPath(const std::string& save_path) {
    namespace fs = std::filesystem;
    const fs::path path(save_path);
    return (path.parent_path() / (path.stem().string() + ".cards.json")).string();
}

}  // namespace

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

    output << "VERSION|21\n";
    output << "PAGE|" << PageToString(state_.page) << "\n";
    output << "DIFFICULTY|" << state_.difficulty << "\n";
    output << "SECTOR|" << state_.sector << "\n";
    output << "DEPTH|" << state_.depth << "\n";
    output << "NODES|" << state_.nodesPerSector << "\n";
    output << "BATTLETURN|" << state_.battleTurn << "\n";
    output << "MARKETUPGRADES|" << state_.marketUpgradeCount << "\n";
    output << "MARKETPURGEUSED|" << BoolToken(state_.marketPurgeUsed) << "\n";
    output << "FIRSTTURN|" << BoolToken(state_.firstTurnOfBattle) << "\n";
    output << "PENDINGSECTOR|" << BoolToken(state_.pendingSectorAdvance) << "\n";
    output << "PENDINGVICTORY|" << BoolToken(state_.pendingVictory) << "\n";
    output << "ACTIVE|" << state_.activeNodeType << "|" << state_.activeNodeName << "\n";
    output << "STATUS|" << state_.statusLine << "\n";
    output << "ROUTESTATE|" << g_sa_beacon_activated << "|"
           << g_sa_escape_stage << "|"
           << g_sa_truth_stage << "|"
           << BoolToken(g_sa_truth_post_battle) << "|"
           << g_sa_skip_segments << "|"
           << BoolToken(g_sa_next_elite) << "|"
           << BoolToken(g_sa_heat_accel) << "|"
           << BoolToken(g_sa_no_boss) << "|"
           << g_sa_permanent_shield << "|"
           << BoolToken(g_sa_escape_battle_pending) << "|"
           << BoolToken(g_sa_escape_intro_pending) << "|"
           << BoolToken(g_sa_game_intro_pending) << "\n";
    output << "ROUTEEPILOGUE|" << g_sa_event_epilogue << "\n";

    const PlayerState& player = state_.player;
    output << "PLAYER|" << player.shipId << "|" << player.shipName << "|"
           << player.hp << "|" << player.maxHp << "|" << player.shield << "|"
           << player.energy << "|" << player.maxEnergy << "|" << player.heat << "|"
           << player.maxHeat << "|" << player.scrap << "|" << player.nextTurnEnergy << "|"
           << player.openingEnergyBonus << "|" << player.openingShieldBonus << "|"
           << player.nextAttackMultiplier << "|" << player.salvageBonus << "|"
           << player.attacksPlayedThisTurn << "|" << BoolToken(player.emergencyBatteryUsed) << "|"
           << player.overheatRounds << "|" << player.form << "\n";

    output << "AUX|" << state_.auxiliaryShips.size() << "\n";
    for (const AuxiliaryShipState& aux : state_.auxiliaryShips) {
        output << "AUXUNIT|" << aux.unitId << "|" << aux.displayName << "|"
               << aux.attack << "|" << aux.shield << "|" << aux.remainingTurns << "\n";
    }

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
    output << "EXILE|" << player.exilePile.size() << "\n";
    for (const std::string& card : player.exilePile) {
        output << "CARD|" << card << "\n";
    }
    output << "ACTIVEABILITIES|" << player.activeAbilities.size() << "\n";
    for (const ActiveAbilityState& ability : player.activeAbilities) {
        output << "ABILITY|" << ability.cardRef << "|" << ability.name << "|" << ability.nameEn
               << "|" << ability.description << "|" << ability.descriptionEn << "\n";
    }
    output << "BLACKBOX|" << player.blackboxImprintRef << "|" << player.blackboxCopyCount << "\n";
    output << "BLACKBOXPENDING|" << BoolToken(player.blackboxSelectionPending) << "|"
           << player.pendingBlackboxCopyCount << "\n";
    output << "COND|" << player.summonCardPlayedDrawThisTurnBuff << "|"
           << player.formChangesThisTurn << "|"
           << player.firstEnterHighHeatEnergyThisBattle << "|"
           << player.firstEnterHighHeatDrawThisBattle << "|"
           << player.eachEnterHighHeatDrawThisBattle << "|"
           << player.firstSummonEnemyHeatPerTurnThisBattle << "|"
           << player.firstSummonEnemyHeatAttackBonusPerTurnThisBattle << "|"
           << player.enterOverloadEnemyHeatThisBattle << "|"
           << player.leaveDefenseEnemyHeatThisBattle << "|"
           << BoolToken(player.firstEnterHighHeatTriggeredThisBattle) << "|"
           << BoolToken(player.firstSummonEnemyHeatTriggeredThisTurn) << "\n";
    output << "COND2|" << player.heatGainedThisTurn << "|"
           << player.heatReducedThisTurn << "|"
           << player.enemyHeatRaisedThisTurn << "|"
           << player.nextHeatGainReductionThisTurn << "|"
           << player.nextAttackEnemyHeatBonusThisTurn << "|"
           << player.attackBonusPerHeatThisTurn << "|"
           << player.attackBonusPerHeatCapThisTurn << "|"
           << player.attackBonusFromHeatGainThisBattle << "|"
           << player.attackBonusPerHeatGainPointThisBattle << "|"
           << player.killAttackBonusThisBattle << "|"
           << player.auxTriggerEnemyHeatBonusThisTurn << "|"
           << player.formChangeDrawThisTurn << "|"
           << player.overheatSelfDamageBlockThisTurn << "|"
           << player.auxOnRemoveDrawThisBattle << "|"
           << player.auxOnRemoveEnergyThisBattle << "|"
           << BoolToken(player.enteredOverloadThisTurn) << "|"
           << BoolToken(player.exitedDefenseThisTurn) << "|"
           << BoolToken(player.exitedFormThisTurn) << "\n";
    output << "COND3|" << player.nextTurnDraw << "|"
           << player.enterDefenseShieldThisBattle << "|"
           << player.exitDefenseEnergyThisBattle << "|"
           << player.firstEnterOverloadDrawThisBattle << "|"
           << player.firstExitDefenseEnergyThisBattle << "|"
           << BoolToken(player.firstEnterOverloadDrawTriggeredThisBattle) << "|"
           << BoolToken(player.firstExitDefenseEnergyTriggeredThisBattle) << "\n";
    output << "COND4|" << player.auxEndTurnExtraTriggersThisTurn << "|"
           << player.auxEffectPercentBonusThisTurn << "|"
           << player.overloadIncomingDamageReducePercentThisTurn << "|"
           << player.firstSummonExtraCopies << "|"
           << player.firstSummonExtraScope << "|"
           << BoolToken(player.firstSummonUsedThisBattle) << "|"
           << BoolToken(player.firstSummonUsedThisTurn) << "\n";

    const EnemyState& enemy = state_.enemy;
    output << "ENEMY|" << enemy.enemyId << "|" << enemy.name << "|" << enemy.behavior << "|"
           << enemy.hp << "|" << enemy.maxHp << "|" << enemy.shield << "|"
           << enemy.intentKind << "|" << enemy.intentLabel << "|" << enemy.intentValue << "|"
           << enemy.patternIndex << "|" << enemy.weakenTurns << "|" << enemy.shieldLockTurns << "|"
           << enemy.heat << "|" << enemy.maxHeat << "|" << enemy.overheatRounds << "|"
           << enemy.coolingCooldownTurns << "\n";

    output << "ROUTE|" << state_.routeOptions.size() << "\n";
    for (const NodeOption& option : state_.routeOptions) {
        output << "NODE|" << option.typeId << "|" << option.name << "|" << option.detail
               << "|" << option.danger << "|" << option.enemyId << "\n";
    }

    output << "MARKETOFFERS|" << state_.marketOfferCardIds.size() << "\n";
    for (const std::string& card : state_.marketOfferCardIds) {
        output << "MARKETOFFER|" << card << "\n";
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

    // Upgraded card definitions now come only from data/cards.json. If an
    // older sidecar still exists beside the save, remove it so the save folder
    // reflects the new single-source-of-truth model.
    std::error_code remove_error;
    std::filesystem::remove(UpgradedCardsJsonPath(path), remove_error);

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
    int loaded_beacon_activated = 0;
    int loaded_escape_stage = 0;
    int loaded_truth_stage = 0;
    bool loaded_truth_post_battle = false;
    int loaded_skip_segments = 0;
    bool loaded_next_elite = false;
    bool loaded_heat_accel = false;
    bool loaded_no_boss = false;
    int loaded_permanent_shield = 0;
    bool loaded_escape_battle_pending = false;
    bool loaded_escape_intro_pending = false;
    bool loaded_game_intro_pending = false;
    std::string loaded_event_epilogue;

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
        } else if (key == "MARKETUPGRADES" && parts.size() >= 2) {
            loaded.marketUpgradeCount = ToInt(parts[1], 0);
        } else if (key == "MARKETPURGEUSED" && parts.size() >= 2) {
            loaded.marketPurgeUsed = parts[1] == "1";
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
        } else if (key == "ROUTESTATE" && parts.size() >= 13) {
            loaded_beacon_activated = ToInt(parts[1], 0);
            loaded_escape_stage = ToInt(parts[2], 0);
            loaded_truth_stage = ToInt(parts[3], 0);
            loaded_truth_post_battle = parts[4] == "1";
            loaded_skip_segments = ToInt(parts[5], 0);
            loaded_next_elite = parts[6] == "1";
            loaded_heat_accel = parts[7] == "1";
            loaded_no_boss = parts[8] == "1";
            loaded_permanent_shield = ToInt(parts[9], 0);
            loaded_escape_battle_pending = parts[10] == "1";
            loaded_escape_intro_pending = parts[11] == "1";
            loaded_game_intro_pending = parts[12] == "1";
        } else if (key == "ROUTEEPILOGUE") {
            loaded_event_epilogue = ValueAfterKey(line, "ROUTEEPILOGUE|");
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
                if (parts.size() >= 19) {
                    loaded.player.overheatRounds = ToInt(parts[18], 0);
                }
                if (parts.size() >= 20) {
                    loaded.player.form = ToInt(parts[19], 0);
                }
            } else {
                loaded.player.nextAttackMultiplier = ToInt(parts[12], 1);
                loaded.player.salvageBonus = ToInt(parts[13], 0);
                loaded.player.attacksPlayedThisTurn = ToInt(parts[14], 0);
                loaded.player.emergencyBatteryUsed = parts[15] == "1";
            }
        } else if (key == "AUXUNIT" && parts.size() >= 6) {
            AuxiliaryShipState aux;
            aux.unitId = parts[1];
            aux.displayName = parts[2];
            aux.attack = ToInt(parts[3], 0);
            aux.shield = ToInt(parts[4], 0);
            aux.remainingTurns = ToInt(parts[5], -1);
            loaded.auxiliaryShips.push_back(aux);
        } else if (key == "RELIC" && parts.size() >= 2) {
            loaded.player.relics.push_back(parts[1]);
        } else if (key == "UPGRADEDTYPE" && parts.size() >= 2) {
            loaded.upgradedCardTypes.push_back(parts[1]);
        } else if (key == "DRAW") {
            loaded.player.drawPile.clear();
            activePile = &loaded.player.drawPile;
        } else if (key == "HAND") {
            loaded.player.hand.clear();
            activePile = &loaded.player.hand;
        } else if (key == "DISCARD") {
            loaded.player.discardPile.clear();
            activePile = &loaded.player.discardPile;
        } else if (key == "EXILE") {
            loaded.player.exilePile.clear();
            activePile = &loaded.player.exilePile;
        } else if (key == "ACTIVEABILITIES") {
            loaded.player.activeAbilities.clear();
            activePile = nullptr;
        } else if (key == "ABILITY" && parts.size() >= 6) {
            ActiveAbilityState ability;
            ability.cardRef = parts[1];
            ability.name = parts[2];
            ability.nameEn = parts[3];
            ability.description = parts[4];
            ability.descriptionEn = parts[5];
            loaded.player.activeAbilities.push_back(ability);
        } else if (key == "BLACKBOX" && parts.size() >= 3) {
            loaded.player.blackboxImprintRef = parts[1];
            loaded.player.blackboxCopyCount = ToInt(parts[2], 0);
        } else if (key == "BLACKBOXPENDING" && parts.size() >= 3) {
            loaded.player.blackboxSelectionPending = parts[1] == "1";
            loaded.player.pendingBlackboxCopyCount = ToInt(parts[2], 0);
        } else if (key == "COND" && parts.size() >= 12) {
            loaded.player.summonCardPlayedDrawThisTurnBuff = ToInt(parts[1], 0);
            loaded.player.formChangesThisTurn = ToInt(parts[2], 0);
            loaded.player.firstEnterHighHeatEnergyThisBattle = ToInt(parts[3], 0);
            loaded.player.firstEnterHighHeatDrawThisBattle = ToInt(parts[4], 0);
            loaded.player.eachEnterHighHeatDrawThisBattle = ToInt(parts[5], 0);
            loaded.player.firstSummonEnemyHeatPerTurnThisBattle = ToInt(parts[6], 0);
            loaded.player.firstSummonEnemyHeatAttackBonusPerTurnThisBattle = ToInt(parts[7], 0);
            loaded.player.enterOverloadEnemyHeatThisBattle = ToInt(parts[8], 0);
            loaded.player.leaveDefenseEnemyHeatThisBattle = ToInt(parts[9], 0);
            loaded.player.firstEnterHighHeatTriggeredThisBattle = parts[10] == "1";
            loaded.player.firstSummonEnemyHeatTriggeredThisTurn = parts[11] == "1";
        } else if (key == "COND2" && parts.size() >= 19) {
            loaded.player.heatGainedThisTurn = ToInt(parts[1], 0);
            loaded.player.heatReducedThisTurn = ToInt(parts[2], 0);
            loaded.player.enemyHeatRaisedThisTurn = ToInt(parts[3], 0);
            loaded.player.nextHeatGainReductionThisTurn = ToInt(parts[4], 0);
            loaded.player.nextAttackEnemyHeatBonusThisTurn = ToInt(parts[5], 0);
            loaded.player.attackBonusPerHeatThisTurn = ToInt(parts[6], 0);
            loaded.player.attackBonusPerHeatCapThisTurn = ToInt(parts[7], 0);
            loaded.player.attackBonusFromHeatGainThisBattle = ToInt(parts[8], 0);
            loaded.player.attackBonusPerHeatGainPointThisBattle = ToInt(parts[9], 0);
            loaded.player.killAttackBonusThisBattle = ToInt(parts[10], 0);
            loaded.player.auxTriggerEnemyHeatBonusThisTurn = ToInt(parts[11], 0);
            loaded.player.formChangeDrawThisTurn = ToInt(parts[12], 0);
            loaded.player.overheatSelfDamageBlockThisTurn = ToInt(parts[13], 0);
            loaded.player.auxOnRemoveDrawThisBattle = ToInt(parts[14], 0);
            loaded.player.auxOnRemoveEnergyThisBattle = ToInt(parts[15], 0);
            loaded.player.enteredOverloadThisTurn = parts[16] == "1";
            loaded.player.exitedDefenseThisTurn = parts[17] == "1";
            loaded.player.exitedFormThisTurn = parts[18] == "1";
        } else if (key == "COND3" && parts.size() >= 8) {
            loaded.player.nextTurnDraw = ToInt(parts[1], 0);
            loaded.player.enterDefenseShieldThisBattle = ToInt(parts[2], 0);
            loaded.player.exitDefenseEnergyThisBattle = ToInt(parts[3], 0);
            loaded.player.firstEnterOverloadDrawThisBattle = ToInt(parts[4], 0);
            loaded.player.firstExitDefenseEnergyThisBattle = ToInt(parts[5], 0);
            loaded.player.firstEnterOverloadDrawTriggeredThisBattle = parts[6] == "1";
            loaded.player.firstExitDefenseEnergyTriggeredThisBattle = parts[7] == "1";
        } else if (key == "COND4" && parts.size() >= 8) {
            loaded.player.auxEndTurnExtraTriggersThisTurn = ToInt(parts[1], 0);
            loaded.player.auxEffectPercentBonusThisTurn = ToInt(parts[2], 0);
            loaded.player.overloadIncomingDamageReducePercentThisTurn = ToInt(parts[3], 0);
            loaded.player.firstSummonExtraCopies = ToInt(parts[4], 0);
            loaded.player.firstSummonExtraScope = ToInt(parts[5], 0);
            loaded.player.firstSummonUsedThisBattle = parts[6] == "1";
            loaded.player.firstSummonUsedThisTurn = parts[7] == "1";
        } else if (key == "CARD" && activePile != nullptr) {
            const std::string card_ref = ValueAfterKey(line, "CARD|");
            if (!card_ref.empty()) {
                activePile->push_back(card_ref);
            }
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
            if (parts.size() >= 15) {
                loaded.enemy.heat = ToInt(parts[13], 0);
                loaded.enemy.maxHeat = ToInt(parts[14], 10);
            }
            if (parts.size() >= 16) {
                loaded.enemy.overheatRounds = ToInt(parts[15], 0);
            } else if (loaded.enemy.heat >= loaded.enemy.maxHeat) {
                loaded.enemy.overheatRounds = 1;
            }
            if (parts.size() >= 17) {
                loaded.enemy.coolingCooldownTurns = ToInt(parts[16], 0);
            }
        } else if (key == "NODE" && parts.size() >= 5) {
            NodeOption option;
            option.typeId = parts[1];
            option.name = parts[2];
            option.detail = parts[3];
            option.danger = ToInt(parts[4], 1);
            if (parts.size() >= 6) {
                option.enemyId = parts[5];
            }
            loaded.routeOptions.push_back(option);
        } else if (key == "MARKETOFFER" && parts.size() >= 2) {
            loaded.marketOfferCardIds.push_back(parts[1]);
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
    loaded.marketUpgradeCount = std::max(0, loaded.marketUpgradeCount);
    loaded.player.maxHp = std::max(1, loaded.player.maxHp);
    loaded.player.hp = std::max(0, std::min(loaded.player.hp, loaded.player.maxHp));
    loaded.player.maxEnergy = std::max(1, loaded.player.maxEnergy);
    loaded.player.energy = std::max(0, loaded.player.energy);
    loaded.player.maxHeat = std::max(1, loaded.player.maxHeat);
    loaded.player.heat = std::max(0, std::min(loaded.player.heat, loaded.player.maxHeat));
    loaded.player.shield = std::max(0, loaded.player.shield);
    loaded.player.nextAttackMultiplier = std::max(1, loaded.player.nextAttackMultiplier);
    loaded.player.overheatRounds = std::max(0, std::min(loaded.player.overheatRounds, 3));
    loaded.player.form = std::max(0, std::min(2, loaded.player.form));
    loaded.player.blackboxCopyCount = std::max(0, loaded.player.blackboxCopyCount);
    loaded.player.pendingBlackboxCopyCount = std::max(0, loaded.player.pendingBlackboxCopyCount);
    loaded.player.summonCardPlayedDrawThisTurnBuff = std::max(0, loaded.player.summonCardPlayedDrawThisTurnBuff);
    loaded.player.formChangesThisTurn = std::max(0, loaded.player.formChangesThisTurn);
    loaded.player.firstEnterHighHeatEnergyThisBattle = std::max(0, loaded.player.firstEnterHighHeatEnergyThisBattle);
    loaded.player.firstEnterHighHeatDrawThisBattle = std::max(0, loaded.player.firstEnterHighHeatDrawThisBattle);
    loaded.player.eachEnterHighHeatDrawThisBattle = std::max(0, loaded.player.eachEnterHighHeatDrawThisBattle);
    loaded.player.firstSummonEnemyHeatPerTurnThisBattle = std::max(0, loaded.player.firstSummonEnemyHeatPerTurnThisBattle);
    loaded.player.firstSummonEnemyHeatAttackBonusPerTurnThisBattle =
        std::max(0, loaded.player.firstSummonEnemyHeatAttackBonusPerTurnThisBattle);
    loaded.player.enterOverloadEnemyHeatThisBattle = std::max(0, loaded.player.enterOverloadEnemyHeatThisBattle);
    loaded.player.leaveDefenseEnemyHeatThisBattle = std::max(0, loaded.player.leaveDefenseEnemyHeatThisBattle);
    loaded.player.heatGainedThisTurn = std::max(0, loaded.player.heatGainedThisTurn);
    loaded.player.heatReducedThisTurn = std::max(0, loaded.player.heatReducedThisTurn);
    loaded.player.enemyHeatRaisedThisTurn = std::max(0, loaded.player.enemyHeatRaisedThisTurn);
    loaded.player.nextHeatGainReductionThisTurn = std::max(0, loaded.player.nextHeatGainReductionThisTurn);
    loaded.player.nextAttackEnemyHeatBonusThisTurn = std::max(0, loaded.player.nextAttackEnemyHeatBonusThisTurn);
    loaded.player.attackBonusPerHeatThisTurn = std::max(0, loaded.player.attackBonusPerHeatThisTurn);
    loaded.player.attackBonusPerHeatCapThisTurn = std::max(0, loaded.player.attackBonusPerHeatCapThisTurn);
    loaded.player.attackBonusFromHeatGainThisBattle = std::max(0, loaded.player.attackBonusFromHeatGainThisBattle);
    loaded.player.attackBonusPerHeatGainPointThisBattle =
        std::max(0, loaded.player.attackBonusPerHeatGainPointThisBattle);
    loaded.player.killAttackBonusThisBattle = std::max(0, loaded.player.killAttackBonusThisBattle);
    loaded.player.auxTriggerEnemyHeatBonusThisTurn = std::max(0, loaded.player.auxTriggerEnemyHeatBonusThisTurn);
    loaded.player.formChangeDrawThisTurn = std::max(0, loaded.player.formChangeDrawThisTurn);
    loaded.player.overheatSelfDamageBlockThisTurn = std::max(0, loaded.player.overheatSelfDamageBlockThisTurn);
    loaded.player.auxOnRemoveDrawThisBattle = std::max(0, loaded.player.auxOnRemoveDrawThisBattle);
    loaded.player.auxOnRemoveEnergyThisBattle = std::max(0, loaded.player.auxOnRemoveEnergyThisBattle);
    loaded.player.nextTurnDraw = std::max(0, loaded.player.nextTurnDraw);
    loaded.player.enterDefenseShieldThisBattle = std::max(0, loaded.player.enterDefenseShieldThisBattle);
    loaded.player.exitDefenseEnergyThisBattle = std::max(0, loaded.player.exitDefenseEnergyThisBattle);
    loaded.player.firstEnterOverloadDrawThisBattle = std::max(0, loaded.player.firstEnterOverloadDrawThisBattle);
    loaded.player.firstExitDefenseEnergyThisBattle = std::max(0, loaded.player.firstExitDefenseEnergyThisBattle);
    loaded.player.auxEndTurnExtraTriggersThisTurn = std::max(0, loaded.player.auxEndTurnExtraTriggersThisTurn);
    loaded.player.auxEffectPercentBonusThisTurn = std::max(0, loaded.player.auxEffectPercentBonusThisTurn);
    loaded.player.overloadIncomingDamageReducePercentThisTurn =
        std::max(0, std::min(100, loaded.player.overloadIncomingDamageReducePercentThisTurn));
    loaded.player.firstSummonExtraCopies = std::max(0, loaded.player.firstSummonExtraCopies);
    loaded.player.firstSummonExtraScope = std::max(0, loaded.player.firstSummonExtraScope);
    if (loaded.page != AppPage::Battle || loaded.player.heat < loaded.player.maxHeat) {
        loaded.player.overheatRounds = 0;
    } else if (loaded.player.overheatRounds == 0) {
        loaded.player.overheatRounds = 1;
    }
    loaded.enemy.hp = std::max(0, std::min(loaded.enemy.hp, loaded.enemy.maxHp));
    loaded.enemy.shield = std::max(0, loaded.enemy.shield);
    loaded.enemy.maxHeat = std::max(1, loaded.enemy.maxHeat);
    loaded.enemy.heat = std::max(0, std::min(loaded.enemy.heat, loaded.enemy.maxHeat));
    loaded.enemy.overheatRounds = std::max(0, std::min(loaded.enemy.overheatRounds, 3));
    if (loaded.enemy.heat < loaded.enemy.maxHeat) {
        loaded.enemy.overheatRounds = 0;
    } else if (loaded.enemy.overheatRounds == 0) {
        loaded.enemy.overheatRounds = 1;
    }
    loaded.enemy.coolingCooldownTurns = std::max(0, loaded.enemy.coolingCooldownTurns);
    loaded.enemy.weakenTurns = std::max(0, loaded.enemy.weakenTurns);
    loaded.enemy.shieldLockTurns = std::max(0, loaded.enemy.shieldLockTurns);
    for (AuxiliaryShipState& aux : loaded.auxiliaryShips) {
        aux.attack = std::max(0, aux.attack);
        aux.shield = std::max(0, aux.shield);
        if (aux.remainingTurns == 0) {
            aux.remainingTurns = -1;
        }
    }
    loaded.auxiliaryShips.erase(
        std::remove_if(loaded.auxiliaryShips.begin(),
                       loaded.auxiliaryShips.end(),
                       [](const AuxiliaryShipState& aux) {
                           return aux.unitId.empty() || aux.displayName.empty();
                       }),
        loaded.auxiliaryShips.end());

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
    for (const std::string& cardId : loaded.upgradedCardTypes) {
        if (cardById(cardId) == nullptr) {
            if (error != nullptr) {
                *error = "Save file references an unknown upgraded card.";
            }
            return false;
        }
    }
    const std::vector<std::vector<std::string>*> piles = {
        &loaded.player.drawPile, &loaded.player.hand, &loaded.player.discardPile, &loaded.player.exilePile,
        &loaded.reward.cardIds, &loaded.marketOfferCardIds,
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
    if (!loaded.player.blackboxImprintRef.empty() && cardById(loaded.player.blackboxImprintRef) == nullptr) {
        if (error != nullptr) {
            *error = "Save file references an unknown blackbox imprint card.";
        }
        return false;
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

    g_sa_beacon_activated = std::max(0, std::min(1, loaded_beacon_activated));
    g_sa_escape_stage = std::max(0, loaded_escape_stage);
    g_sa_truth_stage = std::max(0, loaded_truth_stage);
    g_sa_truth_post_battle = loaded_truth_post_battle;
    g_sa_skip_segments = std::max(0, loaded_skip_segments);
    g_sa_next_elite = loaded_next_elite;
    g_sa_heat_accel = loaded_heat_accel;
    g_sa_no_boss = loaded_no_boss;
    g_sa_permanent_shield = std::max(0, loaded_permanent_shield);
    g_sa_escape_battle_pending = loaded_escape_battle_pending;
    g_sa_escape_intro_pending = loaded_escape_intro_pending;
    g_sa_game_intro_pending = loaded_game_intro_pending;
    g_sa_event_epilogue = loaded_event_epilogue;

    state_ = loaded;
    normalizeUpgradedCards();
    recordCurrentOwnedCards();
    resumePage_ = (state_.page == AppPage::MainMenu) ? AppPage::Map : state_.page;
    if (error != nullptr) {
        *error = "";
    }
    return true;
}
