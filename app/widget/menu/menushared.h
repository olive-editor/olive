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

#ifndef MENUSHARED_H
#define MENUSHARED_H

#include "widget/menu/menu.h"

OLIVE_NAMESPACE_ENTER

/**
 * @brief A static object that provides various "stock" menus for use throughout the application
 */
class MenuShared : public QObject {
  Q_OBJECT
public:
  MenuShared();

  static void CreateInstance();
  static void DestroyInstance();

  void Retranslate();

  void AddItemsForNewMenu(Menu* m);
  void AddItemsForEditMenu(Menu* m, bool for_clips);
  void AddItemsForInOutMenu(Menu* m);
  void AddItemsForClipEditMenu(Menu* m);
  void AddItemsForTimeRulerMenu(Menu* m);

  void AboutToShowTimeRulerActions();

  static MenuShared* instance();

private:
  // "New" menu shared items
  QAction* new_project_item_;
  QAction* new_sequence_item_;
  QAction* new_folder_item_;

  // "Edit" menu shared items
  QAction* edit_cut_item_;
  QAction* edit_copy_item_;
  QAction* edit_paste_item_;
  QAction* edit_paste_insert_item_;
  QAction* edit_duplicate_item_;
  QAction* edit_delete_item_;
  QAction* edit_ripple_delete_item_;
  QAction* edit_split_item_;

  // "In/Out" menu shared items
  QAction* inout_set_in_item_;
  QAction* inout_set_out_item_;
  QAction* inout_reset_in_item_;
  QAction* inout_reset_out_item_;
  QAction* inout_clear_inout_item_;

  // "Clip Edit" menu shared items
  QAction* clip_add_default_transition_item_;
  QAction* clip_link_unlink_item_;
  QAction* clip_enable_disable_item_;
  QAction* clip_nest_item_;

  // TimeRuler menu shared items
  QActionGroup* frame_view_mode_group_;
  QAction* view_timecode_view_dropframe_item_;
  QAction* view_timecode_view_nondropframe_item_;
  QAction* view_timecode_view_seconds_item_;
  QAction* view_timecode_view_frames_item_;
  QAction* view_timecode_view_milliseconds_item_;

  static MenuShared* instance_;

private slots:
  void SplitAtPlayheadTriggered();

  void DeleteSelectedTriggered();

  void RippleDeleteTriggered();

  void SetInTriggered();

  void SetOutTriggered();

  void ResetInTriggered();

  void ResetOutTriggered();

  void ClearInOutTriggered();

  void ToggleLinksTriggered();

  void CutTriggered();

  void CopyTriggered();

  void PasteTriggered();

  void PasteInsertTriggered();

  void DuplicateTriggered();

  void EnableDisableTriggered();

  void NestTriggered();

  void DefaultTransitionTriggered();

  /**
   * @brief A slot for the timecode display menu items
   *
   * Assumes a QAction* sender() and its data() is a member of enum Timecode::Display. Uses the data() to signal a
   * timecode change throughout the rest of the application.
   */
  void TimecodeDisplayTriggered();

};

OLIVE_NAMESPACE_EXIT

#endif // MENUSHARED_H
