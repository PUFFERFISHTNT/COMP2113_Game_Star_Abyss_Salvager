#include "engine/game_battle.h"

#include <algorithm>
#include <array>
#include <functional>

extern int g_sa_permanent_shield;

namespace {

constexpr int kPlayerFormNeutral = 0;
constexpr int kPlayerFormOverload = 1;
constexpr int kPlayerFormDefense = 2;
constexpr int kHighHeatThreshold = 6;
constexpr int kFirstSummonScopeBattleOnce = 1;
constexpr int kFirstSummonScopePerTurn = 2;
constexpr const char* kTemporaryCopyPrefix = "tmp|";
constexpr const char* kAuxUnitInterceptor = "aux_interceptor";
constexpr const char* kAuxUnitGuardian = "aux_guardian";
constexpr const char* kAuxUnitRepair = "aux_repair";
constexpr const char* kAuxUnitPulse = "aux_pulse";
constexpr const char* kAuxUnitQueenWasp = "aux_queen_wasp";
constexpr const char* kAuxUnitThermalEscort = "aux_thermal_escort";
constexpr const char* kAuxUnitBurner = "aux_burner";

struct AuxiliaryUnitRecipe {
    std::string unitId;
    std::string displayName;
    int attack = 0;
    int shield = 0;
    int heal = 0;
    int endTurnDamage = 0;
    int endTurnEnemyHeat = 0;
    int endTurnExtraOtherTriggers = 0;
};

enum class AuxiliaryTriggerContext {
    Immediate = 0,
    EndTurn,
};

int MaxAuxiliaryShipSlots(const PlayerState& player) {
    (void)player;
    return kMaxAuxiliaryShips;
}

std::pair<std::string, std::string> AuxiliaryUnitProfileForCard(const CardDef& card) {
    // Keep summon cards data-driven while still providing stable type ids for UI.
    const std::string lowered = card.name;
    if (lowered.find("Repair") != std::string::npos || lowered.find("维修") != std::string::npos) {
        return {kAuxUnitRepair, "Repair Drone"};
    }
    if (lowered.find("Pulse") != std::string::npos || lowered.find("脉冲") != std::string::npos) {
        return {kAuxUnitPulse, "Pulse Drone"};
    }
    if (lowered.find("Guard") != std::string::npos || lowered.find("护卫") != std::string::npos ||
        card.summonDroneShield > card.summonDroneAttack) {
        return {kAuxUnitGuardian, "Guardian Drone"};
    }
    if (lowered.find("Intercept") != std::string::npos || lowered.find("拦截") != std::string::npos ||
        card.summonDroneAttack >= card.summonDroneShield) {
        return {kAuxUnitInterceptor, "Interceptor Drone"};
    }
    return {"aux_drone", "Auxiliary Drone"};
}

std::array<std::string, 4> BasicAuxiliaryUnitIds() {
    return {kAuxUnitInterceptor, kAuxUnitGuardian, kAuxUnitRepair, kAuxUnitPulse};
}

AuxiliaryUnitRecipe BasicAuxiliaryRecipeForId(const std::string& unit_id, bool upgraded_tier) {
    if (unit_id == kAuxUnitGuardian) {
        return {kAuxUnitGuardian, "Guardian Drone", 0, upgraded_tier ? 4 : 3, 0, 0, 0, 0};
    }
    if (unit_id == kAuxUnitRepair) {
        return {kAuxUnitRepair, "Repair Drone", 0, 0, upgraded_tier ? 4 : 3, 0, 0, 0};
    }
    if (unit_id == kAuxUnitPulse) {
        return {kAuxUnitPulse, "Pulse Drone", upgraded_tier ? 3 : 2, upgraded_tier ? 3 : 2, 0, 0, 0, 0};
    }
    return {kAuxUnitInterceptor, "Interceptor Drone", upgraded_tier ? 4 : 3, 0, 0, 0, 0, 0};
}

AuxiliaryUnitRecipe AuxiliaryRecipeForCard(const std::string& base_card_id, const CardDef& card) {
    if (base_card_id == "df_27") {
        return {kAuxUnitQueenWasp, "Queen Wasp Drone", std::max(0, card.summonDroneAttack),
                std::max(0, card.summonDroneShield), 0, 0, 0,
                std::max(0, card.auxEndTurnExtraTriggerTimes)};
    }
    if (base_card_id == "dfh_08") {
        return {kAuxUnitThermalEscort, "Thermal Escort Drone", std::max(0, card.summonDroneAttack),
                std::max(0, card.summonDroneShield), 0, 0, std::max(0, card.enemyHeat),
                std::max(0, card.auxEndTurnExtraTriggerTimes)};
    }
    if (base_card_id == "dfh_01") {
        return {kAuxUnitBurner, "Burner Drone", 0, 0, 0, 0, std::max(0, card.enemyHeat), 0};
    }

    const auto profile = AuxiliaryUnitProfileForCard(card);
    const int shield = profile.first == kAuxUnitRepair ? 0 : std::max(0, card.summonDroneShield);
    const int heal = profile.first == kAuxUnitRepair ? std::max(0, card.summonDroneShield) : 0;
    return {profile.first, profile.second, std::max(0, card.summonDroneAttack), shield, heal, 0, 0, 0};
}

AuxiliaryShipState MakeAuxiliaryShipState(const AuxiliaryUnitRecipe& recipe, int remaining_turns) {
    AuxiliaryShipState ship;
    ship.unitId = recipe.unitId;
    ship.displayName = recipe.displayName;
    ship.attack = recipe.attack;
    ship.shield = recipe.shield;
    ship.heal = recipe.heal;
    ship.remainingTurns = remaining_turns;
    ship.endTurnDamage = recipe.endTurnDamage;
    ship.endTurnEnemyHeat = recipe.endTurnEnemyHeat;
    ship.endTurnExtraOtherTriggers = recipe.endTurnExtraOtherTriggers;
    return ship;
}

bool IsUpgradedBasicAuxiliaryTier(const AuxiliaryShipState& ship) {
    if (ship.unitId == kAuxUnitPulse) {
        return ship.attack >= 3 || ship.shield >= 3;
    }
    if (ship.unitId == kAuxUnitGuardian) {
        return ship.shield >= 4;
    }
    if (ship.unitId == kAuxUnitRepair) {
        return ship.heal >= 4;
    }
    if (ship.unitId == kAuxUnitInterceptor) {
        return ship.attack >= 4;
    }
    return ship.attack >= 4 || ship.shield >= 4 || ship.heal >= 4 || ship.attack + ship.shield + ship.heal >= 6;
}

int EffectiveCardCost(const PlayerState& player, const CardDef& card) {
    return player.firstCardCostZeroThisTurn ? 0 : std::max(0, card.cost);
}

template <typename T>
void ShuffleVector(std::vector<T>& values, std::mt19937& rng) {
    std::shuffle(values.begin(), values.end(), rng);
}

bool IsAttackOrHybridSummon(const CardDef& card) {
    return card.summonDroneAttack >= card.summonDroneShield;
}

bool IsAbilityCard(const CardDef& card) {
    return card.type == "Ability";
}

}  // namespace

bool GameEngine::canPlayCard(size_t index) const {
    if (state_.page != AppPage::Battle) {
        return false;
    }
    if (state_.player.blackboxSelectionPending) {
        return index < state_.player.hand.size() && !isTemporaryCopyRef(state_.player.hand[index]);
    }
    if (state_.player.auxTransformSelectionPending) {
        return false;
    }
    if (index >= state_.player.hand.size()) {
        return false;
    }
    const std::string cardRef = unwrapTemporaryCopyRef(state_.player.hand[index]);
    const CardDef* baseCard = cardById(cardRef);
    if (baseCard == nullptr) {
        return false;
    }
    return EffectiveCardCost(state_.player, resolvedCard(cardRef)) <= state_.player.energy;
}

