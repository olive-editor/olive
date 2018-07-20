#ifndef COLORBUTTON_H
#define COLORBUTTON_H

#include <QPushButton>
#include <QColor>
#include <QUndoCommand>

class ColorButton : public QPushButton {
    Q_OBJECT
public:
    ColorButton(QWidget* parent = 0);
    QColor get_color();
    void set_color(QColor c);
private:
    QColor color;
    void set_button_color();
signals:
    void color_changed();
private slots:
    void open_dialog();
};

class ColorCommand : public QUndoCommand {
public:
    ColorCommand(ColorButton* s, QColor o, QColor n);
    void undo();
    void redo();
private:
    ColorButton* sender;
    QColor old_color;
    QColor new_color;
};

#endif // COLORBUTTON_H
