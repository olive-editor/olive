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
