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

#include "effectcontrols.h"

#include <QMenu>
#include <QVBoxLayout>
#include <QResizeEvent>
#include <QScrollBar>
#include <QSplitter>
#include <QScrollArea>
#include <QPushButton>
#include <QSpacerItem>
#include <QLabel>
#include <QVBoxLayout>
#include <QIcon>
#include <QApplication>

#include "panels/panels.h"
#include "project/effect.h"
#include "project/clip.h"
#include "project/transition.h"
#include "ui/collapsiblewidget.h"
#include "project/sequence.h"
#include "project/undo.h"
#include "panels/project.h"
#include "panels/timeline.h"
#include "panels/viewer.h"
#include "panels/grapheditor.h"
#include "ui/viewerwidget.h"
#include "io/clipboard.h"
#include "ui/timelineheader.h"
#include "ui/keyframeview.h"
#include "ui/resizablescrollbar.h"
#include "debug.h"

EffectControls::EffectControls(QWidget *parent) :
  QDockWidget(parent),
  multiple(false),
  zoom(1),
  panel_name(tr("Effects: ")),
  mode(kTransitionNone)
{
  setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

  setup_ui();

  clear_effects(false);
  headers->viewer = panel_sequence_viewer;
  headers->snapping = false;

  effects_area->parent_widget = scrollArea;
  effects_area->keyframe_area = keyframeView;
  effects_area->header = headers;
  keyframeView->header = headers;

  lblMultipleClipsSelected->setVisible(false);

  connect(keyframeView, SIGNAL(wheel_event_signal(QWheelEvent*)), effects_area, SLOT(receive_wheel_event(QWheelEvent*)));
  connect(horizontalScrollBar, SIGNAL(valueChanged(int)), headers, SLOT(set_scroll(int)));
  connect(horizontalScrollBar, SIGNAL(resize_move(double)), keyframeView, SLOT(resize_move(double)));
  connect(horizontalScrollBar, SIGNAL(valueChanged(int)), keyframeView, SLOT(set_x_scroll(int)));
  connect(verticalScrollBar, SIGNAL(valueChanged(int)), keyframeView, SLOT(set_y_scroll(int)));
  connect(verticalScrollBar, SIGNAL(valueChanged(int)), scrollArea->verticalScrollBar(), SLOT(setValue(int)));
  connect(scrollArea->verticalScrollBar(), SIGNAL(valueChanged(int)), verticalScrollBar, SLOT(setValue(int)));
}

EffectControls::~EffectControls() {}

int EffectControls::get_mode() {
  return mode;
}

bool EffectControls::keyframe_focus() {
  return headers->hasFocus() || keyframeView->hasFocus();
}

void EffectControls::set_zoom(bool in) {
  zoom *= (in) ? 2 : 0.5;
  update_keyframes();
}

void EffectControls::menu_select(QAction* q) {
  ComboAction* ca = new ComboAction();
  for (int i=0;i<selected_clips.size();i++) {
    const ClipPtr& c = olive::ActiveSequence->clips.at(selected_clips.at(i));
    if ((c->track < 0) == (effect_menu_subtype == EFFECT_TYPE_VIDEO)) {
      const EffectMeta* meta = reinterpret_cast<const EffectMeta*>(q->data().value<quintptr>());
      if (effect_menu_type == EFFECT_TYPE_TRANSITION) {
        if (c->opening_transition == nullptr) {
          ca->append(new AddTransitionCommand(c, nullptr, nullptr, meta, kTransitionOpening, 30));
        }
        if (c->closing_transition == nullptr) {
          ca->append(new AddTransitionCommand(c, nullptr, nullptr, meta, kTransitionClosing, 30));
        }
      } else {
        ca->append(new AddEffectCommand(c, nullptr, meta));
      }
    }
  }
  olive::UndoStack.push(ca);
  if (effect_menu_type == EFFECT_TYPE_TRANSITION) {
    update_ui(true);
  } else {
    reload_clips();
    panel_sequence_viewer->viewer_widget->frame_update();
  }
}

void EffectControls::update_keyframes() {
  headers->update_zoom(zoom);
  keyframeView->update();
}

void EffectControls::delete_selected_keyframes() {
  keyframeView->delete_selected_keyframes();
}

