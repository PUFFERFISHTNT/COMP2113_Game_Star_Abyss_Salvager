# Star Abyss Salvager (Linux Release)

This folder is a trimmed, GitHub-ready release package for the playable terminal version of **Star Abyss Salvager**.

It is prepared for the final delivery constraint:

- Linux terminal target
- offline build
- `make` only
- no CMake
- no dependency installation from the internet

The UI is built with **FTXUI**, but the dependency is already vendored inside `vendor/FTXUI`, so the game can be compiled directly from this folder.

## What Is Included

This release intentionally keeps only the runtime-critical files:

- the modularized TUI entry point, gameplay engine, and UI code in `src/`
- the matching public headers in `include/`
- the static card data in `data/`
- an empty `save/` directory for runtime save files
- the vendored FTXUI source and headers required for offline compilation
- this `README.md`
- `DEVELOPMENT_STATUS.md`
- a `Makefile` that keeps the Linux delivery flow intact and also works in the current Windows MinGW environment

Legacy prototype files from the older non-TUI version are not included here.

## Requirements

The package is designed for a basic Linux development environment with:

- `g++` with C++17 support
- GNU `make`
- a UTF-8 terminal

No package manager steps are required.

## Build

Run all commands from the root of this `github_release` folder:

```bash
make
```

This builds the executable:

```bash
./star_abyss_salvager
```

## Run

```bash
./star_abyss_salvager
```

You should launch the game from this folder root so that the relative runtime paths continue to work:

- `save/*.sav`
- `data/leaderboard.txt` will be created automatically after the first completed run

## Clean

```bash
make clean
```

## Controls

The game supports both keyboard and mouse through FTXUI.

- `1` to `9`, `0`: choose menu entries or play cards
- `E`: end turn during battle
- `S`: open save manager
- `L`: open load manager
- `Esc`: return to the previous safe menu or the main menu
- `PgUp` / `PgDn`: scroll long pages using the terminal UI container
- `Ctrl+Up` / `Ctrl+Down`: fallback page scrolling for terminals where PageUp/PageDown is inconvenient
- mouse: click buttons, cards, map nodes, rewards, and save slots

## Current Playable Scope

This release already supports:

- main menu
- English / Chinese language toggle
- new run setup with difficulty and starter ship
- sector route selection
- battles with enemy intent display
- card rewards and relic rewards
- repair, salvage, event, market, elite, and boss node flow
- black market card buying and one-card purge service
- manual save names
- autosave slot
- save selection and load selection
- leaderboard writing after victory or defeat

## Important Runtime Notes

- Saves are written into `save/` and use a text-based `.sav` format.
- The game currently stores the player currency internally in a field still named `scrap`, but all player-facing UI presents it as **credits**.
- Gameplay data and rules are still compiled into the program for portability, but the implementation is now split across `src/engine/` and `src/ui/` rather than kept in two oversized source files.
- The purge screen now shows page progress and warns that starter cards may appear on later pages.
- Card text rendering was tightened so longer descriptions are shown more reliably in battle and reward views.

## If the Linux Machine Uses an Older GCC

The provided `Makefile` already links with `-lstdc++fs` to improve compatibility with environments where `std::filesystem` still needs an explicit filesystem library.

## Folder Layout

```text
github_release/
|-- Makefile
|-- README.md
|-- DEVELOPMENT_STATUS.md
|-- data/
|   `-- cards.json
|-- include/
|   |-- engine/
|   |   |-- game_battle.h
|   |   |-- game_content.h
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
|   |   |-- game_content.cpp
|   |   |-- game_encounter.cpp
|   |   |-- game_info.cpp
|   |   |-- game_reward.cpp
|   |   |-- game_route.cpp
|   |   `-- game_save.cpp
|   |-- ui/
|   |   |-- ui_art.cpp
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

## Development Handoff

For a much more detailed explanation of the architecture, gameplay systems, save format, technical debt, and next-step roadmap, read:

- `DEVELOPMENT_STATUS.md`
