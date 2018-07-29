#ifndef CACHER_H
#define CACHER_H

#include <QThread>

struct Clip;

class Cacher : public QThread
{
//	Q_OBJECT
public:
	Cacher(Clip* c);
    void run();

	bool caching;

	// must be set before caching
	long playhead;
	bool write_A;
	bool write_B;
	bool reset;
    Clip* nest;

private:
	Clip* clip;
};

void open_clip_worker(Clip* clip);
void cache_clip_worker(Clip* clip, long playhead, bool write_A, bool write_B, bool reset, Clip *nest);
void close_clip_worker(Clip* clip);

#endif // CACHER_H