void EffectControls::copy(bool del) {
  if (mode == kTransitionNone) {
    bool cleared = false;

    ComboAction* ca = new ComboAction();
    EffectDeleteCommand* del_com = (del) ? new EffectDeleteCommand() : nullptr;
    for (int i=0;i<selected_clips.size();i++) {
      const ClipPtr& c = olive::ActiveSequence->clips.at(selected_clips.at(i));
      for (int j=0;j<c->effects.size();j++) {
        EffectPtr effect = c->effects.at(j);
        if (effect->container->selected) {
          if (!cleared) {
            clear_clipboard();
            cleared = true;
            clipboard_type = CLIPBOARD_TYPE_EFFECT;
          }

          clipboard.append(EffectPtr(effect->copy(nullptr)));

          if (del_com != nullptr) {
            del_com->clips.append(c);
            del_com->fx.append(j);
          }
        }
      }
    }
    if (del_com != nullptr) {
      if (del_com->clips.size() > 0) {
        ca->append(del_com);
      } else {
        delete del_com;
      }
    }
    olive::UndoStack.push(ca);
  }
}

void EffectControls::scroll_to_frame(long frame) {
  scroll_to_frame_internal(horizontalScrollBar, frame, zoom, keyframeView->width());
}

void EffectControls::add_effect_paste_action(QMenu *menu) {
  QAction* paste_action = menu->addAction(tr("&Paste"), panel_timeline, SLOT(paste(bool)));
  paste_action->setEnabled(clipboard.size() > 0 && clipboard_type == CLIPBOARD_TYPE_EFFECT);
}

void EffectControls::cut() {
  copy(true);
}

void EffectControls::show_effect_menu(int type, int subtype) {
  effect_menu_type = type;
  effect_menu_subtype = subtype;

  effects_loaded.lock();

  QMenu effects_menu(this);
  effects_menu.setToolTipsVisible(true);

  for (int i=0;i<effects.size();i++) {
    const EffectMeta& em = effects.at(i);

    if (em.type == type && em.subtype == subtype) {
      QAction* action = new QAction(&effects_menu);
      action->setText(em.name);
      action->setData(reinterpret_cast<quintptr>(&em));
      if (!em.tooltip.isEmpty()) {
        action->setToolTip(em.tooltip);
      }

      QMenu* parent = &effects_menu;
      if (!em.category.isEmpty()) {
        bool found = false;
        for (int j=0;j<effects_menu.actions().size();j++) {
          QAction* action = effects_menu.actions().at(j);
          if (action->menu() != nullptr) {
            if (action->menu()->title() == em.category) {
              parent = action->menu();
              found = true;
              break;
            }
          }
        }
        if (!found) {
          parent = new QMenu(&effects_menu);
          parent->setToolTipsVisible(true);
          parent->setTitle(em.category);

          bool found = false;
          for (int i=0;i<effects_menu.actions().size();i++) {
            QAction* comp_action = effects_menu.actions().at(i);
            if (comp_action->text() > em.category) {
              effects_menu.insertMenu(comp_action, parent);
              found = true;
              break;
            }
          }
          if (!found) effects_menu.addMenu(parent);
        }
      }

      bool found = false;
      for (int i=0;i<parent->actions().size();i++) {
        QAction* comp_action = parent->actions().at(i);
        if (comp_action->text() > action->text()) {
          parent->insertAction(comp_action, action);
          found = true;
          break;
        }
      }
      if (!found) parent->addAction(action);
    }
  }

  effects_loaded.unlock();

  connect(&effects_menu, SIGNAL(triggered(QAction*)), this, SLOT(menu_select(QAction*)));
  effects_menu.exec(QCursor::pos());
}

void EffectControls::clear_effects(bool clear_cache) {
  // clear existing clips
  deselect_all_effects(nullptr);

  // clear graph editor
  if (panel_graph_editor != nullptr) panel_graph_editor->set_row(nullptr);

  QVBoxLayout* video_layout = static_cast<QVBoxLayout*>(video_effect_area->layout());
  QVBoxLayout* audio_layout = static_cast<QVBoxLayout*>(audio_effect_area->layout());
  QLayoutItem* item;
  while ((item = video_layout->takeAt(0))) {
    item->widget()->setParent(nullptr);
    disconnect(static_cast<CollapsibleWidget*>(item->widget()), SIGNAL(deselect_others(QWidget*)), this, SLOT(deselect_all_effects(QWidget*)));
  }
  while ((item = audio_layout->takeAt(0))) {
    item->widget()->setParent(nullptr);
    disconnect(static_cast<CollapsibleWidget*>(item->widget()), SIGNAL(deselect_others(QWidget*)), this, SLOT(deselect_all_effects(QWidget*)));
  }
  lblMultipleClipsSelected->setVisible(false);
  vcontainer->setVisible(false);
  acontainer->setVisible(false);
  headers->setVisible(false);
  keyframeView->setEnabled(false);
  if (clear_cache) selected_clips.clear();
  setWindowTitle(panel_name + "(none)");
}

