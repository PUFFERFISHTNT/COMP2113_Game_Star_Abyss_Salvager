#include "engine/game_route.h"

#include <algorithm>

namespace {

// Shuffles the given vector with the supplied random generator and returns nothing.
template <typename T>
void ShuffleVector(std::vector<T>& values, std::mt19937& rng) {
    std::shuffle(values.begin(), values.end(), rng);
}

}  // namespace

// ============================================================
// Event system persistent state (shared with game_encounter.cpp)
// ============================================================
int g_sa_beacon_activated = 0;
int g_sa_escape_stage = 0;
int g_sa_truth_stage = 0;
bool g_sa_truth_post_battle = false;
int g_sa_skip_segments = 0;
bool g_sa_next_elite = false;
bool g_sa_heat_accel = false;
bool g_sa_no_boss = false;
int g_sa_permanent_shield = 0;
bool g_sa_escape_battle_pending = false;
bool g_sa_escape_intro_pending = true;
bool g_sa_game_intro_pending = true;
std::string g_sa_event_epilogue;

// Starts a new run from difficulty and ship id, resets story flags, and returns nothing.
void GameEngine::startNewRun(int difficulty, const std::string& shipId) {
    resetRunState();
    const ShipTemplate* ship = shipTemplateById(shipId);
    const ShipTemplate& chosenShip = ship ? *ship : ships_.front();

    g_sa_beacon_activated = 0;
    g_sa_escape_stage = 0;
    g_sa_truth_stage = 0;
    g_sa_truth_post_battle = false;
    g_sa_skip_segments = 0;
    g_sa_next_elite = false;
    g_sa_heat_accel = false;
    g_sa_no_boss = false;
    g_sa_permanent_shield = 0;
    g_sa_escape_battle_pending = false;
    g_sa_escape_intro_pending = true;
    g_sa_game_intro_pending = true;
    g_sa_event_epilogue.clear();

    state_.hasRun = true;
    state_.difficulty = difficulty;
    state_.sector = 1;
    state_.depth = 0;
    state_.nodesPerSector = 5;
    setupPlayerForShip(chosenShip, difficulty);
    generateRouteOptions();

    if (g_sa_game_intro_pending) {
        state_.encounter = {};
        state_.encounter.typeId = "event";
        state_.encounter.encounterId = "game_intro";
        state_.encounter.title = "Star Abyss Salvager";
        state_.encounter.description =
            "The year is 3071. Humanity spread across the stars centuries ago, "
            "but the golden age of expansion ended in silence. "
            "Deep in uncharted space lies the Star Abyss -- a vast, churning anomaly "
            "where ships vanish and space itself unravels. "
            "The Federation declared it a forbidden zone, but salvagers still slip through the cordon, "
            "drawn by the wrecks of lost fleets and the promise of fortune. "
            "You are one such salvager: broke, desperate, and deep in debt. "
            "Your ship is held together by scrap metal and stubbornness. "
            "One last haul in the Abyss could clear your ledger -- or bury you alongside the others. "
            "The entry vector is locked. Your reactor hums. "
            "Whatever lies inside, there is no turning back now.";
        state_.encounter.choices = {"Begin voyage"};
        state_.encounter.details = {"Enter the Star Abyss and start your salvage run."};
        state_.page = AppPage::Encounter;
        resumePage_ = AppPage::Encounter;
    } else {
        state_.page = AppPage::Map;
        resumePage_ = AppPage::Map;
    }
    state_.statusLine = "Run initialized. Chart your first jump.";
}

// Moves to the main menu while preserving a resume page when a run exists and returns nothing.
void GameEngine::showMainMenu() {
    if (state_.hasRun && state_.page != AppPage::MainMenu && state_.page != AppPage::Setup && state_.page != AppPage::Info) {
        resumePage_ = state_.page;
    }
    state_.page = AppPage::MainMenu;
}

// Restores the active run to its resume page without input and returns nothing.
void GameEngine::resumeRun() {
    if (!state_.hasRun) {
        return;
    }
    state_.page = resumePage_;
}

// Chooses the map node at the given index, starts its content, and returns nothing.
void GameEngine::chooseNode(size_t index) {
    if (state_.page != AppPage::Map || index >= state_.routeOptions.size()) {
        return;
    }

    const NodeOption node = state_.routeOptions[index];
    state_.activeNodeType = node.typeId;
    state_.activeNodeName = node.name;
    state_.statusLine = "Jump vector locked for " + node.name + ".";

    if (node.typeId == "battle" || node.typeId == "elite" || node.typeId == "boss") {
        startBattle(node.typeId, node.enemyId);
    } else {
        startEncounterForNode(node);
    }
}

