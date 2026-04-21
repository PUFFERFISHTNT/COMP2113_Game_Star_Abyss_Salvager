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
constexpr const char* kTemporaryCopyPrefix = "tmp|";

template <typename T>
void ShuffleVector(std::vector<T>& values, std::mt19937& rng) {
    std::shuffle(values.begin(), values.end(), rng);
}

}  // namespace

std::string GameEngine::baseCardId(const std::string& cardRef) const {
    const std::string normalized =
        cardRef.rfind(kTemporaryCopyPrefix, 0) == 0 ? cardRef.substr(4) : cardRef;
    const auto resolve_alias = [&](const std::string& token) -> std::string {
        const auto by_base_name = std::find_if(cardPool_.begin(), cardPool_.end(), [&](const CardDef& card) {
            return card.id == token || card.name == token || card.nameEn == token;
        });
        if (by_base_name != cardPool_.end()) {
            return by_base_name->id;
        }
        const auto by_upgrade_name = std::find_if(configuredUpgradePool_.begin(), configuredUpgradePool_.end(), [&](const CardDef& card) {
            return card.name == token || card.nameEn == token;
        });
        if (by_upgrade_name != configuredUpgradePool_.end()) {
            return by_upgrade_name->id;
        }
        return token;
    };
    const size_t separator = normalized.rfind('@');
    if (separator == std::string::npos) {
        return resolve_alias(normalized);
    }
    const std::string suffix = normalized.substr(separator + 1);
    if (suffix.empty() ||
        !std::all_of(suffix.begin(), suffix.end(), [](unsigned char ch) { return ch >= '0' && ch <= '9'; })) {
        return resolve_alias(normalized);
    }
    return normalized.substr(0, separator);
}

