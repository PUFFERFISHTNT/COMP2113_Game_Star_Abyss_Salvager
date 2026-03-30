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
            body.push_back(text(Tr(language_, "Card Cost: ", "卡牌费用：") + std::to_string(card != nullptr ? card->cost : 0) + "   " +
                                Tr(language_, "Type: ", "类型：") +
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
            hbox({encounterSaveButton_->Render() | flex, encounterMenuButton_->Render() | flex}),
        });
        return content | vscroll_indicator | yframe | borderDouble | bgcolor(kAbyss);
    }

    Element signal_header = compact_layout
                                ? vbox({
                                      window(text(Tr(language_, "Signal", "信号内容")) | color(kHull),
                                             paragraph(TranslateGameText(language_, state.encounter.description)) | color(Color::White)) |
                                          bgcolor(kPanel),
                                      InfoPanel(language_, Tr(language_, "Ship Status", "飞船状态"), PlayerStatusLines(language_, state), kHull),
                                  })
                                : hbox({
                                      window(text(Tr(language_, "Signal", "信号内容")) | color(kHull),
                                             paragraph(TranslateGameText(language_, state.encounter.description)) | color(Color::White)) |
                                          flex | bgcolor(kPanel),
                                      InfoPanel(language_, Tr(language_, "Ship Status", "飞船状态"), PlayerStatusLines(language_, state), kHull) | flex,
                                  });

    Element content = vbox({
        text(TranslateGameText(language_, state.encounter.title)) | bold | color(kGold) | center,
        separatorDouble(),
        signal_header,
        separator(),
        vbox(std::move(options)),
        separator(),
        text(TranslateGameText(language_, state.statusLine)) | color(kHull),
        hbox({encounterSaveButton_->Render() | flex, encounterMenuButton_->Render() | flex}),
    });

    return content | vscroll_indicator | yframe | borderDouble | bgcolor(kAbyss);
}
