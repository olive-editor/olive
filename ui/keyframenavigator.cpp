#include "keyframenavigator.h"

#include <QHBoxLayout>
#include <QPushButton>

KeyframeNavigator::KeyframeNavigator(QWidget *parent) : QWidget(parent) {
    QSize icon_size(12, 12);
    QSize button_size(20, 20);

    key_controls = new QHBoxLayout();
    key_controls->setSpacing(0);
    key_controls->setMargin(0);
    key_controls->addStretch();

    setLayout(key_controls);

    left_key_nav = new QPushButton();
    left_key_nav->setIcon(QIcon(":/icons/tri-left.png"));
    left_key_nav->setMaximumSize(button_size);
    left_key_nav->setIconSize(icon_size);
    left_key_nav->setVisible(false);
    key_controls->addWidget(left_key_nav);
    connect(left_key_nav, SIGNAL(clicked(bool)), this, SIGNAL(goto_previous_key()));

    key_addremove = new QPushButton();
    key_addremove->setIcon(QIcon(":/icons/diamond.png"));
    key_addremove->setMaximumSize(button_size);
    key_addremove->setIconSize(QSize(8, 8));
    key_addremove->setVisible(false);
    key_controls->addWidget(key_addremove);
    connect(key_addremove, SIGNAL(clicked(bool)), this, SIGNAL(toggle_key()));

    right_key_nav = new QPushButton();
    right_key_nav->setIcon(QIcon(":/icons/tri-right.png"));
    right_key_nav->setMaximumSize(button_size);
    right_key_nav->setIconSize(icon_size);
    right_key_nav->setVisible(false);
    key_controls->addWidget(right_key_nav);
    connect(right_key_nav, SIGNAL(clicked(bool)), this, SIGNAL(goto_next_key()));

    keyframe_enable = new QPushButton(QIcon(":/icons/clock.png"), "");
    keyframe_enable->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    keyframe_enable->setMaximumSize(button_size);
    keyframe_enable->setIconSize(icon_size);
    keyframe_enable->setCheckable(true);
    keyframe_enable->setToolTip("Enable Keyframes");
    connect(keyframe_enable, SIGNAL(clicked(bool)), this, SIGNAL(set_keyframe_enabled(bool)));
    connect(keyframe_enable, SIGNAL(toggled(bool)), this, SLOT(keyframe_ui_enabled(bool)));
    key_controls->addWidget(keyframe_enable);
}

void KeyframeNavigator::enable_keyframes(bool b) {
    keyframe_enable->setChecked(b);
}

void KeyframeNavigator::keyframe_ui_enabled(bool enabled) {
    left_key_nav->setVisible(enabled);
    key_addremove->setVisible(enabled);
    right_key_nav->setVisible(enabled);
}
