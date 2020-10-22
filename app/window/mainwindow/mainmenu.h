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

#include <QMainWindow>
#include <QMenuBar>

#include "dialog/actionsearch/actionsearch.h"
#include "widget/menu/menu.h"

OLIVE_NAMESPACE_ENTER

class MainWindow;

/**
 * @brief Olive's main menubar attached to its main window.
 *
 * Responsible for creating the menu, connecting signals/slots, and retranslating the items on a language change.
 */
class MainMenu : public QMenuBar
{
  Q_OBJECT
public:
  MainMenu(MainWindow *parent);

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
   * Assumes a QAction* sender() and its data() is a member of enum Tool::Item. Uses the data() to signal a
   * Tool change throughout the rest of the application.
   */
  void ToolItemTriggered();

  /**
   * @brief Slot triggered just before the File menu shows
   */
  void FileMenuAboutToShow();

  /**
   * @brief Slot triggered just before the View menu shows
   */
  void ViewMenuAboutToShow();

  /**
   * @brief Slot triggered just before the Tools menu shows
   */
  void ToolsMenuAboutToShow();

  /**
   * @brief Slot triggered just before the Playback menu shows
   */
  void PlaybackMenuAboutToShow();

  /**
   * @brief Slot triggered just before the Sequence menu shows
   */
  void SequenceMenuAboutToShow();

  /**
   * @brief Slot triggered just before the Window menu shows
   */
  void WindowMenuAboutToShow();

  /**
   * @brief Adds items to open recent menu
   */
  void PopulateOpenRecent();

  /**
   * @brief Clears open recent items when menu closes
   */
  void CloseOpenRecentMenu();

  /**
   * @brief Slot for zooming in
   *
   * Finds the currently focused panel and sends it a "zoom in" signal
   */
  void ZoomInTriggered();

  /**
   * @brief Slot for zooming out
   *
   * Finds the currently focused panel and sends it a "zoom out" signal
   */
  void ZoomOutTriggered();

  void IncreaseTrackHeightTriggered();
  void DecreaseTrackHeightTriggered();

  void GoToStartTriggered();
  void PrevFrameTriggered();

  /**
   * @brief Slot for play/pause
   *
   * Finds the currently focused panel and sends it a "play/pause" signal
   */
  void PlayPauseTriggered();

  void PlayInToOutTriggered();

  void LoopTriggered(bool enabled);

  void NextFrameTriggered();
  void GoToEndTriggered();

  void SelectAllTriggered();
  void DeselectAllTriggered();

  void InsertTriggered();
  void OverwriteTriggered();

  void RippleToInTriggered();
  void RippleToOutTriggered();
  void EditToInTriggered();
  void EditToOutTriggered();

  void ActionSearchTriggered();

  void ShuttleLeftTriggered();
  void ShuttleStopTriggered();
  void ShuttleRightTriggered();

  void GoToPrevCutTriggered();
  void GoToNextCutTriggered();

  void SetMarkerTriggered();

  void FullScreenViewerTriggered();

  void ToggleShowAllTriggered();

  void DeleteInOutTriggered();
  void RippleDeleteInOutTriggered();

  void GoToInTriggered();
  void GoToOutTriggered();

  void OpenRecentItemTriggered();

  void SequenceCacheTriggered();
  void SequenceCacheInOutTriggered();

  void HelpFeedbackTriggered();

private:
  /**
   * @brief Set strings based on the current application language.
   */
  void Retranslate();

  Menu* file_menu_;
  Menu* file_new_menu_;
  QAction* file_open_item_;
  Menu* file_open_recent_menu_;
  QAction* file_open_recent_separator_;
  QAction* file_open_recent_clear_item_;
  QAction* file_save_item_;
  QAction* file_save_as_item_;
  QAction* file_save_all_item_;
  QAction* file_import_item_;
  Menu* file_export_menu_;
  QAction* file_export_media_item_;
#ifdef USE_OTIO
  QAction* file_export_otio_item_;
#endif
  QAction* file_project_properties_item_;
  QAction* file_close_project_item_;
  QAction* file_close_all_projects_item_;
  QAction* file_close_all_except_item_;
  QAction* file_exit_item_;

  Menu* edit_menu_;
  QAction* edit_undo_item_;
  QAction* edit_redo_item_;
  QAction* edit_select_all_item_;
  QAction* edit_deselect_all_item_;
  QAction* edit_insert_item_;
  QAction* edit_overwrite_item_;
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

  Menu* sequence_menu_;
  QAction* sequence_cache_item_;
  QAction* sequence_cache_in_to_out_item_;

  Menu* window_menu_;
  QAction* window_menu_separator_;
  QAction* window_maximize_panel_item_;
  QAction* window_lock_layout_item_;
  QAction* window_reset_layout_item_;

  Menu* tools_menu_;
  QActionGroup* tools_group_;
  QAction* tools_pointer_item_;
  QAction* tools_edit_item_;
  QAction* tools_ripple_item_;
  QAction* tools_rolling_item_;
  QAction* tools_razor_item_;
  QAction* tools_slip_item_;
  QAction* tools_slide_item_;
  QAction* tools_hand_item_;
  QAction* tools_zoom_item_;
  QAction* tools_transition_item_;
  QAction* tools_snapping_item_;
  QAction* tools_preferences_item_;

  Menu* help_menu_;
  QAction* help_action_search_item_;
  QAction* help_feedback_item_;
  QAction* help_about_item_;

};

OLIVE_NAMESPACE_EXIT

#endif // MAINMENU_H
