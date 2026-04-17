#include "engine/game_card_services.h"

#include <algorithm>
#include <limits>

namespace {

constexpr int kBlackMarketCardPrice = 28;
constexpr int kBlackMarketPurgePrice = 20;
constexpr size_t kCardServicePageSize = 4;

constexpr const char* kPayloadPrevPurge = "service_prev_purge";
constexpr const char* kPayloadCyclePurge = "service_cycle_purge";
constexpr const char* kPayloadPrevUpgrade = "service_prev_upgrade";
constexpr const char* kPayloadCycleUpgrade = "service_cycle_upgrade";
constexpr const char* kPayloadBackMarket = "service_back_market";
constexpr const char* kPayloadSkipService = "service_skip_service";

template <typename T>
void ShuffleVector(std::vector<T>& values, std::mt19937& rng) {
    std::shuffle(values.begin(), values.end(), rng);
}

}  // namespace

std::string GameEngine::baseCardId(const std::string& cardRef) const {
    const size_t separator = cardRef.rfind('@');
    if (separator == std::string::npos) {
        return cardRef;
    }
    const std::string suffix = cardRef.substr(separator + 1);
    if (suffix.empty() ||
        !std::all_of(suffix.begin(), suffix.end(), [](unsigned char ch) { return ch >= '0' && ch <= '9'; })) {
        return cardRef;
    }
    return cardRef.substr(0, separator);
}

int GameEngine::cardUpgradeLevel(const std::string& cardRef) const {
    const size_t separator = cardRef.rfind('@');
    if (separator == std::string::npos) {
        return 0;
    }
    const std::string suffix = cardRef.substr(separator + 1);
    if (suffix.empty() ||
        !std::all_of(suffix.begin(), suffix.end(), [](unsigned char ch) { return ch >= '0' && ch <= '9'; })) {
        return 0;
    }
    try {
        return std::max(0, std::stoi(suffix));
    } catch (...) {
        return 0;
    }
}

std::string GameEngine::makeCardRef(const std::string& cardId, int upgradeLevel) const {
    return upgradeLevel <= 0 ? cardId : (cardId + "@" + std::to_string(upgradeLevel));
}

const CardDef* GameEngine::upgradedCardByBaseId(const std::string& cardId) const {
    const std::string baseId = baseCardId(cardId);
    const auto found = std::find_if(upgradedCardPool_.begin(), upgradedCardPool_.end(), [&](const CardDef& card) {
        return card.id == baseId;
    });
    return found == upgradedCardPool_.end() ? nullptr : &(*found);
}

void GameEngine::storeUpgradedCard(const std::string& cardId) {
    const CardDef* base = cardById(cardId);
    if (base == nullptr) {
        return;
    }

    CardDef upgraded = applyUpgrade(*base, 1);
    upgraded.id = base->id;
    upgraded.name = base->name + "+++";

    auto found = std::find_if(upgradedCardPool_.begin(), upgradedCardPool_.end(), [&](const CardDef& card) {
        return card.id == upgraded.id;
    });
    if (found == upgradedCardPool_.end()) {
        upgradedCardPool_.push_back(upgraded);
    } else {
        *found = upgraded;
    }
}

void GameEngine::rebuildUpgradedCardPool() {
    std::sort(state_.upgradedCardTypes.begin(), state_.upgradedCardTypes.end());
    state_.upgradedCardTypes.erase(std::unique(state_.upgradedCardTypes.begin(),
                                               state_.upgradedCardTypes.end()),
                                   state_.upgradedCardTypes.end());

    if (!upgradedCardPool_.empty()) {
        std::vector<std::string> upgraded_ids;
        upgraded_ids.reserve(upgradedCardPool_.size());
        for (const CardDef& card : upgradedCardPool_) {
            upgraded_ids.push_back(card.id);
        }
        state_.upgradedCardTypes.insert(state_.upgradedCardTypes.end(), upgraded_ids.begin(), upgraded_ids.end());
        std::sort(state_.upgradedCardTypes.begin(), state_.upgradedCardTypes.end());
        state_.upgradedCardTypes.erase(std::unique(state_.upgradedCardTypes.begin(),
                                                   state_.upgradedCardTypes.end()),
                                       state_.upgradedCardTypes.end());
    }

    if (upgradedCardPool_.empty()) {
        for (const std::string& cardId : state_.upgradedCardTypes) {
            storeUpgradedCard(cardId);
        }
    }
}

bool GameEngine::isCardTypeUpgraded(const std::string& cardId) const {
    return upgradedCardByBaseId(cardId) != nullptr;
}

