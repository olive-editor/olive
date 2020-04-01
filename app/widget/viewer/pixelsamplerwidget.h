#ifndef PIXELSAMPLERWIDGET_H
#define PIXELSAMPLERWIDGET_H

#include <QLabel>
#include <QGroupBox>
#include <QWidget>

#include "render/color.h"

class PixelSamplerWidget : public QGroupBox
{
  Q_OBJECT
public:
  PixelSamplerWidget(QWidget* parent = nullptr);

public slots:
  void SetValues(const Color& color);

private:
  void UpdateLabelInternal();

  Color color_;

  QLabel* label_;

};

class ManagedPixelSamplerWidget : public QWidget
{
  Q_OBJECT
public:
  ManagedPixelSamplerWidget(QWidget* parent = nullptr);

public slots:
  void SetValues(const Color& reference, const Color& display);

private:
  PixelSamplerWidget* reference_view_;

  PixelSamplerWidget* display_view_;

};

#endif // PIXELSAMPLERWIDGET_H
