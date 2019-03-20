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

#ifndef PANEL_H
#define PANEL_H

#include <QDockWidget>

class Panel : public QDockWidget {
  Q_OBJECT
public:
  Panel(QWidget* parent = nullptr);
  virtual ~Panel() override;

  virtual void Retranslate() = 0;

  virtual void LoadLayoutState(const QByteArray& data);
  virtual QByteArray SaveLayoutState();
protected:
  virtual void changeEvent(QEvent* e) override;
};

namespace olive {
  extern QVector<Panel*> panels;
}

#endif // PANEL_H