void GameEngine::playCard(size_t index) {
    if (state_.player.blackboxSelectionPending) {
        if (!resolveBlackboxProtocol(index)) {
            state_.statusLine = "Select a valid hand card to imprint for Blackbox Protocol.";
            return;
        }
        if (state_.enemy.hp <= 0) {
            handleBattleVictory();
        }
        return;
    }
    if (state_.player.auxTransformSelectionPending) {
        state_.statusLine = state_.player.auxTransformTargetChoicePending
            ? "Finish Wing Rotation by choosing a target drone type."
            : "Finish Wing Rotation by choosing an auxiliary ship.";
        return;
    }

    if (!canPlayCard(index)) {
        state_.statusLine = "That module cannot be fired right now.";
        return;
    }

    const std::string cardId = state_.player.hand[index];
    const bool temporaryCopy = isTemporaryCopyRef(cardId);
    const std::string resolvedRef = unwrapTemporaryCopyRef(cardId);
    const CardDef* baseCard = cardById(resolvedRef);
    if (baseCard == nullptr) {
        return;
    }
    CardDef card = resolvedCard(resolvedRef);
    const int playCost = EffectiveCardCost(state_.player, card);
    state_.player.energy -= playCost;
    if (state_.player.firstCardCostZeroThisTurn) {
        state_.player.firstCardCostZeroThisTurn = false;
        log("Stored reverse-entropy window makes this card cost 0.");
    }
    if (IsAbilityCard(card)) {
        // Ability cards leave the combat deck after being played and remain
        // visible as battle-long effects until the encounter ends.
        ActiveAbilityState ability;
        ability.cardRef = temporaryCopy ? "" : resolvedRef;
        ability.name = card.name;
        ability.nameEn = card.nameEn;
        ability.description = card.description;
        ability.descriptionEn = card.descriptionEn;
        state_.player.activeAbilities.push_back(ability);
    } else if (!temporaryCopy) {
        state_.player.discardPile.push_back(resolvedRef);
    }
    state_.player.hand.erase(state_.player.hand.begin() + static_cast<long long>(index));

    const bool playerHighHeat = state_.player.heat >= kHighHeatThreshold;
    const bool enemyHighHeat = state_.enemy.heat >= kHighHeatThreshold;
    const bool inDefense = state_.player.form == kPlayerFormDefense;
    const bool inOverload = state_.player.form == kPlayerFormOverload;
    const bool playedSummonCard = card.summonDroneCount > 0;
    const std::string baseCardId = baseCard != nullptr ? baseCard->id : "";
    int outgoingDamage = card.damage;
    int drawCount = card.draw;
    int enemyHeatDelta = card.enemyHeat;
    int selfDamageValue = card.selfDamage;
    int shieldValue = card.shield;
    int energyValue = card.energy;
    int multiHitValue = std::max(1, card.multiHit);
    int extraTwoDamageHits = 0;
    const int clearedHeat = state_.player.heat;
    const int activeAuxCount = static_cast<int>(state_.auxiliaryShips.size());
    const int activeInterceptorAuxCount = static_cast<int>(std::count_if(
        state_.auxiliaryShips.begin(), state_.auxiliaryShips.end(),
        [](const AuxiliaryShipState& ship) { return ship.unitId == kAuxUnitInterceptor; }));
    const int activeGuardianAuxCount = static_cast<int>(std::count_if(
        state_.auxiliaryShips.begin(), state_.auxiliaryShips.end(),
        [](const AuxiliaryShipState& ship) { return ship.unitId == kAuxUnitGuardian; }));
    const bool hasPulseAux = std::any_of(
        state_.auxiliaryShips.begin(), state_.auxiliaryShips.end(),
        [](const AuxiliaryShipState& ship) { return ship.unitId == kAuxUnitPulse; });
    const int uniqueAuxTypes = static_cast<int>([&]() {
        std::vector<std::string> ids;
        for (const AuxiliaryShipState& ship : state_.auxiliaryShips) {
            if (std::find(ids.begin(), ids.end(), ship.unitId) == ids.end()) {
                ids.push_back(ship.unitId);
            }
        }
        return ids.size();
    }());

    if (baseCardId == "vg_01" && state_.player.heat > 0) {
        outgoingDamage += 2;
    }
    if (baseCardId == "vg_03" && state_.player.heat >= 4) {
        shieldValue += 2;
    }
    if (baseCardId == "vg_07" && state_.player.heatGainedThisTurn > 0) {
        shieldValue += 3;
    }
    if (baseCardId == "vg_08" && state_.player.heat != 0) {
        card.heat = 0;
    }
    if (baseCardId == "vg_14" && state_.player.heat > 0) {
        outgoingDamage += 4;
    }
    if (baseCardId == "vg_09" && playerHighHeat) {
        outgoingDamage += 3;
    }
    if (baseCardId == "vg_11") {
        if (playerHighHeat) {
            outgoingDamage += 4;
        } else {
            selfDamageValue = 0;
        }
    }
    if (baseCardId == "vg_13") {
        state_.player.nextHeatGainReductionThisTurn += std::max(0, card.heat);
        card.heat = 0;
    }
    if (baseCardId == "vg_15") {
        if (state_.player.heat > 0) {
            shieldValue = 0;
        } else {
            drawCount = 0;
        }
    }
    if (baseCardId == "df_06" && activeAuxCount > 0) {
        outgoingDamage += activeAuxCount;
    }
    if (baseCardId == "df_08" && activeAuxCount == 0) {
        shieldValue = 0;
    }
    if (baseCardId == "df_09" && activeAuxCount > 0) {
        card.summonDroneCount = 0;
    }
    if (baseCardId == "df_10" && activeAuxCount == 0) {
        energyValue = 0;
    }
    if (baseCardId == "df_11") {
        outgoingDamage += activeInterceptorAuxCount * 2;
    }
    if (baseCardId == "df_12") {
        shieldValue += activeGuardianAuxCount * 2;
    }
    if (baseCardId == "df_13" && hasPulseAux) {
        drawCount = 3;
    }
    if (baseCardId == "df_20" && activeGuardianAuxCount < 2) {
        drawCount = 0;
    }
    if (baseCardId == "df_21") {
        const int destroyedAuxCount = activeAuxCount;
        outgoingDamage = card.damage * destroyedAuxCount;
        drawCount = card.draw * destroyedAuxCount;
        while (!state_.auxiliaryShips.empty()) {
            AuxiliaryShipState ship = state_.auxiliaryShips.front();
            state_.auxiliaryShips.erase(state_.auxiliaryShips.begin());
            onAuxiliaryShipRemoved(ship, "furnace");
        }
        if (destroyedAuxCount > 0) {
            log("Drone Furnace dismantles " + std::to_string(destroyedAuxCount) + " auxiliary ship(s).");
        }
    }
    if (baseCardId == "df_22" && activeAuxCount > 0) {
        outgoingDamage += activeAuxCount * 3;
    }
    if (baseCardId == "df_27") {
        outgoingDamage = 0;
    }
    if (baseCardId == "dfh_08") {
        enemyHeatDelta = 0;
    }
    if (baseCardId == "dfh_09") {
        enemyHeatDelta *= activeInterceptorAuxCount;
    }
    if (baseCardId == "df_30" && activeAuxCount > 0) {
        extraTwoDamageHits = activeAuxCount;
    }
    if (baseCardId == "ib_05" && inOverload) {
        outgoingDamage = 9;
    }
    if (baseCardId == "ib_27" && inOverload) {
        outgoingDamage = 34;
    }
    if (baseCardId == "ib_03" && state_.player.exitedFormThisTurn) {
        outgoingDamage += 3;
    }
    if (baseCardId == "ib_06" && !inDefense) {
        energyValue = 0;
    }
    if (baseCardId == "ib_04" && !inDefense) {
        drawCount = 0;
    }
    if (baseCardId == "ib_07" && state_.player.exitedDefenseThisTurn) {
        shieldValue += 4;
    }
    if (baseCardId == "ib_12") {
        if (inDefense) {
            drawCount = 0;
        } else {
            energyValue = 0;
        }
    }
    if (baseCardId == "ib_13") {
        card.attackMultiplier = 2;
        if (inOverload) {
            card.attackMultiplier = 1;
            state_.player.nextAttackPreMultiplierBonusPercentThisTurn =
                std::max(state_.player.nextAttackPreMultiplierBonusPercentThisTurn, 50);
        }
    }
    if (baseCardId == "ib_15" && state_.player.form == kPlayerFormNeutral) {
        shieldValue = 0;
    }
    if (baseCardId == "ib_11" && state_.player.formChangesThisTurn == 0) {
        drawCount = 0;
    }
    if (baseCardId == "ib_20" && state_.player.form != kPlayerFormDefense) {
        drawCount = 0;
    }
    if (baseCardId == "ibh_03") {
        enemyHeatDelta = state_.player.formChangesThisTurn > 0 ? card.enemyHeat : 2;
    }
    if (baseCardId == "vgh_02" && state_.player.heatGainedThisTurn == 0) {
        drawCount = 0;
    }
    if (baseCardId == "vgh_03") {
        enemyHeatDelta = playerHighHeat ? 5 : 3;
    }
    if (baseCardId == "vgh_06") {
        enemyHeatDelta = 0;
        state_.player.nextAttackEnemyHeatBonusThisTurn += std::max(0, card.enemyHeat);
    }
    if (baseCardId == "dfh_02") {
        state_.player.auxTriggerEnemyHeatBonusThisTurn += 1;
    }
    if (baseCardId == "dfh_07") {
        state_.player.auxTriggerEnemyHeatBonusThisTurn += 1;
    }
    if (baseCardId == "dfh_03" && activeAuxCount > 0) {
        outgoingDamage += 2;
    }
    if (baseCardId == "dfh_04" && !hasPulseAux) {
        drawCount = 0;
    }
    if (baseCardId == "dfh_06" && activeAuxCount < 2) {
        energyValue = 0;
    }
    if (baseCardId == "ibh_05" && inOverload) {
        outgoingDamage += 2;
    }

    if (card.formChangesThisTurnThreshold > 0 &&
        state_.player.formChangesThisTurn >= card.formChangesThisTurnThreshold) {
        outgoingDamage += std::max(0, card.formChangesThisTurnBonusDamage);
        log("Conditional fire-control bonus is active this turn.");
    }
    if (baseCardId == "vg_17" && playerHighHeat) {
        outgoingDamage = 25;
    }
    if (baseCardId == "vg_18") {
        state_.player.attackBonusPerHeatThisTurn = std::max(state_.player.attackBonusPerHeatThisTurn, card.damage);
        state_.player.attackBonusPerHeatCapThisTurn = std::max(state_.player.attackBonusPerHeatCapThisTurn, 6);
    }
    if (baseCardId == "vg_20" && state_.player.heatReducedThisTurn > 0) {
        multiHitValue += 1;
    }
    if (baseCardId == "vg_21") {
        state_.player.overheatSelfDamageBlockThisTurn = std::max(state_.player.overheatSelfDamageBlockThisTurn, 3);
    }
    if (baseCardId == "vg_23" && !playerHighHeat) {
        drawCount = 0;
    }
    if (baseCardId == "vg_26" && playerHighHeat) {
        state_.enemy.shield = 0;
    }
    if (baseCardId == "vg_27" && playerHighHeat) {
        outgoingDamage += 12;
    }
    if (baseCardId == "vg_28") {
        state_.player.attackBonusPerHeatGainPointThisBattle =
            std::max(state_.player.attackBonusPerHeatGainPointThisBattle, 1);
    }
    if (baseCardId == "df_26" && uniqueAuxTypes < 3) {
        drawCount = 0;
        energyValue = 0;
    }
    if (baseCardId == "df_29") {
        state_.player.auxOnRemoveDrawThisBattle = std::max(state_.player.auxOnRemoveDrawThisBattle, card.draw);
        state_.player.auxOnRemoveEnergyThisBattle = std::max(state_.player.auxOnRemoveEnergyThisBattle, card.energy);
        drawCount = 0;
        energyValue = 0;
    }
    if (baseCardId == "ib_17" && inOverload) {
        outgoingDamage += 5;
    }
    if (baseCardId == "ib_18") {
        drawCount = 0;
        state_.player.formChangeDrawThisTurn += std::max(0, card.draw);
    }
    if (baseCardId == "ib_14") {
        state_.player.leaveDefenseShieldBonusThisTurn =
            std::max(state_.player.leaveDefenseShieldBonusThisTurn, card.leaveDefenseShieldBonus);
        shieldValue = card.shield;
    }
    if (baseCardId == "ib_19") {
        state_.player.nextIncomingDamageFlatReductionThisTurn =
            std::max(state_.player.nextIncomingDamageFlatReductionThisTurn, 8);
    }
    if (baseCardId == "ib_23") {
        if (state_.player.enteredOverloadThisTurn && state_.player.exitedDefenseThisTurn) {
            outgoingDamage = 19;
        }
    }
    if (baseCardId == "ib_26") {
        if (inDefense) {
            energyValue = 1;
            card.formExit = true;
        } else {
            shieldValue = 0;
            energyValue = 0;
        }
    }
    if (baseCardId == "ib_32") {
        if (inDefense) {
            energyValue = 1;
            card.formExit = true;
        } else {
            energyValue = 0;
        }
    }
    if (baseCardId == "ib_21") {
        if (inOverload) {
            card.formExit = true;
            card.formShift = 0;
        } else {
            energyValue = 0;
        }
    }
    if (baseCardId == "ib_28") {
        state_.player.enterDefenseShieldThisBattle = std::max(state_.player.enterDefenseShieldThisBattle, card.shield);
        state_.player.exitDefenseEnergyThisBattle = std::max(state_.player.exitDefenseEnergyThisBattle, card.energy);
        shieldValue = 0;
        energyValue = 0;
    }
    if (baseCardId == "ib_29") {
        state_.player.firstEnterOverloadDrawThisBattle =
            std::max(state_.player.firstEnterOverloadDrawThisBattle, card.draw);
        state_.player.firstExitDefenseEnergyThisBattle =
            std::max(state_.player.firstExitDefenseEnergyThisBattle, card.energy);
        drawCount = 0;
        energyValue = 0;
    }
    if (baseCardId == "vgh_07" && state_.player.enemyHeatRaisedThisTurn > 0) {
        enemyHeatDelta += card.enemyHeat;
    }
    if (baseCardId == "vgh_08") {
        card.heat += 4;
        enemyHeatDelta += 4;
        if (playerHighHeat) {
            outgoingDamage += 7;
        }
    }
    if (baseCardId == "vgh_09") {
        enemyHeatDelta = enemyHighHeat ? 8 : card.enemyHeat;
    }
    if (baseCardId == "ibh_07") {
        if (inOverload) {
            shieldValue = 0;
            enemyHeatDelta = card.enemyHeat;
        } else if (inDefense) {
            shieldValue = card.shield;
            enemyHeatDelta = 4;
        } else {
            shieldValue = 0;
            enemyHeatDelta = 0;
        }
    }
    if (baseCardId == "ibh_09" && enemyHighHeat) {
        outgoingDamage += 5;
        enemyHeatDelta += 4;
    }
    if (baseCardId == "ibh_06" && !inDefense) {
        drawCount = 0;
    }
    if (baseCardId == "vg_25") {
        const int heatChunks = std::min(3, std::max(0, state_.player.heat / 2));
        drawCount = heatChunks * std::max(0, card.draw);
    }
    if (baseCardId == "vgh_10") {
        enemyHeatDelta = std::min(std::max(0, clearedHeat), std::max(0, card.enemyHeat));
    }
    if (baseCardId == "cl_08") {
        drawCount = 0;
        state_.player.nextTurnDraw += std::max(0, card.draw);
        state_.player.nextTurnEnergy += std::max(0, card.nextTurnEnergy);
        state_.player.firstCardCostZeroPendingNextTurn = true;
        card.nextTurnEnergy = 0;
    }
    if (baseCardId == "cl_06" && state_.player.attacksPlayedThisTurn == 0) {
        shieldValue += 7;
    } else if (baseCardId == "cl_06") {
        drawCount = 0;
    }
    if (card.comboBonus > 0 && state_.player.attacksPlayedThisTurn > 0) {
        outgoingDamage += card.comboBonus;
    }
    if (outgoingDamage > 0) {
        if (state_.player.attackBonusPerHeatThisTurn > 0) {
            const int cap = std::max(0, state_.player.attackBonusPerHeatCapThisTurn);
            const int scaled = state_.player.heat * state_.player.attackBonusPerHeatThisTurn;
            outgoingDamage += cap > 0 ? std::min(cap, scaled) : scaled;
        }
        outgoingDamage += state_.player.attackBonusFromHeatGainThisBattle;
        outgoingDamage += state_.player.killAttackBonusThisBattle;
    }
    if (card.shieldToDamage) {
        outgoingDamage += state_.player.shield;
        state_.player.shield = 0;
    }
    if (outgoingDamage > 0 &&
        (state_.player.nextAttackMultiplier > 1 || state_.player.nextAttackPreMultiplierBonusPercentThisTurn > 0)) {
        if (state_.player.nextAttackPreMultiplierBonusPercentThisTurn > 0) {
            outgoingDamage += (outgoingDamage * state_.player.nextAttackPreMultiplierBonusPercentThisTurn) / 100;
        }
        outgoingDamage *= state_.player.nextAttackMultiplier;
        state_.player.nextAttackMultiplier = 1;
        state_.player.nextAttackPreMultiplierBonusPercentThisTurn = 0;
    }
    log("You route " + card.name + ".");
    if (temporaryCopy) {
        log("Temporary copy dissipates after use.");
    }

    if (card.attackMultiplier > 1) {
        state_.player.nextAttackMultiplier *= card.attackMultiplier;
        log("Target lock acquired. The next attack will strike harder.");
    }
    if (shieldValue > 0) {
        state_.player.shield += shieldValue;
        log("Shield rises by " + std::to_string(shieldValue) + ".");
    }
    if (card.heal > 0) {
        state_.player.hp = std::min(state_.player.maxHp, state_.player.hp + card.heal);
        log("Hull restores " + std::to_string(card.heal) + " HP.");
    }
    if (drawCount > 0) {
        drawCards(drawCount);
    }
    if (energyValue > 0) {
        state_.player.energy += energyValue;
        log("Reactor gain: +" + std::to_string(energyValue) + " energy.");
    }
    if (card.nextTurnEnergy != 0) {
        state_.player.nextTurnEnergy += card.nextTurnEnergy;
        log("Next turn energy buffered: +" + std::to_string(card.nextTurnEnergy) + ".");
    }
    if (card.halveHeat) {
        state_.player.heat /= 2;
        syncPlayerOverheatState();
        log("Heat is cut in half.");
    }
    if (card.clearHeat) {
        state_.player.heat = 0;
        syncPlayerOverheatState();
        log("All heat is vented from the reactor.");
    }
    if (baseCardId == "vg_29" && clearedHeat > 0) {
        state_.player.shield += clearedHeat;
        log("Zero-Bound Cooling converts " + std::to_string(clearedHeat) + " cleared heat into shield.");
    }
    if (card.heat > 0 && state_.player.nextHeatGainReductionThisTurn > 0) {
        card.heat = std::max(0, card.heat - state_.player.nextHeatGainReductionThisTurn);
        state_.player.nextHeatGainReductionThisTurn = 0;
    }
    if (card.heat != 0) {
        changeHeat(card.heat);
        if (card.heat > 0) {
            log("Heat climbs by " + std::to_string(card.heat) + ".");
        } else {
            log("Heat drops by " + std::to_string(-card.heat) + ".");
        }
    }
    if ((card.type == "Attack" || outgoingDamage > 0) && state_.player.nextAttackEnemyHeatBonusThisTurn > 0) {
        enemyHeatDelta += state_.player.nextAttackEnemyHeatBonusThisTurn;
        state_.player.nextAttackEnemyHeatBonusThisTurn = 0;
    }
    if (enemyHeatDelta != 0) {
        changeEnemyHeat(enemyHeatDelta);
        if (enemyHeatDelta > 0) {
            log("Enemy heat climbs by " + std::to_string(enemyHeatDelta) + ".");
        } else {
            log("Enemy heat drops by " + std::to_string(-enemyHeatDelta) + ".");
        }
    }
    if (selfDamageValue > 0) {
        applyDamageToPlayer(selfDamageValue);
        log("Feedback lashes your own hull for " + std::to_string(selfDamageValue) + ".");
    }
    if (card.weakenTurns > 0) {
        state_.enemy.weakenTurns = std::max(state_.enemy.weakenTurns, card.weakenTurns);
        log("Enemy fire-control is weakened.");
    }
    if (card.removeEnemyShield > 0) {
        const int removed = std::min(state_.enemy.shield, card.removeEnemyShield);
        state_.enemy.shield = std::max(0, state_.enemy.shield - card.removeEnemyShield);
        if (removed > 0) {
            log("Enemy shield stripped by " + std::to_string(removed) + ".");
        }
    }
    if (card.shieldLockTurns > 0) {
        state_.enemy.shieldLockTurns = std::max(state_.enemy.shieldLockTurns, card.shieldLockTurns);
        log("Enemy shield emitters are jammed.");
    }
    if (card.salvageBonus > 0) {
        state_.player.salvageBonus += card.salvageBonus;
        log("Battle credit payout increased by " + std::to_string(card.salvageBonus) + ".");
    }

    if (card.randomEffect) {
        std::uniform_int_distribution<int> outcome(0, 3);
        const int roll = outcome(rng_);
        if (roll == 0) {
            outgoingDamage += 18;
            changeHeat(2);
            log("Blind jump locks on target. Massive burst incoming.");
        } else if (roll == 1) {
            state_.player.shield += 12;
            drawCards(1);
            log("Blind jump returns with a mirrored shield bloom.");
        } else if (roll == 2) {
            state_.player.energy += 2;
            changeHeat(3);
            log("The jump spikes the reactor. Power surges through the ship.");
        } else {
            applyDamageToPlayer(5);
            drawCards(3);
            log("The jump tears plating free but hands you a fresh tactical line.");
        }
    }

    // Overload form is a high-risk burst mode: all outgoing damage is doubled.
    if (outgoingDamage > 0 && state_.player.form == kPlayerFormOverload) {
        outgoingDamage *= 2;
    }

    if (outgoingDamage > 0 && playerOverheatStage() >= 2) {
        outgoingDamage = (outgoingDamage * 3) / 4;
    }

    if (outgoingDamage > 0) {
        applyDamageToEnemy(outgoingDamage, card.ignoreShield, multiHitValue);
        if (baseCardId == "cl_04" && state_.enemy.hp == 0) {
            state_.player.killAttackBonusThisBattle += 1;
            log("Battlefield Breakdown grants +1 permanent attack bonus this battle.");
        }
        startAnimation("attack", card.name, outgoingDamage, true, 7);
        if (multiHitValue > 1) {
            log(card.name + " deals " + std::to_string(outgoingDamage) + " x " +
                std::to_string(multiHitValue) + ".");
        } else {
            log(card.name + " deals " + std::to_string(outgoingDamage) + ".");
        }
    } else if (shieldValue > 0) {
        startAnimation("shield", card.name, shieldValue, true, 5);
    } else {
        startAnimation("support", card.name, std::max({card.heal, std::abs(card.heat), std::abs(enemyHeatDelta)}), true, 5);
    }
    if (extraTwoDamageHits > 0 && state_.enemy.hp > 0) {
        applyDamageToEnemy(2, false, extraTwoDamageHits);
        log(card.name + " adds " + std::to_string(extraTwoDamageHits) + " extra echo hit(s) for 2 damage.");
    }

    if (card.type == "Attack" || outgoingDamage > 0) {
        ++state_.player.attacksPlayedThisTurn;
    }

    if (card.blackboxProtocol) {
        bool hasSelectableTarget = false;
        for (const std::string& ref : state_.player.hand) {
            if (!isTemporaryCopyRef(ref)) {
                hasSelectableTarget = true;
                break;
            }
        }
        if (!hasSelectableTarget) {
            state_.statusLine = "Blackbox Protocol found no valid hand card to imprint.";
            log("Blackbox Protocol found no valid hand card to imprint.");
            return;
        }
        state_.player.blackboxSelectionPending = true;
        state_.player.pendingBlackboxCopyCount = std::max(1, card.blackboxCopyCount);
        state_.statusLine = "Blackbox Protocol armed. Select a hand card to exile as the imprint.";
        log("Blackbox Protocol awaits imprint target selection.");
        return;
    }

    const bool summoned_aux_carries_own_end_turn_aura = baseCardId == "df_27" || baseCardId == "dfh_08";
    summonAuxiliaryShips(card, baseCardId);
    if (card.auxImmediateTriggerTimes > 0) {
        triggerAuxiliaryShipsNow(card.auxImmediateTriggerTimes);
    }
    if (card.auxEndTurnExtraTriggerTimes > 0 && !summoned_aux_carries_own_end_turn_aura) {
        state_.player.auxEndTurnExtraTriggersThisTurn += card.auxEndTurnExtraTriggerTimes;
        log("Auxiliary ships will trigger extra times at end of turn.");
    }
    if (card.auxEffectPercentBonus > 0) {
        state_.player.auxEffectPercentBonusThisTurn += card.auxEffectPercentBonus;
        log("Auxiliary effect output is amplified this turn.");
    }
    if (card.auxSlotBonus > 0) {
        log("Auxiliary bay remains capped at " + std::to_string(kMaxAuxiliaryShips) + " slot(s).");
    }
    if (card.auxDestroyCount > 0) {
        resolveAuxiliaryShipDestroyRewards(card);
    }
    if (card.firstSummonExtraCopies > 0 && card.firstSummonExtraScope > 0) {
        state_.player.firstSummonExtraCopies = std::max(state_.player.firstSummonExtraCopies, card.firstSummonExtraCopies);
        state_.player.firstSummonExtraScope = std::max(
            state_.player.firstSummonExtraScope,
            std::min(kFirstSummonScopePerTurn, card.firstSummonExtraScope));
        log("Auxiliary launch protocol is now enhanced.");
    }
    if (card.summonCardPlayedDrawThisTurn > 0) {
        state_.player.summonCardPlayedDrawThisTurnBuff += card.summonCardPlayedDrawThisTurn;
        log("Summon-chain draw protocol is armed for this turn.");
    }
    if (card.firstEnterHighHeatEnergyThisBattle > 0) {
        state_.player.firstEnterHighHeatEnergyThisBattle = std::max(
            state_.player.firstEnterHighHeatEnergyThisBattle, card.firstEnterHighHeatEnergyThisBattle);
    }
    if (card.firstEnterHighHeatDrawThisBattle > 0) {
        state_.player.firstEnterHighHeatDrawThisBattle = std::max(
            state_.player.firstEnterHighHeatDrawThisBattle, card.firstEnterHighHeatDrawThisBattle);
    }
    if (card.eachEnterHighHeatDrawThisBattle > 0) {
        state_.player.eachEnterHighHeatDrawThisBattle = std::max(
            state_.player.eachEnterHighHeatDrawThisBattle, card.eachEnterHighHeatDrawThisBattle);
    }
    if (card.firstSummonEnemyHeatPerTurnThisBattle > 0) {
        state_.player.firstSummonEnemyHeatPerTurnThisBattle = std::max(
            state_.player.firstSummonEnemyHeatPerTurnThisBattle, card.firstSummonEnemyHeatPerTurnThisBattle);
    }
    if (card.firstSummonEnemyHeatAttackBonusPerTurnThisBattle > 0) {
        state_.player.firstSummonEnemyHeatAttackBonusPerTurnThisBattle = std::max(
            state_.player.firstSummonEnemyHeatAttackBonusPerTurnThisBattle,
            card.firstSummonEnemyHeatAttackBonusPerTurnThisBattle);
    }
    if (card.enterOverloadEnemyHeatThisBattle > 0) {
        state_.player.enterOverloadEnemyHeatThisBattle = std::max(
            state_.player.enterOverloadEnemyHeatThisBattle, card.enterOverloadEnemyHeatThisBattle);
    }
    if (card.leaveDefenseEnemyHeatThisBattle > 0) {
        state_.player.leaveDefenseEnemyHeatThisBattle = std::max(
            state_.player.leaveDefenseEnemyHeatThisBattle, card.leaveDefenseEnemyHeatThisBattle);
    }
    if (card.overloadIncomingDamageReducePercent > 0) {
        state_.player.overloadIncomingDamageReducePercentThisTurn =
            std::max(state_.player.overloadIncomingDamageReducePercentThisTurn,
                     card.overloadIncomingDamageReducePercent);
        log("Overload instability is damped for this turn.");
    }
    buffAuxiliaryShips(card);
    if (card.formToggle) {
        togglePlayerForm();
    }
    shiftPlayerForm(card);
    if (playedSummonCard) {
        if (state_.player.summonCardPlayedDrawThisTurnBuff > 0) {
            drawCards(state_.player.summonCardPlayedDrawThisTurnBuff);
            log("Summon-chain protocol draws " + std::to_string(state_.player.summonCardPlayedDrawThisTurnBuff) + " card(s).");
        }
        if (!state_.player.firstSummonEnemyHeatTriggeredThisTurn &&
            state_.player.firstSummonEnemyHeatPerTurnThisBattle > 0) {
            int enemyHeatGain = state_.player.firstSummonEnemyHeatPerTurnThisBattle;
            if (IsAttackOrHybridSummon(card)) {
                enemyHeatGain += state_.player.firstSummonEnemyHeatAttackBonusPerTurnThisBattle;
            }
            changeEnemyHeat(enemyHeatGain);
            state_.player.firstSummonEnemyHeatTriggeredThisTurn = true;
            log("First summon this turn adds enemy heat by " + std::to_string(enemyHeatGain) + ".");
        }
    }
    if (baseCardId == "df_15") {
        beginAuxiliaryTransformSelection(card, 1 + cardUpgradeLevel(resolvedRef));
        return;
    }

    if (state_.enemy.hp <= 0) {
        handleBattleVictory();
    } else if (state_.player.hp <= 0) {
        handleDefeat();
    } else if (baseCardId == "cl_08") {
        state_.statusLine = card.name + " bends the next turn, then ends your turn.";
        endTurn();
        return;
    } else {
        state_.statusLine = card.name + " resolved. " + std::to_string(state_.player.energy) + " energy remains.";
    }
}

