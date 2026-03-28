#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>
#include <ftxui/screen/terminal.hpp>

#include "tui_app.h"
#include "tui_game.h"

using namespace ftxui;

namespace {

constexpr size_t kBattleSlots = static_cast<size_t>(kBattleHandLimit);
constexpr size_t kMapSlots = 3;
constexpr size_t kEncounterSlots = 5;
constexpr size_t kRewardSlots = 3;

enum class UiLanguage {
    English,
    Chinese,
};

enum class SaveDialogMode {
    None,
    Save,
    Load,
};

const Color kAbyss = Color::RGB(5, 10, 24);
const Color kPanel = Color::RGB(16, 30, 56);
const Color kPanelAlt = Color::RGB(11, 21, 42);
const Color kHull = Color::RGB(83, 223, 255);
const Color kShield = Color::RGB(124, 178, 255);
const Color kHeat = Color::RGB(255, 111, 74);
const Color kGold = Color::RGB(246, 196, 91);
const Color kEnemy = Color::RGB(255, 129, 136);
const Color kDim = Color::RGB(104, 118, 140);

std::string Tr(UiLanguage language, const std::string& english, const std::string& chinese) {
    return language == UiLanguage::Chinese ? chinese : english;
}

bool StartsWith(const std::string& text, const std::string& prefix) {
    return text.size() >= prefix.size() && text.compare(0, prefix.size(), prefix) == 0;
}

bool EndsWith(const std::string& text, const std::string& suffix) {
    return text.size() >= suffix.size() &&
           text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::string TranslateGameText(UiLanguage language, const std::string& text) {
    if (language == UiLanguage::English) {
        return text;
    }

    static const std::pair<const char*, const char*> kTranslations[] = {
        {"Vanguard", "先锋号"},
        {"Drift Fox", "漂移狐"},
        {"Iron Bastion", "钢铁堡垒"},
        {"Pirate Scout", "海盗侦察舰"},
        {"Raider Frigate", "劫掠护卫舰"},
        {"Scrap Drone", "废料无人机"},
        {"Rogue Miner", "失控矿机"},
        {"Signal Leech", "信号水蛭"},
        {"War Corvette", "战争轻巡"},
        {"Black Market Hunter", "黑市猎手"},
        {"Corrupted AI Ship", "失控 AI 战舰"},
        {"Dread Frigate Omega", "欧米伽无畏护卫舰"},
        {"The Furnace Carrier", "熔炉航母"},
        {"Abyss Core Sentinel", "深渊核心哨兵"},
        {"Laser Burst", "激光爆裂"},
        {"Raise Shield", "升起护盾"},
        {"Coolant Flush", "冷却液冲洗"},
        {"Quick Patch", "快速修补"},
        {"Overclock", "超频"},
        {"Scan Pulse", "扫描脉冲"},
        {"Missile Pod", "导弹舱"},
        {"Piercing Shot", "穿甲射击"},
        {"Burst Fire", "爆发连射"},
        {"Emergency Barrier", "紧急屏障"},
        {"Reactor Boost", "反应堆增幅"},
        {"Reroute Power", "重路供能"},
        {"Stabilizer", "稳定器"},
        {"Salvage Drone", "回收无人机"},
        {"Plasma Lance", "等离子长枪"},
        {"EMP Pulse", "电磁脉冲"},
        {"Hack Jammer", "骇入干扰器"},
        {"Chain Pulse", "连锁脉冲"},
        {"Heat Sink", "散热井"},
        {"System Flush", "系统冲洗"},
        {"Shield Converter", "护盾转换器"},
        {"Overload Cannon", "过载巨炮"},
        {"Reactor Surge", "反应堆激涌"},
        {"Nova Shield", "新星护盾"},
        {"Target Lock", "目标锁定"},
        {"Emergency Vent", "紧急排热"},
        {"Blind Jump Protocol", "盲跳协议"},
        {"Attack", "攻击"},
        {"Defense", "防御"},
        {"Utility", "功能"},
        {"Control", "控制"},
        {"Risk", "风险"},
        {"Deal 6 damage.", "造成 6 点伤害。"},
        {"Gain 5 shield.", "获得 5 点护盾。"},
        {"Reduce 3 heat.", "降低 3 点热量。"},
        {"Recover 4 HP.", "恢复 4 点生命。"},
        {"Gain 1 energy. Gain 1 heat.", "获得 1 点能量，并增加 1 点热量。"},
        {"Draw 2 cards.", "抽 2 张牌。"},
        {"Deal 11 damage.", "造成 11 点伤害。"},
        {"Deal 5 damage. Ignore shield.", "造成 5 点伤害，并无视护盾。"},
        {"Deal 4 damage twice.", "造成 2 次 4 点伤害。"},
        {"Gain 10 shield.", "获得 10 点护盾。"},
        {"Next turn, gain 2 extra energy.", "下个回合额外获得 2 点能量。"},
        {"Draw 1 card and gain 1 shield.", "抽 1 张牌并获得 1 点护盾。"},
        {"Reduce 4 heat. Draw 1 card.", "降低 4 点热量，并抽 1 张牌。"},
        {"If you win this battle, gain +6 scrap.", "若本场战斗获胜，额外获得 6 废料。"},
        {"Deal 14 damage. Gain 2 heat.", "造成 14 点伤害，并增加 2 点热量。"},
        {"Deal 4 damage. Enemy's next 2 attacks lose 2 damage.", "造成 4 点伤害，敌人的接下来 2 次攻击各减少 2 点伤害。"},
        {"Remove enemy shield. They cannot gain shield for 2 turns.", "移除敌方护盾，并使其 2 回合内无法获得护盾。"},
        {"Deal 5 damage. Deal +3 if another attack was played this turn.", "造成 5 点伤害；若本回合已打出过其他攻击牌，则额外造成 3 点伤害。"},
        {"Cut current heat in half.", "将当前热量减半。"},
        {"Reduce 5 heat. Draw 2 cards.", "降低 5 点热量，并抽 2 张牌。"},
        {"Deal damage equal to your current shield, then lose that shield.", "造成等同于当前护盾值的伤害，然后失去这些护盾。"},
        {"Deal 22 damage. Gain 4 heat.", "造成 22 点伤害，并增加 4 点热量。"},
        {"Gain 2 energy. Gain 3 heat.", "获得 2 点能量，并增加 3 点热量。"},
        {"Gain 14 shield.", "获得 14 点护盾。"},
        {"Your next attack card deals double damage.", "你的下一张攻击牌造成双倍伤害。"},
        {"Remove all heat. Lose 2 HP.", "清空全部热量，并失去 2 点生命。"},
        {"Trigger one strong but unpredictable effect.", "触发一个强力但不可预测的效果。"},
        {"Twin Lasers", "双联激光"},
        {"Afterburner Strike", "加力突袭"},
        {"Brace", "稳固防护"},
        {"Cannon Burst", "炮火齐射"},
        {"Scrap Plating", "废铁装甲"},
        {"Heavy Salvo", "重炮齐射"},
        {"Shunt Shield", "偏转护盾"},
        {"Needle Volley", "针刺弹幕"},
        {"Saw Drone", "链锯无人机"},
        {"Charge Drill", "钻头蓄能"},
        {"Mining Beam", "采矿光束"},
        {"Shrapnel Burst", "破片爆发"},
        {"EMP Ripple", "电磁涟漪"},
        {"Heat Spike", "热量尖峰"},
        {"Signal Lash", "信号鞭击"},
        {"Barrage", "弹幕压制"},
        {"Shield Wall", "护盾壁垒"},
        {"Jam Pulse", "干扰脉冲"},
        {"Hunter Volley", "猎手齐射"},
        {"Tracer Burn", "曳光灼烧"},
        {"Glitch Strike", "故障打击"},
        {"Phase Barrier", "相位屏障"},
        {"Entropy Pulse", "熵脉冲"},
        {"Aegis Drum", "神盾鼓动"},
        {"Broadside", "侧舷齐射"},
        {"Missile Rain", "导弹雨"},
        {"Heat Surge", "热浪喷发"},
        {"Forge Cannon", "锻炉巨炮"},
        {"Furnace Lash", "熔炉鞭击"},
        {"Core Barrier", "核心屏障"},
        {"Beam Lance", "光矛穿刺"},
        {"Reality Static", "现实静滞"},
        {"Pirate Ambush", "海盗伏击"},
        {"Elite Intercept", "精英拦截"},
        {"Derelict Wreck", "废弃残骸"},
        {"Repair Station", "维修站"},
        {"Black Market", "黑市"},
        {"Black Market Purge", "黑市移除终端"},
        {"Strange Signal", "诡异信号"},
        {"Fast raiders are sweeping the lane ahead.", "前方航道有高速掠袭舰巡弋。"},
        {"A heavier hostile signature holds position.", "更重型的敌舰信号正在前方驻守。"},
        {"Broken hulls drift in a silent salvage field.", "破碎船壳漂浮在寂静的打捞场中。"},
        {"A maintenance dock answers your ping.", "一座维护船坞回应了你的呼叫。"},
        {"Emergency Drydock", "应急船坞"},
        {"A lightly armed station can patch your hull.", "一座轻武装空间站可以为你的船体进行修补。"},
        {"Encrypted trade beacons flare nearby.", "附近亮起了加密交易信标。"},
        {"Anomalous data bleeds across the sensor grid.", "异常数据正渗入你的传感网格。"},
        {"A warship blocks the sector jump.", "一艘战舰挡住了通往下一星区的跃迁航道。"},
        {"Heat wakes ripple through a carrier hull.", "热浪尾迹正沿着一艘航母的船壳翻涌。"},
        {"The final guardian unfolds in the void.", "最终守卫者在虚空中缓缓展开身形。"},
        {"Patch Hull (+10 HP)", "修复船体（+10 生命）"},
        {"Vent Reactor (-6 Heat, +4 Shield)", "排空反应堆（-6 热量，+4 初始护盾）"},
        {"Install Plating (+4 Max HP)", "安装装甲（+4 最大生命）"},
        {"Reliable, clean repairs for a bruised ship.", "为受损飞船提供稳定而干净的修复。"},
        {"Stabilize the core and harden your next engagement.", "稳定核心，让下一场战斗更稳健。"},
        {"Trade time for permanent hull reinforcement.", "花时间换取永久性的船体强化。"},
        {"Strip Scrap", "拆解废料"},
        {"Recover Module Rack", "回收模块架"},
        {"Leave It", "不去碰它"},
        {"Gain immediate scrap, but risk a little reactor heat.", "立刻获得废料，但会冒一点反应堆升温风险。"},
        {"Turn the wreck into a fresh card reward.", "把残骸转化为一组新的卡牌奖励。"},
        {"Avoid the unknown and conserve resources.", "避开未知风险，保留现有资源。"},
        {"A broker crackles through the static with contraband cards and a purge service.", "一名掮客在静电中接通讯号，兜售走私卡牌和移除服务。"},
        {"Purge 1 card", "移除 1 张卡牌"},
        {"Pay 20 scrap to permanently remove one card from your deck.", "支付 20 废料，永久移除卡组中的 1 张牌。"},
        {"Leave the broker", "离开掮客频道"},
        {"Cut the transmission and keep moving.", "切断传输，继续前进。"},
        {"Select one card to erase from your deck. The black market visit ends immediately after the purge.", "选择 1 张卡牌从卡组中彻底抹除。执行移除后，本次黑市访问会立刻结束。"},
        {"More purge targets", "更多可移除目标"},
        {"Cycle to the next page of cards that can be removed.", "切换到下一页可移除卡牌。"},
        {"Abort the purge and close the channel.", "取消移除并关闭频道。"},
        {"Distress Beacon", "求救信标"},
        {"A damaged salvage tug is limping between asteroids.", "一艘受损的打捞拖船正蹒跚穿过小行星带。"},
        {"Aid the tug", "援助拖船"},
        {"Strip the derelict", "拆解残舰"},
        {"Ignore the call", "忽略求救"},
        {"Gain a modest reward and some goodwill.", "获得一份稳定奖励和一点善意。"},
        {"Take a bigger haul, but risk heat from a stressed reactor.", "可以拿到更大的收益，但反应堆会承受额外热量风险。"},
        {"Keep your run clean and your conscience murky.", "继续你的航程，只留下有点复杂的良心。"},
        {"A harmonic transmission keeps syncing to your nav core.", "一段谐波信号不断与你的导航核心同步。"},
        {"Investigate", "深入调查"},
        {"Ride the signal", "借力信号"},
        {"Mute it", "静音处理"},
        {"Dig for blueprints and possible reward cards.", "挖掘蓝图，并可能得到奖励卡牌。"},
        {"Bank an extra energy spike for the next battle.", "为下一场战斗储存一股额外能量尖峰。"},
        {"Stay safe and keep moving.", "保持安全，继续前进。"},
        {"Reactor Leak", "反应堆泄漏"},
        {"A floating refinery vents unstable coolant into the lane.", "一座漂浮炼厂正把不稳定冷却剂排向航道。"},
        {"Vent it clean", "安全排放"},
        {"Harvest unstable fuel", "收集不稳定燃料"},
        {"Seal and patch", "封堵修补"},
        {"Cool the ship and earn a little scrap.", "让飞船冷却下来，并顺带获得少量废料。"},
        {"Cash out hard, but pay in reactor heat.", "收益更高，但要承受更多反应堆热量。"},
        {"A safe choice that restores a little hull.", "稳妥的选择，可以恢复一点船体。"},
        {"Smuggler Cache", "走私者藏匿点"},
        {"A hidden cargo pod blinks with black market encryption.", "一个隐藏货舱正闪烁着黑市加密信号。"},
        {"Force it open", "强行开启"},
        {"Bribe the lock (10 scrap)", "贿赂锁芯（10 废料）"},
        {"Leave it", "离开这里"},
        {"High-risk relic grab with self-inflicted damage.", "高风险获取遗物，但会承受自伤。"},
        {"Trade 10 scrap for a controlled card reward.", "花费 10 废料，换取一次可控的卡牌奖励。"},
        {"No risk, no gain.", "无风险，也无收益。"},
        {"Battle reward", "战斗奖励"},
        {"Elite salvage secured", "精英战利品到手"},
        {"Boss cache recovered", "Boss 宝藏已回收"},
        {"Recovered salvage module", "打捞模块回收成功"},
        {"Signal blueprint recovered", "信号蓝图已回收"},
        {"Smuggler wares", "走私货品"},
        {"Auto-Cooler", "自动冷却器"},
        {"Reactor Core", "反应堆核心"},
        {"Salvage Scanner", "回收扫描仪"},
        {"Emergency Battery", "紧急电池"},
        {"Reinforced Hull", "加固船体"},
        {"Shield Matrix", "护盾矩阵"},
        {"Heat Regulator", "热量调节器"},
        {"Reduce 1 heat at end of each turn", "每回合结束时降低 1 点热量"},
        {"Gain +1 energy on the first turn of each battle", "每场战斗的首回合获得 +1 能量"},
        {"Increase scrap gains from salvage and battle", "提升打捞与战斗获得的废料"},
        {"Survive one fatal hit with 1 HP once per run", "每轮游戏可在致命伤害时以 1 点生命存活一次"},
        {"Increase max HP by 8", "最大生命值提高 8 点"},
        {"Start each battle with 4 shield", "每场战斗开始时获得 4 点护盾"},
        {"Increase max heat by 2", "最大热量上限提高 2 点"},
        {"Run Complete", "本轮完成"},
        {"Ship Lost", "飞船坠毁"},
        {"Flight Manual", "飞行手册"},
        {"Run Ledger", "航行记录"},
        {"Save Complete", "保存成功"},
        {"Save Failed", "保存失败"},
        {"Save Loaded", "读取成功"},
        {"Load Failed", "读取失败"},
        {"No relic recovered this time.", "本次没有获得遗物。"},
        {"No relics installed.", "当前没有安装遗物。"},
        {"No salvage option.", "没有可选战利品。"},
        {"No signal.", "暂无信号。"},
        {"Awaiting the first exchange.", "等待第一轮交锋。"},
        {"Press Esc or click Back to return.", "按 Esc 或点击返回。"},
        {"Press Back to resume.", "点击返回以继续。"},
        {"Press Back to continue.", "点击返回以继续。"},
        {"Press Back to return.", "点击返回。"},
        {"Choose a next step below.", "请选择下一步。"},
        {"Keyboard: 1-9 and 0 choose cards, E ends turn, S saves, L loads, Esc returns.", "键盘：1-9 和 0 选择卡牌，E 结束回合，S 打开存档，L 打开读档，Esc 返回。"},
        {"No recorded runs yet. Win or lose a few voyages first.", "还没有记录。先去跑几次航程吧。"},
        {"Ledger is empty.", "记录为空。"},
        {"Victory", "胜利"},
        {"Defeat", "失败"},
        {"No active run to save.", "当前没有可保存的航程。"},
        {"No save files", "没有存档"},
        {"Unable to open save file for writing.", "无法打开存档文件进行写入。"},
        {"Save file not found.", "未找到存档文件。"},
        {"Save file references an unknown ship.", "存档引用了未知飞船。"},
        {"Save file references an unknown relic.", "存档引用了未知遗物。"},
        {"Save file references an unknown card.", "存档引用了未知卡牌。"},
        {"Save file references an unknown enemy.", "存档引用了未知敌人。"},
        {"No card drawn", "此槽位暂无手牌"},
        {"No module loaded.", "没有装填模块。"},
        {"Empty Slot", "空槽位"},
        {"Unavailable", "不可用"},
        {"Recovered Relic", "回收遗物"},
        {"Installed Relics", "已安装遗物"},
        {"Run Report", "航程报告"},
        {"Transmission", "讯息窗口"},
        {"Signal", "信号内容"},
        {"Ship Status", "飞船状态"},
        {"Your Systems", "我方系统"},
        {"Enemy Intent", "敌方意图"},
        {"Combat Lane", "战斗航道"},
        {"Command Deck", "指令手牌"},
        {"Battle Log", "战斗日志"},
        {"Player Flagship", "玩家旗舰"},
        {"Abyss Contact", "深渊接触目标"},
        {"Main Menu", "主菜单"},
        {"Launch Setup", "出航设置"},
        {"Difficulty", "难度"},
        {"Starter Ship", "初始飞船"},
        {"Run Stats", "本轮状态"},
        {"Battle Bridge", "战斗舰桥"},
    };
    for (const auto& item : kTranslations) {
        if (text == item.first) {
            return item.second;
        }
    }
    if (text == "If you win this battle, gain +6 credits.") return "若本场战斗获胜，额外获得 6 金币。";
    if (text == "Strip Credits") return "拆解金币";
    if (text == "Gain immediate credits, but risk a little reactor heat.") return "立刻获得金币，但会冒一点反应堆升温风险。";
    if (text == "Pay 20 credits to permanently remove one card from your deck.") return "支付 20 金币，永久移除卡组中的 1 张牌。";
    if (text == "Cool the ship and earn a few credits.") return "让飞船冷却下来，并顺带获得少量金币。";
    if (text == "Bribe the lock (10 credits)") return "贿赂锁芯（10 金币）";
    if (text == "Trade 10 credits for a controlled card reward.") return "花费 10 金币，换取一次可控的卡牌奖励。";
    if (text == "Increase credit gains from salvage and battle") return "提升打捞与战斗获得的金币";

    if (StartsWith(text, "Jump vector locked for ") && EndsWith(text, ".")) {
        const std::string name = text.substr(23, text.size() - 24);
        return "跃迁航线已锁定至 " + TranslateGameText(language, name) + "。";
    }
    if (StartsWith(text, "You route ") && EndsWith(text, ".")) {
        const std::string name = text.substr(10, text.size() - 11);
        return "你启动了 " + TranslateGameText(language, name) + "。";
    }
    if (StartsWith(text, "Shield rises by ") && EndsWith(text, ".")) {
        return "护盾提升 " + text.substr(16, text.size() - 17) + " 点。";
    }
    if (StartsWith(text, "Hull restores ") && EndsWith(text, " HP.")) {
        return "船体恢复 " + text.substr(14, text.size() - 18) + " 点生命。";
    }
    if (StartsWith(text, "Reactor gain: +") && EndsWith(text, " energy.")) {
        return "反应堆增益：+" + text.substr(15, text.size() - 23) + " 点能量。";
    }
    if (StartsWith(text, "Next turn energy buffered: +") && EndsWith(text, ".")) {
        return "下回合已储存 +" + text.substr(28, text.size() - 29) + " 点能量。";
    }
    if (StartsWith(text, "Heat climbs by ") && EndsWith(text, ".")) {
        return "热量上升 " + text.substr(15, text.size() - 16) + " 点。";
    }
    if (StartsWith(text, "Heat drops by ") && EndsWith(text, ".")) {
        return "热量下降 " + text.substr(14, text.size() - 15) + " 点。";
    }
    if (StartsWith(text, "Feedback lashes your own hull for ") && EndsWith(text, ".")) {
        return "反馈冲击自身船体，受到 " + text.substr(35, text.size() - 36) + " 点伤害。";
    }
    if (StartsWith(text, "Enemy shield stripped by ") && EndsWith(text, ".")) {
        return "敌方护盾被削减 " + text.substr(25, text.size() - 26) + " 点。";
    }
    if (StartsWith(text, "Battle credit payout increased by ") && EndsWith(text, ".")) {
        return "本场战斗金币奖励提高 " + text.substr(34, text.size() - 35) + " 点。";
    }
    if (StartsWith(text, "Hand full. ") && EndsWith(text, " is routed to discard.")) {
        const std::string name = text.substr(11, text.size() - 33);
        return "手牌已满，" + TranslateGameText(language, name) + " 被送入弃牌堆。";
    }
    if (StartsWith(text, "Stored anomaly charge grants +") && EndsWith(text, " energy.")) {
        return "储存的异常能量给予你 +" + text.substr(29, text.size() - 37) + " 点能量。";
    }
    if (StartsWith(text, "Deflector envelope stabilizes for +") && EndsWith(text, " shield.")) {
        return "偏导护盾稳定成形，获得 +" + text.substr(35, text.size() - 43) + " 点护盾。";
    }
    if (StartsWith(text, "Turn ") && text.find(" begins with ") != std::string::npos && text.find(" energy and ") != std::string::npos &&
        EndsWith(text, " cards in hand.")) {
        const size_t turn_end = text.find(" begins with ");
        const size_t energy_pos = text.find(" energy and ");
        const std::string turn = text.substr(5, turn_end - 5);
        const std::string energy = text.substr(turn_end + 13, energy_pos - (turn_end + 13));
        const std::string cards = text.substr(energy_pos + 12, text.size() - (energy_pos + 27));
        return "第 " + turn + " 回合开始：能量 " + energy + "，手牌 " + cards + " 张。";
    }
    if (StartsWith(text, "Hostile contact acquired: ") && EndsWith(text, ".")) {
        const std::string name = text.substr(26, text.size() - 27);
        return "发现敌方接触目标：" + TranslateGameText(language, name) + "。";
    }
    if (StartsWith(text, "You absorb ") && EndsWith(text, " damage.")) {
        return "你承受了 " + text.substr(11, text.size() - 19) + " 点伤害。";
    }
    if (StartsWith(text, "The EMP will cut ") && EndsWith(text, " energy from your next turn.")) {
        return "EMP 将使你下回合减少 " + text.substr(17, text.size() - 45) + " 点能量。";
    }
    if (StartsWith(text, "Your reactor spikes by ") && EndsWith(text, " heat.")) {
        return "你的反应堆热量上升 " + text.substr(23, text.size() - 29) + " 点。";
    }
    if (StartsWith(text, "Overheat! Feedback burns through the hull for ") && EndsWith(text, ".")) {
        return "过热！反馈灼穿船体，造成 " + text.substr(47, text.size() - 48) + " 点伤害。";
    }
    if (StartsWith(text, "Repair crews patched the hull for ") && EndsWith(text, " HP.")) {
        return "维修小队修复了船体，恢复 " + text.substr(34, text.size() - 38) + " 点生命。";
    }
    if (StartsWith(text, "New plating installed. Max HP increased by ") && EndsWith(text, ".")) {
        return "新装甲已安装，最大生命提高 " + text.substr(42, text.size() - 43) + " 点。";
    }
    if (StartsWith(text, "You strip the wreck for ") && EndsWith(text, " credits.")) {
        return "你拆解了残骸，获得 " + text.substr(24, text.size() - 33) + " 金币。";
    }
    if (StartsWith(text, "The stranded crew transfers ") && EndsWith(text, " credits and patch gel.")) {
        return "被困船员转交了 " + text.substr(29, text.size() - 52) + " 金币与修补胶。";
    }
    if (StartsWith(text, "Enemy destroyed. Choose your salvage reward.")) {
        return "敌舰已摧毁。请选择你的战利品奖励。";
    }
    if (StartsWith(text, "Sector cleared. Your crew patches ") && text.find(" HP while plotting the next route.") != std::string::npos) {
        const size_t hp_pos = text.find(" HP while plotting the next route.");
        return "星区已清空。船员修复了 " + text.substr(34, hp_pos - 34) + " 点生命，并开始规划下一条航线。";
    }
    if (StartsWith(text, "You strip the beacon ship. The haul is rich, but the core runs hot.")) {
        return "你拆解了信标船，收获丰厚，但核心温度也随之升高。";
    }
    if (StartsWith(text, "You align with the signal and bank +1 energy for your next battle.")) {
        return "你与信号对齐，为下一场战斗储存了 +1 能量。";
    }
    if (StartsWith(text, "You vent the leak safely and salvage intact reactor plates.")) {
        return "你安全排出了泄漏物，并回收了完好的反应堆装甲板。";
    }
    if (StartsWith(text, "You harvest unstable fuel. Profit rises, temperature spikes.")) {
        return "你收集了不稳定燃料。收益更高，但温度也暴涨。";
    }
    if (StartsWith(text, "The cache is trapped, but a relic survives the blast.")) {
        return "藏匿点设有陷阱，但仍有一件遗物在爆炸后幸存。";
    }
    if (StartsWith(text, "A hidden crate opens into a module auction.")) {
        return "隐藏货箱打开了，一场模块拍卖开始了。";
    }
    if (StartsWith(text, "You do not have the 10 credits needed to bribe the lock.")) {
        return "你没有足够的 10 金币来贿赂锁芯。";
    }
    if (StartsWith(text, "A surviving module rack can be integrated into your deck.")) {
        return "幸存的模块架可以并入你的卡组。";
    }
    if (StartsWith(text, "You leave the unstable hulk drifting in the dark.")) {
        return "你让那艘不稳定的残骸继续漂浮在黑暗中。";
    }
    if (StartsWith(text, "You break contact with the black market broker.")) {
        return "你中断了与黑市掮客的联络。";
    }
    if (StartsWith(text, "Not enough credits for that transaction.")) {
        return "金币不足，无法完成这笔交易。";
    }
    if (StartsWith(text, "You leave the signal fading behind you.")) {
        return "你任由那道信号在身后渐渐消散。";
    }
    if (StartsWith(text, "Signal decryption exposes a hidden module cache.")) {
        return "信号解密后，暴露出一处隐藏的模块储藏点。";
    }
    if (StartsWith(text, "You cut the feed before it can warp the route.")) {
        return "你在信号扭曲航线前切断了传输。";
    }
    if (StartsWith(text, "Containment seals hold. Your crew stabilizes the ship.")) {
        return "封锁密封维持住了，船员成功稳定了飞船。";
    }
    if (StartsWith(text, "You mark the cache and move on.")) {
        return "你标记了这处藏匿点，然后继续前进。";
    }
    if (StartsWith(text, "Relic purchased and installed.")) {
        return "遗物已购买并安装。";
    }
    if (StartsWith(text, "Module purchased and queued into your discard pile.")) {
        return "模块已购买，并加入你的弃牌堆。";
    }
    if (StartsWith(text, "Enemy cycle resolved. Your turn begins.")) {
        return "敌方回合已结算，现在轮到你行动。";
    }
    if (StartsWith(text, "The broker opens a purge console. Choose one module to erase.")) {
        return "掮客打开了移除终端。请选择 1 张要抹除的模块牌。";
    }
    if (StartsWith(text, "The broker cannot find that module in your active deck.")) {
        return "掮客无法在当前卡组中找到这张模块牌。";
    }
    if (StartsWith(text, "The broker closes the channel.")) {
        return "掮客关闭了交易频道。";
    }
    if (StartsWith(text, "That module cannot be fired right now.")) {
        return "当前无法打出这张模块牌。";
    }
    if (StartsWith(text, "You skip the module reward and keep moving.")) {
        return "你跳过了这次模块奖励，继续向前。";
    }
    if (StartsWith(text, "The reactor vents cleanly. Your next battle starts with +4 shield.")) {
        return "反应堆排热顺畅。你的下一场战斗将以 +4 护盾开始。";
    }
    if (StartsWith(text, "Abyss link stable. Choose a route.")) {
        return "深渊链路稳定。请选择一条航线。";
    }
    if (StartsWith(text, "Run initialized. Chart your first jump.")) {
        return "航程已初始化。规划你的第一次跃迁。";
    }
    if (StartsWith(text, "Save loaded. Navigation restored.")) {
        return "存档已读取，导航恢复。";
    }
    if (StartsWith(text, "No save files are available.")) {
        return "当前没有可读取的存档。";
    }
    if (StartsWith(text, "Purge 1 copy from the deck. Copies owned: ") && EndsWith(text, ".")) {
        return "从卡组中移除 1 张。当前持有：" + text.substr(39, text.size() - 40) + "。";
    }
    if (StartsWith(text, "Purge 1 copy from the deck. Copies owned: ")) {
        const size_t split = text.find(". ", 39);
        if (split != std::string::npos) {
            const std::string copies = text.substr(39, split - 39);
            const std::string remainder = text.substr(split + 2);
            return "从卡组中移除 1 张。当前持有：" + copies + "。" + TranslateGameText(language, remainder);
        }
        if (EndsWith(text, ".")) {
            return "从卡组中移除 1 张。当前持有：" + text.substr(39, text.size() - 40) + "。";
        }
    }
    if (StartsWith(text, "Battle flow: start each turn with up to 5 cards, spend energy to play modules, then end turn with E.")) {
        return "战斗流程：每回合开始时最多抽到 5 张牌，消耗能量打出模块，然后按 E 结束回合。";
    }
    if (StartsWith(text, "Cards can draw extra modules, but your hand caps at 10 cards just like a deckbuilder battle lane.")) {
        return "部分卡牌可以额外抽牌，但你的手牌上限为 10 张，和牌库构筑战斗一样。";
    }
    if (StartsWith(text, "Shield is temporary: it protects you through the enemy turn, then collapses at the start of your next turn.")) {
        return "护盾是临时的：它会在敌方回合中保护你，并在你下个回合开始时消散。";
    }
    if (StartsWith(text, "Heat is dangerous: overheat damages your hull and can reduce your next draw to 4 cards.")) {
        return "热量非常危险：过热会伤害你的船体，并让你下回合只抽 4 张牌。";
    }
    if (StartsWith(text, "Mouse is enabled. You can click menu items, route nodes, cards, and reward choices directly.")) {
        return "鼠标已启用。你可以直接点击菜单项、路线节点、卡牌和奖励选项。";
    }
    if (StartsWith(text, "Save files now keep deck piles, relics, route state, enemy state, rewards, and encounter choices.")) {
        return "现在的存档会保存牌堆、遗物、路线状态、敌人状态、奖励与事件选项。";
    }
    if (StartsWith(text, "Map flow: choose one node per jump, clear three jumps, then defeat a sector boss.")) {
        return "地图流程：每次跃迁选择一个节点，完成三次跳跃后击败该星区 Boss。";
    }
    if (StartsWith(text, "Star Abyss Salvager now runs through the FTXUI gameplay flow instead of the old placeholder help screen.")) {
        return "Star Abyss Salvager 现在已经使用完整的 FTXUI 游戏流程，而不是之前的帮助占位页。";
    }
    if (StartsWith(text, "Result: ")) {
        return "结果：" + TranslateGameText(language, text.substr(8));
    }
    if (StartsWith(text, "Ship: ")) {
        return "飞船：" + TranslateGameText(language, text.substr(6));
    }
    if (StartsWith(text, "Difficulty: ")) {
        return "难度：" + TranslateGameText(language, text.substr(12));
    }
    if (StartsWith(text, "Final sector: ")) {
        return "最终星区：" + text.substr(14);
    }
    if (StartsWith(text, "Credits banked: ")) {
        return "累计金币：" + text.substr(16);
    }
    if (StartsWith(text, "Relics recovered: ")) {
        return "获得遗物数：" + text.substr(18);
    }
    if (EndsWith(text, " has been integrated into your deck.")) {
        const std::string name = text.substr(0, text.size() - 33);
        return TranslateGameText(language, name) + " 已加入你的卡组。";
    }
    if (EndsWith(text, " has been purged from your deck.")) {
        const std::string name = text.substr(0, text.size() - 30);
        return TranslateGameText(language, name) + " 已从你的卡组中移除。";
    }
    if (StartsWith(text, "Hostile contact acquired: ") && EndsWith(text, ".")) {
        const std::string name = text.substr(26, text.size() - 27);
        return "发现敌方目标：" + TranslateGameText(language, name) + "。";
    }
    if (StartsWith(text, "Your remaining hand is discarded.")) {
        return "你的剩余手牌已全部弃置。";
    }
    if (StartsWith(text, "Auto-Cooler bleeds 1 heat from the reactor.")) {
        return "自动冷却器让反应堆额外降低 1 点热量。";
    }
    if (StartsWith(text, "Your temporary shield field dissipates.")) {
        return "你的临时护盾场已经消散。";
    }
    if (EndsWith(text, "'s temporary shield collapses.")) {
        const std::string name = text.substr(0, text.size() - 29);
        return TranslateGameText(language, name) + " 的临时护盾已经崩解。";
    }
    if (text.find(" executes ") != std::string::npos && EndsWith(text, ".")) {
        const size_t pos = text.find(" executes ");
        return TranslateGameText(language, text.substr(0, pos)) + " 发动了 " +
               TranslateGameText(language, text.substr(pos + 10, text.size() - pos - 11)) + "。";
    }
    if (text.find(" gains ") != std::string::npos && EndsWith(text, " shield.")) {
        const size_t pos = text.find(" gains ");
        if (pos != std::string::npos) {
            return TranslateGameText(language, text.substr(0, pos)) + " 获得了 " +
                   text.substr(pos + 7, text.size() - pos - 14) + " 点护盾。";
        }
    }
    if (text.find(" deals ") != std::string::npos && EndsWith(text, ".")) {
        const size_t pos = text.find(" deals ");
        const std::string name = text.substr(0, pos);
        const std::string tail = text.substr(pos + 7, text.size() - pos - 8);
        return TranslateGameText(language, name) + " 造成了 " + tail + " 点伤害。";
    }
    if (text.find(" resolved. ") != std::string::npos && EndsWith(text, " energy remains.")) {
        const size_t pos = text.find(" resolved. ");
        return TranslateGameText(language, text.substr(0, pos)) + " 结算完成。剩余 " +
               text.substr(pos + 10, text.size() - pos - 25) + " 点能量。";
    }
    if (StartsWith(text, "Enemy shield emitters are jammed.")) return "敌方护盾发射器已被干扰封锁。";
    if (StartsWith(text, "Enemy fire-control is weakened.")) return "敌方火控系统已被削弱。";
    if (StartsWith(text, "Target lock acquired. The next attack will strike harder.")) return "目标锁定完成。下一张攻击牌将造成更高伤害。";
    if (StartsWith(text, "Heat is cut in half.")) return "当前热量已减半。";
    if (StartsWith(text, "All heat is vented from the reactor.")) return "反应堆中的全部热量已排空。";
    if (StartsWith(text, "Shield jam active. Enemy plating fails to form.")) return "护盾干扰生效，敌方护甲未能成型。";
    if (StartsWith(text, "The jump spikes the reactor. Power surges through the ship.")) return "跃迁让反应堆剧烈升温，但强大能量也流遍全舰。";
    if (StartsWith(text, "Blind jump locks on target. Massive burst incoming.")) return "盲跳锁定目标，巨额爆发伤害即将命中。";
    if (StartsWith(text, "Blind jump returns with a mirrored shield bloom.")) return "盲跳返回了一层镜像护盾波。";
    if (StartsWith(text, "The jump tears plating free but hands you a fresh tactical line.")) return "跃迁撕裂了部分装甲，但也为你提供了一手新牌。";
    if (StartsWith(text, "Emergency Battery triggers and keeps the ship alive at 1 HP.")) return "紧急电池触发，飞船以 1 点生命幸存。";
    if (StartsWith(text, "Discard pile reshuffled into the draw pile.")) return "弃牌堆已洗回抽牌堆。";
    if (EndsWith(text, " charges for a heavier strike.")) {
        const std::string name = text.substr(0, text.size() - 29);
        return TranslateGameText(language, name) + " 正在蓄力，为更沉重的一击做准备。";
    }

    return text;
}

ButtonOption AccentButton(Color accent) {
    auto option = ButtonOption::Animated();
    option.transform = [accent](const EntryState& state) {
        Element label = text(" " + state.label + " ") | center;
        if (state.active) {
            label = label | color(Color::Black) | bgcolor(accent) | bold;
        } else if (state.focused) {
            label = label | color(Color::White) | bgcolor(Color::RGB(33, 58, 96)) | bold;
        } else {
            label = label | color(accent);
        }
        return label | border;
    };
    return option;
}

std::string MetricText(const std::string& label, int value, int maxValue) {
    return label + " " + std::to_string(value) + "/" + std::to_string(maxValue);
}

Element GaugePanel(const std::string& label, int value, int maxValue, Color gauge_color, Color text_color) {
    const int safe_max = std::max(1, maxValue);
    const float ratio = static_cast<float>(std::max(0, value)) / static_cast<float>(safe_max);
    return window(text(label) | color(text_color),
                  vbox({
                      text(MetricText(label, value, maxValue)) | color(Color::White),
                      gauge(ratio) | color(gauge_color),
                  })) |
           bgcolor(kPanel);
}

Element InfoPanel(UiLanguage language, const std::string& title, const std::vector<std::string>& lines, Color accent) {
    Elements content;
    for (const std::string& line : lines) {
        content.push_back(paragraph(line) | color(Color::White));
    }
    if (content.empty()) {
        content.push_back(text(TranslateGameText(language, "No signal.")) | color(kDim));
    }
    return window(text(title) | color(accent) | bold, vbox(std::move(content))) | bgcolor(kPanelAlt);
}

std::string BattleKeyLabel(size_t index) {
    return index == 9 ? "0" : std::to_string(index + 1);
}

std::string JoinClauses(UiLanguage language, const std::vector<std::string>& clauses) {
    if (clauses.empty()) {
        return "";
    }
    const std::string separator = language == UiLanguage::Chinese ? "；" : "; ";
    std::string result;
    for (size_t i = 0; i < clauses.size(); ++i) {
        if (i > 0) {
            result += separator;
        }
        result += clauses[i];
    }
    result += language == UiLanguage::Chinese ? "。" : ".";
    return result;
}

std::string CardDescriptionText(UiLanguage language, const CardDef& card) {
    std::vector<std::string> clauses;
    if (card.shieldToDamage) {
        clauses.push_back(Tr(language, "Deal damage equal to your current shield, then lose that shield",
                             "造成等同于当前护盾值的伤害，然后失去这些护盾"));
    } else if (card.damage > 0) {
        std::string damage_clause;
        if (card.multiHit > 1) {
            damage_clause = Tr(language, "Deal ", "造成 ") + std::to_string(card.damage) +
                            Tr(language, " damage ", " 点伤害，共 ") + std::to_string(card.multiHit) +
                            Tr(language, " times", " 次");
        } else {
            damage_clause = Tr(language, "Deal ", "造成 ") + std::to_string(card.damage) + Tr(language, " damage", " 点伤害");
        }
        if (card.comboBonus > 0) {
            damage_clause += Tr(language, ", gain +", "；若本回合已打出其他攻击牌，则额外 +") + std::to_string(card.comboBonus) +
                             Tr(language, " damage if another attack was played this turn", " 点伤害");
        }
        clauses.push_back(damage_clause);
    }
    if (card.ignoreShield) clauses.push_back(Tr(language, "Ignore enemy shield", "无视敌方护盾"));
    if (card.shield > 0) clauses.push_back(Tr(language, "Gain ", "获得 ") + std::to_string(card.shield) + Tr(language, " shield", " 点护盾"));
    if (card.heal > 0) clauses.push_back(Tr(language, "Recover ", "恢复 ") + std::to_string(card.heal) + Tr(language, " HP", " 点生命"));
    if (card.draw > 0) clauses.push_back(Tr(language, "Draw ", "抽 ") + std::to_string(card.draw) + Tr(language, " cards", " 张牌"));
    if (card.energy > 0) clauses.push_back(Tr(language, "Gain ", "获得 ") + std::to_string(card.energy) + Tr(language, " energy", " 点能量"));
    if (card.nextTurnEnergy > 0) clauses.push_back(Tr(language, "Next turn, gain ", "下回合获得 ") + std::to_string(card.nextTurnEnergy) +
                                                   Tr(language, " extra energy", " 点额外能量"));
    if (card.heat > 0) clauses.push_back(Tr(language, "Gain ", "增加 ") + std::to_string(card.heat) + Tr(language, " heat", " 点热量"));
    if (card.heat < 0) clauses.push_back(Tr(language, "Reduce ", "降低 ") + std::to_string(-card.heat) + Tr(language, " heat", " 点热量"));
    if (card.selfDamage > 0) clauses.push_back(Tr(language, "Lose ", "失去 ") + std::to_string(card.selfDamage) + Tr(language, " HP", " 点生命"));
    if (card.weakenTurns > 0) clauses.push_back(Tr(language, "Enemy's next ", "敌方接下来 ") + std::to_string(card.weakenTurns) +
                                                Tr(language, " attacks lose 2 damage", " 次攻击各减少 2 点伤害"));
    if (card.removeEnemyShield > 0) clauses.push_back(Tr(language, "Remove enemy shield", "移除敌方护盾"));
    if (card.shieldLockTurns > 0) clauses.push_back(Tr(language, "Enemy cannot gain shield for ", "敌方 ") + std::to_string(card.shieldLockTurns) +
                                                    Tr(language, " turns", " 回合内无法获得护盾"));
    if (card.attackMultiplier > 1) clauses.push_back(Tr(language, "Your next attack deals x", "你的下一张攻击牌伤害变为 x") + std::to_string(card.attackMultiplier));
    if (card.halveHeat) clauses.push_back(Tr(language, "Halve current heat", "将当前热量减半"));
    if (card.clearHeat) clauses.push_back(Tr(language, "Remove all heat", "清空全部热量"));
    if (card.salvageBonus > 0) clauses.push_back(Tr(language, "If you win this battle, gain +", "若本场战斗获胜，额外获得 +") +
                                                 std::to_string(card.salvageBonus) + Tr(language, " credits", " 金币"));
    if (card.randomEffect) clauses.push_back(Tr(language, "Trigger one strong but unpredictable effect", "触发一个强力但不可预测的效果"));

    const std::string generated = JoinClauses(language, clauses);
    return generated.empty() ? TranslateGameText(language, card.description) : generated;
}

Color CardAccent(const CardDef* card) {
    if (card == nullptr) {
        return kDim;
    }
    if (card->type == "Attack") {
        return Color::RGB(255, 120, 120);
    }
    if (card->type == "Defense") {
        return kShield;
    }
    if (card->type == "Control") {
        return kGold;
    }
    if (card->type == "Risk") {
        return Color::RGB(255, 152, 82);
    }
    return kHull;
}

Color CardBackground(const CardDef* card, bool playable) {
    if (card == nullptr) {
        return kPanelAlt;
    }
    if (!playable) {
        return Color::RGB(12, 21, 38);
    }
    if (card->type == "Attack") {
        return Color::RGB(48, 18, 28);
    }
    if (card->type == "Defense") {
        return Color::RGB(18, 28, 58);
    }
    if (card->type == "Control") {
        return Color::RGB(49, 37, 15);
    }
    if (card->type == "Risk") {
        return Color::RGB(54, 29, 12);
    }
    return Color::RGB(13, 34, 49);
}

std::string EnergyPips(int energy, int max_energy) {
    std::string out = "Energy ";
    for (int i = 0; i < max_energy; ++i) {
        out += (i < energy) ? "[]" : "..";
        if (i + 1 < max_energy) {
            out += " ";
        }
    }
    if (energy > max_energy) {
        out += " +" + std::to_string(energy - max_energy);
    }
    return out;
}

std::string IntentSummary(UiLanguage language, const RunState& state) {
    const std::string label = TranslateGameText(language, state.enemy.intentLabel);
    if (state.enemy.intentKind == "attack") {
        const int value = state.enemy.weakenTurns > 0 ? std::max(0, state.enemy.intentValue - 2) : state.enemy.intentValue;
        const std::string suffix = state.enemy.weakenTurns > 0 ? Tr(language, " (weakened)", "（已削弱）") : "";
        return label + " - " + Tr(language, "Attack ", "攻击 ") + std::to_string(value) + suffix;
    }
    if (state.enemy.intentKind == "shield") {
        if (state.enemy.shieldLockTurns > 0) {
            return label + " - " + Tr(language, "Shield blocked by jammer", "护盾已被干扰封锁");
        }
        return label + " - " + Tr(language, "Gain ", "获得 ") + std::to_string(state.enemy.intentValue) + Tr(language, " shield", " 点护盾");
    }
    if (state.enemy.intentKind == "heat") {
        return label + " - " + Tr(language, "Inflict ", "施加 ") + std::to_string(state.enemy.intentValue) + Tr(language, " heat", " 点热量");
    }
    if (state.enemy.intentKind == "emp") {
        return label + " - " + Tr(language, "Next turn -", "下回合 -") + std::to_string(state.enemy.intentValue) +
               Tr(language, " energy", " 能量");
    }
    if (state.enemy.intentKind == "charge") {
        return label + " - " + Tr(language, "Charge and gain ", "蓄力并获得 ") + std::to_string(state.enemy.intentValue) +
               Tr(language, " shield", " 点护盾");
    }
    return label;
}

std::vector<std::string> PlayerArt(const RunState& state, int tick) {
    const std::string flame_a = (tick % 2 == 0) ? "  vv  " : "  !!  ";
    const std::string flame_b = (tick % 2 == 0) ? " v  v " : " !  ! ";
    if (state.player.shipId == "iron_bastion") {
        return {
            "             /\\             ",
            "        ____/##\\____        ",
            "   ____/___/####\\___\\____   ",
            " _/______/########\\______\\_ ",
            "/_______/##########\\_______\\",
            "\\_______\\##########/_______/",
            " \\______/____##____\\______/ ",
            "        /____||____\\        ",
            "          " + flame_a + "          ",
        };
    }
    if (state.player.shipId == "drift_fox") {
        return {
            "              /\\            ",
            "        _____/::\\____       ",
            "   _____/____/::::\\___\\__   ",
            " _/_________/::::::\\_____\\_ ",
            "/__________/::::::::\\______\\",
            "\\__________\\::::::::/______/",
            "      /_____\\::::::/_____\\  ",
            "         /___\\__/___\\       ",
            "          " + flame_b + "         ",
        };
    }
    return {
        "              /\\            ",
        "       ______/::\\______     ",
        "   ___/_____/::::\\_____\\__  ",
        " _/________/::::::\\________\\",
        "/_________/::::::::\\________\\",
        "\\_________\\::::::::/________/",
        "    /______\\::::::/______\\   ",
        "        /___\\__/___\\         ",
        "          " + flame_a + "          ",
    };
}

std::vector<std::string> EnemyArt(const RunState& state, int tick) {
    const bool pulse = tick % 2 == 0;
    if (state.enemy.enemyId == "furnace_carrier") {
        return {
            "          .-========-.      ",
            "      ___/##======##\\\\___  ",
            "   .-<___############___>-. ",
            " _/____/##############\\____\\",
            "<_____\\##############/_____>",
            " \\_____\\____####____/_____/",
            "    \\\\____\\__##__/____//   ",
            pulse ? "        `--.######.--'      " : "        `--.======.--'      ",
        };
    }
    if (state.enemy.enemyId == "abyss_core_sentinel") {
        return {
            "           .-======-.       ",
            "       .--'  _.._  '--.     ",
            "     ./___./====\\.___\\.    ",
            "    /_____/| [][] |\\_____\\ ",
            "    \\_____\\\\_====_//_____/",
            "       \\\\___\\____/___//    ",
            "         '--.____.--'      ",
            pulse ? "          .-::::::-.       " : "          .-......-.       ",
        };
    }
    return {
        "           .-======-.       ",
        "        .-'  _  _  '-.      ",
        "      ./   _/ || \\_   \\.    ",
        "     /___ /_  ||  _\\ ___\\   ",
        "     |   | 0  || 0 |   |    ",
        "     |   |   /__\\  |   |    ",
        "      \\   \\__\\--/__ /   /   ",
        pulse ? "        '--.______.--'      " : "        '--.======.--'      ",
    };
}

std::string RepeatPatternToWidth(const std::string& pattern, int width) {
    if (width <= 0 || pattern.empty()) {
        return "";
    }
    std::string out;
    out.reserve(static_cast<size_t>(width));
    while (static_cast<int>(out.size()) < width) {
        out += pattern;
    }
    out.resize(static_cast<size_t>(width));
    return out;
}

int BoxWidth(const Box& box) {
    if (box.x_max < box.x_min || box.y_max < box.y_min) {
        return 0;
    }
    return box.x_max - box.x_min + 1;
}

int LaneWidthHint(const Box& lane_box) {
    const int reflected_width = BoxWidth(lane_box);
    if (reflected_width > 0) {
        return reflected_width;
    }
    const int terminal_width = Terminal::Size().dimx;
    return std::max(24, terminal_width / 3 - 8);
}

std::vector<std::string> LaneRows(const RunState& state, int tick, int width) {
    width = std::max(24, width);
    const std::string stars[3] = {
        "..  ..   .. ..   ..  .. ..   ..  .. ",
        " .. .. ..   ..  ..   .. ..  .. ..   ",
        ".. ..   .. ..   .. ..   .. ..   ..  ",
    };
    std::vector<std::string> rows = {
        RepeatPatternToWidth(stars[tick % 3], width),
        RepeatPatternToWidth(stars[(tick + 1) % 3], width),
        std::string(width, '.'),
        RepeatPatternToWidth(stars[(tick + 2) % 3], width),
        RepeatPatternToWidth(stars[tick % 3], width),
    };
    if (!state.animation.active) {
        return rows;
    }
    const int progress = (state.animation.duration <= 1)
                             ? width / 2
                             : (state.animation.tick * (width - 5)) / std::max(1, state.animation.duration - 1);
    int offset = state.animation.playerToEnemy ? progress : (width - 5 - progress);
    offset = std::max(0, std::min(width - 5, offset));
    if (state.animation.kind == "attack" || state.animation.kind == "enemy_attack") {
        rows[2].replace(static_cast<size_t>(offset), 5, state.animation.playerToEnemy ? "===>*" : "*<===");
        if (state.animation.tick * 3 >= state.animation.duration * 2) {
            const int impact = state.animation.playerToEnemy ? width - 6 : 0;
            rows[1].replace(static_cast<size_t>(impact), 5, "[**]");
            rows[2].replace(static_cast<size_t>(impact), 5, "<##>");
            rows[3].replace(static_cast<size_t>(impact), 5, "[**]");
        }
    } else if (state.animation.kind == "shield") {
        rows[1].replace(static_cast<size_t>(offset), 5, "[###]");
        rows[3].replace(static_cast<size_t>(offset), 5, "[###]");
    } else if (state.animation.kind == "heat") {
        rows[1].replace(static_cast<size_t>(offset), 5, "^^^^^");
        rows[2].replace(static_cast<size_t>(offset), 5, "<^^^>");
        rows[3].replace(static_cast<size_t>(offset), 5, "vvvvv");
    } else {
        rows[1].replace(static_cast<size_t>(offset), 5, ".::..");
        rows[2].replace(static_cast<size_t>(offset), 5, "<::> ");
        rows[3].replace(static_cast<size_t>(offset), 5, "..::.");
    }
    return rows;
}

std::vector<std::string> CardFacts(UiLanguage language, const CardDef& card) {
    std::vector<std::string> facts;
    if (card.damage > 0) {
        std::string line = Tr(language, "DMG ", "伤害 ") + std::to_string(card.damage);
        if (card.multiHit > 1) {
            line += Tr(language, " x", " x") + std::to_string(card.multiHit);
        }
        if (card.comboBonus > 0) {
            line += Tr(language, ", combo +", "，连击 +") + std::to_string(card.comboBonus);
        }
        facts.push_back(line);
    }
    if (card.shield > 0) facts.push_back(Tr(language, "Shield +", "护盾 +") + std::to_string(card.shield));
    if (card.heal > 0) facts.push_back(Tr(language, "Heal +", "治疗 +") + std::to_string(card.heal));
    if (card.draw > 0) facts.push_back(Tr(language, "Draw +", "抽牌 +") + std::to_string(card.draw));
    if (card.energy > 0) facts.push_back(Tr(language, "Energy +", "能量 +") + std::to_string(card.energy));
    if (card.nextTurnEnergy > 0) facts.push_back(Tr(language, "Next turn +", "下回合 +") + std::to_string(card.nextTurnEnergy) + Tr(language, " energy", " 能量"));
    if (card.heat > 0) facts.push_back(Tr(language, "Heat +", "热量 +") + std::to_string(card.heat));
    else if (card.heat < 0) facts.push_back(Tr(language, "Heat ", "热量 ") + std::to_string(card.heat));
    if (card.selfDamage > 0) facts.push_back(Tr(language, "Self dmg ", "自伤 ") + std::to_string(card.selfDamage));
    if (card.ignoreShield) facts.push_back(Tr(language, "Ignores shield", "无视护盾"));
    if (card.weakenTurns > 0) facts.push_back(Tr(language, "Weaken x", "削弱 x") + std::to_string(card.weakenTurns));
    if (card.shieldLockTurns > 0) facts.push_back(Tr(language, "Jam shield x", "护盾封锁 x") + std::to_string(card.shieldLockTurns));
    if (card.shieldToDamage) facts.push_back(Tr(language, "Convert own shield", "转化自身护盾"));
    if (card.attackMultiplier > 1) facts.push_back(Tr(language, "Next attack x", "下张攻击 x") + std::to_string(card.attackMultiplier));
    if (card.halveHeat) facts.push_back(Tr(language, "Halve heat", "热量减半"));
    if (card.clearHeat) facts.push_back(Tr(language, "Clear heat", "清空热量"));
    if (card.salvageBonus > 0) facts.push_back(Tr(language, "Battle credits +", "战斗金币 +") + std::to_string(card.salvageBonus));
    if (card.randomEffect) facts.push_back(Tr(language, "Volatile effect", "不稳定效果"));
    return facts;
}

std::vector<std::string> PlayerStatusLines(UiLanguage language, const RunState& state) {
    std::vector<std::string> lines = {
        Tr(language, "Hull ", "船体 ") + std::to_string(state.player.hp) + "/" + std::to_string(state.player.maxHp),
        Tr(language, "Heat ", "热量 ") + std::to_string(state.player.heat) + "/" + std::to_string(state.player.maxHeat),
        Tr(language, "Credits ", "金币 ") + std::to_string(state.player.scrap),
        Tr(language, "Deck ", "卡组 ") + std::to_string(state.player.drawPile.size() + state.player.hand.size() + state.player.discardPile.size()) +
            Tr(language, " cards", " 张"),
    };
    if (state.player.nextAttackMultiplier > 1) lines.push_back(Tr(language, "Target lock ready: next attack x", "目标锁定就绪：下一张攻击 x") + std::to_string(state.player.nextAttackMultiplier) + "。");
    if (state.player.salvageBonus > 0) lines.push_back(Tr(language, "Reward bonus +", "奖励金币加成 +") + std::to_string(state.player.salvageBonus) + Tr(language, " credits.", " 金币。"));
    if (state.player.openingEnergyBonus > 0) lines.push_back(Tr(language, "Next battle +", "下场战斗 +") + std::to_string(state.player.openingEnergyBonus) + Tr(language, " opening energy.", " 初始能量。"));
    if (state.player.openingShieldBonus > 0) lines.push_back(Tr(language, "Next battle +", "下场战斗 +") + std::to_string(state.player.openingShieldBonus) + Tr(language, " opening shield.", " 初始护盾。"));
    return lines;
}

std::vector<std::string> BattlePlayerSummaryLines(UiLanguage language, const RunState& state) {
    std::vector<std::string> lines = {
        Tr(language, "Energy ", "能量 ") + EnergyPips(state.player.energy, state.player.maxEnergy).substr(7),
        Tr(language, "Shield ", "护盾 ") + std::to_string(state.player.shield) + "   " + Tr(language, "Heat ", "热量 ") +
            std::to_string(state.player.heat) + "/" + std::to_string(state.player.maxHeat),
        Tr(language, "Hand ", "手牌 ") + std::to_string(state.player.hand.size()) + "   " +
            Tr(language, "Draw ", "抽牌堆 ") + std::to_string(state.player.drawPile.size()) + "   " +
            Tr(language, "Discard ", "弃牌堆 ") + std::to_string(state.player.discardPile.size()),
        Tr(language, "Credits ", "金币 ") + std::to_string(state.player.scrap),
    };
    if (state.player.nextTurnEnergy > 0) {
        lines.push_back(Tr(language, "Buffered next turn energy +", "下回合储能 +") + std::to_string(state.player.nextTurnEnergy));
    }
    if (state.player.nextAttackMultiplier > 1) {
        lines.push_back(Tr(language, "Target lock ready: next attack x", "目标锁定就绪：下一张攻击 x") + std::to_string(state.player.nextAttackMultiplier));
    }
    if (state.player.salvageBonus > 0) {
        lines.push_back(Tr(language, "Reward bonus +", "奖励金币加成 +") + std::to_string(state.player.salvageBonus) +
                        Tr(language, " credits", " 金币"));
    }
    return lines;
}

std::vector<std::string> BattleEnemySummaryLines(UiLanguage language, const RunState& state) {
    std::vector<std::string> lines = {
        Tr(language, "Intent: ", "意图：") + IntentSummary(language, state),
        Tr(language, "Shield ", "护盾 ") + std::to_string(state.enemy.shield),
    };
    if (state.enemy.weakenTurns > 0) lines.push_back(Tr(language, "Weakened attacks remaining: ", "剩余削弱攻击次数：") + std::to_string(state.enemy.weakenTurns));
    if (state.enemy.shieldLockTurns > 0) lines.push_back(Tr(language, "Shield jam remaining: ", "剩余护盾封锁回合：") + std::to_string(state.enemy.shieldLockTurns));
    return lines;
}

Element BattleEntityPanel(const std::string& title,
                          const std::string& hull_label,
                          int hp,
                          int max_hp,
                          Color accent,
                          const std::vector<std::string>& lines) {
    Elements content = {
        text(MetricText(hull_label, hp, max_hp)) | color(Color::White),
        gauge(static_cast<float>(std::max(0, hp)) / static_cast<float>(std::max(1, max_hp))) | color(accent),
        separator(),
    };
    for (const std::string& line : lines) {
        content.push_back(paragraph(line) | color(Color::White));
    }
    return window(text(title) | color(accent) | bold, vbox(std::move(content))) | bgcolor(kPanel);
}

Element ActionPanel(const std::string& title, const std::string& hint, const Component& button, Color accent) {
    return window(text(title) | color(accent) | bold,
                  vbox({
                      button->Render(),
                      separator(),
                      paragraph(hint) | color(kDim),
                  })) |
           flex | bgcolor(kPanelAlt);
}

std::vector<std::string> RelicLines(UiLanguage language, const GameEngine& engine, const RunState& state) {
    std::vector<std::string> lines;
    for (const std::string& relic_id : state.player.relics) {
        lines.push_back(TranslateGameText(language, engine.relicNameForId(relic_id)) + ": " +
                        TranslateGameText(language, engine.relicDescriptionForId(relic_id)));
    }
    if (lines.empty()) lines.push_back(TranslateGameText(language, "No relics installed."));
    return lines;
}

}  // namespace

class StarAbyssApp {
public:
    explicit StarAbyssApp(ScreenInteractive& screen);
    ~StarAbyssApp();

