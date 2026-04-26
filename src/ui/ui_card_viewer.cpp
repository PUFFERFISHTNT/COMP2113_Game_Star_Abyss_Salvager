#include "ui/star_abyss_app.h"

#include <algorithm>
#include <vector>

#include "ui/ui_panels.h"

#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/terminal.hpp>

using namespace ftxui;

namespace {

int CardViewerCardsPerRow(int terminal_width, size_t visible_cards) {
    if (visible_cards == 0) {
        return 1;
    }
    if (terminal_width >= 156) {
        return static_cast<int>(std::min<size_t>(3, visible_cards));
    }
    if (terminal_width >= 108) {
        return static_cast<int>(std::min<size_t>(2, visible_cards));
    }
    return 1;
}

int CardViewerCardWidth(int terminal_width, int cards_per_row) {
    const int safe_columns = std::max(1, cards_per_row);
    const int available_width = std::max(32, terminal_width - 18);
    const int computed_width = available_width / safe_columns;
    return std::max(28, std::min(44, computed_width));
}

Element CardViewerEntryPanel(UiLanguage language,
                             const CardView& card_view,
                             int copies,
                             bool show_copies,
                             bool obtained,
                             int card_width) {
    const CardDef& card = card_view.def;
    std::vector<std::string> facts = CardFacts(language, card);
    if (show_copies) {
        facts.insert(facts.begin(),
                     Tr(language, "Copies x", "持有数量 x") + std::to_string(std::max(1, copies)));
    }
    const std::string fact_text = CompactFactText(language, facts);
    const Color title_color = obtained ? CardAccent(&card) : Color::RGB(145, 153, 171);
    const Color frame_color = obtained ? CardFrameColor(&card) : Color::RGB(89, 98, 116);
    const Color background = obtained ? CardBackground(&card, true) : Color::RGB(10, 17, 31);
    const Color meta_color = obtained ? kHull : Color::RGB(120, 133, 154);
    const Color fact_color = obtained ? kDim : Color::RGB(117, 126, 142);
    const int text_width = std::max(12, card_width - 4);

    return vbox({
               text(TranslateGameText(language, card.name) +
                    (show_copies ? (Tr(language, " x", " x") + std::to_string(std::max(1, copies))) : "")) |
                   color(title_color) | bold,
               text(obtained ? Tr(language, "Status: Obtained", "状态：已获得")
                             : Tr(language, "Status: Not yet obtained", "状态：尚未获得")) |
                   color(obtained ? kGold : kDim),
               separator(),
               text(Tr(language, "Cost ", "费用 ") + std::to_string(card.cost) + "   " +
                    Tr(language, "Type: ", "类型：") + TranslateGameText(language, card.type)) |
                   color(meta_color),
               separator(),
               WrappedParagraph(CardDescriptionText(language, card), text_width) |
                   color(obtained ? Color::White : Color::RGB(170, 178, 194)),
               WrappedParagraph(fact_text.empty() ? Tr(language, "No additional card facts.", "没有额外卡牌信息。")
                                                  : fact_text,
                                text_width) |
                   color(fact_color),
           }) |
           bgcolor(background) |
           borderStyled(ROUNDED, frame_color) |
           size(WIDTH, EQUAL, card_width) |
           size(HEIGHT, GREATER_THAN, 10);
}

}  // namespace