void GameEngine::endTurn() {
    if (state_.page != AppPage::Battle) {
        return;
    }
    if (state_.player.blackboxSelectionPending) {
        state_.statusLine = "Choose a Blackbox imprint target before ending the turn.";
        return;
    }
    if (state_.player.auxTransformSelectionPending) {
        state_.statusLine = state_.player.auxTransformTargetChoicePending
            ? "Choose a target drone type before ending the turn."
            : "Choose an auxiliary ship before ending the turn.";
        return;
    }

    for (const std::string& card : state_.player.hand) {
        if (isTemporaryCopyRef(card)) {
            continue;
        }
        state_.player.discardPile.push_back(unwrapTemporaryCopyRef(card));
    }
    state_.player.hand.clear();
    log("Your remaining hand is discarded.");

    if (hasRelic("auto_cooler") && state_.player.heat > 0) {
        changeHeat(-1);
        log("Auto-Cooler bleeds 1 heat from the reactor.");
    }

    resolveAuxiliaryShipsEndTurn();
    if (state_.enemy.hp <= 0) {
        handleBattleVictory();
        return;
    }

    resolveEnemyIntent();
    if (state_.player.hp <= 0) {
        handleDefeat();
        return;
    }
    if (state_.enemy.hp <= 0) {
        handleBattleVictory();
        return;
    }

    chooseEnemyIntent();
    startPlayerTurn();
    if (state_.player.hp <= 0 || state_.page != AppPage::Battle) {
        return;
    }
    state_.statusLine = "Enemy cycle resolved. Your turn begins.";
}

