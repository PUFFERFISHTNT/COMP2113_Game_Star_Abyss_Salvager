# Star Abyss Salvager

`Star Abyss Salvager` is a text-based roguelike deckbuilding game developed for the ENGG1340 / COMP2113 course project. The player pilots a salvager ship into the Star Abyss, chooses routes across dangerous sectors, fights hostile ships in turn-based card battles, upgrades the deck, collects relics, and progresses toward different story routes and endings.

## Team Members

The current local Git history shows contributions from:

- `Annie-goat`  Guo Xinyao(3036483478)
- `victorxsy` Xu Siyuan(3036482955)
- `Andy-MRX`  Hu Pengfei(3036588852)
- `Kong Ce` Kong Ce(3036483650)
- `PUFFERFISHTNT` Xu Yukuan(3036478148)

## Game Description

This game combines:

- route selection across sectors
- random encounters and branching events
- turn-based card combat
- deck growth through rewards, upgrades, and purges
- multiple ships with different combat styles
- multiple difficulty levels
- bilingual English / Chinese UI

The player starts a new run by choosing a ship and a difficulty, then advances through nodes such as battle, elite, salvage, repair, event, market, and boss nodes. Different event choices can unlock alternative routes, including the escape route and truth route.

## Features Implemented

### Core Gameplay

- `3` starter ships:
  - `Vanguard`: heat-oriented direct offense
  - `Drift Fox`: auxiliary ship / drone-focused playstyle
  - `Iron Bastion`: defensive form-switching playstyle
- `4` difficulty levels:
  - `Easy`
  - `Normal`
  - `Hard`
  - `Hell`
- sector-based map progression with route choice
- normal battles, elite battles, and boss battles
- enemy intent system
- reward screens for cards, credits, and relics
- black market services:
  - buy colorless cards
  - purge cards
  - upgrade cards
- repair station and event-based free upgrade / purge services
- multiple endings and special route events

### Combat Systems

- turn-based deck combat
- draw pile, discard pile, exile pile, and hand limit
- energy system
- shield system
- heat / overheat system
- auxiliary ship / drone system
- combat forms:
  - `Neutral`
  - `Overload`
  - `Defense`
- card upgrade system using definitions from `data/cards.json`

### Persistent Systems

- manual save / load
- autosave
- obtained card registry
- leaderboard
- event achievement system
- saved language preference

### UI / Quality-of-Life

- bilingual English / Chinese interface
- keyboard and mouse input
- owned card viewer
- full card library viewer
- scrollable long pages
- setup page descriptions for each difficulty and ship

## How the Course Coding Requirements Are Satisfied

### 1. Generation of Random Events

Randomness is used throughout the game, including:

- random route generation in `src/engine/game_route.cpp`
- random event selection
- random enemy selection for battles
- random reward card generation in `src/engine/game_reward.cpp`
- random relic rewards
- random market offers
- random branches in some event outcomes

This directly supports replayability and makes each run different.

### 2. Data Structures for Storing Data

The project uses multiple data structures to represent gameplay state:

- `RunState`, `PlayerState`, `EnemyState`, `EncounterState`, `RewardState`, and `InfoState` in `include/tui_game.h`
- `std::vector` for piles, logs, relics, route options, encounters, and achievements
- `std::array` for fixed UI button groups and page labels
- structured gameplay records such as `CardDef`, `CardView`, `NodeOption`, and `AuxiliaryShipState`

These structures support card combat, routing, save/load, and UI rendering.

### 3. Dynamic Memory Management

The game uses dynamic memory management through standard C++ facilities:

- `std::vector` and `std::string` for dynamically sized runtime data
- `std::shared_ptr` and `std::make_shared` in the FTXUI application shell
- dynamically built FTXUI component trees and modal stacks

This is essential because the number of cards, encounters, route options, save entries, and UI components changes during play.

### 4. File Input / Output

The project reads and writes several files:

- `data/cards.json`: loads card and upgrade definitions
- `save/*.sav`: manual saves and autosave
- `save/ui_language.txt`: saved language preference
- `save/obtained_cards.txt`: unlocked card registry
- `save/completed_events.txt`: completed event achievements
- `data/leaderboard.txt`: recorded run results

This supports persistent progression, save/load, and content loading.

