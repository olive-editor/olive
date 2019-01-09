/* 
 * Olive. Olive is a free non-linear video editor for Windows, macOS, and Linux.
 * Copyright (C) 2018  {{ organization }}
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 *along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include "keyframenavigator.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QApplication>
#include <QDesktopWidget>

KeyframeNavigator::KeyframeNavigator(QWidget *parent) : QWidget(parent) {
	int icon_size_val = 8*QApplication::desktop()->devicePixelRatio();
	int clock_size_val = 12*QApplication::desktop()->devicePixelRatio();
	int button_size_val = 20*QApplication::desktop()->devicePixelRatio();

	QSize icon_size(icon_size_val, icon_size_val);
	QSize clock_size(clock_size_val, clock_size_val);
	QSize button_size(button_size_val, button_size_val);

    key_controls = new QHBoxLayout();
    key_controls->setSpacing(0);
    key_controls->setMargin(0);
	//key_controls->addStretch();

    setLayout(key_controls);

    left_key_nav = new QPushButton();
    left_key_nav->setIcon(QIcon(":/icons/tri-left.png"));
    left_key_nav->setMaximumSize(button_size);
    left_key_nav->setIconSize(icon_size);
    left_key_nav->setVisible(false);
    key_controls->addWidget(left_key_nav);
    connect(left_key_nav, SIGNAL(clicked(bool)), this, SIGNAL(goto_previous_key()));
	connect(left_key_nav, SIGNAL(clicked(bool)), this, SIGNAL(clicked()));

    key_addremove = new QPushButton();
    key_addremove->setIcon(QIcon(":/icons/diamond.png"));
    key_addremove->setMaximumSize(button_size);
    key_addremove->setIconSize(QSize(8, 8));
    key_addremove->setVisible(false);
    key_controls->addWidget(key_addremove);
    connect(key_addremove, SIGNAL(clicked(bool)), this, SIGNAL(toggle_key()));
	connect(key_addremove, SIGNAL(clicked(bool)), this, SIGNAL(clicked()));

    right_key_nav = new QPushButton();
    right_key_nav->setIcon(QIcon(":/icons/tri-right.png"));
    right_key_nav->setMaximumSize(button_size);
    right_key_nav->setIconSize(icon_size);
    right_key_nav->setVisible(false);
    key_controls->addWidget(right_key_nav);
    connect(right_key_nav, SIGNAL(clicked(bool)), this, SIGNAL(goto_next_key()));
	connect(right_key_nav, SIGNAL(clicked(bool)), this, SIGNAL(clicked()));

    keyframe_enable = new QPushButton(QIcon(":/icons/clock.png"), "");
    keyframe_enable->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    keyframe_enable->setMaximumSize(button_size);
	keyframe_enable->setIconSize(clock_size);
    keyframe_enable->setCheckable(true);
    keyframe_enable->setToolTip("Enable Keyframes");
	connect(keyframe_enable, SIGNAL(clicked(bool)), this, SIGNAL(keyframe_enabled_changed(bool)));
    connect(keyframe_enable, SIGNAL(toggled(bool)), this, SLOT(keyframe_ui_enabled(bool)));
	connect(keyframe_enable, SIGNAL(clicked(bool)), this, SIGNAL(clicked()));
    key_controls->addWidget(keyframe_enable);
}

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
