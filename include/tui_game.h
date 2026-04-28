#ifndef TUI_GAME_H
#define TUI_GAME_H

#include <random>
#include <string>
#include <utility>
#include <vector>

inline constexpr int kBattleDrawCount = 5;
inline constexpr int kBattleHandLimit = 10;
inline constexpr int kMaxAuxiliaryShips = 3;
inline constexpr int kMarketCardUpgradeBaseCost = 50;
inline constexpr int kCardUpgradeMaxLevel = 1;

enum class AppPage {
    MainMenu = 0,
    Setup,
    Map,
    Encounter,
    Battle,
    Reward,
    Summary,
    Info,
};

enum class CardUpgradeSource {
    RepairStation = 0,
    Market,
    EventReward,
};

enum class CardRarity {
    Common = 0,
    Uncommon,
    Rare,
};

struct CardDef {
    std::string id;
    std::string name;
    std::string nameEn;
    std::string type;
    std::string description;
    std::string descriptionEn;
    CardRarity rarity = CardRarity::Common;
    int starterCopies = 0;
    int cost = 0;
    int damage = 0;
    int shield = 0;
    int heal = 0;
    int draw = 0;
    int energy = 0;
    int nextTurnEnergy = 0;
    int heat = 0;
    int enemyHeat = 0;
    int selfDamage = 0;
    int multiHit = 1;
    int comboBonus = 0;
    int attackMultiplier = 1;
    int weakenTurns = 0;
    int removeEnemyShield = 0;
    int shieldLockTurns = 0;
    int salvageBonus = 0;
    int summonDroneCount = 0;
    int summonDroneAttack = 0;
    int summonDroneShield = 0;
    int summonDroneDuration = 0;
    int buffDroneAttack = 0;
    int buffDroneShield = 0;
    int auxImmediateTriggerTimes = 0;
    int auxEndTurnExtraTriggerTimes = 0;
    int auxEffectPercentBonus = 0;
    int auxSlotBonus = 0;
    int auxDestroyCount = 0;
    int auxDestroyRewardMultiplierPercent = 0;
    int firstSummonExtraCopies = 0;
    int firstSummonExtraScope = 0;
    int summonCardPlayedDrawThisTurn = 0;
    int formChangesThisTurnThreshold = 0;
    int formChangesThisTurnBonusDamage = 0;
    int firstEnterHighHeatEnergyThisBattle = 0;
    int firstEnterHighHeatDrawThisBattle = 0;
    int eachEnterHighHeatDrawThisBattle = 0;
    int firstSummonEnemyHeatPerTurnThisBattle = 0;
    int firstSummonEnemyHeatAttackBonusPerTurnThisBattle = 0;
    int enterOverloadEnemyHeatThisBattle = 0;
    int leaveDefenseEnemyHeatThisBattle = 0;
    int formShift = 0;
    int overloadIncomingDamageReducePercent = 0;
    int blackboxCopyCount = 0;
    int leaveDefenseEnergyBonus = 0;
    int leaveDefenseShieldBonus = 0;
    bool formToggle = false;
    bool formExit = false;
    bool ignoreShield = false;
    bool shieldToDamage = false;
    bool halveHeat = false;
    bool clearHeat = false;
    bool randomEffect = false;
    bool blackboxProtocol = false;
};

struct CardView {
    std::string ref;
    CardDef def;
    int upgradeLevel = 0;
};

struct CardOwnershipEntry {
    CardView card;
    int copies = 0;
};

struct CardLibraryEntry {
    CardView card;
    bool obtained = false;
};

struct AchievementEntry {
    std::string id;
    std::string title;
    std::string description;
    bool obtained = false;
};

struct NodeOption {
    std::string typeId;
    std::string name;
    std::string detail;
    int danger = 1;
    std::string enemyId;
};

struct EncounterState {
    std::string typeId;
    std::string encounterId;
    std::string title;
    std::string description;
    std::vector<std::string> choices;
    std::vector<std::string> details;
    std::vector<std::string> payloadIds;
    std::vector<int> payloadValues;
};

struct RewardState {
    std::string banner;
    int scrap = 0;
    std::string relicId;
    std::vector<std::string> cardIds;
};

struct InfoState {
    std::string title;
    std::string footer;
    std::vector<std::string> lines;
};

struct AnimationState {
    std::string kind;
    std::string label;
    int magnitude = 0;
    bool playerToEnemy = true;
    int tick = 0;
    int duration = 0;
    bool active = false;
};

struct ActiveAbilityState {
    std::string cardRef;
    std::string name;
    std::string nameEn;
    std::string description;
    std::string descriptionEn;
};

struct PlayerState {
    std::string shipId = "vanguard";
    std::string shipName = "Vanguard";
    int hp = 40;
    int maxHp = 40;
    int shield = 0;
    int energy = 3;
    int maxEnergy = 3;
    int heat = 0;
    int maxHeat = 10;
    int overheatRounds = 0;
    int scrap = 0;
    int nextTurnEnergy = 0;
    int nextTurnDraw = 0;
    int nextBattleFirstAttackMultiplier = 1;
    int nextBattleHeal = 0;
    int openingEnergyBonus = 0;
    int openingShieldBonus = 0;
    int nextAttackMultiplier = 1;
    int nextIncomingDamageFlatReductionThisTurn = 0;
    int nextAttackPreMultiplierBonusPercentThisTurn = 0;
    int salvageBonus = 0;
    int attacksPlayedThisTurn = 0;
    int form = 0;
    int auxEndTurnExtraTriggersThisTurn = 0;
    int auxEffectPercentBonusThisTurn = 0;
    int auxSlotBonusThisBattle = 0;
    int overloadIncomingDamageReducePercentThisTurn = 0;
    int firstSummonExtraCopies = 0;
    int firstSummonExtraScope = 0;
    int blackboxCopyCount = 0;
    int summonCardPlayedDrawThisTurnBuff = 0;
    int formChangesThisTurn = 0;
    int firstEnterHighHeatEnergyThisBattle = 0;
    int firstEnterHighHeatDrawThisBattle = 0;
    int eachEnterHighHeatDrawThisBattle = 0;
    int firstSummonEnemyHeatPerTurnThisBattle = 0;
    int firstSummonEnemyHeatAttackBonusPerTurnThisBattle = 0;
    int enterOverloadEnemyHeatThisBattle = 0;
    int leaveDefenseEnemyHeatThisBattle = 0;
    int heatGainedThisTurn = 0;
    int heatReducedThisTurn = 0;
    int enemyHeatRaisedThisTurn = 0;
    int nextHeatGainReductionThisTurn = 0;
    int nextAttackEnemyHeatBonusThisTurn = 0;
    int attackBonusPerHeatThisTurn = 0;
    int attackBonusPerHeatCapThisTurn = 0;
    int attackBonusFromHeatGainThisBattle = 0;
    int attackBonusPerHeatGainPointThisBattle = 0;
    int killAttackBonusThisBattle = 0;
    int auxTriggerEnemyHeatBonusThisTurn = 0;
    int formChangeDrawThisTurn = 0;
    int overheatSelfDamageBlockThisTurn = 0;
    int auxOnRemoveDrawThisBattle = 0;
    int auxOnRemoveEnergyThisBattle = 0;
    int enterDefenseShieldThisBattle = 0;
    int exitDefenseEnergyThisBattle = 0;
    int firstEnterOverloadDrawThisBattle = 0;
    int firstExitDefenseEnergyThisBattle = 0;
    int leaveDefenseShieldBonusThisTurn = 0;
    bool firstSummonUsedThisBattle = false;
    bool firstSummonUsedThisTurn = false;
    bool firstEnterHighHeatTriggeredThisBattle = false;
    bool firstEnterOverloadDrawTriggeredThisBattle = false;
    bool firstExitDefenseEnergyTriggeredThisBattle = false;
    bool firstSummonEnemyHeatTriggeredThisTurn = false;
    bool firstFormChangeRelicTriggeredThisBattle = false;
    bool enteredOverloadThisTurn = false;
    bool exitedDefenseThisTurn = false;
    bool exitedFormThisTurn = false;
    bool firstCardCostZeroPendingNextTurn = false;
    bool firstCardCostZeroThisTurn = false;
    bool blackboxSelectionPending = false;
    int pendingBlackboxCopyCount = 0;
    std::string blackboxImprintRef;
    bool auxTransformSelectionPending = false;
    bool auxTransformTargetChoicePending = false;
    int auxTransformRemainingSelections = 0;
    int auxTransformSelectedShipIndex = -1;
    bool emergencyBatteryUsed = false;
    std::vector<std::string> relics;
    std::vector<std::string> drawPile;
    std::vector<std::string> hand;
    std::vector<std::string> discardPile;
    std::vector<std::string> exilePile;
    std::vector<ActiveAbilityState> activeAbilities;
};

