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

#ifndef SOURCEICONVIEW_H
#define SOURCEICONVIEW_H

#include <QListView>
#include <QDropEvent>

class Project;

class SourceIconView : public QListView {
    Q_OBJECT
public:
    SourceIconView(QWidget* parent = 0);
    Project* project_parent;

    void mousePressEvent(QMouseEvent* event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dropEvent(QDropEvent* event);
signals:
    void changed_root();
private slots:
    void show_context_menu();
    void item_click(const QModelIndex& index);
};

#endif // SOURCEICONVIEW_H