// Sends the engine back to the main menu without input and returns nothing.
void GameEngine::returnToMenu() {
    showMainMenu();
}

// Opens the setup page for another run without input and returns nothing.
void GameEngine::startAnotherRun() {
    state_.page = AppPage::Setup;
}

// Builds route options for the current sector and story state and returns nothing.
void GameEngine::generateRouteOptions() {
    state_.routeOptions.clear();

    // --- Escape route ---
    if (g_sa_escape_stage >= 1 && g_sa_escape_stage <= 8) {
        if (g_sa_escape_intro_pending) {
            state_.routeOptions.push_back({"event", "Escape Decision", "The Ark's beacon falls silent. A new heading crystallizes on the nav console.", 1, ""});
            return;
        }
        if (g_sa_escape_battle_pending) {
            std::uniform_int_distribution<int> typeRoll(1, 100);
            if (typeRoll(rng_) <= 35) {
                state_.routeOptions.push_back({"elite", "Escape Blockade", "A heavily armed ship blocks your escape route.", 3, ""});
            } else {
                state_.routeOptions.push_back({"battle", "Escape Skirmish", "Hostile contacts on the escape vector. No way around them.", 2, ""});
            }
            return;
        }
        switch (g_sa_escape_stage) {
            case 1: state_.routeOptions.push_back({"event", "Federal Patrol Outpost", "A federation checkpoint blocks the route. Patrol drones sweep the area.", 3, ""}); break;
            case 2: state_.routeOptions.push_back({"event", "Abandoned Smuggling Port", "A deserted port with some supplies left behind.", 1, ""}); break;
            case 3: state_.routeOptions.push_back({"event", "Deep Space Radiation Belt", "A deadly band of cosmic radiation stretches across the path ahead.", 3, ""}); break;
            case 4: state_.routeOptions.push_back({"event", "Pirate Pursuit Ship", "A pirate craft locks onto your trail and closes in fast.", 4, ""}); break;
            case 5: state_.routeOptions.push_back({"event", "Drifting Escape Pod", "A damaged escape pod tumbles through the void with intact supplies.", 1, ""}); break;
            case 6: state_.routeOptions.push_back({"event", "Unstable Jump Point", "An unstable warp corridor shimmers ahead, offering a risky shortcut.", 3, ""}); break;
            case 7: state_.routeOptions.push_back({"event", "Star Abyss Edge Barrier", "An energy barrier marks the edge of the Star Abyss.", 4, ""}); break;
            case 8: state_.routeOptions.push_back({"event", "Temporary Safe Station", "A small outpost at the edge of the abyss offers one last chance to prepare.", 2, ""}); break;
        }
        return;
    }

    // --- Truth route ---
    if (g_sa_truth_stage >= 1 && g_sa_truth_stage <= 6) {
        if (!g_sa_truth_post_battle) {
            switch (g_sa_truth_stage) {
                case 1: state_.routeOptions.push_back({"elite", "Memory Remnant", "A twisted echo of forgotten memories blocks the corridor ahead.", 4, ""}); break;
                case 2: state_.routeOptions.push_back({"elite", "Rogue Maintenance AI", "A malfunctioning AI system guards the old bridge with lethal force.", 4, ""}); break;
                case 3: state_.routeOptions.push_back({"elite", "Phantom Guardian", "An illusory sentinel materializes, defending the captain's sanctum.", 5, ""}); break;
                case 4: state_.routeOptions.push_back({"elite", "Space Ripper", "A spatial anomaly tears through reality, attacking anything in its path.", 5, ""}); break;
                case 5: state_.routeOptions.push_back({"elite", "Pseudo-God Apostle", "A being of immense power emerges from the origin horizon.", 5, ""}); break;
                case 6: state_.routeOptions.push_back({"boss", "Creation Core Guardian", "The ultimate guardian of the Ark's creation core unfolds in the void.", 6, "creation_core_guardian"}); break;
            }
        } else {
            switch (g_sa_truth_stage) {
                case 1: state_.routeOptions.push_back({"event", "Creation Memory Fragment", "The corridor pulses with ancient data streams from a lost civilization.", 2, ""}); break;
                case 2: state_.routeOptions.push_back({"event", "Rogue Guardian AI", "The bridge's AI system lies defeated, its data banks exposed.", 2, ""}); break;
                case 3: state_.routeOptions.push_back({"event", "First Captain's Consciousness", "An echo of the original captain lingers in the ship's neural network.", 2, ""}); break;
                case 4: state_.routeOptions.push_back({"event", "Spatial Collapse Point", "Reality bends and tears around you. The fabric of space is failing.", 3, ""}); break;
                case 5: state_.routeOptions.push_back({"event", "Star Abyss Birth Truth", "You have reached the origin horizon. The truth of creation lies before you.", 3, ""}); break;
            }
        }
        return;
    }

    // --- Normal route: boss ---
    if (state_.depth >= state_.nodesPerSector) {
        if (g_sa_no_boss) {
            state_.routeOptions.push_back({"event", "Strange Signal", "Anomalous data bleeds across the sensor grid.", 2, ""});
            return;
        }
        if (state_.sector == 1) {
            state_.routeOptions.push_back({"boss", "Dread Frigate Omega", "A warship blocks the sector jump.", 4, "dread_frigate_omega"});
        } else if (state_.sector == 2) {
            state_.routeOptions.push_back({"boss", "The Furnace Carrier", "Heat wakes ripple through a carrier hull.", 5, "furnace_carrier"});
        } else {
            state_.routeOptions.push_back({"boss", "Abyss Core Sentinel", "The final guardian unfolds in the void.", 6, "abyss_core_sentinel"});
        }
        return;
    }

    // --- Normal route options ---
    std::vector<std::string> types = {"battle", "salvage", "repair", "event", "elite"};
    if (state_.sector >= 2) {
        types.push_back("market");
    }
    bool force_next_elite = false;
    if (g_sa_next_elite) {
        force_next_elite = true;
        g_sa_next_elite = false;
    }
    ShuffleVector(types, rng_);
    if (force_next_elite) {
        types.erase(std::remove(types.begin(), types.end(), "elite"), types.end());
        types.resize(std::min<size_t>(2, types.size()));
        types.push_back("elite");
        ShuffleVector(types, rng_);
    } else {
        types.resize(std::min<size_t>(3, types.size()));
    }

    for (const std::string& type : types) {
        if (type == "battle") {
            std::vector<std::string> battle_ids;
            if (state_.sector == 1) {
                battle_ids = {"pirate_needle", "scrapbreaker_tug", "signal_leech_mk2"};
            } else if (state_.sector == 2) {
                battle_ids = {"cinder_hauler", "furnace_tender", "ember_rack_skiff"};
            } else {
                battle_ids = {"phase_skewer", "null_choir", "core_warder"};
            }
            std::uniform_int_distribution<size_t> pick(0, battle_ids.size() - 1);
            const std::string enemy_id = battle_ids[pick(rng_)];
            const EnemyTemplate* enemy = enemyTemplateById(enemy_id);
            const std::string enemy_name = enemy != nullptr ? enemy->name : "Hostile Contact";
            state_.routeOptions.push_back({"battle",
                                           enemy_name,
                                           "Hostile signatures spike ahead.",
                                           2,
                                           enemy_id});
        } else if (type == "elite") {
            std::string enemy_id = "rogue_miner_colossus";
            if (state_.sector == 2) {
                enemy_id = "slag_tribunal";
            } else if (state_.sector >= 3) {
                enemy_id = "reality_harrower";
            }
            const EnemyTemplate* enemy = enemyTemplateById(enemy_id);
            const std::string enemy_name = enemy != nullptr ? enemy->name : "Elite Intercept";
            state_.routeOptions.push_back({"elite",
                                           enemy_name,
                                           "A heavy hostile vessel blocks the lane.",
                                           3,
                                           enemy_id});
        } else if (type == "salvage") {
            state_.routeOptions.push_back({"salvage", "Derelict Wreck", "Broken hulls drift in a silent salvage field.", 1, ""});
        } else if (type == "repair") {
            state_.routeOptions.push_back({"repair", "Repair Station", "A maintenance dock answers your ping.", 1, ""});
        } else if (type == "market") {
            state_.routeOptions.push_back({"market", "Black Market", "Encrypted trade beacons flare nearby.", 2, ""});
        } else {
            std::uniform_int_distribution<int> storyBossRoll(1, 100);
            if (storyBossRoll(rng_) <= 18) {
                std::string storyBossId;
                std::string storyBossName;
                if (state_.sector == 1) {
                    storyBossId = "the_pale_broker";
                    storyBossName = "Broker Echo";
                } else if (state_.sector == 2) {
                    storyBossId = "saint_of_cinders";
                    storyBossName = "Cinder Shrine Signal";
                } else {
                    std::uniform_int_distribution<int> deepRoll(0, 1);
                    if (deepRoll(rng_) == 0) {
                        storyBossId = "the_choir_beneath";
                        storyBossName = "Beneath Choir Resonance";
                    } else {
                        storyBossId = "salvager_zero";
                        storyBossName = "Prototype Zero Trace";
                    }
                }
                state_.routeOptions.push_back({"event",
                                               storyBossName,
                                               "A high-priority anomalous war signature requests immediate attention.",
                                               4,
                                               storyBossId});
            } else {
                state_.routeOptions.push_back({"event", "Strange Signal", "Anomalous data bleeds across the sensor grid.", 2, ""});
            }
        }
    }

    if (state_.player.hp < state_.player.maxHp / 2) {
        const bool hasRepair = std::any_of(state_.routeOptions.begin(), state_.routeOptions.end(), [](const NodeOption& option) {
            return option.typeId == "repair";
        });
        if (!hasRepair && !state_.routeOptions.empty()) {
            state_.routeOptions.back() = {"repair", "Emergency Drydock", "A lightly armed station can patch your hull.", 1, ""};
        }
    }
}

