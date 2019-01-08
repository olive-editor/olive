#include "embeddedfilechooser.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QFileInfo>
#include <QPushButton>
#include <QFileDialog>

EmbeddedFileChooser::EmbeddedFileChooser(QWidget* parent) : QWidget(parent) {
	QHBoxLayout* layout = new QHBoxLayout();
	layout->setMargin(0);
	setLayout(layout);
	file_label = new QLabel();
	update_label();
	layout->addWidget(file_label);
	QPushButton* browse_button = new QPushButton("...");
	browse_button->setFixedWidth(25);
	layout->addWidget(browse_button);
	connect(browse_button, SIGNAL(clicked(bool)), this, SLOT(browse()));
}

const QString &EmbeddedFileChooser::getFilename() {
	return filename;
}

const QString &EmbeddedFileChooser::getPreviousValue() {
	return old_filename;
}

void EmbeddedFileChooser::setFilename(const QString &s) {
	old_filename = filename;
	filename = s;
	update_label();
	emit changed();
}

void EmbeddedFileChooser::update_label() {
	QString l = "<html>File: ";
	if (filename.isEmpty()) {
		l += "(none)";
	} else {
		bool file_exists = QFileInfo::exists(filename);
		if (!file_exists) l += "<font color='red'>";
		QString short_fn = filename.mid(filename.lastIndexOf('/')+1);
		if (short_fn.size() > 20) {
			l += "..." + short_fn.right(20);
		} else {
			l += short_fn;
		}
		if (!file_exists) l += "</font>";
	}
	l += "</html>";
	file_label->setText(l);
}

void EmbeddedFileChooser::browse() {
	QString fn = QFileDialog::getOpenFileName(this);
	if (!fn.isEmpty()) {
		setFilename(fn);
	}
}
