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
#include "render/rendertexture.h"

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

  const rational& Timebase();
  void SetTimebase(const rational& timebase);

  NodeInput* texture_input();
  NodeInput* samples_input();
  NodeInput* length_input();

  RenderTexturePtr GetTexture(const rational& time);
  QByteArray GetSamples(const rational& in, const rational& out);

  virtual void InvalidateCache(const rational &start_range, const rational &end_range, NodeInput *from = nullptr) override;

  void SetViewerSize(const int& width, const int& height);

  const int& ViewerWidth();
  const int& ViewerHeight();

  rational Length();

signals:
  void TimebaseChanged(const rational&);

  void TextureChangedBetween(const rational&, const rational&);

  void SizeChanged(int width, int height);

protected:
  virtual QVariant Value(NodeOutput* output, const rational &in, const rational &out) override;

private:
  NodeInput* texture_input_;

  NodeInput* samples_input_;

  NodeInput* length_input_;

  rational timebase_;

  int viewer_width_;

  int viewer_height_;

};

#endif // VIEWER_H
