# DEVELOPMENT STATUS

## 1. Purpose of This Document

This file is a technical handoff for the current repository version of **Star Abyss Salvager**.

It documents:

- what the current playable build already does
- how the runtime is structured
- which files own which responsibilities
- how battle, map, event, reward, and save flows currently work
- what technical debt still exists
- how future development should continue without breaking the working core

This is intentionally much more detailed than a normal README because the project is already beyond the "blank skeleton" stage and now needs a stable engineering baseline for future expansion.

## 2. Release Scope

This repository is a **runtime-focused game package** for the current FTXUI version of the project.

It is no longer organized around only two giant source files. The codebase now has:

- a thin bootstrap layer in `src/tui_main.cpp`
- a top-level application controller in `src/tui_app.cpp`
- a central gameplay API in `src/tui_game.cpp`
- gameplay modules under `src/engine/`
- UI modules under `src/ui/`
- public headers under `include/`
- runtime card data in `data/cards.json`
- vendored FTXUI sources under `vendor/FTXUI`

Included:

- current FTXUI-based entry point
- modularized gameplay engine
- modularized UI controller and page renderers
- vendored offline FTXUI source
- static card data file
- runtime save directory
- cross-platform `Makefile` with POSIX and Windows branches

Excluded:

- older prototype gameplay files from the legacy non-TUI version
- CMake-based build flow
- test sources
- proposal and course paperwork
- any dependency that would require installation from the internet

This means the release package is optimized for final offline delivery, not for preserving every historical experiment.

## 3. High-Level Technical Summary

The current playable version is a **terminal roguelike deckbuilder** with a state-driven architecture.

At runtime there are four practical layers:

1. `src/tui_main.cpp`
   Boots FTXUI, enables mouse tracking, and starts the main screen loop.

2. `src/tui_app.cpp`
   Constructs the top-level `StarAbyssApp`, wires buttons and dialogs, and owns lifecycle behavior such as autosave and the UI ticker.

3. `src/tui_game.cpp`
   Hosts the `GameEngine` façade, the canonical `RunState`, and high-level gameplay entry points.

4. `src/engine/*.cpp` and `src/ui/*.cpp`
   Hold the real feature modules: battle rules, encounters, routes, rewards, saves, dev tools, localization, input routing, page renderers, card viewer, and shared panels.

This is a better split than the older architecture:

- bootstrap stays in `tui_main.cpp`
- app assembly stays in `tui_app.cpp`
- engine API stays centered in `tui_game.cpp` / `include/tui_game.h`
- gameplay rules are separated into engine modules
- rendering and input are separated into UI modules

That separation should be preserved. Future refactors should continue splitting by responsibility rather than re-centralizing logic into one file.

## 4. Core Source Files

### 4.1 `src/tui_main.cpp`

Responsibility:

- create `ftxui::ScreenInteractive::Fullscreen()`
- enable mouse support with `TrackMouse(true)`
- create the root UI component via `CreateApp(screen)`
- start the event loop

This file is intentionally tiny and should stay tiny.

### 4.2 `src/tui_app.cpp` and `include/ui/star_abyss_app.h`

Responsibility:

- construct `StarAbyssApp`
- wire FTXUI buttons, toggles, menus, modal stacks, and overlays
- manage language preference persistence
- own autosave hooks and save dialog entry points
- bridge between `GameEngine` and the modular UI render / input files
- host the long-lived UI state that is not part of `RunState`

Important note:

`StarAbyssApp` is now the real UI shell. Rendering and input are mostly split out into `src/ui/`, but the controller is still the main integration point.

### 4.3 `src/tui_game.cpp` and `include/tui_game.h`

Responsibility:

- define the public gameplay API used by the UI
- own the canonical `RunState`
- expose `GameEngine` methods for run flow, battles, encounters, rewards, saves, and information screens
- define the state structs used by both engine and UI
- aggregate the private helper surface implemented across `src/engine/*.cpp`

This pair is still the central gameplay authority, even though the implementation is now split across multiple engine files.

