#include "mainwindow.h"
#include <QApplication>
#include <QDebug>

extern "C" {
	#include <libavformat/avformat.h>
	#include <libavfilter/avfilter.h>
}

int main(int argc, char *argv[]) {
	QString appName = "Olive (January 2019 | Alpha";
#ifdef GITHASH
	appName += " | ";
	appName += GITHASH;
#endif
	appName += ")";

	bool launch_fullscreen = false;
	QString load_proj;

	if (argc > 1) {
		for (int i=1;i<argc;i++) {
			if (argv[i][0] == '-') {
				if (!strcmp(argv[1], "--version") || !strcmp(argv[1], "-v")) {
#ifndef GITHASH
				printf("[WARNING] No Git commit information found\n");
#endif
					printf("%s\n", appName.toUtf8().constData());
					return 0;
				} else if (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-h")) {
					printf("Usage: %s [options] [filename]\n\n[filename] is the file to open on startup.\n\nOptions:\n\t-v, --version\tShow version information\n\t-h, --help\tShow this help\n\t-f, --fullscreen\tStart in full screen mode\n\n", argv[0]);
					return 0;
				} else if (!strcmp(argv[1], "--fullscreen") || !strcmp(argv[1], "-f")) {
					launch_fullscreen = true;
				} else {
					printf("[ERROR] Unknown argument '%s'\n", argv[1]);
					return 1;
				}
			} else if (load_proj.isEmpty()) {
				load_proj = argv[i];
			}
		}
	}

	// init ffmpeg subsystem
	av_register_all();
	avfilter_register_all();

	QApplication a(argc, argv);

	MainWindow w(NULL, appName);
	w.updateTitle("");

	if (!load_proj.isEmpty()) {
		w.launch_with_project(load_proj);
	}
	if (launch_fullscreen) {
		w.showFullScreen();
	} else {
		w.showMaximized();
	}

	return a.exec();
}
