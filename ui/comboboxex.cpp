#include "comboboxex.h"

#include "project/undo.h"

#include <QUndoCommand>
#include <QWheelEvent>

class ComboBoxExCommand : public QUndoCommand {
public:
    ComboBoxExCommand(ComboBoxEx* obj, int old_index, int new_index) :
        combobox(obj), old_val(old_index), new_val(new_index), done(true) {}
    void undo() {
        combobox->setCurrentIndex(old_val);
        done = false;
    }
    void redo() {
        if (!done) {
            combobox->setCurrentIndex(new_val);
        }
    }
private:
    ComboBoxEx* combobox;
    int old_val;
    int new_val;
    bool done;
};

ComboBoxEx::ComboBoxEx(QWidget *parent) : QComboBox(parent), index(0) {
    connect(this, SIGNAL(activated(int)), this, SLOT(index_changed(int)));
}

void ComboBoxEx::index_changed(int i) {
    if (index != i) {
        undo_stack.push(new ComboBoxExCommand(this, index, i));
        index = i;
    }
}

void ComboBoxEx::wheelEvent(QWheelEvent* e) {
    e->ignore();
}
