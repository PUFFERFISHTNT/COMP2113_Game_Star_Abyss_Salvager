#include "engine/game_save.h"
#include "engine/game_card_json.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>

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

    output << "VERSION|10\n";
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
           << enemy.patternIndex << "|" << enemy.weakenTurns << "|" << enemy.shieldLockTurns << "|"
           << enemy.heat << "|" << enemy.maxHeat << "|" << enemy.overheatRounds << "\n";

    output << "ROUTE|" << state_.routeOptions.size() << "\n";
    for (const NodeOption& option : state_.routeOptions) {
        output << "NODE|" << option.typeId << "|" << option.name << "|" << option.detail
               << "|" << option.danger << "\n";
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

    std::string upgraded_error;
    if (!SaveUpgradedCardsToJsonFile(UpgradedCardsJsonPath(path), upgradedCardPool_, &upgraded_error)) {
        if (error != nullptr) {
            *error = upgraded_error;
        }
        return false;
    }

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
            if (parts.size() >= 15) {
                loaded.enemy.heat = ToInt(parts[13], 0);
                loaded.enemy.maxHeat = ToInt(parts[14], 10);
            }
            if (parts.size() >= 16) {
                loaded.enemy.overheatRounds = ToInt(parts[15], 0);
            } else if (loaded.enemy.heat >= loaded.enemy.maxHeat) {
                loaded.enemy.overheatRounds = 1;
            }
        } else if (key == "NODE" && parts.size() >= 5) {
            loaded.routeOptions.push_back({parts[1], parts[2], parts[3], ToInt(parts[4], 1)});
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
    for (const std::string& cardId : loaded.upgradedCardTypes) {
        if (cardById(cardId) == nullptr) {
            if (error != nullptr) {
                *error = "Save file references an unknown upgraded card.";
            }
            return false;
        }
    }
    const std::vector<std::vector<std::string>*> piles = {
        &loaded.player.drawPile, &loaded.player.hand, &loaded.player.discardPile,
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
    upgradedCardPool_.clear();

    std::string upgraded_error;
    const std::string upgraded_json_path = UpgradedCardsJsonPath(path);
    if (std::filesystem::exists(upgraded_json_path)) {
        if (!LoadUpgradedCardsFromJsonFile(upgraded_json_path, &upgradedCardPool_, &upgraded_error)) {
            if (error != nullptr) {
                *error = upgraded_error;
            }
            return false;
        }
        for (const CardDef& card : upgradedCardPool_) {
            if (cardById(card.id) == nullptr) {
                if (error != nullptr) {
                    *error = "Upgraded card JSON references an unknown base card.";
                }
                return false;
            }
        }
    }
    normalizeUpgradedCards();
    recordCurrentOwnedCards();
    resumePage_ = (state_.page == AppPage::MainMenu) ? AppPage::Map : state_.page;
    if (error != nullptr) {
        *error = "";
    }
    return true;
}