void GameEngine::markCardTypeUpgraded(const std::string& cardId) {
    const std::string baseId = baseCardId(cardId);
    if (baseId.empty() || isCardTypeUpgraded(baseId)) {
        return;
    }
    state_.upgradedCardTypes.push_back(baseId);
    std::sort(state_.upgradedCardTypes.begin(), state_.upgradedCardTypes.end());
    storeUpgradedCard(baseId);
}

void GameEngine::normalizeUpgradedCards() {
    std::vector<std::string> discovered;
    auto collect = [&](const std::vector<std::string>& pile) {
        for (const std::string& ref : pile) {
            if (cardUpgradeLevel(ref) > 0) {
                discovered.push_back(baseCardId(ref));
            }
        }
    };
    collect(state_.player.drawPile);
    collect(state_.player.hand);
    collect(state_.player.discardPile);
    collect(state_.reward.cardIds);
    collect(state_.marketOfferCardIds);

    state_.upgradedCardTypes.insert(state_.upgradedCardTypes.end(), discovered.begin(), discovered.end());

    const auto strip_refs = [&](std::vector<std::string>& pile) {
        for (std::string& ref : pile) {
            ref = baseCardId(ref);
        }
    };
    strip_refs(state_.player.drawPile);
    strip_refs(state_.player.hand);
    strip_refs(state_.player.discardPile);
    strip_refs(state_.reward.cardIds);
    strip_refs(state_.marketOfferCardIds);
    rebuildUpgradedCardPool();
}

bool GameEngine::cardTypeSupportsUpgrade(const std::string& cardId) const {
    const CardDef* card = cardById(cardId);
    if (card == nullptr) {
        return false;
    }
    return card->damage != 0 ||
           card->shield != 0 ||
           card->heal != 0 ||
           card->draw != 0 ||
           card->energy != 0 ||
           card->nextTurnEnergy != 0 ||
           card->heat != 0 ||
           card->selfDamage != 0 ||
           card->multiHit > 1 ||
           card->comboBonus != 0 ||
           card->attackMultiplier > 1 ||
           card->weakenTurns != 0 ||
           (card->removeEnemyShield > 0 && card->removeEnemyShield < 999) ||
           card->shieldLockTurns != 0 ||
           card->salvageBonus != 0;
}

CardDef GameEngine::applyUpgrade(const CardDef& baseCard, int upgradeLevel) const {
    CardDef upgraded = baseCard;
    for (int level = 0; level < upgradeLevel; ++level) {
        const auto scale_value = [](int value) {
            if (value == 0) {
                return 0;
            }
            const int magnitude = std::abs(value);
            const int scaled = (magnitude * 3 + 1) / 2;
            return value > 0 ? scaled : -scaled;
        };

        upgraded.damage = scale_value(upgraded.damage);
        upgraded.shield = scale_value(upgraded.shield);
        upgraded.heal = scale_value(upgraded.heal);
        upgraded.draw = scale_value(upgraded.draw);
        upgraded.energy = scale_value(upgraded.energy);
        upgraded.nextTurnEnergy = scale_value(upgraded.nextTurnEnergy);
        upgraded.heat = scale_value(upgraded.heat);
        upgraded.selfDamage = scale_value(upgraded.selfDamage);
        if (upgraded.multiHit > 1) {
            upgraded.multiHit = scale_value(upgraded.multiHit);
        }
        upgraded.comboBonus = scale_value(upgraded.comboBonus);
        if (upgraded.attackMultiplier > 1) {
            upgraded.attackMultiplier = scale_value(upgraded.attackMultiplier);
        }
        upgraded.weakenTurns = scale_value(upgraded.weakenTurns);
        if (upgraded.removeEnemyShield > 0 && upgraded.removeEnemyShield < 999) {
            upgraded.removeEnemyShield = scale_value(upgraded.removeEnemyShield);
        }
        upgraded.shieldLockTurns = scale_value(upgraded.shieldLockTurns);
        upgraded.salvageBonus = scale_value(upgraded.salvageBonus);
    }
    return upgraded;
}

CardDef GameEngine::resolvedCard(const std::string& cardRef) const {
    const CardDef* upgraded = upgradedCardByBaseId(cardRef);
    if (upgraded != nullptr) {
        return *upgraded;
    }
    const CardDef* base = cardById(cardRef);
    if (base == nullptr) {
        return CardDef{};
    }
    const int effectiveUpgradeLevel = cardUpgradeLevel(cardRef);
    CardDef resolved = applyUpgrade(*base, effectiveUpgradeLevel);
    if (effectiveUpgradeLevel > 0) {
        resolved.name += "+++";
    }
    return resolved;
}