struct AuxiliaryShipState {
    std::string unitId;
    std::string displayName;
    int attack = 0;
    int shield = 0;
    int heal = 0;
    int remainingTurns = 0;
    int endTurnDamage = 0;
    int endTurnEnemyHeat = 0;
    int endTurnExtraOtherTriggers = 0;
};

struct EnemyState {
    std::string enemyId;
    std::string name;
    std::string behavior;
    int hp = 0;
    int maxHp = 0;
    int shield = 0;
    int heat = 0;
    int maxHeat = 10;
    int overheatRounds = 0;
    int coolingCooldownTurns = 0;
    std::string intentKind;
    std::string intentLabel;
    int intentValue = 0;
    std::string secondaryIntentKind;
    std::string secondaryIntentLabel;
    int secondaryIntentValue = 0;
    int patternIndex = 0;
    int weakenTurns = 0;
    int shieldLockTurns = 0;
    int nextAttackBonusDamage = 0;
};

struct RunState {
    bool hasRun = false;
    AppPage page = AppPage::MainMenu;
    int difficulty = 1;
    int sector = 1;
    int depth = 0;
    int nodesPerSector = 5;
    int battleTurn = 0;
    int marketUpgradeCount = 0;
    bool marketPurgeUsed = false;
    bool repairStationPatchUsed = false;
    bool repairStationVentUsed = false;
    bool repairStationPlatingUsed = false;
    bool repairStationUpgradeUsed = false;
    bool firstTurnOfBattle = true;
    bool pendingSectorAdvance = false;
    bool pendingVictory = false;
    std::string activeNodeType;
    std::string activeNodeName;
    std::string statusLine;
    PlayerState player;
    EnemyState enemy;
    // Legacy saves may still carry type-level upgrade markers. They are
    // normalized into per-instance refs during load and are not written again.
    std::vector<std::string> upgradedCardTypes;
    std::vector<std::string> marketOfferCardIds;
    std::vector<NodeOption> routeOptions;
    EncounterState encounter;
    RewardState reward;
    InfoState info;
    AnimationState animation;
    std::vector<std::string> battleLog;
    std::vector<AuxiliaryShipState> auxiliaryShips;
};

class GameEngine {
public:
    // what it does: Constructs the game engine and initializes its internal data.
    // what the inputs are: None.
    // what the outputs are: Creates a GameEngine instance (constructor).
    GameEngine();

    // what it does: Provides read-only access to the current run state.
    // what the inputs are: None.
    // what the outputs are: Returns the current run state (const RunState&).
    const RunState& state() const;

    // what it does: Starts a new run with the selected difficulty and ship.
    // what the inputs are: difficulty - selected difficulty level; shipId - selected ship identifier.
    // what the outputs are: No return value (void).
    void startNewRun(int difficulty, const std::string& shipId);

    // what it does: Saves the current run state to a file.
    // what the inputs are: path - target save file path; error - optional error message output.
    // what the outputs are: Returns true when saving succeeds; otherwise false (bool).
    bool saveToFile(const std::string& path, std::string* error) const;

    // what it does: Loads a run state from a save file.
    // what the inputs are: path - source save file path; error - optional error message output.
    // what the outputs are: Returns true when loading succeeds; otherwise false (bool).
    bool loadFromFile(const std::string& path, std::string* error);

    // what it does: Sets a deterministic random seed for testing.
    // what the inputs are: seed - seed value for the random number generator.
    // what the outputs are: No return value (void).
    void setSeedForTesting(unsigned int seed);

    // what it does: Switches the engine to the main menu page.
    // what the inputs are: None.
    // what the outputs are: No return value (void).
    void showMainMenu();

    // what it does: Resumes the current run from the saved resume page.
    // what the inputs are: None.
    // what the outputs are: No return value (void).
    void resumeRun();

    // what it does: Shows the help information screen.
    // what the inputs are: None.
    // what the outputs are: No return value (void).
    void showHelp();

    // what it does: Shows the achievement information screen.
    // what the inputs are: None.
    // what the outputs are: No return value (void).
    void showAchievements();

    // what it does: Shows leaderboard information loaded from a file.
    // what the inputs are: path - leaderboard file path.
    // what the outputs are: No return value (void).
    void showLeaderboard(const std::string& path);

    // what it does: Shows a custom information page.
    // what the inputs are: title - page title; lines - page body lines; footer - footer text.
    // what the outputs are: No return value (void).
    void showInfo(const std::string& title,
                  const std::vector<std::string>& lines,
                  const std::string& footer);

    // what it does: Selects a route node from the current map options.
    // what the inputs are: index - zero-based route option index.
    // what the outputs are: No return value (void).
    void chooseNode(size_t index);

    // what it does: Resolves a selected encounter choice.
    // what the inputs are: index - zero-based encounter choice index.
    // what the outputs are: No return value (void).
    void resolveEncounter(size_t index);