int GameEngine::cardUpgradeLevel(const std::string& cardRef) const {
    const std::string normalized =
        cardRef.rfind(kTemporaryCopyPrefix, 0) == 0 ? cardRef.substr(4) : cardRef;
    const auto alias_level = [&](const std::string& token) -> int {
        const auto found = std::find_if(configuredUpgradePool_.begin(), configuredUpgradePool_.end(), [&](const CardDef& card) {
            return card.name == token || card.nameEn == token;
        });
        return found == configuredUpgradePool_.end() ? 0 : 1;
    };
    const size_t separator = normalized.rfind('@');
    if (separator == std::string::npos) {
        return alias_level(normalized);
    }
    const std::string suffix = normalized.substr(separator + 1);
    if (suffix.empty() ||
        !std::all_of(suffix.begin(), suffix.end(), [](unsigned char ch) { return ch >= '0' && ch <= '9'; })) {
        return alias_level(normalized);
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

std::string GameEngine::canonicalCardRef(const std::string& cardRef) const {
    return makeCardRef(baseCardId(cardRef), cardUpgradeLevel(cardRef));
}

const CardDef* GameEngine::configuredUpgradeCardByBaseId(const std::string& cardId) const {
    const std::string baseId = baseCardId(cardId);
    const auto found = std::find_if(
        configuredUpgradePool_.begin(), configuredUpgradePool_.end(),
        [&](const CardDef& card) { return card.id == baseId; });
    return found == configuredUpgradePool_.end() ? nullptr : &(*found);
}

void GameEngine::normalizeUpgradedCards() {
    const auto normalize_pile = [&](std::vector<std::string>& pile) {
        for (std::string& ref : pile) {
            ref = canonicalCardRef(ref);
        }
    };
    normalize_pile(state_.player.drawPile);
    normalize_pile(state_.player.hand);
    normalize_pile(state_.player.discardPile);
    normalize_pile(state_.reward.cardIds);
    normalize_pile(state_.marketOfferCardIds);

    // Legacy saves stored upgrades by card type. Convert them into per-instance
    // upgrades by upgrading every matching copy that still exists in the deck.
    for (const std::string& legacyType : state_.upgradedCardTypes) {
        const std::string baseId = baseCardId(legacyType);
        if (baseId.empty()) {
            continue;
        }
        const auto upgrade_matching_refs = [&](std::vector<std::string>& pile) {
            for (std::string& ref : pile) {
                if (baseCardId(ref) == baseId) {
                    ref = makeCardRef(baseId, std::max(1, cardUpgradeLevel(ref)));
                }
            }
        };
        upgrade_matching_refs(state_.player.drawPile);
        upgrade_matching_refs(state_.player.hand);
        upgrade_matching_refs(state_.player.discardPile);
        upgrade_matching_refs(state_.reward.cardIds);
        upgrade_matching_refs(state_.marketOfferCardIds);
    }

    state_.upgradedCardTypes.clear();
}

std::string GameEngine::makeDeckInstancePayload(const std::string& pileId, size_t index) const {
    return "instance:" + pileId + ":" + std::to_string(index);
}

bool GameEngine::parseDeckInstancePayload(const std::string& payload, std::string* pileId, size_t* index) const {
    if (payload.rfind("instance:", 0) != 0) {
        return false;
    }
    const size_t first_separator = payload.find(':', 9);
    if (first_separator == std::string::npos) {
        return false;
    }
    const std::string parsedPileId = payload.substr(9, first_separator - 9);
    const std::string parsedIndex = payload.substr(first_separator + 1);
    if (parsedPileId.empty() || parsedIndex.empty() ||
        !std::all_of(parsedIndex.begin(), parsedIndex.end(), [](unsigned char ch) { return ch >= '0' && ch <= '9'; })) {
        return false;
    }
    try {
        if (pileId != nullptr) {
            *pileId = parsedPileId;
        }
        if (index != nullptr) {
            *index = static_cast<size_t>(std::stoul(parsedIndex));
        }
        return true;
    } catch (...) {
        return false;
    }
}

bool GameEngine::replaceDeckInstanceCardRef(const std::string& payload, const std::string& newCardRef) {
    std::string pileId;
    size_t index = 0;
    if (!parseDeckInstancePayload(payload, &pileId, &index)) {
        return false;
    }

    std::vector<std::string>* targetPile = nullptr;
    if (pileId == "hand") {
        targetPile = &state_.player.hand;
    } else if (pileId == "draw") {
        targetPile = &state_.player.drawPile;
    } else if (pileId == "discard") {
        targetPile = &state_.player.discardPile;
    } else {
        return false;
    }

    if (targetPile == nullptr || index >= targetPile->size()) {
        return false;
    }

    (*targetPile)[index] = canonicalCardRef(newCardRef);
    return true;
}

bool GameEngine::cardTypeSupportsUpgrade(const std::string& cardId) const {
    return configuredUpgradeCardByBaseId(cardId) != nullptr;
}

CardDef GameEngine::resolvedCard(const std::string& cardRef) const {
    const CardDef* base = cardById(cardRef);
    if (base == nullptr) {
        return CardDef{};
    }
    const int effectiveUpgradeLevel = cardUpgradeLevel(cardRef);
    if (effectiveUpgradeLevel <= 0) {
        return *base;
    }

    const CardDef* configured = configuredUpgradeCardByBaseId(base->id);
    if (configured != nullptr) {
        return *configured;
    }

    CardDef fallback = *base;
    fallback.name += "+++";
    if (!fallback.nameEn.empty()) {
        fallback.nameEn += "+++";
    }
    return fallback;
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
            result.push_back({canonicalCardRef(ref), resolvedCard(ref), cardUpgradeLevel(ref)});
        }
    }
    return result;
}

std::vector<CardView> GameEngine::handCards() const {
    std::vector<CardView> result;
    for (const std::string& ref : state_.player.hand) {
        if (cardById(ref) != nullptr) {
            result.push_back({canonicalCardRef(ref), resolvedCard(ref), cardUpgradeLevel(ref)});
        }
    }
    return result;
}

std::vector<CardView> GameEngine::drawPileCards() const {
    std::vector<CardView> result;
    result.reserve(state_.player.drawPile.size());
    for (auto it = state_.player.drawPile.rbegin(); it != state_.player.drawPile.rend(); ++it) {
        if (cardById(*it) != nullptr) {
            result.push_back({canonicalCardRef(*it), resolvedCard(*it), cardUpgradeLevel(*it)});
        }
    }
    return result;
}

std::vector<CardView> GameEngine::discardPileCards() const {
    std::vector<CardView> result;
    result.reserve(state_.player.discardPile.size());
    for (auto it = state_.player.discardPile.rbegin(); it != state_.player.discardPile.rend(); ++it) {
        if (cardById(*it) != nullptr) {
            result.push_back({canonicalCardRef(*it), resolvedCard(*it), cardUpgradeLevel(*it)});
        }
    }
    return result;
}