std::string GameEngine::displayCardName(const std::string& cardRef) const {
    const CardDef resolved = resolvedCard(cardRef);
    return resolved.name.empty() ? baseCardId(cardRef) : resolved.name;
}

int GameEngine::cardUpgradeCost(CardUpgradeSource source) const {
    if (source == CardUpgradeSource::RepairStation || source == CardUpgradeSource::EventReward) {
        return 0;
    }

    long long cost = kMarketCardUpgradeBaseCost;
    for (int i = 0; i < state_.marketUpgradeCount; ++i) {
        cost *= 2;
        if (cost >= std::numeric_limits<int>::max()) {
            return std::numeric_limits<int>::max();
        }
    }
    return static_cast<int>(cost);
}

std::vector<CardView> GameEngine::rewardCards() const {
    std::vector<CardView> result;
    for (const std::string& ref : state_.reward.cardIds) {
        if (cardById(ref) != nullptr) {
            result.push_back({ref, resolvedCard(ref), isCardTypeUpgraded(ref) ? 1 : cardUpgradeLevel(ref)});
        }
    }
    return result;
}

std::vector<CardView> GameEngine::handCards() const {
    std::vector<CardView> result;
    for (const std::string& ref : state_.player.hand) {
        if (cardById(ref) != nullptr) {
            result.push_back({ref, resolvedCard(ref), isCardTypeUpgraded(ref) ? 1 : cardUpgradeLevel(ref)});
        }
    }
    return result;
}

bool GameEngine::upgradeCard(const std::string& cardRef,
                             CardUpgradeSource source,
                             std::string* error) {
    if (!state_.hasRun) {
        if (error != nullptr) {
            *error = "No active run to upgrade.";
        }
        return false;
    }

    const std::string baseId = baseCardId(cardRef);
    std::vector<std::vector<std::string>*> piles = {
        &state_.player.hand,
        &state_.player.drawPile,
        &state_.player.discardPile,
    };

    const bool ownsCardType = std::any_of(piles.begin(), piles.end(), [&](const std::vector<std::string>* pile) {
        return std::any_of(pile->begin(), pile->end(), [&](const std::string& ownedRef) {
            return baseCardId(ownedRef) == baseId;
        });
    });

    if (!ownsCardType) {
        if (error != nullptr) {
            *error = "That card is not in the current deck.";
        }
        return false;
    }
    if (!cardTypeSupportsUpgrade(baseId)) {
        if (error != nullptr) {
            *error = "That card does not have an upgrade effect.";
        }
        return false;
    }

    if (isCardTypeUpgraded(baseId)) {
        if (error != nullptr) {
            *error = "That card is already fully upgraded.";
        }
        return false;
    }

    const int price = cardUpgradeCost(source);
    if (state_.player.scrap < price) {
        if (error != nullptr) {
            *error = "Not enough credits to upgrade that card.";
        }
        return false;
    }

    if (price > 0) {
        state_.player.scrap -= price;
    }
    markCardTypeUpgraded(baseId);
    if (source == CardUpgradeSource::Market) {
        ++state_.marketUpgradeCount;
        state_.statusLine = displayCardName(baseId) + " upgraded for " + std::to_string(price) + " credits.";
    } else if (source == CardUpgradeSource::RepairStation) {
        state_.statusLine = displayCardName(baseId) + " upgraded for free at the repair station.";
    } else {
        state_.statusLine = displayCardName(baseId) + " upgraded for free.";
    }
    if (error != nullptr) {
        *error = "";
    }
    return true;
}

std::vector<std::pair<std::string, int>> GameEngine::purgeableCards() const {
    std::vector<std::string> allCards;
    allCards.insert(allCards.end(), state_.player.drawPile.begin(), state_.player.drawPile.end());
    allCards.insert(allCards.end(), state_.player.hand.begin(), state_.player.hand.end());
    allCards.insert(allCards.end(), state_.player.discardPile.begin(), state_.player.discardPile.end());

    for (std::string& ref : allCards) {
        ref = baseCardId(ref);
    }

    std::sort(allCards.begin(), allCards.end(), [&](const std::string& lhs, const std::string& rhs) {
        const std::string leftName = displayCardName(lhs);
        const std::string rightName = displayCardName(rhs);
        if (leftName != rightName) {
            return leftName < rightName;
        }
        return lhs < rhs;
    });

    std::vector<std::pair<std::string, int>> result;
    for (const std::string& cardRef : allCards) {
        if (!result.empty() && result.back().first == cardRef) {
            ++result.back().second;
        } else {
            result.push_back({cardRef, 1});
        }
    }
    return result;
}

