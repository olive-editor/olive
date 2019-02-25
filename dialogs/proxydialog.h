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

#ifndef PROXYDIALOG_H
#define PROXYDIALOG_H

#include <QDialog>
#include <QVector>
#include <QComboBox>

#include "project/media.h"

/**
 * @brief The ProxyDialog class
 *
 * Dialog to set up proxy generation of footage
 */
class ProxyDialog : public QDialog {
  Q_OBJECT
public:
  /**
   * @brief ProxyDialog Constructor
   * @param parent
   *
   * Parent widget to become modal to.
   *
   * @param footage
   *
   * List of Footage items to process.
   */
  ProxyDialog(QWidget* parent, const QVector<Media *> &media);
public slots:
  /**
   * @brief Accept changes
   *
   * Called when the user clicks OK on the dialog. Verifies all proxies, asking the user whether they want to overwrite
   * existing proxies if necessary, and if everything is valid, queues the footage with ProxyGenerator.
   */
  virtual void accept() override;
private:
  /**
   * @brief User's desired dimensions
   *
   * Always a fraction of the original video size (e.g. 1/2, 1/4, 1/8, etc.)
   */
  QComboBox* size_combobox;

  /**
   * @brief User's desired proxy format
   *
   * e.g. ProRes, DNxHD, etc.
   */
  QComboBox* format_combobox;

  /**
   * @brief Allows users to set the directory to store proxies in
   */
  QComboBox* location_combobox;

  /**
   * @brief Stores the custom location to store proxies if the user sets a custom location
   */
  QString custom_location;

  /**
   * @brief Stores the default subdirectory to be made next to the source (dependent on the user's language)
   *
   * "Proxy" in en-US.
   */
  QString proxy_folder_name;

  /**
   * @brief Stored list of footage to make proxies for
   */
  QVector<Media*> selected_media;
private slots:
  /**
   * @brief Slot when the user changes the location
   *
   * Triggered when the user changes the index in the location combobox.
   *
   * @param i
   *
   * location_combobox's new selected index
   */
  void location_changed(int i);
};

#endif // PROXYDIALOG_H