### 4.4 `src/engine/`

Responsibility:

- `game_battle.cpp`: turn flow, card effects, heat/form/auxiliary combat logic, enemy intent resolution
- `game_card_services.cpp`: market purge/upgrade services, event/repair service screens, deck-instance upgrade handling
- `game_card_json.cpp`: card JSON loader, path resolution, upgrade-definition parsing
- `game_content.cpp`: initialize enemies, ships, relics, obtained-card registry, and card pool loading
- `game_encounter.cpp`: node-specific encounter branching and event outcomes
- `game_info.cpp`: help text, summary screens, leaderboard presentation and writing
- `game_reward.cpp`: reward generation, random card/relic selection
- `game_route.cpp`: route generation, sector progression, special-route state handling
- `game_save.cpp`: save/load serialization, validation, save slot helpers
- `game_dev.cpp`: developer mode hooks and debug scene helpers

This directory is the main gameplay implementation surface.

### 4.5 `src/ui/`

Responsibility:

- `ui_input.cpp`: global keyboard/mouse routing, service-screen shortcuts, page-specific hotkeys
- `ui_sync.cpp`: refresh visible labels and active page state from `RunState`
- `ui_panels.cpp`: shared card/status/action panel building blocks
- `ui_localization.cpp`: UI and gameplay string translation helpers
- `ui_card_viewer.cpp`: owned-card viewer and library rendering
- `ui_save_dialog.cpp`: save/load modal rendering
- `ui_art.cpp`: ASCII art and decorative rendering helpers
- `ui_page_*.cpp`: page-specific renderers for main menu, setup, map, encounter, battle, reward, summary, and info screens

This directory now carries most presentation logic, which is a major improvement over the earlier all-in-one UI layout.

### 4.6 Public Headers and Runtime Assets

Important support files:

- `include/engine/*.h`: per-module engine declarations and high-level ownership notes
- `include/ui/*.h`: UI helper declarations and modal/render contracts
- `data/cards.json`: runtime card database and upgrade definitions
- `Makefile`: offline build entry point and object-layout definition
- `README.md`: quick-start and operator-facing build/run instructions
- `DEVELOPMENT_STATUS.md`: technical handoff and current architecture status

### 4.7 Current Repository Layout

The current top-level structure that matters for development is:

- `src/`
- `src/engine/`
- `src/ui/`
- `include/`
- `include/engine/`
- `include/ui/`
- `data/`
- `save/`
- `vendor/FTXUI/`
- `obj/` after builds

There is also an untracked local workspace file under `src/engine/*.code-workspace` in the current tree; it is editor metadata, not part of the runtime architecture.

## 5. Runtime State Architecture

The project now revolves around a single central state object: `RunState`.

That state is made of several sub-structures:

- `PlayerState`
- `EnemyState`
- `EncounterState`
- `RewardState`
- `InfoState`
- `AnimationState`
- route option list
- battle log list
- auxiliary ship list
- market offer cache

This gives the current game a flat, serializable data model that is easy to save and reload.

### 5.1 `AppPage`

`AppPage` is the top-level screen state machine:

- `MainMenu`
- `Setup`
- `Map`
- `Encounter`
- `Battle`
- `Reward`
- `Summary`
- `Info`

This page enum is extremely important because both the engine and UI use it to decide what input is valid and what screen should be drawn.

### 5.2 `PlayerState`

The player state currently tracks:

- ship identity and name
- HP and max HP
- shield
- energy and max energy
- heat and max heat
- overheat rounds
- credits, still stored internally in the `scrap` field
- next turn energy buffer
- next turn draw buffer
- opening battle bonuses
- next attack multiplier
- reward bonus from salvage effects
- attack count played this turn
- combat form state
- many temporary combat modifiers for heat, summon, overload, and auxiliary-ship effects
- blackbox protocol pending state
- emergency battery consumption flag
- relic list
- draw pile
- hand
- discard pile
- exile pile
- active ability list

