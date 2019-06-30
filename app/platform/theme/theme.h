#ifndef _OLIVE_THEME_H_
#define _OLIVE_THEME_H_

#include <string>
#include <QColor>

using namespace std;

namespace olive {

class Theme {

private:
  QColor primaryColor_;
  QColor secondaryColor_;
  QColor surfaceColor_;
  QColor surfaceDarkColor_;
  QColor surfaceBrightColor_;
  QColor surfaceTextColor_;
  QColor surfaceContrastColor_;


public:
  Theme(
    QColor primaryColor,
    QColor secondaryColor,
    QColor surfaceColor,
    QColor surfaceDarkColor,
    QColor surfaceBrightColor,
    QColor surfaceTextColor,
    QColor surfaceContrastColor) : 
    primaryColor_(primaryColor),
    secondaryColor_(secondaryColor),
    surfaceColor_(surfaceColor),
    surfaceDarkColor_(surfaceDarkColor),
    surfaceBrightColor_(surfaceBrightColor),
    surfaceTextColor_(surfaceTextColor),
    surfaceContrastColor_(surfaceContrastColor) {
  }

  Theme(Theme const& rhs) : 
    primaryColor_(rhs.primaryColor()),
    secondaryColor_(rhs.secondaryColor()),
    surfaceColor_(rhs.surfaceColor()),
    surfaceDarkColor_(rhs.surfaceDarkColor()),
    surfaceBrightColor_(rhs.surfaceBrightColor()),
    surfaceTextColor_(rhs.surfaceTextColor()),
    surfaceContrastColor_(rhs.surfaceContrastColor()) {
  }

  inline const QColor& primaryColor() const { return primaryColor_; }
  inline const QColor& secondaryColor() const { return secondaryColor_; }
  inline const QColor& surfaceColor() const { return surfaceColor_; }
  inline const QColor& surfaceDarkColor() const { return surfaceDarkColor_; }
  inline const QColor& surfaceBrightColor() const { return surfaceBrightColor_; }
  inline const QColor& surfaceTextColor() const { return surfaceTextColor_; }
  inline const QColor& surfaceContrastColor() const { return surfaceContrastColor_; }

};

}

#endif