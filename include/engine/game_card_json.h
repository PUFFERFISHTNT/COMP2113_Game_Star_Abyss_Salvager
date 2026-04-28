#ifndef GAME_CARD_JSON_H
#define GAME_CARD_JSON_H

#include <string>
#include <vector>

#include "tui_game.h"

// This module owns JSON loading for the base card list plus the upgrade
// definitions authored alongside each card in data/cards.json.

// Looks up the cards JSON location without taking arguments and returns the path string.
std::string ResolveCardsJsonPath();

// Reads card and upgrade data from path into the provided vectors and reports success or failure.
bool LoadCardsFromJsonFile(const std::string& path,
                           std::vector<CardDef>* cards,
                           std::vector<CardDef>* upgrades,
                           std::string* error);

#endif