std::vector<std::pair<std::string, int>> GameEngine::upgradeableCards() const {
    std::vector<std::string> allCards;
    const auto collect = [&](const std::vector<std::string>& pile) {
        for (const std::string& cardRef : pile) {
            const std::string baseId = baseCardId(cardRef);
            if (!isCardTypeUpgraded(baseId) && cardTypeSupportsUpgrade(baseId)) {
                allCards.push_back(baseId);
            }
        }
    };
    collect(state_.player.drawPile);
    collect(state_.player.hand);
    collect(state_.player.discardPile);

    std::sort(allCards.begin(), allCards.end(), [&](const std::string& lhs, const std::string& rhs) {
        const std::string leftName = displayCardName(lhs);
        const std::string rightName = displayCardName(rhs);
        if (leftName != rightName) {
            return leftName < rightName;
        }
        return lhs < rhs;
    });

    std::vector<std::pair<std::string, int>> result;
    for (const std::string& cardRef : allCards) {
        if (!result.empty() && result.back().first == cardRef) {
            ++result.back().second;
        } else {
            result.push_back({cardRef, 1});
        }
    }
    return result;
}

bool GameEngine::purgeCardFromDeck(const std::string& cardId) {
    const std::string baseId = baseCardId(cardId);
    std::vector<std::vector<std::string>*> piles = {
        &state_.player.hand,
        &state_.player.drawPile,
        &state_.player.discardPile,
    };
    for (std::vector<std::string>* pile : piles) {
        std::vector<std::string>::iterator it = std::find_if(pile->begin(), pile->end(), [&](const std::string& ownedRef) {
            return baseCardId(ownedRef) == baseId;
        });
        if (it != pile->end()) {
            pile->erase(it);
            return true;
        }
    }
    return false;
}

bool GameEngine::upgradeRandomCard(CardUpgradeSource source,
                                   std::string* upgradedCardName,
                                   std::string* error) {
    const std::vector<std::pair<std::string, int>> candidates = upgradeableCards();
    if (candidates.empty()) {
        if (error != nullptr) {
            *error = "No cards can be upgraded right now.";
        }
        return false;
    }

    std::vector<std::string> candidateIds;
    for (const auto& entry : candidates) {
        candidateIds.push_back(entry.first);
    }
    ShuffleVector(candidateIds, rng_);
    const std::string upgradedName = displayCardName(candidateIds.front());
    if (!upgradeCard(candidateIds.front(), source, error)) {
        return false;
    }
    if (upgradedCardName != nullptr) {
        *upgradedCardName = upgradedName;
    }
    return true;
}

void GameEngine::openMarketShop() {
    if (state_.marketOfferCardIds.empty()) {
        state_.marketOfferCardIds = randomCardChoices(3);
    }

    state_.encounter = {};
    state_.encounter.typeId = "market";
    state_.encounter.encounterId = "market_shop";
    state_.encounter.title = "Black Market";
    state_.encounter.description = "A broker crackles through the static with contraband cards and ship services.";
    state_.page = AppPage::Encounter;
    resumePage_ = AppPage::Encounter;

    for (const std::string& cardId : state_.marketOfferCardIds) {
        const CardDef* card = cardById(cardId);
        if (card != nullptr) {
            state_.encounter.choices.push_back(displayCardName(card->id));
            state_.encounter.details.push_back(card->description);
            state_.encounter.payloadIds.push_back("card:" + card->id);
            state_.encounter.payloadValues.push_back(kBlackMarketCardPrice);
        }
    }

    state_.encounter.choices.push_back("Upgrade 1 card");
    state_.encounter.details.push_back("Open the upgrade console and improve one card in your deck.");
    state_.encounter.payloadIds.push_back("upgrade");
    state_.encounter.payloadValues.push_back(cardUpgradeCost(CardUpgradeSource::Market));

    if (!state_.marketPurgeUsed) {
        state_.encounter.choices.push_back("Purge 1 card");
        state_.encounter.details.push_back("Open the purge console. Each market visit allows only one purge.");
        state_.encounter.payloadIds.push_back("purge");
        state_.encounter.payloadValues.push_back(kBlackMarketPurgePrice);
    }

    state_.encounter.choices.push_back("Leave the broker");
    state_.encounter.details.push_back("Cut the transmission and keep moving.");
    state_.encounter.payloadIds.push_back("leave");
    state_.encounter.payloadValues.push_back(0);
}

void GameEngine::clearMarketState() {
    state_.marketOfferCardIds.clear();
    state_.marketPurgeUsed = false;
}

