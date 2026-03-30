#ifndef UI_SAVE_DIALOG_H
#define UI_SAVE_DIALOG_H

enum class SaveDialogMode {
    None,
    Save,
    Load,
};

bool IsAllowedSaveNameChar(unsigned char ch);

#endif