    // what it does: Checks whether a hand card can currently be played.
    // what the inputs are: index - zero-based hand card index.
    // what the outputs are: Returns true when the card is playable; otherwise false (bool).
    bool canPlayCard(size_t index) const;

    // what it does: Plays a card from the player's hand.
    // what the inputs are: index - zero-based hand card index.
    // what the outputs are: No return value (void).
    void playCard(size_t index);

    // what it does: Upgrades a specific card instance when the selected service allows it.
    // what the inputs are: cardRef - card instance reference; source - upgrade source; error - optional error message output.
    // what the outputs are: Returns true when the upgrade succeeds; otherwise false (bool).
    bool upgradeCard(const std::string& cardRef,
                     CardUpgradeSource source = CardUpgradeSource::Market,
                     std::string* error = nullptr);

    // what it does: Ends the player's current battle turn.
    // what the inputs are: None.
    // what the outputs are: No return value (void).
    void endTurn();

    // what it does: Applies the selected reward choice.
    // what the inputs are: index - zero-based reward option index.
    // what the outputs are: No return value (void).
    void chooseReward(int index);

    // what it does: Returns the engine to the main menu after recording run state.
    // what the inputs are: None.
    // what the outputs are: No return value (void).
    void returnToMenu();

    // what it does: Starts another run from the summary flow.
    // what the inputs are: None.
    // what the outputs are: No return value (void).
    void startAnotherRun();

    // what it does: Advances the active animation frame.
    // what the inputs are: None.
    // what the outputs are: No return value (void).
    void tickAnimation();

    // what it does: Finds a card definition by identifier.
    // what the inputs are: id - card identifier to look up.
    // what the outputs are: Returns the matching card definition pointer, or nullptr if absent (const CardDef*).
    const CardDef* cardById(const std::string& id) const;

    // what it does: Resolves a card reference into its effective card definition.
    // what the inputs are: cardRef - card identifier or upgraded instance reference.
    // what the outputs are: Returns the resolved card definition (CardDef).
    CardDef resolvedCard(const std::string& cardRef) const;

    // what it does: Builds the display name for a card reference.
    // what the inputs are: cardRef - card identifier or upgraded instance reference.
    // what the outputs are: Returns the display card name (std::string).
    std::string displayCardName(const std::string& cardRef) const;

    // what it does: Converts a card reference to its canonical stored form.
    // what the inputs are: cardRef - card identifier or instance reference.
    // what the outputs are: Returns the canonical card reference (std::string).
    std::string canonicalCardRef(const std::string& cardRef) const;

    // what it does: Reads the upgrade level encoded in a card reference.
    // what the inputs are: cardRef - card identifier or instance reference.
    // what the outputs are: Returns the upgrade level (int).
    int cardUpgradeLevel(const std::string& cardRef) const;

    // what it does: Calculates the scrap cost for a card upgrade source.
    // what the inputs are: source - upgrade service source.
    // what the outputs are: Returns the upgrade cost in scrap (int).
    int cardUpgradeCost(CardUpgradeSource source) const;

    // what it does: Builds card views for the current reward choices.
    // what the inputs are: None.
    // what the outputs are: Returns reward card views (std::vector<CardView>).
    std::vector<CardView> rewardCards() const;

    // what it does: Builds card views for the player's hand.
    // what the inputs are: None.
    // what the outputs are: Returns hand card views (std::vector<CardView>).
    std::vector<CardView> handCards() const;

    // what it does: Builds card views for the player's draw pile.
    // what the inputs are: None.
    // what the outputs are: Returns draw pile card views (std::vector<CardView>).
    std::vector<CardView> drawPileCards() const;

    // what it does: Builds card views for the player's discard pile.
    // what the inputs are: None.
    // what the outputs are: Returns discard pile card views (std::vector<CardView>).
    std::vector<CardView> discardPileCards() const;

    // what it does: Generates random card choices from a specific rarity pool.
    // what the inputs are: count - number of card ids to choose; rarity - rarity pool to sample.
    // what the outputs are: Returns selected card identifiers (std::vector<std::string>).
    std::vector<std::string> randomCardChoicesByRarity(size_t count, CardRarity rarity);

    // what it does: Groups all owned cards by card identity for collection display.
    // what the inputs are: None.
    // what the outputs are: Returns owned-card entries (std::vector<CardOwnershipEntry>).
    std::vector<CardOwnershipEntry> ownedCards() const;

    // what it does: Builds card views for every owned card instance.
    // what the inputs are: None.
    // what the outputs are: Returns owned card instances (std::vector<CardView>).
    std::vector<CardView> ownedCardInstances() const;

    // what it does: Builds the full card library with ownership flags.
    // what the inputs are: None.
    // what the outputs are: Returns card library entries (std::vector<CardLibraryEntry>).
    std::vector<CardLibraryEntry> allCardLibraryEntries() const;

    // what it does: Builds event achievement entries with completion flags.
    // what the inputs are: None.
    // what the outputs are: Returns event achievement entries (std::vector<AchievementEntry>).
    std::vector<AchievementEntry> allEventAchievementEntries() const;

    // what it does: Builds enemy discovery achievement entries with completion flags.
    // what the inputs are: None.
    // what the outputs are: Returns enemy achievement entries (std::vector<AchievementEntry>).
    std::vector<AchievementEntry> allEnemyAchievementEntries() const;

    // what it does: Counts obtained cards in the card library.
    // what the inputs are: None.
    // what the outputs are: Returns the obtained card count (size_t).
    size_t obtainedCardCount() const;

    // what it does: Counts all cards available in the card library.
    // what the inputs are: None.
    // what the outputs are: Returns the total card count (size_t).
    size_t totalCardCount() const;

    // what it does: Counts completed event achievements.
    // what the inputs are: None.
    // what the outputs are: Returns completed event achievement count (size_t).
    size_t completedEventAchievementCount() const;

    // what it does: Counts all event achievements.
    // what the inputs are: None.
    // what the outputs are: Returns total event achievement count (size_t).
    size_t totalEventAchievementCount() const;

    // what it does: Counts enemies seen by the player.
    // what the inputs are: None.
    // what the outputs are: Returns seen enemy count (size_t).
    size_t seenEnemyCount() const;

    // what it does: Counts all enemy templates that can be discovered.
    // what the inputs are: None.
    // what the outputs are: Returns total enemy count (size_t).
    size_t totalEnemyCount() const;

    // what it does: Counts all completed achievements across tracked categories.
    // what the inputs are: None.
    // what the outputs are: Returns completed achievement count (size_t).
    size_t completedAchievementCount() const;

    // what it does: Reports whether auxiliary ship transformation selection is active.
    // what the inputs are: None.
    // what the outputs are: Returns true when ship selection is pending; otherwise false (bool).
    bool auxiliaryTransformSelectionPending() const;

    // what it does: Reports whether an auxiliary transformation target must be chosen.
    // what the inputs are: None.
    // what the outputs are: Returns true when target selection is pending; otherwise false (bool).
    bool auxiliaryTransformTargetChoicePending() const;