void GameEngine::enterMarketPurge(size_t offset) {
    const std::vector<std::pair<std::string, int>> candidates = purgeableCards();
    const size_t totalPages = std::max<size_t>(1, (candidates.size() + kCardServicePageSize - 1) / kCardServicePageSize);
    const size_t safeOffset =
        candidates.empty() ? 0 : std::min(offset, ((candidates.size() - 1) / kCardServicePageSize) * kCardServicePageSize);
    const size_t currentPage = candidates.empty() ? 1 : (safeOffset / kCardServicePageSize) + 1;
    const size_t upper = std::min(candidates.size(), safeOffset + kCardServicePageSize);

    state_.encounter = {};
    state_.encounter.typeId = "market";
    state_.encounter.encounterId = "market_purge";
    state_.encounter.title = "Black Market Purge";
    state_.encounter.description =
        "Select one card to erase from your deck. Price: 20 credits. Each market visit allows only one purge. Page " +
        std::to_string(currentPage) + "/" + std::to_string(totalPages) +
        ". Starter cards may appear on later pages. Scroll with mouse wheel, [ ], or PgUp/PgDn if needed.";
    state_.page = AppPage::Encounter;
    resumePage_ = AppPage::Encounter;

    for (size_t i = safeOffset; i < upper; ++i) {
        state_.encounter.choices.push_back(displayCardName(candidates[i].first));
        state_.encounter.details.push_back("Purge 1 copy from the deck. Copies owned: " +
                                           std::to_string(candidates[i].second) + ".");
        state_.encounter.payloadIds.push_back(candidates[i].first);
        state_.encounter.payloadValues.push_back(kBlackMarketPurgePrice);
    }
    if (safeOffset > 0) {
        const size_t previousOffset = safeOffset >= kCardServicePageSize ? safeOffset - kCardServicePageSize : 0;
        state_.encounter.choices.push_back("Previous purge targets");
        state_.encounter.details.push_back("Return to the previous page of cards that can be removed. Previous page: " +
                                           std::to_string(currentPage - 1) + "/" + std::to_string(totalPages) + ".");
        state_.encounter.payloadIds.push_back(kPayloadPrevPurge);
        state_.encounter.payloadValues.push_back(static_cast<int>(previousOffset));
    }
    if (upper < candidates.size()) {
        state_.encounter.choices.push_back("More purge targets");
        state_.encounter.details.push_back("Cycle to the next page of cards that can be removed. Next page: " +
                                           std::to_string(currentPage + 1) + "/" + std::to_string(totalPages) + ".");
        state_.encounter.payloadIds.push_back(kPayloadCyclePurge);
        state_.encounter.payloadValues.push_back(static_cast<int>(upper));
    }
    state_.encounter.choices.push_back("Back to broker");
    state_.encounter.details.push_back("Return to the black market offers.");
    state_.encounter.payloadIds.push_back(kPayloadBackMarket);
    state_.encounter.payloadValues.push_back(0);
}

void GameEngine::enterMarketUpgrade(size_t offset) {
    const std::vector<std::pair<std::string, int>> candidates = upgradeableCards();
    const size_t totalPages = std::max<size_t>(1, (candidates.size() + kCardServicePageSize - 1) / kCardServicePageSize);
    const size_t safeOffset =
        candidates.empty() ? 0 : std::min(offset, ((candidates.size() - 1) / kCardServicePageSize) * kCardServicePageSize);
    const size_t currentPage = candidates.empty() ? 1 : (safeOffset / kCardServicePageSize) + 1;
    const size_t upper = std::min(candidates.size(), safeOffset + kCardServicePageSize);
    const int price = cardUpgradeCost(CardUpgradeSource::Market);

    state_.encounter = {};
    state_.encounter.typeId = "market";
    state_.encounter.encounterId = "market_upgrade";
    state_.encounter.title = "Black Market Upgrade";
    state_.encounter.description =
        "Select one card to upgrade. Current price: " + std::to_string(price) + " credits. Page " +
        std::to_string(currentPage) + "/" + std::to_string(totalPages) +
        ". Scroll with mouse wheel, [ ], or PgUp/PgDn if needed.";
    state_.page = AppPage::Encounter;
    resumePage_ = AppPage::Encounter;

    for (size_t i = safeOffset; i < upper; ++i) {
        state_.encounter.choices.push_back(displayCardName(candidates[i].first));
        state_.encounter.details.push_back("Upgrade 1 card type in the deck. Copies owned: " +
                                           std::to_string(candidates[i].second) + ".");
        state_.encounter.payloadIds.push_back(candidates[i].first);
        state_.encounter.payloadValues.push_back(price);
    }
    if (safeOffset > 0) {
        const size_t previousOffset = safeOffset >= kCardServicePageSize ? safeOffset - kCardServicePageSize : 0;
        state_.encounter.choices.push_back("Previous upgrade targets");
        state_.encounter.details.push_back("Return to the previous page of cards that can be upgraded. Previous page: " +
                                           std::to_string(currentPage - 1) + "/" + std::to_string(totalPages) + ".");
        state_.encounter.payloadIds.push_back(kPayloadPrevUpgrade);
        state_.encounter.payloadValues.push_back(static_cast<int>(previousOffset));
    }
    if (upper < candidates.size()) {
        state_.encounter.choices.push_back("More upgrade targets");
        state_.encounter.details.push_back("Cycle to the next page of cards that can be upgraded. Next page: " +
                                           std::to_string(currentPage + 1) + "/" + std::to_string(totalPages) + ".");
        state_.encounter.payloadIds.push_back(kPayloadCycleUpgrade);
        state_.encounter.payloadValues.push_back(static_cast<int>(upper));
    }
    state_.encounter.choices.push_back("Back to broker");
    state_.encounter.details.push_back("Return to the black market offers.");
    state_.encounter.payloadIds.push_back(kPayloadBackMarket);
    state_.encounter.payloadValues.push_back(0);
}

