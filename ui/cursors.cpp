#include "cursors.h"

#include <QPixmap>
#include <QCursor>

#include <QDebug>

QCursor Olive::left_side;
QCursor Olive::right_side;

QCursor load_cursor(QString file, const int hotX, const int hotY, const bool right_aligned){
	int hotX_out;
	QPixmap temp = QPixmap(file);
	right_aligned? hotX_out = temp.width() : hotX_out = hotX;
	return QCursor(temp,hotX_out,hotY);
}

void initCustomCursors(){
	qInfo() << "Loading Custom Cursors";
	Olive::left_side = load_cursor(":/cursors/left_side.png", 0,-1, false);
	Olive::right_side = load_cursor(":/cursors/right_side.png", 0,-1, true);
}