    // what it does: Reports the auxiliary ship currently selected for transformation.
    // what the inputs are: None.
    // what the outputs are: Returns the selected auxiliary ship index, or -1 when none (int).
    int auxiliaryTransformSelectedShipIndex() const;

    // what it does: Reports how many auxiliary transformations remain.
    // what the inputs are: None.
    // what the outputs are: Returns the remaining transformation count (int).
    int auxiliaryTransformRemainingSelections() const;

    // what it does: Lists target unit identifiers available for the current auxiliary transformation.
    // what the inputs are: None.
    // what the outputs are: Returns target unit identifiers (std::vector<std::string>).
    std::vector<std::string> auxiliaryTransformTargetUnitIds() const;

    // what it does: Selects an auxiliary ship to transform.
    // what the inputs are: index - zero-based auxiliary ship index.
    // what the outputs are: Returns true when selection succeeds; otherwise false (bool).
    bool selectAuxiliaryTransformShip(size_t index);

    // what it does: Selects a transformation target for the selected auxiliary ship.
    // what the inputs are: index - zero-based target option index.
    // what the outputs are: Returns true when selection succeeds; otherwise false (bool).
    bool selectAuxiliaryTransformTarget(size_t index);

    // what it does: Builds the display name for the current difficulty.
    // what the inputs are: None.
    // what the outputs are: Returns the difficulty name (std::string).
    std::string difficultyName() const;

    // what it does: Finds a ship display name by ship identifier.
    // what the inputs are: shipId - ship identifier to look up.
    // what the outputs are: Returns the ship name, or a fallback name (std::string).
    std::string shipNameForId(const std::string& shipId) const;

    // what it does: Finds a relic display name by relic identifier.
    // what the inputs are: relicId - relic identifier to look up.
    // what the outputs are: Returns the relic name, or a fallback name (std::string).
    std::string relicNameForId(const std::string& relicId) const;

    // what it does: Finds a relic description by relic identifier.
    // what the inputs are: relicId - relic identifier to look up.
    // what the outputs are: Returns the relic description, or an empty fallback (std::string).
    std::string relicDescriptionForId(const std::string& relicId) const;

    // ---------------------------------------------------------------
    // Developer mode hooks (additive only). These functions are used
    // exclusively by the optional developer overlay declared in
    // include/engine/game_dev.h. They MUST NOT be called from the
    // normal gameplay flow. Implementations live in
    // src/engine/game_dev.cpp so no other engine file is touched.
    // ---------------------------------------------------------------
    // what it does: Ensures a developer-mode run exists before applying debug actions.
    // what the inputs are: difficulty - difficulty to use when starting; shipId - ship identifier to use when starting.
    // what the outputs are: No return value (void).
    void dev_initRunIfNeeded(int difficulty, const std::string& shipId);

    // what it does: Sets the current run difficulty for developer testing.
    // what the inputs are: difficulty - new difficulty level.
    // what the outputs are: No return value (void).
    void dev_setDifficulty(int difficulty);

    // what it does: Sets the current player ship for developer testing.
    // what the inputs are: shipId - ship identifier to apply.
    // what the outputs are: No return value (void).
    void dev_setShip(const std::string& shipId);

    // what it does: Sets the player's current hull value for developer testing.
    // what the inputs are: hp - new hull value.
    // what the outputs are: No return value (void).
    void dev_setHp(int hp);

    // what it does: Sets the player's maximum hull value for developer testing.
    // what the inputs are: maxHp - new maximum hull value.
    // what the outputs are: No return value (void).
    void dev_setMaxHp(int maxHp);

    // what it does: Sets the player's current energy for developer testing.
    // what the inputs are: energy - new energy value.
    // what the outputs are: No return value (void).
    void dev_setEnergy(int energy);

    // what it does: Sets the player's scrap amount for developer testing.
    // what the inputs are: scrap - new scrap value.
    // what the outputs are: No return value (void).
    void dev_setScrap(int scrap);

    // what it does: Sets the player's heat value for developer testing.
    // what the inputs are: heat - new heat value.
    // what the outputs are: No return value (void).
    void dev_setHeat(int heat);

    // what it does: Sets the player's shield value for developer testing.
    // what the inputs are: shield - new shield value.
    // what the outputs are: No return value (void).
    void dev_setShield(int shield);

    // what it does: Adjusts the player's current hull by a delta.
    // what the inputs are: delta - signed hull change.
    // what the outputs are: No return value (void).
    void dev_addHp(int delta);

    // what it does: Adjusts the player's current energy by a delta.
    // what the inputs are: delta - signed energy change.
    // what the outputs are: No return value (void).
    void dev_addEnergy(int delta);

    // what it does: Adjusts the player's scrap by a delta.
    // what the inputs are: delta - signed scrap change.
    // what the outputs are: No return value (void).
    void dev_addScrap(int delta);

    // what it does: Adjusts the player's heat by a delta.
    // what the inputs are: delta - signed heat change.
    // what the outputs are: No return value (void).
    void dev_addHeat(int delta);

    // what it does: Adjusts the player's shield by a delta.
    // what the inputs are: delta - signed shield change.
    // what the outputs are: No return value (void).
    void dev_addShield(int delta);

    // what it does: Reloads the current encounter through developer routing.
    // what the inputs are: None.
    // what the outputs are: No return value (void).
    void dev_reloadCurrentEncounter();

    // what it does: Opens a debug scene directly by scene type and payload.
    // what the inputs are: sceneKey - debug scene category; payload - optional scene-specific payload.
    // what the outputs are: No return value (void).
    void dev_openDebugScene(const std::string& sceneKey,
                            const std::string& payload = "");

    // what it does: Ends a developer-mode session and restores saved collection state.
    // what the inputs are: None.
    // what the outputs are: No return value (void).
    void dev_shutdownSession();

    // what it does: Moves the run to a specific sector and map depth.
    // what the inputs are: sector - target sector number; depth - target route depth.
    // what the outputs are: No return value (void).
    void dev_jumpToSector(int sector, int depth);

    // what it does: Adds a card to the draw pile for developer testing.
    // what the inputs are: cardId - card identifier to add.
    // what the outputs are: No return value (void).
    void dev_addCardToDrawPile(const std::string& cardId);

    // what it does: Adds a card to the hand for developer testing.
    // what the inputs are: cardId - card identifier to add.
    // what the outputs are: No return value (void).
    void dev_addCardToHand(const std::string& cardId);

    // what it does: Attempts to play the top card in hand for developer testing.
    // what the inputs are: None.
    // what the outputs are: Returns true when a card was played; otherwise false (bool).
    bool dev_playTopOfHand();

    // what it does: Starts a battle directly against a selected enemy.
    // what the inputs are: nodeType - battle node type; enemyId - enemy template identifier.
    // what the outputs are: No return value (void).
    void dev_startBattleWith(const std::string& nodeType, const std::string& enemyId);