void EffectControls::deselect_all_effects(QWidget* sender) {
  for (int i=0;i<selected_clips.size();i++) {
    const ClipPtr& c = olive::ActiveSequence->clips.at(selected_clips.at(i));
    for (int j=0;j<c->effects.size();j++) {
      if (c->effects.at(j)->container != sender) {
        c->effects.at(j)->container->header_click(false, false);
      }
    }
  }
  panel_sequence_viewer->viewer_widget->update();
}

void EffectControls::open_effect(QVBoxLayout* layout, EffectPtr e) {
  CollapsibleWidget* container = e->container;
  layout->addWidget(container);
  connect(container, SIGNAL(deselect_others(QWidget*)), this, SLOT(deselect_all_effects(QWidget*)));
}

void EffectControls::setup_ui() {
  QWidget* contents = new QWidget(this);

  QHBoxLayout* hlayout = new QHBoxLayout(contents);
  hlayout->setSpacing(0);
  hlayout->setMargin(0);

  QSplitter* splitter = new QSplitter();
  splitter->setOrientation(Qt::Horizontal);
  splitter->setChildrenCollapsible(false);

  scrollArea = new QScrollArea();
  scrollArea->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Expanding);
  scrollArea->setFrameShape(QFrame::NoFrame);
  scrollArea->setFrameShadow(QFrame::Plain);
  scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  scrollArea->setWidgetResizable(true);

  QWidget* scrollAreaWidgetContents = new QWidget();

  QHBoxLayout* scrollAreaLayout = new QHBoxLayout(scrollAreaWidgetContents);
  scrollAreaLayout->setSpacing(0);
  scrollAreaLayout->setMargin(0);

  effects_area = new EffectsArea();
  effects_area->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(effects_area, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(effects_area_context_menu()));

  QVBoxLayout* effects_area_layout = new QVBoxLayout(effects_area);
  effects_area_layout->setSpacing(0);
  effects_area_layout->setMargin(0);

  vcontainer = new QWidget();
  QVBoxLayout* vcontainerLayout = new QVBoxLayout(vcontainer);
  vcontainerLayout->setSpacing(0);
  vcontainerLayout->setMargin(0);

  QWidget* veHeader = new QWidget();
  veHeader->setObjectName(QStringLiteral("veHeader"));
  veHeader->setStyleSheet(QLatin1String("#veHeader { background: rgba(0, 0, 0, 0.25); }"));

  QHBoxLayout* veHeaderLayout = new QHBoxLayout(veHeader);
  veHeaderLayout->setSpacing(0);
  veHeaderLayout->setMargin(0);

  QPushButton* btnAddVideoEffect = new QPushButton();
  btnAddVideoEffect->setIcon(QIcon(":/icons/add-effect.png"));
  btnAddVideoEffect->setToolTip(tr("Add Video Effect"));
  veHeaderLayout->addWidget(btnAddVideoEffect);
  connect(btnAddVideoEffect, SIGNAL(clicked(bool)), this, SLOT(video_effect_click()));

  veHeaderLayout->addStretch();

  QLabel* lblVideoEffects = new QLabel();
  QFont font;
  font.setPointSize(9);
  lblVideoEffects->setFont(font);
  lblVideoEffects->setAlignment(Qt::AlignCenter);
  lblVideoEffects->setText(tr("VIDEO EFFECTS"));
  veHeaderLayout->addWidget(lblVideoEffects);

  veHeaderLayout->addStretch();

  QPushButton* btnAddVideoTransition = new QPushButton();
  btnAddVideoTransition->setIcon(QIcon(":/icons/add-transition.png"));
  btnAddVideoTransition->setToolTip(tr("Add Video Transition"));
  connect(btnAddVideoTransition, SIGNAL(clicked(bool)), this, SLOT(video_transition_click()));
  veHeaderLayout->addWidget(btnAddVideoTransition);

  vcontainerLayout->addWidget(veHeader);

  video_effect_area = new QWidget();
  QVBoxLayout* veAreaLayout = new QVBoxLayout(video_effect_area);
  veAreaLayout->setSpacing(0);
  veAreaLayout->setMargin(0);

  vcontainerLayout->addWidget(video_effect_area);

  effects_area_layout->addWidget(vcontainer);

  acontainer = new QWidget();
  QVBoxLayout* acontainerLayout = new QVBoxLayout(acontainer);
  acontainerLayout->setSpacing(0);
  acontainerLayout->setMargin(0);
  QWidget* aeHeader = new QWidget();
  aeHeader->setObjectName(QStringLiteral("aeHeader"));
  aeHeader->setStyleSheet(QLatin1String("#aeHeader { background: rgba(0, 0, 0, 0.25); }"));

  QHBoxLayout* aeHeaderLayout = new QHBoxLayout(aeHeader);
  aeHeaderLayout->setSpacing(0);
  aeHeaderLayout->setMargin(0);

  QPushButton* btnAddAudioEffect = new QPushButton();
  btnAddAudioEffect->setIcon(QIcon(":/icons/add-effect.png"));
  btnAddAudioEffect->setToolTip(tr("Add Audio Effect"));
  connect(btnAddAudioEffect, SIGNAL(clicked(bool)), this, SLOT(audio_effect_click()));
  aeHeaderLayout->addWidget(btnAddAudioEffect);

  aeHeaderLayout->addStretch();

  QLabel* lblAudioEffects = new QLabel();
  lblAudioEffects->setFont(font);
  lblAudioEffects->setAlignment(Qt::AlignCenter);
  lblAudioEffects->setText(tr("AUDIO EFFECTS"));
  aeHeaderLayout->addWidget(lblAudioEffects);

  aeHeaderLayout->addStretch();

  QPushButton* btnAddAudioTransition = new QPushButton();
  btnAddAudioTransition->setIcon(QIcon(":/icons/add-transition.png"));
  btnAddAudioTransition->setToolTip(tr("Add Audio Transition"));
  connect(btnAddAudioTransition, SIGNAL(clicked(bool)), this, SLOT(audio_transition_click()));
  aeHeaderLayout->addWidget(btnAddAudioTransition);

  acontainerLayout->addWidget(aeHeader);

  audio_effect_area = new QWidget();
  QVBoxLayout* aeAreaLayout = new QVBoxLayout(audio_effect_area);
  aeAreaLayout->setSpacing(0);
  aeAreaLayout->setMargin(0);

  acontainerLayout->addWidget(audio_effect_area);

  effects_area_layout->addWidget(acontainer);

  lblMultipleClipsSelected = new QLabel();
  lblMultipleClipsSelected->setAlignment(Qt::AlignCenter);
  lblMultipleClipsSelected->setText(tr("(Multiple clips selected)"));
  effects_area_layout->addWidget(lblMultipleClipsSelected);

  effects_area_layout->addStretch();

  scrollAreaLayout->addWidget(effects_area);

  scrollArea->setWidget(scrollAreaWidgetContents);
  splitter->addWidget(scrollArea);

  QWidget* keyframeArea = new QWidget();

  QSizePolicy keyframe_sp;
  keyframe_sp.setHorizontalPolicy(QSizePolicy::Minimum);
  keyframe_sp.setVerticalPolicy(QSizePolicy::Preferred);
  keyframe_sp.setHorizontalStretch(1);
  keyframeArea->setSizePolicy(keyframe_sp);

  QVBoxLayout* keyframeAreaLayout = new QVBoxLayout(keyframeArea);
  keyframeAreaLayout->setSpacing(0);
  keyframeAreaLayout->setMargin(0);

  headers = new TimelineHeader();
  keyframeAreaLayout->addWidget(headers);

  QWidget* keyframeCenterWidget = new QWidget();

  QHBoxLayout* keyframeCenterLayout = new QHBoxLayout(keyframeCenterWidget);
  keyframeCenterLayout->setSpacing(0);
  keyframeCenterLayout->setMargin(0);

  keyframeView = new KeyframeView();

  keyframeCenterLayout->addWidget(keyframeView);

  verticalScrollBar = new QScrollBar();
  verticalScrollBar->setOrientation(Qt::Vertical);

  keyframeCenterLayout->addWidget(verticalScrollBar);


  keyframeAreaLayout->addWidget(keyframeCenterWidget);

  horizontalScrollBar = new ResizableScrollBar();
  horizontalScrollBar->setOrientation(Qt::Horizontal);

  keyframeAreaLayout->addWidget(horizontalScrollBar);

  splitter->addWidget(keyframeArea);

  hlayout->addWidget(splitter);

  setWidget(contents);
}

