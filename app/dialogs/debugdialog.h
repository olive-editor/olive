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

#ifndef DEBUGDIALOG_H
#define DEBUGDIALOG_H

#include <QDialog>
class QTextEdit;

class DebugDialog : public QDialog {
	Q_OBJECT
public:
	DebugDialog(QWidget* parent = 0);
public slots:
	void update_log();
protected:
	void showEvent(QShowEvent* event);
private:
	QTextEdit* textEdit;
};

namespace Olive {
    extern DebugDialog* DebugDialog;
}

#endif // DEBUGDIALOG_H