    // what it does: Resolves the current battle as a victory for developer testing.
    // what the inputs are: None.
    // what the outputs are: No return value (void).
    void dev_skipCurrentBattleAsVictory();

    // what it does: Forces a custom encounter state for developer testing.
    // what the inputs are: typeId - encounter type; encounterId - encounter identifier; title - encounter title; description - encounter text; choices - choice labels; details - choice detail lines.
    // what the outputs are: No return value (void).
    void dev_forceEncounter(const std::string& typeId,
                            const std::string& encounterId,
                            const std::string& title,
                            const std::string& description,
                            const std::vector<std::string>& choices,
                            const std::vector<std::string>& details);

    // what it does: Sets the escape route story stage for developer testing.
    // what the inputs are: stage - target escape stage.
    // what the outputs are: No return value (void).
    void dev_setEscapeStage(int stage);

    // what it does: Sets the truth route story stage for developer testing.
    // what the inputs are: stage - target truth stage; postBattle - whether to show the post-battle state.
    // what the outputs are: No return value (void).
    void dev_setTruthStage(int stage, bool postBattle);

    // what it does: Shows an ending summary directly for developer testing.
    // what the inputs are: kind - ending kind selector, where 0 is normal, 1 is escape, and 2 is truth.
    // what the outputs are: No return value (void).
    void dev_showEnding(int kind);  // 0 normal, 1 escape, 2 truth

    // what it does: Returns a developer-mode run to the map page.
    // what the inputs are: None.
    // what the outputs are: No return value (void).
    void dev_returnToMap();

    // what it does: Lists all card identifiers available to developer mode.
    // what the inputs are: None.
    // what the outputs are: Returns card identifiers (std::vector<std::string>).
    std::vector<std::string> dev_listCardIds() const;

    // what it does: Lists cards with display names for developer menus.
    // what the inputs are: None.
    // what the outputs are: Returns card identifier/name pairs (std::vector<std::pair<std::string, std::string>>).
    std::vector<std::pair<std::string, std::string>> dev_listCardsWithNames() const;

    // what it does: Lists enemies with display names for developer menus.
    // what the inputs are: None.
    // what the outputs are: Returns enemy identifier/name pairs (std::vector<std::pair<std::string, std::string>>).
    std::vector<std::pair<std::string, std::string>> dev_listEnemiesWithNames() const;

    // what it does: Lists ships with display names for developer menus.
    // what the inputs are: None.
    // what the outputs are: Returns ship identifier/name pairs (std::vector<std::pair<std::string, std::string>>).
    std::vector<std::pair<std::string, std::string>> dev_listShipsWithNames() const;

private:
    struct IntentDef {
        std::string kind;
        std::string label;
        int value = 0;
    };

    struct EnemyTemplate {
        std::string id;
        std::string name;
        std::string behavior;
        int baseHp = 0;
        bool elite = false;
        bool boss = false;
        std::vector<IntentDef> pattern;
        int maxHeat = 10;
        int minSector = 1;
        int maxSector = 3;
    };

    struct ShipTemplate {
        std::string id;
        std::string name;
        int maxHp = 40;
        int maxEnergy = 3;
        int maxHeat = 10;
        std::vector<std::string> extraCards;
        std::vector<std::string> relics;
    };

    struct RelicDef {
        std::string id;
        std::string name;
        std::string description;
    };

    RunState state_;
    AppPage resumePage_ = AppPage::Map;
    std::mt19937 rng_;
    std::vector<CardDef> cardPool_;
    std::vector<CardDef> configuredUpgradePool_;
    std::vector<std::string> obtainedCardIds_;
    std::vector<std::pair<std::string, std::string>> completedEventAchievements_;
    std::vector<std::string> seenEnemyIds_;
    bool dev_session_active_ = false;
    bool dev_obtained_snapshot_valid_ = false;
    std::vector<std::string> dev_obtained_snapshot_;
    std::vector<EnemyTemplate> enemyPool_;
    std::vector<ShipTemplate> ships_;
    std::vector<RelicDef> relicPool_;

    // what it does: Loads built-in content and persistent registries into the engine.
    // what the inputs are: None.
    // what the outputs are: No return value (void).
    void initializeData();

    // what it does: Resets the active run state to defaults.
    // what the inputs are: None.
    // what the outputs are: No return value (void).
    void resetRunState();

    // what it does: Applies ship stats, starter cards, and relics to the player.
    // what the inputs are: ship - selected ship template; difficulty - selected difficulty level.
    // what the outputs are: No return value (void).
    void setupPlayerForShip(const ShipTemplate& ship, int difficulty);

    // what it does: Grants a relic to the player if it exists and is not already owned.
    // what the inputs are: relicId - relic identifier to grant.
    // what the outputs are: No return value (void).
    void grantRelic(const std::string& relicId);

    // what it does: Checks whether the player owns a relic.
    // what the inputs are: relicId - relic identifier to check.
    // what the outputs are: Returns true when owned; otherwise false (bool).
    bool hasRelic(const std::string& relicId) const;

    // what it does: Finds the configured upgrade card for a base card id.
    // what the inputs are: cardId - base card identifier.
    // what the outputs are: Returns the upgrade card definition pointer, or nullptr if absent (const CardDef*).
    const CardDef* configuredUpgradeCardByBaseId(const std::string& cardId) const;

    // what it does: Converts legacy type-level upgrades into per-instance card references.
    // what the inputs are: None.
    // what the outputs are: No return value (void).
    void normalizeUpgradedCards();

    // what it does: Checks whether a card type can be upgraded.
    // what the inputs are: cardId - card identifier to check.
    // what the outputs are: Returns true when upgradeable; otherwise false (bool).
    bool cardTypeSupportsUpgrade(const std::string& cardId) const;

    // what it does: Builds a payload that identifies a card instance in a deck pile.
    // what the inputs are: pileId - deck pile identifier; index - zero-based pile index.
    // what the outputs are: Returns the encoded deck instance payload (std::string).
    std::string makeDeckInstancePayload(const std::string& pileId, size_t index) const;

    // what it does: Parses a deck instance payload into its pile and index parts.
    // what the inputs are: payload - encoded deck instance payload; pileId - output pile identifier; index - output pile index.
    // what the outputs are: Returns true when parsing succeeds; otherwise false (bool).
    bool parseDeckInstancePayload(const std::string& payload, std::string* pileId, size_t* index) const;

    // what it does: Replaces the card reference stored at a deck instance payload.
    // what the inputs are: payload - encoded deck instance payload; newCardRef - replacement card reference.
    // what the outputs are: Returns true when replacement succeeds; otherwise false (bool).
    bool replaceDeckInstanceCardRef(const std::string& payload, const std::string& newCardRef);

    // what it does: Builds the localized-style label for the player's current form.
    // what the inputs are: None.
    // what the outputs are: Returns the player form label (std::string).
    std::string playerFormLabel() const;

