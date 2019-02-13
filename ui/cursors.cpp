#include "cursors.h"

#include <QPixmap>
#include <QCursor>

#include <QDebug>

QCursor Olive::Cursor_LeftTrim;
QCursor Olive::Cursor_RightTrim;

QCursor load_cursor(const QString& file, int hotX, int hotY, const bool& right_aligned){
    // load specified file into a pixmap
    QPixmap temp(file);

    // set cursor's horizontal hotspot
    if (right_aligned) {
        hotX = temp.width() - hotX;
    }

    // return cursor
    return QCursor(temp, hotX, hotY);
}

void init_custom_cursors(){
    qInfo() << "Initializing custom cursors";
    Olive::Cursor_LeftTrim = load_cursor(":/cursors/left_side.png", 0, -1, false);
    Olive::Cursor_RightTrim = load_cursor(":/cursors/right_side.png", 0, -1, true);
    qInfo() << "Finished initializing custom cursors";
}
