#include "ui/ui_save_dialog.h"

#include <algorithm>
#include <cctype>
#include <filesystem>

#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/terminal.hpp>

#include "ui/star_abyss_app.h"
#include "ui/ui_localization.h"
#include "ui/ui_panels.h"

using namespace ftxui;

bool IsAllowedSaveNameChar(unsigned char ch) {
    return std::isalnum(ch) != 0 || ch == '_' || ch == '-';
}

void StarAbyssApp::SaveRun() {
    SaveToSlot(saveNameInput_, true);
}

void StarAbyssApp::LoadRun() {
    LoadSelectedSave();
}

void StarAbyssApp::OpenSaveDialog(SaveDialogMode mode) {
    saveDialogMode_ = mode;
    saveDialogVisible_ = true;
    saveDialogScrollLine_ = 0;
    RefreshSaveEntries();
    if (mode == SaveDialogMode::Save && saveNameInput_.empty()) {
        saveNameInput_ = "save1";
    }
    if (mode == SaveDialogMode::Save && !engine_.state().hasRun) {
        saveDialogVisible_ = false;
        engine_.showInfo(Tr(language_, "Save Failed", "保存失败"),
                         {TranslateGameText(language_, "No active run to save.")},
                         Tr(language_, "Press Back to continue.", "点击返回以继续。"));
    }
}

void StarAbyssApp::CloseSaveDialog() {
    saveDialogVisible_ = false;
    saveDialogMode_ = SaveDialogMode::None;
}

std::string StarAbyssApp::SanitizeSaveName(const std::string& raw_name) const {
    std::string cleaned;
    for (unsigned char ch : raw_name) {
        if (IsAllowedSaveNameChar(ch)) {
            cleaned.push_back(static_cast<char>(std::tolower(ch)));
        } else if (ch == ' ' || ch == '.') {
            cleaned.push_back('_');
        }
    }
    while (!cleaned.empty() && cleaned.front() == '_') {
        cleaned.erase(cleaned.begin());
    }
    while (!cleaned.empty() && cleaned.back() == '_') {
        cleaned.pop_back();
    }
    return cleaned.empty() ? "save" : cleaned;
}

void StarAbyssApp::RefreshSaveEntries() {
    namespace fs = std::filesystem;
    saveEntries_.clear();
    saveSlotNames_.clear();
    savePaths_.clear();
    fs::create_directories("save");

    std::vector<fs::path> paths;
    for (const auto& entry : fs::directory_iterator("save")) {
        if (!entry.is_regular_file()) {
            continue;
        }
        const fs::path path = entry.path();
        const std::string extension = path.extension().string();
        if (extension == ".sav" || extension == ".txt") {
            paths.push_back(path);
        }
    }
    std::sort(paths.begin(), paths.end(), [](const fs::path& lhs, const fs::path& rhs) {
        const bool lhs_autosave = lhs.stem().string() == "autosave";
        const bool rhs_autosave = rhs.stem().string() == "autosave";
        if (lhs_autosave != rhs_autosave) {
            return lhs_autosave;
        }
        return lhs.filename().string() < rhs.filename().string();
    });

    for (const fs::path& path : paths) {
        const std::string slot_name = path.stem().string();
        savePaths_.push_back(path.string());
        saveSlotNames_.push_back(slot_name);
        saveEntries_.push_back(slot_name == "autosave" ? Tr(language_, "autosave (automatic)", "autosave（自动）") : slot_name);
    }
    if (saveEntries_.empty()) {
        saveEntries_.push_back(TranslateGameText(language_, "No save files"));
    }
    saveSelected_ = std::max(0, std::min(saveSelected_, static_cast<int>(saveEntries_.size()) - 1));
    if (!saveSlotNames_.empty() && saveDialogMode_ == SaveDialogMode::Save && saveNameInput_.empty()) {
        saveNameInput_ = saveSlotNames_[static_cast<size_t>(saveSelected_)];
    }
}

std::string StarAbyssApp::SaveDisplayName(size_t index) const {
    if (index >= saveSlotNames_.size()) {
        return "";
    }
    const std::string& entry = saveSlotNames_[index];
    if (entry == "autosave") {
        return Tr(language_, "autosave (automatic)", "autosave（自动）");
    }
    return entry;
}

void StarAbyssApp::SaveToSlot(const std::string& slot_name, bool show_feedback) {
    if (!engine_.state().hasRun) {
        if (show_feedback) {
            engine_.showInfo(Tr(language_, "Save Failed", "保存失败"),
                             {TranslateGameText(language_, "No active run to save.")},
                             Tr(language_, "Press Back to continue.", "点击返回以继续。"));
        }
        return;
    }

    namespace fs = std::filesystem;
    fs::create_directories("save");
    const std::string sanitized = SanitizeSaveName(slot_name);
    const std::string path = (fs::path("save") / (sanitized + ".sav")).string();
    std::string error;
    if (engine_.saveToFile(path, &error)) {
        RefreshSaveEntries();
        saveNameInput_ = sanitized;
        if (show_feedback) {
            infoReturnsToRun_ = engine_.state().hasRun;
            engine_.showInfo(Tr(language_, "Save Complete", "保存成功"),
                             {Tr(language_, "Run saved to ", "存档已写入 ") + path + "。"},
                             Tr(language_, "Press Back to resume.", "点击返回以继续。"));
        }
        CloseSaveDialog();
    } else if (show_feedback) {
        engine_.showInfo(Tr(language_, "Save Failed", "保存失败"),
                         {TranslateGameText(language_, error)},
                         Tr(language_, "Press Back to continue.", "点击返回以继续。"));
    }
}

