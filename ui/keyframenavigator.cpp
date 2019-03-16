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

#include "keyframenavigator.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QApplication>
#include <QDesktopWidget>
#include <QDebug>

#include "ui/icons.h"

KeyframeNavigator::KeyframeNavigator(QWidget *parent, bool addLeftPad) : QWidget(parent) {
  setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Expanding);

  key_controls = new QHBoxLayout(this);
  key_controls->setSpacing(0);
  key_controls->setMargin(0);

  if (addLeftPad) {
    key_controls->addStretch();
  }

  left_key_nav = new QPushButton(this);
  left_key_nav->setIcon(olive::icon::LeftArrow);
  left_key_nav->setIconSize(left_key_nav->iconSize()*0.5);
  left_key_nav->setVisible(false);
  key_controls->addWidget(left_key_nav);
  connect(left_key_nav, SIGNAL(clicked(bool)), this, SIGNAL(goto_previous_key()));
  connect(left_key_nav, SIGNAL(clicked(bool)), this, SIGNAL(clicked()));

  key_addremove = new QPushButton(this);
  key_addremove->setIcon(olive::icon::Diamond);
  key_addremove->setIconSize(key_addremove->iconSize()*0.5);
  key_addremove->setVisible(false);
  key_controls->addWidget(key_addremove);
  connect(key_addremove, SIGNAL(clicked(bool)), this, SIGNAL(toggle_key()));
  connect(key_addremove, SIGNAL(clicked(bool)), this, SIGNAL(clicked()));

  right_key_nav = new QPushButton(this);
  right_key_nav->setIcon(olive::icon::RightArrow);
  right_key_nav->setIconSize(right_key_nav->iconSize()*0.5);
  right_key_nav->setVisible(false);
  key_controls->addWidget(right_key_nav);
  connect(right_key_nav, SIGNAL(clicked(bool)), this, SIGNAL(goto_next_key()));
  connect(right_key_nav, SIGNAL(clicked(bool)), this, SIGNAL(clicked()));

  keyframe_enable = new QPushButton(this);
  keyframe_enable->setIcon(olive::icon::Clock);
  keyframe_enable->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
  keyframe_enable->setIconSize(keyframe_enable->iconSize()*0.75);
  keyframe_enable->setCheckable(true);
  keyframe_enable->setToolTip(tr("Enable Keyframes"));
  connect(keyframe_enable, SIGNAL(clicked(bool)), this, SIGNAL(keyframe_enabled_changed(bool)));
  connect(keyframe_enable, SIGNAL(toggled(bool)), this, SLOT(keyframe_ui_enabled(bool)));
  connect(keyframe_enable, SIGNAL(clicked(bool)), this, SIGNAL(clicked()));
  key_controls->addWidget(keyframe_enable);
}

KeyframeNavigator::~KeyframeNavigator() {}

void KeyframeNavigator::enable_keyframes(bool b) {
  keyframe_enable->setChecked(b);
}

void KeyframeNavigator::enable_keyframe_toggle(bool b) {
  keyframe_enable->setVisible(b);
}

void KeyframeNavigator::keyframe_ui_enabled(bool enabled) {
  left_key_nav->setVisible(enabled);
  key_addremove->setVisible(enabled);
  right_key_nav->setVisible(enabled);
}
