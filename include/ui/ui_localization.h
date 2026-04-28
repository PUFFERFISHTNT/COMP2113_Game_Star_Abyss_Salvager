#ifndef UI_LOCALIZATION_H
#define UI_LOCALIZATION_H

#include <string>

enum class UiLanguage {
    English,
    Chinese,
};

// what it does: Chooses localized UI text for the active language.
// what the inputs are: language - selected UI language; english - English text; chinese - Chinese text.
// what the outputs are: Returns the selected localized text (std::string).
std::string Tr(UiLanguage language, const std::string& english, const std::string& chinese);

// what it does: Checks whether text starts with a given prefix.
// what the inputs are: text - source string to inspect; prefix - prefix to match.
// what the outputs are: Returns true when text begins with prefix; otherwise false (bool).
bool StartsWith(const std::string& text, const std::string& prefix);

// what it does: Checks whether text ends with a given suffix.
// what the inputs are: text - source string to inspect; suffix - suffix to match.
// what the outputs are: Returns true when text ends with suffix; otherwise false (bool).
bool EndsWith(const std::string& text, const std::string& suffix);

// what it does: Translates known gameplay text into the requested language.
// what the inputs are: language - selected UI language; text - source gameplay text.
// what the outputs are: Returns translated gameplay text when available, otherwise the source text (std::string).
std::string TranslateGameText(UiLanguage language, const std::string& text);

#endif
