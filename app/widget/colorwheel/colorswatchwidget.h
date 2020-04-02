#ifndef COLORSWATCHWIDGET_H
#define COLORSWATCHWIDGET_H

#include <QOpenGLWidget>

#include "render/backend/opengl/openglshader.h"
#include "render/color.h"
#include "render/colorprocessor.h"

class ColorSwatchWidget : public QWidget
{
  Q_OBJECT
public:
  ColorSwatchWidget(QWidget* parent = nullptr);

  const Color& GetSelectedColor() const;

  void SetColorProcessor(ColorProcessorPtr to_linear, ColorProcessorPtr to_display);

public slots:
  void SetSelectedColor(const Color& c);

signals:
  void SelectedColorChanged(const Color& c);

protected:
  virtual void mousePressEvent(QMouseEvent* e) override;

  virtual void mouseMoveEvent(QMouseEvent* e) override;

  virtual Color GetColorFromScreenPos(const QPoint& p) const = 0;

  virtual void SelectedColorChangedEvent(const Color& c, bool external);

  Qt::GlobalColor GetUISelectorColor() const;

  Color GetManagedColor(const Color& input) const;

private:
  void SetSelectedColorInternal(const Color& c, bool external);

  Color selected_color_;

  ColorProcessorPtr to_linear_processor_;

  ColorProcessorPtr to_display_processor_;

};

#endif // COLORSWATCHWIDGET_H
