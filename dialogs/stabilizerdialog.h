/* 
 * Olive. Olive is a free non-linear video editor for Windows, macOS, and Linux.
 * Copyright (C) 2018  {{ organization }}
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 *along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
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
