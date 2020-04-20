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

/*
 * Node Styles
 *
 * To add a style for a given node, it involves several steps.
 *
 * 1. Introduce a Node Style class. This must inherit from the
 *    NodeDefaultStyleWidget class.
 * 2. Name the Node such that it matches the layout of the
 *    Olive directory structure in which it is found. That is,
 *    it should be Node[SubDirectory][Subdirectory][Etc]Style.
 *    For example, if the node directory is node - block - gap,
 *    the class should be NodeBlockGapStyle.
 * 3. Copy the relevant stanzas that follow, and hook up any
 *    relevant Q_PROPERTY values to their respective functions.
 * 4. In the NodeStylesRegistry class that follows, add a single
 *    instance of the relevant newly created class. It should follow
 *    the Olive naming convention of underscores and lowercase,
 *    and match the class definition without the Node prefix. For
 *    example, NodeInputMediaVideoStyle becomes input_media_video_style_.
 *    Note the trailing underscore convention.
 * 5. Register the key and value pair in nodeviewitemwidgetproxy.cpp.
 *    The string must be identical to the Node class identifier given
 *    in the relevant source code file.
 * 6. Add the relevant stylings to the Olive light and dark themes
 *    located in style.css for each. The name must match the node
 *    style class, such as NodeInputMediaVideoStyle class.
 *
 */

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

class NodeAudioPanStyle : public NodeDefaultStyleWidget {
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

class NodeBlockGapStyle : public NodeDefaultStyleWidget {
  Q_OBJECT
  Q_PROPERTY(QColor titlebarColor READ TitleBarColor WRITE SetTitleBarColor DESIGNABLE true)
  Q_PROPERTY(QColor borderColor READ BorderColor WRITE SetBorderColor DESIGNABLE true)
};

class NodeMathMathStyle : public NodeDefaultStyleWidget {
  Q_OBJECT
  Q_PROPERTY(QColor titlebarColor READ TitleBarColor WRITE SetTitleBarColor DESIGNABLE true)
  Q_PROPERTY(QColor borderColor READ BorderColor WRITE SetBorderColor DESIGNABLE true)
};

class NodeMathTrigonometryStyle : public NodeDefaultStyleWidget {
  Q_OBJECT
  Q_PROPERTY(QColor titlebarColor READ TitleBarColor WRITE SetTitleBarColor DESIGNABLE true)
  Q_PROPERTY(QColor borderColor READ BorderColor WRITE SetBorderColor DESIGNABLE true)
};

class NodeGeneratorSolidStyle : public NodeDefaultStyleWidget {
  Q_OBJECT
  Q_PROPERTY(QColor titlebarColor READ TitleBarColor WRITE SetTitleBarColor DESIGNABLE true)
  Q_PROPERTY(QColor borderColor READ BorderColor WRITE SetBorderColor DESIGNABLE true)
};

class NodeManipulationMergeStyle : public NodeDefaultStyleWidget {
  Q_OBJECT
  Q_PROPERTY(QColor titlebarColor READ TitleBarColor WRITE SetTitleBarColor DESIGNABLE true)
  Q_PROPERTY(QColor borderColor READ BorderColor WRITE SetBorderColor DESIGNABLE true)
};

class NodeConvolutionBlurStyle : public NodeDefaultStyleWidget {
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
  NodeAudioPanStyle audio_pan_style_;
  NodeBlockClipStyle block_clip_style_;
  NodeBlockGapStyle block_gap_style_;
  NodeMathMathStyle math_math_style_;
  NodeMathTrigonometryStyle math_trigonometry_style_;
  NodeGeneratorSolidStyle generator_solid_style_;
  NodeManipulationMergeStyle manipulation_merge_style_;
  NodeConvolutionBlurStyle convolution_blur_style_;

  QMap<QString, NodeDefaultStyleWidget*> registry_;
};

#endif // NODEVIEWITEMWIDGETPROXY_H
