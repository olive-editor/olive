#include "checkboxex.h"

#include "project/undo.h"

CheckboxEx::CheckboxEx(QWidget* parent) : QCheckBox(parent) {
//	connect(this, SIGNAL(clicked(bool)), this, SLOT(checkbox_command()));
}

void CheckboxEx::checkbox_command() {
	CheckboxCommand* c = new CheckboxCommand(this);
	Olive::UndoStack.push(c);
}