void GameEngine::enterRepairUpgrade(size_t offset) {
    const std::vector<std::pair<std::string, int>> candidates = upgradeableCards();
    const size_t totalPages = std::max<size_t>(1, (candidates.size() + kCardServicePageSize - 1) / kCardServicePageSize);
    const size_t safeOffset =
        candidates.empty() ? 0 : std::min(offset, ((candidates.size() - 1) / kCardServicePageSize) * kCardServicePageSize);
    const size_t currentPage = candidates.empty() ? 1 : (safeOffset / kCardServicePageSize) + 1;
    const size_t upper = std::min(candidates.size(), safeOffset + kCardServicePageSize);

    state_.encounter = {};
    state_.encounter.typeId = "repair";
    state_.encounter.encounterId = "repair_upgrade";
    state_.encounter.title = "Repair Station Retrofit";
    state_.encounter.description =
        "Select one card to upgrade for free at the repair station. Page " +
        std::to_string(currentPage) + "/" + std::to_string(totalPages) +
        ". Scroll with mouse wheel, [ ], or PgUp/PgDn if needed.";
    state_.page = AppPage::Encounter;
    resumePage_ = AppPage::Encounter;

    for (size_t i = safeOffset; i < upper; ++i) {
        state_.encounter.choices.push_back(displayCardName(candidates[i].first));
        state_.encounter.details.push_back("Upgrade 1 card type in the deck. Copies owned: " +
                                           std::to_string(candidates[i].second) + ".");
        state_.encounter.payloadIds.push_back(candidates[i].first);
        state_.encounter.payloadValues.push_back(0);
    }
    if (safeOffset > 0) {
        const size_t previousOffset = safeOffset >= kCardServicePageSize ? safeOffset - kCardServicePageSize : 0;
        state_.encounter.choices.push_back("Previous upgrade targets");
        state_.encounter.details.push_back("Return to the previous page of cards that can be upgraded. Previous page: " +
                                           std::to_string(currentPage - 1) + "/" + std::to_string(totalPages) + ".");
        state_.encounter.payloadIds.push_back(kPayloadPrevUpgrade);
        state_.encounter.payloadValues.push_back(static_cast<int>(previousOffset));
    }
    if (upper < candidates.size()) {
        state_.encounter.choices.push_back("More upgrade targets");
        state_.encounter.details.push_back("Cycle to the next page of cards that can be upgraded. Next page: " +
                                           std::to_string(currentPage + 1) + "/" + std::to_string(totalPages) + ".");
        state_.encounter.payloadIds.push_back(kPayloadCycleUpgrade);
        state_.encounter.payloadValues.push_back(static_cast<int>(upper));
    }
    state_.encounter.choices.push_back("Skip reward");
    state_.encounter.details.push_back("Leave the retrofit console unused and continue.");
    state_.encounter.payloadIds.push_back(kPayloadSkipService);
    state_.encounter.payloadValues.push_back(0);
}

