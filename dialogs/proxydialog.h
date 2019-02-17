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

#ifndef PROXYDIALOG_H
#define PROXYDIALOG_H

#include <QDialog>
#include <QVector>
#include <QComboBox>

#include "project/footage.h"

class ProxyDialog : public QDialog {
	Q_OBJECT
public:
    ProxyDialog(QWidget* parent, const QVector<FootagePtr>& footage);
public slots:
	// called if user clicks "OK" on the dialog
	virtual void accept() override;
private:
	// user's desired dimensions
	QComboBox* size_combobox;

	// user's desired proxy format
	QComboBox* format_combobox;

	// allows users to set the location to store proxies
	QComboBox* location_combobox;

	// stores the custom location to store proxies if the user sets a custom location
	QString custom_location;

	// stores the subdirectory to be made next to the source (dependent on the user's language)
	QString proxy_folder_name;

	// list of footage to make proxies for
    QVector<FootagePtr> selected_footage;
private slots:
	// triggered when the user changes the index in the location combobox
	void location_changed(int i);
};

#endif // PROXYDIALOG_H
