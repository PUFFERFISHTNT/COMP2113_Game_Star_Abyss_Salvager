#ifndef GAME_CARD_JSON_H
#define GAME_CARD_JSON_H

#include <string>
#include <vector>

#include "tui_game.h"

// This module owns JSON loading for original card definitions and JSON
// persistence for upgraded card definitions stored alongside save files.

bool LoadCardsFromJsonFile(const std::string& path,
                           std::vector<CardDef>* cards,
                           std::string* error);
bool LoadUpgradedCardsFromJsonFile(const std::string& path,
                                   std::vector<CardDef>* cards,
                                   std::string* error);
bool SaveUpgradedCardsToJsonFile(const std::string& path,
                                 const std::vector<CardDef>& cards,
                                 std::string* error);

#endif