    Component root() const { return root_; }

private:
    void SyncUi();
    bool HandleEvent(Event event);
    void SaveRun();
    void LoadRun();
    void BackFromInfo();
    void OpenSaveDialog(SaveDialogMode mode);
    void CloseSaveDialog();
    void RefreshSaveEntries();
    void SaveToSlot(const std::string& slot_name, bool show_feedback);
    void LoadSelectedSave();
    void AutoSave();
    std::string SanitizeSaveName(const std::string& raw_name) const;
    std::string SaveDisplayName(size_t index) const;

    Element RenderMainMenu() const;
    Element RenderSetup() const;
    Element RenderMap() const;
    Element RenderEncounter() const;
    Element RenderBattle() const;
    Element RenderReward() const;
    Element RenderSummary() const;
    Element RenderInfo() const;
    Element RenderSaveDialog() const;

    ScreenInteractive& screen_;
    GameEngine engine_;
    std::atomic<bool> running_{true};
    std::thread ticker_;
    int activePage_ = 0;
    int difficultyIndex_ = 1;
    int shipIndex_ = 0;
    int ambientTick_ = 0;
    bool infoReturnsToRun_ = false;
    UiLanguage language_ = UiLanguage::English;
    SaveDialogMode saveDialogMode_ = SaveDialogMode::None;
    bool saveDialogVisible_ = false;
    int saveSelected_ = 0;
    std::string saveNameInput_ = "save1";
    mutable Box battleLaneRowsBox_;
    std::vector<std::string> saveEntries_;
    std::vector<std::string> saveSlotNames_;
    std::vector<std::string> savePaths_;

