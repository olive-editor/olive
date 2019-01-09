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
#ifndef MEDIA_H
#define MEDIA_H

#include <QList>
#include <QVariant>

#define MEDIA_TYPE_FOOTAGE 0
#define MEDIA_TYPE_SEQUENCE 1
#define MEDIA_TYPE_FOLDER 2

struct Footage;
class MediaThrobber;
struct Sequence;
#include <QIcon>

class Media
{
public:
    Media(Media* iparent);
    ~Media();
    Footage *to_footage();
    Sequence* to_sequence();
    void set_footage(Footage* f);
    void set_sequence(Sequence* s);
    void set_folder();
    void set_icon(const QIcon &ico);
    void set_parent(Media* p);
    void update_tooltip(const QString& error = 0);
    void *to_object();
    int get_type();
    const QString& get_name();
    void set_name(const QString& n);
    MediaThrobber* throbber;

	double get_frame_rate(int stream = -1);
	int get_sampling_rate(int stream = -1);

    // item functions
    void appendChild(Media *child);
    bool setData(int col, const QVariant &value);
    Media *child(int row);
    int childCount() const;
    int columnCount() const;
    QVariant data(int column, int role);
    int row() const;
    Media *parentItem();
    void removeChild(int i);

    bool root;
    int temp_id;
    int temp_id2;
private:
    int type;
    void* object;

    // item functions
    QList<Media*> children;
    Media* parent;
    QString folder_name;
    QString tooltip;
    QIcon icon;
};

#endif // MEDIA_H
