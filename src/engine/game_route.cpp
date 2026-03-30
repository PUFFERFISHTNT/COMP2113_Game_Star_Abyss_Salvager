#include "engine/game_route.h"

#include <algorithm>

namespace {

template <typename T>
void ShuffleVector(std::vector<T>& values, std::mt19937& rng) {
    std::shuffle(values.begin(), values.end(), rng);
}

}  // namespace

void GameEngine::startNewRun(int difficulty, const std::string& shipId) {
    resetRunState();
    const ShipTemplate* ship = shipTemplateById(shipId);
    const ShipTemplate& chosenShip = ship ? *ship : ships_.front();

    state_.hasRun = true;
    state_.difficulty = difficulty;
    state_.sector = 1;
    state_.depth = 0;
    state_.nodesPerSector = 3;
    setupPlayerForShip(chosenShip, difficulty);
    generateRouteOptions();
    state_.page = AppPage::Map;
    resumePage_ = AppPage::Map;
    state_.statusLine = "Run initialized. Chart your first jump.";
}

void GameEngine::showMainMenu() {
    if (state_.hasRun && state_.page != AppPage::MainMenu && state_.page != AppPage::Setup && state_.page != AppPage::Info) {
        resumePage_ = state_.page;
    }
    state_.page = AppPage::MainMenu;
}

void GameEngine::resumeRun() {
    if (!state_.hasRun) {
        return;
    }
    state_.page = resumePage_;
}

void GameEngine::chooseNode(size_t index) {
    if (state_.page != AppPage::Map || index >= state_.routeOptions.size()) {
        return;
    }

    const NodeOption node = state_.routeOptions[index];
    state_.activeNodeType = node.typeId;
    state_.activeNodeName = node.name;
    state_.statusLine = "Jump vector locked for " + node.name + ".";

    if (node.typeId == "battle" || node.typeId == "elite" || node.typeId == "boss") {
        startBattle(node.typeId);
    } else {
        startEncounterForNode(node);
    }
}

void GameEngine::returnToMenu() {
    showMainMenu();
}

void GameEngine::startAnotherRun() {
    state_.page = AppPage::Setup;
}

void GameEngine::generateRouteOptions() {
    state_.routeOptions.clear();
    if (state_.depth >= state_.nodesPerSector) {
        if (state_.sector == 1) {
            state_.routeOptions.push_back({"boss", "Dread Frigate Omega", "A warship blocks the sector jump.", 4});
        } else if (state_.sector == 2) {
            state_.routeOptions.push_back({"boss", "The Furnace Carrier", "Heat wakes ripple through a carrier hull.", 5});
        } else {
            state_.routeOptions.push_back({"boss", "Abyss Core Sentinel", "The final guardian unfolds in the void.", 6});
        }
        return;
    }

    std::vector<std::string> types = {"battle", "salvage", "repair", "event"};
    if (state_.sector >= 2) {
        types.push_back("elite");
        types.push_back("market");
    }
    ShuffleVector(types, rng_);
    types.resize(std::min<size_t>(3, types.size()));

    for (const std::string& type : types) {
        if (type == "battle") {
            state_.routeOptions.push_back({"battle", "Pirate Ambush", "Fast raiders are sweeping the lane ahead.", 2});
        } else if (type == "elite") {
            state_.routeOptions.push_back({"elite", "Elite Intercept", "A heavier hostile signature holds position.", 3});
        } else if (type == "salvage") {
            state_.routeOptions.push_back({"salvage", "Derelict Wreck", "Broken hulls drift in a silent salvage field.", 1});
        } else if (type == "repair") {
            state_.routeOptions.push_back({"repair", "Repair Station", "A maintenance dock answers your ping.", 1});
        } else if (type == "market") {
            state_.routeOptions.push_back({"market", "Black Market", "Encrypted trade beacons flare nearby.", 2});
        } else {
            state_.routeOptions.push_back({"event", "Strange Signal", "Anomalous data bleeds across the sensor grid.", 2});
        }
    }

    if (state_.player.hp < state_.player.maxHp / 2) {
        const bool hasRepair = std::any_of(state_.routeOptions.begin(), state_.routeOptions.end(), [](const NodeOption& option) {
            return option.typeId == "repair";
        });
        if (!hasRepair && !state_.routeOptions.empty()) {
            state_.routeOptions.back() = {"repair", "Emergency Drydock", "A lightly armed station can patch your hull.", 1};
        }
    }
}

