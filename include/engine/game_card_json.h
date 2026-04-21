#ifndef GAME_CARD_JSON_H
#define GAME_CARD_JSON_H

#include <string>
#include <vector>

#include "tui_game.h"

// This module owns JSON loading for the base card list plus the upgrade
// definitions authored alongside each card in data/cards.json.

bool LoadCardsFromJsonFile(const std::string& path,
                           std::vector<CardDef>* cards,
                           std::vector<CardDef>* upgrades,
                           std::string* error);

#endif
