#!/usr/bin/env python3
# English: One-shot tool to rescale every card's "upgrade" block (~30% numeric bump),
# align upgrade cost rules, and rewrite descriptions so numbers match JSON stats.

from __future__ import annotations

import json
import math
import re
from collections import defaultdict
from copy import deepcopy
from pathlib import Path

SCALE = 1.3

# All int-like JSON keys the game loader understands (see game_card_json.cpp).
ALL_INT_KEYS = [
    "cost",
    "damage",
    "shield",
    "heal",
    "draw",
    "energy",
    "nextTurnEnergy",
    "heat",
    "enemyHeat",
    "selfDamage",
    "multiHit",
    "comboBonus",
    "attackMultiplier",
    "weakenTurns",
    "removeEnemyShield",
    "shieldLockTurns",
    "salvageBonus",
    "summonDroneCount",
    "summonDroneAttack",
    "summonDroneShield",
    "summonDroneDuration",
    "buffDroneAttack",
    "buffDroneShield",
    "auxImmediateTriggerTimes",
    "auxEndTurnExtraTriggerTimes",
    "auxEffectPercentBonus",
    "auxDestroyCount",
    "auxDestroyRewardMultiplierPercent",
    "firstSummonExtraCopies",
    "firstSummonExtraScope",
    "summonCardPlayedDrawThisTurn",
    "formChangesThisTurnThreshold",
    "formChangesThisTurnBonusDamage",
    "firstEnterHighHeatEnergyThisBattle",
    "firstEnterHighHeatDrawThisBattle",
    "eachEnterHighHeatDrawThisBattle",
    "firstSummonEnemyHeatPerTurnThisBattle",
    "firstSummonEnemyHeatAttackBonusPerTurnThisBattle",
    "enterOverloadEnemyHeatThisBattle",
    "leaveDefenseEnemyHeatThisBattle",
    "formShift",
    "overloadIncomingDamageReducePercent",
    "blackboxCopyCount",
    "leaveDefenseEnergyBonus",
    "leaveDefenseShieldBonus",
    "starterCopies",
]

BOOL_KEYS = [
    "blackboxProtocol",
    "formToggle",
    "formExit",
    "ignoreShield",
    "shieldToDamage",
    "halveHeat",
    "clearHeat",
    "randomEffect",
]

FIELD_MATCH_PRIORITY = [
    "damage",
    "shield",
    "heal",
    "enemyHeat",
    "heat",
    "selfDamage",
    "draw",
    "energy",
    "nextTurnEnergy",
    "multiHit",
    "comboBonus",
    "attackMultiplier",
    "weakenTurns",
    "shieldLockTurns",
    "salvageBonus",
    "summonDroneCount",
    "summonDroneAttack",
    "summonDroneShield",
    "summonDroneDuration",
    "buffDroneAttack",
    "buffDroneShield",
    "auxImmediateTriggerTimes",
    "auxEndTurnExtraTriggerTimes",
    "auxEffectPercentBonus",
    "auxDestroyCount",
    "auxDestroyRewardMultiplierPercent",
    "firstSummonExtraCopies",
    "summonCardPlayedDrawThisTurn",
    "formChangesThisTurnThreshold",
    "formChangesThisTurnBonusDamage",
    "firstEnterHighHeatEnergyThisBattle",
    "firstEnterHighHeatDrawThisBattle",
    "eachEnterHighHeatDrawThisBattle",
    "firstSummonEnemyHeatPerTurnThisBattle",
    "firstSummonEnemyHeatAttackBonusPerTurnThisBattle",
    "enterOverloadEnemyHeatThisBattle",
    "leaveDefenseEnemyHeatThisBattle",
    "overloadIncomingDamageReducePercent",
    "blackboxCopyCount",
    "leaveDefenseEnergyBonus",
    "leaveDefenseShieldBonus",
    "removeEnemyShield",
    "cost",
]


def scale_numeric_value(v: int) -> int:
    """English: ~+30% for positive counts; mirror magnitude for negative debuffs/cooling."""
    if v == 0:
        return 0
    if v > 0:
        return max(1, int(math.ceil(v * SCALE)))
    return -max(1, int(math.ceil(abs(v) * SCALE)))


