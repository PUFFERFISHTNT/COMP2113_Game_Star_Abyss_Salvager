#include "engine/game_battle.h"

#include <algorithm>

namespace {

constexpr int kMaxAuxiliaryShips = 3;
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

template <typename T>
void ShuffleVector(std::vector<T>& values, std::mt19937& rng) {
    std::shuffle(values.begin(), values.end(), rng);
}

bool IsAttackOrHybridSummon(const CardDef& card) {
    return card.summonDroneAttack >= card.summonDroneShield;
}

}  // namespace

bool GameEngine::canPlayCard(size_t index) const {
    if (state_.page != AppPage::Battle) {
        return false;
    }
    if (state_.player.blackboxSelectionPending) {
        return index < state_.player.hand.size();
    }
    if (index >= state_.player.hand.size()) {
        return false;
    }
    const std::string cardRef = unwrapTemporaryCopyRef(state_.player.hand[index]);
    const CardDef* baseCard = cardById(cardRef);
    if (baseCard == nullptr) {
        return false;
    }
    return resolvedCard(cardRef).cost <= state_.player.energy;
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

    state_.player.energy -= card.cost;
    if (!temporaryCopy) {
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
    const int activeAuxCount = static_cast<int>(state_.auxiliaryShips.size());
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
    if (baseCardId == "vg_08" && state_.player.heat == 0) {
        card.heat += 1;
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
        state_.player.nextHeatGainReductionThisTurn += 2;
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
    if (baseCardId == "df_13" && hasPulseAux) {
        drawCount += 1;
    }
    if (baseCardId == "df_22" && activeAuxCount > 0) {
        outgoingDamage += activeAuxCount * 3;
    }
    if (baseCardId == "df_30" && activeAuxCount > 0) {
        outgoingDamage += activeAuxCount * 2;
    }
    if (baseCardId == "ib_05" && inOverload) {
        outgoingDamage = 12;
    }
    if (baseCardId == "ib_27" && inOverload) {
        outgoingDamage = 46;
    }
    if (baseCardId == "ib_03" && state_.player.exitedFormThisTurn) {
        outgoingDamage += 3;
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
    if (baseCardId == "ib_13" && inOverload) {
        card.attackMultiplier = 3;
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
        enemyHeatDelta = state_.player.formChangesThisTurn > 0 ? 5 : 2;
    }
    if (baseCardId == "vgh_02" && state_.player.heatGainedThisTurn == 0) {
        drawCount = 0;
    }
    if (baseCardId == "vgh_03") {
        enemyHeatDelta = playerHighHeat ? 5 : 3;
    }
    if (baseCardId == "vgh_06") {
        enemyHeatDelta = 0;
        state_.player.nextAttackEnemyHeatBonusThisTurn += 3;
    }
    if (baseCardId == "dfh_02") {
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
        outgoingDamage += 3;
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
        state_.player.attackBonusPerHeatThisTurn = std::max(state_.player.attackBonusPerHeatThisTurn, 1);
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
        state_.player.auxOnRemoveDrawThisBattle = std::max(state_.player.auxOnRemoveDrawThisBattle, 1);
        state_.player.auxOnRemoveEnergyThisBattle = std::max(state_.player.auxOnRemoveEnergyThisBattle, 1);
    }
    if (baseCardId == "ib_17" && inOverload) {
        outgoingDamage += 7;
    }
    if (baseCardId == "ib_18") {
        drawCount = 0;
        state_.player.formChangeDrawThisTurn += 1;
    }
    if (baseCardId == "ib_23") {
        if (state_.player.enteredOverloadThisTurn && state_.player.exitedDefenseThisTurn) {
            outgoingDamage = 26;
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
    if (baseCardId == "vgh_07" && state_.player.enemyHeatRaisedThisTurn > 0) {
        enemyHeatDelta += 4;
    }
    if (baseCardId == "vgh_08") {
        card.heat += 4;
        enemyHeatDelta += 4;
        if (playerHighHeat) {
            outgoingDamage += 7;
        }
    }
    if (baseCardId == "vgh_09") {
        enemyHeatDelta = enemyHighHeat ? 8 : 5;
    }
    if (baseCardId == "ibh_07") {
        if (inOverload) {
            shieldValue = 0;
            enemyHeatDelta = 6;
        } else if (inDefense) {
            shieldValue = 7;
            enemyHeatDelta = 4;
        } else {
            shieldValue = 0;
            enemyHeatDelta = 0;
        }
    }
    if (baseCardId == "ibh_09" && enemyHighHeat) {
        outgoingDamage += 7;
        enemyHeatDelta += 4;
    }
    if (baseCardId == "cl_06" && state_.player.attacksPlayedThisTurn == 0) {
        shieldValue += 7;
        drawCount += 1;
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
    if (outgoingDamage > 0 && state_.player.nextAttackMultiplier > 1) {
        outgoingDamage *= state_.player.nextAttackMultiplier;
        state_.player.nextAttackMultiplier = 1;
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

    summonAuxiliaryShips(card);
    if (card.auxImmediateTriggerTimes > 0) {
        triggerAuxiliaryShipsNow(card.auxImmediateTriggerTimes);
    }
    if (card.auxEndTurnExtraTriggerTimes > 0) {
        state_.player.auxEndTurnExtraTriggersThisTurn += card.auxEndTurnExtraTriggerTimes;
        log("Auxiliary ships will trigger extra times at end of turn.");
    }
    if (card.auxEffectPercentBonus > 0) {
        state_.player.auxEffectPercentBonusThisTurn += card.auxEffectPercentBonus;
        log("Auxiliary effect output is amplified this turn.");
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

    if (state_.enemy.hp <= 0) {
        handleBattleVictory();
    } else if (state_.player.hp <= 0) {
        handleDefeat();
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
    state_.player.salvageBonus = 0;
    state_.player.attacksPlayedThisTurn = 0;
    state_.player.form = kPlayerFormNeutral;
    state_.player.auxEndTurnExtraTriggersThisTurn = 0;
    state_.player.auxEffectPercentBonusThisTurn = 0;
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
    state_.player.auxOnRemoveDrawThisBattle = 0;
    state_.player.auxOnRemoveEnergyThisBattle = 0;
    state_.player.blackboxCopyCount = 0;
    state_.player.firstSummonUsedThisBattle = false;
    state_.player.firstSummonUsedThisTurn = false;
    state_.player.firstEnterHighHeatTriggeredThisBattle = false;
    state_.player.firstSummonEnemyHeatTriggeredThisTurn = false;
    state_.player.enteredOverloadThisTurn = false;
    state_.player.exitedDefenseThisTurn = false;
    state_.player.exitedFormThisTurn = false;
    state_.player.blackboxSelectionPending = false;
    state_.player.pendingBlackboxCopyCount = 0;
    state_.player.blackboxImprintRef.clear();
    state_.player.exilePile.clear();
    state_.auxiliaryShips.clear();
    state_.page = AppPage::Battle;
    resumePage_ = AppPage::Battle;
    resetBattleDeck();
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
    if (state_.firstTurnOfBattle && state_.player.openingEnergyBonus > 0) {
        energy += state_.player.openingEnergyBonus;
        log("Stored anomaly charge grants +" + std::to_string(state_.player.openingEnergyBonus) + " energy.");
        state_.player.openingEnergyBonus = 0;
    }
    state_.player.energy = std::max(0, energy);
    state_.player.nextTurnEnergy = 0;
    state_.player.attacksPlayedThisTurn = 0;
    state_.player.auxEndTurnExtraTriggersThisTurn = 0;
    state_.player.auxEffectPercentBonusThisTurn = 0;
    state_.player.overloadIncomingDamageReducePercentThisTurn = 0;
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
    state_.player.enteredOverloadThisTurn = false;
    state_.player.exitedDefenseThisTurn = false;
    state_.player.exitedFormThisTurn = false;
    state_.player.blackboxSelectionPending = false;
    state_.player.pendingBlackboxCopyCount = 0;
    addBlackboxTurnCopies();
    ++state_.battleTurn;

    const int startShield = (hasRelic("shield_matrix") && state_.battleTurn == 1 ? 4 : 0) + state_.player.openingShieldBonus;
    if (startShield > 0) {
        state_.player.shield += startShield;
        log("Deflector envelope stabilizes for +" + std::to_string(startShield) + " shield.");
        state_.player.openingShieldBonus = 0;
    }

    drawCards(drawTarget - static_cast<int>(state_.player.hand.size()));
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
    state_.reward.cardIds = randomCardChoices(3);

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
    state_.player.auxEndTurnExtraTriggersThisTurn = 0;
    state_.player.auxEffectPercentBonusThisTurn = 0;
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
    state_.player.auxOnRemoveDrawThisBattle = 0;
    state_.player.auxOnRemoveEnergyThisBattle = 0;
    state_.player.firstSummonUsedThisBattle = false;
    state_.player.firstSummonUsedThisTurn = false;
    state_.player.firstEnterHighHeatTriggeredThisBattle = false;
    state_.player.firstSummonEnemyHeatTriggeredThisTurn = false;
    state_.player.enteredOverloadThisTurn = false;
    state_.player.exitedDefenseThisTurn = false;
    state_.player.exitedFormThisTurn = false;
    state_.player.blackboxCopyCount = 0;
    state_.player.blackboxSelectionPending = false;
    state_.player.pendingBlackboxCopyCount = 0;
    state_.player.blackboxImprintRef.clear();
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
    state_.player.auxEndTurnExtraTriggersThisTurn = 0;
    state_.player.auxEffectPercentBonusThisTurn = 0;
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
    state_.player.auxOnRemoveDrawThisBattle = 0;
    state_.player.auxOnRemoveEnergyThisBattle = 0;
    state_.player.firstSummonUsedThisBattle = false;
    state_.player.firstSummonUsedThisTurn = false;
    state_.player.firstEnterHighHeatTriggeredThisBattle = false;
    state_.player.firstSummonEnemyHeatTriggeredThisTurn = false;
    state_.player.enteredOverloadThisTurn = false;
    state_.player.exitedDefenseThisTurn = false;
    state_.player.exitedFormThisTurn = false;
    state_.player.blackboxCopyCount = 0;
    state_.player.blackboxSelectionPending = false;
    state_.player.pendingBlackboxCopyCount = 0;
    state_.player.blackboxImprintRef.clear();
    state_.player.exilePile.clear();
    state_.enemy.heat = 0;
    state_.enemy.overheatRounds = 0;
    state_.enemy.coolingCooldownTurns = 0;
    state_.auxiliaryShips.clear();
    fillSummary(false);
}

void GameEngine::applyDamageToPlayer(int amount) {
    int remaining = amount;
    // Overload form doubles incoming damage, which defines the form's drawback.
    if (state_.player.form == kPlayerFormOverload && remaining > 0) {
        const int dampen = std::max(0, std::min(100, state_.player.overloadIncomingDamageReducePercentThisTurn));
        const int overload_multiplier = std::max(100, 200 - dampen);
        remaining = (remaining * overload_multiplier + 99) / 100;
    }
    if (playerOverheatStage() >= 2 && remaining > 0) {
        remaining = (remaining * 5 + 3) / 4;
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

void GameEngine::summonAuxiliaryShips(const CardDef& card) {
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

    // Auxiliary ships are bounded by a hard slot cap so future summon cards can
    // use "replace oldest" behavior without extra UI.
    for (int i = 0; i < card.summonDroneCount + extraCopies; ++i) {
        if (state_.auxiliaryShips.size() >= static_cast<size_t>(kMaxAuxiliaryShips)) {
            log("Auxiliary bay is full. Oldest support craft is dismissed.");
            onAuxiliaryShipRemoved(state_.auxiliaryShips.front(), "replaced");
            state_.auxiliaryShips.erase(state_.auxiliaryShips.begin());
        }

        AuxiliaryShipState drone;
        const auto profile = AuxiliaryUnitProfileForCard(card);
        drone.unitId = profile.first;
        drone.displayName = profile.second;
        drone.attack = std::max(0, card.summonDroneAttack);
        drone.shield = std::max(0, card.summonDroneShield);
        drone.remainingTurns = card.summonDroneDuration > 0 ? card.summonDroneDuration : -1;
        state_.auxiliaryShips.push_back(drone);
        log(drone.displayName + " deployed.");
    }
}

void GameEngine::resolveAuxiliaryShipsEndTurn() {
    if (state_.auxiliaryShips.empty()) {
        return;
    }

    const int triggerTimes = std::max(1, 1 + state_.player.auxEndTurnExtraTriggersThisTurn);
    const int bonusPercent = std::max(0, state_.player.auxEffectPercentBonusThisTurn);
    for (AuxiliaryShipState& ship : state_.auxiliaryShips) {
        for (int trigger = 0; trigger < triggerTimes; ++trigger) {
            const int attack = (ship.attack * (100 + bonusPercent) + 99) / 100;
            const int shield = (ship.shield * (100 + bonusPercent) + 99) / 100;
            if (attack > 0) {
                applyDamageToEnemy(attack, false, 1);
                log(ship.displayName + " fires for " + std::to_string(attack) + ".");
                if (state_.player.auxTriggerEnemyHeatBonusThisTurn > 0) {
                    changeEnemyHeat(state_.player.auxTriggerEnemyHeatBonusThisTurn);
                }
            }
            if (shield > 0) {
                state_.player.shield += shield;
                log(ship.displayName + " reinforces shield by " + std::to_string(shield) + ".");
            }
        }
        if (ship.remainingTurns > 0) {
            --ship.remainingTurns;
        }
    }

    clearDestroyedAuxiliaryShips();
}

void GameEngine::triggerAuxiliaryShipsNow(int times) {
    if (times <= 0 || state_.auxiliaryShips.empty()) {
        return;
    }
    const int bonusPercent = std::max(0, state_.player.auxEffectPercentBonusThisTurn);
    for (int t = 0; t < times; ++t) {
        for (const AuxiliaryShipState& ship : state_.auxiliaryShips) {
            const int attack = (ship.attack * (100 + bonusPercent) + 99) / 100;
            const int shield = (ship.shield * (100 + bonusPercent) + 99) / 100;
            if (attack > 0) {
                applyDamageToEnemy(attack, false, 1);
                log(ship.displayName + " executes an immediate attack for " + std::to_string(attack) + ".");
                if (state_.player.auxTriggerEnemyHeatBonusThisTurn > 0) {
                    changeEnemyHeat(state_.player.auxTriggerEnemyHeatBonusThisTurn);
                }
            }
            if (shield > 0) {
                state_.player.shield += shield;
                log(ship.displayName + " grants immediate shield +" + std::to_string(shield) + ".");
            }
        }
    }
}

void GameEngine::resolveAuxiliaryShipDestroyRewards(const CardDef& card) {
    if (card.auxDestroyCount <= 0) {
        return;
    }
    if (state_.auxiliaryShips.empty()) {
        log("No auxiliary ship is available to dismantle.");
        return;
    }

    const int multiplier = std::max(0, card.auxDestroyRewardMultiplierPercent);
    int destroyed = 0;
    while (destroyed < card.auxDestroyCount && !state_.auxiliaryShips.empty()) {
        AuxiliaryShipState ship = state_.auxiliaryShips.front();
        state_.auxiliaryShips.erase(state_.auxiliaryShips.begin());
        onAuxiliaryShipRemoved(ship, "dismantled");
        const int attackReward = (ship.attack * multiplier + 99) / 100;
        const int shieldReward = (ship.shield * multiplier + 99) / 100;
        if (attackReward > 0) {
            applyDamageToEnemy(attackReward, false, 1);
        }
        if (shieldReward > 0) {
            state_.player.shield += shieldReward;
        }
        log("Dismantled " + ship.displayName + " for immediate reward.");
        ++destroyed;
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
        if (state_.player.leaveDefenseEnemyHeatThisBattle > 0) {
            changeEnemyHeat(state_.player.leaveDefenseEnemyHeatThisBattle);
            log("Defense stance release inflicts enemy heat.");
        }
    }

    state_.player.form = nextForm;
    ++state_.player.formChangesThisTurn;
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
    if (state_.player.formChangeDrawThisTurn > 0) {
        drawCards(state_.player.formChangeDrawThisTurn);
        log("Form resonance draws " + std::to_string(state_.player.formChangeDrawThisTurn) + " card(s).");
    }
    log("Combat form shifted to " + playerFormLabel() + ".");
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
        if (previousForm == kPlayerFormDefense) {
            state_.player.exitedDefenseThisTurn = true;
            state_.player.exitedFormThisTurn = true;
        } else if (previousForm != kPlayerFormNeutral) {
            state_.player.exitedFormThisTurn = true;
        }
        if (state_.player.form == kPlayerFormOverload && previousForm != kPlayerFormOverload) {
            state_.player.enteredOverloadThisTurn = true;
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