    std::vector<std::string> difficultyEntries_ = {"Easy", "Normal", "Hard"};
    std::vector<std::string> shipEntries_ = {"Vanguard", "Drift Fox", "Iron Bastion"};

    std::array<std::string, 7> menuLabels_{};
    std::array<std::string, kMapSlots> mapLabels_{};
    std::array<std::string, kEncounterSlots> encounterLabels_{};
    std::array<std::string, kBattleSlots> battleCardLabels_{};
    std::array<std::string, kRewardSlots> rewardLabels_{};
    std::string startRunLabel_ = "Launch Run";
    std::string setupBackLabel_ = "Back";
    std::string mapSaveLabel_ = "Save Run";
    std::string mapMenuLabel_ = "Main Menu";
    std::string encounterSaveLabel_ = "Save Run";
    std::string encounterMenuLabel_ = "Main Menu";
    std::string battleEndTurnLabel_ = "End Turn";
    std::string battleSaveLabel_ = "Save Run";
    std::string battleMenuLabel_ = "Main Menu";
    std::string rewardSkipLabel_ = "Skip Card Reward";
    std::string rewardMenuLabel_ = "Main Menu";
    std::string infoBackLabel_ = "Back";
    std::string summaryMenuLabel_ = "Main Menu";
    std::string summaryNewRunLabel_ = "New Run";
    std::string summaryExitLabel_ = "Exit";
    std::string saveConfirmLabel_ = "Save";
    std::string loadConfirmLabel_ = "Load";
    std::string autoSaveLabel_ = "Auto Save";
    std::string closeDialogLabel_ = "Close";

