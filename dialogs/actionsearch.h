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

#ifndef ACTIONSEARCH_H
#define ACTIONSEARCH_H

#include <QDialog>
#include <QLineEdit>
#include <QListWidget>

class QListWidget;
class QMenu;

class ActionSearchList : public QListWidget {
	Q_OBJECT
public:
	ActionSearchList(QWidget* parent);
protected:
	void mouseDoubleClickEvent(QMouseEvent *event);
signals:
	void dbl_click();
};

class ActionSearch : public QDialog
{
	Q_OBJECT
public:
	ActionSearch(QWidget* parent = nullptr);
private slots:
	void search_update(const QString& s, const QString &p = nullptr, QMenu *parent = nullptr);
	void perform_action();
	void move_selection_up();
	void move_selection_down();
private:
	ActionSearchList* list_widget;
};

class ActionSearchEntry : public QLineEdit {
	Q_OBJECT
public:
	ActionSearchEntry(QWidget* parent);
protected:
	void keyPressEvent(QKeyEvent * event);
signals:
	void moveSelectionUp();
	void moveSelectionDown();
};

#endif // ACTIONSEARCH_H
