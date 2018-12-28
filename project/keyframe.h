#ifndef KEYFRAME_H
#define KEYFRAME_H



#include <QVariant>
#include <QPoint>

class KeyframeData
{
public:
	QVariant data;
	QPoint handle_pre;
	QPoint handle_post;
};

#endif // KEYFRAME_H