This is no longer a minimal player struct. It now supports a richer combat ruleset with forms, auxiliary ships, temporary summons, heat triggers, and battle-only modifiers.

### 5.3 `EnemyState`

The enemy state tracks:

- enemy identity and display name
- behavior label
- HP and max HP
- shield
- heat and max heat
- enemy overheat state
- cooling cooldown
- current intent kind
- current intent label
- current intent value
- pattern index
- weaken turns
- shield lock turns

This supports intent forecasting and multi-turn enemy patterns.

### 5.4 `EncounterState`

This state powers repair nodes, events, salvage screens, and the black market UI.

It stores:

- encounter type
- encounter id
- title
- description
- visible choices
- visible detail strings
- hidden payload ids
- hidden payload values

This is flexible enough to power both button-list encounters and full-deck service galleries, but it is still string-heavy and should eventually become more strongly typed.

### 5.5 `RewardState`

This tracks:

- reward banner
- credits gained
- optional relic id
- card reward ids

It is used after battle, salvage rewards, and certain event outcomes.

### 5.6 `AnimationState`

This tracks:

- animation kind
- label
- magnitude
- direction
- current tick
- duration
- active flag

It is lightweight but effective for terminal battle feedback.

### 5.7 `RunState` as the Save Boundary

`RunState` itself also carries several fields that are not part of the nested sub-structs:

- sector and depth progression
- node count per sector
- battle turn count
- market upgrade and purge visit state
- pending sector advance / victory flags
- active node type and name
- current status line
- route options
- battle log
- current market offer ids
- auxiliary ship state

In practice, `RunState` is the authoritative save/load boundary for the current game.

## 6. Gameplay Systems Already Implemented

The game is no longer only a proposal skeleton. It already has a playable vertical slice.

### 6.1 Main Menu

Implemented:

- resume current run
- start new run
- load save
- leaderboard
- help
- English / Chinese language toggle
- exit

### 6.2 Run Setup

Implemented:

- difficulty selection
- starter ship selection

Current ships:

- Vanguard
- Drift Fox
- Iron Bastion

Current difficulty labels:

- Easy
- Normal
- Hard

### 6.3 Route / Node Flow

Implemented node families:

- normal battle
- elite battle
- salvage
- repair
- random event
- black market
- boss battle

Sector flow:

- choose one node from up to three route options
- clear several route depths
- fight a boss at sector end
- advance to the next sector or finish the run

Current sector structure is simplified but fully playable.

### 6.4 Battle System

The battle loop is already close to a simplified deckbuilder model inspired by **Slay the Spire**:

- start turn
- reset energy
- draw up to five cards
- show enemy intent
- play cards while energy allows
- end turn
- enemy resolves its intent
- next player turn begins

Implemented player-side mechanics:

- temporary shield
- energy spending
- draw and discard
- reshuffle discard pile into draw pile
- heat gain and heat reduction
- overheat self-damage
- combo attacks
- next-attack multipliers
- next-turn energy effects
- random-effect card behavior
- salvage bonus payout effects

Implemented enemy-side mechanics:

- attack intent
- shield intent
- heat intent
- EMP intent
- charge intent
- weakened attacks
- shield lock handling
- turn pattern cycling

Implemented end conditions:

- enemy death creates a reward screen
- player death creates a summary screen

### 6.5 Reward System

Implemented:

- card reward choice
- relic reward on elite and boss victories
- credit reward summary
- reward banner text

Reward screen already distinguishes:

- card reward
- card reward plus relic reward

### 6.6 Events and Non-Battle Encounters

Implemented encounter families include:

- repair station choices
- repair-station full-deck upgrade service
- salvage credits
- salvage card reward
- distress beacon outcomes
- strange signal outcomes
- event-based full-deck upgrade service
- reactor leak outcomes
- smuggler cache outcomes

These are currently authored directly in code and already support different risk / reward branches.

### 6.7 Black Market

Implemented:

- card purchase offers
- one-card purge service
- full-deck upgrade service
- purge pagination for larger decks
- visible current credit amount in the market UI
- explicit return action from the upgrade screen even when the player cannot afford the service

This is a real functional shop, not only a placeholder.

### 6.8 Save / Load

Implemented:

- manual named saves
- autosave slot
- save list browsing
- load selection
- slot overwrite behavior
- save directory creation
- content validation on load
- load feedback and error feedback

This is already one of the strongest systems in the current build.

### 6.9 Leaderboard

Implemented:

- appending run results to a runtime-created `data/leaderboard.txt`
- viewing leaderboard entries in the info screen

## 7. Content Database Status

The current release is **partly data-driven and partly code-authored**.

Loaded from `data/cards.json`:

- cards
- upgrade definitions for cards

Initialized in `GameEngine::initializeData()` inside `src/engine/game_content.cpp`:

- enemies
- ships
- relics

Advantages of the current approach:

- cards can now be balanced and upgraded without recompiling every definition into C++
- enemies, ships, and relics are still easy to validate because they remain code-authored
- easy offline delivery
- save validation can still check ids against known runtime databases

Disadvantages:

- the content pipeline is mixed instead of unified
- enemies, relics, and route/event authored content still require C++ edits
- translation coverage becomes manual
- some balancing work is now in JSON while some remains in code, which increases cognitive overhead

This hybrid approach is a reasonable intermediate state. The next clean architectural step would be to choose one consistent authoring boundary per content family rather than keeping an accidental mix.

## 8. Battle Rules in the Current Build

### 8.1 Turn Start

At the start of the player's turn:

- previous temporary player shield collapses
- player energy is recalculated from base energy plus pending bonuses
- first-turn relic and anomaly bonuses can apply
- shield-matrix style opening bonuses can apply
- up to five cards are drawn
- if heat is capped, draw can be reduced to four

### 8.2 Hand and Deck Rules

Current rules:

- normal draw target is 5 cards
- hand limit is 10 cards
- extra draws above the hand limit are discarded automatically
- discard pile reshuffles into draw pile when needed

### 8.3 Shield Rules

Current implementation uses temporary combat shield logic:

- player shield protects during the enemy turn
- player shield collapses at the start of the next player turn
- enemy shield also behaves as temporary and is cleared before the enemy performs its new action

This is simple and readable, though later versions may want permanent and temporary shield types to be split.

### 8.4 Heat Rules

Heat is already one of the core mechanics.

Current behavior:

- cards can increase heat
- cards can reduce heat
- relics can reduce end-of-turn heat
- when heat exceeds max heat, the overflow is converted into self-damage
- at max heat, next-turn draw can be reduced

This already creates a meaningful risk / reward axis.

### 8.5 Enemy Intent Rules

Each enemy displays:

- intent label
- intent type summary
- numeric value where relevant

Current intent categories:

- attack
- shield
- heat
- EMP
- charge

This is already enough for tactical planning and is one of the major readability improvements of the FTXUI version.

### 8.6 Reward Payout Rules

Current payout logic includes:

- base credits by sector
- bonus credits for elite and boss fights
- relic-modified credit gains
- card-modified salvage bonus gains

Internal note:

The variable is still named `scrap` in code for backward compatibility, but every player-facing surface should continue to say **credits**.

## 9. Current Card System Status

The card system is no longer a stub. It supports multiple effect fields at once.

Implemented effect categories include:

- direct damage
- multi-hit attacks
- shield gain
- healing
- card draw
- immediate energy gain
- next-turn energy gain
- heat gain
- heat reduction
- self-damage
- weaken effect
- enemy shield removal
- enemy shield lock
- next attack multiplier
- current shield to damage conversion
- heat halving
- full heat clear
- battle payout bonus
- random volatile effect
- auxiliary ship summoning and buffing
- form shifts and form toggles
- overload damage reduction
- blackbox temporary-copy effects
- upgrade-aware card instance references

This is a good systems foundation for future card pool expansion.

## 10. Current Enemy System Status