def should_scale_key(key: str, base_val: int) -> bool:
    if key in ("starterCopies", "formShift", "firstSummonExtraScope"):
        return False
    if key == "removeEnemyShield" and abs(base_val) <= 1:
        return False
    if key == "multiHit" and base_val <= 1:
        return False
    if key == "attackMultiplier" and base_val <= 1:
        return False
    return True


def get_int(card: dict, key: str) -> int:
    v = card.get(key, 0)
    if isinstance(v, bool):
        return 0
    try:
        return int(v)
    except (TypeError, ValueError):
        return 0


def has_numeric_gameplay(card: dict) -> bool:
    for key in ALL_INT_KEYS:
        if key in ("cost", "starterCopies", "formShift", "firstSummonExtraScope"):
            continue
        v = get_int(card, key)
        if v == 0:
            continue
        if key == "multiHit" and v <= 1:
            continue
        if key == "attackMultiplier" and v <= 1:
            continue
        if key == "removeEnemyShield" and abs(v) <= 1:
            continue
        return True
    return False


def build_field_pool(source: dict) -> dict[int, list[str]]:
    pool: dict[int, list[str]] = defaultdict(list)
    for key in FIELD_MATCH_PRIORITY:
        pool[get_int(source, key)].append(key)
    return dict(pool)


def pool_take(pool: dict[int, list[str]], n: int, preferred: list[str]) -> str | None:
    row = pool.get(n, [])
    for p in preferred:
        if p in row:
            row.remove(p)
            return p
    if row:
        k = row.pop(0)
        return k
    return None


def pool_remove_key(pool: dict[int, list[str]], key: str, source: dict) -> None:
    v = get_int(source, key)
    row = pool.get(v, [])
    if key in row:
        row.remove(key)


def context_keys_zh(ctx: str) -> list[str]:
    tail = ctx[-24:]
    if "高热量" in tail or "High Heat" in tail:
        return [
            "firstEnterHighHeatEnergyThisBattle",
            "firstEnterHighHeatDrawThisBattle",
            "eachEnterHighHeatDrawThisBattle",
            "nextTurnEnergy",
            "energy",
            "draw",
        ]
    if "抽" in tail:
        return [
            "draw",
            "eachEnterHighHeatDrawThisBattle",
            "firstEnterHighHeatDrawThisBattle",
            "summonCardPlayedDrawThisTurn",
        ]
    if "能量" in tail:
        return ["nextTurnEnergy", "energy", "firstEnterHighHeatEnergyThisBattle"]
    if "伤害" in tail or "点伤" in tail:
        return ["damage", "comboBonus", "formChangesThisTurnBonusDamage", "multiHit"]
    if "格挡" in tail or "护盾" in tail:
        return ["shield", "summonDroneShield", "leaveDefenseShieldBonus"]
    if "生命" in tail or "治疗" in tail or "恢复" in tail:
        return ["heal"]
    if "敌" in tail and ("热量" in tail or "热" in tail):
        return [
            "enemyHeat",
            "enterOverloadEnemyHeatThisBattle",
            "leaveDefenseEnemyHeatThisBattle",
            "firstSummonEnemyHeatPerTurnThisBattle",
            "firstSummonEnemyHeatAttackBonusPerTurnThisBattle",
        ]
    if "热量" in tail or "热" in tail:
        return ["heat", "enemyHeat"]
    if "回合" in tail and ("形态" in tail or "过载" in tail or "防御" in tail):
        return ["formChangesThisTurnThreshold", "weakenTurns", "shieldLockTurns"]
    if "附属" in tail or "无人机" in tail:
        return [
            "summonDroneCount",
            "summonDroneAttack",
            "summonDroneShield",
            "summonDroneDuration",
            "firstSummonExtraCopies",
        ]
    if "次" in tail and ("触发" in tail or "附属" in tail):
        return ["auxImmediateTriggerTimes", "auxEndTurnExtraTriggerTimes"]
    if "%" in tail or "百分" in tail:
        return ["overloadIncomingDamageReducePercent", "auxEffectPercentBonus", "auxDestroyRewardMultiplierPercent"]
    if "金币" in tail or "废料" in tail or "credits" in tail:
        return ["salvageBonus"]
    return []


