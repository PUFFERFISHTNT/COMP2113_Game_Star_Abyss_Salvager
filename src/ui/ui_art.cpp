#include "ui/ui_art.h"

#include <algorithm>
#include <string>
#include <unordered_map>

#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>
#include <ftxui/screen/terminal.hpp>

using namespace ftxui;

namespace {

struct GlyphStyle {
    std::string glyph;
    Color color = Color::White;
    bool emphasized = false;
};

using Palette = std::unordered_map<char, GlyphStyle>;

// what it does: Repeats a text pattern until it fills a target width.
// what the inputs are: pattern - source pattern string; width - desired output width.
// what the outputs are: Returns the repeated and trimmed text (std::string).
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

// what it does: Calculates the visible width of an FTXUI box.
// what the inputs are: box - measured FTXUI box.
// what the outputs are: Returns the box width in cells, or zero for invalid boxes (int).
int BoxWidth(const Box& box) {
    if (box.x_max < box.x_min || box.y_max < box.y_min) {
        return 0;
    }
    return box.x_max - box.x_min + 1;
}

// what it does: Repeats a glyph string a fixed number of times.
// what the inputs are: glyph - glyph text to repeat; count - repetition count.
// what the outputs are: Returns the repeated glyph text (std::string).
std::string RepeatGlyph(const std::string& glyph, int count) {
    std::string out;
    out.reserve(glyph.size() * static_cast<size_t>(count));
    for (int i = 0; i < count; ++i) {
        out += glyph;
    }
    return out;
}

// what it does: Renders one encoded sprite row using a glyph palette.
// what the inputs are: row - encoded sprite row; palette - token-to-style map.
// what the outputs are: Returns the rendered row element (ftxui::Element).
Element RenderSpriteRow(const std::string& row, const Palette& palette) {
    Elements segments;
    size_t cursor = 0;
    while (cursor < row.size()) {
        const char token = row[cursor];
        size_t end = cursor + 1;
        while (end < row.size() && row[end] == token) {
            ++end;
        }

        const int count = static_cast<int>(end - cursor);
        if (token == ' ') {
            segments.push_back(text(std::string(static_cast<size_t>(count), ' ')));
        } else {
            const auto found = palette.find(token);
            const GlyphStyle style = found != palette.end()
                                         ? found->second
                                         : GlyphStyle{"█", Color::White, false};
            Element segment = text(RepeatGlyph(style.glyph, count)) | color(style.color);
            if (style.emphasized) {
                segment = segment | bold;
            }
            segments.push_back(segment);
        }
        cursor = end;
    }
    return hbox(std::move(segments));
}

// what it does: Renders encoded sprite rows into FTXUI elements.
// what the inputs are: rows - encoded sprite rows; palette - token-to-style map.
// what the outputs are: Returns rendered sprite rows (ftxui::Elements).
Elements RenderSprite(const std::vector<std::string>& rows, const Palette& palette) {
    Elements rendered;
    rendered.reserve(rows.size());
    for (const std::string& row : rows) {
        rendered.push_back(RenderSpriteRow(row, palette));
    }
    return rendered;
}

// what it does: Builds the animated color palette for the Vanguard sprite.
// what the inputs are: tick - animation frame counter.
// what the outputs are: Returns the sprite palette (Palette).
Palette VanguardPalette(int tick) {
    const Color engine = tick % 2 == 0 ? Color::RGB(255, 156, 71) : Color::RGB(255, 214, 92);
    return {
        {'a', {"█", Color::RGB(141, 228, 255), true}},
        {'b', {"█", Color::RGB(70, 132, 209), false}},
        {'c', {"█", Color::RGB(247, 208, 96), true}},
        {'e', {"█", engine, true}},
    };
}

// what it does: Builds the animated color palette for the Drift Fox sprite.
// what the inputs are: tick - animation frame counter.
// what the outputs are: Returns the sprite palette (Palette).
Palette DriftFoxPalette(int tick) {
    const Color engine = tick % 2 == 0 ? Color::RGB(255, 146, 78) : Color::RGB(255, 196, 97);
    return {
        {'a', {"█", Color::RGB(168, 255, 241), true}},
        {'b', {"█", Color::RGB(66, 190, 210), false}},
        {'c', {"█", Color::RGB(255, 179, 94), true}},
        {'e', {"█", engine, true}},
    };
}

// what it does: Builds the animated color palette for the Iron Bastion sprite.
// what the inputs are: tick - animation frame counter.
// what the outputs are: Returns the sprite palette (Palette).
Palette IronBastionPalette(int tick) {
    const Color engine = tick % 2 == 0 ? Color::RGB(255, 170, 99) : Color::RGB(255, 224, 120);
    return {
        {'a', {"█", Color::RGB(186, 206, 229), true}},
        {'b', {"█", Color::RGB(95, 123, 166), false}},
        {'c', {"█", Color::RGB(255, 214, 120), true}},
        {'e', {"█", engine, true}},
    };
}

// what it does: Builds the animated color palette for common enemy sprites.
// what the inputs are: tick - animation frame counter.
// what the outputs are: Returns the sprite palette (Palette).
Palette CommonEnemyPalette(int tick) {
    const Color engine = tick % 2 == 0 ? Color::RGB(255, 101, 98) : Color::RGB(255, 169, 84);
    return {
        {'r', {"█", Color::RGB(230, 98, 109), true}},
        {'m', {"█", Color::RGB(134, 53, 88), false}},
        {'y', {"█", Color::RGB(255, 205, 102), true}},
        {'e', {"█", engine, true}},
    };
}

// what it does: Builds the animated color palette for the Furnace Carrier sprite.
// what the inputs are: tick - animation frame counter.
// what the outputs are: Returns the sprite palette (Palette).
Palette FurnaceCarrierPalette(int tick) {
    const Color engine = tick % 2 == 0 ? Color::RGB(255, 124, 73) : Color::RGB(255, 189, 101);
    return {
        {'r', {"█", Color::RGB(201, 89, 83), true}},
        {'m', {"█", Color::RGB(109, 47, 54), false}},
        {'y', {"█", Color::RGB(255, 186, 67), true}},
        {'e', {"█", engine, true}},
    };
}

// what it does: Builds the animated color palette for the Abyss Core Sentinel sprite.
// what the inputs are: tick - animation frame counter.
// what the outputs are: Returns the sprite palette (Palette).
Palette SentinelPalette(int tick) {
    const Color engine = tick % 2 == 0 ? Color::RGB(163, 229, 136) : Color::RGB(125, 255, 209);
    return {
        {'m', {"█", Color::RGB(118, 77, 156), false}},
        {'g', {"█", Color::RGB(112, 221, 176), true}},
        {'y', {"█", Color::RGB(225, 255, 168), true}},
        {'e', {"█", engine, true}},
    };
}

// what it does: Builds the animated color palette for the Genesis Guardian sprite.
// what the inputs are: tick - animation frame counter.
// what the outputs are: Returns the sprite palette (Palette).
Palette GenesisGuardianPalette(int tick) {
    const Color engine = tick % 2 == 0 ? Color::RGB(159, 241, 255) : Color::RGB(255, 215, 138);
    return {
        {'v', {"█", Color::RGB(108, 82, 178), false}},
        {'c', {"█", Color::RGB(112, 228, 255), true}},
        {'g', {"█", Color::RGB(184, 255, 220), true}},
        {'y', {"█", Color::RGB(255, 231, 163), true}},
        {'e', {"█", engine, true}},
    };
}

// what it does: Provides encoded sprite rows for the Vanguard ship.
// what the inputs are: None.
// what the outputs are: Returns encoded sprite rows (std::vector<std::string>).
std::vector<std::string> VanguardRows() {
    return {
        "          c          ",
        "        ccccc        ",
        "      bbaaaaabb      ",
        "    bbbaaaaaaabbb    ",
        "  bbbaaaaacaaaaabbb  ",
        "    aaabccccccbaaa   ",
        "      aaabbbbaaa     ",
        "        e    e       ",
    };
}

// what it does: Provides encoded sprite rows for the Drift Fox ship.
// what the inputs are: None.
// what the outputs are: Returns encoded sprite rows (std::vector<std::string>).
std::vector<std::string> DriftFoxRows() {
    return {
        "          c          ",
        "        ccccc        ",
        "      bbbaaaabbb     ",
        "    bbbaaaaaaaaabbb  ",
        " bbbbaaaaccccaaaabbbb",
        "    aaaabbbbbbaaaa   ",
        "      aaab  baaa     ",
        "       e e  e e      ",
    };
}

// what it does: Provides encoded sprite rows for the Iron Bastion ship.
// what the inputs are: None.
// what the outputs are: Returns encoded sprite rows (std::vector<std::string>).
std::vector<std::string> IronBastionRows() {
    return {
        "        ccccc        ",
        "      bbcccccbb      ",
        "    bbbaaaaaaaabbb   ",
        "  bbbaaaaaaaaaaaabbb ",
        "  aaaabbbccccbbbaaaa ",
        "  bbbaaaaaaaaaaaabbb ",
        "      aaabbbbaaa     ",
        "       eee eee       ",
    };
}

// what it does: Provides encoded sprite rows for a common enemy ship.
// what the inputs are: None.
// what the outputs are: Returns encoded sprite rows (std::vector<std::string>).
std::vector<std::string> CommonEnemyRows() {
    return {
        "        yyyyy        ",
        "      mmryrrymm      ",
        "    mmrrrrrrrrrmm    ",
        "   mmrrrrmmmrrrrmm   ",
        "    rrrmmyyymmyrrr   ",
        "      mmrrrrrmm      ",
        "        rrerr        ",
    };
}

// what it does: Provides encoded sprite rows for the Furnace Carrier enemy.
// what the inputs are: None.
// what the outputs are: Returns encoded sprite rows (std::vector<std::string>).
std::vector<std::string> FurnaceCarrierRows() {
    return {
        "      yyyyyyyyy      ",
        "   mmrrrrrrrrrrrmm   ",
        "  mmrrrryyyyyrrrrmm  ",
        " mmrrrrrmmmmmrrrrrmm ",
        "  rrrrrmmyyymmrrrrr  ",
        "    mmrrrrrrrrrmm    ",
        "      rrreeerrr      ",
    };
}

// what it does: Provides encoded sprite rows for the Abyss Core Sentinel enemy.
// what the inputs are: None.
// what the outputs are: Returns encoded sprite rows (std::vector<std::string>).
std::vector<std::string> SentinelRows() {
    return {
        "          y          ",
        "       gyyyyyg       ",
        "      mmgggggmm      ",
        "    mmggmmmmmggmm    ",
        "   mmggmmyyymmggmm   ",
        "    mmggmmmmmggmm    ",
        "      mmgggggmm      ",
        "        e  e         ",
    };
}

// what it does: Provides encoded sprite rows for the Creation Core Guardian enemy.
// what the inputs are: None.
// what the outputs are: Returns encoded sprite rows (std::vector<std::string>).
std::vector<std::string> GenesisGuardianRows() {
    return {
        "          yyy          ",
        "       ccyyyycc        ",
        "     vvccggggccvv      ",
        "   vvccggvvvvggccvv    ",
        "  vvccggvvyyvvggccvv   ",
        " vvccggvvyyyyvvggccvv  ",
        "  vvccggvvyyvvggccvv   ",
        "   vvccggvvvvggccvv    ",
        "     vvccggggccvv      ",
        "       cceeveecc       ",
    };
}

}  // namespace