    std::array<Component, 7> menuButtons_{};
    std::array<Component, kMapSlots> mapButtons_{};
    std::array<Component, kEncounterSlots> encounterButtons_{};
    std::array<Component, kBattleSlots> battleCardButtons_{};
    std::array<Component, kRewardSlots> rewardButtons_{};

    Component difficultyToggle_;
    Component shipToggle_;
    Component startRunButton_;
    Component setupBackButton_;
    Component mapSaveButton_;
    Component mapMenuButton_;
    Component encounterSaveButton_;
    Component encounterMenuButton_;
    Component battleEndTurnButton_;
    Component battleSaveButton_;
    Component battleMenuButton_;
    Component rewardSkipButton_;
    Component rewardMenuButton_;
    Component infoBackButton_;
    Component summaryMenuButton_;
    Component summaryNewRunButton_;
    Component summaryExitButton_;
    Component saveNameInputComponent_;
    Component saveEntriesMenu_;
    Component saveConfirmButton_;
    Component loadConfirmButton_;
    Component autoSaveButton_;
    Component closeDialogButton_;

    Component mainMenuPage_;
    Component setupPage_;
    Component mapPage_;
    Component encounterPage_;
    Component battlePage_;
    Component rewardPage_;
    Component summaryPage_;
    Component infoPage_;
    Component saveDialogPage_;
    Component pages_;
    Component root_;
};

StarAbyssApp::StarAbyssApp(ScreenInteractive& screen) : screen_(screen) {
    menuButtons_[0] = Button(&menuLabels_[0], [&] { engine_.resumeRun(); SyncUi(); }, AccentButton(kHull));
    menuButtons_[1] = Button(&menuLabels_[1], [&] { engine_.startAnotherRun(); SyncUi(); }, AccentButton(kGold));
    menuButtons_[2] = Button(&menuLabels_[2], [&] { OpenSaveDialog(SaveDialogMode::Load); SyncUi(); }, AccentButton(kGold));
    menuButtons_[3] = Button(&menuLabels_[3], [&] {
        infoReturnsToRun_ = false;
        engine_.showLeaderboard("data/leaderboard.txt");
        SyncUi();
    }, AccentButton(kHull));
    menuButtons_[4] = Button(&menuLabels_[4], [&] {
        infoReturnsToRun_ = false;
        engine_.showHelp();
        SyncUi();
    }, AccentButton(kHull));
    menuButtons_[5] = Button(&menuLabels_[5], [&] {
        language_ = language_ == UiLanguage::English ? UiLanguage::Chinese : UiLanguage::English;
        SyncUi();
    }, AccentButton(kGold));
    menuButtons_[6] = Button(&menuLabels_[6], [&] { screen_.ExitLoopClosure()(); }, AccentButton(kEnemy));

    for (size_t i = 0; i < mapButtons_.size(); ++i) {
        mapButtons_[i] = Button(&mapLabels_[i], [&, i] { engine_.chooseNode(i); AutoSave(); SyncUi(); }, AccentButton(kHull));
    }
    for (size_t i = 0; i < encounterButtons_.size(); ++i) {
        encounterButtons_[i] = Button(&encounterLabels_[i], [&, i] { engine_.resolveEncounter(i); AutoSave(); SyncUi(); }, AccentButton(kGold));
    }
    for (size_t i = 0; i < rewardButtons_.size(); ++i) {
        rewardButtons_[i] = Button(&rewardLabels_[i], [&, i] { engine_.chooseReward(static_cast<int>(i)); AutoSave(); SyncUi(); }, AccentButton(kGold));
    }
    for (size_t i = 0; i < battleCardButtons_.size(); ++i) {
        battleCardButtons_[i] = Button(&battleCardLabels_[i], [&, i] { engine_.playCard(i); AutoSave(); SyncUi(); }, AccentButton(kHull));
    }

    difficultyToggle_ = Toggle(&difficultyEntries_, &difficultyIndex_);
    shipToggle_ = Toggle(&shipEntries_, &shipIndex_);
    startRunButton_ = Button(&startRunLabel_, [&] {
        const std::array<std::string, 3> ships = {"vanguard", "drift_fox", "iron_bastion"};
        engine_.startNewRun(difficultyIndex_, ships[static_cast<size_t>(shipIndex_)]);
        AutoSave();
        SyncUi();
    }, AccentButton(kGold));
    setupBackButton_ = Button(&setupBackLabel_, [&] { engine_.showMainMenu(); SyncUi(); }, AccentButton(kEnemy));

    mapSaveButton_ = Button(&mapSaveLabel_, [&] { OpenSaveDialog(SaveDialogMode::Save); SyncUi(); }, AccentButton(kGold));
    mapMenuButton_ = Button(&mapMenuLabel_, [&] { engine_.returnToMenu(); SyncUi(); }, AccentButton(kEnemy));
    encounterSaveButton_ = Button(&encounterSaveLabel_, [&] { OpenSaveDialog(SaveDialogMode::Save); SyncUi(); }, AccentButton(kGold));
    encounterMenuButton_ = Button(&encounterMenuLabel_, [&] { engine_.returnToMenu(); SyncUi(); }, AccentButton(kEnemy));
    battleEndTurnButton_ = Button(&battleEndTurnLabel_, [&] { engine_.endTurn(); AutoSave(); SyncUi(); }, AccentButton(kGold));
    battleSaveButton_ = Button(&battleSaveLabel_, [&] { OpenSaveDialog(SaveDialogMode::Save); SyncUi(); }, AccentButton(kGold));
    battleMenuButton_ = Button(&battleMenuLabel_, [&] { engine_.returnToMenu(); SyncUi(); }, AccentButton(kEnemy));
    rewardSkipButton_ = Button(&rewardSkipLabel_, [&] { engine_.chooseReward(-1); AutoSave(); SyncUi(); }, AccentButton(kEnemy));
    rewardMenuButton_ = Button(&rewardMenuLabel_, [&] { engine_.returnToMenu(); SyncUi(); }, AccentButton(kEnemy));
    infoBackButton_ = Button(&infoBackLabel_, [&] { BackFromInfo(); }, AccentButton(kHull));
    summaryMenuButton_ = Button(&summaryMenuLabel_, [&] { engine_.showMainMenu(); SyncUi(); }, AccentButton(kHull));
    summaryNewRunButton_ = Button(&summaryNewRunLabel_, [&] { engine_.startAnotherRun(); SyncUi(); }, AccentButton(kGold));
    summaryExitButton_ = Button(&summaryExitLabel_, [&] { screen_.ExitLoopClosure()(); }, AccentButton(kEnemy));
    saveNameInputComponent_ = Input(&saveNameInput_, "");
    saveEntriesMenu_ = Menu(&saveEntries_, &saveSelected_);
    saveConfirmButton_ = Button(&saveConfirmLabel_, [&] { SaveRun(); SyncUi(); }, AccentButton(kGold));
    loadConfirmButton_ = Button(&loadConfirmLabel_, [&] { LoadRun(); SyncUi(); }, AccentButton(kHull));
    autoSaveButton_ = Button(&autoSaveLabel_, [&] { SaveToSlot("autosave", true); SyncUi(); }, AccentButton(kGold));
    closeDialogButton_ = Button(&closeDialogLabel_, [&] { CloseSaveDialog(); SyncUi(); }, AccentButton(kEnemy));

    mainMenuPage_ = Renderer(Container::Vertical(std::vector<Component>(menuButtons_.begin(), menuButtons_.end())), [&] { return RenderMainMenu(); });
    setupPage_ = Renderer(Container::Vertical({difficultyToggle_, shipToggle_, startRunButton_, setupBackButton_}), [&] { return RenderSetup(); });
    mapPage_ = Renderer(Container::Vertical({mapButtons_[0], mapButtons_[1], mapButtons_[2], mapSaveButton_, mapMenuButton_}), [&] { return RenderMap(); });
    encounterPage_ = Renderer(Container::Vertical({encounterButtons_[0], encounterButtons_[1], encounterButtons_[2], encounterButtons_[3], encounterButtons_[4], encounterSaveButton_, encounterMenuButton_}), [&] {
        return RenderEncounter();
    });
    {
        std::vector<Component> battle_components(battleCardButtons_.begin(), battleCardButtons_.end());
        battle_components.push_back(battleEndTurnButton_);
        battle_components.push_back(battleSaveButton_);
        battle_components.push_back(battleMenuButton_);
        battlePage_ = Renderer(Container::Vertical(std::move(battle_components)), [&] { return RenderBattle(); });
    }
    rewardPage_ = Renderer(Container::Vertical({rewardButtons_[0], rewardButtons_[1], rewardButtons_[2], rewardSkipButton_, rewardMenuButton_}), [&] {
        return RenderReward();
    });
    summaryPage_ = Renderer(Container::Vertical({summaryMenuButton_, summaryNewRunButton_, summaryExitButton_}), [&] { return RenderSummary(); });
    infoPage_ = Renderer(Container::Vertical({infoBackButton_}), [&] { return RenderInfo(); });
    saveDialogPage_ = Renderer(Container::Vertical({saveNameInputComponent_, saveEntriesMenu_, saveConfirmButton_, loadConfirmButton_, autoSaveButton_, closeDialogButton_}),
                               [&] { return RenderSaveDialog(); });

    std::vector<Component> page_list = {
        mainMenuPage_, setupPage_, mapPage_, encounterPage_, battlePage_, rewardPage_, summaryPage_, infoPage_,
    };
    pages_ = Container::Tab(std::move(page_list), &activePage_);
    root_ = CatchEvent(Modal(pages_, saveDialogPage_, &saveDialogVisible_), [&](Event event) { return HandleEvent(event); });

    RefreshSaveEntries();
    SyncUi();
    ticker_ = std::thread([this]() {
        while (running_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(90));
            screen_.Post(Event::Custom);
        }
    });
}