// Starts the selected node's battle, service, story event, or encounter and returns nothing.
void GameEngine::startEncounterForNode(const NodeOption& node) {
    if (node.typeId == "battle" || node.typeId == "elite" || node.typeId == "boss") {
        startBattle(node.typeId, node.enemyId);
        return;
    }
    state_.encounter = {};
    state_.encounter.typeId = node.typeId;
    state_.encounter.title = node.name;
    state_.encounter.description = node.detail;
    state_.page = AppPage::Encounter;
    resumePage_ = AppPage::Encounter;

    // --- Repair ---
    if (node.typeId == "repair") {
        state_.repairStationPatchUsed = false;
        state_.repairStationVentUsed = false;
        state_.repairStationPlatingUsed = false;
        state_.repairStationUpgradeUsed = false;
        openRepairStation();
        return;
    }

    // --- Salvage ---
    if (node.typeId == "salvage") {
        state_.encounter.choices = {"Strip Credits", "Recover Module Rack", "Leave It"};
        state_.encounter.details = {
            "Gain immediate credits, but risk a little reactor heat.",
            "Turn the wreck into a fresh card reward.",
            "Avoid the unknown and conserve resources.",
        };
        return;
    }

    // --- Market ---
    if (node.typeId == "market") {
        clearMarketState();
        state_.marketOfferCardIds = randomMarketCardChoices(3);
        openMarketShop();
        return;
    }

    if (node.typeId == "event" && !node.enemyId.empty()) {
        const EnemyTemplate* forcedEnemy = enemyTemplateById(node.enemyId);
        if (forcedEnemy != nullptr) {
            state_.encounter.encounterId = "event_story_boss";
            state_.encounter.title = node.name.empty() ? "Anomalous War Signature" : node.name;
            state_.encounter.description =
                "The signal condenses into a hostile flagship profile. "
                "Engaging now could uncover hidden truths, but retreat is still possible.";
            state_.encounter.choices = {"Engage", "Withdraw"};
            state_.encounter.details = {
                "Force a boss battle against " + forcedEnemy->name + ".",
                "Abort contact and continue your route.",
            };
            state_.encounter.payloadIds = {node.enemyId, ""};
            state_.encounter.payloadValues = {0, 0};
            return;
        }
    }

    // --- Escape Route Prologue ---
    if (g_sa_escape_stage >= 1 && g_sa_escape_intro_pending) {
        state_.encounter.typeId = "event";
        state_.encounter.encounterId = "escape_prologue";
        state_.encounter.title = "Escape Decision";
        state_.encounter.description =
            "You sealed the Ark's beacon and cut the signal dead. "
            "The surrounding space warps and groans as the abyss loses its anchor. "
            "Your navigation computer recalculates: a narrow corridor toward open space "
            "still exists, but it threads through hostile territory, federal patrols, "
            "radiation belts, and pirate hunting grounds. "
            "There will be no resupply, no allies, no second chances. "
            "You grip the helm and set the heading. The escape begins now.";
        state_.encounter.choices = {"Engage engines"};
        state_.encounter.details = {"Commit to the escape route. There is no turning back."};
        return;
    }

    // --- Escape Route Events ---
    if (g_sa_escape_stage >= 1 && g_sa_escape_stage <= 8) {
        state_.encounter.typeId = "event";
        switch (g_sa_escape_stage) {
            case 1:
                state_.encounter.encounterId = "escape_1";
                state_.encounter.title = "Federal Patrol Outpost";
                state_.encounter.description = "A federation checkpoint blocks the route. Patrol drones sweep the area.";
                state_.encounter.choices = {"Pay transit fee", "Power down and drift", "Force through"};
                state_.encounter.details = {"Costs 12 credits.", "+4 heat.", "Forced normal battle. Defeat: game over."};
                break;
            case 2:
                state_.encounter.encounterId = "escape_2";
                state_.encounter.title = "Abandoned Smuggling Port";
                state_.encounter.description = "A deserted port with some supplies left behind.";
                state_.encounter.choices = {"Emergency repairs", "Clean heat exchangers", "Scavenge supplies"};
                state_.encounter.details = {"+10 HP.", "Reduce 6 heat.", "+14 credits."};
                break;
            case 3:
                state_.encounter.encounterId = "escape_3";
                state_.encounter.title = "Deep Space Radiation Belt";
                state_.encounter.description = "A deadly band of cosmic radiation stretches across the path ahead.";
                state_.encounter.choices = {"Full speed through", "Slow detour", "Activate anti-rad shield"};
                state_.encounter.details = {"Skip 1 segment, lose 6 HP.", "+3 heat.", "+2 permanent max HP, costs 10 credits."};
                break;
            case 4:
                state_.encounter.encounterId = "escape_4";
                state_.encounter.title = "Pirate Pursuit Ship";
                state_.encounter.description = "A pirate craft locks onto your trail and closes in fast.";
                state_.encounter.choices = {"Pay them off", "Jettison cargo", "Counter-attack"};
                state_.encounter.details = {"Costs 15 credits.", "Delete 1 random card.", "Forced elite battle. Defeat: game over."};
                break;
            case 5:
                state_.encounter.encounterId = "escape_5";
                state_.encounter.title = "Drifting Escape Pod";
                state_.encounter.description = "A damaged escape pod tumbles through the void with intact supplies.";
                state_.encounter.choices = {"Take medical kit", "Drain fuel", "Take emergency chip"};
                state_.encounter.details = {"+8 HP.", "+2 initial energy next battle.", "+3 HP recovery next battle."};
                break;
            case 6:
                state_.encounter.encounterId = "escape_6";
                state_.encounter.title = "Unstable Jump Point";
                state_.encounter.description = "An unstable warp corridor shimmers ahead, offering a risky shortcut.";
                state_.encounter.choices = {"Force jump", "Stabilize coordinates", "Abandon jump"};
                state_.encounter.details = {"Skip 2 segments, +7 heat.", "Reduce 5 heat, costs 8 credits.", "Gain 1 random card, lose 5 HP."};
                break;
            case 7:
                state_.encounter.encounterId = "escape_7";
                state_.encounter.title = "Star Abyss Edge Barrier";
                state_.encounter.description = "An energy barrier marks the edge of the Star Abyss.";
                state_.encounter.choices = {"Energy breach", "Spend supplies to breach", "Find a gap"};
                state_.encounter.details = {"+6 heat.", "Costs 18 credits.", "Lose 5 HP."};
                break;
            case 8:
                state_.encounter.encounterId = "escape_8";
                state_.encounter.title = "Temporary Safe Station";
                state_.encounter.description = "A small outpost at the edge of the abyss offers one last chance to prepare.";
                state_.encounter.choices = {"Repair hull", "Optimize engines", "Remove junk cards"};
                state_.encounter.details = {"+5 HP, costs 12 credits.", "+1 initial energy, +2 heat.", "Delete 1 low-quality card, lose 4 HP."};
                break;
        }
        return;
    }

    // --- Truth Route Events (post-battle, stages 1-5) ---
    if (g_sa_truth_stage >= 1 && g_sa_truth_stage <= 5 && g_sa_truth_post_battle) {
        state_.encounter.typeId = "event";
        switch (g_sa_truth_stage) {
            case 1:
                state_.encounter.encounterId = "truth_1";
                state_.encounter.title = "Creation Memory Fragment";
                state_.encounter.description =
                    "What floats in the Star Abyss is not mere ship wreckage. "
                    "They are the remains of the first interstellar colonization fleet, the Ark. "
                    "Every anomalous signal you collected is its voice echoing across billions of spatial residues.";
                state_.encounter.choices = {"Merge memories", "Extract energy", "Save the log"};
                state_.encounter.details = {"Upgrade 2 random cards.", "Permanent max HP +6.", "Gain 1 rare card."};
                break;
            case 2:
                state_.encounter.encounterId = "truth_2";
                state_.encounter.title = "Rogue Guardian AI";
                state_.encounter.description =
                    "The Ark did not crash by accident. It entered this space deliberately. "
                    "The Federation's Restricted Zone was created to conceal its existence "
                    "and prevent anyone from discovering its true mission.";
                state_.encounter.choices = {"Force breakthrough", "Override permissions", "Sneak around"};
                state_.encounter.details = {"Gain a rare relic, lose 8 HP.", "+2 initial energy next battle, +5 heat.", "Costs 12 credits."};
                break;
            case 3:
                state_.encounter.encounterId = "truth_3";
                state_.encounter.title = "First Captain's Consciousness";
                state_.encounter.description =
                    "The Ark's mission was not colonization but a creation experiment. "
                    "Humanity attempted to use the Ark Core to forge a stable new universe. "
                    "The Star Abyss is what remained after that experiment failed.";
                state_.encounter.choices = {"Listen to the truth", "Gain ship permissions", "Drain core energy"};
                state_.encounter.details = {"Gain 1 rare card.",
                                            "+4 permanent opening shield. Stacks with other opening shield effects.",
                                            "Reduce 10 heat, +10 HP."};
                break;
            case 4:
                state_.encounter.encounterId = "truth_4";
                state_.encounter.title = "Spatial Collapse Point";
                state_.encounter.description =
                    "The Star Abyss's expansion, spatial distortions, and ship malfunctions "
                    "are not hazards but a newborn universe struggling and spiraling out of control. "
                    "Every step deeper draws you closer to this new world's heart.";
                state_.encounter.choices = {"Engines full to stabilize", "Dump supplies to brace", "Emergency dodge"};
                state_.encounter.details = {"+8 heat.", "Costs 20 credits.", "Lose 9 HP."};
                break;
            case 5:
                state_.encounter.encounterId = "truth_5";
                state_.encounter.title = "Star Abyss Birth Truth";
                state_.encounter.description =
                    "The Ark Core is neither an energy source nor a weapon. "
                    "It is the cosmic singularity of a new world. "
                    "Whoever controls it can dictate the rules of this space.";
                state_.encounter.choices = {"Accept the truth", "Extract the origin"};
                state_.encounter.details = {"All cards permanently enhanced.", "Gain Origin relic, lose 10 HP."};
                break;
        }
        return;
    }

    // --- Random Events (A/B/C/D weighted) ---
    std::uniform_int_distribution<int> eventRoll(1, 100);
    const int r = eventRoll(rng_);

    if (r <= 15) {
        state_.encounter.encounterId = "event_a1";
        state_.encounter.title = "Distress Beacon";
        state_.encounter.description = "A faint distress signal pulses from a wrecked fleet. Someone may still be alive.";
        state_.encounter.choices = {"Go to rescue", "Relay coordinates", "Ignore and leave"};
        state_.encounter.details = {"Gain 10 HP and 12 credits.", "Reduce 3 heat and gain 1 random card.", "Continue without delay."};
    } else if (r <= 25) {
        state_.encounter.encounterId = "event_a2";
        state_.encounter.title = "Ghost Ship";
        state_.encounter.description = "A derelict warship drifts silently. Its hull is intact but shows no life signs.";
        state_.encounter.choices = {"Board and explore", "Send a hail", "Open fire"};
        state_.encounter.details = {"Lose 5 HP but gain 1 rare relic.", "Gain 8 HP and 2 heat.", "Forced into a small battle. Defeat: game over."};
    } else if (r <= 35) {
        state_.encounter.encounterId = "event_b1";
        state_.encounter.title = "Strange Signal";
        state_.encounter.description = "An anomalous frequency keeps syncing to your navigation core.";
        state_.encounter.choices = {"Trace the signal", "Block interference", "Record the spectrum"};
        state_.encounter.details = {"Elite encounter chance increased next. Gain 1 random card.", "Reduce 4 heat.", "Gain 8 credits."};
    } else if (r <= 45) {
        state_.encounter.encounterId = "event_b2";
        state_.encounter.title = "Space Fold";
        state_.encounter.description = "A spatial anomaly warps the region ahead. Energy readings are off the charts.";
        state_.encounter.choices = {"Enter the fold", "Stabilize the space", "Steer clear"};
        state_.encounter.details = {"Skip 1 segment, +5 heat, gain 1 rare card.", "-4 heat, +2 permanent shield, costs 10 credits.", "Next battle's first attack enhanced."};
    } else if (r <= 65) {
        state_.encounter.encounterId = "event_b3";
        state_.encounter.title = "Ark Beacon";
        state_.encounter.description = "A massive signal source emerges from the void. This is the Ark's beacon.";
        state_.encounter.choices = {"Activate the beacon", "Dismantle the beacon", "Block the beacon"};
        state_.encounter.details = {"Gain rare card, +8 heat. Unlocks true ending.", "+25 credits, gain relic, -6 HP.", "Trigger escape route. No more bosses."};
    } else if (r <= 73) {
        state_.encounter.encounterId = "event_c1";
        state_.encounter.title = "Reactor Leak";
        state_.encounter.description = "Warning lights flash as unstable coolant leaks from the reactor.";
        state_.encounter.choices = {"Manual voltage stabilization", "Activate emergency cooling", "Silence the alarm"};
        state_.encounter.details = {"Lose 3 HP but reduce 4 heat.", "Costs 8 credits.", "Heat builds faster next segment."};
    } else if (r <= 79) {
        state_.encounter.encounterId = "event_c2";
        state_.encounter.title = "Hull Breach";
        state_.encounter.description = "The ship's hull has been compromised. Atmosphere is venting fast.";
        state_.encounter.choices = {"Emergency welding", "Dump cargo to seal", "Leave it"};
        state_.encounter.details = {"-4 HP, +3 heat, costs 10 credits.", "Costs 8 credits.", "Lose 8 HP."};
    } else if (r <= 85) {
        state_.encounter.encounterId = "event_c3";
        state_.encounter.title = "Deck Chaos";
        state_.encounter.description = "The card module system has malfunctioned. Data corruption detected.";
        state_.encounter.choices = {"Manual sorting", "Reboot the system", "Force format"};
        state_.encounter.details = {"Upgrade 2 random cards, lose 4 HP.", "-5 heat, delete 1 low-quality card, costs 10 credits.", "Costs 15 credits. Clear all low-quality cards."};
    } else if (r <= 90) {
        state_.encounter.encounterId = "event_d1";
        state_.encounter.title = "Smuggler Cache";
        state_.encounter.description = "A hidden cargo pod blinks with black market encryption signals.";
        state_.encounter.choices = {"Take everything", "Take only essentials", "Leave it alone"};
        state_.encounter.details = {"+16 credits and 1 random card.", "+6 HP.", "Continue without risk."};
    } else if (r <= 94) {
        state_.encounter.encounterId = "event_d2";
        state_.encounter.title = "Wandering Trader";
        state_.encounter.description = "A lone merchant vessel emerges from the static, offering deals.";
        state_.encounter.choices = {"Buy cards cheap", "Sell junk cards", "Decline the deal"};
        state_.encounter.details = {"Costs 18 credits. Gain 2 random cards.", "Delete 1 random card. Gain 10 credits.", "Continue."};
    } else if (r <= 97) {
        state_.encounter.encounterId = "event_d3";
        state_.encounter.title = "Pirate Ambush";
        state_.encounter.description = "A pirate interceptor drops out of warp and hails you with demands.";
        state_.encounter.choices = {"Pay the ransom", "Fight back", "Full speed escape"};
        state_.encounter.details = {"Costs 12 credits.", "Forced elite battle. Defeat: game over.", "+6 heat."};
    } else {
        state_.encounter.encounterId = "event_d4";
        state_.encounter.title = "Cryo Pod";
        state_.encounter.description = "A drifting cryo pod with a faint life sign. Someone is in deep freeze.";
        state_.encounter.choices = {"Wake the sleeper", "Drain cryo energy", "Scavenge supplies"};
        state_.encounter.details = {"Cards enhanced. Elite encounter chance increased next.", "+5 heat, +6 HP, costs 8 credits.", "+18 credits, gain card, -8 HP."};
    }
}

