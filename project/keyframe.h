#ifndef KEYFRAME_H
#define KEYFRAME_H

#include <QVariant>

class EffectField;

class EffectKeyframe {
public:
	EffectKeyframe();

	long time;
	int type;
	QVariant data;

	// only for bezier type
	double pre_handle_x;
	double pre_handle_y;
	double post_handle_x;
	double post_handle_y;
};

void delete_keyframes(QVector<EffectField *> &selected_key_fields, QVector<int> &selected_keys);

#endif // KEYFRAME_H
