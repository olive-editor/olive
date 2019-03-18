#include "icons.h"

#include <QDebug>

#include "global/global.h"
#include "global/config.h"

QIcon olive::icon::LeftArrow;
QIcon olive::icon::RightArrow;
QIcon olive::icon::UpArrow;
QIcon olive::icon::DownArrow;
QIcon olive::icon::Diamond;
QIcon olive::icon::Clock;

QIcon olive::icon::MediaVideo;
QIcon olive::icon::MediaAudio;
QIcon olive::icon::MediaImage;
QIcon olive::icon::MediaError;
QIcon olive::icon::MediaSequence;
QIcon olive::icon::MediaFolder;

QIcon olive::icon::ViewerGoToStart;
QIcon olive::icon::ViewerPrevFrame;
QIcon olive::icon::ViewerPlay;
QIcon olive::icon::ViewerPause;
QIcon olive::icon::ViewerNextFrame;
QIcon olive::icon::ViewerGoToEnd;

QIcon olive::icon::CreateIconFromSVG(const QString &path, bool create_disabled)
{
  QIcon icon;

  QPainter p;

  // Draw the icon as a solid color
  QPixmap normal(path);

  // Color the icon dark if the user is using a dark theme
  if (olive::styling::UseDarkIcons()) {
    p.begin(&normal);
    p.setCompositionMode(QPainter::CompositionMode_SourceIn);
    p.fillRect(normal.rect(), QColor(32, 32, 32));
    p.end();
  }

  icon.addPixmap(normal, QIcon::Normal, QIcon::On);

  if (create_disabled) {

    // Create semi-transparent disabled icon
    QPixmap disabled(normal.size());
    disabled.fill(Qt::transparent);

    // draw semi-transparent version of icon for the disabled variant
    p.begin(&disabled);
    p.setCompositionMode(QPainter::CompositionMode_SourceOver);
    p.setOpacity(0.5);
    p.drawPixmap(0, 0, normal);
    p.end();

    icon.addPixmap(disabled, QIcon::Disabled, QIcon::On);

  }

  return icon;
}

void olive::icon::Initialize()
{
  qInfo() << "Initializing icons";

  LeftArrow = CreateIconFromSVG(":/icons/tri-left.svg", false);
  RightArrow = CreateIconFromSVG(":/icons/tri-right.svg", false);
  UpArrow = CreateIconFromSVG(":/icons/tri-up.svg", false);
  DownArrow = CreateIconFromSVG(":/icons/tri-down.svg", false);
  Diamond = CreateIconFromSVG(":/icons/diamond.svg", false);
  Clock = CreateIconFromSVG(":/icons/clock.svg", false);

  MediaVideo = CreateIconFromSVG(":/icons/videosource.svg", false);
  MediaAudio = CreateIconFromSVG(":/icons/audiosource.svg", false);
  MediaImage = CreateIconFromSVG(":/icons/imagesource.svg", false);
  MediaError = CreateIconFromSVG(":/icons/error.svg", false);
  MediaSequence = CreateIconFromSVG(":/icons/sequence.svg", false);
  MediaFolder = CreateIconFromSVG(":/icons/folder.svg", false);

  ViewerGoToStart = CreateIconFromSVG(QStringLiteral(":/icons/prev.svg"));
  ViewerPrevFrame = CreateIconFromSVG(QStringLiteral(":/icons/rew.svg"));
  ViewerPlay = CreateIconFromSVG(QStringLiteral(":/icons/play.svg"));
  ViewerPause = CreateIconFromSVG(":/icons/pause.svg", false);
  ViewerNextFrame = CreateIconFromSVG(QStringLiteral(":/icons/ff.svg"));
  ViewerGoToEnd = CreateIconFromSVG(QStringLiteral(":/icons/next.svg"));

  qInfo() << "Finished initializing icons";
}
