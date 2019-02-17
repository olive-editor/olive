/***

    Olive - Non-Linear Video Editor
    Copyright (C) 2019  Olive Team

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#ifndef MEDIAPROPERTIESDIALOG_H
#define MEDIAPROPERTIESDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QListWidget>
#include <QDoubleSpinBox>
#include <QCheckBox>

#include "project/footage.h"
#include "project/media.h"

class MediaPropertiesDialog : public QDialog {
	Q_OBJECT
public:
    MediaPropertiesDialog(QWidget *parent, Media* i);
private:
	QComboBox* interlacing_box;
	QLineEdit* name_box;
    Media* item;
	QListWidget* track_list;
	QDoubleSpinBox* conform_fr;
	QCheckBox* premultiply_alpha_setting;
private slots:
	void accept();
};

#endif // MEDIAPROPERTIESDIALOG_H