void EffectControls::update_scrollbar() {
  verticalScrollBar->setMaximum(qMax(0, effects_area->height() - keyframeView->height() - headers->height()));
  verticalScrollBar->setPageStep(verticalScrollBar->height());
}

void EffectControls::queue_post_update() {
  keyframeView->update();
  update_scrollbar();
}

void EffectControls::effects_area_context_menu() {
  QMenu menu(this);

  add_effect_paste_action(&menu);

  menu.exec(QCursor::pos());
}

void EffectControls::load_effects() {
  lblMultipleClipsSelected->setVisible(multiple);

  if (!multiple) {
    // load in new clips
    for (int i=0;i<selected_clips.size();i++) {
      ClipPtr c = olive::ActiveSequence->clips.at(selected_clips.at(i));
      QVBoxLayout* layout;
      if (c->track < 0) {
        vcontainer->setVisible(true);
        layout = static_cast<QVBoxLayout*>(video_effect_area->layout());
      } else {
        acontainer->setVisible(true);
        layout = static_cast<QVBoxLayout*>(audio_effect_area->layout());
      }
      if (mode == kTransitionNone) {
        for (int j=0;j<c->effects.size();j++) {
          open_effect(layout, c->effects.at(j));
        }
      } else if (mode == kTransitionOpening && c->opening_transition != nullptr) {
        open_effect(layout, c->opening_transition);
      } else if (mode == kTransitionClosing && c->closing_transition != nullptr) {
        open_effect(layout, c->closing_transition);
      }
    }
    if (selected_clips.size() > 0) {
      setWindowTitle(panel_name + olive::ActiveSequence->clips.at(selected_clips.at(0))->name);
      keyframeView->setEnabled(true);
      headers->setVisible(true);

      QTimer::singleShot(50, this, SLOT(queue_post_update()));
    }
  }
}

