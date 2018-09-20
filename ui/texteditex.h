#ifndef TEXTEDITEX_H
#define TEXTEDITEX_H

#include <QTextEdit>

class TextEditEx : public QTextEdit {
	Q_OBJECT
public:
	TextEditEx(QWidget* parent = 0);
	void setPlainTextEx(const QString &text);
	const QString& getPreviousValue();
private slots:
	void updateInternals();
private:
	QString previousText;
	QString text;
};

#endif // TEXTEDITEX_H
