#ifndef ICONS_H
#define ICONS_H

#include <QIcon>

namespace olive {
  namespace icon {
    extern QIcon LeftArrow;
    extern QIcon RightArrow;
    extern QIcon UpArrow;
    extern QIcon DownArrow;
    extern QIcon Diamond;
    extern QIcon Clock;

    extern QIcon MediaVideo;
    extern QIcon MediaAudio;
    extern QIcon MediaImage;
    extern QIcon MediaError;
    extern QIcon MediaSequence;
    extern QIcon MediaFolder;

    extern QIcon ViewerGoToStart;
    extern QIcon ViewerPrevFrame;
    extern QIcon ViewerPlay;
    extern QIcon ViewerPause;
    extern QIcon ViewerNextFrame;
    extern QIcon ViewerGoToEnd;

    void Initialize();

    /**
     * @brief Converts an SVG into a QIcon with a semi-transparent for the QIcon::Disabled property
     *
     * @param path
     *
     * Path to SVG file
     */
    QIcon CreateIconFromSVG(const QString &path);
  }
}

#endif // ICONS_H
