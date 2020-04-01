#ifndef COLORDIALOG_H
#define COLORDIALOG_H

#include <QDialog>

#include "render/color.h"
#include "render/colormanager.h"

class ColorDialog : public QDialog
{
public:
  ColorDialog(ColorManager* color_manager, QWidget* parent = nullptr);

private:
  ColorManager* color_manager_;

};

#endif // COLORDIALOG_H
