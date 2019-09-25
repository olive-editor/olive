/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

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

#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <QCheckBox>
#include <QDialog>
#include <QMenuBar>
#include <QTabWidget>

#include "tabs/preferencestab.h"

/**
 * @brief The PreferencesDialog class
 *
 * A dialog for the global application settings. Mostly an interface for Config. Can be loaded from any part of the
 * application.
 */
class PreferencesDialog : public QDialog
{
  Q_OBJECT

public:
  /**
   * @brief PreferencesDialog Constructor
   *
   * @param parent
   *
   * QWidget parent. Usually MainWindow.
   */
  explicit PreferencesDialog(QWidget *parent, QMenuBar* main_menu_bar);

private slots:
  /**
   * @brief Override of accept to save preferences to Config.
   */
  virtual void accept() override;

private:
  void AddTab(PreferencesTab* tab, const QString& title);

  QTabWidget* tab_widget_;

  QList<PreferencesTab*> tabs_;

  /**
   * @brief Stored default Sequence object
   *
   * Default Sequence settings are loaded into an actual Sequence object that can be loaded into NewSequenceDialog
   * for the sake of familiarity with the user.
   */
  //Sequence default_sequence;

  /**
   * @brief Add an automated QCheckBox+boolean value pair
   *
   * Many preferences are simple true/false (or on/off) options. Rather than adding a QCheckBox for each one and
   * manually setting its checked value to the configuration setting (and vice versa when saving), this convenience
   * function will add it to an automated set of checkboxes, automatically setting the checked state to the current
   * setting, and then saving the new checked state back to the setting when the user accepts the changes (clicks OK).
   *
   * @param ui
   *
   * A valid QCheckBox item. This function does not take ownership of the QWidget or place it in a layout anywhere.
   *
   * @param value
   *
   * A pointer to the Boolean value this QCheckBox should be shared with. The QCheckBox widget's checked state will be
   * set to the value of this pointer.
   *
   * @param restart_required
   *
   * Defaults to FALSE, set this to TRUE if changing this setting should prompt the user for a restart of Olive before
   * the setting change takes effect.
   */
  void AddBoolPair(QCheckBox* ui, bool* value, bool restart_required = false);

  /**
   * @brief Internal array managed by AddBoolPair(). Do not access this directly.
   */
  QVector<QCheckBox*> bool_ui;

  /**
   * @brief Internal array managed by AddBoolPair(). Do not access this directly.
   */
  QVector<bool*> bool_value;

  /**
   * @brief Internal array managed by AddBoolPair(). Do not access this directly.
   */
  QVector<bool> bool_restart_required;
};

#endif // PREFERENCESDIALOG_H
