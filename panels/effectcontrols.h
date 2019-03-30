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

#ifndef EFFECTCONTROLS_H
#define EFFECTCONTROLS_H

#include <QUndoCommand>
#include <QMutex>
#include <QMenu>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QLabel>
#include <QScrollBar>
#include <QHBoxLayout>
#include <QSplitter>

#include "project/projectelements.h"
#include "timeline/track.h"
#include "ui/timelineheader.h"
#include "ui/keyframeview.h"
#include "ui/resizablescrollbar.h"
#include "ui/keyframeview.h"
#include "ui/panel.h"
#include "ui/effectui.h"

class EffectsArea : public QWidget {
  Q_OBJECT
public:
  EffectsArea(QWidget* parent = nullptr);
  QScrollArea* parent_widget;
  KeyframeView* keyframe_area;
  TimelineHeader* header;
protected:
  void resizeEvent(QResizeEvent*);
public slots:
  void receive_wheel_event(QWheelEvent* e);
};

class EffectControls : public Panel
{
  Q_OBJECT
public:
  explicit EffectControls(QWidget *parent = nullptr);
  virtual ~EffectControls() override;


  void Reload();
  void SetClips();
  void Clear(bool clear_cache = true);

  bool IsEffectSelected(Effect* e);

  void DeleteSelectedEffects();
  virtual bool focused() override;
  void set_zoom(bool in);
  void delete_selected_keyframes();
  void scroll_to_frame(long frame);

  double zoom;

  ResizableScrollBar* horizontalScrollBar;
  QScrollBar* verticalScrollBar;

  virtual void Retranslate() override;

  virtual void LoadLayoutState(const QByteArray& data) override;
  virtual QByteArray SaveLayoutState() override;
public slots:
  void cut();
  void copy(bool del = false);
  void update_keyframes();
private slots:
  void menu_select(QAction* q);

  void video_effect_click();
  void audio_effect_click();
  void video_transition_click();
  void audio_transition_click();

  void deselect_all_effects(QWidget*);

  void update_scrollbar();
  void queue_post_update();

  void effects_area_context_menu();
protected:
  virtual void resizeEvent(QResizeEvent *event) override;
private:
  QVector<Clip*> selected_clips_;
  QVector<EffectUI*> open_effects_;

  void Load();

  void DeleteEffect(ComboAction* ca, Effect* effect_ref);

  void show_effect_menu(int type, Track::Type subtype);
  void load_keyframes();
  void open_effect(QVBoxLayout* hlayout, Effect *e);
  void UpdateTitle();

  void setup_ui();

  int effect_menu_type;
  Track::Type effect_menu_subtype;
  QString panel_name;

  QWidget* video_effect_area;
  QWidget* audio_effect_area;
  QVBoxLayout* video_effect_layout;
  QVBoxLayout* audio_effect_layout;

  QSplitter* splitter;
  QPushButton* btnAddVideoEffect;
  QLabel* lblVideoEffects;
  QLabel* lblAudioEffects;
  QPushButton* btnAddVideoTransition;
  QPushButton* btnAddAudioEffect;
  QPushButton* btnAddAudioTransition;
  TimelineHeader* headers;
  EffectsArea* effects_area;
  QScrollArea* scrollArea;
  KeyframeView* keyframeView;
  QWidget* vcontainer;
  QWidget* acontainer;
};

#endif // EFFECTCONTROLS_H
