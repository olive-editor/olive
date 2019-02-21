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

#ifndef PROJECTMODEL_H
#define PROJECTMODEL_H

#include <QAbstractItemModel>

#include "project/media.h"

class ProjectModel : public QAbstractItemModel
{
	Q_OBJECT
public:
	ProjectModel(QObject* parent = nullptr);
	~ProjectModel() override;

	void make_root();
	void destroy_root();
	void clear();
    Media* get_root();
	QVariant data(const QModelIndex &index, int role) const override;
	Qt::ItemFlags flags(const QModelIndex &index) const override;
	QVariant headerData(int section, Qt::Orientation orientation,
						int role = Qt::DisplayRole) const override;
	QModelIndex index(int row, int column,
					  const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex create_index(int arow, int acolumn, void *adata);
	QModelIndex parent(const QModelIndex &index) const override;
	bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    Media* getItem(const QModelIndex &index) const;

    void appendChild(Media* parent, Media* child);
    void moveChild(Media* child, Media* to);
    void removeChild(Media* parent, Media* m);
    Media* child(int i, Media* parent = nullptr);
    int childCount(Media* parent = nullptr);
    void set_icon(Media* m, const QIcon &ico);

private:
    Media* root_item;
};

namespace olive {
    extern ProjectModel project_model;
}

#endif // PROJECTMODEL_H
