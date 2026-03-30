#include "tui_game.h"

#include <algorithm>
#include <utility>

namespace {

template <typename T>
void ShuffleVector(std::vector<T>& values, std::mt19937& rng) {
    std::shuffle(values.begin(), values.end(), rng);
}

}  // namespace

GameEngine::GameEngine() : rng_(std::random_device{}()) {
    initializeData();
    resetRunState();
    fillHelpInfo();
}

const RunState& GameEngine::state() const {
    return state_;
}

void GameEngine::resetRunState() {
    state_ = RunState{};
    state_.difficulty = 1;
    state_.page = AppPage::MainMenu;
    state_.statusLine = "Abyss link stable. Choose a route.";
    resumePage_ = AppPage::Map;
}

void GameEngine::setSeedForTesting(unsigned int seed) {
    rng_.seed(seed);
}

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

const CardDef* GameEngine::cardById(const std::string& id) const {
    const auto found = std::find_if(cardPool_.begin(), cardPool_.end(), [&](const CardDef& card) {
        return card.id == id;
    });
    return found == cardPool_.end() ? nullptr : &(*found);
}

const GameEngine::EnemyTemplate& GameEngine::pickEnemy(const std::string& nodeType) {
    std::vector<const EnemyTemplate*> candidates;
    for (const EnemyTemplate& enemy : enemyPool_) {
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
    return *(candidates.empty() ? &enemyPool_.front() : candidates.front());
}

const GameEngine::EnemyTemplate* GameEngine::enemyTemplateById(const std::string& id) const {
    const auto found = std::find_if(enemyPool_.begin(), enemyPool_.end(), [&](const EnemyTemplate& enemy) {
        return enemy.id == id;
    });
    return found == enemyPool_.end() ? nullptr : &(*found);
}

const GameEngine::ShipTemplate* GameEngine::shipTemplateById(const std::string& id) const {
    const auto found = std::find_if(ships_.begin(), ships_.end(), [&](const ShipTemplate& ship) {
        return ship.id == id;
    });
    return found == ships_.end() ? nullptr : &(*found);
}

const GameEngine::RelicDef* GameEngine::relicById(const std::string& id) const {
    const auto found = std::find_if(relicPool_.begin(), relicPool_.end(), [&](const RelicDef& relic) {
        return relic.id == id;
    });
    return found == relicPool_.end() ? nullptr : &(*found);
}
