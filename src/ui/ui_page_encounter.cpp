#include "ui/star_abyss_app.h"

#include <algorithm>
#include <vector>

#include "ui/ui_panels.h"

#include <ftxui/screen/terminal.hpp>

using namespace ftxui;

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
        state.encounter.encounterId == "market_upgrade" || state.encounter.encounterId == "event_upgrade";
    const bool is_purge_screen =
        state.encounter.encounterId == "market_purge" || state.encounter.encounterId == "event_purge";
    for (size_t i = 0; i < visible_options; ++i) {
        const std::string payload = i < state.encounter.payloadIds.size() ? state.encounter.payloadIds[i] : "";
        const int price = i < state.encounter.payloadValues.size() ? state.encounter.payloadValues[i] : 0;
        const std::string detail = i < state.encounter.details.size() ? TranslateGameText(language_, state.encounter.details[i])
                                                                      : TranslateGameText(language_, "Unavailable");

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
        Element content = vbox({
            text(TranslateGameText(language_, state.encounter.title)) | bold | color(kGold) | center,
            separatorDouble(),
            market_header,
            separator(),
            vbox(std::move(options)),
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

    Element content = vbox({
        text(TranslateGameText(language_, state.encounter.title)) | bold | color(kGold) | center,
        separatorDouble(),
        signal_header,
        separator(),
        vbox(std::move(options)),
        separator(),
        text(TranslateGameText(language_, state.statusLine)) | color(kHull),
        hbox({encounterOwnedCardsButton_->Render() | flex, encounterSaveButton_->Render() | flex, encounterMenuButton_->Render() | flex}),
    });

    return ApplyVerticalScrollFrame(content, pageScrollLines_[PageIndex(AppPage::Encounter)]) | borderDouble | bgcolor(kAbyss);
}
