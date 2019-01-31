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

class ProxyGenerator : public QThread
{
public:
	ProxyGenerator();
	void run();
	void queue(const ProxyInfo& info);
	void cancel();
private:
	QVector<ProxyInfo> proxy_queue;
	QWaitCondition waitCond;
	QMutex mutex;
	bool cancelled;
};

// proxy generator is a global omnipotent entity
extern ProxyGenerator proxy_generator;

#endif // PROXYGENERATOR_H
