#include "icons.h"

#include "oliveglobal.h"

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

QIcon olive::icon::CreateIconFromSVG(const QString &path)
{
  QIcon icon;

  QPixmap normal(path);
  icon.addPixmap(normal, QIcon::Normal, QIcon::On);

  QPixmap disabled(normal.size());
  disabled.fill(Qt::transparent);

  // draw semi-transparent version of icon for the disabled variant
  QPainter p(&disabled);
  p.setOpacity(0.5);
  p.drawPixmap(0, 0, normal);
  p.end();

  icon.addPixmap(disabled, QIcon::Disabled, QIcon::On);

  return icon;
}

void olive::icon::Initialize()
{
  LeftArrow = QIcon(":/icons/tri-left.svg");
  RightArrow = QIcon(":/icons/tri-right.svg");
  UpArrow = QIcon(":/icons/tri-up.svg");
  DownArrow = QIcon(":/icons/tri-down.svg");
  Diamond = QIcon(":/icons/diamond.svg");
  Clock = QIcon(":/icons/clock.svg");

  MediaVideo = QIcon(":/icons/videosource.svg");
  MediaAudio = QIcon(":/icons/audiosource.svg");
  MediaImage = QIcon(":/icons/imagesource.svg");
  MediaError = QIcon(":/icons/error.svg");
  MediaSequence = QIcon(":/icons/sequence.svg");
  MediaFolder = QIcon(":/icons/folder.svg");

  ViewerGoToStart = CreateIconFromSVG(QStringLiteral(":/icons/prev.svg"));
  ViewerPrevFrame = CreateIconFromSVG(QStringLiteral(":/icons/rew.svg"));
  ViewerPlay = CreateIconFromSVG(QStringLiteral(":/icons/play.svg"));
  ViewerPause = QPixmap(":/icons/pause.svg");
  ViewerNextFrame = CreateIconFromSVG(QStringLiteral(":/icons/ff.svg"));
  ViewerGoToEnd = CreateIconFromSVG(QStringLiteral(":/icons/next.svg"));
}
