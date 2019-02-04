#include "cursors.h"

#include <QPixmap>
#include <QCursor>

#include <QDebug>

QCursor OLIVE_CURSORS::left_arrow;
QCursor OLIVE_CURSORS::right_arrow;

QCursor load_cursor(QString file, const int hotX, const int hotY, const bool right_aligend){
	int hotoutX;
	QPixmap temp = QPixmap(file);
	right_aligend? hotoutX = temp.width() : hotoutX = hotX;
	return QCursor(temp,hotoutX,hotY);
}

void initCustomCursors(){
	qInfo() << "Loading Custom Cursors";
	OLIVE_CURSORS::left_arrow = load_cursor(":/cursors/Cursor_Left_Arrow.png", 0,-1, false);
	OLIVE_CURSORS::right_arrow = load_cursor(":/cursors/Cursor_Right_Arrow.png", 0,-1, true);
}
