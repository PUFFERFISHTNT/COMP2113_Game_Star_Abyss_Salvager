#include "ui/ui_localization.h"

#include <string>
#include <utility>

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
    if (StartsWith(text, "Select one card to erase from your deck. The black market visit ends immediately after the purge. Page ") &&
        text.find(". Starter cards may appear on later pages.") != std::string::npos) {
        const size_t page_start = std::string("Select one card to erase from your deck. The black market visit ends immediately after the purge. Page ").size();
        const size_t page_end = text.find(". Starter cards may appear on later pages.");
        return "选择 1 张卡牌从卡组中彻底抹除。执行移除后，本次黑市访问会立刻结束。第 " +
               text.substr(page_start, page_end - page_start) + " 页。初始卡可能会出现在后续页面。";
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
    if (StartsWith(text, "Heat is dangerous: overheat damages your hull and can reduce your next draw to 4 cards.")) {
        return "热量非常危险：过热会伤害你的船体，并让你下回合只抽 4 张牌。";
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
