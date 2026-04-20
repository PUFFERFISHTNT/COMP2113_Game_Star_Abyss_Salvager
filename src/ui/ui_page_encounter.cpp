#include "ui/star_abyss_app.h"

#include <algorithm>
#include <map>
#include <vector>

#include "ui/ui_panels.h"

#include <ftxui/screen/terminal.hpp>

using namespace ftxui;

namespace {

// Service screens reuse the encounter page, so these helpers keep the card-grid
// behavior local instead of leaking service-specific formatting into the engine.
int ServiceCardsPerRow(int available_width, size_t visible_cards) {
    if (visible_cards == 0) {
        return 1;
    }
    if (available_width >= 108) {
        return static_cast<int>(std::min<size_t>(2, visible_cards));
    }
    return 1;
}

int ServiceCardWidth(int available_width, int cards_per_row) {
    const int safe_columns = std::max(1, cards_per_row);
    const int computed_width = std::max(30, (available_width - 4) / safe_columns);
    return std::max(28, std::min(44, computed_width));
}

// Service payloads sometimes refer to upgraded instances via "base@level".
// The preview renderer needs the base id and numeric level separately so it can
// ask the engine for the next upgraded snapshot without mutating game state.
std::string BaseCardRef(const std::string& card_ref) {
    const size_t separator = card_ref.rfind('@');
    if (separator == std::string::npos) {
        return card_ref;
    }
    const std::string suffix = card_ref.substr(separator + 1);
    if (suffix.empty() ||
        !std::all_of(suffix.begin(), suffix.end(), [](unsigned char ch) { return ch >= '0' && ch <= '9'; })) {
        return card_ref;
    }
    return card_ref.substr(0, separator);
}

int CardRefUpgradeLevel(const std::string& card_ref) {
    const size_t separator = card_ref.rfind('@');
    if (separator == std::string::npos) {
        return 0;
    }
    const std::string suffix = card_ref.substr(separator + 1);
    if (suffix.empty() ||
        !std::all_of(suffix.begin(), suffix.end(), [](unsigned char ch) { return ch >= '0' && ch <= '9'; })) {
        return 0;
    }
    try {
        return std::max(0, std::stoi(suffix));
    } catch (...) {
        return 0;
    }
}

std::string MakeCardRef(const std::string& base_id, int upgrade_level) {
    return upgrade_level <= 0 ? base_id : (base_id + "@" + std::to_string(upgrade_level));
}

// Build a compact "before -> after" delta list so upgrade screens can explain
// exactly what changes when the player confirms the service.
void PushUpgradeDelta(std::vector<std::string>* lines,
                      UiLanguage language,
                      const std::string& label_en,
                      const std::string& label_zh,
                      int before,
                      int after) {
    if (lines == nullptr || before == after) {
        return;
    }
    lines->push_back(Tr(language, label_en, label_zh) + " " + std::to_string(before) + " -> " + std::to_string(after));
}

std::vector<std::string> UpgradeDeltaLines(UiLanguage language, const CardDef& current, const CardDef& upgraded) {
    std::vector<std::string> lines;
    PushUpgradeDelta(&lines, language, "Cost", "费用", current.cost, upgraded.cost);
    PushUpgradeDelta(&lines, language, "Damage", "伤害", current.damage, upgraded.damage);
    PushUpgradeDelta(&lines, language, "Shield", "护盾", current.shield, upgraded.shield);
    PushUpgradeDelta(&lines, language, "Heal", "治疗", current.heal, upgraded.heal);
    PushUpgradeDelta(&lines, language, "Draw", "抽牌", current.draw, upgraded.draw);
    PushUpgradeDelta(&lines, language, "Energy", "能量", current.energy, upgraded.energy);
    PushUpgradeDelta(&lines, language, "Next-turn energy", "下回合能量", current.nextTurnEnergy, upgraded.nextTurnEnergy);
    PushUpgradeDelta(&lines, language, "Self heat", "自身热量", current.heat, upgraded.heat);
    PushUpgradeDelta(&lines, language, "Enemy heat", "敌方热量", current.enemyHeat, upgraded.enemyHeat);
    PushUpgradeDelta(&lines, language, "Self damage", "自伤", current.selfDamage, upgraded.selfDamage);
    PushUpgradeDelta(&lines, language, "Hits", "连击次数", current.multiHit, upgraded.multiHit);
    PushUpgradeDelta(&lines, language, "Combo bonus", "连携加成", current.comboBonus, upgraded.comboBonus);
    PushUpgradeDelta(&lines, language, "Attack multiplier", "攻击倍率", current.attackMultiplier, upgraded.attackMultiplier);
    PushUpgradeDelta(&lines, language, "Weaken turns", "削弱回合", current.weakenTurns, upgraded.weakenTurns);
    PushUpgradeDelta(&lines, language, "Shield lock", "禁盾回合", current.shieldLockTurns, upgraded.shieldLockTurns);
    PushUpgradeDelta(&lines, language, "Salvage bonus", "额外金币", current.salvageBonus, upgraded.salvageBonus);
    if (current.removeEnemyShield != upgraded.removeEnemyShield) {
        if (upgraded.removeEnemyShield >= 999) {
            lines.push_back(Tr(language,
                               "Remove enemy shield: partial -> full",
                               "移除敌方护盾：部分 -> 全部"));
        } else {
            PushUpgradeDelta(&lines, language, "Enemy shield removed", "移除敌方护盾", current.removeEnemyShield, upgraded.removeEnemyShield);
        }
    }
    return lines;
}

}  // namespace

