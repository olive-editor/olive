#include "debugdialog.h"

#include <QTextEdit>
#include <QVBoxLayout>
#include <QScrollBar>

#include "debug.h"

DebugDialog* Olive::DebugDialog = nullptr;

DebugDialog::DebugDialog(QWidget *parent) : QDialog(parent) {
	setWindowTitle(tr("Debug Log"));

	QVBoxLayout* layout = new QVBoxLayout(this);

	textEdit = new QTextEdit(this);
	textEdit->setWordWrapMode(QTextOption::NoWrap);
	layout->addWidget(textEdit);
}

void DebugDialog::update_log() {
	textEdit->setHtml(get_debug_str());
	textEdit->verticalScrollBar()->setValue(textEdit->verticalScrollBar()->maximum());
}

void DebugDialog::showEvent(QShowEvent *) {
	update_log();
}
