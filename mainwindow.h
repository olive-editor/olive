#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class Project;
class EffectControls;
class Viewer;
class Timeline;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

private slots:
	void on_action_Import_triggered();

	void on_actionExit_triggered();

	void on_actionAbout_triggered();

	void on_actionDelete_triggered();

	void on_actionSelect_All_triggered();

	void on_actionSequence_triggered();

	void on_actionZoom_In_triggered();

	void on_actionZoom_out_triggered();

	void on_actionTimeline_Track_Lines_toggled(bool arg1);

	void on_actionExport_triggered();

	void on_actionProject_2_toggled(bool arg1);

	void on_actionEffect_Controls_toggled(bool arg1);

	void on_actionViewer_toggled(bool arg1);

	void on_actionTimeline_toggled(bool arg1);

	void on_actionRipple_Delete_triggered();

    void on_action_Undo_triggered();

    void on_action_Redo_triggered();

    void on_actionSplit_at_Playhead_triggered();

private:
	Ui::MainWindow *ui;
	void setup_layout();
};

#endif // MAINWINDOW_H