// Encounter rendering branches the most by event type. This module keeps market
// UI, generic events, and purge flows together without leaking into battle/map.
Element StarAbyssApp::RenderEncounter() const {
    const RunState& state = engine_.state();
    const int terminal_width = Terminal::Size().dimx;
    const bool compact_layout = terminal_width < 118;
    const int status_panel_width = std::max(22, std::min(28, terminal_width / 5));
    const int signal_text_width = compact_layout ? std::max(24, terminal_width - 10)
                                                 : std::max(28, terminal_width - status_panel_width - 14);
    const size_t visible_options = std::min(state.encounter.choices.size(), encounterButtons_.size());
    Elements options;
    const bool is_upgrade_screen =
        state.encounter.encounterId == "market_upgrade" || state.encounter.encounterId == "event_upgrade" ||
        state.encounter.encounterId == "repair_upgrade";
    const bool is_purge_screen =
        state.encounter.encounterId == "market_purge" || state.encounter.encounterId == "event_purge";
    const bool is_service_card_screen = is_upgrade_screen || is_purge_screen;
    const bool use_full_upgrade_gallery = is_upgrade_screen;
    serviceCardBoxes_.clear();
    serviceCardChoiceIndices_.clear();
    serviceActionVisible_ = false;
    serviceActionChoiceIndex_ = 0;
    serviceHoveredCardIndex_ = -1;
    for (size_t i = 0; i < visible_options; ++i) {
        const std::string payload = i < state.encounter.payloadIds.size() ? state.encounter.payloadIds[i] : "";
        const int price = i < state.encounter.payloadValues.size() ? state.encounter.payloadValues[i] : 0;
        const std::string detail = i < state.encounter.details.size() ? TranslateGameText(language_, state.encounter.details[i])
                                                                      : TranslateGameText(language_, "Unavailable");

        if (use_full_upgrade_gallery && !payload.empty() && payload != "service_skip_service" &&
            payload != "service_back_market") {
            continue;
        }

        Elements body = {encounterButtons_[i]->Render(), separator()};
        if (state.encounter.typeId == "market" && StartsWith(payload, "card:")) {
            const std::string card_ref = payload.substr(5);
            const CardDef* base_card = engine_.cardById(card_ref);
            const CardDef card = engine_.resolvedCard(card_ref);
            body.push_back(text(Tr(language_, "Card Offer", "卡牌商品")) | color(kHull));
            body.push_back(text(Tr(language_, "Price ", "售价 ") + std::to_string(price) + Tr(language_, " credits", " 金币")) | color(kGold));
            body.push_back(text(Tr(language_, "Card Cost: ", "卡牌费用：") + std::to_string(base_card != nullptr ? card.cost : 0) + "   " +
                                Tr(language_, "Type: ", "类型：") +
                                (base_card != nullptr ? TranslateGameText(language_, card.type) : TranslateGameText(language_, "Unavailable"))) |
                           color(kDim));
            body.push_back(separator());
            body.push_back(paragraph(base_card != nullptr ? CardDescriptionText(language_, card) : detail) | color(Color::White));
            if (base_card != nullptr) {
                body.push_back(separator());
                for (const std::string& fact : CardFacts(language_, card)) {
                    body.push_back(text(fact) | color(kDim));
                }
            }
            const Color frame_color = CardFrameColor(base_card != nullptr ? &card : nullptr);
            options.push_back(vbox({
                                 text(std::to_string(i + 1) + ". " + encounterLabels_[i]) | color(kGold) | bold,
                                 separator(),
                                 vbox(std::move(body)),
                             }) |
                             bgcolor(kPanel) |
                             borderStyled(ROUNDED, frame_color));
            continue;
        } else if (state.encounter.typeId == "market" && payload == "upgrade") {
            body.push_back(text(Tr(language_, "Service", "商店服务")) | color(kHull));
            body.push_back(text(Tr(language_, "Price ", "售价 ") + std::to_string(price) + Tr(language_, " credits", " 金币")) | color(kGold));
            body.push_back(separator());
            body.push_back(paragraph(detail) | color(Color::White));
        } else if (state.encounter.typeId == "market" && payload == "purge") {
            body.push_back(text(Tr(language_, "Service", "商店服务")) | color(kHull));
            body.push_back(text(Tr(language_, "Price ", "售价 ") + std::to_string(price) + Tr(language_, " credits", " 金币")) | color(kGold));
            body.push_back(separator());
            body.push_back(paragraph(detail) | color(Color::White));
        } else if ((is_upgrade_screen || is_purge_screen) &&
                   !payload.empty() &&
                   payload != "service_prev_purge" &&
                   payload != "service_cycle_purge" &&
                   payload != "service_prev_upgrade" &&
                   payload != "service_cycle_upgrade" &&
                   payload != "service_back_market" &&
                   payload != "service_skip_service") {
            const CardDef* base_card = engine_.cardById(payload);
            const CardDef card = engine_.resolvedCard(payload);
            body.push_back(text(is_upgrade_screen ? Tr(language_, "Upgrade Service", "升级服务")
                                                  : Tr(language_, "Purge Service", "移除服务")) |
                           color(is_upgrade_screen ? kHull : kEnemy));
            if (price > 0) {
                body.push_back(text(Tr(language_, "Price ", "售价 ") + std::to_string(price) + Tr(language_, " credits", " 金币")) | color(kGold));
            } else {
                body.push_back(text(Tr(language_, "Price Free", "价格：免费")) | color(kGold));
            }
            body.push_back(separator());
            body.push_back(paragraph(detail) | color(Color::White));
            if (base_card != nullptr) {
                body.push_back(separator());
                body.push_back(text(Tr(language_, "Target card: ", "目标卡牌：") + TranslateGameText(language_, card.name)) | color(kEnemy));
                body.push_back(paragraph(CardDescriptionText(language_, card)) | color(kDim));
                if (is_upgrade_screen) {
                    const std::string preview_ref = MakeCardRef(BaseCardRef(payload), CardRefUpgradeLevel(payload) + 1);
                    const CardDef preview = engine_.resolvedCard(preview_ref);
                    const std::vector<std::string> deltas = UpgradeDeltaLines(language_, card, preview);
                    if (!deltas.empty()) {
                        body.push_back(separator());
                        body.push_back(text(Tr(language_, "Upgrade after:", "升级后效果：")) | color(kGold));
                        body.push_back(paragraph(CardDescriptionText(language_, preview)) | color(Color::White));
                        for (const std::string& line : deltas) {
                            body.push_back(text("+ " + line) | color(kGold));
                        }
                    }
                }
            }
        } else if (payload == "service_prev_purge" || payload == "service_cycle_purge" ||
                   payload == "service_prev_upgrade" || payload == "service_cycle_upgrade") {
            body.push_back(text(Tr(language_, "Browse More", "查看更多")) | color(kHull));
            body.push_back(separator());
            body.push_back(paragraph(detail) | color(Color::White));
        } else {
            body.push_back(paragraph(detail) | color(Color::White));
        }

        options.push_back(window(text(std::to_string(i + 1) + ". " + encounterLabels_[i]) | color(kGold), vbox(std::move(body))) | bgcolor(kPanel));
    }

    Element service_deck_panel = text("");
    if (is_service_card_screen) {
        std::vector<CardOwnershipEntry> owned_cards = engine_.ownedCards();
        std::sort(owned_cards.begin(), owned_cards.end(), [&](const CardOwnershipEntry& lhs, const CardOwnershipEntry& rhs) {
            const std::string left_name = TranslateGameText(language_, lhs.card.def.name);
            const std::string right_name = TranslateGameText(language_, rhs.card.def.name);
            if (left_name != right_name) {
                return left_name < right_name;
            }
            return lhs.card.ref < rhs.card.ref;
        });

        std::map<std::string, int> selectable_hotkeys;
        for (size_t i = 0; i < visible_options; ++i) {
            const std::string payload = i < state.encounter.payloadIds.size() ? state.encounter.payloadIds[i] : "";
            if (!payload.empty() && payload != "service_skip_service" && payload != "service_back_market" &&
                payload != "leave") {
                selectable_hotkeys[payload] = static_cast<int>(i) + 1;
            }
        }

        const int panel_width = use_full_upgrade_gallery
                                    ? std::max(36, terminal_width - 10)
                                    : (compact_layout ? std::max(36, terminal_width - 10)
                                                      : std::max(42, terminal_width - status_panel_width - 46));
        const int cards_per_row = ServiceCardsPerRow(panel_width, owned_cards.size());
        serviceCardsPerRow_ = std::max(1, cards_per_row);
        const int card_width = ServiceCardWidth(panel_width, cards_per_row);
        size_t selectable_count = 0;
        for (const CardOwnershipEntry& entry : owned_cards) {
            if (selectable_hotkeys.find(entry.card.ref) != selectable_hotkeys.end()) {
                ++selectable_count;
            }
        }
        if (selectable_count == 0) {
            serviceFocusedCardIndex_ = 0;
        } else {
            serviceFocusedCardIndex_ = std::max(0, std::min(serviceFocusedCardIndex_, static_cast<int>(selectable_count) - 1));
        }
        serviceCardBoxes_.assign(selectable_count, {});
        serviceCardChoiceIndices_.assign(selectable_count, 0);
        size_t selectable_index = 0;
        Elements card_rows;
        Elements current_row;
        for (const CardOwnershipEntry& entry : owned_cards) {
            const auto found = selectable_hotkeys.find(entry.card.ref);
            const bool selectable = found != selectable_hotkeys.end();
            const bool focused_target = selectable &&
                                        static_cast<int>(selectable_index) ==
                                            (serviceHoveredCardIndex_ >= 0 ? serviceHoveredCardIndex_ : serviceFocusedCardIndex_);
            const CardDef* preview_ptr = nullptr;
            CardDef preview_card;
            if (selectable && is_upgrade_screen && entry.card.upgradeLevel < kCardUpgradeMaxLevel) {
                // Preview the next upgrade level using a synthetic card ref so
                // the player can inspect the result before committing.
                const std::string preview_ref = MakeCardRef(BaseCardRef(entry.card.ref), entry.card.upgradeLevel + 1);
                preview_card = engine_.resolvedCard(preview_ref);
                preview_ptr = &preview_card;
            }
            Element card_panel = ServiceDeckCardPanel(language_,
                                                     entry,
                                                     selectable,
                                                     selectable ? found->second : 0,
                                                     use_full_upgrade_gallery,
                                                     focused_target,
                                                     is_purge_screen,
                                                     card_width,
                                                     preview_ptr);
            if (use_full_upgrade_gallery && selectable) {
                serviceCardChoiceIndices_[selectable_index] = static_cast<size_t>(found->second - 1);
                card_panel |= reflect(serviceCardBoxes_[selectable_index]);
                ++selectable_index;
            }
            current_row.push_back(std::move(card_panel));
            if (static_cast<int>(current_row.size()) == cards_per_row) {
                card_rows.push_back(hbox(std::move(current_row)));
                current_row = {};
            }
        }
        if (!current_row.empty()) {
            card_rows.push_back(hbox(std::move(current_row)));
        }

        Elements body_cards;
        for (size_t row_index = 0; row_index < card_rows.size(); ++row_index) {
            if (row_index > 0) {
                body_cards.push_back(separator());
            }
            body_cards.push_back(std::move(card_rows[row_index]));
        }
        if (body_cards.empty()) {
            body_cards.push_back(text(Tr(language_, "No cards are available in the current deck.", "当前牌组中没有可显示的卡牌。")) | color(kDim));
        }

        const int total_owned_cards = static_cast<int>(state.player.drawPile.size() + state.player.hand.size() + state.player.discardPile.size());
        Elements service_body = {
            text(Tr(language_, "Current deck: ", "当前牌组：") + std::to_string(total_owned_cards) +
                 Tr(language_, " total cards across ", " 张卡牌，共 ") +
                 std::to_string(owned_cards.size()) +
                 Tr(language_, " card types.", " 种卡牌。")) |
                color(Color::White),
        };
        if (use_full_upgrade_gallery) {
            service_body.push_back(text(Tr(language_,
                                           "Hover to highlight. Click a card or use Up/Down/Left/Right and Enter to upgrade. Scroll vertically to browse the full deck.",
                                           "鼠标悬停可高亮；点击卡牌，或用上下左右键配合回车升级；上下滚动可浏览完整牌组。")) |
                                   color(kGold));
        } else {
            service_body.push_back(text(is_purge_screen
                                            ? Tr(language_,
                                                 "Cards marked with hotkeys on this page can be purged now.",
                                                 "当前页带快捷键标记的卡牌可立即移除。")
                                            : Tr(language_,
                                                 "Cards marked with hotkeys on this page can be upgraded now.",
                                                 "当前页带快捷键标记的卡牌可立即升级。")) |
                                   color(kGold));
        }
        if (use_full_upgrade_gallery || !selectable_hotkeys.empty()) {
            service_body.push_back(text(Tr(language_, "Focused card is highlighted in bright gold.", "当前选中的卡牌会以亮金色强高亮显示。")) |
                                   color(Color::RGB(255, 245, 160)));
        } else if (!use_full_upgrade_gallery) {
            service_body.push_back(text(Tr(language_, "Move focus onto a target button to highlight that card here.", "把焦点移动到左侧目标按钮上，就会在这里同步高亮对应卡牌。")) |
                                   color(kDim));
        }
        if (!options.empty()) {
            serviceActionVisible_ = true;
            serviceActionChoiceIndex_ = state.encounter.choices.size() - 1;
            service_body.push_back(separator());
            service_body.push_back(window(text(Tr(language_, "Service Action", "服务操作")) | color(kHull),
                                          vbox(std::move(options))) | reflect(serviceActionBox_) | bgcolor(kPanel));
        }
        service_body.push_back(separator());
        service_body.push_back(vbox(std::move(body_cards)));
        service_deck_panel =
            window(text(is_purge_screen ? Tr(language_, "Deck Purge Targets", "牌组移除总览")
                                        : Tr(language_, "Deck Upgrade Targets", "牌组升级总览")) |
                       color(is_purge_screen ? kEnemy : kHull) | bold,
                   vbox(std::move(service_body))) |
            bgcolor(kPanelAlt);
    }

    if (state.encounter.typeId == "market") {
        std::vector<std::string> broker_lines = {
            Tr(language_, "Credits on hand: ", "当前金币：") + std::to_string(state.player.scrap),
            Tr(language_, "Card offers cost 28 credits each.", "每张卡牌售价 28 金币。"),
            Tr(language_, "Upgrade service cost rises: 50, 100, then doubles each time.", "升级服务价格递增：50、100，之后每次翻倍。"),
            state.marketPurgeUsed
                ? Tr(language_, "Purge service already used on this visit.", "本次进店的删牌服务已使用。")
                : Tr(language_, "Purge service costs 20 credits and can be used once per visit.", "删牌服务价格 20 金币，每次进店只能使用一次。"),
        };
        if (state.encounter.encounterId == "market_purge") {
            broker_lines.push_back(Tr(language_, "Pick a target card, browse more pages, or go back to the broker.", "选择要移除的卡牌、浏览更多页面，或返回商店。"));
        } else if (state.encounter.encounterId == "market_upgrade") {
            broker_lines.push_back(Tr(language_, "Pick a target card, browse more pages, or go back to the broker.", "选择要升级的卡牌、浏览更多页面，或返回商店。"));
        } else {
            broker_lines.push_back(Tr(language_, "Buy a card, upgrade a card, purge once, or leave the channel.", "购买卡牌、升级卡牌、删一次牌，或直接离开频道。"));
        }
        Element market_header = compact_layout
                                    ? vbox({
                                          window(text(Tr(language_, "Market Feed", "商店讯号")) | color(kHull),
                                                 paragraph(TranslateGameText(language_, state.encounter.description)) | color(Color::White)) |
                                              bgcolor(kPanel),
                                          InfoPanel(language_, Tr(language_, "Broker Ledger", "商店账本"), broker_lines, kGold),
                                      })
                                    : hbox({
                                          window(text(Tr(language_, "Market Feed", "商店讯号")) | color(kHull),
                                                 paragraph(TranslateGameText(language_, state.encounter.description)) | color(Color::White)) |
                                              flex | bgcolor(kPanel),
                                          InfoPanel(language_, Tr(language_, "Broker Ledger", "商店账本"), broker_lines, kGold) | flex,
                                      });
        Element body_section =
            use_full_upgrade_gallery
                ? service_deck_panel
                : is_service_card_screen && !compact_layout
                ? hbox({
                      vbox(std::move(options)) | flex,
                      separator(),
                      service_deck_panel | flex,
                  })
                : vbox({
                      vbox(std::move(options)),
                      is_service_card_screen ? separator() : separatorEmpty(),
                      is_service_card_screen ? service_deck_panel : text(""),
                  });
        Element content = vbox({
            text(TranslateGameText(language_, state.encounter.title)) | bold | color(kGold) | center,
            separatorDouble(),
            market_header,
            separator(),
            body_section,
            separator(),
            text(TranslateGameText(language_, state.statusLine)) | color(kHull),
            hbox({encounterOwnedCardsButton_->Render() | flex, encounterSaveButton_->Render() | flex, encounterMenuButton_->Render() | flex}),
        });
        return ApplyVerticalScrollFrame(content, pageScrollLines_[PageIndex(AppPage::Encounter)]) | borderDouble | bgcolor(kAbyss);
    }

    Element signal_header = compact_layout
                                ? vbox({
                                      window(text(Tr(language_, "Signal", "信号内容")) | color(kHull),
                                             WrappedParagraph(TranslateGameText(language_, state.encounter.description), signal_text_width) | color(Color::White)) |
                                          bgcolor(kPanel),
                                      InfoPanel(language_, Tr(language_, "Ship Status", "飞船状态"), PlayerStatusLines(language_, state), kHull),
                                  })
                                : hbox({
                                      window(text(Tr(language_, "Signal", "信号内容")) | color(kHull),
                                             WrappedParagraph(TranslateGameText(language_, state.encounter.description), signal_text_width) | color(Color::White)) |
                                          flex | bgcolor(kPanel),
                                      InfoPanel(language_, Tr(language_, "Ship Status", "飞船状态"), PlayerStatusLines(language_, state), kHull) |
                                          size(WIDTH, EQUAL, status_panel_width),
                                  });

    Element body_section =
        use_full_upgrade_gallery
            ? service_deck_panel
            : is_service_card_screen && !compact_layout
            ? hbox({
                  vbox(std::move(options)) | flex,
                  separator(),
                  service_deck_panel | flex,
              })
            : vbox({
                  vbox(std::move(options)),
                  is_service_card_screen ? separator() : separatorEmpty(),
                  is_service_card_screen ? service_deck_panel : text(""),
              });

    Element content = vbox({
        text(TranslateGameText(language_, state.encounter.title)) | bold | color(kGold) | center,
        separatorDouble(),
        signal_header,
        separator(),
        body_section,
        separator(),
        text(TranslateGameText(language_, state.statusLine)) | color(kHull),
        hbox({encounterOwnedCardsButton_->Render() | flex, encounterSaveButton_->Render() | flex, encounterMenuButton_->Render() | flex}),
    });

    return ApplyVerticalScrollFrame(content, pageScrollLines_[PageIndex(AppPage::Encounter)]) | borderDouble | bgcolor(kAbyss);
}