def context_keys_en(ctx: str) -> list[str]:
    low = ctx[-48:].lower()
    if "draw" in low:
        return [
            "draw",
            "eachEnterHighHeatDrawThisBattle",
            "firstEnterHighHeatDrawThisBattle",
            "summonCardPlayedDrawThisTurn",
        ]
    if "energy" in low:
        if "next turn" in low or "start of next turn" in low:
            return ["nextTurnEnergy", "energy", "firstEnterHighHeatEnergyThisBattle"]
        return ["energy", "nextTurnEnergy", "firstEnterHighHeatEnergyThisBattle"]
    if "damage" in low or "hit" in low:
        return ["damage", "comboBonus", "formChangesThisTurnBonusDamage", "multiHit"]
    if "block" in low or "shield" in low:
        return ["shield", "summonDroneShield", "leaveDefenseShieldBonus"]
    if "hp" in low or "heal" in low:
        return ["heal", "selfDamage"]
    if "heat" in low and ("enemy" in low or "apply" in low):
        return [
            "enemyHeat",
            "enterOverloadEnemyHeatThisBattle",
            "leaveDefenseEnemyHeatThisBattle",
            "firstSummonEnemyHeatPerTurnThisBattle",
            "firstSummonEnemyHeatAttackBonusPerTurnThisBattle",
        ]
    if "heat" in low:
        return ["heat", "enemyHeat"]
    if "auxiliary" in low or "drone" in low:
        return ["summonDroneCount", "summonDroneAttack", "summonDroneShield", "summonDroneDuration"]
    return []


def composite_specs(source: dict) -> dict[int, dict]:
    out: dict[int, dict] = {}
    a = get_int(source, "firstSummonEnemyHeatPerTurnThisBattle")
    b = get_int(source, "firstSummonEnemyHeatAttackBonusPerTurnThisBattle")
    if a and b:
        out[a + b] = {"kind": "sum", "keys": ["firstSummonEnemyHeatPerTurnThisBattle", "firstSummonEnemyHeatAttackBonusPerTurnThisBattle"]}
    return out


def rewrite_plain_zh(desc: str, source: dict, target: dict) -> str:
    """English: Fix common non-backtick templates (starter cards)."""
    d = desc
    if get_int(source, "damage") > 0:
        d = re.sub(
            r"造成\s*(\d+)\s*伤害",
            lambda m: f"造成 {get_int(target, 'damage')} 伤害" if int(m.group(1)) == get_int(source, "damage") else m.group(0),
            d,
        )
    if get_int(source, "shield") > 0:
        d = re.sub(
            r"获得\s*(\d+)\s*格挡",
            lambda m: f"获得 {get_int(target, 'shield')} 格挡" if int(m.group(1)) == get_int(source, "shield") else m.group(0),
            d,
        )
    return d


def rewrite_plain_en(desc: str, source: dict, target: dict) -> str:
    d = desc
    if get_int(source, "damage") > 0:
        d = re.sub(
            r"Deal\s+(\d+)\s+damage",
            lambda m: f"Deal {get_int(target, 'damage')} damage" if int(m.group(1)) == get_int(source, "damage") else m.group(0),
            d,
            flags=re.IGNORECASE,
        )
    if get_int(source, "shield") > 0:
        d = re.sub(
            r"Gain\s+(\d+)\s+Block",
            lambda m: f"Gain {get_int(target, 'shield')} Block" if int(m.group(1)) == get_int(source, "shield") else m.group(0),
            d,
            flags=re.IGNORECASE,
        )
    return d


def rewrite_zh_backticks(desc: str, source: dict, target: dict) -> str:
    if not desc:
        return desc
    pool = build_field_pool(source)
    composites = composite_specs(source)

    def repl(m: re.Match[str]) -> str:
        start = m.start()
        end_m = m.end()
        ctx = desc[max(0, start - 16) : start] + desc[end_m : min(len(desc), end_m + 12)]
        n = int(m.group(1))
        hv = get_int(source, "heat")
        if hv < 0 and abs(hv) == n and ("降低" in ctx or "失去" in ctx or "热量" in ctx or "热" in ctx):
            return f"`{abs(get_int(target, 'heat'))}`"
        if n in composites:
            spec = composites[n]
            keys = spec["keys"]
            total = sum(get_int(target, k) for k in keys)
            for k in keys:
                pool_remove_key(pool, k, source)
            return f"`{total}`"

        prefs = context_keys_zh(ctx) + FIELD_MATCH_PRIORITY
        key = pool_take(pool, n, prefs)
        if key is not None:
            return f"`{get_int(target, key)}`"
        for fk in FIELD_MATCH_PRIORITY:
            if get_int(source, fk) == n:
                return f"`{get_int(target, fk)}`"
        return f"`{n}`"

    out = re.sub(r"`(\d+)`", repl, desc)
    return rewrite_plain_zh(out, source, target)


