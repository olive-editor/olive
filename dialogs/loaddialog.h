#ifndef LOADDIALOG_H
#define LOADDIALOG_H

#include <QDialog>

class QProgressBar;

class LoadDialog : public QDialog
{
public:
	LoadDialog(QWidget* parent = 0);
private:
	QProgressBar* bar;
};

#endif // LOADDIALOG_H
