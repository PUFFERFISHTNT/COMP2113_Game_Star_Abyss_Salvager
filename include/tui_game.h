#ifndef TUI_GAME_H
#define TUI_GAME_H

#include <random>
#include <string>
#include <utility>
#include <vector>

inline constexpr int kBattleDrawCount = 5;
inline constexpr int kBattleHandLimit = 10;
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
    int openingEnergyBonus = 0;
    int openingShieldBonus = 0;
    int nextAttackMultiplier = 1;
    int salvageBonus = 0;
    int attacksPlayedThisTurn = 0;
    int form = 0;
    int auxEndTurnExtraTriggersThisTurn = 0;
    int auxEffectPercentBonusThisTurn = 0;
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
    bool firstSummonUsedThisBattle = false;
    bool firstSummonUsedThisTurn = false;
    bool firstEnterHighHeatTriggeredThisBattle = false;
    bool firstEnterOverloadDrawTriggeredThisBattle = false;
    bool firstExitDefenseEnergyTriggeredThisBattle = false;
    bool firstSummonEnemyHeatTriggeredThisTurn = false;
    bool enteredOverloadThisTurn = false;
    bool exitedDefenseThisTurn = false;
    bool exitedFormThisTurn = false;
    bool blackboxSelectionPending = false;
    int pendingBlackboxCopyCount = 0;
    std::string blackboxImprintRef;
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
    int remainingTurns = 0;
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
    int patternIndex = 0;
    int weakenTurns = 0;
    int shieldLockTurns = 0;
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
    GameEngine();

    const RunState& state() const;

    void startNewRun(int difficulty, const std::string& shipId);
    bool saveToFile(const std::string& path, std::string* error) const;
    bool loadFromFile(const std::string& path, std::string* error);
    void setSeedForTesting(unsigned int seed);

    void showMainMenu();
    void resumeRun();
    void showHelp();
    void showLeaderboard(const std::string& path);
    void showInfo(const std::string& title,
                  const std::vector<std::string>& lines,
                  const std::string& footer);

    void chooseNode(size_t index);
    void resolveEncounter(size_t index);
    bool canPlayCard(size_t index) const;
    void playCard(size_t index);
    bool upgradeCard(const std::string& cardRef,
                     CardUpgradeSource source = CardUpgradeSource::Market,
                     std::string* error = nullptr);
    void endTurn();
    void chooseReward(int index);
    void returnToMenu();
    void startAnotherRun();
    void tickAnimation();

    const CardDef* cardById(const std::string& id) const;
    CardDef resolvedCard(const std::string& cardRef) const;
    std::string displayCardName(const std::string& cardRef) const;
    std::string canonicalCardRef(const std::string& cardRef) const;
    int cardUpgradeLevel(const std::string& cardRef) const;
    int cardUpgradeCost(CardUpgradeSource source) const;
    std::vector<CardView> rewardCards() const;
    std::vector<CardView> handCards() const;
    std::vector<CardView> drawPileCards() const;
    std::vector<CardView> discardPileCards() const;
    std::vector<CardOwnershipEntry> ownedCards() const;
    std::vector<CardView> ownedCardInstances() const;
    std::vector<CardLibraryEntry> allCardLibraryEntries() const;
    size_t obtainedCardCount() const;
    size_t totalCardCount() const;

    std::string difficultyName() const;
    std::string shipNameForId(const std::string& shipId) const;
    std::string relicNameForId(const std::string& relicId) const;
    std::string relicDescriptionForId(const std::string& relicId) const;

    // ---------------------------------------------------------------
    // Developer mode hooks (additive only). These functions are used
    // exclusively by the optional developer overlay declared in
    // include/engine/game_dev.h. They MUST NOT be called from the
    // normal gameplay flow. Implementations live in
    // src/engine/game_dev.cpp so no other engine file is touched.
    // ---------------------------------------------------------------
    void dev_initRunIfNeeded(int difficulty, const std::string& shipId);
    void dev_setDifficulty(int difficulty);
    void dev_setShip(const std::string& shipId);
    void dev_setHp(int hp);
    void dev_setMaxHp(int maxHp);
    void dev_setEnergy(int energy);
    void dev_setScrap(int scrap);
    void dev_setHeat(int heat);
    void dev_setShield(int shield);
    void dev_addHp(int delta);
    void dev_addEnergy(int delta);
    void dev_addScrap(int delta);
    void dev_addHeat(int delta);
    void dev_addShield(int delta);
    void dev_reloadCurrentEncounter();
    void dev_openDebugScene(const std::string& sceneKey,
                            const std::string& payload = "");
    void dev_shutdownSession();
    void dev_jumpToSector(int sector, int depth);
    void dev_addCardToDrawPile(const std::string& cardId);
    void dev_addCardToHand(const std::string& cardId);
    bool dev_playTopOfHand();
    void dev_startBattleWith(const std::string& nodeType, const std::string& enemyId);
    void dev_skipCurrentBattleAsVictory();
    void dev_forceEncounter(const std::string& typeId,
                            const std::string& encounterId,
                            const std::string& title,
                            const std::string& description,
                            const std::vector<std::string>& choices,
                            const std::vector<std::string>& details);
    void dev_setEscapeStage(int stage);
    void dev_setTruthStage(int stage, bool postBattle);
    void dev_showEnding(int kind);  // 0 normal, 1 escape, 2 truth
    void dev_returnToMap();
    std::vector<std::string> dev_listCardIds() const;
    std::vector<std::pair<std::string, std::string>> dev_listCardsWithNames() const;
    std::vector<std::pair<std::string, std::string>> dev_listEnemiesWithNames() const;
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
    bool dev_session_active_ = false;
    bool dev_obtained_snapshot_valid_ = false;
    std::vector<std::string> dev_obtained_snapshot_;
    std::vector<EnemyTemplate> enemyPool_;
    std::vector<ShipTemplate> ships_;
    std::vector<RelicDef> relicPool_;

    void initializeData();
    void resetRunState();
    void setupPlayerForShip(const ShipTemplate& ship, int difficulty);
    void grantRelic(const std::string& relicId);
    bool hasRelic(const std::string& relicId) const;
    const CardDef* configuredUpgradeCardByBaseId(const std::string& cardId) const;
    void normalizeUpgradedCards();
    bool cardTypeSupportsUpgrade(const std::string& cardId) const;
    std::string makeDeckInstancePayload(const std::string& pileId, size_t index) const;
    bool parseDeckInstancePayload(const std::string& payload, std::string* pileId, size_t* index) const;
    bool replaceDeckInstanceCardRef(const std::string& payload, const std::string& newCardRef);
    std::string playerFormLabel() const;
    std::string baseCardId(const std::string& cardRef) const;
    std::string makeCardRef(const std::string& cardId, int upgradeLevel) const;

    void generateRouteOptions();
    void startEncounterForNode(const NodeOption& node);
    void startBattle(const std::string& nodeType, const std::string& forcedEnemyId = "");
    void resetBattleDeck();
    void startPlayerTurn();
    void chooseEnemyIntent();
    void resolveEnemyIntent();
    void handleBattleVictory();
    void handleDefeat();
    void finishCurrentNode();

    void applyDamageToPlayer(int amount);
    void applyDamageToEnemy(int amount, bool ignoreShield, int hits);
    void drawCards(int count);
    void reshuffleDiscardIntoDraw();
    void changeHeat(int delta);
    bool playerIsOverheated() const;
    int playerOverheatStage() const;
    void syncPlayerOverheatState();
    void advancePlayerOverheatRound();
    void summonAuxiliaryShips(const CardDef& card);
    void resolveAuxiliaryShipsEndTurn();
    void triggerAuxiliaryShipsNow(int times);
    void resolveAuxiliaryShipDestroyRewards(const CardDef& card);
    bool resolveBlackboxProtocol(size_t handIndex);
    bool isTemporaryCopyRef(const std::string& cardRef) const;
    std::string makeTemporaryCopyRef(const std::string& cardRef) const;
    std::string unwrapTemporaryCopyRef(const std::string& cardRef) const;
    void addBlackboxTurnCopies();
    void buffAuxiliaryShips(const CardDef& card);
    void onAuxiliaryShipRemoved(const AuxiliaryShipState& ship, const char* reason);
    void shiftPlayerForm(const CardDef& card);
    void togglePlayerForm();
    void clearDestroyedAuxiliaryShips();
    void changeEnemyHeat(int delta);
    bool enemyIsOverheated() const;
    int enemyOverheatStage() const;
    int effectiveEnemyAttackDamage() const;
    bool enemyShouldUseCooling() const;
    int enemyCoolingAmount() const;
    void triggerEnemyCooling();
    void syncEnemyOverheatState();
    void advanceEnemyOverheatRound();
    void log(const std::string& line);
    bool hasObtainedCard(const std::string& cardId) const;
    void recordObtainedCard(const std::string& cardRef);
    void recordCurrentOwnedCards();
    void queueOwnedCardToDiscard(const std::string& cardRef);
    void queueOwnedCardsToDiscard(const std::vector<std::string>& cardRefs);
    void loadObtainedCardRegistry();
    bool saveObtainedCardRegistry(std::string* error = nullptr) const;
    void startAnimation(const std::string& kind,
                        const std::string& label,
                        int magnitude,
                        bool playerToEnemy,
                        int duration);

    std::vector<std::string> randomCardChoices(size_t count);
    std::vector<std::string> randomMarketCardChoices(size_t count);
    std::string randomRelic();
    const EnemyTemplate& pickEnemy(const std::string& nodeType, const std::string& forcedEnemyId = "");
    const EnemyTemplate* enemyTemplateById(const std::string& id) const;
    const ShipTemplate* shipTemplateById(const std::string& id) const;
    const RelicDef* relicById(const std::string& id) const;

    void fillHelpInfo();
    void fillSummary(bool victory);
    void openMarketShop();
    void clearMarketState();
    bool resolvePurgeSelection(size_t index);
    bool resolveUpgradeSelection(size_t index);
    void enterMarketPurge(size_t offset);
    void enterMarketUpgrade(size_t offset);
    void enterRepairUpgrade(size_t offset);
    void enterEventPurge(size_t offset);
    void enterEventUpgrade(size_t offset);
    std::vector<std::string> upgradeableCardInstanceRefs() const;
    std::vector<std::pair<std::string, int>> purgeableCards() const;
    std::vector<std::pair<std::string, int>> upgradeableCards() const;
    bool purgeCardFromDeck(const std::string& cardId);
    bool upgradeRandomCard(CardUpgradeSource source, std::string* upgradedCardName, std::string* error);
};

#endif