Element StarAbyssApp::RenderCardViewer() const {
    const int terminal_width = Terminal::Size().dimx;
    const int terminal_height = Terminal::Size().dimy;
    const int dialog_width = std::min(148, std::max(54, terminal_width - 6));
    const int dialog_height = std::max(18, terminal_height - 6);

    Elements card_rows;
    Elements current_row;
    std::string title;
    std::string summary;

    if (cardViewerMode_ == CardViewerMode::OwnedCards) {
        std::vector<CardOwnershipEntry> owned_cards = engine_.ownedCards();
        std::sort(owned_cards.begin(), owned_cards.end(), [&](const CardOwnershipEntry& lhs, const CardOwnershipEntry& rhs) {
            const std::string left_name = TranslateGameText(language_, lhs.card.def.name);
            const std::string right_name = TranslateGameText(language_, rhs.card.def.name);
            if (left_name != right_name) {
                return left_name < right_name;
            }
            return lhs.card.ref < rhs.card.ref;
        });

        const size_t visible_cards = owned_cards.size();
        const int cards_per_row = CardViewerCardsPerRow(dialog_width, visible_cards);
        const int card_width = CardViewerCardWidth(dialog_width, cards_per_row);
        const int total_owned_cards = static_cast<int>(engine_.state().player.drawPile.size() +
                                                       engine_.state().player.hand.size() +
                                                       engine_.state().player.discardPile.size());
        title = Tr(language_, "Owned Cards", "已拥有卡牌");
        summary = Tr(language_, "Current deck: ", "当前牌组：") + std::to_string(total_owned_cards) +
                  Tr(language_, " total cards across ", " 张卡牌，共 ") +
                  std::to_string(owned_cards.size()) +
                  Tr(language_, " card types.", " 种卡牌。");

        if (owned_cards.empty()) {
            card_rows.push_back(text(Tr(language_, "No owned cards are available right now.", "当前没有可显示的已拥有卡牌。")) | color(kDim));
        } else {
            for (const CardOwnershipEntry& entry : owned_cards) {
                current_row.push_back(CardViewerEntryPanel(language_, entry.card, entry.copies, true, true, card_width));
                if (static_cast<int>(current_row.size()) == cards_per_row) {
                    card_rows.push_back(hbox(std::move(current_row)));
                    current_row = {};
                }
            }
        }
    } else {
        std::vector<CardLibraryEntry> library_cards = engine_.allCardLibraryEntries();
        std::sort(library_cards.begin(), library_cards.end(), [&](const CardLibraryEntry& lhs, const CardLibraryEntry& rhs) {
            const std::string left_name = TranslateGameText(language_, lhs.card.def.name);
            const std::string right_name = TranslateGameText(language_, rhs.card.def.name);
            if (left_name != right_name) {
                return left_name < right_name;
            }
            return lhs.card.ref < rhs.card.ref;
        });

        const size_t visible_cards = library_cards.size();
        const int cards_per_row = CardViewerCardsPerRow(dialog_width, visible_cards);
        const int card_width = CardViewerCardWidth(dialog_width, cards_per_row);
        title = Tr(language_, "Card Library", "卡牌图鉴");
        summary = Tr(language_, "Archive progress: ", "图鉴进度：") +
                  std::to_string(engine_.obtainedCardCount()) + "/" + std::to_string(engine_.totalCardCount()) +
                  Tr(language_, " cards obtained.", " 张卡牌已获得。");

        if (library_cards.empty()) {
            card_rows.push_back(text(Tr(language_, "No cards are loaded from the card database.", "当前卡牌数据库中没有可显示的卡牌。")) | color(kDim));
        } else {
            for (const CardLibraryEntry& entry : library_cards) {
                current_row.push_back(CardViewerEntryPanel(language_, entry.card, 1, false, entry.obtained, card_width));
                if (static_cast<int>(current_row.size()) == cards_per_row) {
                    card_rows.push_back(hbox(std::move(current_row)));
                    current_row = {};
                }
            }
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

    Element body = vbox({
        text(summary) | color(Color::White),
        text(cardViewerMode_ == CardViewerMode::OwnedCards
                 ? Tr(language_, "Press D or click Close to return to the run.", "按 D 或点击关闭以返回当前航程。")
                 : Tr(language_, "Press C or click Close to return to the main menu.", "按 C 或点击关闭以返回主菜单。")) |
            color(kGold),
        separator(),
        vbox(std::move(body_cards)),
        separator(),
        text(ScrollHintLine(language_)) | color(kDim) | center,
        separator(),
        cardViewerCloseButton_->Render(),
    });

    return window(text(title) | color(kGold) | bold,
                  ApplyVerticalScrollFrame(body, cardViewerScrollLine_) | size(HEIGHT, LESS_THAN, dialog_height)) |
           size(WIDTH, LESS_THAN, dialog_width) |
           bgcolor(kPanelAlt) |
           center;
}
