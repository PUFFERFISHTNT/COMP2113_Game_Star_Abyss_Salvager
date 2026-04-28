#include "engine/game_card_json.h"

#include <cctype>
#include <filesystem>
#include <functional>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#elif defined(__linux__)
#include <unistd.h>
#elif defined(_WIN32)
#include <windows.h>
#endif

namespace {

// what it does: Resolves the current executable path for locating data files.
// what the inputs are: None.
// what the outputs are: Returns the executable path, or empty path if unavailable (std::filesystem::path).
std::filesystem::path ExecutablePath() {
#if defined(__APPLE__)
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    std::vector<char> buffer(size + 1, '\0');
    if (_NSGetExecutablePath(buffer.data(), &size) == 0) {
        return std::filesystem::weakly_canonical(std::filesystem::path(buffer.data()));
    }
#elif defined(__linux__)
    std::vector<char> buffer(4096, '\0');
    const ssize_t size = ::readlink("/proc/self/exe", buffer.data(), buffer.size() - 1);
    if (size > 0) {
        buffer[static_cast<size_t>(size)] = '\0';
        return std::filesystem::weakly_canonical(std::filesystem::path(buffer.data()));
    }
#elif defined(_WIN32)
    std::vector<char> buffer(MAX_PATH, '\0');
    const DWORD size = GetModuleFileNameA(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (size > 0 && size < buffer.size()) {
        buffer[static_cast<size_t>(size)] = '\0';
        return std::filesystem::weakly_canonical(std::filesystem::path(buffer.data()));
    }
#endif
    return {};
}

struct JsonValue {
    enum class Type {
        Null,
        Bool,
        Number,
        String,
        Object,
        Array,
    };

    Type type = Type::Null;
    bool bool_value = false;
    double number_value = 0.0;
    std::string string_value;
    std::map<std::string, JsonValue> object_value;
    std::vector<JsonValue> array_value;
};

class JsonParser {
public:
    // what it does: Creates a parser for a JSON source string.
    // what the inputs are: source - JSON text to parse.
    // what the outputs are: Constructs a JsonParser instance.
    explicit JsonParser(const std::string& source) : source_(source) {}

    // what it does: Parses the complete JSON source into a root value.
    // what the inputs are: value - parsed root output; error - optional error message output.
    // what the outputs are: Returns true when parsing succeeds; otherwise false (bool).
    bool Parse(JsonValue* value, std::string* error) {
        SkipWhitespace();
        if (!ParseValue(value, error)) {
            return false;
        }
        SkipWhitespace();
        if (!IsDone()) {
            if (error != nullptr) {
                *error = "Unexpected trailing JSON content.";
            }
            return false;
        }
        return true;
    }

private:
    // what it does: Parses a single JSON value at the current position.
    // what the inputs are: value - parsed value output; error - optional error message output.
    // what the outputs are: Returns true when parsing succeeds; otherwise false (bool).
    bool ParseValue(JsonValue* value, std::string* error) {
        if (IsDone()) {
            if (error != nullptr) {
                *error = "Unexpected end of JSON input.";
            }
            return false;
        }

        const char ch = source_[position_];
        if (ch == '"') {
            value->type = JsonValue::Type::String;
            return ParseString(&value->string_value, error);
        }
        if (ch == '{') {
            value->type = JsonValue::Type::Object;
            return ParseObject(&value->object_value, error);
        }
        if (ch == '[') {
            value->type = JsonValue::Type::Array;
            return ParseArray(&value->array_value, error);
        }
        if (ch == 't' || ch == 'f') {
            value->type = JsonValue::Type::Bool;
            return ParseBool(&value->bool_value, error);
        }
        if (ch == 'n') {
            value->type = JsonValue::Type::Null;
            return ParseNull(error);
        }
        if (ch == '-' || std::isdigit(static_cast<unsigned char>(ch)) != 0) {
            value->type = JsonValue::Type::Number;
            return ParseNumber(&value->number_value, error);
        }

        if (error != nullptr) {
            *error = std::string("Unexpected JSON token: ") + ch;
        }
        return false;
    }

    // what it does: Parses a JSON object into key/value fields.
    // what the inputs are: object - parsed object output; error - optional error message output.
    // what the outputs are: Returns true when parsing succeeds; otherwise false (bool).
    bool ParseObject(std::map<std::string, JsonValue>* object, std::string* error) {
        ++position_;  // consume {
        SkipWhitespace();
        if (!IsDone() && source_[position_] == '}') {
            ++position_;
            return true;
        }

        while (!IsDone()) {
            std::string key;
            if (!ParseString(&key, error)) {
                return false;
            }
            SkipWhitespace();
            if (IsDone() || source_[position_] != ':') {
                if (error != nullptr) {
                    *error = "Expected ':' after object key.";
                }
                return false;
            }
            ++position_;
            SkipWhitespace();

            JsonValue value;
            if (!ParseValue(&value, error)) {
                return false;
            }
            (*object)[key] = value;

            SkipWhitespace();
            if (!IsDone() && source_[position_] == '}') {
                ++position_;
                return true;
            }
            if (IsDone() || source_[position_] != ',') {
                if (error != nullptr) {
                    *error = "Expected ',' or '}' in object.";
                }
                return false;
            }
            ++position_;
            SkipWhitespace();
        }

        if (error != nullptr) {
            *error = "Unterminated JSON object.";
        }
        return false;
    }

    // what it does: Parses a JSON array into ordered values.
    // what the inputs are: array - parsed array output; error - optional error message output.
    // what the outputs are: Returns true when parsing succeeds; otherwise false (bool).
    bool ParseArray(std::vector<JsonValue>* array, std::string* error) {
        ++position_;  // consume [
        SkipWhitespace();
        if (!IsDone() && source_[position_] == ']') {
            ++position_;
            return true;
        }

        while (!IsDone()) {
            JsonValue value;
            if (!ParseValue(&value, error)) {
                return false;
            }
            array->push_back(value);

            SkipWhitespace();
            if (!IsDone() && source_[position_] == ']') {
                ++position_;
                return true;
            }
            if (IsDone() || source_[position_] != ',') {
                if (error != nullptr) {
                    *error = "Expected ',' or ']' in array.";
                }
                return false;
            }
            ++position_;
            SkipWhitespace();
        }

        if (error != nullptr) {
            *error = "Unterminated JSON array.";
        }
        return false;
    }

    // what it does: Parses a JSON string and supported escape sequences.
    // what the inputs are: value - parsed string output; error - optional error message output.
    // what the outputs are: Returns true when parsing succeeds; otherwise false (bool).
    bool ParseString(std::string* value, std::string* error) {
        if (IsDone() || source_[position_] != '"') {
            if (error != nullptr) {
                *error = "Expected JSON string.";
            }
            return false;
        }

        ++position_;  // consume opening quote
        std::string result;
        while (!IsDone()) {
            const char ch = source_[position_++];
            if (ch == '"') {
                *value = result;
                return true;
            }
            if (ch == '\\') {
                if (IsDone()) {
                    if (error != nullptr) {
                        *error = "Invalid escape sequence in JSON string.";
                    }
                    return false;
                }
                const char escaped = source_[position_++];
                switch (escaped) {
                case '"': result.push_back('"'); break;
                case '\\': result.push_back('\\'); break;
                case '/': result.push_back('/'); break;
                case 'b': result.push_back('\b'); break;
                case 'f': result.push_back('\f'); break;
                case 'n': result.push_back('\n'); break;
                case 'r': result.push_back('\r'); break;
                case 't': result.push_back('\t'); break;
                default:
                    if (error != nullptr) {
                        *error = "Unsupported JSON escape sequence.";
                    }
                    return false;
                }
            } else {
                result.push_back(ch);
            }
        }

        if (error != nullptr) {
            *error = "Unterminated JSON string.";
        }
        return false;
    }

    // what it does: Parses a JSON boolean literal.
    // what the inputs are: value - parsed boolean output; error - optional error message output.
    // what the outputs are: Returns true when parsing succeeds; otherwise false (bool).
    bool ParseBool(bool* value, std::string* error) {
        if (source_.compare(position_, 4, "true") == 0) {
            *value = true;
            position_ += 4;
            return true;
        }
        if (source_.compare(position_, 5, "false") == 0) {
            *value = false;
            position_ += 5;
            return true;
        }
        if (error != nullptr) {
            *error = "Invalid JSON boolean.";
        }
        return false;
    }

    // what it does: Parses a JSON null literal.
    // what the inputs are: error - optional error message output.
    // what the outputs are: Returns true when parsing succeeds; otherwise false (bool).
    bool ParseNull(std::string* error) {
        if (source_.compare(position_, 4, "null") == 0) {
            position_ += 4;
            return true;
        }
        if (error != nullptr) {
            *error = "Invalid JSON null.";
        }
        return false;
    }

    // what it does: Parses a JSON number into a double value.
    // what the inputs are: value - parsed number output; error - optional error message output.
    // what the outputs are: Returns true when parsing succeeds; otherwise false (bool).
    bool ParseNumber(double* value, std::string* error) {
        const size_t start = position_;
        if (source_[position_] == '-') {
            ++position_;
        }
        while (!IsDone() && std::isdigit(static_cast<unsigned char>(source_[position_])) != 0) {
            ++position_;
        }
        if (!IsDone() && source_[position_] == '.') {
            ++position_;
            while (!IsDone() && std::isdigit(static_cast<unsigned char>(source_[position_])) != 0) {
                ++position_;
            }
        }
        if (!IsDone() && (source_[position_] == 'e' || source_[position_] == 'E')) {
            ++position_;
            if (!IsDone() && (source_[position_] == '+' || source_[position_] == '-')) {
                ++position_;
            }
            while (!IsDone() && std::isdigit(static_cast<unsigned char>(source_[position_])) != 0) {
                ++position_;
            }
        }

        try {
            *value = std::stod(source_.substr(start, position_ - start));
            return true;
        } catch (...) {
            if (error != nullptr) {
                *error = "Invalid JSON number.";
            }
            return false;
        }
    }

    // what it does: Advances past whitespace characters in the JSON source.
    // what the inputs are: None.
    // what the outputs are: No return value (void).
    void SkipWhitespace() {
        while (!IsDone() && std::isspace(static_cast<unsigned char>(source_[position_])) != 0) {
            ++position_;
        }
    }

    // what it does: Checks whether the parser has consumed the full source.
    // what the inputs are: None.
    // what the outputs are: Returns true at end of source; otherwise false (bool).
    bool IsDone() const {
        return position_ >= source_.size();
    }

    const std::string& source_;
    size_t position_ = 0;
};

// what it does: Reads a text file into memory.
// what the inputs are: path - file path to read; content - file content output; error - optional error message output.
// what the outputs are: Returns true when reading succeeds; otherwise false (bool).
bool ReadFile(const std::string& path, std::string* content, std::string* error) {
    std::ifstream input(path.c_str(), std::ios::binary);
    if (!input) {
        if (error != nullptr) {
            *error = "Unable to open JSON file.";
        }
        return false;
    }
    std::ostringstream buffer;
    buffer << input.rdbuf();
    *content = buffer.str();
    return true;
}

// what it does: Looks up a field in a JSON object map.
// what the inputs are: object - object fields to search; key - field name to find.
// what the outputs are: Returns the field value pointer, or nullptr if missing (const JsonValue*).
const JsonValue* FindObjectField(const std::map<std::string, JsonValue>& object, const std::string& key) {
    const auto found = object.find(key);
    return found == object.end() ? nullptr : &found->second;
}

// what it does: Reads a string field from a JSON object with fallback.
// what the inputs are: object - object fields to search; key - field name; fallback - value used when absent or wrong type.
// what the outputs are: Returns the field string or fallback value (std::string).
std::string GetStringField(const std::map<std::string, JsonValue>& object,
                           const std::string& key,
                           const std::string& fallback = "") {
    const JsonValue* value = FindObjectField(object, key);
    if (value == nullptr || value->type != JsonValue::Type::String) {
        return fallback;
    }
    return value->string_value;
}

// what it does: Reads an integer field from a JSON object with fallback.
// what the inputs are: object - object fields to search; key - field name; fallback - value used when absent or wrong type.
// what the outputs are: Returns the field integer or fallback value (int).
int GetIntField(const std::map<std::string, JsonValue>& object,
                const std::string& key,
                int fallback = 0) {
    const JsonValue* value = FindObjectField(object, key);
    if (value == nullptr || value->type != JsonValue::Type::Number) {
        return fallback;
    }
    return static_cast<int>(value->number_value);
}

// what it does: Reads a boolean field from a JSON object with fallback.
// what the inputs are: object - object fields to search; key - field name; fallback - value used when absent or wrong type.
// what the outputs are: Returns the field boolean or fallback value (bool).
bool GetBoolField(const std::map<std::string, JsonValue>& object,
                  const std::string& key,
                  bool fallback = false) {
    const JsonValue* value = FindObjectField(object, key);
    if (value == nullptr || value->type != JsonValue::Type::Bool) {
        return fallback;
    }
    return value->bool_value;
}

// what it does: Reads an object field from a JSON object.
// what the inputs are: object - object fields to search; key - field name.
// what the outputs are: Returns nested object pointer, or nullptr if absent or wrong type (const std::map<std::string, JsonValue>*).
const std::map<std::string, JsonValue>* GetObjectField(
    const std::map<std::string, JsonValue>& object, const std::string& key) {
    const JsonValue* value = FindObjectField(object, key);
    if (value == nullptr || value->type != JsonValue::Type::Object) {
        return nullptr;
    }
    return &value->object_value;
}

// what it does: Converts a rarity token into a CardRarity value.
// what the inputs are: rarity - rarity token from JSON.
// what the outputs are: Returns the parsed card rarity (CardRarity).
CardRarity ParseRarity(const std::string& rarity) {
    if (rarity == "uncommon" || rarity == "blue") {
        return CardRarity::Uncommon;
    }
    if (rarity == "rare" || rarity == "gold") {
        return CardRarity::Rare;
    }
    return CardRarity::Common;
}

// what it does: Converts a CardRarity value into its JSON token.
// what the inputs are: rarity - card rarity value.
// what the outputs are: Returns the rarity token (std::string).
std::string RarityToken(CardRarity rarity) {
    switch (rarity) {
    case CardRarity::Common:
        return "common";
    case CardRarity::Uncommon:
        return "uncommon";
    case CardRarity::Rare:
        return "rare";
    }
    return "common";
}

// what it does: Parses card definitions and optional upgrades from a JSON root.
// what the inputs are: root - parsed JSON root; upgraded_cards - whether entries are legacy upgrades; cards - card output; upgrades - upgrade output; error - optional error message output.
// what the outputs are: Returns true when parsing succeeds; otherwise false (bool).
bool ParseCardList(const JsonValue& root,
                   bool upgraded_cards,
                   std::vector<CardDef>* cards,
                   std::vector<CardDef>* upgrades,
                   std::string* error) {
    if (root.type != JsonValue::Type::Object) {
        if (error != nullptr) {
            *error = "Card JSON root must be an object.";
        }
        return false;
    }
    const JsonValue* cards_value = FindObjectField(root.object_value, "cards");
    if (cards_value == nullptr || cards_value->type != JsonValue::Type::Array) {
        if (error != nullptr) {
            *error = "Card JSON must contain a cards array.";
        }
        return false;
    }

    cards->clear();
    if (upgrades != nullptr) {
        upgrades->clear();
    }
    std::function<bool(const std::map<std::string, JsonValue>&, const CardDef*, bool, CardDef*)> parse_card;
    parse_card = [&](const std::map<std::string, JsonValue>& object,
                     const CardDef* fallback,
                     bool allow_upgrade_block,
                     CardDef* out_card) -> bool {
        CardDef card = fallback != nullptr ? *fallback : CardDef{};
        card.id = upgraded_cards
                      ? GetStringField(object, "base_id", GetStringField(object, "id", card.id))
                      : GetStringField(object, "id", card.id);
        card.name = GetStringField(object, "name", card.name);
        card.nameEn = GetStringField(object, "name_en", card.nameEn);
        card.type = GetStringField(object, "type", card.type);
        card.description = GetStringField(object, "description", card.description);
        card.descriptionEn = GetStringField(object, "description_en", card.descriptionEn);
        card.rarity = ParseRarity(GetStringField(object, "rarity", RarityToken(card.rarity)));
        card.starterCopies = GetIntField(object, "starterCopies", card.starterCopies);
        card.cost = GetIntField(object, "cost", card.cost);
        card.damage = GetIntField(object, "damage", card.damage);
        card.shield = GetIntField(object, "shield", card.shield);
        card.heal = GetIntField(object, "heal", card.heal);
        card.draw = GetIntField(object, "draw", card.draw);
        card.energy = GetIntField(object, "energy", card.energy);
        card.nextTurnEnergy = GetIntField(object, "nextTurnEnergy", card.nextTurnEnergy);
        card.heat = GetIntField(object, "heat", card.heat);
        card.enemyHeat = GetIntField(object, "enemyHeat", card.enemyHeat);
        card.selfDamage = GetIntField(object, "selfDamage", card.selfDamage);
        card.multiHit = GetIntField(object, "multiHit", card.multiHit == 0 ? 1 : card.multiHit);
        card.comboBonus = GetIntField(object, "comboBonus", card.comboBonus);
        card.attackMultiplier = GetIntField(object, "attackMultiplier", card.attackMultiplier == 0 ? 1 : card.attackMultiplier);
        card.weakenTurns = GetIntField(object, "weakenTurns", card.weakenTurns);
        card.removeEnemyShield = GetIntField(object, "removeEnemyShield", card.removeEnemyShield);
        card.shieldLockTurns = GetIntField(object, "shieldLockTurns", card.shieldLockTurns);
        card.salvageBonus = GetIntField(object, "salvageBonus", card.salvageBonus);
        card.summonDroneCount = GetIntField(object, "summonDroneCount", card.summonDroneCount);
        card.summonDroneAttack = GetIntField(object, "summonDroneAttack", card.summonDroneAttack);
        card.summonDroneShield = GetIntField(object, "summonDroneShield", card.summonDroneShield);
        card.summonDroneDuration = GetIntField(object, "summonDroneDuration", card.summonDroneDuration);
        card.buffDroneAttack = GetIntField(object, "buffDroneAttack", card.buffDroneAttack);
        card.buffDroneShield = GetIntField(object, "buffDroneShield", card.buffDroneShield);
        card.auxImmediateTriggerTimes = GetIntField(object, "auxImmediateTriggerTimes", card.auxImmediateTriggerTimes);
        card.auxEndTurnExtraTriggerTimes = GetIntField(object, "auxEndTurnExtraTriggerTimes", card.auxEndTurnExtraTriggerTimes);
        card.auxEffectPercentBonus = GetIntField(object, "auxEffectPercentBonus", card.auxEffectPercentBonus);
        card.auxSlotBonus = GetIntField(object, "auxSlotBonus", card.auxSlotBonus);
        card.auxDestroyCount = GetIntField(object, "auxDestroyCount", card.auxDestroyCount);
        card.auxDestroyRewardMultiplierPercent =
            GetIntField(object, "auxDestroyRewardMultiplierPercent", card.auxDestroyRewardMultiplierPercent);
        card.firstSummonExtraCopies = GetIntField(object, "firstSummonExtraCopies", card.firstSummonExtraCopies);
        card.firstSummonExtraScope = GetIntField(object, "firstSummonExtraScope", card.firstSummonExtraScope);
        card.summonCardPlayedDrawThisTurn =
            GetIntField(object, "summonCardPlayedDrawThisTurn", card.summonCardPlayedDrawThisTurn);
        card.formChangesThisTurnThreshold = GetIntField(object, "formChangesThisTurnThreshold", card.formChangesThisTurnThreshold);
        card.formChangesThisTurnBonusDamage =
            GetIntField(object, "formChangesThisTurnBonusDamage", card.formChangesThisTurnBonusDamage);
        card.firstEnterHighHeatEnergyThisBattle =
            GetIntField(object, "firstEnterHighHeatEnergyThisBattle", card.firstEnterHighHeatEnergyThisBattle);
        card.firstEnterHighHeatDrawThisBattle =
            GetIntField(object, "firstEnterHighHeatDrawThisBattle", card.firstEnterHighHeatDrawThisBattle);
        card.eachEnterHighHeatDrawThisBattle =
            GetIntField(object, "eachEnterHighHeatDrawThisBattle", card.eachEnterHighHeatDrawThisBattle);
        card.firstSummonEnemyHeatPerTurnThisBattle =
            GetIntField(object, "firstSummonEnemyHeatPerTurnThisBattle", card.firstSummonEnemyHeatPerTurnThisBattle);
        card.firstSummonEnemyHeatAttackBonusPerTurnThisBattle = GetIntField(
            object,
            "firstSummonEnemyHeatAttackBonusPerTurnThisBattle",
            card.firstSummonEnemyHeatAttackBonusPerTurnThisBattle);
        card.enterOverloadEnemyHeatThisBattle =
            GetIntField(object, "enterOverloadEnemyHeatThisBattle", card.enterOverloadEnemyHeatThisBattle);
        card.leaveDefenseEnemyHeatThisBattle =
            GetIntField(object, "leaveDefenseEnemyHeatThisBattle", card.leaveDefenseEnemyHeatThisBattle);
        card.formShift = GetIntField(object, "formShift", card.formShift);
        card.overloadIncomingDamageReducePercent =
            GetIntField(object, "overloadIncomingDamageReducePercent", card.overloadIncomingDamageReducePercent);
        card.blackboxCopyCount = GetIntField(object, "blackboxCopyCount", card.blackboxCopyCount);
        card.leaveDefenseEnergyBonus = GetIntField(object, "leaveDefenseEnergyBonus", card.leaveDefenseEnergyBonus);
        card.leaveDefenseShieldBonus = GetIntField(object, "leaveDefenseShieldBonus", card.leaveDefenseShieldBonus);
        card.blackboxProtocol = GetBoolField(object, "blackboxProtocol", card.blackboxProtocol);
        card.formToggle = GetBoolField(object, "formToggle", card.formToggle);
        card.formExit = GetBoolField(object, "formExit", card.formExit);
        card.ignoreShield = GetBoolField(object, "ignoreShield", card.ignoreShield);
        card.shieldToDamage = GetBoolField(object, "shieldToDamage", card.shieldToDamage);
        card.halveHeat = GetBoolField(object, "halveHeat", card.halveHeat);
        card.clearHeat = GetBoolField(object, "clearHeat", card.clearHeat);
        card.randomEffect = GetBoolField(object, "randomEffect", card.randomEffect);
        if (card.id.empty() || card.name.empty()) {
            if (error != nullptr) {
                *error = "Card JSON entry is missing id or name.";
            }
            return false;
        }
        if (card.nameEn.empty()) {
            card.nameEn = card.id;
        }
        if (card.descriptionEn.empty()) {
            card.descriptionEn = card.description;
        }
        *out_card = card;
        if (allow_upgrade_block && upgrades != nullptr) {
            const auto* upgrade_object = GetObjectField(object, "upgrade");
            if (upgrade_object != nullptr) {
                CardDef upgraded_card;
                if (!parse_card(*upgrade_object, &card, false, &upgraded_card)) {
                    return false;
                }
                upgraded_card.id = card.id;
                if (upgraded_card.name.empty()) {
                    upgraded_card.name = card.name + "+++";
                }
                if (upgraded_card.nameEn.empty()) {
                    upgraded_card.nameEn = card.nameEn + "+++";
                }
                upgrades->push_back(upgraded_card);
            }
        }
        return true;
    };

    for (const JsonValue& entry : cards_value->array_value) {
        if (entry.type != JsonValue::Type::Object) {
            if (error != nullptr) {
                *error = "Each card entry must be a JSON object.";
            }
            return false;
        }

        CardDef card;
        if (!parse_card(entry.object_value, nullptr, !upgraded_cards, &card)) {
            return false;
        }
        cards->push_back(card);
    }
    return true;
}

}  // namespace

// what it does: Resolves the best available path to the cards JSON data file.
// what the inputs are: None.
// what the outputs are: Returns the selected cards JSON path (std::string).
std::string ResolveCardsJsonPath() {
    namespace fs = std::filesystem;

    const std::vector<fs::path> candidates = {
        fs::path("data") / "cards.json",
        ExecutablePath().empty() ? fs::path() : (ExecutablePath().parent_path() / "data" / "cards.json"),
        ExecutablePath().empty() ? fs::path() : (ExecutablePath().parent_path().parent_path() / "data" / "cards.json"),
    };

    for (const fs::path& candidate : candidates) {
        if (!candidate.empty() && fs::exists(candidate)) {
            return candidate.string();
        }
    }

    return (fs::path("data") / "cards.json").string();
}

// what it does: Loads card definitions and upgrades from a JSON file.
// what the inputs are: path - JSON file path; cards - card output; upgrades - upgrade output; error - optional error message output.
// what the outputs are: Returns true when loading succeeds; otherwise false (bool).
bool LoadCardsFromJsonFile(const std::string& path,
                           std::vector<CardDef>* cards,
                           std::vector<CardDef>* upgrades,
                           std::string* error) {
    std::string content;
    if (!ReadFile(path, &content, error)) {
        return false;
    }
    JsonValue root;
    JsonParser parser(content);
    if (!parser.Parse(&root, error)) {
        return false;
    }
    return ParseCardList(root, false, cards, upgrades, error);
}
