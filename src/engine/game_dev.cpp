// =============================================================================
// game_dev.cpp
//
// Developer mode (debug overlay).
//
// Implements the engine ``dev_*`` hooks declared in tui_game.h and the
// FTXUI overlay declared in include/engine/game_dev.h. Two separate
// component trees are produced:
//
//   * The password / options modal  ->  DevModeUi::component()
//   * The persistent debug overlay  ->  DevModeUi::overlay()
//
// The persistent overlay renders a floating "DEV" button at the bottom-left
// corner of the screen once the password has been entered correctly. Click
// (or press the keyboard shortcut) to open a self-contained debug panel
// that allows live HP/Shield/Energy/Heat/Credits adjustment, free card
// library access, and event reload. The panel is independent of the
// existing modal stack (cards / save dialog / dev options) and remains
// available across map / encounter / battle / reward pages.
//
// All UI strings are routed through Tr(language_, en, zh) or
// TranslateGameText for strict English/Chinese separation.
// =============================================================================

#include "engine/game_dev.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>
#include <ftxui/component/component_options.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>
#include <utility>

#include "ui/ui_panels.h"

extern int g_sa_beacon_activated;
extern int g_sa_escape_stage;
extern int g_sa_truth_stage;
extern bool g_sa_truth_post_battle;
extern int g_sa_skip_segments;
extern bool g_sa_next_elite;
extern bool g_sa_heat_accel;
extern bool g_sa_no_boss;
extern int g_sa_permanent_shield;
extern bool g_sa_escape_battle_pending;
extern bool g_sa_escape_intro_pending;
extern bool g_sa_game_intro_pending;
extern std::string g_sa_event_epilogue;

// =============================================================================
// Engine-side ``dev_*`` hook implementations.
// =============================================================================

void GameEngine::dev_initRunIfNeeded(int difficulty, const std::string& shipId) {
    if (!dev_session_active_) {
        dev_obtained_snapshot_ = obtainedCardIds_;
        dev_obtained_snapshot_valid_ = true;
        dev_session_active_ = true;
    }
    if (!state_.hasRun) {
        startNewRun(difficulty, shipId);
        g_sa_game_intro_pending = false;
        state_.encounter = {};
        state_.page = AppPage::Map;
        resumePage_ = AppPage::Map;
    }
}

void GameEngine::dev_setDifficulty(int difficulty) {
    state_.difficulty = difficulty;
}

void GameEngine::dev_setShip(const std::string& shipId) {
    const ShipTemplate* ship = shipTemplateById(shipId);
    if (ship == nullptr) {
        return;
    }
    setupPlayerForShip(*ship, state_.difficulty);
}

void GameEngine::dev_setHp(int hp) {
    if (hp < 0) hp = 0;
    if (hp > state_.player.maxHp) state_.player.maxHp = hp;
    state_.player.hp = hp;
}

void GameEngine::dev_setMaxHp(int maxHp) {
    if (maxHp < 1) maxHp = 1;
    state_.player.maxHp = maxHp;
    if (state_.player.hp > maxHp) state_.player.hp = maxHp;
}

void GameEngine::dev_setEnergy(int energy) {
    if (energy < 0) energy = 0;
    state_.player.energy = energy;
    if (state_.player.maxEnergy < energy) state_.player.maxEnergy = energy;
}

void GameEngine::dev_setScrap(int scrap) {
    if (scrap < 0) scrap = 0;
    state_.player.scrap = scrap;
}

void GameEngine::dev_setHeat(int heat) {
    if (heat < 0) heat = 0;
    if (heat > state_.player.maxHeat) heat = state_.player.maxHeat;
    state_.player.heat = heat;
    syncPlayerOverheatState();
}

void GameEngine::dev_setShield(int shield) {
    if (shield < 0) shield = 0;
    state_.player.shield = shield;
}

void GameEngine::dev_addHp(int delta) {
    dev_setHp(state_.player.hp + delta);
}

void GameEngine::dev_addEnergy(int delta) {
    dev_setEnergy(state_.player.energy + delta);
}

void GameEngine::dev_addScrap(int delta) {
    dev_setScrap(state_.player.scrap + delta);
}

void GameEngine::dev_addHeat(int delta) {
    dev_setHeat(state_.player.heat + delta);
}

void GameEngine::dev_addShield(int delta) {
    dev_setShield(state_.player.shield + delta);
}

void GameEngine::dev_shutdownSession() {
    if (dev_obtained_snapshot_valid_) {
        obtainedCardIds_ = dev_obtained_snapshot_;
        saveObtainedCardRegistry(nullptr);
    }
    dev_obtained_snapshot_.clear();
    dev_obtained_snapshot_valid_ = false;
    dev_session_active_ = false;

    resetRunState();
    clearMarketState();

    g_sa_beacon_activated = 0;
    g_sa_escape_stage = 0;
    g_sa_truth_stage = 0;
    g_sa_truth_post_battle = false;
    g_sa_skip_segments = 0;
    g_sa_next_elite = false;
    g_sa_heat_accel = false;
    g_sa_no_boss = false;
    g_sa_permanent_shield = 0;
    g_sa_escape_battle_pending = false;
    g_sa_escape_intro_pending = true;
    g_sa_game_intro_pending = true;
    g_sa_event_epilogue.clear();
}

void GameEngine::dev_jumpToSector(int sector, int depth) {
    if (sector < 1) sector = 1;
    if (sector > 3) sector = 3;
    if (depth < 0) depth = 0;
    if (depth >= state_.nodesPerSector) depth = state_.nodesPerSector - 1;
    state_.sector = sector;
    state_.depth = depth;
    state_.encounter = {};
    state_.reward = {};
    state_.activeNodeType.clear();
    state_.activeNodeName.clear();
    generateRouteOptions();
    state_.page = AppPage::Map;
    resumePage_ = AppPage::Map;
    state_.statusLine = "Dev jump applied.";
}

void GameEngine::dev_addCardToDrawPile(const std::string& cardId) {
    if (cardById(cardId) == nullptr) return;
    state_.player.drawPile.push_back(makeCardRef(cardId, 0));
    recordObtainedCard(cardId);
}

void GameEngine::dev_addCardToHand(const std::string& cardId) {
    if (cardById(cardId) == nullptr) return;
    if (static_cast<int>(state_.player.hand.size()) >= kBattleHandLimit) {
        state_.player.discardPile.push_back(makeCardRef(cardId, 0));
    } else {
        state_.player.hand.push_back(makeCardRef(cardId, 0));
    }
    recordObtainedCard(cardId);
}

bool GameEngine::dev_playTopOfHand() {
    if (state_.page != AppPage::Battle || state_.player.hand.empty()) {
        return false;
    }
    const size_t lastIndex = state_.player.hand.size() - 1;
    if (!canPlayCard(lastIndex)) {
        state_.player.energy = std::max(state_.player.energy, 9);
    }
    playCard(lastIndex);
    return true;
}

void GameEngine::dev_startBattleWith(const std::string& nodeType, const std::string& enemyId) {
    state_.encounter = {};
    state_.reward = {};
    startBattle(nodeType, enemyId);
}

void GameEngine::dev_skipCurrentBattleAsVictory() {
    if (state_.page != AppPage::Battle) {
        return;
    }
    state_.enemy.hp = 0;
    handleBattleVictory();
}

void GameEngine::dev_forceEncounter(const std::string& typeId,
                                    const std::string& encounterId,
                                    const std::string& title,
                                    const std::string& description,
                                    const std::vector<std::string>& choices,
                                    const std::vector<std::string>& details) {
    state_.encounter = {};
    state_.encounter.typeId = typeId;
    state_.encounter.encounterId = encounterId;
    state_.encounter.title = title;
    state_.encounter.description = description;
    state_.encounter.choices = choices;
    state_.encounter.details = details;
    state_.activeNodeType = typeId;
    state_.page = AppPage::Encounter;
    resumePage_ = AppPage::Encounter;
}

void GameEngine::dev_setEscapeStage(int stage) {
    if (stage < 1) stage = 1;
    if (stage > 8) stage = 8;
    g_sa_beacon_activated = 1;
    g_sa_escape_stage = stage;
    g_sa_escape_intro_pending = (stage == 1);
    g_sa_truth_stage = 0;
    g_sa_truth_post_battle = false;

    NodeOption node{"event", "Escape", "Dev jump", 1, ""};
    startEncounterForNode(node);
}

