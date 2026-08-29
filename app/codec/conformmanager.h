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

#ifndef CONFORMMANAGER_H
#define CONFORMMANAGER_H

#include <QMutex>
#include <QObject>

#include "decoder.h"
#include "task/conform/conform.h"

namespace olive
{

class ConformManager : public QObject {
	Q_OBJECT
public:
	static void CreateInstance()
	{
		if (!instance_) {
			instance_ = new ConformManager();
		}
	}

	static void DestroyInstance()
	{
		delete instance_;
		instance_ = nullptr;
	}

	static ConformManager *instance()
	{
		return instance_;
	}

	enum ConformState { kConformExists, kConformGenerating };

	struct Conform {
		ConformState state;
		QVector<QString> filenames;
		ConformTask *task;
	};

	/**
     * @brief Get conform state, and start conforming if no conform exists
     *
     * Thread-safe.
     */
	Conform GetConformState(const QString &decoder_id,
							const QString &cache_path,
							const Decoder::CodecStream &stream,
							const AudioParams &params, bool wait);

signals:
	void ConformReady();

private:
	ConformManager() = default;

	static ConformManager *instance_;

	QMutex mutex_;

	QWaitCondition conform_done_condition_;

	struct ConformData {
		Decoder::CodecStream stream;
		AudioParams params;
		ConformTask *task;
		QVector<QString> working_filename;
		QVector<QString> finished_filename;
	};

	QVector<ConformData> conforming_;

	/**
     * @brief Get the destination filename of an audio stream conformed to a set of parameters
     */
	static QVector<QString>
	GetConformedFilename(const QString &cache_path,
						 const Decoder::CodecStream &stream,
						 const AudioParams &params);

	static bool AllConformsExist(const QVector<QString> &filenames);

private slots:
	void ConformTaskFinished(Task *task, bool succeeded);
};

} // namespace olive

#endif // CONFORMMANAGER_H
