#include "engine/game_card_json.h"

#include <cctype>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace {

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
    explicit JsonParser(const std::string& source) : source_(source) {}

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

    void SkipWhitespace() {
        while (!IsDone() && std::isspace(static_cast<unsigned char>(source_[position_])) != 0) {
            ++position_;
        }
    }

    bool IsDone() const {
        return position_ >= source_.size();
    }

    const std::string& source_;
    size_t position_ = 0;
};

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

const JsonValue* FindObjectField(const std::map<std::string, JsonValue>& object, const std::string& key) {
    const auto found = object.find(key);
    return found == object.end() ? nullptr : &found->second;
}

std::string GetStringField(const std::map<std::string, JsonValue>& object,
                           const std::string& key,
                           const std::string& fallback = "") {
    const JsonValue* value = FindObjectField(object, key);
    if (value == nullptr || value->type != JsonValue::Type::String) {
        return fallback;
    }
    return value->string_value;
}

int GetIntField(const std::map<std::string, JsonValue>& object,
                const std::string& key,
                int fallback = 0) {
    const JsonValue* value = FindObjectField(object, key);
    if (value == nullptr || value->type != JsonValue::Type::Number) {
        return fallback;
    }
    return static_cast<int>(value->number_value);
}

bool GetBoolField(const std::map<std::string, JsonValue>& object,
                  const std::string& key,
                  bool fallback = false) {
    const JsonValue* value = FindObjectField(object, key);
    if (value == nullptr || value->type != JsonValue::Type::Bool) {
        return fallback;
    }
    return value->bool_value;
}

CardRarity ParseRarity(const std::string& rarity) {
    if (rarity == "uncommon" || rarity == "blue") {
        return CardRarity::Uncommon;
    }
    if (rarity == "rare" || rarity == "gold") {
        return CardRarity::Rare;
    }
    return CardRarity::Common;
}

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

bool ParseCardList(const JsonValue& root,
                   bool upgraded_cards,
                   std::vector<CardDef>* cards,
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
    for (const JsonValue& entry : cards_value->array_value) {
        if (entry.type != JsonValue::Type::Object) {
            if (error != nullptr) {
                *error = "Each card entry must be a JSON object.";
            }
            return false;
        }

        CardDef card;
        card.id = upgraded_cards
                      ? GetStringField(entry.object_value, "base_id", GetStringField(entry.object_value, "id"))
                      : GetStringField(entry.object_value, "id");
        card.name = GetStringField(entry.object_value, "name");
        card.type = GetStringField(entry.object_value, "type");
        card.description = GetStringField(entry.object_value, "description");
        card.rarity = ParseRarity(GetStringField(entry.object_value, "rarity", "common"));
        card.starterCopies = GetIntField(entry.object_value, "starterCopies", 0);
        card.cost = GetIntField(entry.object_value, "cost", 0);
        card.damage = GetIntField(entry.object_value, "damage", 0);
        card.shield = GetIntField(entry.object_value, "shield", 0);
        card.heal = GetIntField(entry.object_value, "heal", 0);
        card.draw = GetIntField(entry.object_value, "draw", 0);
        card.energy = GetIntField(entry.object_value, "energy", 0);
        card.nextTurnEnergy = GetIntField(entry.object_value, "nextTurnEnergy", 0);
        card.heat = GetIntField(entry.object_value, "heat", 0);
        card.enemyHeat = GetIntField(entry.object_value, "enemyHeat", 0);
        card.selfDamage = GetIntField(entry.object_value, "selfDamage", 0);
        card.multiHit = GetIntField(entry.object_value, "multiHit", 1);
        card.comboBonus = GetIntField(entry.object_value, "comboBonus", 0);
        card.attackMultiplier = GetIntField(entry.object_value, "attackMultiplier", 1);
        card.weakenTurns = GetIntField(entry.object_value, "weakenTurns", 0);
        card.removeEnemyShield = GetIntField(entry.object_value, "removeEnemyShield", 0);
        card.shieldLockTurns = GetIntField(entry.object_value, "shieldLockTurns", 0);
        card.salvageBonus = GetIntField(entry.object_value, "salvageBonus", 0);
        card.ignoreShield = GetBoolField(entry.object_value, "ignoreShield", false);
        card.shieldToDamage = GetBoolField(entry.object_value, "shieldToDamage", false);
        card.halveHeat = GetBoolField(entry.object_value, "halveHeat", false);
        card.clearHeat = GetBoolField(entry.object_value, "clearHeat", false);
        card.randomEffect = GetBoolField(entry.object_value, "randomEffect", false);

        if (card.id.empty() || card.name.empty()) {
            if (error != nullptr) {
                *error = "Card JSON entry is missing id or name.";
            }
            return false;
        }
        cards->push_back(card);
    }
    return true;
}

