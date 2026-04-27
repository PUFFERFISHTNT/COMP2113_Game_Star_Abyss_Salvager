#include "engine/game_info.h"

#include <fstream>
#include <vector>

namespace {

std::string DifficultyLabel(int difficulty) {
    if (difficulty == 0) {
        return "Easy";
    }
    if (difficulty == 2) {
        return "Hard";
    }
    if (difficulty >= 3) {
        return "Hell";
    }
    return "Normal";
}

const std::vector<std::pair<std::string, std::string>>& EventAchievementCatalog() {
    static const std::vector<std::pair<std::string, std::string>> catalog = {
        {"game_intro", "Star Abyss Salvager"},
        {"event_a1", "Distress Beacon"},
        {"event_a2", "Ghost Ship"},
        {"event_b1", "Strange Signal"},
        {"event_b2", "Space Fold"},
        {"event_b3", "Ark Beacon"},
        {"event_c1", "Reactor Leak"},
        {"event_c2", "Hull Breach"},
        {"event_c3", "Deck Chaos"},
        {"event_d1", "Smuggler Cache"},
        {"event_d2", "Wandering Trader"},
        {"event_d3", "Pirate Ambush"},
        {"event_d4", "Cryo Pod"},
        {"event_story_boss", "Anomalous War Signature"},
        {"escape_prologue", "Escape Decision"},
        {"escape_1", "Federal Patrol Outpost"},
        {"escape_2", "Abandoned Smuggling Port"},
        {"escape_3", "Deep Space Radiation Belt"},
        {"escape_4", "Pirate Pursuit Ship"},
        {"escape_5", "Drifting Escape Pod"},
        {"escape_6", "Unstable Jump Point"},
        {"escape_7", "Star Abyss Edge Barrier"},
        {"escape_8", "Temporary Safe Station"},
        {"truth_1", "Creation Memory Fragment"},
        {"truth_2", "Rogue Guardian AI"},
        {"truth_3", "First Captain's Consciousness"},
        {"truth_4", "Spatial Collapse Point"},
        {"truth_5", "Star Abyss Birth Truth"},
    };
    return catalog;
}

}  // namespace

void GameEngine::showHelp() {
    fillHelpInfo();
    state_.page = AppPage::Info;
}

void GameEngine::showAchievements() {
    state_.info = {};
    state_.info.title = "Completed Events";
    state_.info.footer = "Press Esc or click Back to return.";
    if (completedEventAchievements_.empty()) {
        state_.info.lines.push_back("No completed events recorded yet.");
    } else {
        for (const auto& entry : completedEventAchievements_) {
            state_.info.lines.push_back(entry.second);
        }
    }
    state_.page = AppPage::Info;
}

std::vector<AchievementEntry> GameEngine::allEventAchievementEntries() const {
    std::vector<AchievementEntry> result;
    const auto& catalog = EventAchievementCatalog();
    result.reserve(catalog.size());
    for (const auto& entry : catalog) {
        result.push_back({entry.first,
                          entry.second,
                          "Complete this event at least once.",
                          hasCompletedEventAchievement(entry.first)});
    }
    return result;
}

std::vector<AchievementEntry> GameEngine::allEnemyAchievementEntries() const {
    std::vector<AchievementEntry> result;
    result.reserve(enemyPool_.size());
    for (const EnemyTemplate& enemy : enemyPool_) {
        result.push_back({enemy.id,
                          enemy.name,
                          "Encounter this enemy in battle at least once.",
                          hasSeenEnemy(enemy.id)});
    }
    std::sort(result.begin(), result.end(), [](const AchievementEntry& lhs, const AchievementEntry& rhs) {
        if (lhs.title != rhs.title) {
            return lhs.title < rhs.title;
        }
        return lhs.id < rhs.id;
    });
    return result;
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

size_t GameEngine::completedEventAchievementCount() const {
    return completedEventAchievements_.size();
}

size_t GameEngine::totalEventAchievementCount() const {
    return EventAchievementCatalog().size();
}

size_t GameEngine::seenEnemyCount() const {
    return seenEnemyIds_.size();
}

size_t GameEngine::totalEnemyCount() const {
    return enemyPool_.size();
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
        "Heat is dangerous: overflow feedback damages your hull, and sustained overheat in battle cuts draw, lowers outgoing damage, raises incoming damage, and eventually burns 5 HP each turn.",
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
