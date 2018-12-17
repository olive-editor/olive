#ifndef LOADDIALOG_H
#define LOADDIALOG_H

#include <QDialog>

class QProgressBar;
struct Sequence;
class Media;
struct Footage;
class QHBoxLayout;
class LoadThread;

class LoadDialog : public QDialog
{
    Q_OBJECT
public:
    LoadDialog(QWidget* parent, bool autorecovery);
private slots:
    void cancel();
    void thread_done();
private:
	QProgressBar* bar;
    QPushButton* cancel_button;
    QHBoxLayout* hboxLayout;
    LoadThread* lt;
};

#endif // LOADDIALOG_H