StarAbyssApp::~StarAbyssApp() {
    running_ = false;
    if (ticker_.joinable()) {
        ticker_.join();
    }
}

void StarAbyssApp::SyncUi() {
    const RunState& state = engine_.state();
    activePage_ = static_cast<int>(state.page);
    difficultyEntries_[0] = Tr(language_, "Easy", "简单");
    difficultyEntries_[1] = Tr(language_, "Normal", "普通");
    difficultyEntries_[2] = Tr(language_, "Hard", "困难");
    shipEntries_[0] = TranslateGameText(language_, "Vanguard");
    shipEntries_[1] = TranslateGameText(language_, "Drift Fox");
    shipEntries_[2] = TranslateGameText(language_, "Iron Bastion");
    startRunLabel_ = Tr(language_, "Launch Run", "开始出航");
    setupBackLabel_ = Tr(language_, "Back", "返回");
    mapSaveLabel_ = Tr(language_, "Save Run", "保存航程");
    mapMenuLabel_ = Tr(language_, "Main Menu", "主菜单");
    encounterSaveLabel_ = Tr(language_, "Save Run", "保存航程");
    encounterMenuLabel_ = Tr(language_, "Main Menu", "主菜单");
    battleEndTurnLabel_ = Tr(language_, "End Turn", "结束回合");
    battleSaveLabel_ = Tr(language_, "Save Run", "保存航程");
    battleMenuLabel_ = Tr(language_, "Main Menu", "主菜单");
    rewardSkipLabel_ = Tr(language_, "Skip Card Reward", "跳过卡牌奖励");
    rewardMenuLabel_ = Tr(language_, "Main Menu", "主菜单");
    infoBackLabel_ = Tr(language_, "Back", "返回");
    summaryMenuLabel_ = Tr(language_, "Main Menu", "主菜单");
    summaryNewRunLabel_ = Tr(language_, "New Run", "新航程");
    summaryExitLabel_ = Tr(language_, "Exit", "退出");
    saveConfirmLabel_ = Tr(language_, "Save", "保存");
    loadConfirmLabel_ = Tr(language_, "Load", "读取");
    autoSaveLabel_ = Tr(language_, "Auto Save", "自动存档");
    closeDialogLabel_ = Tr(language_, "Close", "关闭");

    menuLabels_[0] = state.hasRun ? Tr(language_, "Resume Current Run", "继续当前航程")
                                  : Tr(language_, "Resume Current Run (Unavailable)", "继续当前航程（不可用）");
    menuLabels_[1] = Tr(language_, "Start New Run", "开始新航程");
    menuLabels_[2] = Tr(language_, "Load Save", "读取存档");
    menuLabels_[3] = Tr(language_, "Leaderboard", "排行榜");
    menuLabels_[4] = Tr(language_, "Help", "帮助");
    menuLabels_[5] = Tr(language_, "Language: English", "语言：中文");
    menuLabels_[6] = Tr(language_, "Exit", "退出");

    for (std::string& label : mapLabels_) label = TranslateGameText(language_, "Unavailable");
    for (std::string& label : encounterLabels_) label = TranslateGameText(language_, "Unavailable");
    for (std::string& label : battleCardLabels_) label = TranslateGameText(language_, "Empty Slot");
    for (std::string& label : rewardLabels_) label = TranslateGameText(language_, "Unavailable");

    for (size_t i = 0; i < state.routeOptions.size() && i < mapLabels_.size(); ++i) {
        mapLabels_[i] = TranslateGameText(language_, state.routeOptions[i].name);
    }
    for (size_t i = 0; i < state.encounter.choices.size() && i < encounterLabels_.size(); ++i) {
        encounterLabels_[i] = TranslateGameText(language_, state.encounter.choices[i]);
    }
    const std::vector<const CardDef*> hand = engine_.handCards();
    for (size_t i = 0; i < hand.size() && i < battleCardLabels_.size(); ++i) {
        battleCardLabels_[i] = TranslateGameText(language_, hand[i]->name);
    }
    const std::vector<const CardDef*> rewards = engine_.rewardCards();
    for (size_t i = 0; i < rewards.size() && i < rewardLabels_.size(); ++i) {
        rewardLabels_[i] = TranslateGameText(language_, rewards[i]->name);
    }
    if (saveDialogVisible_) {
        RefreshSaveEntries();
    }
}