void GameEngine::startBattle(const std::string& nodeType, const std::string& forcedEnemyId) {
    const EnemyTemplate& enemy = pickEnemy(nodeType, forcedEnemyId);
    state_.enemy.enemyId = enemy.id;
    state_.enemy.name = enemy.name;
    state_.enemy.behavior = enemy.behavior;
    int enemyHp = enemy.baseHp + (state_.sector - 1) * (enemy.boss ? 8 : (enemy.elite ? 5 : 3));
    if (state_.difficulty <= 0) {
        enemyHp = std::max(1, (enemyHp * 90 + 99) / 100);
    } else if (state_.difficulty >= 2) {
        enemyHp = std::max(1, (enemyHp * 115 + 99) / 100);
    }
    state_.enemy.maxHp = enemyHp;
    state_.enemy.hp = state_.enemy.maxHp;
    state_.enemy.shield = 0;
    state_.enemy.heat = 0;
    state_.enemy.maxHeat = enemy.maxHeat;
    state_.enemy.overheatRounds = 0;
    state_.enemy.coolingCooldownTurns = 0;
    state_.enemy.patternIndex = 0;
    state_.enemy.weakenTurns = 0;
    state_.enemy.shieldLockTurns = 0;
    state_.battleLog.clear();
    state_.battleTurn = 0;
    state_.firstTurnOfBattle = true;
    state_.player.overheatRounds = 0;
    state_.player.nextAttackMultiplier = 1;
    state_.player.nextIncomingDamageFlatReductionThisTurn = 0;
    state_.player.nextAttackPreMultiplierBonusPercentThisTurn = 0;
    state_.player.salvageBonus = 0;
    state_.player.attacksPlayedThisTurn = 0;
    state_.player.form = kPlayerFormNeutral;
    state_.player.auxEndTurnExtraTriggersThisTurn = 0;
    state_.player.auxEffectPercentBonusThisTurn = 0;
    state_.player.auxSlotBonusThisBattle = 0;
    state_.player.overloadIncomingDamageReducePercentThisTurn = 0;
    state_.player.firstSummonExtraCopies = 0;
    state_.player.firstSummonExtraScope = 0;
    state_.player.summonCardPlayedDrawThisTurnBuff = 0;
    state_.player.formChangesThisTurn = 0;
    state_.player.firstEnterHighHeatEnergyThisBattle = 0;
    state_.player.firstEnterHighHeatDrawThisBattle = 0;
    state_.player.eachEnterHighHeatDrawThisBattle = 0;
    state_.player.firstSummonEnemyHeatPerTurnThisBattle = 0;
    state_.player.firstSummonEnemyHeatAttackBonusPerTurnThisBattle = 0;
    state_.player.enterOverloadEnemyHeatThisBattle = 0;
    state_.player.leaveDefenseEnemyHeatThisBattle = 0;
    state_.player.heatGainedThisTurn = 0;
    state_.player.heatReducedThisTurn = 0;
    state_.player.enemyHeatRaisedThisTurn = 0;
    state_.player.nextHeatGainReductionThisTurn = 0;
    state_.player.nextAttackEnemyHeatBonusThisTurn = 0;
    state_.player.attackBonusPerHeatThisTurn = 0;
    state_.player.attackBonusPerHeatCapThisTurn = 0;
    state_.player.attackBonusFromHeatGainThisBattle = 0;
    state_.player.attackBonusPerHeatGainPointThisBattle = 0;
    state_.player.killAttackBonusThisBattle = 0;
    state_.player.auxTriggerEnemyHeatBonusThisTurn = 0;
    state_.player.formChangeDrawThisTurn = 0;
    state_.player.overheatSelfDamageBlockThisTurn = 0;
    state_.player.leaveDefenseShieldBonusThisTurn = 0;
    state_.player.auxOnRemoveDrawThisBattle = 0;
    state_.player.auxOnRemoveEnergyThisBattle = 0;
    state_.player.enterDefenseShieldThisBattle = 0;
    state_.player.exitDefenseEnergyThisBattle = 0;
    state_.player.firstEnterOverloadDrawThisBattle = 0;
    state_.player.firstExitDefenseEnergyThisBattle = 0;
    state_.player.nextTurnDraw = 0;
    state_.player.blackboxCopyCount = 0;
    state_.player.firstCardCostZeroPendingNextTurn = false;
    state_.player.firstCardCostZeroThisTurn = false;
    state_.player.firstSummonUsedThisBattle = false;
    state_.player.firstSummonUsedThisTurn = false;
    state_.player.firstEnterHighHeatTriggeredThisBattle = false;
    state_.player.firstEnterOverloadDrawTriggeredThisBattle = false;
    state_.player.firstExitDefenseEnergyTriggeredThisBattle = false;
    state_.player.firstSummonEnemyHeatTriggeredThisTurn = false;
    state_.player.firstFormChangeRelicTriggeredThisBattle = false;
    state_.player.enteredOverloadThisTurn = false;
    state_.player.exitedDefenseThisTurn = false;
    state_.player.exitedFormThisTurn = false;
    state_.player.blackboxSelectionPending = false;
    state_.player.pendingBlackboxCopyCount = 0;
    state_.player.blackboxImprintRef.clear();
    clearAuxiliaryTransformSelection();
    state_.player.exilePile.clear();
    state_.player.activeAbilities.clear();
    state_.auxiliaryShips.clear();
    state_.page = AppPage::Battle;
    resumePage_ = AppPage::Battle;
    resetBattleDeck();
    if (state_.player.nextBattleHeal > 0) {
        state_.player.hp = std::min(state_.player.maxHp, state_.player.hp + state_.player.nextBattleHeal);
        log("Stored emergency recovery restores " + std::to_string(state_.player.nextBattleHeal) + " HP.");
        state_.player.nextBattleHeal = 0;
    }
    if (state_.player.nextBattleFirstAttackMultiplier > 1) {
        state_.player.nextAttackMultiplier = std::max(state_.player.nextAttackMultiplier, state_.player.nextBattleFirstAttackMultiplier);
        log("Stored targeting surge primes your first attack.");
        state_.player.nextBattleFirstAttackMultiplier = 1;
    }
    syncPlayerOverheatState();
    chooseEnemyIntent();
    startPlayerTurn();
    log("Hostile contact acquired: " + state_.enemy.name + ".");
}

void GameEngine::resetBattleDeck() {
    for (const std::string& card : state_.player.hand) {
        state_.player.drawPile.push_back(card);
    }
    for (const std::string& card : state_.player.discardPile) {
        state_.player.drawPile.push_back(card);
    }
    state_.player.hand.clear();
    state_.player.discardPile.clear();
    ShuffleVector(state_.player.drawPile, rng_);
    state_.player.shield = 0;
}

void GameEngine::startPlayerTurn() {
    if (state_.battleTurn > 0 && state_.player.shield > 0) {
        log("Your temporary shield field dissipates.");
    }
    state_.player.shield = 0;

    if (state_.battleTurn > 0) {
        advancePlayerOverheatRound();
        advanceEnemyOverheatRound();
    }

    if (playerOverheatStage() >= 3) {
        state_.player.hp -= 5;
        if (state_.player.hp <= 0 && hasRelic("emergency_battery") && !state_.player.emergencyBatteryUsed) {
            state_.player.hp = 1;
            state_.player.emergencyBatteryUsed = true;
            log("Emergency Battery triggers and keeps the ship alive at 1 HP.");
        }
        state_.player.hp = std::max(0, state_.player.hp);
        log("Player overheat burns 5 HP.");
        if (state_.player.hp == 0) {
            handleDefeat();
            return;
        }
    }

    const int drawTarget = playerOverheatStage() >= 1 ? (kBattleDrawCount - 1) : kBattleDrawCount;
    int energy = state_.player.maxEnergy + state_.player.nextTurnEnergy;
    if (state_.firstTurnOfBattle && hasRelic("reactor_core")) {
        energy += 1;
    }
    if (hasRelic("origin")) {
        energy += 2;
    }
    if (state_.firstTurnOfBattle && state_.player.openingEnergyBonus > 0) {
        energy += state_.player.openingEnergyBonus;
        log("Stored anomaly charge grants +" + std::to_string(state_.player.openingEnergyBonus) + " energy.");
        state_.player.openingEnergyBonus = 0;
    }
    state_.player.energy = std::max(0, energy);
    state_.player.nextTurnEnergy = 0;
    const int extraTurnDraw = std::max(0, state_.player.nextTurnDraw);
    state_.player.nextTurnDraw = 0;
    state_.player.firstCardCostZeroThisTurn = state_.player.firstCardCostZeroPendingNextTurn;
    state_.player.firstCardCostZeroPendingNextTurn = false;
    state_.player.attacksPlayedThisTurn = 0;
    state_.player.auxEndTurnExtraTriggersThisTurn = 0;
    state_.player.auxEffectPercentBonusThisTurn = 0;
    state_.player.auxSlotBonusThisBattle = 0;
    state_.player.overloadIncomingDamageReducePercentThisTurn = 0;
    state_.player.nextIncomingDamageFlatReductionThisTurn = 0;
    state_.player.nextAttackPreMultiplierBonusPercentThisTurn = 0;
    state_.player.firstSummonUsedThisTurn = false;
    state_.player.summonCardPlayedDrawThisTurnBuff = 0;
    state_.player.formChangesThisTurn = 0;
    state_.player.firstSummonEnemyHeatTriggeredThisTurn = false;
    state_.player.heatGainedThisTurn = 0;
    state_.player.heatReducedThisTurn = 0;
    state_.player.enemyHeatRaisedThisTurn = 0;
    state_.player.nextHeatGainReductionThisTurn = 0;
    state_.player.nextAttackEnemyHeatBonusThisTurn = 0;
    state_.player.attackBonusPerHeatThisTurn = 0;
    state_.player.attackBonusPerHeatCapThisTurn = 0;
    state_.player.auxTriggerEnemyHeatBonusThisTurn = 0;
    state_.player.formChangeDrawThisTurn = 0;
    state_.player.overheatSelfDamageBlockThisTurn = 0;
    state_.player.leaveDefenseShieldBonusThisTurn = 0;
    state_.player.enteredOverloadThisTurn = false;
    state_.player.exitedDefenseThisTurn = false;
    state_.player.exitedFormThisTurn = false;
    state_.player.blackboxSelectionPending = false;
    state_.player.pendingBlackboxCopyCount = 0;
    clearAuxiliaryTransformSelection();
    addBlackboxTurnCopies();
    ++state_.battleTurn;

    const int startShield = (hasRelic("shield_matrix") && state_.battleTurn == 1 ? 4 : 0) +
                            state_.player.openingShieldBonus +
                            (state_.battleTurn == 1 ? g_sa_permanent_shield : 0);
    if (startShield > 0) {
        state_.player.shield += startShield;
        log("Deflector envelope stabilizes for +" + std::to_string(startShield) + " shield.");
        state_.player.openingShieldBonus = 0;
    }

    drawCards(drawTarget);
    if (extraTurnDraw > 0) {
        drawCards(extraTurnDraw);
        log("Stored draw protocol draws " + std::to_string(extraTurnDraw) + " extra card(s).");
    }
    if (state_.player.firstCardCostZeroThisTurn) {
        log("Reverse-Entropy Jump makes your first card this turn cost 0.");
    }
    state_.firstTurnOfBattle = false;
    log("Turn " + std::to_string(state_.battleTurn) + " begins with " + std::to_string(state_.player.energy) +
        " energy and " + std::to_string(static_cast<int>(state_.player.hand.size())) + " cards in hand.");
}

void GameEngine::chooseEnemyIntent() {
    const EnemyTemplate* enemy = enemyTemplateById(state_.enemy.enemyId);
    if (enemy == nullptr || enemy->pattern.empty()) {
        state_.enemy.intentKind = "attack";
        state_.enemy.intentLabel = "Attack";
        state_.enemy.intentValue = 6;
        return;
    }

    if (enemyShouldUseCooling()) {
        state_.enemy.intentKind = "cool";
        state_.enemy.intentLabel = "Emergency Cooling";
        state_.enemy.intentValue = enemyCoolingAmount();
        return;
    }

    int index = state_.enemy.patternIndex % static_cast<int>(enemy->pattern.size());
    if (enemy->behavior == "chaotic") {
        std::uniform_int_distribution<int> randomIndex(0, static_cast<int>(enemy->pattern.size()) - 1);
        index = randomIndex(rng_);
    }

    const IntentDef& intent = enemy->pattern[static_cast<size_t>(index)];
    state_.enemy.intentKind = intent.kind;
    state_.enemy.intentLabel = intent.label;
    state_.enemy.intentValue = intent.value;
}

