/***

    Olive - Non-Linear Video Editor
    Copyright (C) 2019  Olive Team

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#include "mainwindow.h"
#include <QApplication>

#include "debug.h"

#include "oliveglobal.h"

#include "io/config.h"

extern "C" {
	#include <libavformat/avformat.h>
	#include <libavfilter/avfilter.h>
}

int main(int argc, char *argv[]) {   
    olive::Global = std::unique_ptr<OliveGlobal>(new OliveGlobal);

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
                    printf("%s\n", olive::AppName.toUtf8().constData());
					return 0;
				} else if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) {
					printf("Usage: %s [options] [filename]\n\n"
						   "[filename] is the file to open on startup.\n\n"
						   "Options:\n"
						   "\t-v, --version\t\tShow version information\n"
						   "\t-h, --help\t\tShow this help\n"
						   "\t-f, --fullscreen\tStart in full screen mode\n"
						   "\t--disable-shaders\tDisable OpenGL shaders (for debugging)\n"
						   "\t--no-debug\t\tDisable internal debug log and output directly to console\n"
						   "\t--disable-blend-modes\tDisable shader-based blending for older GPUs\n"
						   "\t--translation <file>\tSet an external language file to use\n"
						   "\n"
						   "Environment Variables:\n"
						   "\tOLIVE_EFFECTS_PATH\tSpecify a path to search for GLSL shader effects\n"
						   "\tFREI0R_PATH\t\tSpecify a path to search for Frei0r effects\n"
						   "\tOLIVE_LANG_PATH\t\tSpecify a path to search for translation files\n"
						   "\n", argv[0]);
					return 0;
				} else if (!strcmp(argv[i], "--fullscreen") || !strcmp(argv[i], "-f")) {
					launch_fullscreen = true;
				} else if (!strcmp(argv[i], "--disable-shaders")) {
                    olive::CurrentRuntimeConfig.shaders_are_enabled = false;
				} else if (!strcmp(argv[i], "--no-debug")) {
					use_internal_logger = false;
				} else if (!strcmp(argv[i], "--disable-blend-modes")) {
                    olive::CurrentRuntimeConfig.disable_blending = true;
				} else if (!strcmp(argv[i], "--translation")) {
					if (i + 1 < argc && argv[i + 1][0] != '-') {
						// load translation file
                        olive::CurrentRuntimeConfig.external_translation_file = argv[i + 1];

						i++;
					} else {
						printf("[ERROR] No translation file specified\n");
						return 1;
					}
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

    // Initialize ffmpeg subsystem
    // (these have been deprecated in FFmpeg 4, but are still necessary for FFmpeg 3)
    av_register_all();
    avfilter_register_all();

    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);

	QApplication a(argc, argv);
    a.setWindowIcon(QIcon(":/icons/olive64.png"));

    QCoreApplication::setOrganizationName("olivevideoeditor.org");
    QCoreApplication::setOrganizationDomain("olivevideoeditor.org");
    QCoreApplication::setApplicationName("Olive");
    QGuiApplication::setDesktopFileName("org.olivevideoeditor.Olive");

    MainWindow w(nullptr);

    // connect main window's first paint to global's init finished function
    QObject::connect(&w, SIGNAL(finished_first_paint()), olive::Global.get(), SLOT(finished_initialize()));

    if (!load_proj.isEmpty()) {
        olive::Global->load_project_on_launch(load_proj);
	}
	if (launch_fullscreen) {
		w.showFullScreen();
	} else {
		w.showMaximized();
	}

	return a.exec();
}
