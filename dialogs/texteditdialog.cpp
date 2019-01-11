#include "texteditdialog.h"

#include <QVBoxLayout>
#include <QPlainTextEdit>
#include <QDialogButtonBox>

TextEditDialog::TextEditDialog(QWidget *parent, const QString &s) :
	QDialog(parent)
{
    setWindowTitle(tr("Edit Text"));

	QVBoxLayout* layout = new QVBoxLayout();
	setLayout(layout);

	textEdit = new QPlainTextEdit();
	textEdit->setPlainText(s);
	layout->addWidget(textEdit);

	QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	layout->addWidget(buttons);
	connect(buttons, SIGNAL(accepted()), this, SLOT(save()));
	connect(buttons, SIGNAL(rejected()), this, SLOT(cancel()));
}

const QString& TextEditDialog::get_string() {
	return result_str;
}

void TextEditDialog::save() {
	result_str = textEdit->toPlainText();
	accept();
}

void TextEditDialog::cancel() {
	reject();
}
