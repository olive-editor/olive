#ifndef _OLIVE_ITHEME_SERVICE_IMPL_H_
#define _OLIVE_ITHEME_SERVICE_IMPL_H_

#include "platform/theme/themeservice.h"

namespace olive {

class ThemeService : public IThemeService {

private:
  static ThemeService* instance_;

  Theme theme_;

protected:
  ThemeService();

public:
  static void Initialize();
  inline static ThemeService* const instance() {
    return ThemeService::instance_;
  }

  void setTheme(Theme const& theme) override;
  const Theme& getTheme() const override;

};

}

#endif