void GameEngine::dev_setTruthStage(int stage, bool postBattle) {
    if (stage < 1) stage = 1;
    if (stage > 6) stage = 6;
    g_sa_beacon_activated = 1;
    g_sa_truth_stage = stage;
    g_sa_truth_post_battle = postBattle;
    g_sa_escape_stage = 0;
    g_sa_escape_intro_pending = false;

    if (postBattle && stage <= 5) {
        NodeOption node{"event", "Truth", "Dev jump", 1, ""};
        startEncounterForNode(node);
    } else {
        state_.activeNodeType = "elite";
        state_.statusLine = "Dev: truth stage advanced.";
        state_.page = AppPage::Map;
        resumePage_ = AppPage::Map;
        generateRouteOptions();
    }
}

void GameEngine::dev_showEnding(int kind) {
    fillSummary(true);
    if (kind == 1) {
        state_.info.lines.push_back("");
        state_.info.lines.push_back("You kept the Ark's call silenced.");
        state_.info.lines.push_back("No deeper descent, no revelations, no final stand.");
        state_.info.lines.push_back("Your prow pierced the mist of the Star Abyss,");
        state_.info.lines.push_back("and for the first time, the light of distant stars fell clearly upon the deck.");
        state_.info.lines.push_back("");
        state_.info.lines.push_back("You survived.");
        state_.info.lines.push_back("The secrets of the Star Abyss remain buried,");
        state_.info.lines.push_back("and you are among the very few salvagers to walk away whole.");
    } else if (kind == 2) {
        state_.info.lines.push_back("");
        state_.info.lines.push_back("You stand at the origin of everything.");
        state_.info.lines.push_back("");
        state_.info.lines.push_back("Here there are no wrecks, no enemies, no pirates or federations.");
        state_.info.lines.push_back("Before you is not a ship's core, nor the heart of a star sector,");
        state_.info.lines.push_back("but a universe that was just born and fell immediately into slumber.");
        state_.info.lines.push_back("");
        state_.info.lines.push_back("The Ark fleet carried humanity's last hope for continuity,");
        state_.info.lines.push_back("attempting to pry open an entirely new habitable spacetime.");
        state_.info.lines.push_back("They ignited the singularity but could not control its expansion.");
        state_.info.lines.push_back("The runaway energy tore apart the fleet, shredded space itself,");
        state_.info.lines.push_back("and turned the ground into a chaotic, ever-expanding death zone --");
        state_.info.lines.push_back("what would come to be known as the Star Abyss.");
        state_.info.lines.push_back("");
        state_.info.lines.push_back("And you -- a debt-ridden salvager who entered the dead zone to survive --");
        state_.info.lines.push_back("became the only person to reach the singularity and touch the origin.");
    } else {
        state_.info.lines.push_back("");
        state_.info.lines.push_back("You destroyed the guardian units of the Ark Core,");
        state_.info.lines.push_back("and the turmoil of the Star Abyss gradually subsided.");
        state_.info.lines.push_back("");
        state_.info.lines.push_back("You survived the Star Abyss, but never touched its truth.");
    }
}

void GameEngine::dev_returnToMap() {
    state_.encounter = {};
    state_.reward = {};
    state_.activeNodeType.clear();
    state_.activeNodeName.clear();
    generateRouteOptions();
    state_.page = AppPage::Map;
    resumePage_ = AppPage::Map;
    state_.statusLine = "Dev: returned to map.";
}

std::vector<std::string> GameEngine::dev_listCardIds() const {
    std::vector<std::string> ids;
    ids.reserve(cardPool_.size());
    for (const CardDef& def : cardPool_) {
        ids.push_back(def.id);
    }
    return ids;
}

std::vector<std::pair<std::string, std::string>>
GameEngine::dev_listCardsWithNames() const {
    std::vector<std::pair<std::string, std::string>> out;
    out.reserve(cardPool_.size());
    for (const CardDef& def : cardPool_) {
        out.emplace_back(def.id, def.name);
    }
    return out;
}

std::vector<std::pair<std::string, std::string>>
GameEngine::dev_listEnemiesWithNames() const {
    std::vector<std::pair<std::string, std::string>> out;
    out.reserve(enemyPool_.size());
    for (const EnemyTemplate& tpl : enemyPool_) {
        out.emplace_back(tpl.id, tpl.name);
    }
    return out;
}

std::vector<std::pair<std::string, std::string>>
GameEngine::dev_listShipsWithNames() const {
    std::vector<std::pair<std::string, std::string>> out;
    out.reserve(ships_.size());
    for (const ShipTemplate& sh : ships_) {
        out.emplace_back(sh.id, sh.name);
    }
    return out;
}

// =============================================================================
// Static encounter catalogue (used both for "Trigger Encounter" and the
// in-battle "Reload Event" feature). Each entry carries fully-written
// English and Chinese variants so events can be re-loaded with their full
// description text even after partial state mutation elsewhere.
// =============================================================================