Enemy content already includes:

- normal enemies
- elite enemies
- boss enemies

Each enemy template currently defines:

- id
- name
- behavior tag
- base HP
- elite flag
- boss flag
- ordered intent pattern

Some enemies use deterministic patterns. The chaotic AI ship already uses a randomized intent selection model.

What is still missing compared to a richer final deckbuilder:

- multi-phase bosses
- deeper status interactions
- enemy-specific passive traits
- summoned units
- intent mutation based on HP thresholds

## 11. UI Architecture and Presentation Status

The FTXUI UI is currently one of the strongest parts of the project.

### 11.1 Current UI Features

Implemented:

- fullscreen interface
- mouse support
- button-based navigation
- keyboard shortcuts
- bilingual labels
- battle bridge layout
- ASCII ship and enemy art panels
- combat lane animation area
- card-panel style hand layout
- full-deck service gallery for upgrade / purge screens
- battle log
- save and load manager dialogs
- map dashboard
- reward screen with card cost and type

### 11.2 Current UI Strategy

The UI does not mutate game rules directly. Instead:

- the engine owns the state
- the UI reads the state
- the UI triggers engine actions through button callbacks and keyboard events
- `SyncUi()` rebuilds labels from the latest engine state

This is good architecture and should remain the rule.

### 11.3 Service Screen Interaction Status

The service-screen interaction layer is no longer only button-driven.

Current behavior for `market_upgrade`, `repair_upgrade`, and `event_upgrade`:

- card upgrades can be selected from a full-deck gallery with keyboard or mouse
- service exit actions are mapped to explicit keyboard shortcuts instead of relying on generic menu escape behavior
- black market upgrade uses `Esc`, `B`, or `0` to return to the broker
- repair-station and event upgrade screens use `Esc` or `0` to skip and continue
- the service action card is directly clickable with the mouse
- the service action target is resolved by payload id, not by assuming it is the last visible choice
- the first stray mouse release after entering a service screen is ignored until the page has seen an intentional hover or press

This matters because full-gallery service pages otherwise risk accidental upgrades or trapping the player in a credits-insufficient state.

### 11.4 Translation Strategy

The project currently has a mixed translation strategy:

- some UI labels are directly bilingual through `Tr(...)`
- many gameplay strings are translated through `TranslateGameText(...)`
- some dynamic lines are pattern-matched and rewritten in the UI layer

This works, but it is fragile.

Key limitation:

If new gameplay text is added and not registered in the translation helpers, Chinese mode can still show English fragments.

Future fix:

- move to id-based localization instead of text-matching localization

## 12. Save / Load Technical Design

The save system is text-based and versioned.

Current first line:

- `VERSION|3`

The save format serializes a large portion of live runtime state, including:

- page
- difficulty
- sector
- route depth
- node count
- battle turn counters
- pending sector flags
- active node identity
- status line
- player stats
- relic list
- draw pile
- hand
- discard pile
- enemy state
- route options
- encounter data
- reward data
- info panel data
- battle log
- animation state
- auxiliary ship state

This is already much more complete than a basic "HP only" save system.

### 12.1 Save Strengths

- human-readable
- easy to inspect manually
- easy to diff
- robust enough to restore a mid-run battle or encounter state
- validates ids against the runtime card / relic / enemy database on load

### 12.2 Save Weaknesses

- stringly typed
- delimiter-based
- no escaping layer for special characters
- schema evolution is manual
- content ids are safe, but arbitrary text fields could become harder to maintain as content grows

### 12.3 Recommendation for Future Save Work

Future upgrades should preserve the current strengths while improving maintainability:

- introduce explicit schema migration helpers
- move repeated state blocks into helper functions
- optionally switch to a simple tagged JSON-like format once the project no longer needs to minimize dependencies so aggressively

## 13. Data and File I/O Dependencies

The current repository has a small but important set of runtime file dependencies.

Required relative paths:

