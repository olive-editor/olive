#ifndef PROXYGENERATOR_H
#define PROXYGENERATOR_H

#include <QThread>
#include <QVector>
#include <QMutex>
#include <QWaitCondition>

struct Footage;

struct ProxyInfo {
	Footage* footage;
	double size_multiplier;
	int codec_type;
	QString path;
};

class ProxyGenerator : public QThread {
    Q_OBJECT
public:
	ProxyGenerator();
	void run();
	void queue(const ProxyInfo& info);
	void cancel();
	double get_proxy_progress(Footage* f);
private:
	// queue of footage to process proxies for
	QVector<ProxyInfo> proxy_queue;

	// threading objects
	QWaitCondition waitCond;
	QMutex mutex;

	// set to true if you want to permanently close ProxyGenerator
	bool cancelled;

	// set to true if you want to abort the footage currently being processed
	bool skip;

	// stores progress in percent of proxy currently being processed
	double current_progress;

	// function that performs the actual transcode
	void transcode(const ProxyInfo& info);
};

// proxy generator is a global omnipotent entity
extern ProxyGenerator proxy_generator;

#endif // PROXYGENERATOR_H
