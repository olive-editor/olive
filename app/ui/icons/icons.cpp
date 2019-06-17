#include "icons.h"

#include <QDebug>

/// Works in conjunction with `genicons.sh` to generate and utilize icons of specific sizes
const int ICON_SIZE_COUNT = 4;
const int ICON_SIZES[] = {
  16,
  32,
  64,
  128
};

/// Internal icon library for use throughout Olive without having to regenerate constantly
QIcon olive::icon::GoToStart;
QIcon olive::icon::PrevFrame;
QIcon olive::icon::Play;
QIcon olive::icon::Pause;
QIcon olive::icon::NextFrame;
QIcon olive::icon::GoToEnd;
QIcon olive::icon::New;
QIcon olive::icon::Open;
QIcon olive::icon::Save;
QIcon olive::icon::Undo;
QIcon olive::icon::Redo;
QIcon olive::icon::TreeView;
QIcon olive::icon::ListView;
QIcon olive::icon::IconView;

void olive::icon::LoadAll()
{
  GoToStart = Create("prev");
  PrevFrame = Create("rew");
  Play = Create("play");
  Pause = Create("pause");
  NextFrame = Create("ff");
  GoToEnd = Create("next");

  New = Create("new");
  Open = Create("open");
  Save = Create("save");
  Undo = Create("undo");
  Redo = Create("redo");
  TreeView = Create("treeview");
  ListView = Create("listview");
  IconView = Create("iconview");
}

QIcon olive::icon::Create(const QString &name)
{
  QIcon icon;

  for (int i=0;i<ICON_SIZE_COUNT;i++) {
    icon.addFile(QString(":/icons/%1.%2.png").arg(name, QString::number(ICON_SIZES[i])),
                 QSize(ICON_SIZES[i], ICON_SIZES[i]));
  }

  return icon;
}