    // what it does: Extracts the base card id from a card reference.
    // what the inputs are: cardRef - card identifier or encoded reference.
    // what the outputs are: Returns the base card identifier (std::string).
    std::string baseCardId(const std::string& cardRef) const;

    // what it does: Builds a card reference for a card id and upgrade level.
    // what the inputs are: cardId - base card identifier; upgradeLevel - upgrade level to encode.
    // what the outputs are: Returns the encoded card reference (std::string).
    std::string makeCardRef(const std::string& cardId, int upgradeLevel) const;

    // what it does: Generates the next set of route options for the map.
    // what the inputs are: None.
    // what the outputs are: No return value (void).
    void generateRouteOptions();

    // what it does: Starts the encounter or battle associated with a route node.
    // what the inputs are: node - selected route node option.
    // what the outputs are: No return value (void).
    void startEncounterForNode(const NodeOption& node);

    // what it does: Starts a battle for a route node type and optional forced enemy.
    // what the inputs are: nodeType - battle node type; forcedEnemyId - optional enemy identifier.
    // what the outputs are: No return value (void).
    void startBattle(const std::string& nodeType, const std::string& forcedEnemyId = "");

    // what it does: Resets and shuffles the player's battle deck.
    // what the inputs are: None.
    // what the outputs are: No return value (void).
    void resetBattleDeck();

    // what it does: Starts a new player turn during battle.
    // what the inputs are: None.
    // what the outputs are: No return value (void).
    void startPlayerTurn();

    // what it does: Chooses the enemy's next intent for battle.
    // what the inputs are: None.
    // what the outputs are: No return value (void).
    void chooseEnemyIntent();

    // what it does: Resolves the enemy's current battle intent.
    // what the inputs are: None.
    // what the outputs are: No return value (void).
    void resolveEnemyIntent();

    // what it does: Applies victory rewards and progression after a battle win.
    // what the inputs are: None.
    // what the outputs are: No return value (void).
    void handleBattleVictory();

    // what it does: Applies defeat flow and summary state.
    // what the inputs are: None.
    // what the outputs are: No return value (void).
    void handleDefeat();

    // what it does: Completes the active route node and advances map progress.
    // what the inputs are: None.
    // what the outputs are: No return value (void).
    void finishCurrentNode();

    // what it does: Applies incoming damage to the player.
    // what the inputs are: amount - incoming damage amount.
    // what the outputs are: No return value (void).
    void applyDamageToPlayer(int amount);

    // what it does: Applies damage hits to the enemy.
    // what the inputs are: amount - damage per hit; ignoreShield - whether shield is bypassed; hits - number of hits.
    // what the outputs are: No return value (void).
    void applyDamageToEnemy(int amount, bool ignoreShield, int hits);

    // what it does: Draws cards from the draw pile into the player's hand.
    // what the inputs are: count - number of cards to draw.
    // what the outputs are: No return value (void).
    void drawCards(int count);

    // what it does: Moves discard pile cards back into the draw pile and shuffles them.
    // what the inputs are: None.
    // what the outputs are: No return value (void).
    void reshuffleDiscardIntoDraw();

    // what it does: Adjusts player heat and applies related overheat state.
    // what the inputs are: delta - signed heat change.
    // what the outputs are: No return value (void).
    void changeHeat(int delta);

    // what it does: Checks whether the player is currently overheated.
    // what the inputs are: None.
    // what the outputs are: Returns true when overheated; otherwise false (bool).
    bool playerIsOverheated() const;

    // what it does: Calculates the player's current overheat stage.
    // what the inputs are: None.
    // what the outputs are: Returns the overheat stage (int).
    int playerOverheatStage() const;

    // what it does: Synchronizes player overheat flags with current heat.
    // what the inputs are: None.
    // what the outputs are: No return value (void).
    void syncPlayerOverheatState();

    // what it does: Advances player overheat duration by one round.
    // what the inputs are: None.
    // what the outputs are: No return value (void).
    void advancePlayerOverheatRound();

    // what it does: Summons auxiliary ships from a summon card effect.
    // what the inputs are: card - card definition with summon data; baseCardId - optional base card identifier.
    // what the outputs are: No return value (void).
    void summonAuxiliaryShips(const CardDef& card, const std::string& baseCardId = "");

    // what it does: Resolves all auxiliary ship end-turn effects.
    // what the inputs are: None.
    // what the outputs are: No return value (void).
    void resolveAuxiliaryShipsEndTurn();

    // what it does: Resolves a single auxiliary ship trigger.
    // what the inputs are: shipIndex - zero-based auxiliary ship index; allowAura - whether aura bonuses apply; immediateContext - whether this is an immediate trigger.
    // what the outputs are: No return value (void).
    void resolveAuxiliaryShipTrigger(size_t shipIndex, bool allowAura, bool immediateContext);

    // what it does: Triggers auxiliary ships immediately a fixed number of times.
    // what the inputs are: times - number of trigger passes.
    // what the outputs are: No return value (void).
    void triggerAuxiliaryShipsNow(int times);

    // what it does: Grants rewards caused by destroying auxiliary ships.
    // what the inputs are: card - card definition that caused destruction.
    // what the outputs are: No return value (void).
    void resolveAuxiliaryShipDestroyRewards(const CardDef& card);

    // what it does: Resolves effects that occur when an auxiliary ship is removed.
    // what the inputs are: ship - removed auxiliary ship state; reason - removal reason text.
    // what the outputs are: No return value (void).
    void resolveAuxiliaryShipRemovalBurst(const AuxiliaryShipState& ship, const char* reason);

    // what it does: Handles blackbox protocol selection for a played hand card.
    // what the inputs are: handIndex - zero-based hand card index.
    // what the outputs are: Returns true when blackbox handling consumes play flow; otherwise false (bool).
    bool resolveBlackboxProtocol(size_t handIndex);

    // what it does: Resolves the relic trigger for the first form change in a battle.
    // what the inputs are: None.
    // what the outputs are: No return value (void).
    void resolveFirstFormChangeRelic();

    // what it does: Begins auxiliary ship transformation selection.
    // what the inputs are: card - card requesting transformation; transformCount - number of transformations to select.
    // what the outputs are: No return value (void).
    void beginAuxiliaryTransformSelection(const CardDef& card, int transformCount);

    // what it does: Resolves auxiliary ship selection for transformation.
    // what the inputs are: index - zero-based auxiliary ship index.
    // what the outputs are: Returns true when selection succeeds; otherwise false (bool).
    bool resolveAuxiliaryTransformSelection(size_t index);

    // what it does: Resolves target selection for an auxiliary transformation.
    // what the inputs are: index - zero-based target option index.
    // what the outputs are: Returns true when target selection succeeds; otherwise false (bool).
    bool resolveAuxiliaryTransformTargetSelection(size_t index);

