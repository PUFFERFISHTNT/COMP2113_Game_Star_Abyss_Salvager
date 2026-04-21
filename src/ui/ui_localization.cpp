#include "ui/ui_localization.h"

#include "engine/game_card_json.h"

#include <map>
#include <string>
#include <utility>
#include <vector>

// Centralizes all UI copy translation so page render modules can stay focused on
// layout and interaction, while gameplay text continues to map 1:1 with the
// original single-file implementation.

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
    static std::map<std::string, std::string> zh_to_en;
    static std::map<std::string, std::string> en_to_zh;
    static std::vector<std::pair<std::string, std::string>> name_zh_to_en;
    static bool card_maps_ready = false;
    if (!card_maps_ready) {
        card_maps_ready = true;
        std::vector<CardDef> cards;
        std::vector<CardDef> upgrades;
        std::string load_error;
        if (LoadCardsFromJsonFile(ResolveCardsJsonPath(), &cards, &upgrades, &load_error)) {
            const auto add_pair = [&](const std::string& zh, const std::string& en, bool is_name) {
                if (zh.empty() || en.empty() || zh == en) {
                    return;
                }
                zh_to_en[zh] = en;
                en_to_zh[en] = zh;
                if (is_name) {
                    name_zh_to_en.push_back({zh, en});
                }
            };
            for (const CardDef& card : cards) {
                add_pair(card.name, card.nameEn, true);
                add_pair(card.description, card.descriptionEn, false);
            }
            for (const CardDef& card : upgrades) {
                add_pair(card.name, card.nameEn, true);
                add_pair(card.description, card.descriptionEn, false);
            }
        }
    }

    if (language == UiLanguage::English) {
        if (!text.empty() && text.back() == '+') {
            const size_t base_end = text.find_last_not_of('+');
            if (base_end != std::string::npos) {
                return TranslateGameText(language, text.substr(0, base_end + 1)) + text.substr(base_end + 1);
            }
        }
        const auto exact = zh_to_en.find(text);
        if (exact != zh_to_en.end()) {
            return exact->second;
        }
        std::string translated = text;
        for (const auto& item : name_zh_to_en) {
            size_t pos = translated.find(item.first);
            while (pos != std::string::npos) {
                translated.replace(pos, item.first.size(), item.second);
                pos = translated.find(item.first, pos + item.second.size());
            }
        }
        return translated;
    }

    if (!text.empty() && text.back() == '+') {
        const size_t base_end = text.find_last_not_of('+');
        if (base_end != std::string::npos) {
            return TranslateGameText(language, text.substr(0, base_end + 1)) + text.substr(base_end + 1);
        }
    }
    const auto exact_zh = en_to_zh.find(text);
    if (exact_zh != en_to_zh.end()) {
        return exact_zh->second;
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
        {"Pirate Needle", "海盗尖针舰"},
        {"Scrapbreaker Tug", "碎铁拖船"},
        {"Signal Leech Mk-II", "信号水蛭 Mk-II"},
        {"Rogue Miner Colossus", "失控矿业巨像"},
        {"Dread Frigate Omega", "欧米伽无畏护卫舰"},
        {"Cinder Hauler", "炽烬运载舰"},
        {"Furnace Tender", "熔炉补给舰"},
        {"Ember Rack Skiff", "余烬挂架艇"},
        {"Slag Tribunal", "熔渣裁决者"},
        {"The Furnace Carrier", "熔炉航母"},
        {"Phase Skewer", "相位穿刺者"},
        {"Null Choir", "虚无合唱团"},
        {"Core Warder", "核心守望者"},
        {"Reality Harrower", "现实劫掠者"},
        {"Abyss Core Sentinel", "深渊核心哨兵"},
        {"The Pale Broker", "苍白掮客"},
        {"Saint of Cinders", "灰烬圣者"},
        {"The Choir Beneath", "深处合唱"},
        {"Salvager Zero", "零号打捞者"},
        {"Hostile Contact", "敌方接触目标"},
        {"Anomalous War Signature", "异常战争信号"},
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
        {"Emergency Cooling", "紧急冷却"},
        {"Thermal Lance", "热能长枪"},
        {"Reactor Spike", "反应堆尖峰"},
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
        {"Deal 7 damage. Add 3 heat to the enemy.", "造成 7 点伤害，并使敌方增加 3 点热量。"},
        {"Add 6 heat to the enemy.", "使敌方增加 6 点热量。"},
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
        {"Needle Burst", "尖针齐射"},
        {"Afterburn Dash", "加力突袭"},
        {"Scrap Bulkhead", "碎铁隔板"},
        {"Tow Ram", "拖拽冲角"},
        {"Brace Thrusters", "稳推姿态"},
        {"Pulse Jam", "脉冲干扰"},
        {"Seismic Beam", "震裂光束"},
        {"Shrapnel Wash", "破片洪流"},
        {"Ore Plating", "矿层装甲"},
        {"Slag Exhaust", "熔渣排气"},
        {"Cargo Swing", "货舱横扫"},
        {"Insulated Plating", "隔热装甲"},
        {"Thermal Screen", "热幕护屏"},
        {"Boiler Surge", "锅炉激涌"},
        {"Coolant Wall", "冷却壁"},
        {"Pipe Lash", "管线鞭击"},
        {"Incendiary Burst", "燃烧爆裂"},
        {"Rack Detonation", "挂架殉爆"},
        {"Verdict Barrier", "裁决壁障"},
        {"Molten Sentence", "熔刑宣告"},
        {"Execution Lance", "处决长枪"},
        {"Suppression Pulse", "压制脉冲"},
        {"Phase Jab", "相位突刺"},
        {"Warp Friction", "曲跃摩擦"},
        {"Skewer Drive", "穿刺推进"},
        {"Silence Vector", "寂静向量"},
        {"Null Resonance", "虚无共振"},
        {"Echo Collapse", "回响坍缩"},
        {"Static Break", "静电断裂"},
        {"Core Guard", "核心护卫"},
        {"Core Drill Spin-Up", "核心钻机蓄压"},
        {"Beam Sweep", "光束横扫"},
        {"Anchor Field", "锚定力场"},
        {"Containment Leak", "约束泄漏"},
        {"Rift Slash", "裂隙斩击"},
        {"Entropy Bloom", "熵增绽放"},
        {"Fracture Veil", "裂界帷幕"},
        {"Static Snare", "静滞陷网"},
        {"Contract Lock", "契约封锁"},
        {"Broker Screen", "掮客屏障"},
        {"Asset Seizure", "资产扣押"},
        {"Salvage Burn", "打捞灼烧"},
        {"Liquidation Shot", "清算射击"},
        {"Kiln Mantle", "窑炉圣袍"},
        {"Censer Exhaust", "香炉排烟"},
        {"Sanctified Pressure", "圣化蓄压"},
        {"Foundry Verdict", "铸炉裁决"},
        {"Ash Injection", "灰烬注入"},
        {"Ashen Choir", "灰烬圣咏"},
        {"Mute Pulse", "静默脉冲"},
        {"Resonance Fever", "共振热病"},
        {"Echo Veil", "回声帷幕"},
        {"Hymn of Collapse", "坍缩圣歌"},
        {"Silence Cascade", "静默级联"},
        {"Prototype Shell", "原型机壳"},
        {"Smelter Shell", "熔炼外壳"},
        {"Reactor Flare", "反应堆闪焰"},
        {"Reactor Overwrite", "反应堆覆写"},
        {"Salvage Breaker", "打捞破碎锤"},
        {"System Refit Pulse", "系统重构脉冲"},
        {"Zero-State Windup", "零态蓄能"},
        {"Collapse Verdict", "坍缩裁决"},
        {"Origin Collapse", "原初坍缩"},
        {"Abyss Pressure", "深渊压强"},
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
        {"Hostile signatures spike ahead.", "前方敌对信号陡增。"},
        {"A heavy hostile vessel blocks the lane.", "一艘重型敌舰挡住了航道。"},
        {"A high-priority anomalous war signature requests immediate attention.", "一道高优先级异常战舰信号请求立即处理。"},
        {"Broker Echo", "掮客残响"},
        {"Cinder Shrine Signal", "灰烬圣坛信号"},
        {"Beneath Choir Resonance", "深处合唱共振"},
        {"Prototype Zero Trace", "零号原型踪迹"},
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
        {"Use the station fabricators to upgrade one card for free.", "使用维修站的制造设备，免费升级 1 种卡牌。"},
        {"Strip Scrap", "拆解废料"},
        {"Recover Module Rack", "回收模块架"},
        {"Leave It", "不去碰它"},
        {"Gain immediate scrap, but risk a little reactor heat.", "立刻获得废料，但会冒一点反应堆升温风险。"},
        {"Turn the wreck into a fresh card reward.", "把残骸转化为一组新的卡牌奖励。"},
        {"Avoid the unknown and conserve resources.", "避开未知风险，保留现有资源。"},
        {"A broker crackles through the static with contraband cards and a purge service.", "一名掮客在静电中接通讯号，兜售走私卡牌和移除服务。"},
        {"A broker crackles through the static with contraband cards and ship services.", "一名掮客在静电中接通讯号，兜售走私卡牌与舰船服务。"},
        {"Upgrade 1 card", "升级 1 张卡牌"},
        {"Open the upgrade console and improve one card in your deck.", "打开升级终端，强化你卡组中的 1 张牌。"},
        {"Open the purge console. Each market visit allows only one purge.", "打开删牌终端。每次进入商店只能删 1 张牌。"},
        {"Purge 1 card", "移除 1 张卡牌"},
        {"Pay 20 scrap to permanently remove one card from your deck.", "支付 20 废料，永久移除卡组中的 1 张牌。"},
        {"Leave the broker", "离开掮客频道"},
        {"Cut the transmission and keep moving.", "切断传输，继续前进。"},
        {"Black Market Upgrade", "黑市升级终端"},
        {"Repair Station Retrofit", "维修站改装终端"},
        {"Select one card to erase from your deck. The black market visit ends immediately after the purge.", "选择 1 张卡牌从卡组中彻底抹除。执行移除后，本次黑市访问会立刻结束。"},
        {"More purge targets", "更多可移除目标"},
        {"More upgrade targets", "更多可升级目标"},
        {"Previous purge targets", "上一页可移除目标"},
        {"Previous upgrade targets", "上一页可升级目标"},
        {"Cycle to the next page of cards that can be removed.", "切换到下一页可移除卡牌。"},
        {"Return to the previous page of cards that can be removed.", "返回上一页可移除卡牌。"},
        {"Return to the previous page of cards that can be upgraded.", "返回上一页可升级卡牌。"},
        {"Back to broker", "返回商店"},
        {"Return to the black market offers.", "返回黑市商店列表。"},
        {"Abort the purge and close the channel.", "取消移除并关闭频道。"},
        {"Distress Beacon", "求救信标"},
        {"A damaged salvage tug is limping between asteroids.", "一艘受损的打捞拖船正蹒跚穿过小行星带。"},
        {"Aid the tug", "援助拖船"},
        {"Strip the derelict", "拆解残舰"},
        {"Ignore the call", "忽略求救"},
        {"Gain a modest reward and some goodwill.", "获得一份稳定奖励和一点善意。"},
        {"Gain a modest reward, some goodwill, and a random card upgrade.", "获得一份稳定奖励、一点善意，以及一次随机卡牌升级。"},
        {"Take a bigger haul, but risk heat from a stressed reactor.", "可以拿到更大的收益，但反应堆会承受额外热量风险。"},
        {"Keep your run clean and your conscience murky.", "继续你的航程，只留下有点复杂的良心。"},
        {"A harmonic transmission keeps syncing to your nav core.", "一段谐波信号不断与你的导航核心同步。"},
        {"Investigate", "深入调查"},
        {"Ride the signal", "借力信号"},
        {"Mute it", "静音处理"},
        {"Dig for blueprints and possible reward cards.", "挖掘蓝图，并可能得到奖励卡牌。"},
        {"Decode the lattice and choose one card to upgrade.", "解码调谐阵列，并自选 1 张卡牌升级。"},
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
        {"Smuggler Purge Rig", "走私者删牌终端"},
        {"A hidden cargo pod blinks with black market encryption.", "一个隐藏货舱正闪烁着黑市加密信号。"},
        {"Force it open", "强行开启"},
        {"Bribe the lock (10 scrap)", "贿赂锁芯（10 废料）"},
        {"Leave it", "离开这里"},
        {"High-risk relic grab with self-inflicted damage.", "高风险获取遗物，但会承受自伤。"},
        {"Trade 10 scrap for a controlled card reward.", "花费 10 废料，换取一次可控的卡牌奖励。"},
        {"Trade 10 credits for a controlled purge reward.", "花费 10 金币，换取一次可控的删牌奖励。"},
        {"No risk, no gain.", "无风险，也无收益。"},
        {"Signal Calibration", "信号校准"},
        {"Skip reward", "跳过奖励"},
        {"Leave the purge rig untouched and continue.", "不使用删牌终端，直接继续前进。"},
        {"Leave the tuning lattice unused and continue.", "不使用调谐阵列，直接继续前进。"},
        {"Leave the retrofit console unused and continue.", "不使用改装终端，直接继续前进。"},
        {"Upgrade exactly 1 copy in the deck.", "精确升级牌组中的 1 个卡牌实例。"},
        {"Upgrade Service", "升级服务"},
        {"Price Free", "价格：免费"},
        {"Purge service already used on this visit.", "本次进店的删牌服务已使用。"},
        {"Upgrade service cost rises: 50, 100, then doubles each time.", "升级服务价格递增：50、100，之后每次翻倍。"},
        {"Buy a card, upgrade a card, purge once, or leave the channel.", "购买卡牌、升级卡牌、删一次牌，或直接离开频道。"},
        {"Pick a target card, browse more pages, or go back to the broker.", "选择目标卡牌、浏览更多页面，或返回商店。"},
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
        {"Save file references an unknown upgraded card.", "存档引用了未知的已升级卡牌类型。"},
        {"Save file references an unknown enemy.", "存档引用了未知敌人。"},
        {"Unable to write upgraded card JSON file.", "无法写入升级卡牌 JSON 文件。"},
        {"Upgraded card JSON references an unknown base card.", "升级卡牌 JSON 引用了未知的原始卡牌。"},
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
        {"Interceptor Drone", "拦截机"},
        {"Guardian Drone", "护卫机"},
        {"Repair Drone", "维修机"},
        {"Pulse Drone", "脉冲机"},
        {"Auxiliary Drone", "附属舰"},
        {"Main Menu", "主菜单"},
        {"Launch Setup", "出航设置"},
        {"Difficulty", "难度"},
        {"Starter Ship", "初始飞船"},
        {"Run Stats", "本轮状态"},
        {"Battle Bridge", "战斗舰桥"},

        // ---- New event system: titles ----
        {"Ghost Ship", "幽灵船"},
        {"Space Fold", "空间折叠"},
        {"Ark Beacon", "方舟信标"},
        {"Hull Breach", "船体破损"},
        {"Deck Chaos", "模块混乱"},
        {"Wandering Trader", "流浪商人"},
        {"Cryo Pod", "冷冻舱"},
        {"Federal Patrol Outpost", "联邦巡逻哨站"},
        {"Abandoned Smuggling Port", "废弃走私港"},
        {"Deep Space Radiation Belt", "深空辐射带"},
        {"Pirate Pursuit Ship", "海盗追击舰"},
        {"Drifting Escape Pod", "漂流逃生舱"},
        {"Unstable Jump Point", "不稳定跃迁点"},
        {"Star Abyss Edge Barrier", "星渊边界屏障"},
        {"Temporary Safe Station", "临时安全站"},
        {"Escape Skirmish", "逃离遇战"},
        {"Escape Blockade", "逃离封锁"},
        {"Escape Decision", "逃离抄择"},
        {"Memory Remnant", "记忆残片"},
        {"Rogue Maintenance AI", "失控维护 AI"},
        {"Phantom Guardian", "幻影守卫"},
        {"Space Ripper", "空间撕裂者"},
        {"Pseudo-God Apostle", "伪神使徒"},
        {"Creation Core Guardian", "创世核心守卫"},
        {"Creation Memory Fragment", "创世记忆碎片"},
        {"Rogue Guardian AI", "失控守卫 AI"},
        {"First Captain's Consciousness", "初代舰长的意识"},
        {"Spatial Collapse Point", "空间坍缩点"},
        {"Star Abyss Birth Truth", "星渊诞生之真相"},

        // ---- New event system: route descriptions ----
        {"A federation checkpoint blocks the route. Patrol drones sweep the area.", "联邦检查站封锁了航路。巡逻无人机在区域内扫荡。"},
        {"A deserted port with some supplies left behind.", "一座荒废的港口，留有一些补给。"},
        {"A deadly band of cosmic radiation stretches across the path ahead.", "前方有一条致命的宇宙辐射带横贯航路。"},
        {"A pirate craft locks onto your trail and closes in fast.", "一艘海盗船锁定了你的航迹，正在快速逼近。"},
        {"A damaged escape pod tumbles through the void with intact supplies.", "一个损坏的逃生舱带着完好的补给在虚空中翻滚。"},
        {"An unstable warp corridor shimmers ahead, offering a risky shortcut.", "前方闪烁着一条不稳定的跃迁走廊，提供了一条冒险的捷径。"},
        {"An energy barrier marks the edge of the Star Abyss.", "一道能量屏障标志着星渊的边界。"},
        {"A small outpost at the edge of the abyss offers one last chance to prepare.", "深渊边缘的一座小哨站，提供最后一次准备的机会。"},
        {"Hostile contacts on the escape vector. No way around them.", "逃离航线上出现敌方目标。无法绕过。"},
        {"A heavily armed ship blocks your escape route.", "一艘重武装舰船挡住了你的逃离航线。"},
        {"The Ark's beacon falls silent. A new heading crystallizes on the nav console.", "方舟信标归于沉寂。导航台上凝聚出了一条新航向。"},
        {"A twisted echo of forgotten memories blocks the corridor ahead.", "一段扭曲的遗忘记忆回声挡住了前方走廊。"},
        {"A malfunctioning AI system guards the old bridge with lethal force.", "一套失控的 AI 系统以致命武力守卫着旧舰桥。"},
        {"An illusory sentinel materializes, defending the captain's sanctum.", "一位幻影哨兵凭空出现，守护着舰长的圣殿。"},
        {"A spatial anomaly tears through reality, attacking anything in its path.", "一个空间异常正在撕裂现实，攻击路径上的一切。"},
        {"A being of immense power emerges from the origin horizon.", "一个拥有巨大力量的存在从起源视界中浮现。"},
        {"The ultimate guardian of the Ark's creation core unfolds in the void.", "方舟创世核心的终极守卫在虚空中缓缓展开。"},
        {"The corridor pulses with ancient data streams from a lost civilization.", "走廊中脉动着来自失落文明的远古数据流。"},
        {"The bridge's AI system lies defeated, its data banks exposed.", "舰桥的 AI 系统已被击败，数据库暴露在外。"},
        {"An echo of the original captain lingers in the ship's neural network.", "初代舰长的回声残留在飞船的神经网络中。"},
        {"Reality bends and tears around you. The fabric of space is failing.", "现实在你周围弯曲撕裂。空间结构正在崩溃。"},
        {"You have reached the origin horizon. The truth of creation lies before you.", "你已抵达起源视界。创世的真相就在眼前。"},
        {"A faint distress signal pulses from a wrecked fleet. Someone may still be alive.", "一段微弱的求救信号从残骸舰队中传来。或许还有人活着。"},
        {"A derelict warship drifts silently. Its hull is intact but shows no life signs.", "一艘废弃战舰在寂静中漂流。船体完好却毫无生命迹象。"},
        {"An anomalous frequency keeps syncing to your navigation core.", "一个异常频率不断与你的导航核心同步。"},
        {"A spatial anomaly warps the region ahead. Energy readings are off the charts.", "前方空间异常正在扭曲区域。能量读数爆表。"},
        {"A massive signal source emerges from the void. This is the Ark's beacon.", "一个巨大的信号源从虚空中浮现。这是方舟的信标。"},
        {"Warning lights flash as unstable coolant leaks from the reactor.", "警示灯闪烁，不稳定冷却液正从反应堆中泄漏。"},
        {"The ship's hull has been compromised. Atmosphere is venting fast.", "飞船船体已被破坏。大气正在快速泄漏。"},
        {"The card module system has malfunctioned. Data corruption detected.", "卡牌模块系统发生故障。检测到数据损坏。"},
        {"A hidden cargo pod blinks with black market encryption signals.", "一个隐藏的货舱正闪烁着黑市加密信号。"},
        {"A lone merchant vessel emerges from the static, offering deals.", "一艘孤独的商船从静电中浮现，提供交易。"},
        {"A pirate interceptor drops out of warp and hails you with demands.", "一艘海盗截击舰脱离跃迁，向你发出勒索通信。"},
        {"A drifting cryo pod with a faint life sign. Someone is in deep freeze.", "一个漂浮的冷冻舱中有微弱生命信号。有人处于深度冷冻中。"},

        // ---- New event system: choices ----
        {"Go to rescue", "前往救援"},
        {"Relay coordinates", "转发坐标"},
        {"Ignore and leave", "无视并离开"},
        {"Board and explore", "登船探索"},
        {"Send a hail", "发送问候"},
        {"Open fire", "开火攻击"},
        {"Trace the signal", "追踪信号"},
        {"Block interference", "屏蔽干扰"},
        {"Record the spectrum", "记录频谱"},
        {"Enter the fold", "进入折叠"},
        {"Stabilize the space", "稳定空间"},
        {"Steer clear", "绕道避开"},
        {"Activate the beacon", "激活信标"},
        {"Dismantle the beacon", "拆解信标"},
        {"Block the beacon", "封锁信标"},
        {"Manual voltage stabilization", "手动电压稳定"},
        {"Activate emergency cooling", "启动紧急冷却"},
        {"Silence the alarm", "关闭警报"},
        {"Emergency welding", "紧急焊接"},
        {"Dump cargo to seal", "抛弃货物封堵"},
        {"Leave it", "放置不管"},
        {"Manual sorting", "手动分拣"},
        {"Reboot the system", "重启系统"},
        {"Force format", "强制格式化"},
        {"Take everything", "全部拿走"},
        {"Take only essentials", "只取必需品"},
        {"Leave it alone", "不去理会"},
        {"Buy cards cheap", "低价购买卡牌"},
        {"Sell junk cards", "卖掉废牌"},
        {"Decline the deal", "拒绝交易"},
        {"Pay the ransom", "支付赎金"},
        {"Fight back", "反击"},
        {"Full speed escape", "全速逃跑"},
        {"Wake the sleeper", "唤醒沉睡者"},
        {"Drain cryo energy", "抽取冷冻能量"},
        {"Scavenge supplies", "搜刮补给"},
        {"Pay transit fee", "支付通行费"},
        {"Power down and drift", "关闭动力漂移通过"},
        {"Force through", "强行突破"},
        {"Emergency repairs", "紧急维修"},
        {"Clean heat exchangers", "清洁热交换器"},
        {"Full speed through", "全速穿越"},
        {"Slow detour", "缓速绕行"},
        {"Activate anti-rad shield", "启动防辐射护盾"},
        {"Pay them off", "花钱打发"},
        {"Jettison cargo", "抛弃货物"},
        {"Counter-attack", "反击"},
        {"Take medical kit", "取走医疗包"},
        {"Drain fuel", "抽取燃料"},
        {"Take emergency chip", "取走应急芯片"},
        {"Force jump", "强制跃迁"},
        {"Stabilize coordinates", "稳定坐标"},
        {"Abandon jump", "放弃跃迁"},
        {"Energy breach", "能量突破"},
        {"Spend supplies to breach", "消耗物资突破"},
        {"Find a gap", "寻找缺口"},
        {"Repair hull", "修复船体"},
        {"Optimize engines", "优化引擎"},
        {"Remove junk cards", "移除废牌"},
        {"Merge memories", "融合记忆"},
        {"Extract energy", "提取能量"},
        {"Save the log", "保存日志"},
        {"Force breakthrough", "强行突破"},
        {"Override permissions", "覆写权限"},
        {"Sneak around", "潜行绕过"},
        {"Listen to the truth", "聆听真相"},
        {"Gain ship permissions", "获取飞船权限"},
        {"Drain core energy", "抽取核心能量"},
        {"Engines full to stabilize", "全力引擎稳定"},
        {"Dump supplies to brace", "丢弃物资加固"},
        {"Emergency dodge", "紧急闪避"},
        {"Accept the truth", "接受真相"},
        {"Extract the origin", "提取起源"},
        {"Withdraw", "撤离"},

        // ---- New event system: details ----
        {"The signal condenses into a hostile flagship profile. Engaging now could uncover hidden truths, but retreat is still possible.",
         "那道信号凝聚成一艘敌对旗舰的轮廓。现在交战也许能揭开隐藏的真相，但你仍然可以选择撤退。"},
        {"Gain 10 HP and 12 credits.", "获得 10 生命和 12 金币。"},
        {"Reduce 3 heat and gain 1 random card.", "减少 3 热量并获得 1 张随机卡牌。"},
        {"Continue without delay.", "继续前进，不耽搁。"},
        {"Lose 5 HP but gain 1 rare relic.", "失去 5 生命，但获得 1 件稀有遗物。"},
        {"Gain 8 HP and 2 heat.", "获得 8 生命和 2 热量。"},
        {"Forced into a small battle. Defeat: game over.", "被迫进入小型战斗。失败即结束。"},
        {"Higher encounter chance next. Gain 1 random card.", "下一次遭遇概率提高。获得 1 张随机卡牌。"},
        {"Reduce 4 heat.", "减少 4 热量。"},
        {"Gain 8 credits.", "获得 8 金币。"},
        {"Skip 1 segment, +5 heat, gain 1 rare card.", "跳过 1 个节点，+5 热量，获得 1 张稀有卡牌。"},
        {"-4 heat, +2 permanent shield, costs 10 credits.", "-4 热量，+2 永久护盾，花费 10 金币。"},
        {"Next battle charged attack enhanced.", "下一场战斗蓄力攻击增强。"},
        {"Gain legendary card, +8 heat. Unlocks true ending.", "获得传说卡牌，+8 热量。解锁真结局。"},
        {"+25 credits, gain relic, -6 HP.", "+25 金币，获得遗物，-6 生命。"},
        {"Trigger escape route. No more bosses.", "触发逃离路线。不再有 Boss 战。"},
        {"Lose 3 HP but reduce 4 heat.", "失去 3 生命，但减少 4 热量。"},
        {"Costs 8 credits.", "花费 8 金币。"},
        {"Heat builds faster next segment.", "下一段热量积累更快。"},
        {"-4 HP, +3 heat, costs 10 credits.", "-4 生命，+3 热量，花费 10 金币。"},
        {"Lose 8 HP.", "失去 8 生命。"},
        {"Upgrade 2 random cards, lose 4 HP.", "升级 2 张随机卡牌，失去 4 生命。"},
        {"-5 heat, delete 1 low-quality card, costs 10 credits.", "-5 热量，删除 1 张低品质卡牌，花费 10 金币。"},
        {"Costs 15 credits. Clear all low-quality cards.", "花费 15 金币。清除所有低品质卡牌。"},
        {"+16 credits and 1 random card.", "+16 金币和 1 张随机卡牌。"},
        {"+6 HP.", "+6 生命。"},
        {"Continue without risk.", "继续前进，无风险。"},
        {"Costs 18 credits. Gain 2 random cards.", "花费 18 金币。获得 2 张随机卡牌。"},
        {"Delete 1 random card. Gain 10 credits.", "删除 1 张随机卡牌。获得 10 金币。"},
        {"Continue.", "继续。"},
        {"Costs 12 credits.", "花费 12 金币。"},
        {"Forced elite battle. Defeat: game over.", "强制精英战斗。失败即结束。"},
        {"+6 heat.", "+6 热量。"},
        {"Cards enhanced. Next fight must be elite.", "卡牌增强。下次战斗必为精英。"},
        {"+5 heat, +6 HP, costs 8 credits.", "+5 热量，+6 生命，花费 8 金币。"},
        {"+18 credits, gain card, -8 HP.", "+18 金币，获得卡牌，-8 生命。"},
        {"+4 heat.", "+4 热量。"},
        {"Forced normal battle. Defeat: game over.", "强制普通战斗。失败即结束。"},
        {"+10 HP.", "+10 生命。"},
        {"Reduce 6 heat.", "减少 6 热量。"},
        {"+14 credits.", "+14 金币。"},
        {"Skip 1 segment, lose 6 HP.", "跳过 1 个节点，失去 6 生命。"},
        {"+3 heat.", "+3 热量。"},
        {"+2 permanent max HP, costs 10 credits.", "+2 永久最大生命，花费 10 金币。"},
        {"Costs 15 credits.", "花费 15 金币。"},
        {"Delete 1 random card.", "删除 1 张随机卡牌。"},
        {"+8 HP.", "+8 生命。"},
        {"+2 initial energy next battle.", "下一场战斗初始能量 +2。"},
        {"+3 HP recovery next battle.", "下一场战斗恢复 3 生命。"},
        {"Skip 2 segments, +7 heat.", "跳过 2 个节点，+7 热量。"},
        {"Reduce 5 heat, costs 8 credits.", "减少 5 热量，花费 8 金币。"},
        {"Gain 1 random card, lose 5 HP.", "获得 1 张随机卡牌，失去 5 生命。"},
        {"Costs 18 credits.", "花费 18 金币。"},
        {"Lose 5 HP.", "失去 5 生命。"},
        {"+5 HP, costs 12 credits.", "+5 生命，花费 12 金币。"},
        {"+1 initial energy, +2 heat.", "+1 初始能量，+2 热量。"},
        {"Delete 1 low-quality card, lose 4 HP.", "删除 1 张低品质卡牌，失去 4 生命。"},
        {"Upgrade 2 random cards.", "升级 2 张随机卡牌。"},
        {"Permanent max HP +6.", "永久最大生命 +6。"},
        {"Gain 1 special Truth card.", "获得 1 张特殊真相卡牌。"},
        {"Gain a rare relic, lose 8 HP.", "获得 1 件稀有遗物，失去 8 生命。"},
        {"+2 initial energy next battle, +5 heat.", "下一场战斗初始能量 +2，+5 热量。"},
        {"Gain 1 legendary card.", "获得 1 张传说卡牌。"},
        {"+4 permanent opening shield.", "+4 永久初始护盾。"},
        {"Reduce 10 heat, +10 HP.", "减少 10 热量，+10 生命。"},
        {"+8 heat.", "+8 热量。"},
        {"Costs 20 credits.", "花费 20 金币。"},
        {"Lose 9 HP.", "失去 9 生命。"},
        {"All cards permanently enhanced.", "所有卡牌永久增强。"},
        {"Gain ultimate relic, lose 10 HP.", "获得终极遗物，失去 10 生命。"},
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
    if (StartsWith(text, "Enemy heat climbs by ") && EndsWith(text, ".")) {
        return "敌方热量上升 " + text.substr(21, text.size() - 22) + " 点。";
    }
    if (StartsWith(text, "Enemy heat drops by ") && EndsWith(text, ".")) {
        return "敌方热量下降 " + text.substr(20, text.size() - 21) + " 点。";
    }
    if (text == "Enemy overheat begins. Attack output drops by 25%.") {
        return "敌方进入过热。输出伤害降低 25%。";
    }
    if (text == "Enemy overheat deepens. Incoming damage rises by 25%.") {
        return "敌方过热加深。受到伤害提高 25%。";
    }
    if (text == "Enemy overheat peaks. It will burn for 5 HP each turn.") {
        return "敌方过热达到峰值。此后每回合燃烧 5 点生命。";
    }
    if (text == "Enemy overheat clears.") {
        return "敌方过热已解除。";
    }
    if (text == "Enemy overheat burns 5 HP.") {
        return "敌方因过热燃烧，失去 5 点生命。";
    }
    if (text == "Player overheat begins. Draw drops by 1.") {
        return "我方进入过热。抽牌减少 1 张。";
    }
    if (text == "Player overheat deepens. Outgoing damage drops by 25%, incoming damage rises by 25%.") {
        return "我方过热加深。输出伤害降低 25%，受到伤害提高 25%。";
    }
    if (text == "Player overheat peaks. It will burn for 5 HP each turn.") {
        return "我方过热达到峰值。此后每回合失去 5 点生命。";
    }
    if (text == "Player overheat clears.") {
        return "我方过热已解除。";
    }
    if (text == "Player overheat burns 5 HP.") {
        return "我方因过热失去 5 点生命。";
    }
    if (StartsWith(text, "Cooling cooldown: ")) {
        return "冷却冷却中：" + text.substr(18);
    }
    if (StartsWith(text, "Reduce own heat ")) {
        return "自身热量 -" + text.substr(16);
    }
    if (StartsWith(text, " vents reactor heat by ") && EndsWith(text, ".")) {
        const size_t split = text.find(" vents reactor heat by ");
        const std::string name = text.substr(0, split);
        const std::string value = text.substr(split + 22, text.size() - split - 23);
        return TranslateGameText(language, name) + " 排出反应堆热量 " + value + " 点。";
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
    if (StartsWith(text, "The broker opens an upgrade console. Choose one module to improve.")) {
        return "掮客打开了升级终端。请选择 1 张要强化的模块牌。";
    }
    if (StartsWith(text, "The station opens a retrofit console. Choose one card to upgrade for free.")) {
        return "维修站打开了改装终端。请选择 1 种卡牌免费升级。";
    }
    if (StartsWith(text, "The broker has already performed one purge this visit.")) {
        return "掮客本次进店已经执行过一次删牌服务。";
    }
    if (StartsWith(text, "The service cannot find that module in your active deck.")) {
        return "服务终端无法在当前卡组中找到这张模块牌。";
    }
    if (StartsWith(text, "You return to the broker's main trade feed.")) {
        return "你回到了掮客的主交易频道。";
    }
    if (StartsWith(text, "You leave the purge console untouched.")) {
        return "你没有使用删牌终端。";
    }
    if (StartsWith(text, "You leave the upgrade console untouched.")) {
        return "你没有使用升级终端。";
    }
    if (StartsWith(text, "You leave the retrofit console unused.")) {
        return "你没有使用改装终端。";
    }
    if (StartsWith(text, "Signal decryption reveals a precision tuning pattern. Choose one card to upgrade.")) {
        return "信号解密后显露出一套精密调谐图样。请选择 1 张卡牌升级。";
    }
    if (StartsWith(text, "The bribe exposes a silent purge rig. Remove one card from your deck.")) {
        return "贿赂成功后，一台隐蔽的删牌终端显露出来。请选择 1 张卡牌移除。";
    }
    if (StartsWith(text, "You leave the purge rig untouched.")) {
        return "你没有使用这台删牌终端。";
    }
    if (StartsWith(text, "You leave the tuning lattice unused.")) {
        return "你没有使用这组调谐阵列。";
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
    if (StartsWith(text, "No active run to upgrade.")) {
        return "当前没有可升级的航程。";
    }
    if (StartsWith(text, "No cards can be upgraded right now.")) {
        return "当前没有可升级的卡牌。";
    }
    if (StartsWith(text, "That card is not in the current deck.")) {
        return "当前卡组中找不到这张牌。";
    }
    if (StartsWith(text, "That card does not have an upgrade effect.")) {
        return "这张牌没有可升级的数值效果。";
    }
    if (StartsWith(text, "That card is already fully upgraded.")) {
        return "这张牌已经升到最高级。";
    }
    if (StartsWith(text, "Not enough credits to upgrade that card.")) {
        return "金币不足，无法升级这张牌。";
    }
    if (StartsWith(text, "Purge 1 copy from the deck. Copies owned: ") && EndsWith(text, ".")) {
        return "从卡组中移除 1 张。当前持有：" + text.substr(39, text.size() - 40) + "。";
    }
    if (StartsWith(text, "Upgrade 1 copy in the deck. Copies owned: ") && EndsWith(text, ".")) {
        return "从卡组中升级 1 张。当前持有：" + text.substr(40, text.size() - 41) + "。";
    }
    if (StartsWith(text, "Upgrade 1 card type in the deck. Copies owned: ") && EndsWith(text, ".")) {
        return "升级该种卡牌。当前持有：" + text.substr(45, text.size() - 46) + "。";
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
    if (StartsWith(text, "Select one card to upgrade. Current price: ") &&
        text.find(" credits. Page ") != std::string::npos &&
        text.find(". Scroll with mouse wheel, [ ], or PgUp/PgDn if needed.") != std::string::npos) {
        const size_t price_start = std::string("Select one card to upgrade. Current price: ").size();
        const size_t price_end = text.find(" credits. Page ");
        const size_t page_start = price_end + std::string(" credits. Page ").size();
        const size_t page_end = text.find(". Scroll with mouse wheel, [ ], or PgUp/PgDn if needed.");
        return "选择 1 张卡牌升级。当前价格：" + text.substr(price_start, price_end - price_start) +
               " 金币。第 " + text.substr(page_start, page_end - page_start) +
               " 页。需要时可用鼠标滚轮、[ ] 或 PgUp/PgDn 滚动。";
    }
    if (StartsWith(text, "Select one card to upgrade. Current price: ") &&
        text.find(" credits. The full deck is shown below; scroll vertically if needed.") != std::string::npos) {
        const size_t price_start = std::string("Select one card to upgrade. Current price: ").size();
        const size_t price_end = text.find(" credits. The full deck is shown below; scroll vertically if needed.");
        return "选择 1 张卡牌升级。当前价格：" + text.substr(price_start, price_end - price_start) +
               " 金币。下方会显示整个牌组，需要时可纵向滚动浏览。";
    }
    if (StartsWith(text, "A hidden rig can erase one card from your deck for free. Page ") &&
        text.find(". Starter cards may appear on later pages. Scroll with mouse wheel, [ ], or PgUp/PgDn if needed.") != std::string::npos) {
        const size_t page_start = std::string("A hidden rig can erase one card from your deck for free. Page ").size();
        const size_t page_end = text.find(". Starter cards may appear on later pages. Scroll with mouse wheel, [ ], or PgUp/PgDn if needed.");
        return "一台隐藏终端可以免费移除你卡组中的 1 张牌。第 " +
               text.substr(page_start, page_end - page_start) +
               " 页。初始卡可能会出现在后续页面。需要时可用鼠标滚轮、[ ] 或 PgUp/PgDn 滚动。";
    }
    if (StartsWith(text, "A tuning lattice lets you upgrade one card for free. Page ") &&
        text.find(". Scroll with mouse wheel, [ ], or PgUp/PgDn if needed.") != std::string::npos) {
        const size_t page_start = std::string("A tuning lattice lets you upgrade one card for free. Page ").size();
        const size_t page_end = text.find(". Scroll with mouse wheel, [ ], or PgUp/PgDn if needed.");
        return "一组调谐阵列可以免费升级你卡组中的 1 张牌。第 " +
               text.substr(page_start, page_end - page_start) +
               " 页。需要时可用鼠标滚轮、[ ] 或 PgUp/PgDn 滚动。";
    }
    if (text == "A tuning lattice lets you upgrade one card for free. The full deck is shown below; scroll vertically if needed.") {
        return "一组调谐阵列可以免费升级你卡组中的 1 张牌。下方会显示整个牌组，需要时可纵向滚动浏览。";
    }
    if (StartsWith(text, "Select one card to upgrade for free at the repair station. Page ") &&
        text.find(". Scroll with mouse wheel, [ ], or PgUp/PgDn if needed.") != std::string::npos) {
        const size_t page_start = std::string("Select one card to upgrade for free at the repair station. Page ").size();
        const size_t page_end = text.find(". Scroll with mouse wheel, [ ], or PgUp/PgDn if needed.");
        return "选择 1 种卡牌在维修站免费升级。第 " +
               text.substr(page_start, page_end - page_start) +
               " 页。需要时可用鼠标滚轮、[ ] 或 PgUp/PgDn 滚动。";
    }
    if (text == "Select one card to upgrade for free at the repair station. The full deck is shown below; scroll vertically if needed.") {
        return "选择 1 张卡牌在维修站免费升级。下方会显示整个牌组，需要时可纵向滚动浏览。";
    }
    if (StartsWith(text, "Select one card to erase from your deck. Price: 20 credits. Each market visit allows only one purge. Page ") &&
        text.find(". Starter cards may appear on later pages. Scroll with mouse wheel, [ ], or PgUp/PgDn if needed.") != std::string::npos) {
        const size_t page_start =
            std::string("Select one card to erase from your deck. Price: 20 credits. Each market visit allows only one purge. Page ").size();
        const size_t page_end = text.find(". Starter cards may appear on later pages. Scroll with mouse wheel, [ ], or PgUp/PgDn if needed.");
        return "选择 1 张卡牌从卡组中彻底抹除。价格 20 金币，每次进店只能使用一次删牌服务。第 " +
               text.substr(page_start, page_end - page_start) +
               " 页。初始卡可能会出现在后续页面。需要时可用鼠标滚轮、[ ] 或 PgUp/PgDn 滚动。";
    }
    if (StartsWith(text, "Select one card to erase from your deck. The black market visit ends immediately after the purge. Page ") &&
        text.find(". Starter cards may appear on later pages.") != std::string::npos) {
        const size_t page_start = std::string("Select one card to erase from your deck. The black market visit ends immediately after the purge. Page ").size();
        const size_t page_end = text.find(". Starter cards may appear on later pages.");
        return "选择 1 张卡牌从卡组中彻底抹除。执行移除后，本次黑市访问会立刻结束。第 " +
               text.substr(page_start, page_end - page_start) + " 页。初始卡可能会出现在后续页面。";
    }
    if (StartsWith(text, "Select one card to erase from your deck. The black market visit ends immediately after the purge. Page ") &&
        text.find(". Starter cards may appear on later pages. Scroll with mouse wheel, [ ], or PgUp/PgDn if needed.") != std::string::npos) {
        const size_t page_start = std::string("Select one card to erase from your deck. The black market visit ends immediately after the purge. Page ").size();
        const size_t page_end = text.find(". Starter cards may appear on later pages. Scroll with mouse wheel, [ ], or PgUp/PgDn if needed.");
        return "选择 1 张卡牌从卡组中彻底抹除。执行移除后，本次黑市访问会立刻结束。第 " +
               text.substr(page_start, page_end - page_start) + " 页。初始卡可能会出现在后续页面。需要时可用鼠标滚轮、[ ] 或 PgUp/PgDn 滚动。";
    }
    if (StartsWith(text, "Cycle to the next page of cards that can be removed. Next page: ") &&
        EndsWith(text, ". Scroll controls stay available while browsing.")) {
        const size_t prefix = std::string("Cycle to the next page of cards that can be removed. Next page: ").size();
        const size_t suffix_length = std::string(". Scroll controls stay available while browsing.").size();
        return "切换到下一页可移除卡牌。下一页：" + text.substr(prefix, text.size() - prefix - suffix_length) +
               "。浏览时滚动控制仍然可用。";
    }
    if (StartsWith(text, "Return to the previous page of cards that can be removed. Previous page: ") && EndsWith(text, ".")) {
        const size_t prefix = std::string("Return to the previous page of cards that can be removed. Previous page: ").size();
        return "返回上一页可移除卡牌。上一页：" + text.substr(prefix, text.size() - prefix - 1) + "。";
    }
    if (StartsWith(text, "Return to the previous page of cards that can be upgraded. Previous page: ") && EndsWith(text, ".")) {
        const size_t prefix = std::string("Return to the previous page of cards that can be upgraded. Previous page: ").size();
        return "返回上一页可升级卡牌。上一页：" + text.substr(prefix, text.size() - prefix - 1) + "。";
    }
    if (StartsWith(text, "Cycle to the next page of cards that can be upgraded. Next page: ") && EndsWith(text, ".")) {
        const size_t prefix = std::string("Cycle to the next page of cards that can be upgraded. Next page: ").size();
        return "切换到下一页可升级卡牌。下一页：" + text.substr(prefix, text.size() - prefix - 1) + "。";
    }
    if (StartsWith(text, "Cycle to the next page of cards that can be removed. Next page: ") && EndsWith(text, ".")) {
        const size_t prefix = std::string("Cycle to the next page of cards that can be removed. Next page: ").size();
        return "切换到下一页可移除卡牌。下一页：" + text.substr(prefix, text.size() - prefix - 1) + "。";
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
    if (StartsWith(text, "Heat is dangerous: overflow feedback damages your hull, and sustained overheat in battle cuts draw, lowers outgoing damage, raises incoming damage, and eventually burns 5 HP each turn.")) {
        return "热量非常危险：超出上限的反馈会伤害你的船体，而战斗中的持续过热会减少抽牌、降低输出、提高受到伤害，并最终每回合失去 5 点生命。";
    }
    if (StartsWith(text, "Mouse clicks are enabled on compatible terminals, but some terminals do not forward mouse wheel events.")) {
        return "兼容的终端支持鼠标点击，但有些终端不会传递鼠标滚轮事件。";
    }
    if (StartsWith(text, "If scrolling does not work in your terminal, use [ and ] or PgUp/PgDn instead.")) {
        return "如果你的终端里无法滚动，请改用 [ 和 ]，或 PgUp/PgDn。";
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
    if (text.find(" upgraded for ") != std::string::npos && EndsWith(text, " credits.")) {
        const size_t pos = text.find(" upgraded for ");
        return TranslateGameText(language, text.substr(0, pos)) + " 已升级，花费 " +
               text.substr(pos + 14, text.size() - pos - 23) + " 金币。";
    }
    if (EndsWith(text, " upgraded for free.")) {
        const std::string suffix = " upgraded for free.";
        const std::string name = text.substr(0, text.size() - suffix.size());
        return TranslateGameText(language, name) + " 已免费升级。";
    }
    if (EndsWith(text, " upgraded for free at the repair station.")) {
        const std::string suffix = " upgraded for free at the repair station.";
        const std::string name = text.substr(0, text.size() - suffix.size());
        return TranslateGameText(language, name) + " 已在维修站免费升级。";
    }
    if (StartsWith(text, "The stranded crew transfers 8 credits, patch gel, and upgrades ") && EndsWith(text, ".")) {
        const std::string prefix = "The stranded crew transfers 8 credits, patch gel, and upgrades ";
        const std::string name = text.substr(prefix.size(), text.size() - prefix.size() - 1);
        return "被困船员转交了 8 金币、修补胶，并升级了 " + TranslateGameText(language, name) + "。";
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

    // ---- New event system: status lines ----
    // Repair station
    if (text == "Repair crews patched the hull for 10 HP.") return "维修人员修补了船体，恢复 10 生命。";
    if (text == "The reactor vents cleanly. Your next battle starts with +4 shield.") return "反应堆顺利排气。下一场战斗开局 +4 护盾。";
    if (text == "New plating installed. Max HP increased by 4.") return "新装甲板安装完毕。最大生命增加 4。";
    if (text == "The station opens a retrofit console. Choose one card to upgrade for free.") return "站点打开了改装控制台。选择 1 张卡牌免费升级。";
    // Salvage
    if (text == "A surviving module rack can be integrated into your deck.") return "一个幸存的模块架可以整合进你的卡组。";
    if (text == "You leave the unstable hulk drifting in the dark.") return "你将不稳定的残骸留在黑暗中漂流。";
    // Market
    if (text == "You break contact with the black market broker.") return "你断开了与黑市掮客的通信。";
    if (text == "The broker opens a purge console. Choose one module to erase.") return "掮客打开了清除控制台。选择 1 个模块抹除。";
    if (text == "The broker opens an upgrade console. Choose one module to improve.") return "掮客打开了升级控制台。选择 1 个模块改良。";
    if (text == "Not enough credits for that transaction.") return "金币不足，无法完成交易。";
    if (text == "Module purchased and queued into your discard pile.") return "模块已购入，进入弃牌堆。";
    if (text == "The broker closes the channel.") return "掮客关闭了通信频道。";
    // Event A1 – Distress Beacon
    if (text == "Rescued the stranded crew. Gained 10 HP and 12 credits.") return "救出了被困船员。获得 10 生命和 12 金币。";
    if (text == "Relayed the coordinates. -3 heat and gained a card.") return "转发了坐标。-3 热量，获得 1 张卡牌。";
    if (text == "You leave the signal fading behind you.") return "你将渐弱的信号抛在身后。";
    // Event A2 – Ghost Ship
    if (text == "Lost 5 HP exploring, but found a rare relic aboard the ghost ship.") return "探索中失去 5 生命，但在幽灵船上找到了稀有遗物。";
    if (text == "The ghost ship responds with supplies. +8 HP, +2 heat.") return "幽灵船提供了补给。+8 生命，+2 热量。";
    if (text == "The ghost ship's defenses activate! Battle!") return "幽灵船的防御系统启动了！战斗！";
    // Event B1 – Strange Signal
    if (text == "Locked onto the signal. Gained a card. Next encounter may be elite.") return "锁定了信号。获得 1 张卡牌。下次遭遇可能是精英。";
    if (text == "Blocked the interference. -4 heat.") return "屏蔽了干扰。-4 热量。";
    if (text == "Recorded the spectrum. +8 credits.") return "记录了频谱。+8 金币。";
    // Event B2 – Space Fold
    if (text == "Entered the fold. Skip 1 segment. +5 heat. Gained a rare card.") return "进入折叠。跳过 1 个节点。+5 热量。获得 1 张稀有卡牌。";
    if (text == "Stabilized the space. -4 heat, +2 permanent shield. Costs 10 credits.") return "稳定了空间。-4 热量，+2 永久护盾。花费 10 金币。";
    if (text == "Not enough credits. The space fold collapses.") return "金币不足。空间折叠坍缩了。";
    if (text == "Steered clear. Next battle's first attack enhanced.") return "绕道避开。下场战斗首次攻击增强。";
    // Event B3 – Ark Beacon
    if (text == "The Ark's beacon activates. Gained a legendary card. +8 heat. Something awakens.") return "方舟信标激活。获得 1 张传说卡牌。+8 热量。有什么苏醒了。";
    if (text == "Dismantled the beacon. +25 credits, gained a relic, -6 HP.") return "拆解了信标。+25 金币，获得遗物，-6 生命。";
    if (text == "Blocked the beacon. Escape route initiated. No more boss encounters.") return "封锁了信标。逃离路线已启动。不再有 Boss 战。";
    if (text == "Signal lock confirmed. Story boss battle engaged.") return "信号锁定完成。剧情 Boss 战已开始。";
    if (text == "You break lock and leave the anomalous signature behind.") return "你解除锁定，把那道异常信号抛在身后。";
    if (StartsWith(text, "Force a boss battle against ") && EndsWith(text, ".")) {
        const std::string prefix = "Force a boss battle against ";
        const std::string name = text.substr(prefix.size(), text.size() - prefix.size() - 1);
        return "强制与 " + TranslateGameText(language, name) + " 进行一场 Boss 战。";
    }
    if (text == "Abort contact and continue your route.") return "中止接触，继续你的航线。";
    // Event C1 – Reactor Leak
    if (text == "Manual stabilization. -3 HP, -4 heat.") return "手动稳定。-3 生命，-4 热量。";
    if (text == "Emergency cooling activated. -8 credits.") return "紧急冷却已启动。-8 金币。";
    if (text == "Not enough credits for emergency cooling.") return "金币不足，无法启动紧急冷却。";
    if (text == "Alarm silenced. Heat will build faster next segment.") return "警报已关闭。下一段热量积累更快。";
    // Event C2 – Hull Breach
    if (text == "Emergency welding complete. -4 HP, +3 heat, -10 credits.") return "紧急焊接完成。-4 生命，+3 热量，-10 金币。";
    if (text == "Not enough credits for welding supplies.") return "金币不足，无法购买焊接材料。";
    if (text == "Dumped cargo to seal the breach. -8 credits.") return "抛弃货物封堵裂口。-8 金币。";
    if (text == "Not enough credits to dump cargo.") return "金币不足，无法抛弃货物。";
    if (text == "Left the breach open. Lost 8 HP from atmosphere venting.") return "放置裂口不管。因大气泄漏失去 8 生命。";
    // Event C3 – Deck Chaos
    if (text == "Manual sorting complete. 2 cards upgraded, -4 HP.") return "手动分拣完成。2 张卡牌已升级，-4 生命。";
    if (text == "System rebooted. -5 heat, removed a low-quality card. -10 credits.") return "系统已重启。-5 热量，移除了 1 张低品质卡牌。-10 金币。";
    if (text == "Not enough credits to reboot the system.") return "金币不足，无法重启系统。";
    if (text == "Force format complete. All low-quality cards purged. -15 credits.") return "强制格式化完成。所有低品质卡牌已清除。-15 金币。";
    if (text == "Not enough credits for force format.") return "金币不足，无法强制格式化。";
    // Event D1 – Smuggler Cache
    if (text == "Took everything. +16 credits and gained a card.") return "全部拿走。+16 金币，获得 1 张卡牌。";
    if (text == "Took only essentials. +6 HP.") return "只取了必需品。+6 生命。";
    if (text == "Left the cache alone. No risk, no reward.") return "没有碰藏匿点。无风险，也无收获。";
    // Event D2 – Wandering Trader
    if (text == "Bought 2 cards. -18 credits.") return "购买了 2 张卡牌。-18 金币。";
    if (text == "Not enough credits to buy cards.") return "金币不足，无法购买卡牌。";
    if (text == "Sold a junk card. +10 credits.") return "卖掉了 1 张废牌。+10 金币。";
    if (text == "Declined the trader's offer.") return "拒绝了商人的交易。";
    // Event D3 – Pirate Ambush
    if (text == "Paid the ransom. -12 credits.") return "支付了赎金。-12 金币。";
    if (text == "Not enough credits to pay the ransom.") return "金币不足，无法支付赎金。";
    if (text == "You fight back against the pirates! Elite battle!") return "你反击海盗！精英战斗！";
    if (text == "Escaped at full speed. +6 heat.") return "全速逃跑了。+6 热量。";
    // Event D4 – Cryo Pod
    if (text == "Woke the sleeper. Cards enhanced. Next encounter may be elite.") return "唤醒了沉睡者。卡牌增强。下次遭遇可能是精英。";
    if (text == "Drained cryo energy. +5 heat, +6 HP, -8 credits.") return "抽取了冷冻能量。+5 热量，+6 生命，-8 金币。";
    if (text == "Not enough credits to drain cryo energy.") return "金币不足，无法抽取冷冻能量。";
    if (text == "Scavenged supplies. +18 credits, gained a card, -8 HP.") return "搜刮了补给。+18 金币，获得 1 张卡牌，-8 生命。";
    // Escape prologue
    if (text == "You sealed the Ark's beacon and cut the signal dead. The surrounding space warps and groans as the abyss loses its anchor. Your navigation computer recalculates: a narrow corridor toward open space still exists, but it threads through hostile territory, federal patrols, radiation belts, and pirate hunting grounds. There will be no resupply, no allies, no second chances. You grip the helm and set the heading. The escape begins now.") return "你封锁了方舟信标，切断了信号。周围的空间随着星渊失去锚点而扭曲呻吟。导航计算机重新规划：通往开阔太空的狭窄走廊仍然存在，但它穿越了敌占区、联邦巡逻带、辐射带和海盗猎场。没有补给，没有盟友，没有第二次机会。你紧握舵盘，设定航向。逃离现在开始。";
    if (text == "Engage engines") return "启动引擎";
    if (text == "Commit to the escape route. There is no turning back.") return "踏上逃离路线。无法回头。";
    if (text == "Engines engaged. The escape corridor opens ahead.") return "引擎已启动。逃离走廊在前方展开。";
    // Escape route event status lines
    if (text == "Paid the transit fee. -12 credits.") return "支付了通行费。-12 金币。";
    if (text == "Not enough credits for the transit fee.") return "金币不足，无法支付通行费。";
    if (text == "Powered down and drifted through. +4 heat.") return "关闭动力漂移通过。+4 热量。";
    if (text == "Forced through the checkpoint! Battle!") return "强行突破了检查站！战斗！";
    if (text == "Emergency repairs. +10 HP.") return "紧急维修。+10 生命。";
    if (text == "Cleaned heat exchangers. -6 heat.") return "清洁了热交换器。-6 热量。";
    if (text == "Scavenged supplies. +14 credits.") return "搜刮了补给。+14 金币。";
    if (text == "Full speed through the radiation belt. Skip 1 segment, -6 HP.") return "全速穿越辐射带。跳过 1 个节点，-6 生命。";
    if (text == "Slow detour around the belt. +3 heat.") return "绕道避开辐射带。+3 热量。";
    if (text == "Anti-rad shield active. +2 permanent max HP, -10 credits.") return "防辐射护盾启动。+2 永久最大生命，-10 金币。";
    if (text == "Not enough credits for the anti-rad shield.") return "金币不足，无法启动防辐射护盾。";
    if (text == "Paid off the pirates. -15 credits.") return "花钱打发了海盗。-15 金币。";
    if (text == "Not enough credits to pay off the pirates.") return "金币不足，无法打发海盗。";
    if (text == "Jettisoned cargo. Lost 1 random card.") return "抛弃了货物。失去 1 张随机卡牌。";
    if (text == "Counter-attacking the pirates! Elite battle!") return "反击海盗！精英战斗！";
    if (text == "Took the medical kit. +8 HP.") return "取走了医疗包。+8 生命。";
    if (text == "Drained fuel. +2 initial energy next battle.") return "抽取了燃料。下场战斗初始能量 +2。";
    if (text == "Took the emergency chip. +3 HP recovery.") return "取走了应急芯片。+3 生命恢复。";
    if (text == "Forced the jump. Skip 2 segments. +7 heat.") return "强制跃迁。跳过 2 个节点。+7 热量。";
    if (text == "Stabilized coordinates. -5 heat, -8 credits.") return "稳定了坐标。-5 热量，-8 金币。";
    if (text == "Not enough credits to stabilize.") return "金币不足，无法稳定坐标。";
    if (text == "Abandoned the jump. Gained a card, -5 HP.") return "放弃了跃迁。获得 1 张卡牌，-5 生命。";
    if (text == "Energy breach through the barrier. +6 heat.") return "能量突破屏障。+6 热量。";
    if (text == "Spent supplies to breach the barrier. -18 credits.") return "消耗物资突破了屏障。-18 金币。";
    if (text == "Not enough credits to breach.") return "金币不足，无法突破。";
    if (text == "Found a gap. Lost 5 HP squeezing through.") return "找到了缺口。挤过去失去 5 生命。";
    if (text == "Repaired hull. +5 HP, -12 credits.") return "修复了船体。+5 生命，-12 金币。";
    if (text == "Not enough credits for repairs.") return "金币不足，无法修理。";
    if (text == "Optimized engines. +1 initial energy, +2 heat.") return "优化了引擎。+1 初始能量，+2 热量。";
    if (text == "Removed junk cards. -4 HP.") return "移除了废牌。-4 生命。";
    // Truth route event status lines
    if (text == "Merged memories. 2 cards upgraded.") return "融合了记忆。2 张卡牌已升级。";
    if (text == "Extracted energy. Permanent max HP +6.") return "提取了能量。永久最大生命 +6。";
    if (text == "Saved the log. Gained a Truth card.") return "保存了日志。获得 1 张真相卡牌。";
    if (text == "Force breakthrough. Gained a rare relic. -8 HP.") return "强行突破。获得稀有遗物。-8 生命。";
    if (text == "Override permissions. +2 initial energy next battle, +5 heat.") return "覆写权限。下场战斗初始能量 +2，+5 热量。";
    if (text == "Sneaked around the AI. -12 credits.") return "潜行绕过了 AI。-12 金币。";
    if (text == "Not enough credits to bypass.") return "金币不足，无法绕过。";
    if (text == "Listened to the truth. Gained a legendary card.") return "聆听了真相。获得 1 张传说卡牌。";
    if (text == "Gained ship permissions. +4 permanent opening shield.") return "获取了飞船权限。+4 永久初始护盾。";
    if (text == "Drained core energy. -10 heat, +10 HP.") return "抽取了核心能量。-10 热量，+10 生命。";
    if (text == "Engines full. +8 heat.") return "引擎全力运转。+8 热量。";
    if (text == "Dumped supplies to brace. -20 credits.") return "丢弃物资加固。-20 金币。";
    if (text == "Not enough credits.") return "金币不足。";
    if (text == "Emergency dodge. -9 HP.") return "紧急闪避。-9 生命。";
    if (text == "Accepted the truth. All cards permanently enhanced.") return "接受了真相。所有卡牌永久增强。";
    if (text == "Extracted the origin. Gained ultimate relic, -10 HP.") return "提取了起源。获得终极遗物，-10 生命。";
    if (text == "The signal fades. Nothing to be done.") return "信号消散了。无能为力。";
    // Epilogues (Category A – Rescue & Exploration)
    if (text == "The distress signal fades into deep space. You don't know if you did the right thing.") return "求救信号渐渐消失在深空中。你不知道自己做的是否正确。";
    if (text == "Another wrecked ship left behind.") return "又一艘残骸被遗弃了。";
    if (text == "People die in the Star Abyss every day. One more or less makes no difference.") return "星渊中每天都有人死去。多一个少一个并无区别。";
    // Epilogues (Category C – Ship Malfunctions)
    if (text == "Malfunction temporarily resolved. But the ship won't hold forever.") return "故障暂时排除。但飞船不会永远撑下去。";
    if (text == "System alert cleared. The next breakdown could be the next jump away.") return "系统警报已清除。下一次故障也许就在下次跃迁。";
    if (text == "You barely stabilized the hull. But danger never truly fades.") return "你勉强稳住了船体。但危险从未真正消散。";
    // Epilogues (Category D – Black Market & Underworld)
    if (text == "The pirate tracking signal vanishes. You're safe for now.") return "海盗追踪信号消失了。你暂时安全。";
    if (text == "The shadow of black market forces always looms over you.") return "黑市势力的阴影始终笼罩着你。";
    if (text == "You took what wasn't yours. Someday you'll pay it back.") return "你拿走了不属于你的东西。总有一天要还回去。";
    // Ending info.lines — Escape ending
    if (text == "You kept the Ark's call silenced.") return "你压下了方舟的呼唤。";
    if (text == "No deeper descent, no revelations, no final stand.") return "不再深入，不再揭示，不再决战。";
    if (text == "Your prow pierced the mist of the Star Abyss,") return "你的船头刺穿了星渊的迷雾，";
    if (text == "and for the first time, the light of distant stars fell clearly upon the deck.") return "远方星光第一次清晰地洒落在甲板上。";
    if (text == "You survived.") return "你活了下来。";
    if (text == "The secrets of the Star Abyss remain buried,") return "星渊的秘密依旧被掩埋，";
    if (text == "and you are among the very few salvagers to walk away whole.") return "而你是极少数全身而退的打捞者之一。";
    // Ending info.lines — Normal ending
    if (text == "You destroyed the guardian units of the Ark Core,") return "你摧毁了方舟核心的守卫单元，";
    if (text == "and the turmoil of the Star Abyss gradually subsided.") return "星渊的动荡逐渐平息了。";
    if (text == "The world will tell your tale: a salvager who ventured into the dead zone,") return "世人会传颂你的故事：一个闯入死区的打捞者，";
    if (text == "quelled the catastrophe of the forbidden sector, and returned with glory.") return "平息了禁区的灾难，荣耀归来。";
    if (text == "The Federation will seal this chapter of history,") return "联邦会封存这段历史，";
    if (text == "the black market will whisper your name,") return "黑市会低语你的名字，";
    if (text == "and you can finally shed your debts and leave behind this deep space.") return "而你终于可以甩掉债务，离开这片深空。";
    if (text == "Yet in the silence of the return voyage, a faint shadow of doubt lingers.") return "然而在归途的沉寂中，一丝疑虑挥之不去。";
    if (text == "Those undeciphered signals, the misaligned wreckage,") return "那些未解读的信号、错位的残骸、";
    if (text == "the impossible spatial fluctuations...") return "不可能的空间波动……";
    if (text == "They did not vanish with the battle's end,") return "它们并未随战斗结束而消失，";
    if (text == "but transformed into a faint whisper left in your wake.") return "而是化作你身后一缕淡淡的低语。";
    if (text == "You survived the Star Abyss, but never touched its truth.") return "你在星渊中幸存，却从未触及它的真相。";
    // Ending info.lines — True ending
    if (text == "You stand at the origin of everything.") return "你站在一切的起源之处。";
    if (text == "Here there are no wrecks, no enemies, no pirates or federations.") return "这里没有残骸、没有敌人、没有海盗也没有联邦。";
    if (text == "Before you is not a ship's core, nor the heart of a star sector,") return "你面前的不是飞船核心，也不是星区中枢，";
    if (text == "but a universe that was just born and fell immediately into slumber.") return "而是一个刚刚诞生便立即陷入沉睡的宇宙。";
    if (text == "The Ark fleet carried humanity's last hope for continuity,") return "方舟舰队承载着人类延续的最后希望，";
    if (text == "attempting to pry open an entirely new habitable spacetime.") return "试图撬开一个全新的宜居时空。";
    if (text == "They ignited the singularity but could not control its expansion.") return "他们点燃了奇点，却无法控制其膨胀。";
    if (text == "The runaway energy tore apart the fleet, shredded space itself,") return "失控的能量撕裂了舰队、撕碎了空间本身，";
    if (text == "and turned the ground into a chaotic, ever-expanding death zone --") return "将这片区域变成了一个混乱且不断扩张的死亡地带——";
    if (text == "what would come to be known as the Star Abyss.") return "日后被称为星渊的地方。";
    if (text == "The Federation buried the truth, declaring the area forbidden,") return "联邦掩埋了真相，将此区域宣布为禁区，";
    if (text == "because they feared the world would learn:") return "因为他们害怕世人得知：";
    if (text == "humanity once possessed the power of creation,") return "人类曾拥有创世之力，";
    if (text == "and witnessed its collapse with their own hands.") return "并亲手见证了它的崩塌。";
    if (text == "And you -- a debt-ridden salvager who entered the dead zone to survive --") return "而你——一个为了生存而闯入死区的负债打捞者——";
    if (text == "became the only person to reach the singularity and touch the origin.") return "成为了唯一抵达奇点、触碰起源的人。";
    // Route labels
    if (text == "Escape Route") return "逃离路线";
    if (text == "Truth Route") return "真相路线";
    if (text == "Escape: ") return "逃离：";
    if (StartsWith(text, "Escape: ")) return "逃离：" + TranslateGameText(language, text.substr(8));
    if (StartsWith(text, "Truth: ")) return "真相：" + TranslateGameText(language, text.substr(7));
    if (StartsWith(text, "Truth Battle: ")) return "真相战斗：" + TranslateGameText(language, text.substr(14));
    // Costs 8 credits (for cargo dump)
    if (text == "Costs 8 credits.") return "花费 8 金币。";
    // Reward banner
    if (text == "Recovered salvage module") return "回收的打捞模块";
    // Truth route long descriptions
    if (text == "What floats in the Star Abyss is not mere ship wreckage. They are the remains of the first interstellar colonization fleet, the Ark. Every anomalous signal you collected is its voice echoing across billions of spatial residues.") return "星渊中漂浮的并非普通船只残骸。它们是第一支星际殖民舰队——方舟的遗骸。你收集的每一个异常信号，都是它穿越数十亿空间残余的回声。";
    if (text == "The Ark did not crash by accident. It entered this space deliberately. The Federation's Restricted Zone was created to conceal its existence and prevent anyone from discovering its true mission.") return "方舟并非意外坠毁。它是有意进入这片空间的。联邦的禁区就是为了掩盖它的存在，防止任何人发现它真正的使命。";
    if (text == "The Ark's mission was not colonization but a creation experiment. Humanity attempted to use the Ark Core to forge a stable new universe. The Star Abyss is what remained after that experiment failed.") return "方舟的使命并非殖民，而是一次创世实验。人类试图利用方舟核心铸造一个稳定的新宇宙。星渊就是那次实验失败后的遗留。";
    if (text == "The Star Abyss's expansion, spatial distortions, and ship malfunctions are not hazards but a newborn universe struggling and spiraling out of control. Every step deeper draws you closer to this new world's heart.") return "星渊的膨胀、空间扭曲和飞船故障并非灾害，而是一个新生宇宙在挣扎中失控。每深入一步，你都离这个新世界的心脏更近一分。";
    if (text == "The Ark Core is neither an energy source nor a weapon. It is the cosmic singularity of a new world. Whoever controls it can dictate the rules of this space.") return "方舟核心既非能源也非武器。它是一个新世界的宇宙奇点。谁掌控了它，就能主宰这片空间的规则。";
    // Dynamic status with number: "You strip the wreck for X credits."
    if (StartsWith(text, "You strip the wreck for ") && EndsWith(text, " credits.")) {
        const size_t prefix = std::string("You strip the wreck for ").size();
        return "你拆解残骸获得了 " + text.substr(prefix, text.size() - prefix - 9) + " 金币。";
    }
    // Jump vector locked for <node name>.
    if (StartsWith(text, "Jump vector locked for ") && EndsWith(text, ".")) {
        const std::string name = text.substr(22, text.size() - 23);
        return "跃迁矢量已锁定：" + TranslateGameText(language, name) + "。";
    }
    // Sector cleared
    if (text == "Sector cleared. Your crew patches 6 HP while plotting the next route.") return "星区已清除。船员在规划下一航路时修补了 6 生命。";
    if (text == "Run initialized. Chart your first jump.") return "航程已初始化。规划你的第一次跃迁。";
    // Game opening story
    if (text == "Star Abyss Salvager") return "星渊打捞者";
    if (text == "The year is 3071. Humanity spread across the stars centuries ago, but the golden age of expansion ended in silence. Deep in uncharted space lies the Star Abyss -- a vast, churning anomaly where ships vanish and space itself unravels. The Federation declared it a forbidden zone, but salvagers still slip through the cordon, drawn by the wrecks of lost fleets and the promise of fortune. You are one such salvager: broke, desperate, and deep in debt. Your ship is held together by scrap metal and stubbornness. One last haul in the Abyss could clear your ledger -- or bury you alongside the others. The entry vector is locked. Your reactor hums. Whatever lies inside, there is no turning back now.") return "公元3071年。人类在数百年前就已遍布星际，但扩张的黄金时代在沉寂中终结。在未知星域深处，横亘着星渊——一片巨大而动荡的异常区域，飞船在此消失，空间本身也在此崩解。联邦将其宣布为禁区，但打捞者们依然穿过封锁线潜入其中，被失落舰队的残骸和财富的承诺所吸引。你就是这样一名打捞者：身无分文、走投无路、债台高筑。你的飞船靠废铁和倔强勉强维持运转。在星渊中最后搏一把，或许能还清账单——也可能让你和其他人一起长眠于此。进入矢量已锁定。反应堆低声嗡鸣。无论深处有什么，都已无法回头。";
    if (text == "Begin voyage") return "开始航程";
    if (text == "Enter the Star Abyss and start your salvage run.") return "进入星渊，开始你的打捞航程。";
    if (text == "The Abyss awaits. Chart your first jump.") return "星渊在等待。规划你的第一次跃迁。";
    // Auxiliary and conditional-system logs
    if (text == "Auxiliary command protocol duplicates this launch.") return "附属舰协议触发，本次召唤获得了额外复制。";
    if (text == "Auxiliary launch protocol is now enhanced.") return "附属舰发射协议已强化。";
    if (text == "Auxiliary ships will trigger extra times at end of turn.") return "附属舰将在回合结束时额外触发。";
    if (text == "Auxiliary effect output is amplified this turn.") return "本回合附属舰效果已放大。";
    if (text == "Conditional fire-control bonus is active this turn.") return "条件火控加成已在本回合生效。";
    if (text == "Summon-chain draw protocol is armed for this turn.") return "本回合召唤连锁抽牌协议已启用。";
    if (text == "First high-heat entry bonus is triggered.") return "首次进入高热量奖励已触发。";
    if (text == "Recurring high-heat entry bonus draws cards.") return "后续高热量触发了额外抽牌。";
    if (text == "Defense stance release inflicts enemy heat.") return "离开防御形态时已对敌方施加热量。";
    if (text == "Overload entry inflicts enemy heat.") return "进入过载形态时已对敌方施加热量。";
    if (text == "Auxiliary ship formation is recalibrated.") return "附属舰编队参数已重校准。";
    if (text == "Overload instability is damped for this turn.") return "本回合过载不稳定性已缓和。";
    if (text == "Battlefield Breakdown grants +1 permanent attack bonus this battle.") return "战场拆解触发：本场战斗永久攻击加成 +1。";
    if (text == "Overheat feedback is fully blocked this turn.") return "本回合过热反噬已被完全抵消。";
    if (text == "Temporary copy dissipates after use.") return "临时复制牌在使用后消散。";
    if (text == "Blackbox Protocol awaits imprint target selection.") return "黑箱协议等待选择刻印目标。";
    if (text == "Blackbox Protocol cannot imprint a temporary copy.") return "黑箱协议不能刻印临时复制牌。";
    if (text == "Blackbox Protocol found no valid hand card to imprint.") return "黑箱协议未找到可刻印的有效手牌。";
    if (text == "Blackbox imprint replicates into hand.") return "黑箱刻印已在手牌中复制生成。";
    if (StartsWith(text, "Dismantled ") && EndsWith(text, " for immediate reward.")) {
        const std::string name = text.substr(10, text.size() - 31);
        return "拆解了 " + TranslateGameText(language, name) + "，并立即结算收益。";
    }
    if (EndsWith(text, " deployed.")) {
        const std::string name = text.substr(0, text.size() - 10);
        return TranslateGameText(language, name) + " 已部署。";
    }
    if (text == "Auxiliary bay is full. Oldest support craft is dismissed.") return "附属舰槽位已满，最旧单位被替换。";
    if (text == "No auxiliary ship is available to dismantle.") return "当前没有可拆解的附属舰。";
    if (StartsWith(text, " fires for ") && EndsWith(text, ".")) {
        const size_t split = text.find(" fires for ");
        return TranslateGameText(language, text.substr(0, split)) + " 造成 " +
               text.substr(split + 11, text.size() - split - 12) + " 点伤害。";
    }
    if (StartsWith(text, " reinforces shield by ") && EndsWith(text, ".")) {
        const size_t split = text.find(" reinforces shield by ");
        return TranslateGameText(language, text.substr(0, split)) + " 提供了 " +
               text.substr(split + 22, text.size() - split - 23) + " 点护盾。";
    }
    if (StartsWith(text, " executes an immediate attack for ") && EndsWith(text, ".")) {
        const size_t split = text.find(" executes an immediate attack for ");
        return TranslateGameText(language, text.substr(0, split)) + " 立即攻击造成 " +
               text.substr(split + 33, text.size() - split - 34) + " 点伤害。";
    }
    if (StartsWith(text, " grants immediate shield +") && EndsWith(text, ".")) {
        const size_t split = text.find(" grants immediate shield +");
        return TranslateGameText(language, text.substr(0, split)) + " 立即提供 +" +
               text.substr(split + 25, text.size() - split - 26) + " 点护盾。";
    }
    if (StartsWith(text, "Summon-chain protocol draws ") && EndsWith(text, " card(s).")) {
        return "召唤连锁协议抽取了 " + text.substr(28, text.size() - 37) + " 张牌。";
    }
    if (StartsWith(text, "Form resonance draws ") && EndsWith(text, " card(s).")) {
        return "形态共振触发，抽取了 " + text.substr(20, text.size() - 29) + " 张牌。";
    }
    if (StartsWith(text, "First summon this turn adds enemy heat by ") && EndsWith(text, ".")) {
        return "本回合首次召唤使敌方热量增加 " + text.substr(41, text.size() - 42) + " 点。";
    }

    return text;
}
