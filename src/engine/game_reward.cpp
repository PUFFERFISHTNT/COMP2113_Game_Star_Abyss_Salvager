#include "engine/game_reward.h"

#include <algorithm>

namespace {

template <typename T>
void ShuffleVector(std::vector<T>& values, std::mt19937& rng) {
    std::shuffle(values.begin(), values.end(), rng);
}

}  // namespace

void GameEngine::chooseReward(int index) {
    if (state_.page != AppPage::Reward) {
        return;
    }

    if (index >= 0 && static_cast<size_t>(index) < state_.reward.cardIds.size()) {
        const std::string& cardId = state_.reward.cardIds[static_cast<size_t>(index)];
        state_.player.discardPile.push_back(cardId);
        const CardDef* card = cardById(cardId);
        const std::string cardName = card != nullptr ? card->name : "module";
        state_.statusLine = cardName + " has been integrated into your deck.";
    } else {
        state_.statusLine = "You skip the module reward and keep moving.";
    }

    finishCurrentNode();
}

std::vector<const CardDef*> GameEngine::rewardCards() const {
    std::vector<const CardDef*> result;
    for (const std::string& id : state_.reward.cardIds) {
        const CardDef* card = cardById(id);
        if (card != nullptr) {
            result.push_back(card);
        }
    }
    return result;
}

std::vector<const CardDef*> GameEngine::handCards() const {
    std::vector<const CardDef*> result;
    for (const std::string& id : state_.player.hand) {
        const CardDef* card = cardById(id);
        if (card != nullptr) {
            result.push_back(card);
        }
    }
    return result;
}

std::vector<std::string> GameEngine::randomCardChoices(size_t count) {
    std::vector<std::string> pool;
    for (const CardDef& card : cardPool_) {
        if (card.id != "laser_burst" && card.id != "raise_shield" && card.id != "coolant_flush" &&
            card.id != "quick_patch" && card.id != "overclock" && card.id != "scan_pulse") {
            pool.push_back(card.id);
        }
    }
    ShuffleVector(pool, rng_);
    if (pool.size() > count) {
        pool.resize(count);
    }
    return pool;
}

std::string GameEngine::randomRelic() {
    std::vector<std::string> candidates;
    for (const RelicDef& relic : relicPool_) {
        if (!hasRelic(relic.id)) {
            candidates.push_back(relic.id);
        }
    }
    if (candidates.empty()) {
        return "";
    }
    ShuffleVector(candidates, rng_);
    return candidates.front();
}