std::vector<CardOwnershipEntry> GameEngine::ownedCards() const {
    std::vector<std::string> owned_refs;
    owned_refs.insert(owned_refs.end(), state_.player.drawPile.begin(), state_.player.drawPile.end());
    owned_refs.insert(owned_refs.end(), state_.player.hand.begin(), state_.player.hand.end());
    owned_refs.insert(owned_refs.end(), state_.player.discardPile.begin(), state_.player.discardPile.end());

    for (std::string& ref : owned_refs) {
        ref = canonicalCardRef(ref);
    }

    std::sort(owned_refs.begin(), owned_refs.end(), [&](const std::string& lhs, const std::string& rhs) {
        const std::string left_name = displayCardName(lhs);
        const std::string right_name = displayCardName(rhs);
        if (left_name != right_name) {
            return left_name < right_name;
        }
        return lhs < rhs;
    });

    std::vector<CardOwnershipEntry> result;
    for (const std::string& ref : owned_refs) {
        if (cardById(ref) == nullptr) {
            continue;
        }
        if (!result.empty() && result.back().card.ref == ref) {
            ++result.back().copies;
            continue;
        }
        result.push_back({{ref, resolvedCard(ref), cardUpgradeLevel(ref)}, 1});
    }
    return result;
}

std::vector<CardView> GameEngine::ownedCardInstances() const {
    struct CardInstance {
        std::string pileId;
        size_t index = 0;
        std::string ref;
    };

    std::vector<CardInstance> instances;
    const auto collect = [&](const std::vector<std::string>& pile, const std::string& pileId) {
        for (size_t i = 0; i < pile.size(); ++i) {
            if (cardById(pile[i]) != nullptr) {
                instances.push_back({pileId, i, canonicalCardRef(pile[i])});
            }
        }
    };
    collect(state_.player.drawPile, "draw");
    collect(state_.player.hand, "hand");
    collect(state_.player.discardPile, "discard");

    std::sort(instances.begin(), instances.end(), [&](const CardInstance& lhs, const CardInstance& rhs) {
        const std::string left_name = displayCardName(lhs.ref);
        const std::string right_name = displayCardName(rhs.ref);
        if (left_name != right_name) {
            return left_name < right_name;
        }
        if (lhs.ref != rhs.ref) {
            return lhs.ref < rhs.ref;
        }
        if (lhs.pileId != rhs.pileId) {
            return lhs.pileId < rhs.pileId;
        }
        return lhs.index < rhs.index;
    });

    std::vector<CardView> result;
    result.reserve(instances.size());
    for (const CardInstance& instance : instances) {
        result.push_back({makeDeckInstancePayload(instance.pileId, instance.index),
                          resolvedCard(instance.ref),
                          cardUpgradeLevel(instance.ref)});
    }
    return result;
}

std::vector<CardLibraryEntry> GameEngine::allCardLibraryEntries() const {
    std::vector<const CardDef*> cards;
    cards.reserve(cardPool_.size());
    for (const CardDef& card : cardPool_) {
        cards.push_back(&card);
    }
    std::sort(cards.begin(), cards.end(), [&](const CardDef* lhs, const CardDef* rhs) {
        if (lhs->name != rhs->name) {
            return lhs->name < rhs->name;
        }
        return lhs->id < rhs->id;
    });

    std::vector<CardLibraryEntry> result;
    result.reserve(cards.size());
    for (const CardDef* card : cards) {
        result.push_back({{card->id, *card, 0}, hasObtainedCard(card->id)});
    }
    return result;
}

size_t GameEngine::obtainedCardCount() const {
    return obtainedCardIds_.size();
}

