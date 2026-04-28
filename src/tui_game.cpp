#include "tui_game.h"

#include <algorithm>
#include <utility>

namespace {

// Shuffles the given vector with the supplied random generator and returns nothing.
template <typename T>
void ShuffleVector(std::vector<T>& values, std::mt19937& rng) {
    std::shuffle(values.begin(), values.end(), rng);
}

}  // namespace

// Creates the game engine, loads content and registries, and leaves the help page data ready.
GameEngine::GameEngine() : rng_(std::random_device{}()) {
    initializeData();
    loadObtainedCardRegistry();
    loadCompletedEventAchievementRegistry();
    loadSeenEnemyRegistry();
    resetRunState();
    fillHelpInfo();
}

// Returns the current run state by const reference without changing it.
const RunState& GameEngine::state() const {
    return state_;
}

// Clears the active run back to menu defaults and returns nothing.
void GameEngine::resetRunState() {
    state_ = RunState{};
    state_.difficulty = 1;
    state_.page = AppPage::MainMenu;
    state_.statusLine = "Abyss link stable. Choose a route.";
    resumePage_ = AppPage::Map;
}

// Seeds the engine random generator with the provided value and returns nothing.
void GameEngine::setSeedForTesting(unsigned int seed) {
    rng_.seed(seed);
}

// Advances animation timing without input and returns nothing.
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

// Looks up the base card behind the given id or reference and returns its definition when found.
const CardDef* GameEngine::cardById(const std::string& id) const {
    const std::string baseId = baseCardId(id);
    const auto found = std::find_if(cardPool_.begin(), cardPool_.end(), [&](const CardDef& card) {
        return card.id == baseId;
    });
    return found == cardPool_.end() ? nullptr : &(*found);
}

// Chooses an enemy for the node type and optional forced id and returns the selected template.
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

// Searches enemy templates by id and returns the matching pointer when present.
const GameEngine::EnemyTemplate* GameEngine::enemyTemplateById(const std::string& id) const {
    const auto found = std::find_if(enemyPool_.begin(), enemyPool_.end(), [&](const EnemyTemplate& enemy) {
        return enemy.id == id;
    });
    return found == enemyPool_.end() ? nullptr : &(*found);
}

// Searches ship templates by id and returns the matching pointer when present.
const GameEngine::ShipTemplate* GameEngine::shipTemplateById(const std::string& id) const {
    const auto found = std::find_if(ships_.begin(), ships_.end(), [&](const ShipTemplate& ship) {
        return ship.id == id;
    });
    return found == ships_.end() ? nullptr : &(*found);
}

// Searches relic definitions by id and returns the matching pointer when present.
const GameEngine::RelicDef* GameEngine::relicById(const std::string& id) const {
    const auto found = std::find_if(relicPool_.begin(), relicPool_.end(), [&](const RelicDef& relic) {
        return relic.id == id;
    });
    return found == relicPool_.end() ? nullptr : &(*found);
}
