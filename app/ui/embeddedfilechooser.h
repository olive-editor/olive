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

#ifndef EMBEDDEDFILECHOOSER_H
#define EMBEDDEDFILECHOOSER_H

#include <QWidget>

class QLabel;

class EmbeddedFileChooser : public QWidget {
	Q_OBJECT
public:
	EmbeddedFileChooser(QWidget* parent = 0);

	const QString& getFilename();
	const QString& getPreviousValue();
	void setFilename(const QString& s);
signals:
	void changed();
private:
	QLabel* file_label;
	QString filename;
	QString old_filename;
	void update_label();
private slots:
	void browse();
};

#endif // EMBEDDEDFILECHOOSER_H
