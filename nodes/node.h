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

#ifndef EFFECT_H
#define EFFECT_H

#include <memory>
#include <QObject>
#include <QString>
#include <QVector>
#include <QColor>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QMutex>
#include <QThread>
#include <QLabel>
#include <QWidget>
#include <QGridLayout>
#include <QPushButton>
#include <QMouseEvent>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <random>

#include "timeline/tracktypes.h"
#include "rendering/qopenglshaderprogramptr.h"
#include "inputs.h"
#include "effects/effectgizmo.h"

class EffectGizmo;
class KeyframeDataChange;

class TimelineObject;

class Node;
using NodePtr = std::shared_ptr<Node>;

enum NodeType {
  kTransformEffect,
  kTextInput,
  kSolidInput,
  kNoiseInput,
  kVolumeEffect,
  kPanEffect,
  kToneInput,
  kShakeEffect,
  kTimecodeEffect,
  kMaskEffect,
  kFillLeftRightEffect,
  kVstEffect,
  kCornerPinEffect,
  kRichTextInput,
  kMediaInput,
  kShaderEffect,
  kImageOutput,
  kCrossDissolveTransition,
  kLinearFadeTransition,
  kExponentialFadeTransition,
  kLogarithmicFadeTransition,
  kInvalidNode
};

double log_volume(double linear);

enum EffectType {
  EFFECT_TYPE_INVALID,
  EFFECT_TYPE_EFFECT,
  EFFECT_TYPE_TRANSITION
};

enum EffectKeyframeType {
  EFFECT_KEYFRAME_LINEAR,
  EFFECT_KEYFRAME_BEZIER,
  EFFECT_KEYFRAME_HOLD
};

struct GLTextureCoords {
  QMatrix4x4 matrix;

  QVector3D vertex_top_left;
  QVector3D vertex_top_right;
  QVector3D vertex_bottom_left;
  QVector3D vertex_bottom_right;

  QVector2D texture_top_left;
  QVector2D texture_top_right;
  QVector2D texture_bottom_left;
  QVector2D texture_bottom_right;

  float opacity;
};

class Node : public QObject {
  Q_OBJECT
public:
  Node(TimelineObject *c);
  ~Node();

  TimelineObject* parent_clip;

  virtual QString name() = 0;
  virtual QString id() = 0;
  virtual QString category();
  virtual QString description() = 0;
  virtual EffectType type() = 0;
  virtual olive::TrackType subtype() = 0;
  virtual bool IsCreatable();
  virtual NodePtr Create(TimelineObject *c) = 0;

  void AddRow(EffectRow* row);
  int IndexOfRow(EffectRow* row);
  EffectRow* row(int i);
  int row_count();

  EffectGizmo* add_gizmo(int type);
  EffectGizmo* gizmo(int i);
  int gizmo_count();

  QVector<NodeEdgePtr> GetAllEdges();

  bool IsEnabled();
  bool IsExpanded();

  virtual void refresh();

  virtual NodePtr copy(TimelineObject* c);
  void copy_field_keyframes(NodePtr e);

  virtual void load(QXmlStreamReader& stream);
  virtual void custom_load(QXmlStreamReader& stream);
  virtual void save(QXmlStreamWriter& stream);

  void load_from_string(const QByteArray &s);
  QByteArray save_to_string();

  // glsl handling
  bool is_open();
  void open();
  void close();
  bool is_shader_linked();
  QOpenGLShaderProgram* GetShaderPipeline();

  enum VideoEffectFlags {
    ShaderFlag        = 0x1,
    CoordsFlag        = 0x2,
    SuperimposeFlag   = 0x4
  };
  int Flags();
  void SetFlags(int flags);

  int getIterations();
  void setIterations(int i);

  const QPointF& pos();

  virtual void process_image(double timecode, uint8_t* input, uint8_t* output, int size);
  virtual void process_shader(double timecode, GLTextureCoords&, int iteration);
  virtual void process_coords(double timecode, GLTextureCoords& coords, int data);
  virtual GLuint process_superimpose(QOpenGLContext *ctx, double timecode);
  virtual void process_audio(double timecode_start, double timecode_end, float **samples, int nb_samples, int nb_channels, int type);

  virtual void gizmo_draw(double timecode, GLTextureCoords& coords);
  void gizmo_move(EffectGizmo* sender, int x_movement, int y_movement, double timecode, bool done);
  void gizmo_world_to_screen(const QMatrix4x4 &matrix, const QMatrix4x4 &projection);
  bool are_gizmos_enabled();

  /**
   * @brief Get the current clip/media time
   *
   * A convenience function that can be plugged into GetValueAt() to get the value wherever the appropriate Sequence's
   * playhead it.
   *
   * @return
   *
   * Current clip/media time in seconds.
   */
  double Now();

  /**
   * @brief Retrieve the current clip as a frame number
   *
   * Same as Now() but retrieves the value as a frame number (in the appropriate Sequence's frame rate) instead of
   * seconds.
   *
   * @return
   *
   * The current clip time in frames
   */
  long NowInFrames();

  template <typename T>
  T randomNumber()
  {
    static std::random_device device;
    static std::mt19937 generator(device());
    static std::uniform_int_distribution<> distribution(std::numeric_limits<T>::min(), std::numeric_limits<T>::max());
    return distribution(generator);
  }

  template <typename T>
  T randomFloat()
  {
    static std::random_device device;
    static std::mt19937 generator(device());
    static std::uniform_int_distribution<> distribution(-1.0, 1.0);
    return distribution(generator);
  }

public slots:
  void FieldChanged();
  void SetEnabled(bool b);
  void SetExpanded(bool e);
  void SetPos(const QPointF& pos);
signals:
  void EnabledChanged(bool);
private slots:
  void delete_self();
  void move_up();
  void move_down();
  void save_to_file();
  void load_from_file();
protected:
  // glsl effect
  QOpenGLShaderProgramPtr shader_program_;
  QString shader_vert_path_;
  QString shader_frag_path_;
  QString shader_function_name_;

  // superimpose effect
  QImage img;
  GLuint texture;
  QOpenGLContext* texture_ctx;
  int tex_width_;
  int tex_height_;

  // enable effect to update constantly
  virtual bool AlwaysUpdate();

private:
  bool isOpen;
  QVector<EffectRow*> rows;
  QVector<EffectGizmo*> gizmos;
  bool bound;
  int iterations;

  bool enabled_;
  bool expanded_;

  int flags_;

  QVector<KeyframeDataChange*> gizmo_dragging_actions_;

  QPointF pos_;

  // superimpose functions
  virtual void redraw(double timecode);
  bool valueHasChanged(double timecode);
  QVector<QVariant> cachedValues;
  void delete_texture();
  void validate_meta_path();
};

namespace olive {
  extern QVector<NodePtr> node_library;
}

#endif // EFFECT_H
