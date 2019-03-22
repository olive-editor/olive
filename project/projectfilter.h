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

#ifndef PROJECTFILTER_H
#define PROJECTFILTER_H

#include <QSortFilterProxyModel>

class ProjectFilter : public QSortFilterProxyModel {
  Q_OBJECT
public:
  ProjectFilter(QObject *parent = nullptr);

  // are sequences visible
  bool get_show_sequences();

public slots:

  // set whether sequences are visible
  void set_show_sequences(bool b);

  // update search filter
  void update_search_filter(const QString& s);

protected:

  // function that filters whether rows are displayed or not
  virtual bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;

private:

  // internal variable for whether to show sequences
  bool show_sequences;

  // search filter variable
  QString search_filter;

};

#endif // PROJECTFILTER_H
