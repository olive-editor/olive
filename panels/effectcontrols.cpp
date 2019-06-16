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

#include <QVBoxLayout>
#include <QResizeEvent>
#include <QScrollBar>
#include <QScrollArea>
#include <QPushButton>
#include <QSpacerItem>
#include <QLabel>
#include <QVBoxLayout>
#include <QIcon>
#include <QApplication>

#include "panels/panels.h"
#include "nodes/oldeffectnode.h"
#include "effects/effectloaders.h"
#include "effects/transition.h"
#include "timeline/clip.h"
#include "ui/collapsiblewidget.h"
#include "timeline/sequence.h"
#include "undo/undo.h"
#include "undo/undostack.h"
#include "panels/project.h"
#include "panels/timeline.h"
#include "panels/viewer.h"
#include "panels/grapheditor.h"
#include "ui/viewerwidget.h"
#include "ui/menuhelper.h"
#include "ui/icons.h"
#include "global/clipboard.h"
#include "global/config.h"
#include "global/global.h"
#include "ui/timelineheader.h"
#include "ui/keyframeview.h"
#include "ui/resizablescrollbar.h"
#include "global/debug.h"
#include "ui/menu.h"

EffectControls::EffectControls(QWidget *parent) :
  EffectsPanel(parent),
  zoom(1)
{
  setup_ui();
  Retranslate();

  Clear(false);

  headers->viewer = panel_sequence_viewer;
  headers->snapping = false;

  effects_area->parent_widget = scrollArea;
  effects_area->keyframe_area = keyframeView;
  effects_area->header = headers;
  keyframeView->header = headers;

  connect(keyframeView, SIGNAL(wheel_event_signal(QWheelEvent*)), effects_area, SLOT(receive_wheel_event(QWheelEvent*)));
  connect(horizontalScrollBar, SIGNAL(valueChanged(int)), headers, SLOT(set_scroll(int)));
  connect(horizontalScrollBar, SIGNAL(resize_move(double)), keyframeView, SLOT(resize_move(double)));
  connect(horizontalScrollBar, SIGNAL(valueChanged(int)), keyframeView, SLOT(set_x_scroll(int)));
  connect(verticalScrollBar, SIGNAL(valueChanged(int)), keyframeView, SLOT(set_y_scroll(int)));
  connect(verticalScrollBar, SIGNAL(valueChanged(int)), scrollArea->verticalScrollBar(), SLOT(setValue(int)));
  connect(scrollArea->verticalScrollBar(), SIGNAL(valueChanged(int)), verticalScrollBar, SLOT(setValue(int)));
}

void EffectControls::set_zoom(bool in) {
  zoom *= (in) ? 2 : 0.5;
  update_keyframes();

  if (!selected_clips_.isEmpty()) {
    // FIXME
    //scroll_to_frame(selected_clips_.first()->track()->sequence()->playhead);
  }
}

void EffectControls::update_keyframes() {
  for (int i=0;i<open_effects_.size();i++) {
    EffectUI* ui = open_effects_.at(i);
    ui->UpdateFromEffect();
  }

  headers->update_zoom(zoom);
  keyframeView->update();
}

void EffectControls::delete_selected_keyframes() {
  keyframeView->delete_selected_keyframes();
}

void EffectControls::scroll_to_frame(long frame) {
  scroll_to_frame_internal(horizontalScrollBar, frame - keyframeView->visible_in, zoom, keyframeView->width());
}

void EffectControls::UpdateTitle() {
  if (selected_clips_.isEmpty()) {
    setWindowTitle(panel_name + tr("(none)"));
  } else {
    setWindowTitle(panel_name + selected_clips_.first()->name());
  }
}

