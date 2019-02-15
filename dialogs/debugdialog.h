#ifndef DEBUGDIALOG_H
#define DEBUGDIALOG_H

#include <QDialog>
class QTextEdit;

class DebugDialog : public QDialog {
	Q_OBJECT
public:
	DebugDialog(QWidget* parent = 0);
public slots:
	void update_log();
protected:
	void showEvent(QShowEvent* event);
private:
	QTextEdit* textEdit;
};

namespace Olive {
    extern DebugDialog* DebugDialog;
}

#endif // DEBUGDIALOG_H
