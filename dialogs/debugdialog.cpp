#include "debugdialog.h"

#include <QTextEdit>
#include <QVBoxLayout>

#include "debug.h"

DebugDialog* debug_dialog = nullptr;

DebugDialog::DebugDialog(QWidget *parent) : QDialog(parent) {
	setWindowTitle("Debug Log");

	QVBoxLayout* layout = new QVBoxLayout();
	setLayout(layout);

	textEdit = new QTextEdit();
	layout->addWidget(textEdit);
}

void DebugDialog::update_log() {
	textEdit->setHtml(get_debug_str());
}

void DebugDialog::showEvent(QShowEvent *) {
	update_log();
}
