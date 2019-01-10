/* 
 * Olive. Olive is a free non-linear video editor for Windows, macOS, and Linux.
 * Copyright (C) 2018  {{ organization }}
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 *along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
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

	MainWindow w;
	w.appName = appName;
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
