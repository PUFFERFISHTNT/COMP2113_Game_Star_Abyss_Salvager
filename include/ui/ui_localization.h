#ifndef UI_LOCALIZATION_H
#define UI_LOCALIZATION_H

#include <string>

enum class UiLanguage {
    English,
    Chinese,
};

// Chooses between the English and Chinese text for the given language and returns the selected string.
std::string Tr(UiLanguage language, const std::string& english, const std::string& chinese);

// Checks the given text against a prefix and returns whether it starts there.
bool StartsWith(const std::string& text, const std::string& prefix);

// Checks the given text against a suffix and returns whether it ends there.
bool EndsWith(const std::string& text, const std::string& suffix);

// Translates known gameplay text for the given language and returns the translated or original string.
std::string TranslateGameText(UiLanguage language, const std::string& text);

#endif