- `data/cards.json`
- `data/leaderboard.txt` (created automatically after the first finished run)
- `save/`
- `save/*.sav`
- `save/ui_language.txt` (created on first language preference write)
- `save/obtained_cards.txt` (created when card discovery data is persisted)

Important detail:

Gameplay content is no longer entirely compiled into the binary.

- card definitions are loaded from `data/cards.json`
- enemies, ships, relics, route logic, and event logic are still compiled in code

That means the executable should still be launched from the repository root so these relative paths resolve correctly.

## 14. Build and Portability Status

### 14.1 Windows Development Status

The project has already been developed and smoke-tested in the current Windows environment with MinGW `make` and `g++`.

### 14.2 Linux Delivery Status

This repository is still explicitly prepared for Linux delivery by:

- removing CMake from the release flow
- vendoring FTXUI source
- keeping a `make`-based offline build
- keeping the runtime layout relative and self-contained

### 14.3 Build Assumptions

The provided `Makefile` currently assumes:

- `g++`
- GNU make
- C++17 support
- pthread support
- a terminal environment that can display the TUI correctly

It also currently:

- branches between Windows and POSIX behavior
- writes objects into `obj/<compiler-target>/...`
- links vendored FTXUI sources directly instead of using a preinstalled library
- adds `-lstdc++fs` on non-Clang toolchains for compatibility with older filesystem implementations

## 15. Known Technical Debt and Limitations

These are important because the project is now playable enough that future bugs will mostly come from technical debt, not from missing scaffolding.

### 15.1 Internal Currency Naming Debt

Problem:

- player-facing UI says `credits`
- internal variable names still say `scrap`

Risk:

- future contributors may accidentally reintroduce mixed terminology

Recommended fix:

- rename the internal field in a coordinated refactor after save compatibility strategy is decided

### 15.2 Translation Debt

Problem:

- localization is partly string-based rather than id-based

Risk:

- English text can leak into Chinese mode when new content is added
- pattern-based translations become brittle over time

Recommended fix:

- introduce text ids and localized tables for cards, enemies, events, logs, and UI strings

### 15.3 Hardcoded Content Debt

Problem:

- enemies, relics, ships, routes, and event logic are still authored in C++
- cards are authored in JSON, so the content system is split across two styles

Risk:

- onboarding requires understanding both code-authored and JSON-authored content paths
- balancing effort is inconsistent between content families

Recommended fix:

- decide a clear long-term ownership model for each content family and document it explicitly

### 15.4 Engine Size Debt

Problem:

- the engine implementation is modularized, but `GameEngine` still exposes a very broad surface and many systems still share one state object

Risk:

- harder onboarding
- harder testing
- higher merge conflict rate

Recommended fix:

- continue separating helper logic by subsystem and add narrower testable helpers around battle, save, and event logic

### 15.5 UI Controller Size Debt

Problem:

- the UI is split into modules, but `StarAbyssApp` still holds a large amount of long-lived modal, hover, scroll, and page wiring state

Risk:

- future UI polish work becomes cumbersome

Recommended fix:

- keep moving page-specific interaction state behind smaller helper abstractions where possible

### 15.6 Test Coverage Debt

Problem:

- current verification is mostly smoke-level and manual-play level

Risk:

- regressions in save/load and battle math can slip in during feature work

Recommended fix:

- add targeted automated tests for battle rules, reward generation, and save round-trips

### 15.7 Terminal Size Assumption Debt

Problem:

- the UI is designed for reasonably wide terminals

Risk:

- smaller Linux terminals may clip panels or feel crowded

Recommended fix:

- add responsive layout fallbacks for smaller terminal widths

### 15.8 Documentation Drift Debt

Problem:

- the codebase now changes faster than the handoff docs, README notes, and issue summaries

Risk:

- future contributors may trust outdated architecture notes and edit the wrong files

Recommended fix:

- treat `README.md` and `DEVELOPMENT_STATUS.md` as part of the feature-change checklist when modules are moved or responsibilities change

## 16. What Is Strong Enough to Keep