void GameEngine::enterEventPurge(size_t offset) {
    const std::vector<std::pair<std::string, int>> candidates = purgeableCards();
    const size_t totalPages = std::max<size_t>(1, (candidates.size() + kCardServicePageSize - 1) / kCardServicePageSize);
    const size_t safeOffset =
        candidates.empty() ? 0 : std::min(offset, ((candidates.size() - 1) / kCardServicePageSize) * kCardServicePageSize);
    const size_t currentPage = candidates.empty() ? 1 : (safeOffset / kCardServicePageSize) + 1;
    const size_t upper = std::min(candidates.size(), safeOffset + kCardServicePageSize);

    state_.encounter = {};
    state_.encounter.typeId = "event";
    state_.encounter.encounterId = "event_purge";
    state_.encounter.title = "Smuggler Purge Rig";
    state_.encounter.description =
        "A hidden rig can erase one card from your deck for free. Page " +
        std::to_string(currentPage) + "/" + std::to_string(totalPages) +
        ". Starter cards may appear on later pages. Scroll with mouse wheel, [ ], or PgUp/PgDn if needed.";
    state_.page = AppPage::Encounter;
    resumePage_ = AppPage::Encounter;

    for (size_t i = safeOffset; i < upper; ++i) {
        state_.encounter.choices.push_back(displayCardName(candidates[i].first));
        state_.encounter.details.push_back("Purge 1 copy from the deck. Copies owned: " +
                                           std::to_string(candidates[i].second) + ".");
        state_.encounter.payloadIds.push_back(candidates[i].first);
        state_.encounter.payloadValues.push_back(0);
    }
    if (safeOffset > 0) {
        const size_t previousOffset = safeOffset >= kCardServicePageSize ? safeOffset - kCardServicePageSize : 0;
        state_.encounter.choices.push_back("Previous purge targets");
        state_.encounter.details.push_back("Return to the previous page of cards that can be removed. Previous page: " +
                                           std::to_string(currentPage - 1) + "/" + std::to_string(totalPages) + ".");
        state_.encounter.payloadIds.push_back(kPayloadPrevPurge);
        state_.encounter.payloadValues.push_back(static_cast<int>(previousOffset));
    }
    if (upper < candidates.size()) {
        state_.encounter.choices.push_back("More purge targets");
        state_.encounter.details.push_back("Cycle to the next page of cards that can be removed. Next page: " +
                                           std::to_string(currentPage + 1) + "/" + std::to_string(totalPages) + ".");
        state_.encounter.payloadIds.push_back(kPayloadCyclePurge);
        state_.encounter.payloadValues.push_back(static_cast<int>(upper));
    }
    state_.encounter.choices.push_back("Skip reward");
    state_.encounter.details.push_back("Leave the purge rig untouched and continue.");
    state_.encounter.payloadIds.push_back(kPayloadSkipService);
    state_.encounter.payloadValues.push_back(0);
}

void GameEngine::enterEventUpgrade(size_t offset) {
    const std::vector<std::pair<std::string, int>> candidates = upgradeableCards();
    const size_t totalPages = std::max<size_t>(1, (candidates.size() + kCardServicePageSize - 1) / kCardServicePageSize);
    const size_t safeOffset =
        candidates.empty() ? 0 : std::min(offset, ((candidates.size() - 1) / kCardServicePageSize) * kCardServicePageSize);
    const size_t currentPage = candidates.empty() ? 1 : (safeOffset / kCardServicePageSize) + 1;
    const size_t upper = std::min(candidates.size(), safeOffset + kCardServicePageSize);

    state_.encounter = {};
    state_.encounter.typeId = "event";
    state_.encounter.encounterId = "event_upgrade";
    state_.encounter.title = "Signal Calibration";
    state_.encounter.description =
        "A tuning lattice lets you upgrade one card for free. Page " +
        std::to_string(currentPage) + "/" + std::to_string(totalPages) +
        ". Scroll with mouse wheel, [ ], or PgUp/PgDn if needed.";
    state_.page = AppPage::Encounter;
    resumePage_ = AppPage::Encounter;

    for (size_t i = safeOffset; i < upper; ++i) {
        state_.encounter.choices.push_back(displayCardName(candidates[i].first));
        state_.encounter.details.push_back("Upgrade 1 card type in the deck. Copies owned: " +
                                           std::to_string(candidates[i].second) + ".");
        state_.encounter.payloadIds.push_back(candidates[i].first);
        state_.encounter.payloadValues.push_back(0);
    }
    if (safeOffset > 0) {
        const size_t previousOffset = safeOffset >= kCardServicePageSize ? safeOffset - kCardServicePageSize : 0;
        state_.encounter.choices.push_back("Previous upgrade targets");
        state_.encounter.details.push_back("Return to the previous page of cards that can be upgraded. Previous page: " +
                                           std::to_string(currentPage - 1) + "/" + std::to_string(totalPages) + ".");
        state_.encounter.payloadIds.push_back(kPayloadPrevUpgrade);
        state_.encounter.payloadValues.push_back(static_cast<int>(previousOffset));
    }
    if (upper < candidates.size()) {
        state_.encounter.choices.push_back("More upgrade targets");
        state_.encounter.details.push_back("Cycle to the next page of cards that can be upgraded. Next page: " +
                                           std::to_string(currentPage + 1) + "/" + std::to_string(totalPages) + ".");
        state_.encounter.payloadIds.push_back(kPayloadCycleUpgrade);
        state_.encounter.payloadValues.push_back(static_cast<int>(upper));
    }
    state_.encounter.choices.push_back("Skip reward");
    state_.encounter.details.push_back("Leave the tuning lattice unused and continue.");
    state_.encounter.payloadIds.push_back(kPayloadSkipService);
    state_.encounter.payloadValues.push_back(0);
}