void GameEngine::resolveEnemyIntent() {
    if (state_.enemy.shield > 0) {
        log(state_.enemy.name + "'s temporary shield collapses.");
        state_.enemy.shield = 0;
    }

    if (enemyOverheatStage() >= 3) {
        state_.enemy.hp = std::max(0, state_.enemy.hp - 5);
        log("Enemy overheat burns 5 HP.");
        if (state_.enemy.hp == 0) {
            return;
        }
    }

    log(state_.enemy.name + " executes " + state_.enemy.intentLabel + ".");

    if (state_.enemy.intentKind == "attack") {
        const int damage = effectiveEnemyAttackDamage();
        if (state_.enemy.weakenTurns > 0 && state_.enemy.intentValue > 0) {
            --state_.enemy.weakenTurns;
        }
        applyDamageToPlayer(damage);
        startAnimation("enemy_attack", state_.enemy.intentLabel, damage, false, 7);
        log("You absorb " + std::to_string(damage) + " damage.");
    } else if (state_.enemy.intentKind == "shield") {
        if (state_.enemy.shieldLockTurns > 0) {
            log("Shield jam active. Enemy plating fails to form.");
        } else {
            state_.enemy.shield += state_.enemy.intentValue;
            startAnimation("shield", state_.enemy.intentLabel, state_.enemy.intentValue, false, 5);
            log(state_.enemy.name + " gains " + std::to_string(state_.enemy.intentValue) + " shield.");
        }
    } else if (state_.enemy.intentKind == "heat") {
        changeHeat(state_.enemy.intentValue);
        startAnimation("heat", state_.enemy.intentLabel, state_.enemy.intentValue, false, 6);
        log("Your reactor spikes by " + std::to_string(state_.enemy.intentValue) + " heat.");
    } else if (state_.enemy.intentKind == "emp") {
        state_.player.nextTurnEnergy -= state_.enemy.intentValue;
        startAnimation("support", state_.enemy.intentLabel, state_.enemy.intentValue, false, 5);
        log("The EMP will cut " + std::to_string(state_.enemy.intentValue) + " energy from your next turn.");
    } else if (state_.enemy.intentKind == "charge") {
        state_.enemy.shield += state_.enemy.intentValue;
        startAnimation("support", state_.enemy.intentLabel, state_.enemy.intentValue, false, 5);
        log(state_.enemy.name + " charges for a heavier strike.");
    } else if (state_.enemy.intentKind == "cool") {
        triggerEnemyCooling();
    }

    if (state_.enemy.intentKind != "cool" && state_.enemy.coolingCooldownTurns > 0) {
        --state_.enemy.coolingCooldownTurns;
    }
    if (state_.enemy.shieldLockTurns > 0) {
        --state_.enemy.shieldLockTurns;
    }
    if (state_.enemy.intentKind != "cool") {
        ++state_.enemy.patternIndex;
    }
}

void GameEngine::handleBattleVictory() {
    state_.animation.active = false;
    state_.reward = {};
    state_.reward.banner = (state_.activeNodeType == "elite")
                               ? "Elite salvage secured"
                               : (state_.activeNodeType == "boss" ? "Boss cache recovered" : "Battle reward");

    int scrapReward = 12 + state_.sector * 4;
    if (state_.activeNodeType == "elite") {
        scrapReward += 10;
    }
    if (state_.activeNodeType == "boss") {
        scrapReward += 16;
    }
    if (hasRelic("salvage_scanner")) {
        scrapReward += 6;
    }
    scrapReward += state_.player.salvageBonus;
    state_.player.scrap += scrapReward;
    state_.reward.scrap = scrapReward;
    state_.reward.cardIds = randomBattleCardChoices(3, state_.activeNodeType);

    if (state_.activeNodeType == "elite" || state_.activeNodeType == "boss") {
        const std::string relicId = randomRelic();
        state_.reward.relicId = relicId;
        if (!relicId.empty()) {
            grantRelic(relicId);
        }
    }

    state_.player.shield = 0;
    state_.player.energy = state_.player.maxEnergy;
    state_.player.nextTurnEnergy = 0;
    state_.player.overheatRounds = 0;
    state_.player.form = kPlayerFormNeutral;
    state_.player.nextIncomingDamageFlatReductionThisTurn = 0;
    state_.player.nextAttackPreMultiplierBonusPercentThisTurn = 0;
    state_.player.auxEndTurnExtraTriggersThisTurn = 0;
    state_.player.auxEffectPercentBonusThisTurn = 0;
    state_.player.auxSlotBonusThisBattle = 0;
    state_.player.overloadIncomingDamageReducePercentThisTurn = 0;
    state_.player.firstSummonExtraCopies = 0;
    state_.player.firstSummonExtraScope = 0;
    state_.player.summonCardPlayedDrawThisTurnBuff = 0;
    state_.player.formChangesThisTurn = 0;
    state_.player.firstEnterHighHeatEnergyThisBattle = 0;
    state_.player.firstEnterHighHeatDrawThisBattle = 0;
    state_.player.eachEnterHighHeatDrawThisBattle = 0;
    state_.player.firstSummonEnemyHeatPerTurnThisBattle = 0;
    state_.player.firstSummonEnemyHeatAttackBonusPerTurnThisBattle = 0;
    state_.player.enterOverloadEnemyHeatThisBattle = 0;
    state_.player.leaveDefenseEnemyHeatThisBattle = 0;
    state_.player.heatGainedThisTurn = 0;
    state_.player.heatReducedThisTurn = 0;
    state_.player.enemyHeatRaisedThisTurn = 0;
    state_.player.nextHeatGainReductionThisTurn = 0;
    state_.player.nextAttackEnemyHeatBonusThisTurn = 0;
    state_.player.attackBonusPerHeatThisTurn = 0;
    state_.player.attackBonusPerHeatCapThisTurn = 0;
    state_.player.attackBonusFromHeatGainThisBattle = 0;
    state_.player.attackBonusPerHeatGainPointThisBattle = 0;
    state_.player.killAttackBonusThisBattle = 0;
    state_.player.auxTriggerEnemyHeatBonusThisTurn = 0;
    state_.player.formChangeDrawThisTurn = 0;
    state_.player.overheatSelfDamageBlockThisTurn = 0;
    state_.player.leaveDefenseShieldBonusThisTurn = 0;
    state_.player.auxOnRemoveDrawThisBattle = 0;
    state_.player.auxOnRemoveEnergyThisBattle = 0;
    state_.player.firstCardCostZeroPendingNextTurn = false;
    state_.player.firstCardCostZeroThisTurn = false;
    state_.player.firstSummonUsedThisBattle = false;
    state_.player.firstSummonUsedThisTurn = false;
    state_.player.firstEnterHighHeatTriggeredThisBattle = false;
    state_.player.firstSummonEnemyHeatTriggeredThisTurn = false;
    state_.player.firstFormChangeRelicTriggeredThisBattle = false;
    state_.player.enteredOverloadThisTurn = false;
    state_.player.exitedDefenseThisTurn = false;
    state_.player.exitedFormThisTurn = false;
    state_.player.blackboxCopyCount = 0;
    state_.player.blackboxSelectionPending = false;
    state_.player.pendingBlackboxCopyCount = 0;
    state_.player.blackboxImprintRef.clear();
    clearAuxiliaryTransformSelection();
    for (const ActiveAbilityState& ability : state_.player.activeAbilities) {
        if (!ability.cardRef.empty()) {
            state_.player.drawPile.push_back(ability.cardRef);
        }
    }
    state_.player.activeAbilities.clear();
    state_.player.exilePile.clear();
    state_.enemy.heat = 0;
    state_.enemy.overheatRounds = 0;
    state_.enemy.coolingCooldownTurns = 0;
    state_.battleTurn = 0;
    state_.auxiliaryShips.clear();
    state_.pendingSectorAdvance = state_.activeNodeType == "boss" && state_.sector < 3;
    state_.pendingVictory = state_.activeNodeType == "boss" && state_.sector >= 3;
    state_.page = AppPage::Reward;
    resumePage_ = AppPage::Reward;
    state_.statusLine = "Enemy destroyed. Choose your salvage reward.";
}

void GameEngine::handleDefeat() {
    state_.player.overheatRounds = 0;
    state_.player.form = kPlayerFormNeutral;
    state_.player.nextIncomingDamageFlatReductionThisTurn = 0;
    state_.player.nextAttackPreMultiplierBonusPercentThisTurn = 0;
    state_.player.auxEndTurnExtraTriggersThisTurn = 0;
    state_.player.auxEffectPercentBonusThisTurn = 0;
    state_.player.auxSlotBonusThisBattle = 0;
    state_.player.overloadIncomingDamageReducePercentThisTurn = 0;
    state_.player.firstSummonExtraCopies = 0;
    state_.player.firstSummonExtraScope = 0;
    state_.player.summonCardPlayedDrawThisTurnBuff = 0;
    state_.player.formChangesThisTurn = 0;
    state_.player.firstEnterHighHeatEnergyThisBattle = 0;
    state_.player.firstEnterHighHeatDrawThisBattle = 0;
    state_.player.eachEnterHighHeatDrawThisBattle = 0;
    state_.player.firstSummonEnemyHeatPerTurnThisBattle = 0;
    state_.player.firstSummonEnemyHeatAttackBonusPerTurnThisBattle = 0;
    state_.player.enterOverloadEnemyHeatThisBattle = 0;
    state_.player.leaveDefenseEnemyHeatThisBattle = 0;
    state_.player.heatGainedThisTurn = 0;
    state_.player.heatReducedThisTurn = 0;
    state_.player.enemyHeatRaisedThisTurn = 0;
    state_.player.nextHeatGainReductionThisTurn = 0;
    state_.player.nextAttackEnemyHeatBonusThisTurn = 0;
    state_.player.attackBonusPerHeatThisTurn = 0;
    state_.player.attackBonusPerHeatCapThisTurn = 0;
    state_.player.attackBonusFromHeatGainThisBattle = 0;
    state_.player.attackBonusPerHeatGainPointThisBattle = 0;
    state_.player.killAttackBonusThisBattle = 0;
    state_.player.auxTriggerEnemyHeatBonusThisTurn = 0;
    state_.player.formChangeDrawThisTurn = 0;
    state_.player.overheatSelfDamageBlockThisTurn = 0;
    state_.player.leaveDefenseShieldBonusThisTurn = 0;
    state_.player.auxOnRemoveDrawThisBattle = 0;
    state_.player.auxOnRemoveEnergyThisBattle = 0;
    state_.player.firstCardCostZeroPendingNextTurn = false;
    state_.player.firstCardCostZeroThisTurn = false;
    state_.player.firstSummonUsedThisBattle = false;
    state_.player.firstSummonUsedThisTurn = false;
    state_.player.firstEnterHighHeatTriggeredThisBattle = false;
    state_.player.firstSummonEnemyHeatTriggeredThisTurn = false;
    state_.player.firstFormChangeRelicTriggeredThisBattle = false;
    state_.player.enteredOverloadThisTurn = false;
    state_.player.exitedDefenseThisTurn = false;
    state_.player.exitedFormThisTurn = false;
    state_.player.blackboxCopyCount = 0;
    state_.player.blackboxSelectionPending = false;
    state_.player.pendingBlackboxCopyCount = 0;
    state_.player.blackboxImprintRef.clear();
    clearAuxiliaryTransformSelection();
    for (const ActiveAbilityState& ability : state_.player.activeAbilities) {
        if (!ability.cardRef.empty()) {
            state_.player.drawPile.push_back(ability.cardRef);
        }
    }
    state_.player.activeAbilities.clear();
    state_.player.exilePile.clear();
    state_.enemy.heat = 0;
    state_.enemy.overheatRounds = 0;
    state_.enemy.coolingCooldownTurns = 0;
    state_.auxiliaryShips.clear();
    fillSummary(false);
}