std::string EscapeJson(const std::string& value) {
    std::string escaped;
    escaped.reserve(value.size());
    for (char ch : value) {
        switch (ch) {
        case '"': escaped += "\\\""; break;
        case '\\': escaped += "\\\\"; break;
        case '\n': escaped += "\\n"; break;
        case '\r': escaped += "\\r"; break;
        case '\t': escaped += "\\t"; break;
        default: escaped.push_back(ch); break;
        }
    }
    return escaped;
}

void WriteBool(std::ostream& out, const char* key, bool value, bool trailing_comma = true) {
    out << "      \"" << key << "\": " << (value ? "true" : "false");
    if (trailing_comma) {
        out << ",";
    }
    out << "\n";
}

void WriteInt(std::ostream& out, const char* key, int value, bool trailing_comma = true) {
    out << "      \"" << key << "\": " << value;
    if (trailing_comma) {
        out << ",";
    }
    out << "\n";
}

void WriteString(std::ostream& out, const char* key, const std::string& value, bool trailing_comma = true) {
    out << "      \"" << key << "\": \"" << EscapeJson(value) << "\"";
    if (trailing_comma) {
        out << ",";
    }
    out << "\n";
}

}  // namespace

bool LoadCardsFromJsonFile(const std::string& path,
                           std::vector<CardDef>* cards,
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
    return ParseCardList(root, false, cards, error);
}

bool LoadUpgradedCardsFromJsonFile(const std::string& path,
                                   std::vector<CardDef>* cards,
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
    return ParseCardList(root, true, cards, error);
}

bool SaveUpgradedCardsToJsonFile(const std::string& path,
                                 const std::vector<CardDef>& cards,
                                 std::string* error) {
    std::ofstream output(path.c_str(), std::ios::trunc);
    if (!output) {
        if (error != nullptr) {
            *error = "Unable to write upgraded card JSON file.";
        }
        return false;
    }

    output << "{\n";
    output << "  \"version\": 1,\n";
    output << "  \"cards\": [\n";
    for (size_t i = 0; i < cards.size(); ++i) {
        const CardDef& card = cards[i];
        output << "    {\n";
        WriteString(output, "base_id", card.id);
        WriteString(output, "name", card.name);
        WriteString(output, "type", card.type);
        WriteString(output, "description", card.description);
        WriteString(output, "rarity", RarityToken(card.rarity));
        WriteInt(output, "starterCopies", card.starterCopies);
        WriteInt(output, "cost", card.cost);
        WriteInt(output, "damage", card.damage);
        WriteInt(output, "shield", card.shield);
        WriteInt(output, "heal", card.heal);
        WriteInt(output, "draw", card.draw);
        WriteInt(output, "energy", card.energy);
        WriteInt(output, "nextTurnEnergy", card.nextTurnEnergy);
        WriteInt(output, "heat", card.heat);
        WriteInt(output, "enemyHeat", card.enemyHeat);
        WriteInt(output, "selfDamage", card.selfDamage);
        WriteInt(output, "multiHit", card.multiHit);
        WriteInt(output, "comboBonus", card.comboBonus);
        WriteInt(output, "attackMultiplier", card.attackMultiplier);
        WriteInt(output, "weakenTurns", card.weakenTurns);
        WriteInt(output, "removeEnemyShield", card.removeEnemyShield);
        WriteInt(output, "shieldLockTurns", card.shieldLockTurns);
        WriteInt(output, "salvageBonus", card.salvageBonus);
        WriteBool(output, "ignoreShield", card.ignoreShield);
        WriteBool(output, "shieldToDamage", card.shieldToDamage);
        WriteBool(output, "halveHeat", card.halveHeat);
        WriteBool(output, "clearHeat", card.clearHeat);
        WriteBool(output, "randomEffect", card.randomEffect, false);
        output << "    }";
        if (i + 1 < cards.size()) {
            output << ",";
        }
        output << "\n";
    }
    output << "  ]\n";
    output << "}\n";

    if (error != nullptr) {
        *error = "";
    }
    return true;
}
