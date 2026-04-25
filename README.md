# Star Abyss Salvager

This repository contains the current FTXUI-based playable version of **Star Abyss Salvager**.

It is structured as an offline-friendly terminal game project with:

- a thin bootstrap entry point in `src/tui_main.cpp`
- a top-level application controller in `src/tui_app.cpp`
- a central gameplay API in `src/tui_game.cpp`
- gameplay modules under `src/engine/`
- UI modules under `src/ui/`
- public headers under `include/`
- runtime card data in `data/cards.json`
- vendored FTXUI sources under `vendor/FTXUI/`

The project builds with `make` and does not require downloading dependencies from the internet.

## Requirements

The repository is designed for a basic development environment with:

- `g++` with C++17 support
- GNU `make`
- a UTF-8 terminal

No package-manager install steps are required because FTXUI is already vendored.

## Build

Run all commands from the repository root:

```bash
make
```

This builds:

```bash
./star_abyss_salvager
```

## Run

```bash
./star_abyss_salvager
```

Launch the executable from the repository root so relative runtime paths keep working correctly.

Important runtime paths:

- `data/cards.json`
- `save/*.sav`
- `save/ui_language.txt`
- `save/obtained_cards.txt`
- `data/leaderboard.txt` after the first finished run

## Clean

```bash
make clean
```

## Controls

The game supports both keyboard and mouse through FTXUI.

General:

- `1` to `9`, `0`: choose visible menu entries or play cards
- `S`: open the save dialog
- `L`: open the load dialog
- `Esc`: leave the current safe page, close a modal, or return to the main menu depending on context
- `PgUp` / `PgDn`: scroll long pages
- `Ctrl+Up` / `Ctrl+Down`: fallback scrolling for terminals where PageUp/PageDown is inconvenient
- mouse: click buttons, cards, map nodes, rewards, save slots, and service actions

Battle:

- `E`: end turn

Service upgrade screens:

- black market upgrade: `Esc`, `B`, or `0` returns to the broker
- repair-station and event upgrade screens: `Esc` or `0` skips the service
- the visible service action card is directly clickable with the mouse

## Current Playable Scope

The current build already supports:

- main menu
- English / Chinese language toggle
- new run setup with difficulty and starter ship selection
- route selection across battle, elite, salvage, repair, event, market, and boss nodes
- turn-based deck combat with enemy intent display
- reward screens with cards, credits, and relics
- black market card buying, purge service, and full-deck upgrade service
- repair-station and event full-deck upgrade services
- owned-card viewer and card library viewer
- autosave plus manual named saves
- save browsing and load browsing
- leaderboard writing and viewing
- developer mode hooks in the current codebase

## Important Runtime Notes

- Saves are written into `save/` using a text-based `.sav` format.
- Card definitions and upgrade definitions are loaded from `data/cards.json`.
- Older `save/*.cards.json` sidecar files are no longer used; upgrades are resolved from `data/cards.json` and stale sidecars are cleaned on the next save.
- The internal currency field is still named `scrap`, but every player-facing UI surface should display it as **credits**.
- The executable should be run from the repository root because card data, saves, language preferences, and leaderboard files all use relative paths.
- The build system is now modularized around `src/engine/` and `src/ui/`; this repository is no longer organized as only two oversized source files.

## Build and Portability Notes

- The project uses a `make`-based offline build flow.
- FTXUI is linked from vendored source instead of a system-installed package.
- The `Makefile` branches between Windows and POSIX behavior.
- On non-Clang toolchains, the build links `-lstdc++fs` for compatibility with older filesystem implementations.

## Repository Layout

```text
.
|-- Makefile
|-- README.md
|-- DEVELOPMENT_STATUS.md
|-- data/
|   `-- cards.json
|-- include/
|   |-- engine/
|   |   |-- game_battle.h
|   |   |-- game_card_json.h
|   |   |-- game_card_services.h
|   |   |-- game_content.h
|   |   |-- game_dev.h
|   |   |-- game_encounter.h
|   |   |-- game_info.h
|   |   |-- game_reward.h
|   |   |-- game_route.h
|   |   `-- game_save.h
|   |-- ui/
|   |   |-- star_abyss_app.h
|   |   |-- ui_art.h
|   |   |-- ui_localization.h
|   |   |-- ui_panels.h
|   |   `-- ui_save_dialog.h
|   |-- tui_app.h
|   `-- tui_game.h
|-- save/
|-- src/
|   |-- engine/
|   |   |-- game_battle.cpp
|   |   |-- game_card_json.cpp
|   |   |-- game_card_services.cpp
|   |   |-- game_content.cpp
|   |   |-- game_dev.cpp
|   |   |-- game_encounter.cpp
|   |   |-- game_info.cpp
|   |   |-- game_reward.cpp
|   |   |-- game_route.cpp
|   |   `-- game_save.cpp
|   |-- ui/
|   |   |-- ui_art.cpp
|   |   |-- ui_card_viewer.cpp
|   |   |-- ui_input.cpp
|   |   |-- ui_localization.cpp
|   |   |-- ui_page_battle.cpp
|   |   |-- ui_page_encounter.cpp
|   |   |-- ui_page_info.cpp
|   |   |-- ui_page_main_menu.cpp
|   |   |-- ui_page_map.cpp
|   |   |-- ui_page_reward.cpp
|   |   |-- ui_page_setup.cpp
|   |   |-- ui_page_summary.cpp
|   |   |-- ui_panels.cpp
|   |   |-- ui_save_dialog.cpp
|   |   `-- ui_sync.cpp
|   |-- tui_app.cpp
|   |-- tui_game.cpp
|   `-- tui_main.cpp
`-- vendor/
    `-- FTXUI/
        |-- LICENSE
        |-- include/
        `-- src/
```

Notes:

- `obj/` is generated after builds and is not listed above as source structure.
- There may be a local `.code-workspace` file under `src/engine/`; that is editor metadata, not part of the runtime architecture.

## Development Handoff

For a more detailed explanation of architecture, gameplay systems, save format, technical debt, and recommended next steps, read:

- `DEVELOPMENT_STATUS.md`
