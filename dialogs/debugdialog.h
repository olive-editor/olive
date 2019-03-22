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

#ifndef DEBUGDIALOG_H
#define DEBUGDIALOG_H

#include <QDialog>
#include <QTextEdit>

/**
 * @brief The DebugDialog class
 *
 * A dialog to display the current debug output. This dialog is omnipresent and shown and hidden when the user wants
 * to see it. For efficiency, it will not update if it's hidden.
 */
class DebugDialog : public QDialog {
  Q_OBJECT
public:
  /**
   * @brief DebugDialog Constructor
   * @param parent
   *
   * Parent widget. Usually MainWindow.
   */
  DebugDialog(QWidget* parent = nullptr);

  /**
   * @brief Retranslate window title
   *
   * Sets title based on the current translation.
   */
  void Retranslate();
public slots:
  /**
   * @brief Update the visual log with the debug text from get_debug_str()
   */
  void update_log();
protected:
  /**
   * @brief Overrides change event to trigger Retranslate() on a LanguageChange event.
   */
  virtual void changeEvent(QEvent* e) override;
  /**
   * @brief Overrides show event to trigger an update of the visual log (the visual log does not update while the
   * debug dialog is hidden).
   */
  virtual void showEvent(QShowEvent* event) override;
private:
  /**
   * @brief Display widget for the debug dialog.
   */
  QTextEdit* textEdit;
};

namespace olive {
/**
 * @brief Omnipresent instance of DebugDialog to be shown or hidden as the user wants
 */
extern DebugDialog* DebugDialog;
}

#endif // DEBUGDIALOG_H
