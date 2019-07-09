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

#ifndef MAINMENU_H
#define MAINMENU_H

#include <QMenuBar>

#include "widget/menu/menu.h"

/**
 * @brief Olive's main menubar attached to its main window.
 *
 * Responsible for creating the menu, connecting signals/slots, and retranslating the items on a language change.
 */
class MainMenu : public QMenuBar
{
  Q_OBJECT
public:
  MainMenu(QWidget* parent);

protected:
  /**
   * @brief changeEvent
   *
   * Qt changeEvent override to catch a QEvent::LanguageEvent.
   *
   * @param e
   */
  virtual void changeEvent(QEvent* e);

private slots:
  /**
   * @brief A slot for the Tool selection items
   *
   * Assumes a QAction* sender() and its data() is a member of enum olive::tool:Tool. Uses the data() to signal a
   * Tool change throughout the rest of the application.
   */
  void ToolItemTriggered();

private:
  /**
   * @brief Set strings based on the current application language.
   */
  void Retranslate();

  Menu* file_menu_;
  Menu* file_new_menu_;
  QAction* file_open_item_;
  Menu* file_open_recent_menu_;
  QAction* file_open_recent_clear_item_;
  QAction* file_save_item_;
  QAction* file_save_as_item_;
  QAction* file_import_item_;
  QAction* file_export_item_;
  QAction* file_exit_item_;

  Menu* edit_menu_;
  QAction* edit_undo_item_;
  QAction* edit_redo_item_;
  QAction* edit_select_all_item_;
  QAction* edit_deselect_all_item_;
  QAction* edit_ripple_to_in_item_;
  QAction* edit_ripple_to_out_item_;
  QAction* edit_edit_to_in_item_;
  QAction* edit_edit_to_out_item_;
  QAction* edit_delete_inout_item_;
  QAction* edit_ripple_delete_inout_item_;
  QAction* edit_set_marker_item_;

  Menu* view_menu_;
  QAction* view_zoom_in_item_;
  QAction* view_zoom_out_item_;
  QAction* view_increase_track_height_item_;
  QAction* view_decrease_track_height_item_;
  QAction* view_show_all_item_;
  QAction* view_rectified_waveforms_item_;
  QAction* view_timecode_view_frames_item_;
  QAction* view_timecode_view_dropframe_item_;
  QAction* view_timecode_view_nondropframe_item_;
  QAction* view_timecode_view_milliseconds_item_;
  Menu* view_title_safe_area_menu_;
  QAction* title_safe_off_item_;
  QAction* title_safe_default_item_;
  QAction* title_safe_43_item_;
  QAction* title_safe_169_item_;
  QAction* title_safe_custom_item_;
  QAction* view_full_screen_item_;
  QAction* view_full_screen_viewer_item_;

  Menu* playback_menu_;
  QAction* playback_gotostart_item_;
  QAction* playback_prevframe_item_;
  QAction* playback_playpause_item_;
  QAction* playback_playinout_item_;
  QAction* playback_nextframe_item_;
  QAction* playback_gotoend_item_;
  QAction* playback_prevcut_item_;
  QAction* playback_nextcut_item_;
  QAction* playback_gotoin_item_;
  QAction* playback_gotoout_item_;
  QAction* playback_shuttleleft_item_;
  QAction* playback_shuttlestop_item_;
  QAction* playback_shuttleright_item_;
  QAction* playback_loop_item_;

  Menu* window_menu_;
  QAction* window_maximize_panel_item_;
  QAction* window_lock_layout_item_;
  QAction* window_reset_layout_item_;

  Menu* tools_menu_;
  QAction* tools_pointer_item_;
  QAction* tools_edit_item_;
  QAction* tools_ripple_item_;
  QAction* tools_rolling_item_;
  QAction* tools_razor_item_;
  QAction* tools_slip_item_;
  QAction* tools_slide_item_;
  QAction* tools_hand_item_;
  QAction* tools_transition_item_;
  QAction* tools_snapping_item_;
  QAction* tools_autocut_silence_item_;
  QAction* tools_autoscroll_none_item_;
  QAction* tools_autoscroll_page_item_;
  QAction* tools_autoscroll_smooth_item_;
  QAction* tools_preferences_item_;

  Menu* help_menu_;
  QAction* help_action_search_item_;
  QAction* help_debug_log_item_;
  QAction* help_about_item_;
};

#endif // MAINMENU_H