namespace {

struct EncounterEntry {
    const char* labelEn;
    const char* labelZh;
    const char* typeId;
    const char* encounterId;
    const char* titleEn;
    const char* titleZh;
    const char* descriptionEn;
    const char* descriptionZh;
    std::array<const char*, 4> choicesEn;
    std::array<const char*, 4> choicesZh;
    std::array<const char*, 4> detailsEn;
    std::array<const char*, 4> detailsZh;
};

struct SceneEntry {
    const char* labelEn;
    const char* labelZh;
    const char* sceneKey;
    const char* payload;
};

bool IsSelectableMapScene(const SceneEntry& entry) {
    if (entry.sceneKey == nullptr) {
        return false;
    }
    const std::string sceneKey = entry.sceneKey;
    const std::string payload = entry.payload == nullptr ? "" : entry.payload;
    if (sceneKey == "service") {
        return false;
    }
    if (sceneKey == "route" && payload == "game_intro") {
        return false;
    }
    return true;
}

std::vector<std::string> CopyEncounterText(const std::array<const char*, 4>& values) {
    std::vector<std::string> out;
    for (const char* value : values) {
        if (value == nullptr) {
            break;
        }
        out.emplace_back(value);
    }
    return out;
}

void ForceStaticEncounter(GameEngine& engine, const EncounterEntry& entry) {
    engine.dev_forceEncounter(entry.typeId,
                              entry.encounterId == nullptr ? "" : entry.encounterId,
                              entry.titleEn,
                              entry.descriptionEn,
                              CopyEncounterText(entry.choicesEn),
                              CopyEncounterText(entry.detailsEn));
}

const std::vector<EncounterEntry>& EncounterCatalogue() {
    static const std::vector<EncounterEntry> kEntries = {
        {"A1 Distress Beacon", "A1 求救信标", "event", "event_a1",
         "Distress Beacon", "求救信标",
         "A faint distress signal pulses from a wrecked fleet. Someone may still be alive.",
         "残破舰队中传出微弱的求救信号，或许还有幸存者。",
         {"Go to rescue", "Relay coordinates", "Ignore and leave", nullptr},
         {"前往救援", "转发坐标", "忽略离开", nullptr},
         {"Gain 10 HP and 12 credits.", "Reduce 3 heat and gain 1 random card.", "Continue without delay.", nullptr},
         {"获得 10 生命与 12 金币。", "降低 3 热量并获得 1 张随机卡。", "继续航行。", nullptr}},
        {"A2 Ghost Ship", "A2 幽灵舰", "event", "event_a2",
         "Ghost Ship", "幽灵舰",
         "A derelict warship drifts silently. Its hull is intact but shows no life signs.",
         "一艘废弃战舰静静漂浮，外壳完好却毫无生命迹象。",
         {"Board and explore", "Send a hail", "Open fire", nullptr},
         {"登舰探查", "发出呼叫", "开火攻击", nullptr},
         {"Lose 5 HP but gain 1 rare relic.", "Gain 8 HP and 2 heat.", "Forced into a small battle.", nullptr},
         {"失去 5 生命但获得 1 件稀有遗物。", "获得 8 生命，并增加 2 热量。", "强制进入小型战斗。", nullptr}},
        {"B1 Strange Signal", "B1 诡异信号", "event", "event_b1",
         "Strange Signal", "诡异信号",
         "An anomalous frequency keeps syncing to your navigation core.",
         "一段异常频率持续同步到你的导航核心。",
         {"Trace the signal", "Block interference", "Record the spectrum", nullptr},
         {"追踪信号", "屏蔽干扰", "记录频谱", nullptr},
         {"Higher encounter chance next. Gain 1 random card.", "Reduce 4 heat.", "Gain 8 credits.", nullptr},
         {"下一段事件几率提高。获得 1 张随机卡。", "降低 4 热量。", "获得 8 金币。", nullptr}},
        {"B2 Space Fold", "B2 空间折叠", "event", "event_b2",
         "Space Fold", "空间折叠",
         "A spatial anomaly warps the region ahead.",
         "前方区域出现空间异常。",
         {"Enter the fold", "Stabilize the space", "Steer clear", nullptr},
         {"进入折叠", "稳定空间", "绕行回避", nullptr},
         {"Skip 1 segment, +5 heat, gain 1 rare card.", "-4 heat, +2 permanent shield, costs 10 credits.", "Next battle's first attack enhanced.", nullptr},
         {"跳过 1 段路程，+5 热量，获得 1 张稀有卡。", "降低 4 热量，永久 +2 护盾，花费 10 金币。", "下一场战斗充能攻击强化。", nullptr}},
        {"B3 Ark Beacon", "B3 方舟信标", "event", "event_b3",
         "Ark Beacon", "方舟信标",
         "A massive signal source emerges from the void. This is the Ark's beacon.",
         "虚空中浮现出庞大的信号源——方舟信标。",
         {"Activate the beacon", "Dismantle the beacon", "Block the beacon", nullptr},
         {"激活信标", "拆解信标", "封锁信标", nullptr},
         {"Gain rare card, +8 heat. Unlocks true ending.", "+25 credits, gain relic, -6 HP.", "Trigger escape route. No more bosses.", nullptr},
         {"获得 1 张传奇卡，+8 热量。开启真结局。", "+25 金币，获得 1 件遗物，-6 生命。", "触发逃亡路线，不再出现 Boss。", nullptr}},
        {"C1 Reactor Leak", "C1 反应堆泄漏", "event", "event_c1",
         "Reactor Leak", "反应堆泄漏",
         "Warning lights flash as unstable coolant leaks from the reactor.",
         "警示灯闪烁，反应堆冷却液不稳定泄漏。",
         {"Manual voltage stabilization", "Activate emergency cooling", "Silence the alarm", nullptr},
         {"手动稳压", "启动应急冷却", "关闭警报", nullptr},
         {"-3 HP, -4 heat.", "Costs 8 credits.", "Heat builds faster next segment.", nullptr},
         {"-3 生命，降低 4 热量。", "花费 8 金币。", "下一段热量加速积累。", nullptr}},
        {"C2 Hull Breach", "C2 舰体破损", "event", "event_c2",
         "Hull Breach", "舰体破损",
         "The ship's hull has been compromised.",
         "舰体出现破损，大气快速外泄。",
         {"Emergency welding", "Dump cargo to seal", "Leave it", nullptr},
         {"应急焊接", "倾倒货物封口", "置之不理", nullptr},
         {"-4 HP, +3 heat, costs 10 credits.", "Costs 8 credits.", "Lose 8 HP.", nullptr},
         {"-4 生命，+3 热量，花费 10 金币。", "花费 8 金币。", "失去 8 生命。", nullptr}},
        {"C3 Deck Chaos", "C3 牌组紊乱", "event", "event_c3",
         "Deck Chaos", "牌组紊乱",
         "The card module system has malfunctioned.",
         "卡牌模块系统出现故障，数据损毁。",
         {"Manual sorting", "Reboot the system", "Force format", nullptr},
         {"手动整理", "重启系统", "强制格式化", nullptr},
         {"Upgrade 2 random cards, lose 4 HP.", "-5 heat, delete 1 low-quality card, costs 10 credits.", "Costs 15 credits. Clear all low-quality cards.", nullptr},
         {"升级 2 张随机卡，失去 4 生命。", "降低 5 热量，移除 1 张低质量卡，花费 10 金币。", "花费 15 金币，清除所有低质量卡。", nullptr}},
        {"D1 Smuggler Cache", "D1 走私货舱", "event", "event_d1",
         "Smuggler Cache", "走私货舱",
         "A hidden cargo pod blinks with black market encryption signals.",
         "隐藏货舱闪烁着黑市加密信号。",
         {"Take everything", "Take only essentials", "Leave it alone", nullptr},
         {"全部取走", "只取必需品", "置之不理", nullptr},
         {"+16 credits and 1 random card.", "+6 HP.", "Continue without risk.", nullptr},
         {"+16 金币与 1 张随机卡。", "+6 生命。", "无风险继续。", nullptr}},
        {"D2 Wandering Trader", "D2 游商", "event", "event_d2",
         "Wandering Trader", "游商",
         "A lone merchant vessel emerges from the static.",
         "一艘孤独商船从干扰中浮现。",
         {"Buy cards cheap", "Sell junk cards", "Decline the deal", nullptr},
         {"低价购卡", "卖废卡", "拒绝交易", nullptr},
         {"Costs 18 credits. Gain 2 random cards.", "Delete 1 random card. Gain 10 credits.", "Continue.", nullptr},
         {"花费 18 金币，获得 2 张随机卡。", "移除 1 张随机卡，获得 10 金币。", "继续。", nullptr}},
        {"D3 Pirate Ambush", "D3 海盗伏击", "event", "event_d3",
         "Pirate Ambush", "海盗伏击",
         "A pirate interceptor drops out of warp and hails you with demands.",
         "一艘海盗截击舰从跃迁中现身并发出威胁。",
         {"Pay the ransom", "Fight back", "Full speed escape", nullptr},
         {"支付赎金", "回击", "全速脱离", nullptr},
         {"Costs 12 credits.", "Forced elite battle.", "+6 heat.", nullptr},
         {"花费 12 金币。", "强制进入精英战斗。", "+6 热量。", nullptr}},
        {"D4 Cryo Pod", "D4 冷冻舱", "event", "event_d4",
         "Cryo Pod", "冷冻舱",
         "A drifting cryo pod with a faint life sign.",
         "一个漂浮的冷冻舱内有微弱生命迹象。",
         {"Wake the sleeper", "Drain cryo energy", "Scavenge supplies", nullptr},
         {"唤醒沉睡者", "抽取冷冻能量", "搜刮补给", nullptr},
         {"Cards enhanced. Next fight must be elite.", "+5 heat, +6 HP, costs 8 credits.", "+18 credits, gain card, -8 HP.", nullptr},
         {"卡牌强化，下一场必为精英战。", "+5 热量，+6 生命，花费 8 金币。", "+18 金币，获得 1 张卡，-8 生命。", nullptr}},

        {"Repair Station", "维修站", "repair", "",
         "Repair Station", "维修站",
         "Reliable, clean repairs for a bruised ship. Choose how to invest your downtime here.",
         "为受损战舰提供可靠的维修服务。请选择停靠期间的修整方式。",
         {"Patch Hull (+10 HP)", "Vent Reactor (-6 Heat, +4 Shield)", "Install Plating (+4 Max HP)", "Upgrade 1 card"},
         {"修补舰体 (+10 生命)", "排出反应堆 (-6 热量，+4 护盾)", "加装装甲 (+4 最大生命)", "免费升级 1 张卡"},
         {"Reliable repairs.", "Stabilize core.", "Permanent hull boost.", "Free card upgrade."},
         {"可靠的修复。", "稳定核心。", "永久舰体强化。", "免费升级卡牌。"}},
        {"Salvage Wreck", "残骸打捞", "salvage", "",
         "Salvage Wreck", "残骸打捞",
         "Drifting wreckage offers credits or salvageable modules. Choose how to harvest it.",
         "漂浮的残骸藏着金币与可用模块。请选择回收方式。",
         {"Strip Credits", "Recover Module Rack", "Leave It", nullptr},
         {"剥离金币", "回收模块架", "置之不理", nullptr},
         {"Gain credits.", "Card reward.", "Skip.", nullptr},
         {"获得金币。", "卡牌奖励。", "跳过。", nullptr}},
    };
    return kEntries;
}

const std::vector<SceneEntry>& SceneCatalogue() {
    static const std::vector<SceneEntry> kEntries = {
        {"Intro · Game Opening", "序章 · 游戏开场", "route", "game_intro"},
        {"Location · Repair Station", "地点 · 维修站", "location", "repair"},
        {"Location · Salvage Wreck", "地点 · 残骸打捞", "location", "salvage"},
        {"Location · Black Market", "地点 · 黑市", "location", "market"},
        {"Service · Repair Retrofit", "服务 · 维修升级", "service", "repair_upgrade"},
        {"Service · Market Upgrade", "服务 · 黑市升级", "service", "market_upgrade"},
        {"Service · Market Purge", "服务 · 黑市移除", "service", "market_purge"},
        {"Service · Free Event Upgrade", "服务 · 事件免费升级", "service", "event_upgrade"},
        {"Service · Free Event Purge", "服务 · 事件免费移除", "service", "event_purge"},
        {"Event · Distress Beacon", "事件 · 求救信标", "event_static", "event_a1"},
        {"Event · Ghost Ship", "事件 · 幽灵舰", "event_static", "event_a2"},
        {"Event · Strange Signal", "事件 · 诡异信号", "event_static", "event_b1"},
        {"Event · Space Fold", "事件 · 空间折叠", "event_static", "event_b2"},
        {"Event · Ark Beacon", "事件 · 方舟信标", "event_static", "event_b3"},
        {"Event · Reactor Leak", "事件 · 反应堆泄漏", "event_static", "event_c1"},
        {"Event · Hull Breach", "事件 · 舰体破损", "event_static", "event_c2"},
        {"Event · Deck Chaos", "事件 · 牌组紊乱", "event_static", "event_c3"},
        {"Event · Smuggler Cache", "事件 · 走私货舱", "event_static", "event_d1"},
        {"Event · Wandering Trader", "事件 · 游商", "event_static", "event_d2"},
        {"Event · Pirate Ambush", "事件 · 海盗伏击", "event_static", "event_d3"},
        {"Event · Cryo Pod", "事件 · 冷冻舱", "event_static", "event_d4"},
        {"Story Boss · Broker Echo", "剧情 Boss · 经纪人回响", "story_boss", "the_pale_broker"},
        {"Story Boss · Cinder Shrine Signal", "剧情 Boss · 灰烬圣所信号", "story_boss", "saint_of_cinders"},
        {"Story Boss · Beneath Choir Resonance", "剧情 Boss · 深渊合唱回响", "story_boss", "the_choir_beneath"},
        {"Story Boss · Prototype Zero Trace", "剧情 Boss · 零号原型轨迹", "story_boss", "salvager_zero"},
        {"Escape Route · Prologue", "逃亡路线 · 序章", "route", "escape_prologue"},
        {"Escape Route · 1 Patrol Outpost", "逃亡路线 · 1 联邦哨站", "route", "escape_1"},
        {"Escape Route · 2 Smuggling Port", "逃亡路线 · 2 走私港", "route", "escape_2"},
        {"Escape Route · 3 Radiation Belt", "逃亡路线 · 3 辐射带", "route", "escape_3"},
        {"Escape Route · 4 Pirate Pursuit", "逃亡路线 · 4 海盗追击", "route", "escape_4"},
        {"Escape Route · 5 Escape Pod", "逃亡路线 · 5 逃生舱", "route", "escape_5"},
        {"Escape Route · 6 Jump Point", "逃亡路线 · 6 跃迁点", "route", "escape_6"},
        {"Escape Route · 7 Edge Barrier", "逃亡路线 · 7 边缘屏障", "route", "escape_7"},
        {"Escape Route · 8 Safe Station", "逃亡路线 · 8 安全站", "route", "escape_8"},
        {"Truth Route · 1 Memory Fragment", "真相路线 · 1 记忆碎片", "route", "truth_1"},
        {"Truth Route · 2 Guardian AI", "真相路线 · 2 守护 AI", "route", "truth_2"},
        {"Truth Route · 3 Captain Consciousness", "真相路线 · 3 船长意识", "route", "truth_3"},
        {"Truth Route · 4 Collapse Point", "真相路线 · 4 崩塌点", "route", "truth_4"},
        {"Truth Route · 5 Birth Truth", "真相路线 · 5 起源真相", "route", "truth_5"},
    };
    return kEntries;
}

// Look up an encounter by its engine encounterId. Returns nullptr if the
// catalogue does not own that ID (e.g. random escape/truth events).
const EncounterEntry* FindEncounterById(const std::string& id) {
    if (id.empty()) {
        return nullptr;
    }
    for (const EncounterEntry& e : EncounterCatalogue()) {
        if (e.encounterId != nullptr && id == e.encounterId) {
            return &e;
        }
    }
    return nullptr;
}

}  // namespace

