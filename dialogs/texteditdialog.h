/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019  Olive Team

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#ifndef TEXTEDITDIALOG_H
#define TEXTEDITDIALOG_H

#include <QDialog>
#include <QPlainTextEdit>
#include <QFontComboBox>

#include "ui/labelslider.h"
#include "ui/colorbutton.h"

/**
 * @brief The TextEditDialog class
 *
 * A separate window for editing text. This window can be resized arbitrarily and also provides a toolbar for rich text
 * editing (if rich text is enabled). This dialog can be run from anywhere. Once the dialog has closed (i.e. returned
 * from exec() ), the text entered into it can be retrieved using get_string().
 *
 * TODO: Add a live signal for updating the calling function.
 */
class TextEditDialog : public QDialog {
  Q_OBJECT
public:
  /**
   * @brief TextEditDialog Constructor
   *
   * @param parent
   *
   * QWidget parent. Usually MainWindow.
   *
   * @param s
   *
   * The starting string when the dialog opens. It'll be read as rich text HTML or plain text based on the `rich_text`
   * parameter (which defaults to rich text HTML). It can also be left empty to start blank.
   *
   * @param rich_text
   *
   * Set the editing mode of the editor. If TRUE, the dialog will interpret the string in `s` as rich text HTML and also
   * return rich text HTML through get_string(). It'll also show a toolbar with rich text options (i.e. font, italic,
   * underline, size, etc.) If FALSE, the dialog will run in plain text mode interpreting the string in `s` as plain
   * text and returning plain text through get_string(). It also will not show the rich text editing toolbar.
   */
  TextEditDialog(QWidget* parent = nullptr, const QString& s = nullptr, bool rich_text = true);

  /**
   * @brief Retrieve the current text in the dialog
   *
   * This function can be called after the user has accepted the dialog (i.e. made changes and clicked OK).
   * This will return either plain text or rich text (HTML) depending on the mode it's running in (rich/plain text mode
   * is set in the constructor). The value this returns only gets updated when the user clicks OK so it cannot be
   * used to retrieve live text updates from the dialog.
   *
   * @return
   *
   * The text entered once the user accepted this dialog.
   */
  const QString& get_string();
private slots:
  /**
   * @brief Override of accept() to store the entered text string so it can be retrieved by get_string().
   */
  virtual void accept() override;

  /**
   * @brief Slot for the font_weight combobox to set the font weight based on its data value
   *
   * @param i
   *
   * Index of the font_weight to retrieve the desired font weight from
   */
  void SetFontWeight(int i);

  /**
   * @brief Slot for text alignment buttons to set alignment based on their properties
   *
   * Intended slot for left_align_button, center_align_button, right_align_button, and justify_align_button. Pulls
   * from their property("a") value which should be a member of the Qt::Alignment enum.
   */
  void SetAlignmentFromProperty();

  /**
   * @brief Slot for when the text edit widget's cursor moves so the rich text toolbar can stay up to date
   *
   * In rich text mode, different parts of a text document can be formatted in different ways. As the user moves
   * around the text, the UI buttons should be consistent with whatever text is currently selected. This slot should
   * therefore be connected to QTextEdit::cursorPositionChanged() and will change the "checked" state of the formatting
   * buttons and current index of the comboboxes to match the currently selected text.
   */
  void UpdateUIFromTextCursor();
private:

  /**
   * @brief Internal rich text mode value
   *
   * This is set in the constructor and cannot be changed during the lifetime of this dialog.
   */
  bool rich_text_;

  /**
   * @brief Internal storage of text entered, saved when the user clicks OK
   */
  QString result_str;

  /**
   * @brief Main text editing widget
   */
  QTextEdit* textEdit;

  /**
   * @brief Toggle button for setting the italic state of the currently selected text
   */
  QPushButton* italic_button;

  /**
   * @brief Toggle button for setting the underlined state of the currently selected text
   */
  QPushButton* underline_button;

  /**
   * @brief ComboBox for the list of font families that the selected text can be set to
   */
  QFontComboBox* font_list;

  /**
   * @brief ComboBox for the list of font weights that the selected text can be set to
   */
  QComboBox* font_weight;

  /**
   * @brief A slider to set the current font size
   */
  LabelSlider* font_size;

  /**
   * @brief A color selector for setting the current text color
   */
  ColorButton* font_color;

  /**
   * @brief Button for setting the current text row(s) to left alignment
   */
  QPushButton* left_align_button;

  /**
   * @brief Button for setting the current text row(s) to center alignment
   */
  QPushButton* center_align_button;

  /**
   * @brief Button for setting the current text row(s) to right alignment
   */
  QPushButton* right_align_button;

  /**
   * @brief Button for setting the current text row(s) to justified alignment
   */
  QPushButton* justify_align_button;
};

#endif // TEXTEDITDIALOG_H