bool StarAbyssApp::HandleEvent(Event event) {
    if (event == Event::Custom) {
        ++ambientTick_;
        engine_.tickAnimation();
        SyncUi();
        return false;
    }

    if (saveDialogVisible_ && event == Event::Escape) {
        CloseSaveDialog();
        SyncUi();
        return true;
    }
    if (saveDialogVisible_) {
        return false;
    }

    if (event == Event::Escape || event == Event::Character('q')) {
        if (engine_.state().page == AppPage::MainMenu) {
            screen_.ExitLoopClosure()();
        } else if (engine_.state().page == AppPage::Info) {
            BackFromInfo();
        } else {
            engine_.returnToMenu();
            SyncUi();
        }
        return true;
    }

    if (event == Event::Character('s')) {
        OpenSaveDialog(SaveDialogMode::Save);
        SyncUi();
        return true;
    }
    if (event == Event::Character('l')) {
        OpenSaveDialog(SaveDialogMode::Load);
        SyncUi();
        return true;
    }
    if (event == Event::Character('e') && engine_.state().page == AppPage::Battle) {
        engine_.endTurn();
        AutoSave();
        SyncUi();
        return true;
    }

    if (event.is_character()) {
        const std::string chars = event.character();
        if (chars.empty()) return false;
        const char c = chars[0];
        const AppPage page = engine_.state().page;

        if (page == AppPage::Battle && c == '0') {
            engine_.playCard(9);
            AutoSave();
            SyncUi();
            return true;
        }

        if (c >= '1' && c <= '9') {
            const int index = c - '1';
            switch (page) {
            case AppPage::MainMenu:
                if (index == 0) engine_.resumeRun();
                else if (index == 1) engine_.startAnotherRun();
                else if (index == 2) LoadRun();
                else if (index == 3) {
                    infoReturnsToRun_ = false;
                    engine_.showLeaderboard("data/leaderboard.txt");
                } else if (index == 4) {
                    infoReturnsToRun_ = false;
                    engine_.showHelp();
                } else if (index == 5) {
                    language_ = language_ == UiLanguage::English ? UiLanguage::Chinese : UiLanguage::English;
                } else if (index == 6) screen_.ExitLoopClosure()();
                break;
            case AppPage::Map:
                if (static_cast<size_t>(index) < engine_.state().routeOptions.size() && static_cast<size_t>(index) < mapButtons_.size()) {
                    engine_.chooseNode(static_cast<size_t>(index));
                    AutoSave();
                }
                break;
            case AppPage::Encounter:
                if (static_cast<size_t>(index) < engine_.state().encounter.choices.size() && static_cast<size_t>(index) < encounterButtons_.size()) {
                    engine_.resolveEncounter(static_cast<size_t>(index));
                    AutoSave();
                }
                break;
            case AppPage::Battle:
                if (index >= 0 && static_cast<size_t>(index) < kBattleSlots) {
                    engine_.playCard(static_cast<size_t>(index));
                    AutoSave();
                }
                break;
            case AppPage::Reward:
                if (static_cast<size_t>(index) < engine_.state().reward.cardIds.size() && static_cast<size_t>(index) < rewardButtons_.size()) {
                    engine_.chooseReward(index);
                    AutoSave();
                } else if (index == static_cast<int>(engine_.state().reward.cardIds.size())) {
                    engine_.chooseReward(-1);
                    AutoSave();
                }
                break;
            default:
                break;
            }
            SyncUi();
            return true;
        }
    }
    return false;
}

void StarAbyssApp::SaveRun() {
    SaveToSlot(saveNameInput_, true);
}

void StarAbyssApp::LoadRun() {
    LoadSelectedSave();
}

void StarAbyssApp::OpenSaveDialog(SaveDialogMode mode) {
    saveDialogMode_ = mode;
    saveDialogVisible_ = true;
    RefreshSaveEntries();
    if (mode == SaveDialogMode::Save) {
        if (saveNameInput_.empty()) {
            saveNameInput_ = "save1";
        }
    }
    if (mode == SaveDialogMode::Save && !engine_.state().hasRun) {
        saveDialogVisible_ = false;
        engine_.showInfo(Tr(language_, "Save Failed", "保存失败"),
                         {TranslateGameText(language_, "No active run to save.")},
                         Tr(language_, "Press Back to continue.", "点击返回以继续。"));
    }
}

void StarAbyssApp::CloseSaveDialog() {
    saveDialogVisible_ = false;
    saveDialogMode_ = SaveDialogMode::None;
}

std::string StarAbyssApp::SanitizeSaveName(const std::string& raw_name) const {
    std::string cleaned;
    for (char ch : raw_name) {
        if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '_' || ch == '-') {
            cleaned.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
        } else if (ch == ' ' || ch == '.') {
            cleaned.push_back('_');
        }
    }
    while (!cleaned.empty() && cleaned.front() == '_') {
        cleaned.erase(cleaned.begin());
    }
    while (!cleaned.empty() && cleaned.back() == '_') {
        cleaned.pop_back();
    }
    return cleaned.empty() ? "save" : cleaned;
}

void StarAbyssApp::RefreshSaveEntries() {
    namespace fs = std::filesystem;
    saveEntries_.clear();
    saveSlotNames_.clear();
    savePaths_.clear();
    fs::create_directories("save");

    std::vector<fs::path> paths;
    for (const auto& entry : fs::directory_iterator("save")) {
        if (!entry.is_regular_file()) {
            continue;
        }
        const fs::path path = entry.path();
        const std::string extension = path.extension().string();
        if (extension == ".sav" || extension == ".txt") {
            paths.push_back(path);
        }
    }
    std::sort(paths.begin(), paths.end(), [](const fs::path& lhs, const fs::path& rhs) {
        const bool lhs_autosave = lhs.stem().string() == "autosave";
        const bool rhs_autosave = rhs.stem().string() == "autosave";
        if (lhs_autosave != rhs_autosave) {
            return lhs_autosave;
        }
        return lhs.filename().string() < rhs.filename().string();
    });

    for (const fs::path& path : paths) {
        const std::string slot_name = path.stem().string();
        savePaths_.push_back(path.string());
        saveSlotNames_.push_back(slot_name);
        saveEntries_.push_back(slot_name == "autosave" ? Tr(language_, "autosave (automatic)", "autosave（自动）") : slot_name);
    }
    if (saveEntries_.empty()) {
        saveEntries_.push_back(TranslateGameText(language_, "No save files"));
    }
    saveSelected_ = std::max(0, std::min(saveSelected_, static_cast<int>(saveEntries_.size()) - 1));
    if (!saveSlotNames_.empty() && saveDialogMode_ == SaveDialogMode::Save && saveNameInput_.empty()) {
        saveNameInput_ = saveSlotNames_[static_cast<size_t>(saveSelected_)];
    }
}

std::string StarAbyssApp::SaveDisplayName(size_t index) const {
    if (index >= saveSlotNames_.size()) {
        return "";
    }
    const std::string& entry = saveSlotNames_[index];
    if (entry == "autosave") {
        return Tr(language_, "autosave (automatic)", "autosave（自动）");
    }
    return entry;
}

