#ifndef KEYFRAMENAVIGATOR_H
#define KEYFRAMENAVIGATOR_H

#include <QWidget>

class QHBoxLayout;
class QPushButton;

class KeyframeNavigator : public QWidget
{
    Q_OBJECT
public:
    KeyframeNavigator(QWidget* parent = 0);
    void enable_keyframes(bool);
signals:
    void goto_previous_key();
    void toggle_key();
    void goto_next_key();
    void set_keyframe_enabled(bool);
private slots:
    void keyframe_ui_enabled(bool);
private:
    QHBoxLayout* key_controls;
    QPushButton* left_key_nav;
    QPushButton* key_addremove;
    QPushButton* right_key_nav;
    QPushButton* keyframe_enable;
};

#endif // KEYFRAMENAVIGATOR_H