bool GameEngine::resolvePurgeSelection(size_t index) {
    if (index >= state_.encounter.choices.size()) {
        return false;
    }

    const std::string payload = index < state_.encounter.payloadIds.size() ? state_.encounter.payloadIds[index] : "";
    const int price = index < state_.encounter.payloadValues.size() ? state_.encounter.payloadValues[index] : 0;
    const bool marketContext = state_.encounter.encounterId == "market_purge";

    if (payload == kPayloadPrevPurge || payload == kPayloadCyclePurge) {
        if (marketContext) {
            enterMarketPurge(static_cast<size_t>(std::max(0, price)));
        } else {
            enterEventPurge(static_cast<size_t>(std::max(0, price)));
        }
        return true;
    }

    if (payload == kPayloadBackMarket) {
        state_.statusLine = "You return to the broker's main trade feed.";
        openMarketShop();
        return true;
    }

    if (payload == kPayloadSkipService || payload == "leave" || payload.empty()) {
        if (marketContext) {
            state_.statusLine = "You leave the purge console untouched.";
            openMarketShop();
        } else {
            state_.statusLine = "You leave the purge rig untouched.";
            finishCurrentNode();
        }
        return true;
    }

    if (marketContext && state_.marketPurgeUsed) {
        state_.statusLine = "The broker has already performed one purge this visit.";
        openMarketShop();
        return true;
    }
    if (state_.player.scrap < price) {
        state_.statusLine = "Not enough credits for that transaction.";
        return true;
    }
    if (!purgeCardFromDeck(payload)) {
        state_.statusLine = "The service cannot find that module in your active deck.";
        return true;
    }

    if (price > 0) {
        state_.player.scrap -= price;
    }
    state_.statusLine = displayCardName(payload) + " has been purged from your deck.";
    if (marketContext) {
        state_.marketPurgeUsed = true;
        openMarketShop();
    } else {
        finishCurrentNode();
    }
    return true;
}

bool GameEngine::resolveUpgradeSelection(size_t index) {
    if (index >= state_.encounter.choices.size()) {
        return false;
    }

    const std::string payload = index < state_.encounter.payloadIds.size() ? state_.encounter.payloadIds[index] : "";
    const int price = index < state_.encounter.payloadValues.size() ? state_.encounter.payloadValues[index] : 0;
    const bool marketContext = state_.encounter.encounterId == "market_upgrade";
    const bool repairContext = state_.encounter.encounterId == "repair_upgrade";
    const CardUpgradeSource source = marketContext ? CardUpgradeSource::Market
                                                   : (repairContext ? CardUpgradeSource::RepairStation
                                                                    : CardUpgradeSource::EventReward);

    if (payload == kPayloadPrevUpgrade || payload == kPayloadCycleUpgrade) {
        if (marketContext) {
            enterMarketUpgrade(static_cast<size_t>(std::max(0, price)));
        } else if (repairContext) {
            enterRepairUpgrade(static_cast<size_t>(std::max(0, price)));
        } else {
            enterEventUpgrade(static_cast<size_t>(std::max(0, price)));
        }
        return true;
    }

    if (payload == kPayloadBackMarket) {
        state_.statusLine = "You return to the broker's main trade feed.";
        openMarketShop();
        return true;
    }

    if (payload == kPayloadSkipService || payload.empty()) {
        if (marketContext) {
            state_.statusLine = "You leave the upgrade console untouched.";
            openMarketShop();
        } else if (repairContext) {
            state_.statusLine = "You leave the retrofit console unused.";
            finishCurrentNode();
        } else {
            state_.statusLine = "You leave the tuning lattice unused.";
            finishCurrentNode();
        }
        return true;
    }

    std::string error;
    if (!upgradeCard(payload, source, &error)) {
        state_.statusLine = error;
        return true;
    }

    if (marketContext) {
        openMarketShop();
    } else {
        finishCurrentNode();
    }
    return true;
}