void GameEngine::applyDamageToPlayer(int amount) {
    int remaining = amount;
    if (remaining > 0 && state_.player.nextIncomingDamageFlatReductionThisTurn > 0) {
        const int prevented = std::min(remaining, state_.player.nextIncomingDamageFlatReductionThisTurn);
        remaining -= prevented;
        state_.player.nextIncomingDamageFlatReductionThisTurn = 0;
        if (prevented > 0) {
            log("Superconduct Focus prevents " + std::to_string(prevented) + " incoming damage.");
        }
    }
    // Overload form doubles incoming damage, which defines the form's drawback.
    if (state_.player.form == kPlayerFormOverload && remaining > 0) {
        const int dampen = std::max(0, std::min(100, state_.player.overloadIncomingDamageReducePercentThisTurn));
        const int overload_multiplier = std::max(100, 200 - dampen);
        remaining = (remaining * overload_multiplier + 99) / 100;
    }
    if (playerOverheatStage() >= 2 && remaining > 0) {
        remaining = (remaining * 5 + 3) / 4;
    }
    if (hasRelic("origin") && remaining > 0) {
        remaining = (remaining * 3) / 4;
    }
    if (state_.player.shield > 0) {
        const int blocked = std::min(state_.player.shield, remaining);
        state_.player.shield -= blocked;
        remaining -= blocked;
    }
    state_.player.hp -= remaining;
    if (state_.player.hp <= 0 && hasRelic("emergency_battery") && !state_.player.emergencyBatteryUsed) {
        state_.player.hp = 1;
        state_.player.emergencyBatteryUsed = true;
        log("Emergency Battery triggers and keeps the ship alive at 1 HP.");
    }
    state_.player.hp = std::max(0, state_.player.hp);
}

void GameEngine::applyDamageToEnemy(int amount, bool ignoreShield, int hits) {
    for (int hit = 0; hit < hits; ++hit) {
        int remaining = amount;
        if (hasRelic("origin") && remaining > 0) {
            remaining = (remaining * 5 + 3) / 4;
        }
        if (enemyOverheatStage() >= 2 && remaining > 0) {
            remaining = (remaining * 5 + 3) / 4;
        }
        if (!ignoreShield && state_.enemy.shield > 0) {
            const int blocked = std::min(state_.enemy.shield, remaining);
            state_.enemy.shield -= blocked;
            remaining -= blocked;
        }
        state_.enemy.hp -= remaining;
        state_.enemy.hp = std::max(0, state_.enemy.hp);
        if (state_.enemy.hp == 0) {
            break;
        }
    }
}

void GameEngine::drawCards(int count) {
    for (int i = 0; i < count; ++i) {
        if (state_.player.drawPile.empty()) {
            reshuffleDiscardIntoDraw();
        }
        if (state_.player.drawPile.empty()) {
            return;
        }
        const std::string nextCard = state_.player.drawPile.back();
        state_.player.drawPile.pop_back();
        if (state_.player.hand.size() >= kBattleHandLimit) {
            state_.player.discardPile.push_back(nextCard);
            const std::string cardName = displayCardName(nextCard);
            log("Hand full. " + cardName + " is routed to discard.");
            continue;
        }
        state_.player.hand.push_back(nextCard);
    }
}

void GameEngine::reshuffleDiscardIntoDraw() {
    if (state_.player.discardPile.empty()) {
        return;
    }
    for (const std::string& card : state_.player.discardPile) {
        state_.player.drawPile.push_back(card);
    }
    state_.player.discardPile.clear();
    ShuffleVector(state_.player.drawPile, rng_);
    if (state_.page == AppPage::Battle) {
        log("Discard pile reshuffled into the draw pile.");
    }
}

void GameEngine::changeHeat(int delta) {
    const bool wasOverheated = playerIsOverheated();
    const int beforeHeat = state_.player.heat;
    state_.player.heat += delta;
    if (state_.player.heat < 0) {
        state_.player.heat = 0;
    }
    const int clampedWithoutOverflow = state_.player.heat;
    if (state_.page == AppPage::Battle) {
        const int raised = std::max(0, clampedWithoutOverflow - beforeHeat);
        const int reduced = std::max(0, beforeHeat - clampedWithoutOverflow);
        state_.player.heatGainedThisTurn += raised;
        state_.player.heatReducedThisTurn += reduced;
        if (raised > 0 && state_.player.attackBonusPerHeatGainPointThisBattle > 0) {
            state_.player.attackBonusFromHeatGainThisBattle +=
                raised * state_.player.attackBonusPerHeatGainPointThisBattle;
        }
    }
    if (state_.player.heat > state_.player.maxHeat) {
        const int overflow = state_.player.heat - state_.player.maxHeat;
        state_.player.heat = state_.player.maxHeat;
        int selfBurn = overflow + 1;
        if (state_.player.overheatSelfDamageBlockThisTurn > 0) {
            const int blocked = std::min(state_.player.overheatSelfDamageBlockThisTurn, selfBurn);
            selfBurn -= blocked;
            state_.player.overheatSelfDamageBlockThisTurn -= blocked;
        }
        if (selfBurn > 0) {
            applyDamageToPlayer(selfBurn);
            log("Overheat! Feedback burns through the hull for " + std::to_string(selfBurn) + ".");
        } else {
            log("Overheat feedback is fully blocked this turn.");
        }
    }
    if (state_.page == AppPage::Battle) {
        const bool nowOverheated = playerIsOverheated();
        if (!wasOverheated && nowOverheated) {
            // Entry-triggered heat effects are intentionally centralized here so
            // every heat source (card, enemy intent, relic) behaves consistently.
            if (!state_.player.firstEnterHighHeatTriggeredThisBattle &&
                (state_.player.firstEnterHighHeatEnergyThisBattle > 0 ||
                 state_.player.firstEnterHighHeatDrawThisBattle > 0)) {
                state_.player.firstEnterHighHeatTriggeredThisBattle = true;
                if (state_.player.firstEnterHighHeatEnergyThisBattle > 0) {
                    state_.player.energy += state_.player.firstEnterHighHeatEnergyThisBattle;
                }
                if (state_.player.firstEnterHighHeatDrawThisBattle > 0) {
                    drawCards(state_.player.firstEnterHighHeatDrawThisBattle);
                }
                log("First high-heat entry bonus is triggered.");
            } else if (state_.player.eachEnterHighHeatDrawThisBattle > 0) {
                drawCards(state_.player.eachEnterHighHeatDrawThisBattle);
                log("Recurring high-heat entry bonus draws cards.");
            }
        }
        syncPlayerOverheatState();
    } else {
        state_.player.overheatRounds = 0;
    }
}

bool GameEngine::playerIsOverheated() const {
    return state_.player.heat >= state_.player.maxHeat;
}

int GameEngine::playerOverheatStage() const {
    if (!playerIsOverheated()) {
        return 0;
    }
    return std::max(1, std::min(3, state_.player.overheatRounds));
}

void GameEngine::syncPlayerOverheatState() {
    if (state_.page != AppPage::Battle) {
        state_.player.overheatRounds = 0;
        return;
    }

    if (playerIsOverheated()) {
        if (state_.player.overheatRounds == 0) {
            state_.player.overheatRounds = 1;
            log("Player overheat begins. Draw drops by 1.");
        } else {
            state_.player.overheatRounds = std::min(3, state_.player.overheatRounds);
        }
        return;
    }

    if (state_.player.overheatRounds != 0) {
        state_.player.overheatRounds = 0;
        log("Player overheat clears.");
    }
}

void GameEngine::advancePlayerOverheatRound() {
    if (!playerIsOverheated()) {
        state_.player.overheatRounds = 0;
        return;
    }

    if (state_.player.overheatRounds <= 0) {
        state_.player.overheatRounds = 1;
        return;
    }

    const int previous = state_.player.overheatRounds;
    state_.player.overheatRounds = std::min(3, state_.player.overheatRounds + 1);
    if (state_.player.overheatRounds == previous) {
        return;
    }

    if (state_.player.overheatRounds == 2) {
        log("Player overheat deepens. Outgoing damage drops by 25%, incoming damage rises by 25%.");
    } else if (state_.player.overheatRounds == 3) {
        log("Player overheat peaks. It will burn for 5 HP each turn.");
    }
}

void GameEngine::summonAuxiliaryShips(const CardDef& card, const std::string& baseCardId) {
    if (card.summonDroneCount <= 0) {
        return;
    }

    int extraCopies = 0;
    if (state_.player.firstSummonExtraCopies > 0) {
        const bool battle_once_ready =
            state_.player.firstSummonExtraScope == kFirstSummonScopeBattleOnce &&
            !state_.player.firstSummonUsedThisBattle;
        const bool per_turn_ready =
            state_.player.firstSummonExtraScope == kFirstSummonScopePerTurn &&
            !state_.player.firstSummonUsedThisTurn;
        if (battle_once_ready || per_turn_ready) {
            extraCopies = state_.player.firstSummonExtraCopies;
            state_.player.firstSummonUsedThisBattle = true;
            state_.player.firstSummonUsedThisTurn = true;
            log("Auxiliary command protocol duplicates this launch.");
        }
    }

    std::vector<AuxiliaryUnitRecipe> recipes;
    if (baseCardId == "df_16" || baseCardId == "df_28") {
        const auto basic_unit_ids = BasicAuxiliaryUnitIds();
        std::vector<std::string> unit_ids(basic_unit_ids.begin(), basic_unit_ids.end());
        ShuffleVector(unit_ids, rng_);
        const bool upgraded_tier = card.summonDroneAttack >= 4 || card.summonDroneShield >= 4;
        const int total = std::min<int>(static_cast<int>(unit_ids.size()), card.summonDroneCount + extraCopies);
        for (int i = 0; i < total; ++i) {
            recipes.push_back(BasicAuxiliaryRecipeForId(unit_ids[static_cast<size_t>(i)], upgraded_tier));
        }
    } else {
        const AuxiliaryUnitRecipe recipe = AuxiliaryRecipeForCard(baseCardId, card);
        for (int i = 0; i < card.summonDroneCount + extraCopies; ++i) {
            recipes.push_back(recipe);
        }
    }

    // Battle-long bay upgrades raise this cap before replacement is checked.
    const int max_auxiliary_ships = MaxAuxiliaryShipSlots(state_.player);
    for (const AuxiliaryUnitRecipe& recipe : recipes) {
        if (state_.auxiliaryShips.size() >= static_cast<size_t>(max_auxiliary_ships)) {
            log("Auxiliary bay is full. Oldest support craft is dismissed.");
            onAuxiliaryShipRemoved(state_.auxiliaryShips.front(), "replaced");
            state_.auxiliaryShips.erase(state_.auxiliaryShips.begin());
        }

        const int remaining_turns = card.summonDroneDuration > 0 ? card.summonDroneDuration : -1;
        AuxiliaryShipState drone = MakeAuxiliaryShipState(recipe, remaining_turns);
        state_.auxiliaryShips.push_back(drone);
        log(drone.displayName + " deployed.");
    }
}

void GameEngine::resolveAuxiliaryShipsEndTurn() {
    if (state_.auxiliaryShips.empty()) {
        return;
    }

    const int triggerTimes = std::max(1, 1 + state_.player.auxEndTurnExtraTriggersThisTurn);
    const size_t ship_count = state_.auxiliaryShips.size();
    for (size_t ship_index = 0; ship_index < ship_count; ++ship_index) {
        for (int trigger = 0; trigger < triggerTimes; ++trigger) {
            resolveAuxiliaryShipTrigger(ship_index, true, false);
        }
        if (ship_index < state_.auxiliaryShips.size() &&
            state_.auxiliaryShips[ship_index].remainingTurns > 0) {
            --state_.auxiliaryShips[ship_index].remainingTurns;
        }
    }

    clearDestroyedAuxiliaryShips();
}

void GameEngine::resolveAuxiliaryShipTrigger(size_t shipIndex, bool allowAura, bool immediateContext) {
    if (shipIndex >= state_.auxiliaryShips.size()) {
        return;
    }

    const AuxiliaryShipState ship = state_.auxiliaryShips[shipIndex];
    const int bonusPercent = std::max(0, state_.player.auxEffectPercentBonusThisTurn);
    const int attack = (ship.attack * (100 + bonusPercent) + 99) / 100;
    const int shield = (ship.shield * (100 + bonusPercent) + 99) / 100;
    const int heal = (ship.heal * (100 + bonusPercent) + 99) / 100;
    const int bonusDamage = (ship.endTurnDamage * (100 + bonusPercent) + 99) / 100;
    const int bonusHeat = (ship.endTurnEnemyHeat * (100 + bonusPercent) + 99) / 100;
    bool triggered = false;

    if (attack > 0) {
        applyDamageToEnemy(attack, false, 1);
        log(ship.displayName + (immediateContext ? " executes an immediate attack for " : " fires for ") +
            std::to_string(attack) + ".");
        triggered = true;
    }
    if (shield > 0) {
        state_.player.shield += shield;
        log(ship.displayName + (immediateContext ? " grants immediate shield +" : " reinforces shield by ") +
            std::to_string(shield) + ".");
        triggered = true;
    }
    if (heal > 0) {
        const int before = state_.player.hp;
        state_.player.hp = std::min(state_.player.maxHp, state_.player.hp + heal);
        log(ship.displayName + (immediateContext ? " performs immediate hull repair +" : " repairs hull for ") +
            std::to_string(state_.player.hp - before) + " HP.");
        triggered = true;
    }
    if (bonusDamage > 0) {
        applyDamageToEnemy(bonusDamage, false, 1);
        log(ship.displayName +
            (immediateContext ? " unleashes an immediate special strike for " : " unleashes a special strike for ") +
            std::to_string(bonusDamage) + ".");
        triggered = true;
    }
    if (bonusHeat > 0) {
        changeEnemyHeat(bonusHeat);
        log(ship.displayName + (immediateContext ? " spikes immediate enemy heat by " : " spikes enemy heat by ") +
            std::to_string(bonusHeat) + ".");
        triggered = true;
    }
    if (triggered && state_.player.auxTriggerEnemyHeatBonusThisTurn > 0) {
        changeEnemyHeat(state_.player.auxTriggerEnemyHeatBonusThisTurn);
    }
    if (allowAura && ship.endTurnExtraOtherTriggers > 0) {
        for (int aura = 0; aura < ship.endTurnExtraOtherTriggers; ++aura) {
            for (size_t otherIndex = 0; otherIndex < state_.auxiliaryShips.size(); ++otherIndex) {
                if (otherIndex == shipIndex) {
                    continue;
                }
                resolveAuxiliaryShipTrigger(otherIndex, false, immediateContext);
            }
        }
    }
}