void GameEngine::dev_openDebugScene(const std::string& sceneKey,
                                    const std::string& payload) {
    const std::string shipId = state_.player.shipId.empty() ? "vanguard" : state_.player.shipId;
    if (!(sceneKey == "route" && payload == "game_intro")) {
        dev_initRunIfNeeded(state_.difficulty, shipId);
    }

    state_.reward = {};
    state_.animation = {};
    state_.battleLog.clear();
    state_.activeNodeType.clear();
    state_.activeNodeName.clear();

    if (sceneKey == "event_static") {
        const EncounterEntry* entry = FindEncounterById(payload);
        if (entry != nullptr) {
            ForceStaticEncounter(*this, *entry);
        }
        return;
    }

    if (sceneKey == "location") {
        if (payload == "repair") {
            startEncounterForNode({"repair", "Repair Station", "A maintenance dock answers your ping.", 1, ""});
        } else if (payload == "salvage") {
            startEncounterForNode({"salvage", "Derelict Wreck", "Broken hulls drift in a silent salvage field.", 1, ""});
        } else if (payload == "market") {
            startEncounterForNode({"market", "Black Market", "Encrypted trade beacons flare nearby.", 2, ""});
        }
        return;
    }

    if (sceneKey == "service") {
        if (payload == "repair_upgrade") {
            enterRepairUpgrade(0);
        } else if (payload == "market_upgrade") {
            enterMarketUpgrade(0);
        } else if (payload == "market_purge") {
            enterMarketPurge(0);
        } else if (payload == "event_upgrade") {
            enterEventUpgrade(0);
        } else if (payload == "event_purge") {
            enterEventPurge(0);
        }
        return;
    }

    if (sceneKey == "story_boss") {
        const EnemyTemplate* enemy = enemyTemplateById(payload);
        if (enemy != nullptr) {
            startEncounterForNode({"event", enemy->name, "A hidden flagship signature demands immediate attention.", 4, enemy->id});
        }
        return;
    }

    if (sceneKey == "route") {
        if (payload == "game_intro") {
            startNewRun(state_.difficulty, shipId);
            return;
        }

        if (payload == "escape_prologue") {
            g_sa_beacon_activated = 1;
            g_sa_escape_stage = 1;
            g_sa_escape_intro_pending = true;
            g_sa_escape_battle_pending = false;
            g_sa_truth_stage = 0;
            g_sa_truth_post_battle = false;
            startEncounterForNode({"event", "Escape Decision", "Developer route jump.", 1, ""});
            return;
        }

        if (payload.rfind("escape_", 0) == 0) {
            const int stage = std::atoi(payload.c_str() + 7);
            if (stage >= 1 && stage <= 8) {
                g_sa_beacon_activated = 1;
                g_sa_escape_stage = stage;
                g_sa_escape_intro_pending = false;
                g_sa_escape_battle_pending = false;
                g_sa_truth_stage = 0;
                g_sa_truth_post_battle = false;
                startEncounterForNode({"event", "Escape Route", "Developer route jump.", 1, ""});
            }
            return;
        }

        if (payload.rfind("truth_", 0) == 0) {
            const int stage = std::atoi(payload.c_str() + 6);
            if (stage >= 1 && stage <= 5) {
                g_sa_beacon_activated = 1;
                g_sa_truth_stage = stage;
                g_sa_truth_post_battle = true;
                g_sa_escape_stage = 0;
                g_sa_escape_intro_pending = false;
                startEncounterForNode({"event", "Truth Route", "Developer route jump.", 1, ""});
            }
        }
    }
}

