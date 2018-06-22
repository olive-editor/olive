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

    void on_actionCu_t_triggered();

    void on_actionCop_y_triggered();

    void on_action_Paste_triggered();

    void on_action_Save_Project_triggered();

    void on_action_Open_Project_triggered();

    void on_actionProject_triggered();

    void on_actionSave_Project_As_triggered();

    void on_actionDeselect_All_triggered();

    void on_actionGo_to_start_triggered();

    void on_actionReset_to_default_layout_triggered();

    void on_actionPrevious_Frame_triggered();

    void on_actionNext_Frame_triggered();

    void on_actionGo_to_End_triggered();

    void on_actionPlay_Pause_triggered();

    void on_actionCrash_triggered();

    void on_actionEdit_Tool_triggered();

    void on_actionToggle_Snapping_triggered();

    void on_actionPointer_Tool_triggered();

    void on_actionRazor_Tool_triggered();

    void on_actionRipple_Tool_triggered();

    void on_actionRolling_Tool_triggered();

    void on_actionSlip_Tool_triggered();

    void on_actionGo_to_Previous_Cut_triggered();

    void on_actionGo_to_Next_Cut_triggered();

    void autorecover_interval();

    void on_actionEdit_Tool_Also_Seeks_triggered(bool checked);

    void on_actionPreferences_triggered();

private:
	Ui::MainWindow *ui;
	void setup_layout();
    bool save_project_as();
    bool save_project();
    bool can_close_project();
};

#endif // MAINWINDOW_H