void StarAbyssApp::SaveToSlot(const std::string& slot_name, bool show_feedback) {
    if (!engine_.state().hasRun) {
        if (show_feedback) {
            engine_.showInfo(Tr(language_, "Save Failed", "保存失败"),
                             {TranslateGameText(language_, "No active run to save.")},
                             Tr(language_, "Press Back to continue.", "点击返回以继续。"));
        }
        return;
    }

    namespace fs = std::filesystem;
    fs::create_directories("save");
    const std::string sanitized = SanitizeSaveName(slot_name);
    const std::string path = (fs::path("save") / (sanitized + ".sav")).string();
    std::string error;
    if (engine_.saveToFile(path, &error)) {
        RefreshSaveEntries();
        saveNameInput_ = sanitized;
        if (show_feedback) {
            infoReturnsToRun_ = engine_.state().hasRun;
            engine_.showInfo(Tr(language_, "Save Complete", "保存成功"),
                             {Tr(language_, "Run saved to ", "存档已写入 ") + path + "。"},
                             Tr(language_, "Press Back to resume.", "点击返回以继续。"));
        }
        CloseSaveDialog();
    } else if (show_feedback) {
        engine_.showInfo(Tr(language_, "Save Failed", "保存失败"),
                         {TranslateGameText(language_, error)},
                         Tr(language_, "Press Back to continue.", "点击返回以继续。"));
    }
}

void StarAbyssApp::LoadSelectedSave() {
    if (savePaths_.empty()) {
        engine_.showInfo(Tr(language_, "Load Failed", "读取失败"),
                         {Tr(language_, "No save files are available.", "当前没有可读取的存档。")},
                         Tr(language_, "Press Back to return.", "点击返回。"));
        return;
    }
    if (saveSelected_ < 0 || static_cast<size_t>(saveSelected_) >= savePaths_.size()) {
        return;
    }
    const bool had_run = engine_.state().hasRun;
    std::string error;
    if (engine_.loadFromFile(savePaths_[static_cast<size_t>(saveSelected_)], &error)) {
        infoReturnsToRun_ = true;
        engine_.showInfo(Tr(language_, "Save Loaded", "读取成功"),
                         {Tr(language_, "Run restored from ", "已从 ") + savePaths_[static_cast<size_t>(saveSelected_)] + " " +
                              Tr(language_, "loaded.", "恢复航程。")},
                         Tr(language_, "Press Back to resume.", "点击返回以继续。"));
        CloseSaveDialog();
    } else {
        infoReturnsToRun_ = had_run;
        engine_.showInfo(Tr(language_, "Load Failed", "读取失败"),
                         {TranslateGameText(language_, error)},
                         Tr(language_, "Press Back to return.", "点击返回。"));
    }
}

void StarAbyssApp::AutoSave() {
    if (!engine_.state().hasRun) {
        return;
    }
    SaveToSlot("autosave", false);
}

void StarAbyssApp::BackFromInfo() {
    if (infoReturnsToRun_ && engine_.state().hasRun) engine_.resumeRun();
    else engine_.showMainMenu();
    SyncUi();
}

Element StarAbyssApp::RenderMainMenu() const {
    Elements player_lines;
    Elements enemy_lines;
    for (const std::string& line : PlayerArt(engine_.state(), ambientTick_)) player_lines.push_back(text(line) | color(kHull));
    for (const std::string& line : EnemyArt(engine_.state(), ambientTick_)) enemy_lines.push_back(text(line) | color(kEnemy));

    return vbox({
               text(Tr(language_, "STAR ABYSS SALVAGER", "星渊打捞者")) | bold | color(kHull) | center,
               text(Tr(language_, "Terminal roguelike salvage bridge", "终端肉鸽舰桥")) | color(kGold) | center,
               separatorDouble(),
               hbox({
                   window(text(Tr(language_, "Player Flagship", "玩家旗舰")) | color(kHull), vbox(std::move(player_lines))) | flex | bgcolor(kPanel),
                   window(text(Tr(language_, "Abyss Contact", "深渊接触目标")) | color(kEnemy), vbox(std::move(enemy_lines))) | flex | bgcolor(kPanel),
               }),
               separator(),
               window(text(Tr(language_, "Main Menu", "主菜单")) | color(kGold),
                      vbox({
                          paragraph(Tr(language_,
                                       "Chart a route through collapsing sectors, build a stronger module deck, and bring your ship home from the abyss.",
                                       "在崩塌的星区中规划航线，构筑更强的模块卡组，把你的飞船从深渊中带回家。")) |
                              color(Color::White),
                          separator(),
                          menuButtons_[0]->Render(),
                          menuButtons_[1]->Render(),
                          menuButtons_[2]->Render(),
                          menuButtons_[3]->Render(),
                          menuButtons_[4]->Render(),
                          menuButtons_[5]->Render(),
                          menuButtons_[6]->Render(),
                      })) | bgcolor(kPanel),
               text(Tr(language_, "Current language: English", "当前语言：中文")) | color(kHull) | center,
           }) |
           borderDouble | bgcolor(kAbyss);
}

Element StarAbyssApp::RenderSetup() const {
    return vbox({
               text(Tr(language_, "Launch Setup", "出航设置")) | bold | color(kGold) | center,
               text(Tr(language_, "Pick difficulty and hull profile before diving in.", "在跃入深渊前，先选择难度与舰体。")) | color(kHull) | center,
               separatorDouble(),
               window(text(Tr(language_, "Difficulty", "难度")) | color(kHull), difficultyToggle_->Render()) | bgcolor(kPanel),
               window(text(Tr(language_, "Starter Ship", "初始飞船")) | color(kGold), shipToggle_->Render()) | bgcolor(kPanel),
               separator(),
               hbox({startRunButton_->Render() | flex, setupBackButton_->Render() | flex}),
           }) |
           borderDouble | bgcolor(kAbyss);
}

Element StarAbyssApp::RenderMap() const {
    const RunState& state = engine_.state();
    const int deck_size = static_cast<int>(state.player.drawPile.size() + state.player.hand.size() + state.player.discardPile.size());
    Elements options;
    for (size_t i = 0; i < mapButtons_.size(); ++i) {
        const bool active = i < state.routeOptions.size();
        const std::string detail = active ? TranslateGameText(language_, state.routeOptions[i].detail)
                                          : Tr(language_, "No route available.", "没有可选航线。");
        const std::string danger = active ? (Tr(language_, "Danger ", "危险度 ") + std::to_string(state.routeOptions[i].danger))
                                          : Tr(language_, "Offline", "离线");
        options.push_back(window(text(std::to_string(i + 1) + ". " + mapLabels_[i]) | color(kGold),
                                 vbox({
                                     mapButtons_[i]->Render(),
                                     separator(),
                                     paragraph(detail) | color(Color::White),
                                     text(danger) | color(kEnemy),
                                 })) |
                          bgcolor(kPanel));
    }

    return vbox({
               text(Tr(language_, "Sector ", "星区 ") + std::to_string(state.sector) + "  |  " +
                    Tr(language_, "Route ", "航段 ") + std::to_string(std::min(state.depth + 1, state.nodesPerSector + 1))) |
                   bold | color(kHull) | center,
               hbox({
                   GaugePanel(Tr(language_, "Hull", "船体"), state.player.hp, state.player.maxHp, kHull, kHull) | flex,
                   GaugePanel(Tr(language_, "Heat", "热量"), state.player.heat, state.player.maxHeat, kHeat, kHeat) | flex,
                   window(text(Tr(language_, "Run Stats", "本轮状态")) | color(kGold),
                          vbox({
                              text(Tr(language_, "Credits ", "金币 ") + std::to_string(state.player.scrap)) | color(Color::White),
                              text(Tr(language_, "Deck ", "卡组 ") + std::to_string(deck_size) + Tr(language_, " cards", " 张")) | color(Color::White),
                              text(Tr(language_, "Relics ", "遗物 ") + std::to_string(state.player.relics.size())) | color(Color::White),
                          })) |
                       bgcolor(kPanel) | flex,
               }),
               separatorDouble(),
               hbox({
                   vbox(std::move(options)) | flex,
                   InfoPanel(language_, Tr(language_, "Installed Relics", "已安装遗物"), RelicLines(language_, engine_, state), kHull) | flex,
               }),
               separator(),
               text(TranslateGameText(language_, state.statusLine)) | color(kGold),
               hbox({mapSaveButton_->Render() | flex, mapMenuButton_->Render() | flex}),
           }) |
           borderDouble | bgcolor(kAbyss);
}

Element StarAbyssApp::RenderEncounter() const {
    const RunState& state = engine_.state();
    const size_t visible_options = std::min(state.encounter.choices.size(), encounterButtons_.size());
    Elements options;
    for (size_t i = 0; i < visible_options; ++i) {
        const std::string payload = i < state.encounter.payloadIds.size() ? state.encounter.payloadIds[i] : "";
        const int price = i < state.encounter.payloadValues.size() ? state.encounter.payloadValues[i] : 0;
        const std::string detail = i < state.encounter.details.size() ? TranslateGameText(language_, state.encounter.details[i])
                                                                      : TranslateGameText(language_, "Unavailable");

        Elements body = {encounterButtons_[i]->Render(), separator()};
        if (state.encounter.typeId == "market" && StartsWith(payload, "card:")) {
            const CardDef* card = engine_.cardById(payload.substr(5));
            body.push_back(text(Tr(language_, "Card Offer", "卡牌商品")) | color(kHull));
            body.push_back(text(Tr(language_, "Price ", "售价 ") + std::to_string(price) + Tr(language_, " credits", " 金币")) | color(kGold));
            body.push_back(text(Tr(language_, "Card Cost: ", "卡牌费用：") + std::to_string(card != nullptr ? card->cost : 0) + "   " + Tr(language_, "Type: ", "类型：") +
                                (card != nullptr ? TranslateGameText(language_, card->type) : TranslateGameText(language_, "Unavailable"))) |
                           color(kDim));
            body.push_back(separator());
            body.push_back(paragraph(card != nullptr ? CardDescriptionText(language_, *card) : detail) | color(Color::White));
            if (card != nullptr) {
                body.push_back(separator());
                for (const std::string& fact : CardFacts(language_, *card)) {
                    body.push_back(text(fact) | color(kDim));
                }
            }
        } else if (state.encounter.typeId == "market" && payload == "purge") {
            body.push_back(text(Tr(language_, "Service", "商店服务")) | color(kHull));
            body.push_back(text(Tr(language_, "Price ", "售价 ") + std::to_string(price) + Tr(language_, " credits", " 金币")) | color(kGold));
            body.push_back(separator());
            body.push_back(paragraph(detail) | color(Color::White));
        } else if (state.encounter.typeId == "market" && !payload.empty() && payload != "leave" && payload != "market_purge_cycle") {
            const CardDef* card = engine_.cardById(payload);
            body.push_back(text(Tr(language_, "Purge Service", "移除服务")) | color(kEnemy));
            body.push_back(text(Tr(language_, "Price ", "售价 ") + std::to_string(price) + Tr(language_, " credits", " 金币")) | color(kGold));
            body.push_back(separator());
            body.push_back(paragraph(detail) | color(Color::White));
            if (card != nullptr) {
                body.push_back(separator());
                body.push_back(text(Tr(language_, "Target card: ", "目标卡牌：") + TranslateGameText(language_, card->name)) | color(kEnemy));
                body.push_back(paragraph(CardDescriptionText(language_, *card)) | color(kDim));
            }
        } else if (state.encounter.typeId == "market" && payload == "market_purge_cycle") {
            body.push_back(text(Tr(language_, "Browse More", "查看更多")) | color(kHull));
            body.push_back(separator());
            body.push_back(paragraph(detail) | color(Color::White));
        } else {
            body.push_back(paragraph(detail) | color(Color::White));
        }

        options.push_back(window(text(std::to_string(i + 1) + ". " + encounterLabels_[i]) | color(kGold), vbox(std::move(body))) | bgcolor(kPanel));
    }

    if (state.encounter.typeId == "market") {
        std::vector<std::string> broker_lines = {
            Tr(language_, "Credits on hand: ", "当前金币：") + std::to_string(state.player.scrap),
            Tr(language_, "Card offers cost 28 credits each.", "每张卡牌售价 28 金币。"),
            Tr(language_, "Purge service costs 20 credits and removes 1 card.", "移除服务价格 20 金币，每次只能移除 1 张牌。"),
        };
        if (state.encounter.encounterId == "market_purge") {
            broker_lines.push_back(Tr(language_, "Pick a target card, or cycle to the next page.", "选择要移除的卡牌，或切换到下一页。"));
        } else {
            broker_lines.push_back(Tr(language_, "Buy a card, purge a card, or leave the channel.", "购买卡牌、移除卡牌，或直接离开频道。"));
        }
        return vbox({
                   text(TranslateGameText(language_, state.encounter.title)) | bold | color(kGold) | center,
                   separatorDouble(),
                   hbox({
                       window(text(Tr(language_, "Market Feed", "商店讯号")) | color(kHull),
                              paragraph(TranslateGameText(language_, state.encounter.description)) | color(Color::White)) |
                           flex | bgcolor(kPanel),
                       InfoPanel(language_, Tr(language_, "Broker Ledger", "商店账本"), broker_lines, kGold) | flex,
                   }),
                   separator(),
                   vbox(std::move(options)),
                   separator(),
                   text(TranslateGameText(language_, state.statusLine)) | color(kHull),
                   hbox({encounterSaveButton_->Render() | flex, encounterMenuButton_->Render() | flex}),
               }) |
               borderDouble | bgcolor(kAbyss);
    }

    return vbox({
               text(TranslateGameText(language_, state.encounter.title)) | bold | color(kGold) | center,
               separatorDouble(),
               hbox({
                   window(text(Tr(language_, "Signal", "信号内容")) | color(kHull),
                          paragraph(TranslateGameText(language_, state.encounter.description)) | color(Color::White)) |
                       flex | bgcolor(kPanel),
                   InfoPanel(language_, Tr(language_, "Ship Status", "飞船状态"), PlayerStatusLines(language_, state), kHull) | flex,
               }),
               separator(),
               vbox(std::move(options)),
               separator(),
               text(TranslateGameText(language_, state.statusLine)) | color(kHull),
               hbox({encounterSaveButton_->Render() | flex, encounterMenuButton_->Render() | flex}),
           }) |
           borderDouble | bgcolor(kAbyss);
}

