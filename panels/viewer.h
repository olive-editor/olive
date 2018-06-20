#ifndef VIEWER_H
#define VIEWER_H

#include <QDockWidget>

class Timeline;
class ViewerWidget;
struct Sequence;

namespace Ui {
class Viewer;
}

class Viewer : public QDockWidget
{
	Q_OBJECT

public:
	explicit Viewer(QWidget *parent = 0);
	~Viewer();
    void update_sequence();
	void compose();
    void set_playpause_icon(bool play);

	ViewerWidget* viewer_widget;

private slots:
	void on_pushButton_clicked();

	void on_pushButton_5_clicked();

	void on_pushButton_2_clicked();

	void on_pushButton_4_clicked();

    void on_pushButton_3_clicked();

private:
	Ui::Viewer *ui;
};

#endif // VIEWER_H
