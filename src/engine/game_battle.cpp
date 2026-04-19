#include "engine/game_battle.h"

#include <algorithm>

namespace {

template <typename T>
void ShuffleVector(std::vector<T>& values, std::mt19937& rng) {
    std::shuffle(values.begin(), values.end(), rng);
}

}  // namespace

bool GameEngine::canPlayCard(size_t index) const {
    if (state_.page != AppPage::Battle) {
        return false;
    }
    if (index >= state_.player.hand.size()) {
        return false;
    }
    const CardDef* baseCard = cardById(state_.player.hand[index]);
    if (baseCard == nullptr) {
        return false;
    }
    return resolvedCard(state_.player.hand[index]).cost <= state_.player.energy;
}

void GameEngine::playCard(size_t index) {
    if (!canPlayCard(index)) {
        state_.statusLine = "That module cannot be fired right now.";
        return;
    }

    const std::string cardId = state_.player.hand[index];
    const CardDef* baseCard = cardById(cardId);
    if (baseCard == nullptr) {
        return;
    }
    const CardDef card = resolvedCard(cardId);

    state_.player.energy -= card.cost;
    state_.player.discardPile.push_back(cardId);
    state_.player.hand.erase(state_.player.hand.begin() + static_cast<long long>(index));

    int outgoingDamage = card.damage;
    if (card.comboBonus > 0 && state_.player.attacksPlayedThisTurn > 0) {
        outgoingDamage += card.comboBonus;
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

    if (card.attackMultiplier > 1) {
        state_.player.nextAttackMultiplier *= card.attackMultiplier;
        log("Target lock acquired. The next attack will strike harder.");
    }
    if (card.shield > 0) {
        state_.player.shield += card.shield;
        log("Shield rises by " + std::to_string(card.shield) + ".");
    }
    if (card.heal > 0) {
        state_.player.hp = std::min(state_.player.maxHp, state_.player.hp + card.heal);
        log("Hull restores " + std::to_string(card.heal) + " HP.");
    }
    if (card.draw > 0) {
        drawCards(card.draw);
    }
    if (card.energy > 0) {
        state_.player.energy += card.energy;
        log("Reactor gain: +" + std::to_string(card.energy) + " energy.");
    }
    if (card.nextTurnEnergy != 0) {
        state_.player.nextTurnEnergy += card.nextTurnEnergy;
        log("Next turn energy buffered: +" + std::to_string(card.nextTurnEnergy) + ".");
    }
    if (card.halveHeat) {
        state_.player.heat /= 2;
        log("Heat is cut in half.");
    }
    if (card.clearHeat) {
        state_.player.heat = 0;
        log("All heat is vented from the reactor.");
    }
    if (card.heat != 0) {
        changeHeat(card.heat);
        if (card.heat > 0) {
            log("Heat climbs by " + std::to_string(card.heat) + ".");
        } else {
            log("Heat drops by " + std::to_string(-card.heat) + ".");
        }
    }
    if (card.enemyHeat != 0) {
        changeEnemyHeat(card.enemyHeat);
        if (card.enemyHeat > 0) {
            log("Enemy heat climbs by " + std::to_string(card.enemyHeat) + ".");
        } else {
            log("Enemy heat drops by " + std::to_string(-card.enemyHeat) + ".");
        }
    }
    if (card.selfDamage > 0) {
        applyDamageToPlayer(card.selfDamage);
        log("Feedback lashes your own hull for " + std::to_string(card.selfDamage) + ".");
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

    if (outgoingDamage > 0) {
        applyDamageToEnemy(outgoingDamage, card.ignoreShield, std::max(1, card.multiHit));
        startAnimation("attack", card.name, outgoingDamage, true, 7);
        if (card.multiHit > 1) {
            log(card.name + " deals " + std::to_string(outgoingDamage) + " x " +
                std::to_string(card.multiHit) + ".");
        } else {
            log(card.name + " deals " + std::to_string(outgoingDamage) + ".");
        }
    } else if (card.shield > 0) {
        startAnimation("shield", card.name, card.shield, true, 5);
    } else {
        startAnimation("support", card.name, std::max({card.heal, std::abs(card.heat), std::abs(card.enemyHeat)}), true, 5);
    }

    if (card.type == "Attack" || outgoingDamage > 0) {
        ++state_.player.attacksPlayedThisTurn;
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

    for (const std::string& card : state_.player.hand) {
        state_.player.discardPile.push_back(card);
    }
    state_.player.hand.clear();
    log("Your remaining hand is discarded.");

    if (hasRelic("auto_cooler") && state_.player.heat > 0) {
        state_.player.heat = std::max(0, state_.player.heat - 1);
        log("Auto-Cooler bleeds 1 heat from the reactor.");
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
    state_.statusLine = "Enemy cycle resolved. Your turn begins.";
}

void GameEngine::startBattle(const std::string& nodeType) {
    const EnemyTemplate& enemy = pickEnemy(nodeType);
    state_.enemy.enemyId = enemy.id;
    state_.enemy.name = enemy.name;
    state_.enemy.behavior = enemy.behavior;
    state_.enemy.maxHp = enemy.baseHp + (state_.sector - 1) * (enemy.boss ? 8 : (enemy.elite ? 5 : 3));
    state_.enemy.hp = state_.enemy.maxHp;
    state_.enemy.shield = 0;
    state_.enemy.heat = 0;
    state_.enemy.maxHeat = enemy.maxHeat;
    state_.enemy.overheatRounds = 0;
    state_.enemy.patternIndex = 0;
    state_.enemy.weakenTurns = 0;
    state_.enemy.shieldLockTurns = 0;
    state_.battleLog.clear();
    state_.battleTurn = 0;
    state_.firstTurnOfBattle = true;
    state_.player.nextAttackMultiplier = 1;
    state_.player.salvageBonus = 0;
    state_.player.attacksPlayedThisTurn = 0;
    resetBattleDeck();
    chooseEnemyIntent();
    startPlayerTurn();
    state_.page = AppPage::Battle;
    resumePage_ = AppPage::Battle;
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
        advanceEnemyOverheatRound();
    }

    const int drawTarget = state_.player.heat >= state_.player.maxHeat ? (kBattleDrawCount - 1) : kBattleDrawCount;
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
    }

    if (state_.enemy.shieldLockTurns > 0) {
        --state_.enemy.shieldLockTurns;
    }
    ++state_.enemy.patternIndex;
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
    state_.enemy.heat = 0;
    state_.enemy.overheatRounds = 0;
    state_.battleTurn = 0;
    state_.pendingSectorAdvance = state_.activeNodeType == "boss" && state_.sector < 3;
    state_.pendingVictory = state_.activeNodeType == "boss" && state_.sector >= 3;
    state_.page = AppPage::Reward;
    resumePage_ = AppPage::Reward;
    state_.statusLine = "Enemy destroyed. Choose your salvage reward.";
}

void GameEngine::handleDefeat() {
    state_.enemy.heat = 0;
    state_.enemy.overheatRounds = 0;
    fillSummary(false);
}

void GameEngine::applyDamageToPlayer(int amount) {
    int remaining = amount;
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
    state_.player.heat += delta;
    if (state_.player.heat < 0) {
        state_.player.heat = 0;
    }
    if (state_.player.heat > state_.player.maxHeat) {
        const int overflow = state_.player.heat - state_.player.maxHeat;
        state_.player.heat = state_.player.maxHeat;
        applyDamageToPlayer(overflow + 1);
        log("Overheat! Feedback burns through the hull for " + std::to_string(overflow + 1) + ".");
    }
}

void GameEngine::changeEnemyHeat(int delta) {
    state_.enemy.maxHeat = std::max(1, state_.enemy.maxHeat);
    state_.enemy.heat += delta;
    if (state_.enemy.heat < 0) {
        state_.enemy.heat = 0;
    }
    if (state_.enemy.heat > state_.enemy.maxHeat) {
        state_.enemy.heat = state_.enemy.maxHeat;
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