// what it does: Renders the player ship sprite for the current run state.
// what the inputs are: state - current run state; tick - animation frame counter.
// what the outputs are: Returns rendered player art rows (ftxui::Elements).
Elements PlayerArt(const RunState& state, int tick) {
    if (state.player.shipId == "iron_bastion") {
        return RenderSprite(IronBastionRows(), IronBastionPalette(tick));
    }
    if (state.player.shipId == "drift_fox") {
        return RenderSprite(DriftFoxRows(), DriftFoxPalette(tick));
    }
    return RenderSprite(VanguardRows(), VanguardPalette(tick));
}

// what it does: Renders the enemy sprite for the current battle state.
// what the inputs are: state - current run state; tick - animation frame counter.
// what the outputs are: Returns rendered enemy art rows (ftxui::Elements).
Elements EnemyArt(const RunState& state, int tick) {
    if (state.enemy.enemyId == "creation_core_guardian") {
        return RenderSprite(GenesisGuardianRows(), GenesisGuardianPalette(tick));
    }
    if (state.enemy.enemyId == "furnace_carrier") {
        return RenderSprite(FurnaceCarrierRows(), FurnaceCarrierPalette(tick));
    }
    if (state.enemy.enemyId == "abyss_core_sentinel") {
        return RenderSprite(SentinelRows(), SentinelPalette(tick));
    }
    return RenderSprite(CommonEnemyRows(), CommonEnemyPalette(tick));
}

// what it does: Determines a safe width for the combat lane display.
// what the inputs are: lane_box - measured lane box from the previous render.
// what the outputs are: Returns the lane width hint in cells (int).
int LaneWidthHint(const Box& lane_box) {
    const int reflected_width = BoxWidth(lane_box);
    if (reflected_width > 0) {
        return reflected_width;
    }
    const int terminal_width = Terminal::Size().dimx;
    return std::max(24, terminal_width / 3 - 8);
}

// what it does: Builds the animated combat lane rows for the battle screen.
// what the inputs are: state - current run state; tick - animation frame counter; width - requested lane width.
// what the outputs are: Returns combat lane text rows (std::vector<std::string>).
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
        std::string(static_cast<size_t>(width), '.'),
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