### 5. Program Codes in Multiple Files

The project is modularized into multiple source files instead of one monolithic file:

- bootstrap:
  - `src/tui_main.cpp`
- app shell:
  - `src/tui_app.cpp`
- gameplay API:
  - `src/tui_game.cpp`
- engine modules:
  - `src/engine/*.cpp`
- UI modules:
  - `src/ui/*.cpp`
- headers:
  - `include/**/*.h`

This separation improves maintainability and keeps gameplay logic, save logic, routing logic, and rendering logic independent.

### 6. Multiple Difficulty Levels

The game supports `Easy`, `Normal`, `Hard`, and `Hell`.

- `Easy`: baseline forgiving mode
- `Normal`: enemies are more attack-oriented; elite and boss enemies may prepare a second intent
- `Hard`: stronger enemy stats and stronger enemy intent values
- `Hell`: same enemy strength as `Hard`, but enemy intents are hidden as `Unknown`

This requirement is fully implemented and exposed to the player on the setup page.

## Non-Standard Libraries

### FTXUI

This project uses the vendored [FTXUI](https://github.com/ArthurSonzogni/FTXUI) terminal UI library under `vendor/FTXUI/`.

FTXUI is used for:

- full-screen terminal rendering
- buttons, toggles, menus, and modal dialogs
- keyboard and mouse input handling
- page layout and panels
- scrollable pages

No extra download is needed because the library source is already included in this repository.

## Compilation Instructions

Run all commands from the project root:

```bash
make
```

This builds:

```bash
./star_abyss_salvager
```

### Requirements

- `g++` with C++17 support
- GNU `make`
- UTF-8 terminal

The build is offline-friendly. No internet installation is required.

## Execution Instructions

Run from the project root:

```bash
./star_abyss_salvager
```

Please launch the executable from the repository root so the following relative paths work correctly:

- `data/cards.json`
- `save/*.sav`
- `save/ui_language.txt`
- `save/obtained_cards.txt`
- `save/completed_events.txt`
- `data/leaderboard.txt`

## Controls

### General Controls

- `1` - `9`, `0`: choose visible menu entries or play cards
- `S`: open save dialog
- `L`: open load dialog
- `Esc`: close a modal, leave a safe page, or return to main menu depending on context
- `PgUp` / `PgDn`: scroll long pages
- `Ctrl+Up` / `Ctrl+Down`: fallback scrolling
- `[` / `]`: additional scrolling shortcuts
- mouse:
  - click menu buttons
  - click map nodes
  - click encounter options
  - click reward cards
  - click service cards

### Battle Controls

- `1` - `9`, `0`: play cards in hand
- `E`: end turn
- mouse: click cards, auxiliary ship targets, and battle UI buttons

### Service / Upgrade Screens

- black market upgrade screen:
  - `Esc`, `B`, or `0` returns to the broker
- repair / event upgrade screen:
  - `Esc` or `0` skips the service

## What Is Included in the Current Game

The current playable version includes:

- main menu
- run setup
- multilingual UI
- multiple difficulty levels
- three playable ships
- route progression across sectors
- random events and branching narrative
- turn-based card combat
- heat / overheat combat mechanics
- auxiliary ship mechanics
- relic rewards
- card upgrades
- purge systems
- save / load / autosave
- leaderboard viewing
- card library viewing
- event achievements

## Project Structure

```text
.
|-- Makefile
|-- README.md
|-- DEVELOPMENT_STATUS.md
|-- data/
|   |-- cards.json
|   `-- leaderboard.txt
|-- include/
|   |-- engine/
|   |-- ui/
|   |-- tui_app.h
|   `-- tui_game.h
|-- save/
|-- src/
|   |-- engine/
|   |-- ui/
|   |-- tui_app.cpp
|   |-- tui_game.cpp
|   `-- tui_main.cpp
`-- vendor/
    `-- FTXUI/
```

## Notes

- The player-facing currency is displayed as `credits`, although some internal variables still use the older name `scrap`.
- Saves are stored in a text-based `.sav` format.
- Card content is data-driven from `data/cards.json`, while route/event/enemy content is currently authored in C++.
- A more detailed technical handoff is available in `DEVELOPMENT_STATUS.md`.