The following parts are already good architectural decisions and should not be casually thrown away:

- central `RunState` model
- engine / UI separation
- explicit `AppPage` state machine
- offline vendored dependency strategy
- text-based complete save system
- battle intent visibility
- keyboard plus mouse support
- autosave plus manual save coexistence

These are valuable foundations. Future work should refine them, not replace them without a strong reason.

## 17. Recommended Next Development Priorities

The next work should be staged carefully so the playable baseline remains stable.

### Priority 1: Stabilize Current Playable Version

Goals:

- verify all remaining mixed-language strings
- test save/load from every page type
- test battle rewards, elite rewards, and boss rewards
- test black market purchase, purge, and upgrade-service edge cases
- test repair-station and event upgrade screens for accidental mouse input on entry
- test route progression across all three sectors

Reason:

The game is already feature-rich enough that stability now matters more than adding random extra content.

### Priority 2: Improve Gameplay Depth Without Large Refactors

Goals:

- add more cards
- add more enemy patterns
- add more events
- add more relic effects
- add clearer status effect presentation

Reason:

This improves replayability while keeping the existing architecture intact.

### Priority 3: Make Localization Robust

Goals:

- introduce localization ids
- separate UI strings from gameplay rule strings
- replace ad-hoc pattern matching with structured lookup

Reason:

This is one of the most obvious quality gaps still visible to players.

### Priority 4: Refine the Current Modular Split

Recommended follow-up areas:

- further isolate battle-effect helpers that are currently still broad inside `game_battle.cpp`
- narrow `GameEngine` helper surfaces where a full engine include is not required
- keep service-screen UI state from growing too far inside `StarAbyssApp`
- add clearer ownership notes when a new module is introduced

Reason:

The first modular split has already happened. The next step is to make those module boundaries cleaner rather than pretending the project is still monolithic.

### Priority 5: Move Content Out of Source

Goals:

- external card definitions
- external relic definitions
- external enemy pattern definitions
- external event definitions

Reason:

Once mechanics stabilize, content iteration speed becomes more important.

### Priority 6: Add Systematic Testing

Recommended test targets:

- draw / discard / reshuffle behavior
- heat overflow rules
- enemy intent damage math
- relic effect application
- save -> load -> state equivalence
- event branch outcomes
- market purchase and purge behavior

## 18. Concrete Future Feature Ideas

The current build is already playable, but there is still lots of room for improvement.

### 18.1 Battle Improvements

- richer attack animations
- status icons or badges
- more readable buff / debuff tracking
- multi-enemy battles
- card upgrade paths
- persistent and temporary shield distinction
- better boss phase transitions

### 18.2 Route Improvements

- more than three route choices when appropriate
- route preview and sector identity themes
- node rarity weighting
- explicit elite path risk planning
- sector modifiers or environmental hazards

### 18.3 Economy Improvements

- clearer shop inventory tiers
- more relic pricing variation
- card removal price scaling
- optional consumable items
- better reward pacing by sector

### 18.4 Meta and UX Improvements

- run history viewer
- richer leaderboard details
- deck inspection screen
- relic inspection screen
- pause / confirm dialogs for risky actions
- settings for animation speed

## 19. Suggested Roadmap for the Next Team Member

If a new contributor joins and wants the safest possible first tasks, this is the recommended order:

1. Play through the whole current build in both English and Chinese.
2. Fix any remaining mixed-language text.
3. Add regression tests for save/load and battle math.
4. Expand content in the existing architecture.
5. Only after that, start splitting large files and improving localization architecture.

This order minimizes the risk of destabilizing a working playable version.

## 20. Final Assessment

The project is no longer just a proposal or a shell. It now has:

- a working FTXUI game loop
- playable turn-based deck combat
- route progression
- rewards
- save/load
- leaderboard
- bilingual UI
- offline dependency packaging for Linux delivery

The most important remaining work is no longer "make something exist." It is:

- stabilize
- polish
- refactor carefully
- expand content in a controlled way

That is a healthy place for the project to be.