def rewrite_en_numbers(desc: str, source: dict, target: dict) -> str:
    if not desc:
        return desc
    pool = build_field_pool(source)
    composites = composite_specs(source)

    def repl(m: re.Match[str]) -> str:
        start = m.start()
        end_m = m.end()
        ctx = desc[max(0, start - 48) : start] + desc[end_m : min(len(desc), end_m + 24)]
        n = int(m.group(1))
        hv = get_int(source, "heat")
        low = ctx.lower()
        if hv < 0 and abs(hv) == n and ("lose" in low or "heat" in low):
            return str(abs(get_int(target, "heat")))
        if n in composites:
            spec = composites[n]
            keys = spec["keys"]
            total = sum(get_int(target, k) for k in keys)
            for k in keys:
                pool_remove_key(pool, k, source)
            return str(total)

        prefs = context_keys_en(ctx) + FIELD_MATCH_PRIORITY
        key = pool_take(pool, n, prefs)
        if key is not None:
            return str(get_int(target, key))
        for fk in FIELD_MATCH_PRIORITY:
            if get_int(source, fk) == n:
                return str(get_int(target, fk))
        return str(n)

    out = re.sub(r"(?<![A-Za-z0-9_])(\d+)(?![A-Za-z0-9_])", repl, desc)
    return rewrite_plain_en(out, source, target)


def apply_data_fixes(card: dict) -> None:
    # English: Keep a few JSON rows aligned with their long-form descriptions.
    if card.get("id") == "cl_08":
        zh = card.get("description", "")
        m = re.search(r"获得 `(\d+)` 能量", zh)
        if m and get_int(card, "nextTurnEnergy") == 0 and get_int(card, "energy") == 0:
            card["nextTurnEnergy"] = int(m.group(1))


def build_scaled_upgrade(base: dict) -> dict:
    numeric = has_numeric_gameplay(base)
    up = deepcopy(base.get("upgrade") or {})
    for bk in BOOL_KEYS:
        if bk in base:
            up[bk] = base[bk]
    for key in ALL_INT_KEYS:
        if key in ("starterCopies",):
            continue
        bv = get_int(base, key)
        if not should_scale_key(key, bv):
            if key in base:
                up[key] = bv
            continue
        if bv == 0 and key not in base:
            up.pop(key, None)
            continue
        if bv == 0:
            up[key] = 0
            continue
        up[key] = scale_numeric_value(bv)

    base_cost = get_int(base, "cost")
    if numeric:
        up["cost"] = base_cost
    else:
        up["cost"] = max(0, base_cost - 1) if base_cost > 0 else 0
    return up


def sync_card(card: dict, report: list[str]) -> None:
    if "upgrade" not in card:
        report.append(f"[skip] {card.get('id', '?')}: no upgrade block")
        return

    apply_data_fixes(card)
    zh_in = card.get("description", "")
    en_in = card.get("description_en", "")
    up = build_scaled_upgrade(card)
    merged_target = {**card, **up}

    # English: Base card copy must describe base stats only (not upgraded stats).
    card["description"] = rewrite_zh_backticks(zh_in, card, card)
    card["description_en"] = rewrite_en_numbers(en_in, card, card)

    # English: Upgrade copy maps numbers against base fields but prints upgraded values.
    up["description"] = rewrite_zh_backticks(up.get("description", zh_in), card, merged_target)
    up["description_en"] = rewrite_en_numbers(up.get("description_en", en_in), card, merged_target)

    card["upgrade"] = up


def main() -> None:
    root = Path(__file__).resolve().parents[1]
    path = root / "data" / "cards.json"
    data = json.loads(path.read_text(encoding="utf-8"))
    cards = data.get("cards", [])
    report: list[str] = []
    for card in cards:
        sync_card(card, report)

    path.write_text(json.dumps(data, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    rep_path = root / "scripts" / "sync_card_upgrades_report.txt"
    rep_path.write_text("\n".join(report) + ("\n" if report else ""), encoding="utf-8")


if __name__ == "__main__":
    main()