// Completes the current node, advances route state, and returns nothing.
void GameEngine::finishCurrentNode() {
    recordCurrentOwnedCards();
    state_.encounter = {};
    state_.reward = {};
    state_.animation = {};
    clearMarketState();
    state_.battleLog.clear();
    state_.player.shield = 0;
    state_.player.nextTurnEnergy = 0;
    state_.player.nextAttackMultiplier = 1;
    state_.player.attacksPlayedThisTurn = 0;
    state_.battleTurn = 0;

    bool skipNormalAdvance = false;

    // --- Escape Route ---
    if (g_sa_escape_stage >= 1 && g_sa_escape_stage <= 8) {
        if (g_sa_escape_battle_pending) {
            g_sa_escape_battle_pending = false;
            skipNormalAdvance = true;
        } else {
        g_sa_escape_stage++;
        // Random chance of a battle before the next escape event
        if (g_sa_escape_stage >= 2 && g_sa_escape_stage <= 8) {
            std::uniform_int_distribution<int> battleRoll(1, 100);
            if (battleRoll(rng_) <= 65) {
                g_sa_escape_battle_pending = true;
            }
        }
        if (g_sa_escape_stage > 8) {
            fillSummary(true);
            state_.info.lines.push_back("");
            state_.info.lines.push_back("You kept the Ark's call silenced.");
            state_.info.lines.push_back("No deeper descent, no revelations, no final stand.");
            state_.info.lines.push_back("Your prow pierced the mist of the Star Abyss,");
            state_.info.lines.push_back("and for the first time, the light of distant stars fell clearly upon the deck.");
            state_.info.lines.push_back("");
            state_.info.lines.push_back("You survived.");
            state_.info.lines.push_back("The secrets of the Star Abyss remain buried,");
            state_.info.lines.push_back("and you are among the very few salvagers to walk away whole.");
            return;
        }
        skipNormalAdvance = true;
        }
    }

    // --- Truth Route ---
    if (!skipNormalAdvance && g_sa_truth_stage >= 1 && g_sa_truth_stage <= 6) {
        state_.pendingVictory = false;
        state_.pendingSectorAdvance = false;
        if (g_sa_truth_post_battle) {
            g_sa_truth_post_battle = false;
            g_sa_truth_stage++;
        } else if (state_.activeNodeType == "elite" || state_.activeNodeType == "boss") {
            if (g_sa_truth_stage == 6) {
                fillSummary(true);
                state_.info.lines.push_back("");
                state_.info.lines.push_back("You stand at the origin of everything.");
                state_.info.lines.push_back("");
                state_.info.lines.push_back("Here there are no wrecks, no enemies, no pirates or federations.");
                state_.info.lines.push_back("Before you is not a ship's core, nor the heart of a star sector,");
                state_.info.lines.push_back("but a universe that was just born and fell immediately into slumber.");
                state_.info.lines.push_back("");
                state_.info.lines.push_back("The Ark fleet carried humanity's last hope for continuity,");
                state_.info.lines.push_back("attempting to pry open an entirely new habitable spacetime.");
                state_.info.lines.push_back("They ignited the singularity but could not control its expansion.");
                state_.info.lines.push_back("The runaway energy tore apart the fleet, shredded space itself,");
                state_.info.lines.push_back("and turned the ground into a chaotic, ever-expanding death zone --");
                state_.info.lines.push_back("what would come to be known as the Star Abyss.");
                state_.info.lines.push_back("");
                state_.info.lines.push_back("The Federation buried the truth, declaring the area forbidden,");
                state_.info.lines.push_back("because they feared the world would learn:");
                state_.info.lines.push_back("humanity once possessed the power of creation,");
                state_.info.lines.push_back("and witnessed its collapse with their own hands.");
                state_.info.lines.push_back("");
                state_.info.lines.push_back("And you -- a debt-ridden salvager who entered the dead zone to survive --");
                state_.info.lines.push_back("became the only person to reach the singularity and touch the origin.");
                return;
            }
            g_sa_truth_post_battle = true;
        }
        skipNormalAdvance = true;
    }

    // --- Victory check ---
    if (!skipNormalAdvance && state_.pendingVictory) {
        if (g_sa_beacon_activated == 1 && g_sa_truth_stage == 0) {
            state_.pendingVictory = false;
            g_sa_truth_stage = 1;
            g_sa_truth_post_battle = false;
            skipNormalAdvance = true;
        } else {
            fillSummary(true);
            state_.info.lines.push_back("");
            state_.info.lines.push_back("You destroyed the guardian units of the Ark Core,");
            state_.info.lines.push_back("and the turmoil of the Star Abyss gradually subsided.");
            state_.info.lines.push_back("");
            state_.info.lines.push_back("The world will tell your tale: a salvager who ventured into the dead zone,");
            state_.info.lines.push_back("quelled the catastrophe of the forbidden sector, and returned with glory.");
            state_.info.lines.push_back("The Federation will seal this chapter of history,");
            state_.info.lines.push_back("the black market will whisper your name,");
            state_.info.lines.push_back("and you can finally shed your debts and leave behind this deep space.");
            state_.info.lines.push_back("");
            state_.info.lines.push_back("Yet in the silence of the return voyage, a faint shadow of doubt lingers.");
            state_.info.lines.push_back("Those undeciphered signals, the misaligned wreckage,");
            state_.info.lines.push_back("the impossible spatial fluctuations...");
            state_.info.lines.push_back("They did not vanish with the battle's end,");
            state_.info.lines.push_back("but transformed into a faint whisper left in your wake.");
            state_.info.lines.push_back("");
            state_.info.lines.push_back("You survived the Star Abyss, but never touched its truth.");
            return;
        }
    }

    // --- Normal progression ---
    if (!skipNormalAdvance) {
        if (state_.pendingSectorAdvance) {
            state_.pendingSectorAdvance = false;
            ++state_.sector;
            state_.depth = 0;
            state_.player.hp = std::min(state_.player.maxHp, state_.player.hp + 6);
            state_.statusLine = "Sector cleared. Your crew patches 6 HP while plotting the next route.";
        } else if (state_.activeNodeType != "boss") {
            ++state_.depth;
        }

        while (g_sa_skip_segments > 0 && state_.depth < state_.nodesPerSector) {
            state_.depth++;
            g_sa_skip_segments--;
        }

        if (g_sa_heat_accel) {
            changeHeat(2);
            g_sa_heat_accel = false;
        }
    }

    if (!g_sa_event_epilogue.empty()) {
        state_.statusLine = g_sa_event_epilogue;
        g_sa_event_epilogue.clear();
    }

    state_.activeNodeType.clear();
    state_.activeNodeName.clear();

    generateRouteOptions();
    state_.page = AppPage::Map;
    resumePage_ = AppPage::Map;
}
