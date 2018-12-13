#ifndef LOADDIALOG_H
#define LOADDIALOG_H

#include <QDialog>

class QProgressBar;
class Sequence;
class Media;
class Footage;
class QHBoxLayout;

class LoadDialog : public QDialog
{
    Q_OBJECT
public:
    LoadDialog(QWidget* parent, bool autorecovery);
private slots:
    void thread_done();
private:
	QProgressBar* bar;
    QPushButton* cancel_button;
    QHBoxLayout* hboxLayout;
};

#endif // LOADDIALOG_H