    // what it does: Lists current target unit identifiers for auxiliary transformation.
    // what the inputs are: None.
    // what the outputs are: Returns target unit identifiers (std::vector<std::string>).
    std::vector<std::string> currentAuxiliaryTransformTargetUnitIds() const;

    // what it does: Clears auxiliary transformation selection state.
    // what the inputs are: None.
    // what the outputs are: No return value (void).
    void clearAuxiliaryTransformSelection();

    // what it does: Checks whether a card reference is a temporary copy reference.
    // what the inputs are: cardRef - card reference to inspect.
    // what the outputs are: Returns true when the reference is temporary; otherwise false (bool).
    bool isTemporaryCopyRef(const std::string& cardRef) const;

    // what it does: Builds a temporary copy reference for a card.
    // what the inputs are: cardRef - source card reference.
    // what the outputs are: Returns temporary copy reference text (std::string).
    std::string makeTemporaryCopyRef(const std::string& cardRef) const;

    // what it does: Removes temporary-copy wrapping from a card reference.
    // what the inputs are: cardRef - card reference to unwrap.
    // what the outputs are: Returns the original card reference (std::string).
    std::string unwrapTemporaryCopyRef(const std::string& cardRef) const;

    // what it does: Adds temporary blackbox card copies for the current turn.
    // what the inputs are: None.
    // what the outputs are: No return value (void).
    void addBlackboxTurnCopies();

    // what it does: Applies auxiliary ship buffs from a card effect.
    // what the inputs are: card - card definition containing buff values.
    // what the outputs are: No return value (void).
    void buffAuxiliaryShips(const CardDef& card);

    // what it does: Handles bookkeeping after an auxiliary ship is removed.
    // what the inputs are: ship - removed auxiliary ship state; reason - removal reason text.
    // what the outputs are: No return value (void).
    void onAuxiliaryShipRemoved(const AuxiliaryShipState& ship, const char* reason);

    // what it does: Applies a form shift caused by a card.
    // what the inputs are: card - card definition with form-shift data.
    // what the outputs are: No return value (void).
    void shiftPlayerForm(const CardDef& card);

    // what it does: Toggles the player's current form.
    // what the inputs are: None.
    // what the outputs are: No return value (void).
    void togglePlayerForm();

    // what it does: Removes destroyed auxiliary ships from the active list.
    // what the inputs are: None.
    // what the outputs are: No return value (void).
    void clearDestroyedAuxiliaryShips();

    // what it does: Adjusts enemy heat and synchronizes overheat state.
    // what the inputs are: delta - signed enemy heat change.
    // what the outputs are: No return value (void).
    void changeEnemyHeat(int delta);

    // what it does: Checks whether the enemy is currently overheated.
    // what the inputs are: None.
    // what the outputs are: Returns true when overheated; otherwise false (bool).
    bool enemyIsOverheated() const;

    // what it does: Calculates the enemy's current overheat stage.
    // what the inputs are: None.
    // what the outputs are: Returns the overheat stage (int).
    int enemyOverheatStage() const;

    // what it does: Calculates enemy attack damage after modifiers.
    // what the inputs are: None.
    // what the outputs are: Returns effective attack damage (int).
    int effectiveEnemyAttackDamage() const;

    // what it does: Checks whether the enemy should use cooling behavior.
    // what the inputs are: None.
    // what the outputs are: Returns true when cooling should be used; otherwise false (bool).
    bool enemyShouldUseCooling() const;

    // what it does: Calculates the enemy's cooling amount.
    // what the inputs are: None.
    // what the outputs are: Returns heat removed by cooling (int).
    int enemyCoolingAmount() const;

    // what it does: Applies the enemy cooling action.
    // what the inputs are: None.
    // what the outputs are: No return value (void).
    void triggerEnemyCooling();

    // what it does: Synchronizes enemy overheat flags with current heat.
    // what the inputs are: None.
    // what the outputs are: No return value (void).
    void syncEnemyOverheatState();

    // what it does: Advances enemy overheat duration by one round.
    // what the inputs are: None.
    // what the outputs are: No return value (void).
    void advanceEnemyOverheatRound();

    // what it does: Adds a line to the battle log.
    // what the inputs are: line - log message to append.
    // what the outputs are: No return value (void).
    void log(const std::string& line);

    // what it does: Checks whether a card has been recorded as obtained.
    // what the inputs are: cardId - card identifier to check.
    // what the outputs are: Returns true when obtained; otherwise false (bool).
    bool hasObtainedCard(const std::string& cardId) const;

    // what it does: Records a card as obtained in the library registry.
    // what the inputs are: cardRef - card reference to record.
    // what the outputs are: No return value (void).
    void recordObtainedCard(const std::string& cardRef);

    // what it does: Records all currently owned cards as obtained.
    // what the inputs are: None.
    // what the outputs are: No return value (void).
    void recordCurrentOwnedCards();

    // what it does: Queues one owned card reference into the discard pile.
    // what the inputs are: cardRef - card reference to queue.
    // what the outputs are: No return value (void).
    void queueOwnedCardToDiscard(const std::string& cardRef);

    // what it does: Queues multiple owned card references into the discard pile.
    // what the inputs are: cardRefs - card references to queue.
    // what the outputs are: No return value (void).
    void queueOwnedCardsToDiscard(const std::vector<std::string>& cardRefs);

    // what it does: Loads the obtained-card registry from persistent storage.
    // what the inputs are: None.
    // what the outputs are: No return value (void).
    void loadObtainedCardRegistry();

    // what it does: Saves the obtained-card registry to persistent storage.
    // what the inputs are: error - optional error message output.
    // what the outputs are: Returns true when saving succeeds; otherwise false (bool).
    bool saveObtainedCardRegistry(std::string* error = nullptr) const;

    // what it does: Checks whether an event achievement has been completed.
    // what the inputs are: encounterId - encounter identifier to check.
    // what the outputs are: Returns true when completed; otherwise false (bool).
    bool hasCompletedEventAchievement(const std::string& encounterId) const;

    // what it does: Records an event achievement as completed.
    // what the inputs are: encounterId - event encounter identifier; title - achievement title.
    // what the outputs are: No return value (void).
    void recordCompletedEventAchievement(const std::string& encounterId, const std::string& title);

    // what it does: Loads completed event achievements from persistent storage.
    // what the inputs are: None.
    // what the outputs are: No return value (void).
    void loadCompletedEventAchievementRegistry();

    // what it does: Saves completed event achievements to persistent storage.
    // what the inputs are: error - optional error message output.
    // what the outputs are: Returns true when saving succeeds; otherwise false (bool).
    bool saveCompletedEventAchievementRegistry(std::string* error = nullptr) const;

    // what it does: Checks whether an enemy has been seen by the player.
    // what the inputs are: enemyId - enemy identifier to check.
    // what the outputs are: Returns true when seen; otherwise false (bool).
    bool hasSeenEnemy(const std::string& enemyId) const;

