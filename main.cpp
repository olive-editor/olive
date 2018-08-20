#include "mainwindow.h"
#include <QApplication>

extern "C" {
	#include <libavformat/avformat.h>
}

int main(int argc, char *argv[])
{
	// init ffmpeg subsystem
	av_register_all();

    QApplication a(argc, argv);
	MainWindow w;
	w.show();

	return a.exec();
}
