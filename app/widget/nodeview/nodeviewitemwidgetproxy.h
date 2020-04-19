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

#ifndef NODEVIEWITEMWIDGETPROXY_H
#define NODEVIEWITEMWIDGETPROXY_H

#include <QWidget>
#include <QMap>

class NodeStylesRegistry;

/**
 * @brief A proxy object to allow NodeViewItem access to CSS functions
 *
 * QGraphicsItems can't take Q_PROPERTYs for CSS stylesheet input, but QWidgets can. This is a hack to allow CSS
 * properties to be set from CSS and then read by NodeViewItem.
 */
class NodeDefaultStyleWidget : public QWidget {
  Q_OBJECT
  Q_PROPERTY(QColor titlebarColor READ TitleBarColor WRITE SetTitleBarColor DESIGNABLE true)
  Q_PROPERTY(QColor borderColor READ BorderColor WRITE SetBorderColor DESIGNABLE true)
public:
  // NodeDefaultStyleWidget(QWidget* parent = nullptr);
  NodeDefaultStyleWidget() = default;

  QColor TitleBarColor();
  void SetTitleBarColor(QColor color);

  QColor BorderColor();
  void SetBorderColor(QColor color);

private:
  QColor title_bar_color_;
  QColor border_color_;
};

// Follow directory structure, prefix with Node.
class NodeInputMediaAudioStyle : public NodeDefaultStyleWidget {
  Q_OBJECT
  Q_PROPERTY(QColor titlebarColor READ TitleBarColor WRITE SetTitleBarColor DESIGNABLE true)
  Q_PROPERTY(QColor borderColor READ BorderColor WRITE SetBorderColor DESIGNABLE true)
};

class NodeInputMediaVideoStyle : public NodeDefaultStyleWidget {
  Q_OBJECT
  Q_PROPERTY(QColor titlebarColor READ TitleBarColor WRITE SetTitleBarColor DESIGNABLE true)
  Q_PROPERTY(QColor borderColor READ BorderColor WRITE SetBorderColor DESIGNABLE true)
};

class NodeAudioVolumeStyle : public NodeDefaultStyleWidget {
  Q_OBJECT
  Q_PROPERTY(QColor titlebarColor READ TitleBarColor WRITE SetTitleBarColor DESIGNABLE true)
  Q_PROPERTY(QColor borderColor READ BorderColor WRITE SetBorderColor DESIGNABLE true)
};

class NodeDistortTransformStyle : public NodeDefaultStyleWidget {
  Q_OBJECT
  Q_PROPERTY(QColor titlebarColor READ TitleBarColor WRITE SetTitleBarColor DESIGNABLE true)
  Q_PROPERTY(QColor borderColor READ BorderColor WRITE SetBorderColor DESIGNABLE true)
};

class NodeOutputTrackStyle : public NodeDefaultStyleWidget {
  Q_OBJECT
  Q_PROPERTY(QColor titlebarColor READ TitleBarColor WRITE SetTitleBarColor DESIGNABLE true)
  Q_PROPERTY(QColor borderColor READ BorderColor WRITE SetBorderColor DESIGNABLE true)
};

class NodeOutputViewerStyle : public NodeDefaultStyleWidget {
  Q_OBJECT
  Q_PROPERTY(QColor titlebarColor READ TitleBarColor WRITE SetTitleBarColor DESIGNABLE true)
  Q_PROPERTY(QColor borderColor READ BorderColor WRITE SetBorderColor DESIGNABLE true)
};

class NodeBlockClipStyle : public NodeDefaultStyleWidget {
  Q_OBJECT
  Q_PROPERTY(QColor titlebarColor READ TitleBarColor WRITE SetTitleBarColor DESIGNABLE true)
  Q_PROPERTY(QColor borderColor READ BorderColor WRITE SetBorderColor DESIGNABLE true)
};

class NodeStylesRegistry : public QWidget {
public:
  NodeStylesRegistry(QWidget* parent = nullptr);

  static NodeStylesRegistry* GetRegistry(void);
  NodeDefaultStyleWidget* GetStyle(QString id);
private:
  NodeDefaultStyleWidget default_style_;
  NodeDistortTransformStyle distort_transform_style_;
  NodeInputMediaAudioStyle input_media_audio_style_;
  NodeInputMediaVideoStyle input_media_video_style_;
  NodeOutputTrackStyle output_track_style_;
  NodeOutputViewerStyle output_viewer_style_;
  NodeAudioVolumeStyle audio_volume_style_;
  NodeBlockClipStyle block_clip_style_;

  QMap<QString, NodeDefaultStyleWidget*> registry_;
};

#endif // NODEVIEWITEMWIDGETPROXY_H
