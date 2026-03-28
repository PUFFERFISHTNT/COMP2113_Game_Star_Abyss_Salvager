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

- the active TUI entry point and gameplay code in `src/`
- the matching public headers in `include/`
- the leaderboard data file in `data/`
- an empty `save/` directory for runtime save files
- the vendored FTXUI source and headers required for offline compilation
- this `README.md`
- `DEVELOPMENT_STATUS.md`
- a Linux-only `Makefile`

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

- `data/leaderboard.txt`
- `save/*.sav`

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
- Most gameplay content is hardcoded in `src/tui_game.cpp` for portability, so no external card or enemy data files are required for the release build.

## If the Linux Machine Uses an Older GCC

The provided `Makefile` already links with `-lstdc++fs` to improve compatibility with environments where `std::filesystem` still needs an explicit filesystem library.

## Folder Layout

```text
github_release/
|-- Makefile
|-- README.md
|-- DEVELOPMENT_STATUS.md
|-- data/
|   `-- leaderboard.txt
|-- include/
|   |-- tui_app.h
|   `-- tui_game.h
|-- save/
|-- src/
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
