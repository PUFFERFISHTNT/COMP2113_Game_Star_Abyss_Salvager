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
        queueOwnedCardToDiscard(cardId);
        const std::string cardName = displayCardName(cardId);
        state_.statusLine = cardName + " has been integrated into your deck.";
    } else {
        state_.statusLine = "You skip the module reward and keep moving.";
    }

    finishCurrentNode();
}

std::vector<std::string> GameEngine::randomCardChoices(size_t count) {
    std::vector<std::string> pool;
    for (const CardDef& card : cardPool_) {
        if (card.starterCopies == 0) {
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