void EffectControls::delete_effects() {
  // load in new clips
  if (mode == kTransitionNone) {
    EffectDeleteCommand* command = new EffectDeleteCommand();
    for (int i=0;i<selected_clips.size();i++) {
      ClipPtr c = olive::ActiveSequence->clips.at(selected_clips.at(i));
      for (int j=0;j<c->effects.size();j++) {
        EffectPtr effect = c->effects.at(j);
        if (effect->container->selected) {
          command->clips.append(c);
          command->fx.append(j);
        }
      }
    }
    if (command->clips.size() > 0) {
      olive::UndoStack.push(command);
      panel_sequence_viewer->viewer_widget->frame_update();
    } else {
      delete command;
    }
  }
}

void EffectControls::reload_clips() {
  clear_effects(false);
  load_effects();
}

void EffectControls::set_clips(QVector<int>& clips, int m) {
  clear_effects(true);

  // replace clip vector
  selected_clips = clips;
  mode = m;

  load_effects();
}

void EffectControls::video_effect_click() {
  show_effect_menu(EFFECT_TYPE_EFFECT, EFFECT_TYPE_VIDEO);
}

void EffectControls::audio_effect_click() {
  show_effect_menu(EFFECT_TYPE_EFFECT, EFFECT_TYPE_AUDIO);
}

void EffectControls::video_transition_click() {
  show_effect_menu(EFFECT_TYPE_TRANSITION, EFFECT_TYPE_VIDEO);
}

void EffectControls::audio_transition_click() {
  show_effect_menu(EFFECT_TYPE_TRANSITION, EFFECT_TYPE_AUDIO);
}

void EffectControls::resizeEvent(QResizeEvent*) {
  update_scrollbar();
}

bool EffectControls::is_focused() {
  if (this->hasFocus()) return true;
  for (int i=0;i<selected_clips.size();i++) {
    ClipPtr c = olive::ActiveSequence->clips.at(selected_clips.at(i));
    if (c != nullptr) {
      for (int j=0;j<c->effects.size();j++) {
        if (c->effects.at(j)->container->is_focused()) {
          return true;
        }
      }
    } else {
      qWarning() << "Tried to check focus of a nullptr clip";
    }
  }
  return false;
}

EffectsArea::EffectsArea(QWidget* parent) :
  QWidget(parent)
{}

void EffectsArea::receive_wheel_event(QWheelEvent *e) {
  QApplication::sendEvent(this, e);
}
