#ifndef STABILIZERDIALOG_H
#define STABILIZERDIALOG_H

class QVBoxLayout;
class QCheckBox;
class QDialogButtonBox;
class QGroupBox;
class QGridLayout;
class LabelSlider;

#include <QDialog>

class StabilizerDialog : public QDialog
{
    Q_OBJECT
public:
    StabilizerDialog(QWidget* parent = 0);
private slots:
    void set_all_enabled(bool e);
private:
    QVBoxLayout* layout;
    QCheckBox* enable_stab;
    QDialogButtonBox* buttons;
    QGroupBox* analysis;
    QGridLayout* analysis_layout;
    LabelSlider* shakiness_slider;
    LabelSlider* accuracy_slider;
    LabelSlider* stepsize_slider;
    LabelSlider* mincontrast_slider;
    QCheckBox* tripod_mode_box;
    QGroupBox* stabilization;
    QGridLayout* stabilization_layout;
    LabelSlider* smoothing_slider;
    QCheckBox* gaussian_motion;
};

#endif // STABILIZERDIALOG_H