void GameEngine::dev_reloadCurrentEncounter() {
    const std::string& encounterId = state_.encounter.encounterId;

    if (state_.encounter.typeId == "repair" && encounterId.empty()) {
        dev_openDebugScene("location", "repair");
        return;
    }
    if (state_.encounter.typeId == "salvage" && encounterId.empty()) {
        dev_openDebugScene("location", "salvage");
        return;
    }
    if (encounterId == "market_shop") {
        dev_openDebugScene("location", "market");
        return;
    }
    if (encounterId == "repair_upgrade" || encounterId == "market_upgrade" ||
        encounterId == "market_purge" || encounterId == "event_upgrade" ||
        encounterId == "event_purge") {
        dev_openDebugScene("service", encounterId);
        return;
    }
    if (encounterId == "event_story_boss") {
        const std::string enemyId = state_.encounter.payloadIds.empty() ? "" : state_.encounter.payloadIds.front();
        if (!enemyId.empty()) {
            dev_openDebugScene("story_boss", enemyId);
        }
        return;
    }
    if (encounterId == "game_intro" || encounterId == "escape_prologue" ||
        encounterId.rfind("escape_", 0) == 0 || encounterId.rfind("truth_", 0) == 0) {
        dev_openDebugScene("route", encounterId);
        return;
    }
    if (FindEncounterById(encounterId) != nullptr) {
        dev_openDebugScene("event_static", encounterId);
    }
}

// =============================================================================
// DevModeUi implementation.
// =============================================================================

