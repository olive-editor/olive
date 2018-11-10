#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class Project;
class EffectControls;
class Viewer;
class Timeline;

extern QMainWindow* mainWindow;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
	void undo();
	void redo();
	void openSpeedDialog();
	void cut();
	void copy();
	void paste();

protected:
	void closeEvent(QCloseEvent *);
	void paintEvent(QPaintEvent *event);

private slots:
	void on_action_Import_triggered();

	void on_actionExit_triggered();

	void on_actionAbout_triggered();

	void on_actionDelete_triggered();

	void on_actionSelect_All_triggered();

	void on_actionSequence_triggered();

	void on_actionZoom_In_triggered();

    void on_actionZoom_out_triggered();

	void on_actionExport_triggered();

	void on_actionProject_2_toggled(bool arg1);

	void on_actionEffect_Controls_toggled(bool arg1);

	void on_actionViewer_toggled(bool arg1);

	void on_actionTimeline_toggled(bool arg1);

	void on_actionRipple_Delete_triggered();

	void on_actionSplit_at_Playhead_triggered();

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

    void on_actionPreferences_triggered();

    void on_actionIncrease_Track_Height_triggered();

    void on_actionDecrease_Track_Height_triggered();

    void windowMenu_About_To_Be_Shown();

    void on_actionFrames_triggered();

    void on_actionDrop_Frame_triggered();

    void on_actionNon_Drop_Frame_triggered();

    void viewMenu_About_To_Be_Shown();

    void on_actionEdit_Tool_Selects_Links_triggered();

    void on_actionEdit_Tool_Also_Seeks_triggered();

    void toolMenu_About_To_Be_Shown();

    void on_actionDuplicate_triggered();

    void on_actionSelecting_Also_Seeks_triggered();

    void on_actionSeek_to_the_End_of_Pastes_triggered();

    void on_actionAdd_Default_Transition_triggered();

    void on_actionSlide_Tool_triggered();

    void on_actionFolder_triggered();

    void editMenu_About_To_Be_Shown();

    void fileMenu_About_To_Be_Shown();

    void load_recent_project();

    void on_actionScroll_Wheel_Zooms_triggered();

    void on_actionLink_Unlink_triggered();

    void on_actionRipple_To_In_Point_triggered();

    void on_actionRipple_to_Out_Point_triggered();

    void on_actionSet_In_Point_triggered();

    void on_actionSet_Out_Point_triggered();

    void on_actionClear_In_Out_triggered();

    void on_actionDelete_In_Out_triggered();

    void on_actionRipple_Delete_In_Out_triggered();

    void on_actionTimeline_Track_Lines_triggered();

    void on_actionRectified_Waveforms_triggered();

    void on_actionDefault_triggered();

    void on_actionOff_triggered();

    void on_action4_3_triggered();

    void on_action16_9_triggered();

    void on_actionCustom_triggered();

	void on_actionEnable_Drag_Files_to_Timeline_triggered();

    void on_actionAuto_scale_by_Default_triggered();

	void on_actionSet_Edit_Marker_triggered();

	void on_actionEnable_Disable_Clip_triggered();

	void on_actionEnable_Seek_to_Import_triggered();

    void on_actionAudio_Scrubbing_triggered();

	void on_actionTransition_Tool_triggered();

	void on_actionEdit_to_In_Point_triggered();

	void on_actionEdit_to_Out_Point_triggered();

	void on_actionNest_triggered();

private:
	Ui::MainWindow *ui;
	void setup_layout();
    bool save_project_as();
    bool save_project();
    bool can_close_project();
	void updateTitle(const QString &url);
};

#endif // MAINWINDOW_H
