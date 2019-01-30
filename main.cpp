#include "mainwindow.h"
#include <QApplication>

#include "debug.h"

// importing classes for certain command line args
#include "project/effect.h"
#include "ui/renderfunctions.h"

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

	bool use_internal_logger = true;

	if (argc > 1) {
		for (int i=1;i<argc;i++) {
			if (argv[i][0] == '-') {
				if (!strcmp(argv[i], "--version") || !strcmp(argv[i], "-v")) {
#ifndef GITHASH
					qWarning() << "No Git commit information found";
#endif
					printf("%s\n", appName.toUtf8().constData());
					return 0;
				} else if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) {
					printf("Usage: %s [options] [filename]\n\n[filename] is the file to open on startup.\n\nOptions:\n\t-v, --version\tShow version information\n\t-h, --help\tShow this help\n\t-f, --fullscreen\tStart in full screen mode\n\n", argv[0]);
					return 0;
				} else if (!strcmp(argv[i], "--fullscreen") || !strcmp(argv[i], "-f")) {
					launch_fullscreen = true;
				} else if (!strcmp(argv[i], "--disable-shaders")) {
					shaders_are_enabled = false;
				} else if (!strcmp(argv[i], "--no-debug")) {
					use_internal_logger = false;
				} else if (!strcmp(argv[i], "--disable-blend-modes")) {
					disable_blending = true;
				} else {
					printf("[ERROR] Unknown argument '%s'\n", argv[1]);
					return 1;
				}
			} else if (load_proj.isEmpty()) {
				load_proj = argv[i];
			}
		}
	}

	if (use_internal_logger) {
		qInstallMessageHandler(debug_message_handler);
	}

	// init ffmpeg subsystem
	av_register_all();
	avfilter_register_all();

	QApplication a(argc, argv);
	a.setWindowIcon(QIcon(":/icons/olive64.png"));

	MainWindow w(nullptr, appName);
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
