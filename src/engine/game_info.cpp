#include "engine/game_info.h"

#include <fstream>

namespace {

std::string DifficultyLabel(int difficulty) {
    if (difficulty == 0) {
        return "Easy";
    }
    if (difficulty == 2) {
        return "Hard";
    }
    return "Normal";
}

}  // namespace

void GameEngine::showHelp() {
    fillHelpInfo();
    state_.page = AppPage::Info;
}

void GameEngine::showInfo(const std::string& title,
                          const std::vector<std::string>& lines,
                          const std::string& footer) {
    state_.info = {};
    state_.info.title = title;
    state_.info.lines = lines;
    state_.info.footer = footer;
    state_.page = AppPage::Info;
}

void GameEngine::showLeaderboard(const std::string& path) {
    state_.info = {};
    state_.info.title = "Run Ledger";
    state_.info.footer = "Press Esc or click Back to return.";
    std::ifstream input(path.c_str());
    if (!input) {
        state_.info.lines.push_back("No recorded runs yet. Win or lose a few voyages first.");
    } else {
        std::string entry;
        while (std::getline(input, entry)) {
            if (!entry.empty()) {
                state_.info.lines.push_back(entry);
            }
        }
        if (state_.info.lines.empty()) {
            state_.info.lines.push_back("Ledger is empty.");
        }
    }
    state_.page = AppPage::Info;
}

std::string GameEngine::difficultyName() const {
    return DifficultyLabel(state_.difficulty);
}

std::string GameEngine::shipNameForId(const std::string& shipId) const {
    const ShipTemplate* ship = shipTemplateById(shipId);
    return ship != nullptr ? ship->name : ships_.front().name;
}

std::string GameEngine::relicNameForId(const std::string& relicId) const {
    const RelicDef* relic = relicById(relicId);
    return relic != nullptr ? relic->name : relicId;
}

std::string GameEngine::relicDescriptionForId(const std::string& relicId) const {
    const RelicDef* relic = relicById(relicId);
    return relic != nullptr ? relic->description : "";
}

void GameEngine::fillHelpInfo() {
    state_.info = {};
    state_.info.title = "Flight Manual";
    state_.info.footer = "Keyboard: 1-9 and 0 choose cards, E ends turn, S saves, L loads, [/] or PgUp/PgDn scroll, Esc returns.";
    state_.info.lines = {
        "Star Abyss Salvager now runs through the FTXUI gameplay flow instead of the old placeholder help screen.",
        "Map flow: choose one node per jump, clear three jumps, then defeat a sector boss.",
        "Battle flow: start each turn with up to 5 cards, spend energy to play modules, then end turn with E.",
        "Cards can draw extra modules, but your hand caps at 10 cards just like a deckbuilder battle lane.",
        "Shield is temporary: it protects you through the enemy turn, then collapses at the start of your next turn.",
        "Heat is dangerous: overheat damages your hull and can reduce your next draw to 4 cards.",
        "Mouse clicks are enabled on compatible terminals, but some terminals do not forward mouse wheel events.",
        "If scrolling does not work in your terminal, use [ and ] or PgUp/PgDn instead.",
        "Save files now keep deck piles, relics, route state, enemy state, rewards, and encounter choices.",
    };
}

void GameEngine::fillSummary(bool victory) {
    state_.hasRun = false;
    state_.info = {};
    state_.info.title = victory ? "Run Complete" : "Ship Lost";
    state_.info.footer = "Choose a next step below.";
    state_.info.lines = {
        std::string("Result: ") + (victory ? "Victory" : "Defeat"),
        "Ship: " + state_.player.shipName,
        "Difficulty: " + difficultyName(),
        "Final sector: " + std::to_string(state_.sector),
        "Credits banked: " + std::to_string(state_.player.scrap),
        "Relics recovered: " + std::to_string(state_.player.relics.size()),
    };
    state_.page = AppPage::Summary;
    resumePage_ = AppPage::Summary;

    std::ofstream output("data/leaderboard.txt", std::ios::app);
    if (output) {
        output << (victory ? "Victory" : "Defeat")
               << " | Ship " << state_.player.shipName
               << " | Difficulty " << difficultyName()
               << " | Sector " << state_.sector
               << " | Credits " << state_.player.scrap << "\n";
    }
}
