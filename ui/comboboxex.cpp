#include "comboboxex.h"

#include "project/undo.h"
#include "panels/project.h"

#include <QUndoCommand>
#include <QWheelEvent>

class ComboBoxExCommand : public QUndoCommand {
public:
    ComboBoxExCommand(ComboBoxEx* obj, int old_index, int new_index) :
		combobox(obj), old_val(old_index), new_val(new_index), done(true), old_project_changed(project_changed) {}
    void undo() {
        combobox->setCurrentIndex(old_val);
        done = false;
		project_changed = old_project_changed;
    }
    void redo() {
        if (!done) {
            combobox->setCurrentIndex(new_val);
        }
		project_changed = true;
    }
private:
    ComboBoxEx* combobox;
    int old_val;
    int new_val;
    bool done;
	bool old_project_changed;
};

ComboBoxEx::ComboBoxEx(QWidget *parent) : QComboBox(parent), index(0) {
    connect(this, SIGNAL(activated(int)), this, SLOT(index_changed(int)));
}

void ComboBoxEx::setCurrentIndexEx(int i) {
	index = i;
	setCurrentIndex(i);
}

void ComboBoxEx::setCurrentTextEx(const QString &text) {
	setCurrentText(text);
	index = currentIndex();
}

void ComboBoxEx::index_changed(int i) {
	if (index != i) {
//		undo_stack.push(new ComboBoxExCommand(this, index, i));
		index = i;
	}
}

void ComboBoxEx::wheelEvent(QWheelEvent* e) {
    e->ignore();
}