void EffectControls::setup_ui() {
  QWidget* contents = new QWidget(this);

  QHBoxLayout* hlayout = new QHBoxLayout(contents);
  hlayout->setSpacing(0);
  hlayout->setMargin(0);

  splitter = new QSplitter();
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

  QIcon add_effect_icon = olive::icon::CreateIconFromSVG(":/icons/add-effect.svg", false);
  QIcon add_transition_icon = olive::icon::CreateIconFromSVG(":/icons/add-transition.svg", false);

  btnAddVideoEffect = new QPushButton();
  btnAddVideoEffect->setIcon(add_effect_icon);
  veHeaderLayout->addWidget(btnAddVideoEffect);
  connect(btnAddVideoEffect, SIGNAL(clicked(bool)), this, SLOT(video_effect_click()));

  veHeaderLayout->addStretch();

  lblVideoEffects = new QLabel();
  QFont font;
  font.setPointSize(9);
  lblVideoEffects->setFont(font);
  lblVideoEffects->setAlignment(Qt::AlignCenter);
  veHeaderLayout->addWidget(lblVideoEffects);

  veHeaderLayout->addStretch();

  btnAddVideoTransition = new QPushButton();
  btnAddVideoTransition->setIcon(add_transition_icon);
  connect(btnAddVideoTransition, SIGNAL(clicked(bool)), this, SLOT(video_transition_click()));
  veHeaderLayout->addWidget(btnAddVideoTransition);

  vcontainerLayout->addWidget(veHeader);

  video_effect_area = new QWidget();
  video_effect_layout = new QVBoxLayout(video_effect_area);
  video_effect_layout->setSpacing(0);
  video_effect_layout->setMargin(0);

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

  btnAddAudioEffect = new QPushButton();
  btnAddAudioEffect->setIcon(add_effect_icon);
  connect(btnAddAudioEffect, SIGNAL(clicked(bool)), this, SLOT(audio_effect_click()));
  aeHeaderLayout->addWidget(btnAddAudioEffect);

  aeHeaderLayout->addStretch();

  lblAudioEffects = new QLabel();
  lblAudioEffects->setFont(font);
  lblAudioEffects->setAlignment(Qt::AlignCenter);
  aeHeaderLayout->addWidget(lblAudioEffects);

  aeHeaderLayout->addStretch();

  btnAddAudioTransition = new QPushButton();
  btnAddAudioTransition->setIcon(add_transition_icon);
  connect(btnAddAudioTransition, SIGNAL(clicked(bool)), this, SLOT(audio_transition_click()));
  aeHeaderLayout->addWidget(btnAddAudioTransition);

  acontainerLayout->addWidget(aeHeader);

  audio_effect_area = new QWidget();
  audio_effect_layout = new QVBoxLayout(audio_effect_area);
  audio_effect_layout->setSpacing(0);
  audio_effect_layout->setMargin(0);

  acontainerLayout->addWidget(audio_effect_area);

  effects_area_layout->addWidget(acontainer);

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

void EffectControls::Retranslate() {
  panel_name = tr("Effects: ");

  btnAddVideoEffect->setToolTip(tr("Add Video Effect"));
  lblVideoEffects->setText(tr("VIDEO EFFECTS"));
  btnAddVideoTransition->setToolTip(tr("Add Video Transition"));
  btnAddAudioEffect->setToolTip(tr("Add Audio Effect"));
  lblAudioEffects->setText(tr("AUDIO EFFECTS"));
  btnAddAudioTransition->setToolTip(tr("Add Audio Transition"));

  UpdateTitle();
}

void EffectControls::LoadLayoutState(const QByteArray &data)
{
  splitter->restoreState(data);
}

QByteArray EffectControls::SaveLayoutState()
{
  return splitter->saveState();
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
  Menu menu(this);

  olive::MenuHelper.create_effect_paste_action(&menu);

  menu.exec(QCursor::pos());
}

bool EffectControls::focused()
{
  if (this->hasFocus()
      || headers->hasFocus()
      || keyframeView->hasFocus()) {
    return true;
  }

  return EffectsPanel::focused();
}

void EffectControls::video_effect_click() {
  olive::Global->ShowEffectMenu(EFFECT_TYPE_EFFECT, olive::kTypeVideo, selected_clips_);
}

void EffectControls::audio_effect_click() {
  olive::Global->ShowEffectMenu(EFFECT_TYPE_EFFECT, olive::kTypeAudio, selected_clips_);
}

void EffectControls::video_transition_click() {
  olive::Global->ShowEffectMenu(EFFECT_TYPE_TRANSITION, olive::kTypeVideo, selected_clips_);
}

void EffectControls::audio_transition_click() {
  olive::Global->ShowEffectMenu(EFFECT_TYPE_TRANSITION, olive::kTypeAudio, selected_clips_);
}

void EffectControls::resizeEvent(QResizeEvent*) {
  update_scrollbar();
}

void EffectControls::ClearEvent()
{
  keyframeView->SetEffects(open_effects_);

  vcontainer->setVisible(false);
  acontainer->setVisible(false);
  headers->setVisible(false);
  keyframeView->setEnabled(false);

  UpdateTitle();
}

void EffectControls::LoadEvent()
{
  keyframeView->SetEffects(open_effects_);

  if (selected_clips_.size() > 0) {
    keyframeView->setEnabled(true);

    headers->setVisible(true);

    QTimer::singleShot(50, this, SLOT(queue_post_update()));
  }

  for (int i=0;i<open_effects_.size();i++) {
    QVBoxLayout* layout = nullptr;

    EffectUI* container = open_effects_.at(i);
    OldEffectNode* e = open_effects_.at(i)->GetEffect();

    if (e->subtype() == olive::kTypeVideo) {
      vcontainer->setVisible(true);
      layout = video_effect_layout;
    } else if (e->subtype() == olive::kTypeAudio) {
      acontainer->setVisible(true);
      layout = audio_effect_layout;
    }

    if (layout != nullptr) {
      layout->addWidget(container);
    }
  }

  UpdateTitle();
  update_keyframes();
}

EffectsArea::EffectsArea(QWidget* parent) :
  QWidget(parent)
{}

void EffectsArea::resizeEvent(QResizeEvent *)
{
  parent_widget->setMinimumWidth(sizeHint().width());
}

void EffectsArea::receive_wheel_event(QWheelEvent *e) {
  QApplication::sendEvent(this, e);
}
