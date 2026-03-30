#include "ui/ui_art.h"

#include <algorithm>

#include <ftxui/screen/terminal.hpp>

using namespace ftxui;

namespace {

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

}  // namespace

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
            " \\_____\\____####____/_____/ ",
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
