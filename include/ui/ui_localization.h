#ifndef UI_LOCALIZATION_H
#define UI_LOCALIZATION_H

#include <string>

enum class UiLanguage {
    English,
    Chinese,
};

std::string Tr(UiLanguage language, const std::string& english, const std::string& chinese);
bool StartsWith(const std::string& text, const std::string& prefix);
bool EndsWith(const std::string& text, const std::string& suffix);
std::string TranslateGameText(UiLanguage language, const std::string& text);

#endif