void GameEngine::triggerAuxiliaryShipsNow(int times) {
    if (times <= 0 || state_.auxiliaryShips.empty()) {
        return;
    }
    for (int t = 0; t < times; ++t) {
        const size_t shipCount = state_.auxiliaryShips.size();
        for (size_t shipIndex = 0; shipIndex < shipCount; ++shipIndex) {
            resolveAuxiliaryShipTrigger(shipIndex, true, true);
        }
    }
}

void GameEngine::resolveAuxiliaryShipDestroyRewards(const CardDef& card) {
    if (card.auxDestroyCount <= 0) {
        return;
    }
    if (state_.auxiliaryShips.empty()) {
        log("No auxiliary ship is available to dismantle.");
        state_.statusLine = "No auxiliary ship is available to dismantle.";
        return;
    }

    const int multiplier = std::max(0, card.auxDestroyRewardMultiplierPercent);
    int destroyed = 0;
    int totalDamageReward = 0;
    int totalShieldReward = 0;
    int totalHealReward = 0;
    int totalEnemyHeatReward = 0;
    int totalExtraTriggers = 0;
    while (destroyed < card.auxDestroyCount && !state_.auxiliaryShips.empty()) {
        AuxiliaryShipState ship = state_.auxiliaryShips.front();
        state_.auxiliaryShips.erase(state_.auxiliaryShips.begin());
        onAuxiliaryShipRemoved(ship, "dismantled");

        // Lossless Recycling pays out the full trigger package of the removed
        // support craft, including special end-turn payloads on rare drones.
        const int damageReward = ((ship.attack + ship.endTurnDamage) * multiplier + 99) / 100;
        const int shieldReward = (ship.shield * multiplier + 99) / 100;
        const int healReward = (ship.heal * multiplier + 99) / 100;
        const int enemyHeatReward = (ship.endTurnEnemyHeat * multiplier + 99) / 100;
        const int extraTriggers = (ship.endTurnExtraOtherTriggers * multiplier + 99) / 100;
        if (damageReward > 0) {
            applyDamageToEnemy(damageReward, false, 1);
            totalDamageReward += damageReward;
        }
        if (shieldReward > 0) {
            state_.player.shield += shieldReward;
            totalShieldReward += shieldReward;
        }
        if (healReward > 0) {
            const int before = state_.player.hp;
            state_.player.hp = std::min(state_.player.maxHp, state_.player.hp + healReward);
            totalHealReward += state_.player.hp - before;
        }
        if (enemyHeatReward > 0) {
            changeEnemyHeat(enemyHeatReward);
            totalEnemyHeatReward += enemyHeatReward;
        }
        if (extraTriggers > 0) {
            totalExtraTriggers += extraTriggers;
        }
        log("Dismantled " + ship.displayName + " for immediate reward.");
        ++destroyed;
    }
    if (totalExtraTriggers > 0) {
        triggerAuxiliaryShipsNow(totalExtraTriggers);
    }
    if (destroyed > 0) {
        log("Lossless Recycling resolves: +" + std::to_string(totalShieldReward) +
            " shield, +" + std::to_string(totalHealReward) + " HP, " +
            std::to_string(totalDamageReward) + " damage, +" +
            std::to_string(totalEnemyHeatReward) + " enemy heat.");
        startAnimation("support",
                       card.name,
                       std::max({totalDamageReward, totalShieldReward, totalHealReward, totalEnemyHeatReward}),
                       true,
                       5);
    }
}

void GameEngine::onAuxiliaryShipRemoved(const AuxiliaryShipState& ship, const char* reason) {
    (void)ship;
    (void)reason;
    if (state_.player.auxOnRemoveDrawThisBattle > 0) {
        drawCards(state_.player.auxOnRemoveDrawThisBattle);
    }
    if (state_.player.auxOnRemoveEnergyThisBattle > 0) {
        state_.player.energy += state_.player.auxOnRemoveEnergyThisBattle;
    }
}

bool GameEngine::resolveBlackboxProtocol(size_t handIndex) {
    if (handIndex >= state_.player.hand.size()) {
        return false;
    }
    if (isTemporaryCopyRef(state_.player.hand[handIndex])) {
        log("Blackbox Protocol cannot imprint a temporary copy.");
        return false;
    }

    const std::string imprintRef = unwrapTemporaryCopyRef(state_.player.hand[handIndex]);
    state_.player.exilePile.push_back(imprintRef);
    state_.player.blackboxImprintRef = imprintRef;
    state_.player.blackboxCopyCount = std::max(1, state_.player.pendingBlackboxCopyCount);
    state_.player.blackboxSelectionPending = false;
    state_.player.pendingBlackboxCopyCount = 0;
    state_.player.hand.erase(state_.player.hand.begin() + static_cast<long long>(handIndex));
    state_.statusLine = "Blackbox imprint set to " + displayCardName(imprintRef) + ".";
    log("Blackbox Protocol exiles " + displayCardName(imprintRef) + " as the imprint.");
    return true;
}

void GameEngine::clearAuxiliaryTransformSelection() {
    state_.player.auxTransformSelectionPending = false;
    state_.player.auxTransformTargetChoicePending = false;
    state_.player.auxTransformRemainingSelections = 0;
    state_.player.auxTransformSelectedShipIndex = -1;
}

std::vector<std::string> GameEngine::currentAuxiliaryTransformTargetUnitIds() const {
    if (!state_.player.auxTransformSelectionPending || !state_.player.auxTransformTargetChoicePending) {
        return {};
    }
    const int ship_index = state_.player.auxTransformSelectedShipIndex;
    if (ship_index < 0 || static_cast<size_t>(ship_index) >= state_.auxiliaryShips.size()) {
        return {};
    }

    std::vector<std::string> targets;
    const std::string current_unit_id = state_.auxiliaryShips[static_cast<size_t>(ship_index)].unitId;
    for (const std::string& unit_id : BasicAuxiliaryUnitIds()) {
        if (unit_id != current_unit_id) {
            targets.push_back(unit_id);
        }
    }
    return targets;
}

void GameEngine::beginAuxiliaryTransformSelection(const CardDef&, int transformCount) {
    const int available_ships = static_cast<int>(state_.auxiliaryShips.size());
    const int selections = std::min(std::max(1, transformCount), available_ships);
    if (selections <= 0) {
        clearAuxiliaryTransformSelection();
        state_.statusLine = "Wing Rotation resolves, but you have no auxiliary ship to transform.";
        log("Wing Rotation finds no auxiliary ship to transform.");
        return;
    }

    state_.player.auxTransformSelectionPending = true;
    state_.player.auxTransformTargetChoicePending = false;
    state_.player.auxTransformRemainingSelections = selections;
    state_.player.auxTransformSelectedShipIndex = -1;
    state_.statusLine = "Wing Rotation armed. Choose an auxiliary ship to transform.";
    log("Wing Rotation awaits auxiliary ship selection.");
}

bool GameEngine::resolveAuxiliaryTransformSelection(size_t index) {
    if (!state_.player.auxTransformSelectionPending || state_.player.auxTransformTargetChoicePending) {
        return false;
    }
    if (index >= state_.auxiliaryShips.size()) {
        return false;
    }
    state_.player.auxTransformSelectedShipIndex = static_cast<int>(index);
    state_.player.auxTransformTargetChoicePending = true;
    state_.statusLine = "Choose a different basic auxiliary type for Wing Rotation.";
    log("Wing Rotation target ship locked.");
    return true;
}

bool GameEngine::resolveAuxiliaryTransformTargetSelection(size_t index) {
    if (!state_.player.auxTransformSelectionPending || !state_.player.auxTransformTargetChoicePending) {
        return false;
    }
    const int ship_index = state_.player.auxTransformSelectedShipIndex;
    if (ship_index < 0 || static_cast<size_t>(ship_index) >= state_.auxiliaryShips.size()) {
        clearAuxiliaryTransformSelection();
        return false;
    }

    const std::vector<std::string> targets = currentAuxiliaryTransformTargetUnitIds();
    if (index >= targets.size()) {
        return false;
    }

    AuxiliaryShipState& ship = state_.auxiliaryShips[static_cast<size_t>(ship_index)];
    const bool upgraded_tier = IsUpgradedBasicAuxiliaryTier(ship);
    const int remaining_turns = ship.remainingTurns;
    const AuxiliaryUnitRecipe recipe = BasicAuxiliaryRecipeForId(targets[index], upgraded_tier);
    ship = MakeAuxiliaryShipState(recipe, remaining_turns);
    log("Wing Rotation transforms support craft into " + ship.displayName + ".");

    --state_.player.auxTransformRemainingSelections;
    if (state_.player.auxTransformRemainingSelections > 0 && !state_.auxiliaryShips.empty()) {
        state_.player.auxTransformTargetChoicePending = false;
        state_.player.auxTransformSelectedShipIndex = -1;
        state_.statusLine = "Wing Rotation can transform one more auxiliary ship.";
        return true;
    }

    clearAuxiliaryTransformSelection();
    state_.statusLine = "Wing Rotation completes its reconfiguration.";
    if (state_.enemy.hp <= 0) {
        handleBattleVictory();
    } else if (state_.player.hp <= 0) {
        handleDefeat();
    }
    return true;
}

bool GameEngine::auxiliaryTransformSelectionPending() const {
    return state_.player.auxTransformSelectionPending;
}

bool GameEngine::auxiliaryTransformTargetChoicePending() const {
    return state_.player.auxTransformSelectionPending && state_.player.auxTransformTargetChoicePending;
}

int GameEngine::auxiliaryTransformSelectedShipIndex() const {
    return state_.player.auxTransformSelectedShipIndex;
}

int GameEngine::auxiliaryTransformRemainingSelections() const {
    return state_.player.auxTransformRemainingSelections;
}

std::vector<std::string> GameEngine::auxiliaryTransformTargetUnitIds() const {
    return currentAuxiliaryTransformTargetUnitIds();
}

bool GameEngine::selectAuxiliaryTransformShip(size_t index) {
    return resolveAuxiliaryTransformSelection(index);
}

bool GameEngine::selectAuxiliaryTransformTarget(size_t index) {
    return resolveAuxiliaryTransformTargetSelection(index);
}

bool GameEngine::isTemporaryCopyRef(const std::string& cardRef) const {
    return cardRef.rfind(kTemporaryCopyPrefix, 0) == 0;
}

std::string GameEngine::makeTemporaryCopyRef(const std::string& cardRef) const {
    return std::string(kTemporaryCopyPrefix) + canonicalCardRef(cardRef);
}

std::string GameEngine::unwrapTemporaryCopyRef(const std::string& cardRef) const {
    if (!isTemporaryCopyRef(cardRef)) {
        return cardRef;
    }
    return cardRef.substr(std::char_traits<char>::length(kTemporaryCopyPrefix));
}

void GameEngine::addBlackboxTurnCopies() {
    if (state_.player.blackboxImprintRef.empty()) {
        return;
    }

    const int copyCount = std::max(1, state_.player.blackboxCopyCount);
    int spawned = 0;
    for (int i = 0; i < copyCount; ++i) {
        if (state_.player.hand.size() >= kBattleHandLimit) {
            break;
        }
        state_.player.hand.push_back(makeTemporaryCopyRef(state_.player.blackboxImprintRef));
        ++spawned;
    }
    if (spawned > 0) {
        log("Blackbox imprint replicates into hand.");
    }
}

void GameEngine::buffAuxiliaryShips(const CardDef& card) {
    if (card.buffDroneAttack == 0 && card.buffDroneShield == 0) {
        return;
    }
    for (AuxiliaryShipState& ship : state_.auxiliaryShips) {
        ship.attack = std::max(0, ship.attack + card.buffDroneAttack);
        ship.shield = std::max(0, ship.shield + card.buffDroneShield);
    }
    log("Auxiliary ship formation is recalibrated.");
}

