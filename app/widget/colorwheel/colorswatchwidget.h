#ifndef COLORSWATCHWIDGET_H
#define COLORSWATCHWIDGET_H

#include <QOpenGLWidget>

#include "render/backend/opengl/openglshader.h"
#include "render/color.h"

class ColorSwatchWidget : public QWidget
{
  Q_OBJECT
public:
  ColorSwatchWidget(QWidget* parent = nullptr);

  const Color& GetSelectedColor() const;

public slots:
  void SetSelectedColor(const Color& c);

signals:
  void SelectedColorChanged(const Color& c);

protected:
  virtual void mousePressEvent(QMouseEvent* e) override;

  virtual void mouseMoveEvent(QMouseEvent* e) override;

  virtual Color GetColorFromScreenPos(const QPoint& p) const = 0;

  virtual void SelectedColorChangedEvent(const Color& c);

  Qt::GlobalColor GetUISelectorColor() const;

private:
  Color selected_color_;

};

#endif // COLORSWATCHWIDGET_H