void GameEngine::startEncounterForNode(const NodeOption& node) {
    state_.encounter = {};
    state_.encounter.typeId = node.typeId;
    state_.encounter.title = node.name;
    state_.encounter.description = node.detail;
    state_.page = AppPage::Encounter;
    resumePage_ = AppPage::Encounter;

    if (node.typeId == "repair") {
        state_.encounter.choices = {
            "Patch Hull (+10 HP)",
            "Vent Reactor (-6 Heat, +4 Shield)",
            "Install Plating (+4 Max HP)",
        };
        state_.encounter.details = {
            "Reliable, clean repairs for a bruised ship.",
            "Stabilize the core and harden your next engagement.",
            "Trade time for permanent hull reinforcement.",
        };
        return;
    }

    if (node.typeId == "salvage") {
        state_.encounter.choices = {
            "Strip Credits",
            "Recover Module Rack",
            "Leave It",
        };
        state_.encounter.details = {
            "Gain immediate credits, but risk a little reactor heat.",
            "Turn the wreck into a fresh card reward.",
            "Avoid the unknown and conserve resources.",
        };
        return;
    }

    if (node.typeId == "market") {
        state_.encounter.encounterId = "market_shop";
        state_.encounter.description = "A broker crackles through the static with contraband cards and a purge service.";
        const std::vector<std::string> offers = randomCardChoices(3);
        state_.encounter.choices.clear();
        state_.encounter.details.clear();
        state_.encounter.payloadIds.clear();
        state_.encounter.payloadValues.clear();

        for (const std::string& cardId : offers) {
            const CardDef* card = cardById(cardId);
            if (card != nullptr) {
                state_.encounter.choices.push_back(card->name);
                state_.encounter.details.push_back(card->description);
                state_.encounter.payloadIds.push_back("card:" + card->id);
                state_.encounter.payloadValues.push_back(28);
            }
        }
        state_.encounter.choices.push_back("Purge 1 card");
        state_.encounter.details.push_back("Pay 20 credits to permanently remove one card from your deck.");
        state_.encounter.payloadIds.push_back("purge");
        state_.encounter.payloadValues.push_back(20);
        state_.encounter.choices.push_back("Leave the broker");
        state_.encounter.details.push_back("Cut the transmission and keep moving.");
        state_.encounter.payloadIds.push_back("leave");
        state_.encounter.payloadValues.push_back(0);
        return;
    }

    std::uniform_int_distribution<int> pick(0, 3);
    const int eventIndex = pick(rng_);
    if (eventIndex == 0) {
        state_.encounter.encounterId = "distress_beacon";
        state_.encounter.title = "Distress Beacon";
        state_.encounter.description = "A damaged salvage tug is limping between asteroids.";
        state_.encounter.choices = {"Aid the tug", "Strip the derelict", "Ignore the call"};
        state_.encounter.details = {
            "Gain a modest reward and some goodwill.",
            "Take a bigger haul, but risk heat from a stressed reactor.",
            "Keep your run clean and your conscience murky.",
        };
    } else if (eventIndex == 1) {
        state_.encounter.encounterId = "strange_signal";
        state_.encounter.title = "Strange Signal";
        state_.encounter.description = "A harmonic transmission keeps syncing to your nav core.";
        state_.encounter.choices = {"Investigate", "Ride the signal", "Mute it"};
        state_.encounter.details = {
            "Dig for blueprints and possible reward cards.",
            "Bank an extra energy spike for the next battle.",
            "Stay safe and keep moving.",
        };
    } else if (eventIndex == 2) {
        state_.encounter.encounterId = "reactor_leak";
        state_.encounter.title = "Reactor Leak";
        state_.encounter.description = "A floating refinery vents unstable coolant into the lane.";
        state_.encounter.choices = {"Vent it clean", "Harvest unstable fuel", "Seal and patch"};
        state_.encounter.details = {
            "Cool the ship and earn a few credits.",
            "Cash out hard, but pay in reactor heat.",
            "A safe choice that restores a little hull.",
        };
    } else {
        state_.encounter.encounterId = "smuggler_cache";
        state_.encounter.title = "Smuggler Cache";
        state_.encounter.description = "A hidden cargo pod blinks with black market encryption.";
        state_.encounter.choices = {"Force it open", "Bribe the lock (10 credits)", "Leave it"};
        state_.encounter.details = {
            "High-risk relic grab with self-inflicted damage.",
            "Trade 10 credits for a controlled card reward.",
            "No risk, no gain.",
        };
    }
}

void GameEngine::finishCurrentNode() {
    state_.encounter = {};
    state_.reward = {};
    state_.animation = {};
    state_.battleLog.clear();
    state_.player.shield = 0;
    state_.player.nextTurnEnergy = 0;
    state_.player.nextAttackMultiplier = 1;
    state_.player.attacksPlayedThisTurn = 0;
    state_.battleTurn = 0;

    if (state_.pendingVictory) {
        fillSummary(true);
        return;
    }

    if (state_.pendingSectorAdvance) {
        state_.pendingSectorAdvance = false;
        ++state_.sector;
        state_.depth = 0;
        state_.player.hp = std::min(state_.player.maxHp, state_.player.hp + 6);
        state_.statusLine = "Sector cleared. Your crew patches 6 HP while plotting the next route.";
    } else if (state_.activeNodeType != "boss") {
        ++state_.depth;
    }

    state_.activeNodeType.clear();
    state_.activeNodeName.clear();

    generateRouteOptions();
    state_.page = AppPage::Map;
    resumePage_ = AppPage::Map;
}