void GameEngine::shiftPlayerForm(const CardDef& card) {
    if (card.formShift == 0 && !card.formExit) {
        return;
    }
    const int nextForm = card.formExit
        ? kPlayerFormNeutral
        : std::max(kPlayerFormNeutral, std::min(kPlayerFormDefense, card.formShift));
    if (state_.player.form == nextForm) {
        return;
    }

    // Leaving defense form grants configurable transition resources so cards can
    // decide whether they reward energy or immediate shield.
    const int previousForm = state_.player.form;
    if (previousForm == kPlayerFormDefense) {
        state_.player.exitedDefenseThisTurn = true;
        state_.player.exitedFormThisTurn = true;
        if (card.leaveDefenseEnergyBonus > 0) {
            state_.player.nextTurnEnergy += card.leaveDefenseEnergyBonus;
            log("Defense stance release stores +" + std::to_string(card.leaveDefenseEnergyBonus) + " energy for next turn.");
        }
        if (card.leaveDefenseShieldBonus > 0) {
            state_.player.shield += card.leaveDefenseShieldBonus;
            log("Defense stance release grants +" + std::to_string(card.leaveDefenseShieldBonus) + " shield.");
        }
        if (state_.player.leaveDefenseShieldBonusThisTurn > 0) {
            state_.player.shield += state_.player.leaveDefenseShieldBonusThisTurn;
            log("Phase Deflection release grants +" +
                std::to_string(state_.player.leaveDefenseShieldBonusThisTurn) + " shield.");
            state_.player.leaveDefenseShieldBonusThisTurn = 0;
        }
        if (state_.player.leaveDefenseEnemyHeatThisBattle > 0) {
            changeEnemyHeat(state_.player.leaveDefenseEnemyHeatThisBattle);
            log("Defense stance release inflicts enemy heat.");
        }
        if (state_.player.exitDefenseEnergyThisBattle > 0) {
            state_.player.energy += state_.player.exitDefenseEnergyThisBattle;
            log("Perpetual Stillfield grants +" + std::to_string(state_.player.exitDefenseEnergyThisBattle) + " energy.");
        }
        if (!state_.player.firstExitDefenseEnergyTriggeredThisBattle &&
            state_.player.firstExitDefenseEnergyThisBattle > 0) {
            state_.player.energy += state_.player.firstExitDefenseEnergyThisBattle;
            state_.player.firstExitDefenseEnergyTriggeredThisBattle = true;
            log("Dual-phase release grants +" + std::to_string(state_.player.firstExitDefenseEnergyThisBattle) + " energy.");
        }
    }

    state_.player.form = nextForm;
    ++state_.player.formChangesThisTurn;
    resolveFirstFormChangeRelic();
    if (previousForm != kPlayerFormNeutral && previousForm != nextForm) {
        state_.player.exitedFormThisTurn = true;
    }
    if (nextForm == kPlayerFormOverload && previousForm != kPlayerFormOverload) {
        state_.player.enteredOverloadThisTurn = true;
    }
    if (nextForm == kPlayerFormOverload && state_.player.enterOverloadEnemyHeatThisBattle > 0) {
        changeEnemyHeat(state_.player.enterOverloadEnemyHeatThisBattle);
        log("Overload entry inflicts enemy heat.");
    }
    if (nextForm == kPlayerFormOverload && !state_.player.firstEnterOverloadDrawTriggeredThisBattle &&
        state_.player.firstEnterOverloadDrawThisBattle > 0) {
        drawCards(state_.player.firstEnterOverloadDrawThisBattle);
        state_.player.firstEnterOverloadDrawTriggeredThisBattle = true;
        log("Dual-phase entry draws " + std::to_string(state_.player.firstEnterOverloadDrawThisBattle) + " card(s).");
    }
    if (nextForm == kPlayerFormDefense && state_.player.enterDefenseShieldThisBattle > 0) {
        state_.player.shield += state_.player.enterDefenseShieldThisBattle;
        log("Perpetual Stillfield grants +" + std::to_string(state_.player.enterDefenseShieldThisBattle) + " shield.");
    }
    if (state_.player.formChangeDrawThisTurn > 0) {
        drawCards(state_.player.formChangeDrawThisTurn);
        log("Form resonance draws " + std::to_string(state_.player.formChangeDrawThisTurn) + " card(s).");
    }
    log("Combat form shifted to " + playerFormLabel() + ".");
}

void GameEngine::resolveFirstFormChangeRelic() {
    if (!hasRelic("phase_resonator") || state_.player.firstFormChangeRelicTriggeredThisBattle) {
        return;
    }
    state_.player.firstFormChangeRelicTriggeredThisBattle = true;
    drawCards(1);
    log("Phase Resonator draws 1 card on your first form change this battle.");
}

void GameEngine::togglePlayerForm() {
    const int previousForm = state_.player.form;
    if (previousForm == kPlayerFormOverload) {
        state_.player.form = kPlayerFormDefense;
    } else if (previousForm == kPlayerFormDefense) {
        state_.player.form = kPlayerFormOverload;
    } else {
        state_.player.form = kPlayerFormOverload;
    }
    if (previousForm != state_.player.form) {
        ++state_.player.formChangesThisTurn;
        resolveFirstFormChangeRelic();
        if (previousForm == kPlayerFormDefense) {
            state_.player.exitedDefenseThisTurn = true;
            state_.player.exitedFormThisTurn = true;
            if (state_.player.exitDefenseEnergyThisBattle > 0) {
                state_.player.energy += state_.player.exitDefenseEnergyThisBattle;
                log("Perpetual Stillfield grants +" + std::to_string(state_.player.exitDefenseEnergyThisBattle) + " energy.");
            }
            if (state_.player.leaveDefenseShieldBonusThisTurn > 0) {
                state_.player.shield += state_.player.leaveDefenseShieldBonusThisTurn;
                log("Phase Deflection release grants +" +
                    std::to_string(state_.player.leaveDefenseShieldBonusThisTurn) + " shield.");
                state_.player.leaveDefenseShieldBonusThisTurn = 0;
            }
            if (!state_.player.firstExitDefenseEnergyTriggeredThisBattle &&
                state_.player.firstExitDefenseEnergyThisBattle > 0) {
                state_.player.energy += state_.player.firstExitDefenseEnergyThisBattle;
                state_.player.firstExitDefenseEnergyTriggeredThisBattle = true;
                log("Dual-phase release grants +" + std::to_string(state_.player.firstExitDefenseEnergyThisBattle) + " energy.");
            }
        } else if (previousForm != kPlayerFormNeutral) {
            state_.player.exitedFormThisTurn = true;
        }
        if (state_.player.form == kPlayerFormOverload && previousForm != kPlayerFormOverload) {
            state_.player.enteredOverloadThisTurn = true;
        }
        if (state_.player.form == kPlayerFormOverload &&
            !state_.player.firstEnterOverloadDrawTriggeredThisBattle &&
            state_.player.firstEnterOverloadDrawThisBattle > 0) {
            drawCards(state_.player.firstEnterOverloadDrawThisBattle);
            state_.player.firstEnterOverloadDrawTriggeredThisBattle = true;
            log("Dual-phase entry draws " + std::to_string(state_.player.firstEnterOverloadDrawThisBattle) + " card(s).");
        }
        if (state_.player.form == kPlayerFormDefense && state_.player.enterDefenseShieldThisBattle > 0) {
            state_.player.shield += state_.player.enterDefenseShieldThisBattle;
            log("Perpetual Stillfield grants +" + std::to_string(state_.player.enterDefenseShieldThisBattle) + " shield.");
        }
        if (state_.player.formChangeDrawThisTurn > 0) {
            drawCards(state_.player.formChangeDrawThisTurn);
            log("Form resonance draws " + std::to_string(state_.player.formChangeDrawThisTurn) + " card(s).");
        }
    }
    if (previousForm == kPlayerFormDefense && state_.player.leaveDefenseEnemyHeatThisBattle > 0) {
        changeEnemyHeat(state_.player.leaveDefenseEnemyHeatThisBattle);
        log("Defense stance release inflicts enemy heat.");
    }
    if (state_.player.form == kPlayerFormOverload && previousForm != kPlayerFormOverload &&
        state_.player.enterOverloadEnemyHeatThisBattle > 0) {
        changeEnemyHeat(state_.player.enterOverloadEnemyHeatThisBattle);
        log("Overload entry inflicts enemy heat.");
    }
    log("Combat form toggled to " + playerFormLabel() + ".");
}

void GameEngine::clearDestroyedAuxiliaryShips() {
    const auto it = std::remove_if(
        state_.auxiliaryShips.begin(),
        state_.auxiliaryShips.end(),
        [this](const AuxiliaryShipState& ship) {
            if (ship.remainingTurns == 0) {
                onAuxiliaryShipRemoved(ship, "expired");
                return true;
            }
            return false;
        });
    state_.auxiliaryShips.erase(it, state_.auxiliaryShips.end());
}

std::string GameEngine::playerFormLabel() const {
    switch (state_.player.form) {
    case kPlayerFormOverload:
        return "Overload";
    case kPlayerFormDefense:
        return "Defense";
    default:
        return "Neutral";
    }
}

void GameEngine::changeEnemyHeat(int delta) {
    state_.enemy.maxHeat = std::max(1, state_.enemy.maxHeat);
    const int beforeHeat = state_.enemy.heat;
    state_.enemy.heat += delta;
    if (state_.enemy.heat < 0) {
        state_.enemy.heat = 0;
    }
    if (state_.enemy.heat > state_.enemy.maxHeat) {
        state_.enemy.heat = state_.enemy.maxHeat;
    }
    if (state_.page == AppPage::Battle && state_.enemy.heat > beforeHeat) {
        state_.player.enemyHeatRaisedThisTurn += (state_.enemy.heat - beforeHeat);
    }
    syncEnemyOverheatState();
}

bool GameEngine::enemyIsOverheated() const {
    return state_.enemy.heat >= state_.enemy.maxHeat;
}

int GameEngine::enemyOverheatStage() const {
    if (!enemyIsOverheated()) {
        return 0;
    }
    return std::max(1, std::min(3, state_.enemy.overheatRounds));
}

int GameEngine::effectiveEnemyAttackDamage() const {
    int damage = state_.enemy.intentValue;
    if (state_.enemy.weakenTurns > 0) {
        damage = std::max(0, damage - 2);
    }
    if (enemyOverheatStage() >= 1) {
        damage = (damage * 3) / 4;
    }
    return std::max(0, damage);
}

bool GameEngine::enemyShouldUseCooling() const {
    if (state_.enemy.coolingCooldownTurns > 0) {
        return false;
    }
    return state_.enemy.heat >= std::max(1, state_.enemy.maxHeat - 1);
}

int GameEngine::enemyCoolingAmount() const {
    // Enemy emergency cooling scales with sector depth:
    // sector 1 -> 3 heat, sector 2 -> 5 heat, sector 3+ -> 7 heat.
    if (state_.sector <= 1) {
        return 3;
    }
    if (state_.sector == 2) {
        return 5;
    }
    return 7;
}

void GameEngine::triggerEnemyCooling() {
    const int cooling = enemyCoolingAmount();
    changeEnemyHeat(-cooling);
    state_.enemy.coolingCooldownTurns = 2;
    startAnimation("support", state_.enemy.intentLabel, cooling, false, 5);
    log(state_.enemy.name + " vents reactor heat by " + std::to_string(cooling) + ".");
}

void GameEngine::syncEnemyOverheatState() {
    if (enemyIsOverheated()) {
        if (state_.enemy.overheatRounds == 0) {
            state_.enemy.overheatRounds = 1;
            log("Enemy overheat begins. Attack output drops by 25%.");
        } else {
            state_.enemy.overheatRounds = std::min(3, state_.enemy.overheatRounds);
        }
        return;
    }

    if (state_.enemy.overheatRounds != 0) {
        state_.enemy.overheatRounds = 0;
        log("Enemy overheat clears.");
    }
}

void GameEngine::advanceEnemyOverheatRound() {
    if (!enemyIsOverheated()) {
        state_.enemy.overheatRounds = 0;
        return;
    }

    if (state_.enemy.overheatRounds <= 0) {
        state_.enemy.overheatRounds = 1;
        return;
    }

    const int previous = state_.enemy.overheatRounds;
    state_.enemy.overheatRounds = std::min(3, state_.enemy.overheatRounds + 1);
    if (state_.enemy.overheatRounds == previous) {
        return;
    }

    if (state_.enemy.overheatRounds == 2) {
        log("Enemy overheat deepens. Incoming damage rises by 25%.");
    } else if (state_.enemy.overheatRounds == 3) {
        log("Enemy overheat peaks. It will burn for 5 HP each turn.");
    }
}

void GameEngine::log(const std::string& line) {
    state_.battleLog.push_back(line);
    while (state_.battleLog.size() > 7) {
        state_.battleLog.erase(state_.battleLog.begin());
    }
}

void GameEngine::startAnimation(const std::string& kind,
                                const std::string& label,
                                int magnitude,
                                bool playerToEnemy,
                                int duration) {
    state_.animation.kind = kind;
    state_.animation.label = label;
    state_.animation.magnitude = magnitude;
    state_.animation.playerToEnemy = playerToEnemy;
    state_.animation.tick = 0;
    state_.animation.duration = duration;
    state_.animation.active = true;
}
