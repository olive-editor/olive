#include "media.h"

extern "C" {
	#include <libavformat/avformat.h>
}

long Media::get_length_in_frames(float frame_rate) {
	return ceil((float) length / (float) AV_TIME_BASE * frame_rate);
}
