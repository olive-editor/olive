#include "mainwindow.h"
#include <QApplication>

extern "C" {
	#include <libavformat/avformat.h>
	#include <libavfilter/avfilter.h>
}

int main(int argc, char *argv[]) {
	// init ffmpeg subsystem
	av_register_all();
	avfilter_register_all();

	QApplication a(argc, argv);
	MainWindow w;
	if (argc > 1) w.launch_with_project(argv[1]);
	w.showMaximized();

	return a.exec();
}
