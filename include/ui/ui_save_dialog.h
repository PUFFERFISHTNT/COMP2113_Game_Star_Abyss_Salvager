#ifndef UI_SAVE_DIALOG_H
#define UI_SAVE_DIALOG_H

enum class SaveDialogMode {
    None,
    Save,
    Load,
};

// what it does: Checks whether a character is valid for a save-slot name.
// what the inputs are: ch - character byte to validate.
// what the outputs are: Returns true when the character is allowed; otherwise false (bool).
bool IsAllowedSaveNameChar(unsigned char ch);

#endif
