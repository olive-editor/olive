#include "platform/theme/themeservice-impl.h"

#include <QColor>

namespace olive {

ThemeService* ThemeService::instance_ = nullptr;

void ThemeService::Initialize() {
  ThemeService::instance_ = new ThemeService();
}

ThemeService::ThemeService() :
  theme_(
    QColor("#28aaff"),
    QColor("#ff5040"),
    QColor("#212624"),
    QColor("#1a1a1a"),
    QColor("#616161"),
    QColor("#BDBDBD"),
    QColor("#BDBDBD")) {

}

void ThemeService::setTheme(Theme const& theme) {
  theme_ = theme;
  emit onDidChangeTheme(theme_);
}

const Theme& ThemeService::getTheme() const {
  return theme_;
}

}