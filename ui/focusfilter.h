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

#ifndef FOCUSFILTER_H
#define FOCUSFILTER_H

#include <QObject>

/**
 * @brief The FocusFilter class
 *
 * Some keyboard shortcuts/menu actions will do different things depending on the panel that's currently focused.
 * For example, pressing "Set Marker" will set a marker on the main active sequence if the timeline is focused,
 * or on the media in the Media Viewer if the Media Viewer is focused. This class provides slots/functions that
 * can be called that will check which panel is focused and call the appropriate function.
 *
 * Responds to `config.hover_focus`. Default behavior is focus by clicking on the panels, but if `hover_focus` is
 * **TRUE**, the focused panel will be whichever panel has the cursor currently hovering over it.
 */
class FocusFilter : public QObject {
  Q_OBJECT
public:
  /**
     * @brief FocusFilter Constructor
     *
     * Currently empty.
     */
  FocusFilter();

public slots:
  /**
     * @brief Cuts selected clips or selected effects (but not both).
     *
     * If the Effect Controls panel is focused, cuts selected effects. Otherwise cuts selected clips.
     */
  void cut();

  /**
     * @brief Copies selected clips or selected effects (but not both).
     *
     * If the Effect Controls panel is focused, copies selected effects. Otherwise copies selected clips.
     */
  void copy();

  /**
     * @brief Duplicates currently selected items
     *
     * Currently this only duplicates Sequences in the project panel.
     */
  void duplicate();

  /**
     * @brief Go to In Point.
     *
     * Calls go_to_in() on Media Viewer it's focused. Otherwise calls it on Sequence Viewer.
     */
  void go_to_in();

  /**
     * @brief Go to Out Point.
     *
     * Calls go_to_out() on Media Viewer it's focused. Otherwise calls it on Sequence Viewer.
     */
  void go_to_out();

  /**
     * @brief Go to Start
     *
     * Calls go_to_start() on Media Viewer it's focused. Otherwise calls it on Sequence Viewer.
     */
  void go_to_start();

  /**
     * @brief Go to Previous Frame
     *
     * Calls previous_frame() on Media Viewer it's focused. Otherwise calls it on Sequence Viewer.
     */
  void prev_frame();

  /**
     * @brief Play In Point to Out Point
     *
     * Calls play(true) on Media Viewer it's focused. Otherwise calls it on Sequence Viewer.
     */
  void play_in_to_out();

  /**
     * @brief Toggle Play/Pause
     *
     * Calls toggle_play() on Media Viewer it's focused. Otherwise calls it on Sequence Viewer.
     */
  void playpause();

  /**
     * @brief Pause/Shuttle Stop.
     *
     * Calls pause() on Media Viewer it's focused. Otherwise calls it on Sequence Viewer.
     */
  void pause();

  /**
     * @brief Increase Speed/Shuttle Right
     *
     * Calls increase_speed() on Media Viewer it's focused. Otherwise calls it on Sequence Viewer.
     */
  void increase_speed();

  /**
     * @brief Decrease Speed/Shuttle Left
     *
     * Calls decrease_speed() on Media Viewer it's focused. Otherwise calls it on Sequence Viewer.
     */
  void decrease_speed();

  /**
     * @brief Go to Next Frame
     *
     * Calls next_frame() on Media Viewer it's focused. Otherwise calls it on Sequence Viewer.
     */
  void next_frame();

  /**
     * @brief Go to End
     *
     * Calls go_to_end() on Media Viewer it's focused. Otherwise calls it on Sequence Viewer.
     */
  void go_to_end();

  /**
     * @brief Set currently focused viewer to full screen
     *
     * Calls viewer_widget->set_fullscreen() on Media Viewer it's focused. Otherwise calls it on Sequence Viewer.
     */
  void set_viewer_fullscreen();

  /**
     * @brief Set a marker at the current playhead
     *
     * Calls set_marker() on Media Viewer or Sequence Viewer if it's focused. Otherwise calls it on Timeline.
     */
  void set_marker();

  /**
     * @brief Set in point
     *
     * Calls set_in_point() on Media Viewer it's focused. Otherwise calls it on Sequence Viewer.
     */
  void set_in_point();

  /**
     * @brief Set out point
     *
     * Calls set_out_point() on Media Viewer it's focused. Otherwise calls it on Sequence Viewer.
     */
  void set_out_point();

  /**
     * @brief Clear in point
     *
     * Calls clear_in() on Media Viewer it's focused. Otherwise calls it on Sequence Viewer.
     */
  void clear_in();

  /**
     * @brief Clear out point
     *
     * Calls clear_out() on Media Viewer it's focused. Otherwise calls it on Sequence Viewer.
     */
  void clear_out();

  /**
     * @brief Clear in/out point
     *
     * Calls clear_inout() on Media Viewer it's focused. Otherwise calls it on Sequence Viewer.
     */
  void clear_inout();

  /**
     * @brief Delete
     *
     * Calls various delete functions based on which UI elements are focused. Deletes span anywhere from deleting
     * clips (Timeline), to effects (Effect Controls), to markers (TimelineHeader).
     */
  void delete_function();

  /**
     * @brief Select All
     *
     * Calls select_all() on Graph Editor if its focused or Timeline if it's not.
     */
  void select_all();

  /**
     * @brief Zoom In
     *
     * Calls zoom_in() on Effect Controls, Footage Viewer, or Sequence Viewer if one of them is focused. Otherwise
     * calls it on Timeline.
     */
  void zoom_in();

  /**
     * @brief Zoom Out
     *
     * Calls zoom_out() on Effect Controls, Footage Viewer, or Sequence Viewer if one of them is focused. Otherwise
     * calls it on Timeline.
     */
  void zoom_out();
};

namespace olive {
extern FocusFilter FocusFilter;
}

#endif // FOCUSFILTER_H
