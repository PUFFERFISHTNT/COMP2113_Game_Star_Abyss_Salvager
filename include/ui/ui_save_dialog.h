#ifndef UI_SAVE_DIALOG_H
#define UI_SAVE_DIALOG_H

enum class SaveDialogMode {
    None,
    Save,
    Load,
};

// Checks one byte from a save name and returns whether it is allowed.
bool IsAllowedSaveNameChar(unsigned char ch);

#endif
