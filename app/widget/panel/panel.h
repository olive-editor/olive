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

#ifndef PANEL_WIDGET_H
#define PANEL_WIDGET_H

#include <QDockWidget>
#include <QEvent>

/**
 * @brief The PanelWidget class
 *
 * A widget that is always dockable within the MainWindow.
 */
class PanelWidget : public QDockWidget {
  Q_OBJECT
public:
  /**
   * @brief PanelWidget Constructor
   *
   * @param parent
   *
   * The PanelWidget's parent, enforced to help with memory handling. Most of the time this will be an instance of
   * MainWindow.
   */
  PanelWidget(QWidget* parent);
protected:
  /**
   * @brief Set panel's title
   *
   * Use this function to set the title of the panel.
   *
   * A PanelWidget has the default format of "Title: Subtitle" (can differ depending on translation). If no Subtitle
   * is set, the title will just be formatted "Title".
   *
   * @param t
   *
   * String to set the title to
   */
  void SetTitle(const QString& t);

  /**
   * @brief Set panel's subtitle
   *
   * Use this function to set the subtitle of the panel.
   *
   * A PanelWidget has the default format of "Title: Subtitle" (can differ depending on translation). If no Subtitle
   * is set, the title will just be formatted "Title".
   *
   * @param t
   *
   * String to set the subtitle to
   */
  void SetSubtitle(const QString& t);
private:
  /**
   * @brief Internal function that sets the QDockWidget's window title whenever the title/subtitle change.
   *
   * Should be called any time a change is made to title_ or subtitle_
   */
  void UpdateTitle();

  /**
   * @brief Internal title string
   */
  QString title_;

  /**
   * @brief Internal subtitle string
   */
  QString subtitle_;
};

#endif // PANEL_WIDGET_H