namespace dev_mode {

using namespace ftxui;

DevModeUi::DevModeUi(GameEngine& engine,
                     ScreenInteractive& /*screen*/,
                     UiLanguage& language,
                     bool& visible,
                     std::function<void()> on_close)
    : engine_(engine), language_(language), visible_(visible),
      on_close_(std::move(on_close)) {

    // -------- Static catalogue & cached pools --------
    difficulty_entries_ = {"Easy", "Normal", "Hard"};
    auto ships = engine_.dev_listShipsWithNames();
    for (const auto& [id, name] : ships) {
        ship_ids_.push_back(id);
        ship_entries_.push_back(name);
    }
    auto cards = engine_.dev_listCardsWithNames();
    for (const auto& [id, name] : cards) {
        card_ids_.push_back(id);
        card_labels_.push_back(name);
        debug_card_labels_.push_back(name);
    }
    auto enemies = engine_.dev_listEnemiesWithNames();
    for (const auto& [id, name] : enemies) {
        enemy_ids_.push_back(id);
        enemy_labels_.push_back(name);
    }
    for (const auto& entry : SceneCatalogue()) {
        if (!IsSelectableMapScene(entry)) {
            continue;
        }
        encounter_labels_.emplace_back();
        encounter_ids_.push_back(entry.sceneKey == nullptr ? "" : entry.sceneKey);
        encounter_payloads_.push_back(entry.payload == nullptr ? "" : entry.payload);
    }
    rebuildDestinationOptions();
    battle_node_labels_ = {"normal", "elite", "boss"};
    ending_labels_ = {"Normal Ending", "Escape Ending", "Truth Ending"};

    // -------- Password modal components --------
    password_input_component_ = Input(&password_input_, "******");
    password_confirm_button_ = Button(&password_confirm_label_, [&] {
        if (password_input_ == kPassword) {
            password_input_.clear();
            status_line_.clear();
            unlocked_ = true;
            page_ = Main;
        } else {
            status_line_ = password_error_label_;
            password_input_.clear();
        }
    }, AccentButton(kGold));
    password_cancel_button_ = Button(&password_cancel_label_, [&] {
        dismissModal();
    }, AccentButton(kEnemy));

    // -------- Options modal components --------
    difficulty_toggle_ = Toggle(&difficulty_entries_, &difficulty_index_);
    ship_toggle_ = Toggle(&ship_entries_, &ship_index_);
    setup_destination_menu_ = Menu(&destination_labels_, &setup_destination_index_);

    init_run_button_ = Button(&label_init_run_, [&] {
        if (ship_ids_.empty() || ship_index_ < 0 || ship_index_ >= static_cast<int>(ship_ids_.size())) {
            return;
        }
        if (setup_destination_index_ < 0 ||
            setup_destination_index_ >= static_cast<int>(destination_targets_.size())) {
            return;
        }

        engine_.dev_initRunIfNeeded(difficulty_index_, ship_ids_[ship_index_]);

        const auto [sec, dep] = destination_targets_[setup_destination_index_];
        if (sec == -1) {
            engine_.dev_setEscapeStage(dep);
        } else if (sec == -2) {
            engine_.dev_setTruthStage(dep, /*postBattle=*/true);
        } else {
            engine_.dev_jumpToSector(sec, dep);
        }

        dismissModal();
    }, AccentButton(kGold));

    destination_menu_ = Menu(&destination_labels_, &destination_index_);
    jump_button_ = Button(&label_jump_, [&] {
        if (destination_index_ < 0 ||
            destination_index_ >= static_cast<int>(destination_targets_.size())) {
            return;
        }
        const auto [sec, dep] = destination_targets_[destination_index_];
        if (sec == -1) {
            engine_.dev_setEscapeStage(dep);
        } else if (sec == -2) {
            engine_.dev_setTruthStage(dep, /*postBattle=*/true);
        } else {
            engine_.dev_jumpToSector(sec, dep);
        }
        closeDebugPanel();
    }, AccentButton(kHull));

    encounter_menu_ = Menu(&encounter_labels_, &encounter_index_);
    trigger_encounter_button_ = Button(&label_trigger_encounter_, [&] {
        if (encounter_index_ < 0 || encounter_index_ >= static_cast<int>(encounter_ids_.size())) {
            return;
        }
        const std::string sceneKey = encounter_ids_[encounter_index_];
        const std::string payload = encounter_payloads_[encounter_index_];
        closeEventPanel();
        engine_.dev_openDebugScene(sceneKey, payload);
    }, AccentButton(kHull));

    battle_node_toggle_ = Toggle(&battle_node_labels_, &battle_node_index_);
    enemy_menu_ = Menu(&enemy_labels_, &enemy_index_);
    start_battle_button_ = Button(&label_start_battle_, [&] {
        if (enemy_ids_.empty()) return;
        const std::string nodeType = battle_node_labels_[battle_node_index_];
        engine_.dev_startBattleWith(nodeType, enemy_ids_[enemy_index_]);
        dismissModal();
    }, AccentButton(kGold));
    skip_battle_button_ = Button(&label_skip_battle_, [&] {
        engine_.dev_skipCurrentBattleAsVictory();
        dismissModal();
    }, AccentButton(kEnemy));

    card_menu_ = Menu(&card_labels_, &card_index_);
    add_to_hand_button_ = Button(&label_add_to_hand_, [&] {
        if (card_ids_.empty()) return;
        engine_.dev_addCardToHand(card_ids_[card_index_]);
        status_line_ = "OK";
    }, AccentButton(kHull));
    add_to_draw_button_ = Button(&label_add_to_draw_, [&] {
        if (card_ids_.empty()) return;
        engine_.dev_addCardToDrawPile(card_ids_[card_index_]);
        status_line_ = "OK";
    }, AccentButton(kHull));
    play_top_button_ = Button(&label_play_top_, [&] {
        engine_.dev_playTopOfHand();
        status_line_ = "OK";
    }, AccentButton(kGold));

    ending_menu_ = Menu(&ending_labels_, &ending_index_);
    show_ending_button_ = Button(&label_show_ending_, [&] {
        engine_.dev_showEnding(ending_index_);
        dismissModal();
    }, AccentButton(kGold));

    return_map_button_ = Button(&label_return_map_, [&] {
        engine_.dev_returnToMap();
        dismissModal();
    }, AccentButton(kHull));

    close_button_ = Button(&close_label_, [&] {
        dismissModal();
    }, AccentButton(kEnemy));
    modal_exit_dev_button_ = Button(&modal_exit_dev_label_, [&] {
        exitDeveloperMode();
    }, AccentButton(kEnemy));

    password_page_ = Container::Vertical({
        password_input_component_,
        password_confirm_button_,
        password_cancel_button_,
    });

    main_page_ = Container::Vertical({
        difficulty_toggle_,
        ship_toggle_,
        setup_destination_menu_,
        init_run_button_,
        close_button_,
        modal_exit_dev_button_,
    });

    pages_ = Container::Tab({password_page_, main_page_}, &page_);

    root_ = Renderer(pages_, [this] {
        rebuildLabels();
        return page_ == Password ? renderPassword() : renderMain();
    });

    // ---------------------------------------------------------------
    // Persistent debug overlay (floating button + modal panel).
    // ---------------------------------------------------------------
    floating_button_ = Button(&label_floating_button_, [&] { openDebugPanel(); },
                              AccentButton(kEnemy));
    exit_dev_button_ = Button(&label_floating_exit_dev_, [&] { exitDeveloperMode(); },
                              AccentButton(kEnemy));

    auto step_button = [&](std::string* label, std::function<void()> action) {
        return Button(label, std::move(action), AccentButton(kHull));
    };

    dbg_hp_dec5_  = step_button(&label_dec5_, [&] { engine_.dev_addHp(-5); });
    dbg_hp_dec_   = step_button(&label_dec_,  [&] { engine_.dev_addHp(-1); });
    dbg_hp_inc_   = step_button(&label_inc_,  [&] { engine_.dev_addHp(+1); });
    dbg_hp_inc5_  = step_button(&label_inc5_, [&] { engine_.dev_addHp(+5); });

    dbg_shield_dec5_ = step_button(&label_dec5_, [&] { engine_.dev_addShield(-5); });
    dbg_shield_dec_  = step_button(&label_dec_,  [&] { engine_.dev_addShield(-1); });
    dbg_shield_inc_  = step_button(&label_inc_,  [&] { engine_.dev_addShield(+1); });
    dbg_shield_inc5_ = step_button(&label_inc5_, [&] { engine_.dev_addShield(+5); });

    dbg_energy_dec_ = step_button(&label_dec_, [&] { engine_.dev_addEnergy(-1); });
    dbg_energy_inc_ = step_button(&label_inc_, [&] { engine_.dev_addEnergy(+1); });

    dbg_heat_dec5_ = step_button(&label_dec5_, [&] { engine_.dev_addHeat(-5); });
    dbg_heat_dec_  = step_button(&label_dec_,  [&] { engine_.dev_addHeat(-1); });
    dbg_heat_inc_  = step_button(&label_inc_,  [&] { engine_.dev_addHeat(+1); });
    dbg_heat_inc5_ = step_button(&label_inc5_, [&] { engine_.dev_addHeat(+5); });

    dbg_scrap_dec50_ = step_button(&label_dec50_, [&] { engine_.dev_addScrap(-50); });
    dbg_scrap_dec_   = step_button(&label_dec_,   [&] { engine_.dev_addScrap(-1); });
    dbg_scrap_inc_   = step_button(&label_inc_,   [&] { engine_.dev_addScrap(+1); });
    dbg_scrap_inc50_ = step_button(&label_inc50_, [&] { engine_.dev_addScrap(+50); });

    dbg_card_menu_ = Menu(&debug_card_labels_, &debug_card_index_);
    dbg_add_to_hand_button_ = Button(&label_debug_add_to_hand_, [&] {
        if (card_ids_.empty()) return;
        engine_.dev_addCardToHand(card_ids_[debug_card_index_]);
    }, AccentButton(kGold));

    dbg_reload_event_button_ = Button(&label_debug_reload_event_, [&] {
        engine_.dev_reloadCurrentEncounter();
    }, AccentButton(kHull));

    dbg_skip_battle_button_ = Button(&label_debug_skip_battle_, [&] {
        engine_.dev_skipCurrentBattleAsVictory();
    }, AccentButton(kEnemy));
    dbg_return_map_button_ = Button(&label_debug_return_map_, [&] {
        closeDebugPanel();
        engine_.dev_returnToMap();
        openEventPanel();
    }, AccentButton(kHull));
    dbg_close_button_ = Button(&label_debug_close_, [&] { closeDebugPanel(); },
                               AccentButton(kEnemy));
    dbg_exit_dev_button_ = Button(&label_debug_exit_dev_, [&] { exitDeveloperMode(); },
                                  AccentButton(kEnemy));
    event_panel_close_button_ = Button(&label_event_panel_close_, [&] {
        closeEventPanel();
    }, AccentButton(kEnemy));

    debug_panel_container_ = Container::Vertical({
        dbg_hp_dec5_, dbg_hp_dec_, dbg_hp_inc_, dbg_hp_inc5_,
        dbg_shield_dec5_, dbg_shield_dec_, dbg_shield_inc_, dbg_shield_inc5_,
        dbg_energy_dec_, dbg_energy_inc_,
        dbg_heat_dec5_, dbg_heat_dec_, dbg_heat_inc_, dbg_heat_inc5_,
        dbg_scrap_dec50_, dbg_scrap_dec_, dbg_scrap_inc_, dbg_scrap_inc50_,
        dbg_card_menu_, dbg_add_to_hand_button_,
        destination_menu_, jump_button_,
        dbg_skip_battle_button_, dbg_return_map_button_,
        dbg_close_button_, dbg_exit_dev_button_,
    });

    event_panel_container_ = Container::Vertical({
        encounter_menu_,
        trigger_encounter_button_,
        event_panel_close_button_,
    });

    floating_bar_ = Container::Horizontal({floating_button_, exit_dev_button_});
    overlay_interactive_ = Container::Tab({floating_bar_, debug_panel_container_, event_panel_container_}, &overlay_page_);
    overlay_root_ = Renderer(overlay_interactive_, [this] {
        rebuildLabels();
        return renderOverlay();
    });
}

void DevModeUi::open() {
    visible_ = true;
    page_ = unlocked_ ? Main : Password;
    password_input_.clear();
    status_line_.clear();
}

void DevModeUi::dismissModal() {
    visible_ = false;
    password_input_.clear();
    status_line_.clear();
    if (on_close_) on_close_();
}

void DevModeUi::exitDeveloperMode() {
    engine_.dev_shutdownSession();
    visible_ = false;
    resetUiState();
    if (on_close_) on_close_();
}

bool DevModeUi::handleOverlayEvent(Event event) {
    if (!debug_panel_visible_ && !event_panel_visible_) {
        return false;
    }
    if (event == Event::Escape) {
        if (event_panel_visible_) {
            closeEventPanel();
        } else {
            closeDebugPanel();
        }
        return true;
    }
    if (overlay_interactive_ && overlay_interactive_->OnEvent(event)) {
        return true;
    }
    return true;
}

void DevModeUi::resetUiState() {
    page_ = Password;
    password_input_.clear();
    status_line_.clear();
    unlocked_ = false;
    debug_panel_visible_ = false;
    event_panel_visible_ = false;
    overlay_page_ = 0;
    setup_destination_index_ = 0;
    destination_index_ = 0;
    encounter_index_ = 0;
    enemy_index_ = 0;
    battle_node_index_ = 0;
    card_index_ = 0;
    debug_card_index_ = 0;
    ending_index_ = 0;
}

void DevModeUi::openDebugPanel() {
    if (!unlocked_) {
        return;
    }
    event_panel_visible_ = false;
    debug_panel_visible_ = true;
    overlay_page_ = 1;
}

void DevModeUi::closeDebugPanel() {
    debug_panel_visible_ = false;
    overlay_page_ = 0;
}

void DevModeUi::openEventPanel() {
    if (!unlocked_) {
        return;
    }
    debug_panel_visible_ = false;
    event_panel_visible_ = true;
    overlay_page_ = 2;
}

void DevModeUi::closeEventPanel() {
    event_panel_visible_ = false;
    overlay_page_ = 0;
}

void DevModeUi::rebuildDestinationOptions() {
    destination_labels_.clear();
    destination_targets_.clear();
    const bool zh = (language_ == UiLanguage::Chinese);
    for (int s = 1; s <= 3; ++s) {
        for (int d = 0; d < 5; ++d) {
            const std::string label = zh
                ? ("星区 " + std::to_string(s) + " · 节点 " + std::to_string(d + 1))
                : ("Sector " + std::to_string(s) + " · Node " + std::to_string(d + 1));
            destination_labels_.push_back(label);
            destination_targets_.emplace_back(s, d);
        }
    }
    for (int e = 1; e <= 8; ++e) {
        destination_labels_.push_back(zh
            ? ("逃亡路线 · 阶段 " + std::to_string(e))
            : ("Escape Route · Stage " + std::to_string(e)));
        destination_targets_.emplace_back(-1, e);
    }
    for (int t = 1; t <= 6; ++t) {
        destination_labels_.push_back(zh
            ? ("深层真结局 · 阶段 " + std::to_string(t))
            : ("Truth Route · Stage " + std::to_string(t)));
        destination_targets_.emplace_back(-2, t);
    }
}

void DevModeUi::rebuildLabels() {
    const bool zh = (language_ == UiLanguage::Chinese);

    title_label_ = Tr(language_, "Developer Mode", "开发者模式");
    password_prompt_label_ = Tr(language_, "Enter password", "请输入密码");
    password_confirm_label_ = Tr(language_, "Confirm", "确认");
    password_cancel_label_ = Tr(language_, "Close", "关闭");
    password_error_label_ = Tr(language_, "Wrong password.", "密码错误。");
    close_label_ = " x ";
    modal_exit_dev_label_ = Tr(language_, "Exit Developer Mode", "退出开发者模式");

    section_run_ = Tr(language_, "Run Setup", "航程设定");
    section_destination_ = Tr(language_, "Destination", "目的地");
    section_encounter_ = Tr(language_, "Scene Navigation", "场景导航");
    section_battle_ = Tr(language_, "Battle", "战斗");
    section_cards_ = Tr(language_, "Cards", "卡牌");
    section_ending_ = Tr(language_, "Ending", "结局");

    label_difficulty_ = Tr(language_, "Difficulty", "难度");
    label_character_ = Tr(language_, "Character", "角色");
    label_init_run_ = Tr(language_, "Initialize Run", "初始化航程");
    label_jump_ = Tr(language_, "Jump", "跳转");
    label_trigger_encounter_ = Tr(language_, "Open Selected Scene", "打开选中场景");
    label_start_battle_ = Tr(language_, "Start Battle", "开始战斗");
    label_skip_battle_ = Tr(language_, "Skip Battle (Victory)", "跳过战斗（胜利）");
    label_add_to_hand_ = Tr(language_, "Add To Hand", "加入手牌");
    label_add_to_draw_ = Tr(language_, "Add To Draw Pile", "加入抽牌堆");
    label_play_top_ = Tr(language_, "Play Last Card In Hand", "打出手牌末张");
    label_show_ending_ = Tr(language_, "Show Ending", "播放结局");
    label_return_map_ = Tr(language_, "Return To Map", "返回地图");
    label_status_ = Tr(language_, "Status", "状态");

    label_floating_button_ = Tr(language_, " DEV ", " 调试 ");
    label_floating_exit_dev_ = Tr(language_, " Exit Dev Mode ", " 退出开发者模式 ");
    label_debug_panel_title_ = Tr(language_, "Debug Panel", "调试面板");
    label_debug_close_ = Tr(language_, "Close Panel", "关闭面板");
    label_debug_exit_dev_ = Tr(language_, "Exit Developer Mode", "退出开发者模式");
    label_debug_reload_event_ = Tr(language_, "Reload Current Event", "重新加载当前事件");
    label_debug_section_stats_ = Tr(language_, "Live Stats", "实时数值");
    label_debug_section_cards_ = Tr(language_, "Card Library", "卡牌库");
    label_debug_section_misc_ = Tr(language_, "Map Jump", "地图跳转");
    label_debug_hp_ = Tr(language_, "HP", "生命");
    label_debug_shield_ = Tr(language_, "Shield", "护盾");
    label_debug_energy_ = Tr(language_, "Energy", "能量");
    label_debug_heat_ = Tr(language_, "Heat", "热量");
    label_debug_scrap_ = Tr(language_, "Credits", "金币");
    label_debug_add_to_hand_ = Tr(language_, "Add Selected To Hand", "加入手牌");
    label_debug_skip_battle_ = Tr(language_, "Skip Battle", "跳过战斗");
    label_debug_return_map_ = Tr(language_, "Return To Map", "返回地图");
    label_event_panel_title_ = Tr(language_, "Event Selection", "事件选择");
    label_event_panel_confirm_ = Tr(language_, "Trigger Selected Event", "触发选中事件");
    label_event_panel_close_ = Tr(language_, "Close Panel", "关闭面板");

    label_inc_ = Tr(language_, " +1 ", " +1 ");
    label_dec_ = Tr(language_, " -1 ", " -1 ");
    label_inc5_ = Tr(language_, " +5 ", " +5 ");
    label_dec5_ = Tr(language_, " -5 ", " -5 ");
    label_inc50_ = Tr(language_, " +50 ", " +50 ");
    label_dec50_ = Tr(language_, " -50 ", " -50 ");

    difficulty_entries_[0] = Tr(language_, "Easy", "简单");
    difficulty_entries_[1] = Tr(language_, "Normal", "普通");
    difficulty_entries_[2] = Tr(language_, "Hard", "困难");

    auto ships = engine_.dev_listShipsWithNames();
    for (size_t i = 0; i < ships.size() && i < ship_entries_.size(); ++i) {
        ship_entries_[i] = TranslateGameText(language_, ships[i].second);
    }

    auto cards = engine_.dev_listCardsWithNames();
    for (size_t i = 0; i < cards.size() && i < card_labels_.size(); ++i) {
        card_labels_[i] = TranslateGameText(language_, cards[i].second);
    }
    for (size_t i = 0; i < cards.size() && i < debug_card_labels_.size(); ++i) {
        debug_card_labels_[i] = TranslateGameText(language_, cards[i].second);
    }

    auto enemies = engine_.dev_listEnemiesWithNames();
    for (size_t i = 0; i < enemies.size() && i < enemy_labels_.size(); ++i) {
        enemy_labels_[i] = TranslateGameText(language_, enemies[i].second);
    }

    for (size_t i = 0; i < encounter_labels_.size(); ++i) {
        encounter_labels_[i].clear();
        const std::string sceneKey = i < encounter_ids_.size() ? encounter_ids_[i] : "";
        const std::string payload = i < encounter_payloads_.size() ? encounter_payloads_[i] : "";
        for (const auto& scene : SceneCatalogue()) {
            const std::string sceneId = scene.sceneKey == nullptr ? "" : scene.sceneKey;
            const std::string scenePayload = scene.payload == nullptr ? "" : scene.payload;
            if (sceneId == sceneKey && scenePayload == payload) {
                encounter_labels_[i] = zh ? scene.labelZh : scene.labelEn;
                break;
            }
        }
    }

    label_trigger_encounter_ = label_event_panel_confirm_;

    battle_node_labels_[0] = Tr(language_, "Normal", "普通");
    battle_node_labels_[1] = Tr(language_, "Elite", "精英");
    battle_node_labels_[2] = Tr(language_, "Boss", "Boss");

    ending_labels_[0] = Tr(language_, "Normal Ending", "普通结局");
    ending_labels_[1] = Tr(language_, "Escape Ending", "逃亡结局");
    ending_labels_[2] = Tr(language_, "Truth Ending", "深层真结局");

    rebuildDestinationOptions();
}

Element DevModeUi::renderPassword() const {
    Elements rows;
    rows.push_back(text(title_label_) | bold | color(kGold) | center);
    rows.push_back(separator());
    rows.push_back(text(password_prompt_label_) | color(kHull));
    rows.push_back(password_input_component_->Render() | borderLight | color(Color::White));
    rows.push_back(hbox({
        password_confirm_button_->Render() | flex,
        text(" "),
        password_cancel_button_->Render() | flex,
    }));
    if (!status_line_.empty()) {
        rows.push_back(separator());
        rows.push_back(text(status_line_) | color(kEnemy));
    }
    Element panel = window(text(title_label_) | color(kGold), vbox(std::move(rows))) |
                    size(WIDTH, GREATER_THAN, 40) | bgcolor(kPanel);
    return dbox({
        filler() | bgcolor(kAbyss),
        vbox({filler(), hbox({filler(), panel, filler()}), filler()}),
    }) | clear_under;
}

Element DevModeUi::renderMain() const {
    auto section = [&](const std::string& title, std::vector<Element> body) {
        Elements rows;
        rows.push_back(text(title) | bold | color(kGold));
        rows.push_back(separator());
        for (auto& e : body) rows.push_back(std::move(e));
        return window(text(title) | color(kGold), vbox(std::move(rows))) | bgcolor(kPanel);
    };

    Elements sections;
    sections.push_back(section(section_run_, {
        hbox({text(label_difficulty_ + ": ") | color(kHull), difficulty_toggle_->Render()}),
        hbox({text(label_character_ + ": ") | color(kHull), ship_toggle_->Render()}),
    }));
    sections.push_back(section(section_destination_, {
        setup_destination_menu_->Render() | vscroll_indicator | frame |
            size(HEIGHT, LESS_THAN, 12),
        init_run_button_->Render(),
    }));

    Elements rows;
    rows.push_back(hbox({
        text(Tr(language_, "Developer Initialization", "开发者初始化")) | bold | color(kGold) | flex,
        close_button_->Render(),
    }));
    rows.push_back(separatorDouble());
    rows.push_back(text(Tr(language_,
                           "Choose difficulty, ship, and starting location. Initialization uses the normal game's starting values, then closes automatically.",
                           "选择难度、飞船与初始地点。初始化会沿用正常游戏的起始数值，随后自动关闭面板。")) |
                   color(kHull));
    if (!status_line_.empty()) {
        rows.push_back(text(label_status_ + ": " + status_line_) | color(kGold) | center);
    }
    rows.push_back(separator());
    for (auto& sec : sections) rows.push_back(std::move(sec));
    rows.push_back(separator());
    rows.push_back(modal_exit_dev_button_->Render());

    Element panel = vbox(std::move(rows)) | vscroll_indicator | yframe |
                    size(HEIGHT, LESS_THAN, 36) |
                    size(WIDTH, GREATER_THAN, 68) |
                    borderDouble | bgcolor(kAbyss);
    return dbox({
        filler() | bgcolor(kAbyss),
        vbox({filler(), hbox({filler(), panel, filler()}), filler()}),
    }) | clear_under;
}

Element DevModeUi::renderOverlay() const {
    if (!unlocked_ || visible_) {
        return text("");
    }

    if (event_panel_visible_) {
        Element panel = renderEventPanel();
        return dbox({
            filler() | bgcolor(kAbyss),
            vbox({filler(), hbox({filler(), panel | flex_grow, filler()}), filler()}) | bgcolor(kAbyss),
        }) | clear_under;
    }

    if (debug_panel_visible_) {
        Element panel = renderDebugPanel();
        return dbox({
            filler() | bgcolor(kAbyss),
            vbox({filler(), hbox({filler(), panel | flex_grow, filler()}), filler()}) | bgcolor(kAbyss),
        }) | clear_under;
    }

    if (engine_.state().page == AppPage::MainMenu) {
        return text("");
    }

    return vbox({
        filler(),
        hbox({floating_button_->Render(), filler(), exit_dev_button_->Render()}),
    });
}

Element DevModeUi::renderDebugPanel() const {
    const RunState& s = engine_.state();

    auto stat_row = [&](const std::string& label,
                        const std::string& value,
                        std::vector<Component> btns) {
        Elements row;
        row.push_back(text(label + ":") | color(kHull) | size(WIDTH, EQUAL, 8));
        row.push_back(text(value) | color(kGold) | size(WIDTH, EQUAL, 12));
        for (auto& b : btns) {
            row.push_back(b->Render());
            row.push_back(text(" "));
        }
        return hbox(std::move(row));
    };

    Elements stat_rows = {
        stat_row(label_debug_hp_,
                 std::to_string(s.player.hp) + "/" + std::to_string(s.player.maxHp),
                 {dbg_hp_dec5_, dbg_hp_dec_, dbg_hp_inc_, dbg_hp_inc5_}),
        stat_row(label_debug_shield_,
                 std::to_string(s.player.shield),
                 {dbg_shield_dec5_, dbg_shield_dec_, dbg_shield_inc_, dbg_shield_inc5_}),
        stat_row(label_debug_energy_,
                 std::to_string(s.player.energy) + "/" + std::to_string(s.player.maxEnergy),
                 {dbg_energy_dec_, dbg_energy_inc_}),
        stat_row(label_debug_heat_,
                 std::to_string(s.player.heat) + "/" + std::to_string(s.player.maxHeat),
                 {dbg_heat_dec5_, dbg_heat_dec_, dbg_heat_inc_, dbg_heat_inc5_}),
        stat_row(label_debug_scrap_,
                 std::to_string(s.player.scrap),
                 {dbg_scrap_dec50_, dbg_scrap_dec_, dbg_scrap_inc_, dbg_scrap_inc50_}),
    };

    auto window_section = [&](const std::string& title, Elements body) {
        Elements rows;
        rows.push_back(text(title) | bold | color(kGold));
        rows.push_back(separator());
        for (auto& e : body) rows.push_back(std::move(e));
        return window(text(title) | color(kGold), vbox(std::move(rows))) | bgcolor(kPanel);
    };

    Element stats_section = window_section(label_debug_section_stats_, std::move(stat_rows));
    Element cards_section = window_section(label_debug_section_cards_, {
        dbg_card_menu_->Render() | vscroll_indicator | frame |
            size(HEIGHT, LESS_THAN, 10),
        dbg_add_to_hand_button_->Render(),
    });
    Element misc_section = window_section(label_debug_section_misc_, {
        destination_menu_->Render() | vscroll_indicator | frame |
            size(HEIGHT, LESS_THAN, 12),
        jump_button_->Render(),
        hbox({
            dbg_skip_battle_button_->Render() | flex,
            text(" "),
            dbg_return_map_button_->Render() | flex,
        }),
    });

    Elements rows;
    rows.push_back(text(label_debug_panel_title_) | bold | color(kGold) | center);
    rows.push_back(separatorDouble());
    rows.push_back(stats_section);
    rows.push_back(cards_section);
    rows.push_back(misc_section);
    rows.push_back(separator());
    rows.push_back(hbox({
        dbg_close_button_->Render() | flex,
        text(" "),
        dbg_exit_dev_button_->Render() | flex,
    }));

        return (vbox(std::move(rows)) | vscroll_indicator | yframe |
           size(HEIGHT, LESS_THAN, 36) | size(WIDTH, GREATER_THAN, 60) |
            borderDouble | bgcolor(kAbyss)) | clear_under;
}

Element DevModeUi::renderEventPanel() const {
    Elements rows;
    rows.push_back(text(label_event_panel_title_) | bold | color(kGold) | center);
    rows.push_back(separatorDouble());
    rows.push_back(text(Tr(language_,
                           "Choose any map-side event or location. Selecting one will trigger it immediately. Closing this panel keeps the map on its normal three random options.",
                           "选择任意地图侧边会出现的事件或地点。选中后会立即触发；关闭此面板则保留地图原本的三个随机选项。")) |
                   color(kHull));
    rows.push_back(separator());
    rows.push_back(encounter_menu_->Render() | vscroll_indicator | frame |
                   size(HEIGHT, LESS_THAN, 18));
    rows.push_back(separator());
    rows.push_back(trigger_encounter_button_->Render());
    rows.push_back(separator());
    rows.push_back(event_panel_close_button_->Render());

    Element panel = vbox(std::move(rows)) | vscroll_indicator | yframe |
                    size(HEIGHT, LESS_THAN, 34) |
                    size(WIDTH, GREATER_THAN, 70) |
                    borderDouble | bgcolor(kAbyss);
    return panel | clear_under;
}

}  // namespace dev_mode
