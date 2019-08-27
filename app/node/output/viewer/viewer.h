/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

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

#ifndef VIEWER_H
#define VIEWER_H

#include "node/node.h"
#include "panel/viewer/viewer.h"

/**
 * @brief A bridge between a node system and a ViewerPanel
 *
 * Receives update/time change signals from ViewerPanels and responds by sending them a texture of that frame
 */
class ViewerOutput : public Node
{
  Q_OBJECT
public:
  ViewerOutput();

  virtual QString Name() override;
  virtual QString id() override;
  virtual QString Category() override;
  virtual QString Description() override;

  void SetTimebase(const rational& timebase);

  NodeInput* texture_input();

  void AttachViewer(ViewerPanel* viewer);

  virtual void InvalidateCache(const rational &start_range, const rational &end_range) override;
protected:

  virtual void Process() override;

private:
  NodeInput* texture_input_;

  ViewerPanel* attached_viewer_;

  rational timebase_;

private slots:
  void ViewerTimeChanged(const rational& t);

};

#endif // VIEWER_H
