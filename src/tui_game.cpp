#include "tui_game.h"

#include <algorithm>
#include <utility>

namespace {

// what it does: Randomly shuffles a vector using the supplied random generator.
// what the inputs are: values - vector to shuffle; rng - random number generator used for ordering.
// what the outputs are: No return value (void).
template <typename T>
void ShuffleVector(std::vector<T>& values, std::mt19937& rng) {
    std::shuffle(values.begin(), values.end(), rng);
}

}  // namespace

// what it does: Constructs the game engine and initializes content, registries, and default state.
// what the inputs are: None.
// what the outputs are: Creates a GameEngine instance (constructor).
GameEngine::GameEngine() : rng_(std::random_device{}()) {
    initializeData();
    loadObtainedCardRegistry();
    loadCompletedEventAchievementRegistry();
    loadSeenEnemyRegistry();
    resetRunState();
    fillHelpInfo();
}

// what it does: Provides read-only access to the current run state.
// what the inputs are: None.
// what the outputs are: Returns the current run state (const RunState&).
const RunState& GameEngine::state() const {
    return state_;
}

// what it does: Resets the active run state to the main-menu defaults.
// what the inputs are: None.
// what the outputs are: No return value (void).
void GameEngine::resetRunState() {
    state_ = RunState{};
    state_.difficulty = 1;
    state_.page = AppPage::MainMenu;
    state_.statusLine = "Abyss link stable. Choose a route.";
    resumePage_ = AppPage::Map;
}

// what it does: Sets a deterministic random seed for testing.
// what the inputs are: seed - seed value for the random number generator.
// what the outputs are: No return value (void).
void GameEngine::setSeedForTesting(unsigned int seed) {
    rng_.seed(seed);
}

// what it does: Advances the active animation tick and clears finished animations.
// what the inputs are: None.
// what the outputs are: No return value (void).
void GameEngine::tickAnimation() {
    if (!state_.animation.active) {
        return;
    }
    ++state_.animation.tick;
    if (state_.animation.tick >= state_.animation.duration) {
        state_.animation.active = false;
        state_.animation.tick = 0;
    }
}

// what it does: Finds a base card definition by card identifier or reference.
// what the inputs are: id - card identifier or encoded card reference.
// what the outputs are: Returns the matching card definition pointer, or nullptr if absent (const CardDef*).
const CardDef* GameEngine::cardById(const std::string& id) const {
    const std::string baseId = baseCardId(id);
    const auto found = std::find_if(cardPool_.begin(), cardPool_.end(), [&](const CardDef& card) {
        return card.id == baseId;
    });
    return found == cardPool_.end() ? nullptr : &(*found);
}

// what it does: Selects an enemy template for the current sector and node type.
// what the inputs are: nodeType - battle node type; forcedEnemyId - optional enemy identifier override.
// what the outputs are: Returns the selected enemy template (const EnemyTemplate&).
const GameEngine::EnemyTemplate& GameEngine::pickEnemy(const std::string& nodeType,
                                                       const std::string& forcedEnemyId) {
    if (!forcedEnemyId.empty()) {
        const EnemyTemplate* forced = enemyTemplateById(forcedEnemyId);
        if (forced != nullptr) {
            return *forced;
        }
    }

    std::vector<const EnemyTemplate*> candidates;
    for (const EnemyTemplate& enemy : enemyPool_) {
        if (state_.sector < enemy.minSector || state_.sector > enemy.maxSector) {
            continue;
        }
        if (nodeType == "boss" && enemy.boss) {
            if ((state_.sector == 1 && enemy.id == "dread_frigate_omega") ||
                (state_.sector == 2 && enemy.id == "furnace_carrier") ||
                (state_.sector >= 3 && enemy.id == "abyss_core_sentinel")) {
                return enemy;
            }
        }
        if (nodeType == "elite" && enemy.elite) {
            candidates.push_back(&enemy);
        } else if (nodeType == "battle" && !enemy.elite && !enemy.boss) {
            candidates.push_back(&enemy);
        }
    }

    ShuffleVector(candidates, rng_);
    if (!candidates.empty()) {
        return *candidates.front();
    }
    return enemyPool_.front();
}

// what it does: Finds an enemy template by identifier.
// what the inputs are: id - enemy identifier to look up.
// what the outputs are: Returns the enemy template pointer, or nullptr if absent (const EnemyTemplate*).
const GameEngine::EnemyTemplate* GameEngine::enemyTemplateById(const std::string& id) const {
    const auto found = std::find_if(enemyPool_.begin(), enemyPool_.end(), [&](const EnemyTemplate& enemy) {
        return enemy.id == id;
    });
    return found == enemyPool_.end() ? nullptr : &(*found);
}

// what it does: Finds a ship template by identifier.
// what the inputs are: id - ship identifier to look up.
// what the outputs are: Returns the ship template pointer, or nullptr if absent (const ShipTemplate*).
const GameEngine::ShipTemplate* GameEngine::shipTemplateById(const std::string& id) const {
    const auto found = std::find_if(ships_.begin(), ships_.end(), [&](const ShipTemplate& ship) {
        return ship.id == id;
    });
    return found == ships_.end() ? nullptr : &(*found);
}

// what it does: Finds a relic definition by identifier.
// what the inputs are: id - relic identifier to look up.
// what the outputs are: Returns the relic definition pointer, or nullptr if absent (const RelicDef*).
const GameEngine::RelicDef* GameEngine::relicById(const std::string& id) const {
    const auto found = std::find_if(relicPool_.begin(), relicPool_.end(), [&](const RelicDef& relic) {
        return relic.id == id;
    });
    return found == relicPool_.end() ? nullptr : &(*found);
}
