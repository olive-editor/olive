#ifndef ICONS_H
#define ICONS_H

#include <QIcon>

namespace olive {
namespace icon {

// Playback Icons
extern QIcon GoToStart;
extern QIcon PrevFrame;
extern QIcon Play;
extern QIcon Pause;
extern QIcon NextFrame;
extern QIcon GoToEnd;

// Project Management Toolbar Icons
extern QIcon New;
extern QIcon Open;
extern QIcon Save;
extern QIcon Undo;
extern QIcon Redo;
extern QIcon TreeView;
extern QIcon ListView;
extern QIcon IconView;

// Tool Icons
extern QIcon ToolPointer;
extern QIcon ToolEdit;
extern QIcon ToolRipple;
extern QIcon ToolRazor;
extern QIcon ToolSlip;
extern QIcon ToolSlide;
extern QIcon ToolHand;
extern QIcon ToolTransition;

// Miscellaneous Icons
extern QIcon Snapping;
extern QIcon ZoomIn;
extern QIcon ZoomOut;
extern QIcon Record;
extern QIcon Add;

QIcon Create(const QString& name);
void LoadAll();

}
}

#endif // ICONS_H