Element StarAbyssApp::RenderBattle() const {
    const RunState& state = engine_.state();
    Elements ship_lines;
    Elements enemy_lines;
    Elements lane_lines;
    for (const std::string& line : PlayerArt(state, ambientTick_)) ship_lines.push_back(text(line) | color(kHull));
    for (const std::string& line : EnemyArt(state, ambientTick_)) enemy_lines.push_back(text(line) | color(kEnemy));
    for (const std::string& line : LaneRows(state, ambientTick_, LaneWidthHint(battleLaneRowsBox_))) {
        lane_lines.push_back(text(line) | color(kGold));
    }

    Elements log_lines;
    const size_t log_start = state.battleLog.size() > 8 ? state.battleLog.size() - 8 : 0;
    for (size_t i = log_start; i < state.battleLog.size(); ++i) {
        log_lines.push_back(text(TranslateGameText(language_, state.battleLog[i])) | color(Color::White));
    }
    if (log_lines.empty()) log_lines.push_back(text(TranslateGameText(language_, "Awaiting the first exchange.")) | color(kDim));

    const std::vector<const CardDef*> hand = engine_.handCards();
    Elements top_cards;
    Elements bottom_cards;
    const size_t visible_slots = std::max<size_t>(5, std::min(hand.size(), battleCardButtons_.size()));
    for (size_t i = 0; i < visible_slots; ++i) {
        const CardDef* card = i < hand.size() ? hand[i] : nullptr;
        const bool playable = card != nullptr && engine_.canPlayCard(i);
        std::vector<std::string> facts = card != nullptr ? CardFacts(language_, *card)
                                                         : std::vector<std::string>{TranslateGameText(language_, "No module loaded.")};
        Elements fact_lines;
        for (const std::string& fact : facts) {
            fact_lines.push_back(text(fact) | color(card != nullptr ? kDim : Color::RGB(84, 93, 112)));
        }
        const Color accent = CardAccent(card);
        Element panel = window(
            text(BattleKeyLabel(i) + ". " + (card != nullptr ? TranslateGameText(language_, card->name)
                                                             : TranslateGameText(language_, "Empty Slot"))) |
                color(card != nullptr ? (playable ? accent : kDim) : kDim) | bold,
            vbox({
                battleCardButtons_[i]->Render(),
                hbox({
                    text(Tr(language_, "Cost ", "费用 ") + std::to_string(card != nullptr ? card->cost : 0)) |
                        color(card != nullptr ? accent : kDim),
                    filler(),
                    text(card != nullptr ? TranslateGameText(language_, card->type) : TranslateGameText(language_, "No card drawn")) |
                        color(card != nullptr ? (playable ? kHull : kDim) : kDim),
                }),
                separator(),
                paragraph(card != nullptr ? CardDescriptionText(language_, *card)
                                          : Tr(language_, "Draw cards and route power to fill this slot.", "抽牌并调度能量来填满这个槽位。")) |
                    color(card != nullptr ? Color::White : kDim),
                separator(),
                vbox(std::move(fact_lines)),
            })) |
                        bgcolor(CardBackground(card, playable)) |
                        size(WIDTH, EQUAL, 22) |
                        size(HEIGHT, EQUAL, 12);
        if (i < 5) top_cards.push_back(panel);
        else bottom_cards.push_back(panel);
    }

    Elements command_deck_rows;
    command_deck_rows.push_back(hbox(std::move(top_cards)));
    if (!bottom_cards.empty()) {
        command_deck_rows.push_back(separator());
        command_deck_rows.push_back(hbox(std::move(bottom_cards)));
    }

    return vbox({
               text(Tr(language_, "Battle Bridge", "战斗舰桥")) | bold | color(kEnemy) | center,
               text(Tr(language_, "Turn ", "回合 ") + std::to_string(std::max(1, state.battleTurn)) + "  |  " +
                    Tr(language_, "Play cards, then press E to end turn", "出牌后按 E 结束回合")) |
                   color(kGold) | center,
               separatorDouble(),
               hbox({
                   BattleEntityPanel(Tr(language_, "Ship Systems", "舰船系统"),
                                     Tr(language_, "Hull", "船体"),
                                     state.player.hp,
                                     state.player.maxHp,
                                     kHull,
                                     BattlePlayerSummaryLines(language_, state)) |
                       flex,
                   BattleEntityPanel(Tr(language_, "Enemy Intent", "敌方意图"),
                                     Tr(language_, "Enemy Hull", "敌方船体"),
                                     state.enemy.hp,
                                     state.enemy.maxHp,
                                     kEnemy,
                                     BattleEnemySummaryLines(language_, state)) |
                       flex,
               }),
               separator(),
               hbox({
                   window(text(TranslateGameText(language_, state.player.shipName)) | color(kHull), vbox(std::move(ship_lines))) | flex | bgcolor(kPanel),
                   window(text(Tr(language_, "Combat Lane", "战斗航道")) | color(kGold),
                          vbox({
                              vbox(std::move(lane_lines)) | xflex | reflect(battleLaneRowsBox_),
                              separator(),
                              text(state.animation.active
                                       ? (Tr(language_, "Animation: ", "动画：") + TranslateGameText(language_, state.animation.label))
                                       : Tr(language_, "Animation: idle drift", "动画：待机漂移")) |
                                  color(Color::White),
                          })) |
                       flex | bgcolor(kPanel),
                   window(text(TranslateGameText(language_, state.enemy.name)) | color(kEnemy), vbox(std::move(enemy_lines))) | flex | bgcolor(kPanel),
               }),
               separator(),
               window(text(Tr(language_, "Command Deck", "指令手牌")) | color(kGold),
                      vbox(std::move(command_deck_rows))) | bgcolor(kPanelAlt),
               separator(),
               hbox({
                   ActionPanel(Tr(language_, "[E] End Turn", "[E] 结束回合"),
                               Tr(language_, "Resolve your remaining plays and pass to the enemy.", "结算本回合剩余操作，然后交给敌方行动。"),
                               battleEndTurnButton_,
                               kGold),
                   ActionPanel(Tr(language_, "[S] Save Run", "[S] 保存航程"),
                               Tr(language_, "Create a manual save or update the autosave slot.", "创建手动存档，或更新自动存档槽。"),
                               battleSaveButton_,
                               kHull),
                   ActionPanel(Tr(language_, "[Esc] Main Menu", "[Esc] 主菜单"),
                               Tr(language_, "Leave the battle screen and return to the main menu.", "离开战斗界面并返回主菜单。"),
                               battleMenuButton_,
                               kEnemy),
               }),
               window(text(Tr(language_, "Battle Log", "战斗日志")) | color(kHull), vbox(std::move(log_lines))) | bgcolor(kPanel),
               text(TranslateGameText(language_, state.statusLine)) | color(Color::White),
           }) |
           borderDouble | bgcolor(kAbyss);
}

Element StarAbyssApp::RenderReward() const {
    const RunState& state = engine_.state();
    const std::vector<const CardDef*> reward_cards = engine_.rewardCards();
    Elements cards;
    for (size_t i = 0; i < rewardButtons_.size(); ++i) {
        const CardDef* card = i < reward_cards.size() ? reward_cards[i] : nullptr;
        Elements facts;
        if (card != nullptr) {
            for (const std::string& fact : CardFacts(language_, *card)) facts.push_back(text(fact) | color(kDim));
        } else {
            facts.push_back(text(TranslateGameText(language_, "No salvage option.")) | color(kDim));
        }
        cards.push_back(window(text(std::to_string(i + 1) + ". " +
                                    (card != nullptr ? (Tr(language_, "[Card Reward] ", "[卡牌奖励] ") + rewardLabels_[i]) : rewardLabels_[i])) |
                               color(kGold),
                               vbox({
                                   rewardButtons_[i]->Render(),
                                   separator(),
                                   text(card != nullptr
                                            ? (Tr(language_, "Cost ", "费用 ") + std::to_string(card->cost) + "   " +
                                               Tr(language_, "Type: ", "类型：") + TranslateGameText(language_, card->type))
                                            : Tr(language_, "No card offered in this slot.", "这个槽位没有可选卡牌。")) |
                                       color(card != nullptr ? kHull : kDim),
                                   separator(),
                                   paragraph(card != nullptr ? CardDescriptionText(language_, *card)
                                                             : TranslateGameText(language_, "Unavailable")) |
                                       color(Color::White),
                                   separator(),
                                   vbox(std::move(facts)),
                               })) |
                        bgcolor(kPanel));
    }
    std::vector<std::string> relic_lines;
    if (!state.reward.relicId.empty()) {
        relic_lines.push_back(TranslateGameText(language_, engine_.relicNameForId(state.reward.relicId)));
        relic_lines.push_back(TranslateGameText(language_, engine_.relicDescriptionForId(state.reward.relicId)));
    } else {
        relic_lines.push_back(Tr(language_, "This reward contains cards only.", "本次奖励只有卡牌，没有遗物。"));
    }
    return vbox({
               text(TranslateGameText(language_, state.reward.banner)) | bold | color(kGold) | center,
               text(state.reward.relicId.empty()
                        ? Tr(language_, "Reward type: Card reward", "奖励类型：卡牌奖励")
                        : Tr(language_, "Reward type: Card reward + relic reward", "奖励类型：卡牌奖励 + 遗物奖励")) |
                   color(Color::White) | center,
               text(Tr(language_, "Credits gained: ", "获得金币：") + std::to_string(state.reward.scrap)) | color(kHull) | center,
               text(Tr(language_, "Current credits: ", "当前金币：") + std::to_string(state.player.scrap)) | color(kGold) | center,
               separatorDouble(),
               hbox({vbox(std::move(cards)) | flex, InfoPanel(language_, Tr(language_, "Relic Reward", "遗物奖励"), relic_lines, kEnemy) | flex}),
               separator(),
               hbox({rewardSkipButton_->Render() | flex, rewardMenuButton_->Render() | flex}),
               text(TranslateGameText(language_, state.statusLine)) | color(Color::White),
           }) |
           borderDouble | bgcolor(kAbyss);
}

Element StarAbyssApp::RenderSummary() const {
    const RunState& state = engine_.state();
    Elements lines;
    for (const std::string& line : state.info.lines) lines.push_back(text(TranslateGameText(language_, line)) | color(Color::White));
    return vbox({
               text(TranslateGameText(language_, state.info.title)) | bold | color(kGold) | center,
               separatorDouble(),
               window(text(Tr(language_, "Run Report", "航程报告")) | color(kHull), vbox(std::move(lines))) | bgcolor(kPanel),
               hbox({summaryMenuButton_->Render() | flex, summaryNewRunButton_->Render() | flex, summaryExitButton_->Render() | flex}),
           }) |
           borderDouble | bgcolor(kAbyss);
}

Element StarAbyssApp::RenderInfo() const {
    const RunState& state = engine_.state();
    Elements lines;
    for (const std::string& line : state.info.lines) lines.push_back(paragraph(TranslateGameText(language_, line)) | color(Color::White));
    return vbox({
               text(TranslateGameText(language_, state.info.title)) | bold | color(kHull) | center,
               separatorDouble(),
               window(text(Tr(language_, "Transmission", "讯息窗口")) | color(kGold), vbox(std::move(lines))) | bgcolor(kPanel),
               separator(),
               text(TranslateGameText(language_, state.info.footer)) | color(kGold),
               infoBackButton_->Render(),
           }) |
           borderDouble | bgcolor(kAbyss);
}

Element StarAbyssApp::RenderSaveDialog() const {
    const bool save_mode = saveDialogMode_ == SaveDialogMode::Save;
    Elements selection_lines;
    if (!savePaths_.empty() && saveSelected_ >= 0 && static_cast<size_t>(saveSelected_) < savePaths_.size()) {
        const size_t selected = static_cast<size_t>(saveSelected_);
        selection_lines.push_back(text(Tr(language_, "Selected Slot: ", "当前槽位：") + SaveDisplayName(selected)) | color(kGold));
        selection_lines.push_back(paragraph(Tr(language_, "File: ", "文件：") + savePaths_[selected]) | color(Color::White));
        selection_lines.push_back(text(save_mode ? Tr(language_,
                                                       "Saving to an existing name overwrites that slot.",
                                                       "保存到已有名称会覆盖对应存档。")
                                                 : Tr(language_,
                                                       "Use arrow keys or the mouse to select a save, then load it.",
                                                       "可用方向键或鼠标选择存档，然后直接读取。")) |
                                  color(kDim));
    } else {
        selection_lines.push_back(text(TranslateGameText(language_, "No save files")) | color(kDim));
    }

    Elements action_buttons;
    if (save_mode) {
        action_buttons.push_back(saveConfirmButton_->Render() | flex);
        action_buttons.push_back(autoSaveButton_->Render() | flex);
    } else {
        action_buttons.push_back(loadConfirmButton_->Render() | flex);
    }
    action_buttons.push_back(closeDialogButton_->Render() | flex);

    return window(
               text(save_mode ? Tr(language_, "Save Manager", "存档管理")
                              : Tr(language_, "Load Manager", "读档管理")) |
                   color(kGold) | bold,
               vbox({
                   paragraph(save_mode
                                 ? Tr(language_,
                                      "Type a save name, save to it manually, or rely on the automatic autosave slot.",
                                      "输入存档名称后即可手动保存，也可以直接使用自动存档槽。")
                                 : Tr(language_,
                                      "Choose any save slot below and load it directly.",
                                      "从下方选择任意存档并直接读取。")) |
                       color(Color::White),
                   separator(),
                   window(text(Tr(language_, "Save Name", "存档名称")) | color(kHull),
                          vbox({
                              saveNameInputComponent_->Render(),
                              text(Tr(language_, "Allowed characters: letters, numbers, _ and -", "允许字符：字母、数字、_ 和 -")) | color(kDim),
                          })) |
                       bgcolor(kPanel),
                   separator(),
                   window(text(Tr(language_, "Available Saves", "可用存档")) | color(kHull), vbox({saveEntriesMenu_->Render()})) |
                       bgcolor(kPanel),
                   separator(),
                   window(text(Tr(language_, "Selected Save", "当前选中的存档")) | color(kHull), vbox(std::move(selection_lines))) |
                       bgcolor(kPanel),
                   separator(),
                   hbox(std::move(action_buttons)),
               })) |
           size(WIDTH, LESS_THAN, 110) | bgcolor(kPanelAlt) | center;
}

ftxui::Component CreateApp(ftxui::ScreenInteractive& screen) {
    auto app = std::make_shared<StarAbyssApp>(screen);
    return CatchEvent(app->root(), [app](Event) { return false; });
}
