#ifndef GAME_CARD_JSON_H
#define GAME_CARD_JSON_H

#include <string>
#include <vector>

#include "tui_game.h"

// This module owns JSON loading for the base card list plus the upgrade
// definitions authored alongside each card in data/cards.json.

// what it does: Resolves the path to the cards JSON data file.
// what the inputs are: None.
// what the outputs are: Returns the cards JSON file path (std::string).
std::string ResolveCardsJsonPath();

// what it does: Loads base card definitions and upgrade definitions from a JSON file.
// what the inputs are: path - JSON file path; cards - output base-card list; upgrades - output upgrade-card list; error - optional error message output.
// what the outputs are: Returns true when loading succeeds; otherwise false (bool).
bool LoadCardsFromJsonFile(const std::string& path,
                           std::vector<CardDef>* cards,
                           std::vector<CardDef>* upgrades,
                           std::string* error);

#endif
