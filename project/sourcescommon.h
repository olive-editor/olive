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

#ifndef SOURCESCOMMON_H
#define SOURCESCOMMON_H

#include <QModelIndexList>
#include <QTimer>
#include <QVector>

#include "project/footage.h"

class Project;
class QMouseEvent;
class Media;
class QAbstractItemView;
class QDropEvent;

struct Footage;

class SourcesCommon : public QObject {
	Q_OBJECT
public:
	SourcesCommon(Project *parent);
	QAbstractItemView* view;
	void show_context_menu(QWidget* parent, const QModelIndexList &items);

	void mousePressEvent(QMouseEvent* e);
	void mouseDoubleClickEvent(QMouseEvent* e, const QModelIndexList& selected_items);
	void dropEvent(QWidget *parent, QDropEvent* e, const QModelIndex& drop_item, const QModelIndexList &items);

	void item_click(Media* m, const QModelIndex &index);
private slots:
	void create_seq_from_selected();
	void reveal_in_browser();
	void rename_interval();
	void item_renamed(Media *item);

	// proxy functions
	void open_create_proxy_dialog();
	void clear_proxies_from_selected();
private:
	Media* editing_item;
	QModelIndex editing_index;
	QModelIndexList selected_items;
	Project* project_parent;
	void stop_rename_timer();
	QTimer rename_timer;

	// we cache the selected footage items for open_create_proxy_dialog()
    QVector<FootagePtr> cached_selected_footage;
};

#endif // SOURCESCOMMON_H
