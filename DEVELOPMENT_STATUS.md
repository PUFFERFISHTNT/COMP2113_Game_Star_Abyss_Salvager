# DEVELOPMENT STATUS

## 1. Purpose of This Document

This file is a technical handoff for the trimmed Linux release package of **Star Abyss Salvager**.

It documents:

- what the current playable build already does
- how the runtime is structured
- which files own which responsibilities
- how battle, map, event, reward, and save flows currently work
- what technical debt still exists
- how future development should continue without breaking the working core

This is intentionally much more detailed than a normal README because the project is already beyond the "blank skeleton" stage and now needs a stable engineering baseline for future expansion.

## 2. Release Scope

This `github_release` folder is not the full historical repository. It is a **minimal deliverable package** that keeps only the files needed to build and run the current FTXUI game on Linux with `make`.

Included:

- current FTXUI-based entry point
- current gameplay engine
- current UI controller
- vendored offline FTXUI source
- static card data file
- runtime save directory
- Linux-only build script

Excluded:

- older prototype gameplay files from the legacy non-TUI version
- CMake-based build flow
- test sources
- proposal and course paperwork
- any dependency that would require installation from the internet

This means the release package is optimized for final offline delivery, not for preserving every historical experiment.

## 3. High-Level Technical Summary

The current playable version is a **terminal roguelike deckbuilder** with a state-driven architecture.

At runtime there are three main layers:

1. `src/tui_main.cpp`
   Boots FTXUI, enables mouse tracking, and starts the main screen loop.

2. `src/tui_app.cpp`
   Owns presentation, layout, language switching, input routing, save/load dialogs, button wiring, and page rendering.

3. `src/tui_game.cpp`
   Owns gameplay state, battle rules, route generation, encounter resolution, reward generation, serialization, and leaderboard writing.

This is an intentionally good split:

- UI concerns stay in `tui_app.cpp`
- gameplay rules stay in `tui_game.cpp`
- bootstrapping stays in `tui_main.cpp`

That separation should be preserved in future work.

## 4. Core Source Files

### 4.1 `src/tui_main.cpp`

Responsibility:

- create `ftxui::ScreenInteractive::Fullscreen()`
- enable mouse support with `TrackMouse(true)`
- create the root UI component via `CreateApp(screen)`
- start the event loop

This file is intentionally tiny and should stay tiny.

### 4.2 `src/tui_app.cpp`

Responsibility:

- own the top-level `StarAbyssApp` controller
- translate `RunState` into FTXUI layouts
- map keyboard and mouse actions into engine calls
- manage save and load dialog state
- manage English / Chinese UI selection
- render card panels, battle bridge, map screens, reward screens, information screens, and summary screens
- provide battle animation framing, ASCII art placement, and general UX polish

Important note:

`tui_app.cpp` currently also contains a large amount of translation logic and UI helper logic. This is convenient for the playable version, but it is one of the biggest future refactor candidates.

### 4.3 `src/tui_game.cpp`

Responsibility:

- initialize all built-in cards, enemies, relics, and ship templates
- own the canonical run state
- generate route options
- resolve encounters
- run the battle turn loop
- apply card effects
- apply enemy intents
- produce rewards
- serialize saves
- deserialize saves
- validate loaded content references
- write leaderboard entries

This is currently the single gameplay authority.

### 4.4 `include/tui_game.h`

Responsibility:

- define all core state structs
- define `AppPage`
- expose the `GameEngine` public API

This header is the best quick map of the project state model.

### 4.5 `include/tui_app.h`

Responsibility:

- expose `CreateApp(ftxui::ScreenInteractive&)`

Minimal and stable.

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
- credits, still stored internally in the `scrap` field
- next turn energy buffer
- opening battle bonuses
- next attack multiplier
- reward bonus from salvage effects
- attack count played this turn
- emergency battery consumption flag
- relic list
- draw pile
- hand
- discard pile

This is enough to support the current combat loop and save system.

### 5.3 `EnemyState`

The enemy state tracks:

- enemy identity and display name
- behavior label
- HP and max HP
- shield
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

This is flexible, but it is also string-heavy and should eventually become more strongly typed.

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
- salvage credits
- salvage card reward
- distress beacon outcomes
- strange signal outcomes
- reactor leak outcomes
- smuggler cache outcomes

These are currently authored directly in code and already support different risk / reward branches.

### 6.7 Black Market

Implemented:

- card purchase offers
- one-card purge service
- purge pagination for larger decks
- visible current credit amount in the market UI

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

The current release is intentionally **code-authored**, not data-driven.

All of the following are initialized in `GameEngine::initializeData()`:

- cards
- enemies
- ships
- relics

Advantages of the current approach:

- no extra runtime parsing dependencies
- no risk of missing external content files
- easy offline delivery
- easier save validation because ids are known in one place

Disadvantages:

- harder balancing iteration
- more compile-time friction when adding content
- translation coverage becomes manual
- non-programmers cannot easily author new content

This is a major medium-term refactor target, but it was a reasonable choice for achieving a stable playable build quickly.

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

### 11.3 Translation Strategy

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

This is already much more complete than a basic "HP only" save system.

### 12.1 Save Strengths

- human-readable
- easy to inspect manually
- easy to diff
- robust enough to restore a mid-run battle or encounter state
- validates ids against the compiled card / relic / enemy database on load

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

The trimmed release has very small runtime file dependencies.

Required relative paths:

- `data/leaderboard.txt` (created automatically after the first finished run)
- `save/`

Important detail:

Most gameplay content is not loaded from disk. It is compiled into the binary.

That means the game is relatively robust for offline course deployment, as long as the executable is launched from the repository root.

## 14. Build and Portability Status

### 14.1 Windows Development Status

The project has already been developed and smoke-tested in the current Windows environment with MinGW `make` and `g++`.

### 14.2 Linux Delivery Status

This release package is explicitly prepared for Linux delivery by:

- removing CMake from the release flow
- vendoring FTXUI source
- adding a Linux-only `Makefile`
- keeping the runtime layout relative and self-contained

### 14.3 Build Assumptions

The provided release `Makefile` assumes:

- `g++`
- GNU make
- C++17 support
- pthread support
- a terminal environment that can display the TUI correctly

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

- cards, enemies, relics, and ships are all built in code

Risk:

- balancing becomes slower
- content authoring depends on C++ edits

Recommended fix:

- move gameplay content into structured data files after the rules stabilize

### 15.4 Engine Size Debt

Problem:

- `tui_game.cpp` currently owns a lot of responsibilities

Risk:

- harder onboarding
- harder testing
- higher merge conflict rate

Recommended fix:

- split into battle, encounter, rewards, serialization, and content database modules once the playable baseline is accepted

### 15.5 UI Controller Size Debt

Problem:

- `tui_app.cpp` contains rendering, translation, input routing, art generation, save dialog behavior, and page switching

Risk:

- future UI polish work becomes cumbersome

Recommended fix:

- extract reusable view helpers and localization helpers into separate files

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
- test black market purchase and purge edge cases
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

### Priority 4: Split Large Source Files

Recommended split for gameplay:

- `battle_system.cpp`
- `encounter_system.cpp`
- `reward_system.cpp`
- `save_system.cpp`
- `content_database.cpp`

Recommended split for UI:

- `ui_panels.cpp`
- `ui_battle.cpp`
- `ui_map.cpp`
- `ui_dialogs.cpp`
- `ui_localization.cpp`

Reason:

This will reduce merge conflicts and make the codebase easier to maintain.

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
