/*
 * Oak Video Editor - Non-Linear Video Editor
 * Copyright (C) 2026 Oak Team
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

#ifndef PROXYTASK_H
#define PROXYTASK_H

#include "codec/proxymanager.h"
#include "task/task.h"

namespace olive
{

class ProxyTask : public Task {
	Q_OBJECT
public:
	ProxyTask(const QString &source_filename, int stream_index,
			  const ProxyManager::ProxyParams &params,
			  const QString &output_filename);

	/**
	 * @brief Builds the ffmpeg command line for a proxy generation run
	 *
	 * Extracted for testability. The video stream is always mapped first so
	 * that it is stream 0 in the proxy file; audio streams (when enabled)
	 * follow in source order.
	 */
	static QStringList BuildArguments(const QString &source_filename,
									  int stream_index,
									  const ProxyManager::ProxyParams &params,
									  const QString &output_filename);

protected:
	virtual bool Run() override;

private:
	QString source_filename_;
	int stream_index_;
	ProxyManager::ProxyParams params_;
	QString output_filename_;
};

}

#endif // PROXYTASK_H