void StarAbyssApp::LoadSelectedSave() {
    if (savePaths_.empty()) {
        engine_.showInfo(Tr(language_, "Load Failed", "读取失败"),
                         {Tr(language_, "No save files are available.", "当前没有可读取的存档。")},
                         Tr(language_, "Press Back to return.", "点击返回。"));
        return;
    }
    if (saveSelected_ < 0 || static_cast<size_t>(saveSelected_) >= savePaths_.size()) {
        return;
    }
    const bool had_run = engine_.state().hasRun;
    std::string error;
    if (engine_.loadFromFile(savePaths_[static_cast<size_t>(saveSelected_)], &error)) {
        infoReturnsToRun_ = true;
        engine_.showInfo(Tr(language_, "Save Loaded", "读取成功"),
                         {Tr(language_, "Run restored from ", "已从 ") + savePaths_[static_cast<size_t>(saveSelected_)] + " " +
                              Tr(language_, "loaded.", "恢复航程。")},
                         Tr(language_, "Press Back to resume.", "点击返回以继续。"));
        CloseSaveDialog();
    } else {
        infoReturnsToRun_ = had_run;
        engine_.showInfo(Tr(language_, "Load Failed", "读取失败"),
                         {TranslateGameText(language_, error)},
                         Tr(language_, "Press Back to return.", "点击返回。"));
    }
}

void StarAbyssApp::AutoSave() {
    if (!engine_.state().hasRun) {
        return;
    }
    SaveToSlot("autosave", false);
}

// The save dialog is separated from page rendering because it is a modal
// workflow shared by map, encounter, battle, and menu actions.
Element StarAbyssApp::RenderSaveDialog() const {
    const bool save_mode = saveDialogMode_ == SaveDialogMode::Save;
    const int terminal_width = Terminal::Size().dimx;
    const int terminal_height = Terminal::Size().dimy;
    const bool compact_layout = terminal_width < 96;
    const int dialog_width = std::min(110, std::max(40, terminal_width - 4));
    const int dialog_height = std::max(14, terminal_height - 6);
    Elements selection_lines;
    if (!savePaths_.empty() && saveSelected_ >= 0 && static_cast<size_t>(saveSelected_) < savePaths_.size()) {
        const size_t selected = static_cast<size_t>(saveSelected_);
        selection_lines.push_back(text(Tr(language_, "Selected Slot: ", "当前槽位：") + SaveDisplayName(selected)) | color(kGold));
        selection_lines.push_back(paragraph(Tr(language_, "File: ", "文件：") + savePaths_[selected]) | color(Color::White));
        selection_lines.push_back(text(save_mode ? Tr(language_,
                                                       "Saving to an existing name overwrites that slot.",
                                                       "保存到已有名称会覆盖对应存档。")
                                                 : Tr(language_,
                                                       "Use arrow keys or the mouse to select a save, then load it.",
                                                       "可用方向键或鼠标选择存档，然后直接读取。")) |
                                  color(kDim));
    } else {
        selection_lines.push_back(text(TranslateGameText(language_, "No save files")) | color(kDim));
    }

    Elements action_buttons;
    if (save_mode) {
        action_buttons.push_back(saveConfirmButton_->Render() | flex);
        action_buttons.push_back(autoSaveButton_->Render() | flex);
    } else {
        action_buttons.push_back(loadConfirmButton_->Render() | flex);
    }
    action_buttons.push_back(closeDialogButton_->Render() | flex);

    Element actions = compact_layout ? vbox(std::move(action_buttons)) : hbox(std::move(action_buttons));

    Element body = vbox({
        paragraph(save_mode
                      ? Tr(language_,
                           "Type a save name, save to it manually, or rely on the automatic autosave slot.",
                           "输入存档名称后即可手动保存，也可以直接使用自动存档槽。")
                      : Tr(language_,
                           "Choose any save slot below and load it directly.",
                           "从下方选择任意存档并直接读取。")) |
            color(Color::White),
        separator(),
        window(text(Tr(language_, "Save Name", "存档名称")) | color(kHull),
               vbox({
                   saveNameInputComponent_->Render(),
                   text(Tr(language_, "Allowed characters: letters, numbers, _ and -", "允许字符：字母、数字、_ 和 -")) | color(kDim),
               })) |
            bgcolor(kPanel),
        separator(),
        window(text(Tr(language_, "Available Saves", "可用存档")) | color(kHull), vbox({saveEntriesMenu_->Render()})) |
            bgcolor(kPanel),
        separator(),
        window(text(Tr(language_, "Selected Save", "当前选中的存档")) | color(kHull), vbox(std::move(selection_lines))) |
            bgcolor(kPanel),
        separator(),
        actions,
    });

    return window(
               text(save_mode ? Tr(language_, "Save Manager", "存档管理")
                              : Tr(language_, "Load Manager", "读档管理")) |
                   color(kGold) | bold,
               ApplyVerticalScrollFrame(body, saveDialogScrollLine_) | size(HEIGHT, LESS_THAN, dialog_height)) |
           size(WIDTH, LESS_THAN, dialog_width) | bgcolor(kPanelAlt) | center;
}
