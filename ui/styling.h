#ifndef STYLING_H
#define STYLING_H

#include <QColor>

namespace olive {
  namespace styling {

    /**
     * @brief Officially supported styles to use in Olive
     */
    enum Style {
      /**
        Qt Fusion-based cross-platform UI. The default styling of Olive. Can also be heavily customized with a CSS
        file.
        */
      kOliveDefaultDark,

      /**
        Qt Fusion-based cross-platform UI. The default styling of Olive. Can also be heavily customized with a CSS
        file. This will use the
        */
      kOliveDefaultLight,

      /**
        Use current OS's native styling (or at least Qt's default). Most UIs use a light theming, so this will
        automatically implement dark icons/UI elements.
        */
      kNativeDarkIcons,

      /**
        Use current OS's native styling (or at least Qt's default). Most UIs use a light theming, but in case one
        doesn't, this option will provide light icons for use with a dark theme.
        */
      kNativeLightIcons
    };

    /**
     * @brief Return whether to use dark icons or light icons
     * @return
     *
     * **TRUE** if icons should be dark.
     */
    bool UseDarkIcons();

    /**
     * @brief Return whether to use native UI or Fusion
     * @return
     *
     * **TRUE** if UI should use native styling
     */
    bool UseNativeUI();

    /**
     * @brief Return the current icon color based on Config::use_dark_icons.
     *
     * Also used by some other UI elements like the lines and text on the TimelineHeader
     *
     * @return
     *
     * Either white or black depending on Config::use_dark_icons
     */
    QColor GetIconColor();
  }
}

#endif // STYLING_H
