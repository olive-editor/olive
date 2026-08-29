/*
 * Oak Video Editor - Non-Linear Video Editor
 * Copyright (C) 2025 Olive CE Team
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef THREADSAFEMAP_H
#define THREADSAFEMAP_H

#include <QMap>
#include <QMutex>

template <typename K, typename V> class ThreadSafeMap {
public:
	ThreadSafeMap() = default;

	void insert(K key, V value)
	{
		mutex_.lock();
		map_.insert(key, value);
		mutex_.unlock();
	}

private:
	QMutex mutex_;

	QMap<K, V> map_;
};

#endif // THREADSAFEMAP_H