    // what it does: Records an enemy as seen by the player.
    // what the inputs are: enemyId - enemy identifier to record.
    // what the outputs are: No return value (void).
    void recordSeenEnemy(const std::string& enemyId);

    // what it does: Loads the seen-enemy registry from persistent storage.
    // what the inputs are: None.
    // what the outputs are: No return value (void).
    void loadSeenEnemyRegistry();

    // what it does: Saves the seen-enemy registry to persistent storage.
    // what the inputs are: error - optional error message output.
    // what the outputs are: Returns true when saving succeeds; otherwise false (bool).
    bool saveSeenEnemyRegistry(std::string* error = nullptr) const;

    // what it does: Starts a UI animation event.
    // what the inputs are: kind - animation category; label - animation label; magnitude - displayed numeric magnitude; playerToEnemy - animation direction flag; duration - animation duration in ticks.
    // what the outputs are: No return value (void).
    void startAnimation(const std::string& kind,
                        const std::string& label,
                        int magnitude,
                        bool playerToEnemy,
                        int duration);

    // what it does: Generates random card choices from the full card pool.
    // what the inputs are: count - number of card ids to choose.
    // what the outputs are: Returns selected card identifiers (std::vector<std::string>).
    std::vector<std::string> randomCardChoices(size_t count);

    // what it does: Generates random battle reward card choices for a node type.
    // what the inputs are: count - number of card ids to choose; nodeType - active battle node type.
    // what the outputs are: Returns selected card identifiers (std::vector<std::string>).
    std::vector<std::string> randomBattleCardChoices(size_t count, const std::string& nodeType);

    // what it does: Generates random market card choices.
    // what the inputs are: count - number of card ids to choose.
    // what the outputs are: Returns selected card identifiers (std::vector<std::string>).
    std::vector<std::string> randomMarketCardChoices(size_t count);

    // what it does: Chooses a random relic not already owned when possible.
    // what the inputs are: None.
    // what the outputs are: Returns the selected relic identifier (std::string).
    std::string randomRelic();

    // what it does: Selects an enemy template for a battle node.
    // what the inputs are: nodeType - battle node type; forcedEnemyId - optional enemy identifier override.
    // what the outputs are: Returns the chosen enemy template (const EnemyTemplate&).
    const EnemyTemplate& pickEnemy(const std::string& nodeType, const std::string& forcedEnemyId = "");

    // what it does: Finds an enemy template by identifier.
    // what the inputs are: id - enemy identifier to look up.
    // what the outputs are: Returns the enemy template pointer, or nullptr if absent (const EnemyTemplate*).
    const EnemyTemplate* enemyTemplateById(const std::string& id) const;

    // what it does: Finds a ship template by identifier.
    // what the inputs are: id - ship identifier to look up.
    // what the outputs are: Returns the ship template pointer, or nullptr if absent (const ShipTemplate*).
    const ShipTemplate* shipTemplateById(const std::string& id) const;

    // what it does: Finds a relic definition by identifier.
    // what the inputs are: id - relic identifier to look up.
    // what the outputs are: Returns the relic definition pointer, or nullptr if absent (const RelicDef*).
    const RelicDef* relicById(const std::string& id) const;

    // what it does: Fills the help information page state.
    // what the inputs are: None.
    // what the outputs are: No return value (void).
    void fillHelpInfo();

    // what it does: Fills the run summary state after victory or defeat.
    // what the inputs are: victory - whether the run ended in victory.
    // what the outputs are: No return value (void).
    void fillSummary(bool victory);

    // what it does: Opens the repair station encounter service screen.
    // what the inputs are: None.
    // what the outputs are: No return value (void).
    void openRepairStation();

    // what it does: Opens the market shop encounter service screen.
    // what the inputs are: None.
    // what the outputs are: No return value (void).
    void openMarketShop();

    // what it does: Clears market and service selection state.
    // what the inputs are: None.
    // what the outputs are: No return value (void).
    void clearMarketState();

    // what it does: Resolves a selected card purge option.
    // what the inputs are: index - zero-based purge option index.
    // what the outputs are: Returns true when purge succeeds; otherwise false (bool).
    bool resolvePurgeSelection(size_t index);

    // what it does: Resolves a selected card upgrade option.
    // what the inputs are: index - zero-based upgrade option index.
    // what the outputs are: Returns true when upgrade succeeds; otherwise false (bool).
    bool resolveUpgradeSelection(size_t index);

    // what it does: Enters the market purge service at a list offset.
    // what the inputs are: offset - starting option offset.
    // what the outputs are: No return value (void).
    void enterMarketPurge(size_t offset);

    // what it does: Enters the market upgrade service at a list offset.
    // what the inputs are: offset - starting option offset.
    // what the outputs are: No return value (void).
    void enterMarketUpgrade(size_t offset);

    // what it does: Enters the repair purge service at a list offset.
    // what the inputs are: offset - starting option offset.
    // what the outputs are: No return value (void).
    void enterRepairPurge(size_t offset);

    // what it does: Enters the repair upgrade service at a list offset.
    // what the inputs are: offset - starting option offset.
    // what the outputs are: No return value (void).
    void enterRepairUpgrade(size_t offset);

    // what it does: Enters the event purge reward service at a list offset.
    // what the inputs are: offset - starting option offset.
    // what the outputs are: No return value (void).
    void enterEventPurge(size_t offset);

    // what it does: Enters the event upgrade reward service at a list offset.
    // what the inputs are: offset - starting option offset.
    // what the outputs are: No return value (void).
    void enterEventUpgrade(size_t offset);

    // what it does: Lists card instance references that can be upgraded.
    // what the inputs are: None.
    // what the outputs are: Returns upgradeable card references (std::vector<std::string>).
    std::vector<std::string> upgradeableCardInstanceRefs() const;

    // what it does: Lists card ids and counts available for purging.
    // what the inputs are: None.
    // what the outputs are: Returns purgeable card/count pairs (std::vector<std::pair<std::string, int>>).
    std::vector<std::pair<std::string, int>> purgeableCards() const;

    // what it does: Lists card ids and counts available for upgrading.
    // what the inputs are: None.
    // what the outputs are: Returns upgradeable card/count pairs (std::vector<std::pair<std::string, int>>).
    std::vector<std::pair<std::string, int>> upgradeableCards() const;

    // what it does: Removes one copy of a card from the player's deck.
    // what the inputs are: cardId - card identifier to purge.
    // what the outputs are: Returns true when a card was removed; otherwise false (bool).
    bool purgeCardFromDeck(const std::string& cardId);

    // what it does: Upgrades a random eligible card from the player's deck.
    // what the inputs are: source - upgrade source; upgradedCardName - optional upgraded card name output; error - optional error message output.
    // what the outputs are: Returns true when an upgrade succeeds; otherwise false (bool).
    bool upgradeRandomCard(CardUpgradeSource source, std::string* upgradedCardName, std::string* error);
};

#endif
