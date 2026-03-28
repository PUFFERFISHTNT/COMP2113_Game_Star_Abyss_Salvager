#ifndef TUI_GAME_H
#define TUI_GAME_H

#include <random>
#include <string>
#include <utility>
#include <vector>

inline constexpr int kBattleDrawCount = 5;
inline constexpr int kBattleHandLimit = 10;

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

struct CardDef {
    std::string id;
    std::string name;
    std::string type;
    std::string description;
    int cost = 0;
    int damage = 0;
    int shield = 0;
    int heal = 0;
    int draw = 0;
    int energy = 0;
    int nextTurnEnergy = 0;
    int heat = 0;
    int selfDamage = 0;
    int multiHit = 1;
    int comboBonus = 0;
    int attackMultiplier = 1;
    int weakenTurns = 0;
    int removeEnemyShield = 0;
    int shieldLockTurns = 0;
    int salvageBonus = 0;
    bool ignoreShield = false;
    bool shieldToDamage = false;
    bool halveHeat = false;
    bool clearHeat = false;
    bool randomEffect = false;
};

struct NodeOption {
    std::string typeId;
    std::string name;
    std::string detail;
    int danger = 1;
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
    int scrap = 0;
    int nextTurnEnergy = 0;
    int openingEnergyBonus = 0;
    int openingShieldBonus = 0;
    int nextAttackMultiplier = 1;
    int salvageBonus = 0;
    int attacksPlayedThisTurn = 0;
    bool emergencyBatteryUsed = false;
    std::vector<std::string> relics;
    std::vector<std::string> drawPile;
    std::vector<std::string> hand;
    std::vector<std::string> discardPile;
};

struct EnemyState {
    std::string enemyId;
    std::string name;
    std::string behavior;
    int hp = 0;
    int maxHp = 0;
    int shield = 0;
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
    int nodesPerSector = 3;
    int battleTurn = 0;
    bool firstTurnOfBattle = true;
    bool pendingSectorAdvance = false;
    bool pendingVictory = false;
    std::string activeNodeType;
    std::string activeNodeName;
    std::string statusLine;
    PlayerState player;
    EnemyState enemy;
    std::vector<NodeOption> routeOptions;
    EncounterState encounter;
    RewardState reward;
    InfoState info;
    AnimationState animation;
    std::vector<std::string> battleLog;
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
    void endTurn();
    void chooseReward(int index);
    void returnToMenu();
    void startAnotherRun();
    void tickAnimation();

    const CardDef* cardById(const std::string& id) const;
    std::vector<const CardDef*> rewardCards() const;
    std::vector<const CardDef*> handCards() const;

    std::string difficultyName() const;
    std::string shipNameForId(const std::string& shipId) const;
    std::string relicNameForId(const std::string& relicId) const;
    std::string relicDescriptionForId(const std::string& relicId) const;

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
    std::vector<EnemyTemplate> enemyPool_;
    std::vector<ShipTemplate> ships_;
    std::vector<RelicDef> relicPool_;

    void initializeData();
    void resetRunState();
    void setupPlayerForShip(const ShipTemplate& ship, int difficulty);
    void grantRelic(const std::string& relicId);
    bool hasRelic(const std::string& relicId) const;

    void generateRouteOptions();
    void startEncounterForNode(const NodeOption& node);
    void startBattle(const std::string& nodeType);
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
    void log(const std::string& line);
    void startAnimation(const std::string& kind,
                        const std::string& label,
                        int magnitude,
                        bool playerToEnemy,
                        int duration);

    std::vector<std::string> randomCardChoices(size_t count);
    std::string randomRelic();
    const EnemyTemplate& pickEnemy(const std::string& nodeType);
    const EnemyTemplate* enemyTemplateById(const std::string& id) const;
    const ShipTemplate* shipTemplateById(const std::string& id) const;
    const RelicDef* relicById(const std::string& id) const;

    void fillHelpInfo();
    void fillSummary(bool victory);
    void enterMarketPurge(size_t offset);
    std::vector<std::pair<std::string, int>> purgeableCards() const;
    bool purgeCardFromDeck(const std::string& cardId);
};

#endif
