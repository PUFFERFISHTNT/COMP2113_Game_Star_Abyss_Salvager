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
    // Creates the engine, loads content and registries, and leaves it ready at the menu.
    GameEngine();

    // Gives read-only access to the current run state and returns that state reference.
    const RunState& state() const;

    // Starts a run from the selected difficulty and ship id and returns nothing.
    void startNewRun(int difficulty, const std::string& shipId);

    // Writes the current run to the given path, fills error when needed, and returns whether saving worked.
    bool saveToFile(const std::string& path, std::string* error) const;

    // Loads a run from the given path, fills error when needed, and returns whether loading worked.
    bool loadFromFile(const std::string& path, std::string* error);

    // Seeds the random generator with the given value for tests and returns nothing.
    void setSeedForTesting(unsigned int seed);

    // Moves the engine to the main menu without taking arguments and returns nothing.
    void showMainMenu();

    // Resumes the active run from its stored page without input and returns nothing.
    void resumeRun();

    // Opens the help screen without input and returns nothing.
    void showHelp();

    // Opens the achievements screen without input and returns nothing.
    void showAchievements();

    // Loads leaderboard text from the given path and shows it without returning a value.
    void showLeaderboard(const std::string& path);

    // Shows an information page from the given title, lines, and footer and returns nothing.
    void showInfo(const std::string& title,
                  const std::vector<std::string>& lines,
                  const std::string& footer);

    // Chooses the route option at index and starts its node without returning a value.
    void chooseNode(size_t index);

    // Resolves the encounter choice at index and returns nothing.
    void resolveEncounter(size_t index);

    // Checks the hand card at index and returns whether it can be played now.
    bool canPlayCard(size_t index) const;

    // Plays the hand card at index, resolving its effects, and returns nothing.
    void playCard(size_t index);

    // Upgrades the referenced card through the given source, fills error when needed, and returns success.
    bool upgradeCard(const std::string& cardRef,
                     CardUpgradeSource source = CardUpgradeSource::Market,
                     std::string* error = nullptr);

    // Ends the player's current battle turn without input and returns nothing.
    void endTurn();

    // Applies the reward choice at index, or a skip when negative, and returns nothing.
    void chooseReward(int index);

    // Returns to the main menu without input and returns nothing.
    void returnToMenu();

    // Moves from summary toward another setup flow without input and returns nothing.
    void startAnotherRun();

    // Advances animation state by one tick without input and returns nothing.
    void tickAnimation();

    // Looks up a card by id or reference and returns the matching definition pointer when found.
    const CardDef* cardById(const std::string& id) const;

    // Resolves a card reference into its effective card definition and returns that value.
    CardDef resolvedCard(const std::string& cardRef) const;

    // Builds a display name from a card reference and returns the text.
    std::string displayCardName(const std::string& cardRef) const;

    // Normalizes a card reference and returns its canonical stored form.
    std::string canonicalCardRef(const std::string& cardRef) const;

    // Reads the upgrade level from a card reference and returns the numeric level.
    int cardUpgradeLevel(const std::string& cardRef) const;

    // Computes the upgrade cost for the given source and returns the scrap amount.
    int cardUpgradeCost(CardUpgradeSource source) const;

    // Builds card views for current reward choices and returns them.
    std::vector<CardView> rewardCards() const;

    // Builds card views for the player's hand and returns them.
    std::vector<CardView> handCards() const;

    // Builds card views for the draw pile and returns them.
    std::vector<CardView> drawPileCards() const;

    // Builds card views for the discard pile and returns them.
    std::vector<CardView> discardPileCards() const;

    // Samples cards from the requested rarity and returns the chosen ids.
    std::vector<std::string> randomCardChoicesByRarity(size_t count, CardRarity rarity);

    // Groups all currently owned cards and returns the ownership entries.
    std::vector<CardOwnershipEntry> ownedCards() const;

    // Lists owned card instances separately and returns their card views.
    std::vector<CardView> ownedCardInstances() const;

    // Builds the full card library with obtained flags and returns the entries.
    std::vector<CardLibraryEntry> allCardLibraryEntries() const;

    // Builds event achievement entries with completion flags and returns them.
    std::vector<AchievementEntry> allEventAchievementEntries() const;

    // Builds enemy discovery achievement entries with completion flags and returns them.
    std::vector<AchievementEntry> allEnemyAchievementEntries() const;

    // Counts obtained cards without input and returns the count.
    size_t obtainedCardCount() const;

    // Counts all known cards without input and returns the count.
    size_t totalCardCount() const;

    // Counts completed event achievements without input and returns the count.
    size_t completedEventAchievementCount() const;

    // Counts all event achievements without input and returns the count.
    size_t totalEventAchievementCount() const;

    // Counts seen enemies without input and returns the count.
    size_t seenEnemyCount() const;

    // Counts all enemy templates without input and returns the count.
    size_t totalEnemyCount() const;

    // Counts completed achievements across categories without input and returns the count.
    size_t completedAchievementCount() const;

    // Reports whether auxiliary ship selection is waiting and returns that state.
    bool auxiliaryTransformSelectionPending() const;

    // Reports whether an auxiliary transform target is waiting and returns that state.
    bool auxiliaryTransformTargetChoicePending() const;

    // Returns the selected auxiliary ship index, or minus one when none is selected.
    int auxiliaryTransformSelectedShipIndex() const;

    // Returns how many auxiliary transform selections remain.
    int auxiliaryTransformRemainingSelections() const;

    // Lists valid auxiliary transform target unit ids and returns them.
    std::vector<std::string> auxiliaryTransformTargetUnitIds() const;

    // Selects an auxiliary ship by index and returns whether the selection succeeded.
    bool selectAuxiliaryTransformShip(size_t index);

    // Selects an auxiliary transform target by index and returns whether it succeeded.
    bool selectAuxiliaryTransformTarget(size_t index);

    // Builds the current difficulty name without input and returns it.
    std::string difficultyName() const;

    // Looks up a ship id and returns its display name or a fallback.
    std::string shipNameForId(const std::string& shipId) const;

    // Looks up a relic id and returns its display name or a fallback.
    std::string relicNameForId(const std::string& relicId) const;

    // Looks up a relic id and returns its description or empty text.
    std::string relicDescriptionForId(const std::string& relicId) const;

    // ---------------------------------------------------------------
    // Developer mode hooks (additive only). These functions are used
    // exclusively by the optional developer overlay declared in
    // include/engine/game_dev.h. They MUST NOT be called from the
    // normal gameplay flow. Implementations live in
    // src/engine/game_dev.cpp so no other engine file is touched.
    // ---------------------------------------------------------------
    // Ensures a developer run exists for the given difficulty and ship and returns nothing.
    void dev_initRunIfNeeded(int difficulty, const std::string& shipId);

    // Sets the developer difficulty value and returns nothing.
    void dev_setDifficulty(int difficulty);

    // Rebuilds the developer run around the given ship id and returns nothing.
    void dev_setShip(const std::string& shipId);

    // Sets player hull for developer mode and returns nothing.
    void dev_setHp(int hp);

    // Sets player maximum hull for developer mode and returns nothing.
    void dev_setMaxHp(int maxHp);

    // Sets player energy for developer mode and returns nothing.
    void dev_setEnergy(int energy);

    // Sets player scrap for developer mode and returns nothing.
    void dev_setScrap(int scrap);

    // Sets player heat for developer mode and returns nothing.
    void dev_setHeat(int heat);

    // Sets player shield for developer mode and returns nothing.
    void dev_setShield(int shield);

    // Adds the given delta to player hull in developer mode and returns nothing.
    void dev_addHp(int delta);

    // Adds the given delta to player energy in developer mode and returns nothing.
    void dev_addEnergy(int delta);

    // Adds the given delta to player scrap in developer mode and returns nothing.
    void dev_addScrap(int delta);

    // Adds the given delta to player heat in developer mode and returns nothing.
    void dev_addHeat(int delta);

    // Adds the given delta to player shield in developer mode and returns nothing.
    void dev_addShield(int delta);

    // Reloads the current developer encounter without input and returns nothing.
    void dev_reloadCurrentEncounter();

    // Opens a developer scene from the scene key and payload and returns nothing.
    void dev_openDebugScene(const std::string& sceneKey,
                            const std::string& payload = "");

    // Ends the developer session without input and returns nothing.
    void dev_shutdownSession();

    // Jumps to the given sector and depth for developer mode and returns nothing.
    void dev_jumpToSector(int sector, int depth);

    // Adds the chosen card id to the draw pile and returns nothing.
    void dev_addCardToDrawPile(const std::string& cardId);

    // Adds the chosen card id to the hand when possible and returns nothing.
    void dev_addCardToHand(const std::string& cardId);

    // Plays the top developer hand card without input and returns whether a play was attempted.
    bool dev_playTopOfHand();

    // Starts a battle from the given node type and enemy id and returns nothing.
    void dev_startBattleWith(const std::string& nodeType, const std::string& enemyId);

    // Forces the current battle to victory without input and returns nothing.
    void dev_skipCurrentBattleAsVictory();

    // Builds a developer encounter from the given text and choice data and returns nothing.
    void dev_forceEncounter(const std::string& typeId,
                            const std::string& encounterId,
                            const std::string& title,
                            const std::string& description,
                            const std::vector<std::string>& choices,
                            const std::vector<std::string>& details);

    // Moves the escape story to the requested stage and returns nothing.
    void dev_setEscapeStage(int stage);

    // Moves the truth story to the requested stage and post-battle state and returns nothing.
    void dev_setTruthStage(int stage, bool postBattle);

    // Shows the requested developer ending kind and returns nothing.
    void dev_showEnding(int kind);  // 0 normal, 1 escape, 2 truth

    // Sends developer mode back to the map without input and returns nothing.
    void dev_returnToMap();

    // Lists all card ids for developer controls and returns them.
    std::vector<std::string> dev_listCardIds() const;

    // Lists card ids with display names for developer controls and returns the pairs.
    std::vector<std::pair<std::string, std::string>> dev_listCardsWithNames() const;

    // Lists enemy ids with display names for developer controls and returns the pairs.
    std::vector<std::pair<std::string, std::string>> dev_listEnemiesWithNames() const;

    // Lists ship ids with display names for developer controls and returns the pairs.
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

    // Loads built-in content and registries into the engine and returns nothing.
    void initializeData();

    // Resets the current run state to defaults and returns nothing.
    void resetRunState();

    // Applies the chosen ship and difficulty to the player state and returns nothing.
    void setupPlayerForShip(const ShipTemplate& ship, int difficulty);

    // Grants the named relic when available and returns nothing.
    void grantRelic(const std::string& relicId);

    // Checks the named relic against the player state and returns whether it is owned.
    bool hasRelic(const std::string& relicId) const;

    // Finds configured upgrade data for a base card id and returns the definition pointer when present.
    const CardDef* configuredUpgradeCardByBaseId(const std::string& cardId) const;

    // Migrates legacy upgrade markers into card instance references and returns nothing.
    void normalizeUpgradedCards();

    // Checks a card id for upgrade support and returns whether it can be upgraded.
    bool cardTypeSupportsUpgrade(const std::string& cardId) const;

    // Encodes a pile id and index into a deck instance payload and returns it.
    std::string makeDeckInstancePayload(const std::string& pileId, size_t index) const;

    // Parses a deck instance payload into optional outputs and returns whether it was valid.
    bool parseDeckInstancePayload(const std::string& payload, std::string* pileId, size_t* index) const;

    // Replaces the card referenced by a deck instance payload and returns whether replacement worked.
    bool replaceDeckInstanceCardRef(const std::string& payload, const std::string& newCardRef);

    // Builds the current player form label without input and returns it.
    std::string playerFormLabel() const;

    // Extracts the base card id from the given reference and returns it.
    std::string baseCardId(const std::string& cardRef) const;

    // Combines a card id and upgrade level into a stored reference and returns it.
    std::string makeCardRef(const std::string& cardId, int upgradeLevel) const;

    // Generates route options for the current story state and returns nothing.
    void generateRouteOptions();

    // Starts the battle or encounter for the given route node and returns nothing.
    void startEncounterForNode(const NodeOption& node);

    // Starts a battle from node type and optional enemy override and returns nothing.
    void startBattle(const std::string& nodeType, const std::string& forcedEnemyId = "");

    // Rebuilds the battle deck from current piles and returns nothing.
    void resetBattleDeck();

    // Starts a player turn by restoring resources and drawing cards, then returns nothing.
    void startPlayerTurn();

    // Chooses enemy intent for the coming turn and returns nothing.
    void chooseEnemyIntent();

    // Resolves the current enemy intent and returns nothing.
    void resolveEnemyIntent();

    // Applies battle victory rewards and progression and returns nothing.
    void handleBattleVictory();

    // Applies defeat cleanup and summary state and returns nothing.
    void handleDefeat();

    // Finishes the active route node and returns nothing.
    void finishCurrentNode();

    // Applies the given damage amount to the player and returns nothing.
    void applyDamageToPlayer(int amount);

    // Applies repeated damage to the enemy with shield handling and returns nothing.
    void applyDamageToEnemy(int amount, bool ignoreShield, int hits);

    // Draws the requested number of cards into hand and returns nothing.
    void drawCards(int count);

    // Moves discard cards back into draw and shuffles, returning nothing.
    void reshuffleDiscardIntoDraw();

    // Changes player heat by the given delta and returns nothing.
    void changeHeat(int delta);

    // Checks current player heat and returns whether the player is overheated.
    bool playerIsOverheated() const;

    // Reads current player heat state and returns the overheat stage.
    int playerOverheatStage() const;

    // Syncs player overheat counters from heat state and returns nothing.
    void syncPlayerOverheatState();

    // Advances player overheat severity by one round and returns nothing.
    void advancePlayerOverheatRound();

    // Summons auxiliary ships from a card and optional base id and returns nothing.
    void summonAuxiliaryShips(const CardDef& card, const std::string& baseCardId = "");

    // Resolves end-turn auxiliary ship effects without input and returns nothing.
    void resolveAuxiliaryShipsEndTurn();

    // Resolves one auxiliary ship trigger from its index and context flags and returns nothing.
    void resolveAuxiliaryShipTrigger(size_t shipIndex, bool allowAura, bool immediateContext);

    // Triggers auxiliary ships the given number of times and returns nothing.
    void triggerAuxiliaryShipsNow(int times);

    // Applies rewards from destroying auxiliary ships with the given card and returns nothing.
    void resolveAuxiliaryShipDestroyRewards(const CardDef& card);

    // Applies removal burst effects from the removed ship and reason and returns nothing.
    void resolveAuxiliaryShipRemovalBurst(const AuxiliaryShipState& ship, const char* reason);

    // Handles Blackbox Protocol selection for the hand index and returns whether it consumed play.
    bool resolveBlackboxProtocol(size_t handIndex);

    // Resolves the first form-change relic trigger without input and returns nothing.
    void resolveFirstFormChangeRelic();

    // Starts auxiliary transform selection from the card and count and returns nothing.
    void beginAuxiliaryTransformSelection(const CardDef& card, int transformCount);

    // Chooses an auxiliary ship by index for transformation and returns whether it worked.
    bool resolveAuxiliaryTransformSelection(size_t index);

    // Chooses a transform target by index and returns whether it worked.
    bool resolveAuxiliaryTransformTargetSelection(size_t index);

    // Builds current auxiliary transform target ids without input and returns them.
    std::vector<std::string> currentAuxiliaryTransformTargetUnitIds() const;

    // Clears auxiliary transform selection state without input and returns nothing.
    void clearAuxiliaryTransformSelection();

    // Checks a card reference and returns whether it names a temporary copy.
    bool isTemporaryCopyRef(const std::string& cardRef) const;

    // Wraps a card reference as a temporary copy and returns the new reference.
    std::string makeTemporaryCopyRef(const std::string& cardRef) const;

    // Removes temporary copy wrapping from a card reference and returns the base reference.
    std::string unwrapTemporaryCopyRef(const std::string& cardRef) const;

    // Adds Blackbox temporary copies for the turn without input and returns nothing.
    void addBlackboxTurnCopies();

    // Applies auxiliary buffs from the given card and returns nothing.
    void buffAuxiliaryShips(const CardDef& card);

    // Handles bookkeeping for a removed auxiliary ship and reason and returns nothing.
    void onAuxiliaryShipRemoved(const AuxiliaryShipState& ship, const char* reason);

    // Shifts player form according to the given card and returns nothing.
    void shiftPlayerForm(const CardDef& card);

    // Toggles player form without input and returns nothing.
    void togglePlayerForm();

    // Removes destroyed auxiliary ships without input and returns nothing.
    void clearDestroyedAuxiliaryShips();

    // Changes enemy heat by the given delta and returns nothing.
    void changeEnemyHeat(int delta);

    // Checks current enemy heat and returns whether the enemy is overheated.
    bool enemyIsOverheated() const;

    // Reads current enemy heat state and returns the overheat stage.
    int enemyOverheatStage() const;

    // Computes enemy attack damage from current modifiers and returns it.
    int effectiveEnemyAttackDamage() const;

    // Checks the enemy heat state and returns whether cooling should be used.
    bool enemyShouldUseCooling() const;

    // Computes enemy cooling from current sector and returns the amount.
    int enemyCoolingAmount() const;

    // Applies enemy cooling without input and returns nothing.
    void triggerEnemyCooling();

    // Syncs enemy overheat counters from heat state and returns nothing.
    void syncEnemyOverheatState();

    // Advances enemy overheat severity by one round and returns nothing.
    void advanceEnemyOverheatRound();

    // Appends the given line to the battle log and returns nothing.
    void log(const std::string& line);

    // Checks the obtained-card registry for a card id and returns whether it is recorded.
    bool hasObtainedCard(const std::string& cardId) const;

    // Records a card reference as obtained and returns nothing.
    void recordObtainedCard(const std::string& cardRef);

    // Records all currently owned cards as obtained and returns nothing.
    void recordCurrentOwnedCards();

    // Adds one owned card reference to the discard pile and returns nothing.
    void queueOwnedCardToDiscard(const std::string& cardRef);

    // Adds several owned card references to the discard pile and returns nothing.
    void queueOwnedCardsToDiscard(const std::vector<std::string>& cardRefs);

    // Loads obtained-card registry data without input and returns nothing.
    void loadObtainedCardRegistry();

    // Saves obtained-card registry data, fills error when needed, and returns success.
    bool saveObtainedCardRegistry(std::string* error = nullptr) const;

    // Checks the event achievement registry and returns whether the encounter is complete.
    bool hasCompletedEventAchievement(const std::string& encounterId) const;

    // Records a completed event achievement from id and title and returns nothing.
    void recordCompletedEventAchievement(const std::string& encounterId, const std::string& title);

    // Loads completed event achievement data without input and returns nothing.
    void loadCompletedEventAchievementRegistry();

    // Saves completed event achievement data, fills error when needed, and returns success.
    bool saveCompletedEventAchievementRegistry(std::string* error = nullptr) const;

    // Checks the seen-enemy registry for an id and returns whether it is recorded.
    bool hasSeenEnemy(const std::string& enemyId) const;

    // Records an enemy id as seen and returns nothing.
    void recordSeenEnemy(const std::string& enemyId);

    // Loads seen-enemy registry data without input and returns nothing.
    void loadSeenEnemyRegistry();

    // Saves seen-enemy registry data, fills error when needed, and returns success.
    bool saveSeenEnemyRegistry(std::string* error = nullptr) const;

    // Starts an animation from its kind, label, magnitude, direction, and duration and returns nothing.
    void startAnimation(const std::string& kind,
                        const std::string& label,
                        int magnitude,
                        bool playerToEnemy,
                        int duration);

    // Samples general card rewards by count and returns the chosen ids.
    std::vector<std::string> randomCardChoices(size_t count);

    // Samples battle reward cards from count and node type and returns the chosen ids.
    std::vector<std::string> randomBattleCardChoices(size_t count, const std::string& nodeType);

    // Samples market cards by count and returns the chosen ids.
    std::vector<std::string> randomMarketCardChoices(size_t count);

    // Chooses an unowned relic without input and returns its id.
    std::string randomRelic();

    // Chooses an enemy for the node type and optional forced id and returns the template.
    const EnemyTemplate& pickEnemy(const std::string& nodeType, const std::string& forcedEnemyId = "");

    // Looks up an enemy id and returns the template pointer when present.
    const EnemyTemplate* enemyTemplateById(const std::string& id) const;

    // Looks up a ship id and returns the template pointer when present.
    const ShipTemplate* shipTemplateById(const std::string& id) const;

    // Looks up a relic id and returns the definition pointer when present.
    const RelicDef* relicById(const std::string& id) const;

    // Fills help page state without input and returns nothing.
    void fillHelpInfo();

    // Fills summary state from the victory flag and returns nothing.
    void fillSummary(bool victory);

    // Opens the repair station service without input and returns nothing.
    void openRepairStation();

    // Opens the market service without input and returns nothing.
    void openMarketShop();

    // Clears market service state without input and returns nothing.
    void clearMarketState();

    // Resolves the purge option at index and returns whether it was handled.
    bool resolvePurgeSelection(size_t index);

    // Resolves the upgrade option at index and returns whether it was handled.
    bool resolveUpgradeSelection(size_t index);

    // Opens market purge selection at the given offset and returns nothing.
    void enterMarketPurge(size_t offset);

    // Opens market upgrade selection at the given offset and returns nothing.
    void enterMarketUpgrade(size_t offset);

    // Opens repair purge selection at the given offset and returns nothing.
    void enterRepairPurge(size_t offset);

    // Opens repair upgrade selection at the given offset and returns nothing.
    void enterRepairUpgrade(size_t offset);

    // Opens event purge selection at the given offset and returns nothing.
    void enterEventPurge(size_t offset);

    // Opens event upgrade selection at the given offset and returns nothing.
    void enterEventUpgrade(size_t offset);

    // Lists upgradeable card instance references without input and returns them.
    std::vector<std::string> upgradeableCardInstanceRefs() const;

    // Lists purgeable card ids with counts without input and returns them.
    std::vector<std::pair<std::string, int>> purgeableCards() const;

    // Lists upgradeable card ids with counts without input and returns them.
    std::vector<std::pair<std::string, int>> upgradeableCards() const;

    // Removes one card id from the deck and returns whether a card was removed.
    bool purgeCardFromDeck(const std::string& cardId);

    // Upgrades a random eligible card, fills outputs when supplied, and returns success.
    bool upgradeRandomCard(CardUpgradeSource source, std::string* upgradedCardName, std::string* error);
};

#endif
