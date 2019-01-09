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
#ifndef PREVIEWGENERATOR_H
#define PREVIEWGENERATOR_H

#include <QThread>
#include <QSemaphore>

#define ICON_TYPE_VIDEO 0
#define ICON_TYPE_AUDIO 1
#define ICON_TYPE_IMAGE 2
#define ICON_TYPE_ERROR 3

struct Footage;
struct FootageStream;
struct AVFormatContext;
class Media;

class PreviewGenerator : public QThread
{
    Q_OBJECT
public:
	PreviewGenerator(Media*, Footage*, bool);
    void run();
	void cancel();
signals:
	void set_icon(int, bool);
private:
    void parse_media();
	bool retrieve_preview(const QString &hash);
    void generate_waveform();
	void finalize_media();
    AVFormatContext* fmt_ctx;
    Media* media;
    Footage* footage;
	bool retrieve_duration;
	bool contains_still_image;
	bool replace;
	bool cancelled;
	QString data_path;
    QString get_thumbnail_path(const QString &hash, const FootageStream &ms);
    QString get_waveform_path(const QString& hash, const FootageStream &ms);
};

#endif // PREVIEWGENERATOR_H