size_t GameEngine::totalCardCount() const {
    return cardPool_.size();
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

    std::string targetRef;
    std::string pileId;
    size_t pileIndex = 0;
    const bool payloadTargetsInstance = parseDeckInstancePayload(cardRef, &pileId, &pileIndex);
    if (payloadTargetsInstance) {
        const std::vector<std::string>* targetPile = nullptr;
        if (pileId == "hand") {
            targetPile = &state_.player.hand;
        } else if (pileId == "draw") {
            targetPile = &state_.player.drawPile;
        } else if (pileId == "discard") {
            targetPile = &state_.player.discardPile;
        }
        if (targetPile == nullptr || pileIndex >= targetPile->size()) {
            if (error != nullptr) {
                *error = "That card is not in the current deck.";
            }
            return false;
        }
        targetRef = canonicalCardRef((*targetPile)[pileIndex]);
    } else {
        targetRef = canonicalCardRef(cardRef);
    }

    const std::string baseId = baseCardId(targetRef);
    if (targetRef.empty() || baseId.empty() || cardById(baseId) == nullptr) {
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

    const int currentUpgradeLevel = cardUpgradeLevel(targetRef);
    if (currentUpgradeLevel >= kCardUpgradeMaxLevel) {
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

    const std::string upgradedRef = makeCardRef(baseId, currentUpgradeLevel + 1);
    if (payloadTargetsInstance) {
        if (!replaceDeckInstanceCardRef(cardRef, upgradedRef)) {
            if (price > 0) {
                state_.player.scrap += price;
            }
            if (error != nullptr) {
                *error = "That card is not in the current deck.";
            }
            return false;
        }
    } else {
        bool replaced = false;
        std::vector<std::vector<std::string>*> piles = {
            &state_.player.hand,
            &state_.player.drawPile,
            &state_.player.discardPile,
        };
        for (std::vector<std::string>* pile : piles) {
            for (std::string& ownedRef : *pile) {
                if (canonicalCardRef(ownedRef) == targetRef) {
                    ownedRef = upgradedRef;
                    replaced = true;
                    break;
                }
            }
            if (replaced) {
                break;
            }
        }
        if (!replaced) {
            if (price > 0) {
                state_.player.scrap += price;
            }
            if (error != nullptr) {
                *error = "That card is not in the current deck.";
            }
            return false;
        }
    }

    if (source == CardUpgradeSource::Market) {
        ++state_.marketUpgradeCount;
        state_.statusLine = displayCardName(upgradedRef) + " upgraded for " + std::to_string(price) + " credits.";
    } else if (source == CardUpgradeSource::RepairStation) {
        state_.statusLine = displayCardName(upgradedRef) + " upgraded for free at the repair station.";
    } else {
        state_.statusLine = displayCardName(upgradedRef) + " upgraded for free.";
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
            const std::string canonical = canonicalCardRef(cardRef);
            const std::string baseId = baseCardId(canonical);
            if (cardUpgradeLevel(canonical) < kCardUpgradeMaxLevel && cardTypeSupportsUpgrade(baseId)) {
                allCards.push_back(canonical);
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

std::vector<std::string> GameEngine::upgradeableCardInstanceRefs() const {
    std::vector<std::string> result;
    const auto collect = [&](const std::vector<std::string>& pile, const std::string& pileId) {
        for (size_t i = 0; i < pile.size(); ++i) {
            const std::string canonical = canonicalCardRef(pile[i]);
            const std::string baseId = baseCardId(canonical);
            if (cardById(baseId) == nullptr || !cardTypeSupportsUpgrade(baseId)) {
                continue;
            }
            if (cardUpgradeLevel(canonical) >= kCardUpgradeMaxLevel) {
                continue;
            }
            result.push_back(makeDeckInstancePayload(pileId, i));
        }
    };
    collect(state_.player.drawPile, "draw");
    collect(state_.player.hand, "hand");
    collect(state_.player.discardPile, "discard");

    std::sort(result.begin(), result.end(), [&](const std::string& lhs, const std::string& rhs) {
        std::string leftPile;
        std::string rightPile;
        size_t leftIndex = 0;
        size_t rightIndex = 0;
        parseDeckInstancePayload(lhs, &leftPile, &leftIndex);
        parseDeckInstancePayload(rhs, &rightPile, &rightIndex);
        const std::vector<std::string>* leftVector = leftPile == "draw"   ? &state_.player.drawPile
                                              : leftPile == "hand"   ? &state_.player.hand
                                                                     : &state_.player.discardPile;
        const std::vector<std::string>* rightVector = rightPile == "draw"   ? &state_.player.drawPile
                                               : rightPile == "hand"   ? &state_.player.hand
                                                                       : &state_.player.discardPile;
        const std::string leftRef = canonicalCardRef((*leftVector)[leftIndex]);
        const std::string rightRef = canonicalCardRef((*rightVector)[rightIndex]);
        const std::string leftName = displayCardName(leftRef);
        const std::string rightName = displayCardName(rightRef);
        if (leftName != rightName) {
            return leftName < rightName;
        }
        if (leftRef != rightRef) {
            return leftRef < rightRef;
        }
        if (leftPile != rightPile) {
            return leftPile < rightPile;
        }
        return leftIndex < rightIndex;
    });
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
    const std::vector<std::string> candidates = upgradeableCardInstanceRefs();
    if (candidates.empty()) {
        if (error != nullptr) {
            *error = "No cards can be upgraded right now.";
        }
        return false;
    }

    std::vector<std::string> candidateIds = candidates;
    ShuffleVector(candidateIds, rng_);

    std::string pileId;
    size_t pileIndex = 0;
    parseDeckInstancePayload(candidateIds.front(), &pileId, &pileIndex);
    const std::vector<std::string>* pile = pileId == "draw"   ? &state_.player.drawPile
                                           : pileId == "hand" ? &state_.player.hand
                                                              : &state_.player.discardPile;
    const std::string upgradedName =
        (pile != nullptr && pileIndex < pile->size()) ? displayCardName((*pile)[pileIndex]) : "Card";
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
        state_.marketOfferCardIds = randomMarketCardChoices(3);
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
    const std::vector<std::string> candidates = upgradeableCardInstanceRefs();
    const int price = cardUpgradeCost(CardUpgradeSource::Market);
    (void)offset;

    state_.encounter = {};
    state_.encounter.typeId = "market";
    state_.encounter.encounterId = "market_upgrade";
    state_.encounter.title = "Black Market Upgrade";
    state_.encounter.description =
        "Select one card to upgrade. Current price: " + std::to_string(price) +
        " credits. The full deck is shown below; scroll vertically if needed.";
    state_.page = AppPage::Encounter;
    resumePage_ = AppPage::Encounter;

    for (const std::string& candidate : candidates) {
        std::string pileId;
        size_t pileIndex = 0;
        parseDeckInstancePayload(candidate, &pileId, &pileIndex);
        const std::vector<std::string>* pile = pileId == "draw"   ? &state_.player.drawPile
                                               : pileId == "hand" ? &state_.player.hand
                                                                  : &state_.player.discardPile;
        const std::string cardRef = (pile != nullptr && pileIndex < pile->size()) ? canonicalCardRef((*pile)[pileIndex]) : "";
        state_.encounter.choices.push_back(displayCardName(cardRef));
        state_.encounter.details.push_back("Upgrade exactly 1 copy in the deck.");
        state_.encounter.payloadIds.push_back(candidate);
        state_.encounter.payloadValues.push_back(price);
    }
    state_.encounter.choices.push_back("Back to broker");
    state_.encounter.details.push_back("Return to the black market offers.");
    state_.encounter.payloadIds.push_back(kPayloadBackMarket);
    state_.encounter.payloadValues.push_back(0);
}

void GameEngine::enterRepairUpgrade(size_t offset) {
    const std::vector<std::string> candidates = upgradeableCardInstanceRefs();
    (void)offset;

    state_.encounter = {};
    state_.encounter.typeId = "repair";
    state_.encounter.encounterId = "repair_upgrade";
    state_.encounter.title = "Repair Station Retrofit";
    state_.encounter.description =
        "Select one card to upgrade for free at the repair station. The full deck is shown below; scroll vertically if needed.";
    state_.page = AppPage::Encounter;
    resumePage_ = AppPage::Encounter;

    for (const std::string& candidate : candidates) {
        std::string pileId;
        size_t pileIndex = 0;
        parseDeckInstancePayload(candidate, &pileId, &pileIndex);
        const std::vector<std::string>* pile = pileId == "draw"   ? &state_.player.drawPile
                                               : pileId == "hand" ? &state_.player.hand
                                                                  : &state_.player.discardPile;
        const std::string cardRef = (pile != nullptr && pileIndex < pile->size()) ? canonicalCardRef((*pile)[pileIndex]) : "";
        state_.encounter.choices.push_back(displayCardName(cardRef));
        state_.encounter.details.push_back("Upgrade exactly 1 copy in the deck.");
        state_.encounter.payloadIds.push_back(candidate);
        state_.encounter.payloadValues.push_back(0);
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
    const std::vector<std::string> candidates = upgradeableCardInstanceRefs();
    (void)offset;

    state_.encounter = {};
    state_.encounter.typeId = "event";
    state_.encounter.encounterId = "event_upgrade";
    state_.encounter.title = "Signal Calibration";
    state_.encounter.description =
        "A tuning lattice lets you upgrade one card for free. The full deck is shown below; scroll vertically if needed.";
    state_.page = AppPage::Encounter;
    resumePage_ = AppPage::Encounter;

    for (const std::string& candidate : candidates) {
        std::string pileId;
        size_t pileIndex = 0;
        parseDeckInstancePayload(candidate, &pileId, &pileIndex);
        const std::vector<std::string>* pile = pileId == "draw"   ? &state_.player.drawPile
                                               : pileId == "hand" ? &state_.player.hand
                                                                  : &state_.player.discardPile;
        const std::string cardRef = (pile != nullptr && pileIndex < pile->size()) ? canonicalCardRef((*pile)[pileIndex]) : "";
        state_.encounter.choices.push_back(displayCardName(cardRef));
        state_.encounter.details.push_back("Upgrade exactly 1 copy in the deck.");
        state_.encounter.payloadIds.push_back(candidate);
        state_.encounter.payloadValues.push_back(0);
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
