/***

  Oak - Non-Linear Video Editor
  Copyright (C) 2026 Oak Team

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

#include "proxy.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>

#include "config/config.h"

namespace olive
{

ProxyTask::ProxyTask(const QString &source_filename, int stream_index,
					 const ProxyManager::ProxyParams &params,
					 const QString &output_filename)
	: source_filename_(source_filename)
	, stream_index_(stream_index)
	, params_(params)
	, output_filename_(output_filename)
{
	SetTitle(tr("Generating Proxy %1:%2")
				 .arg(source_filename_, QString::number(stream_index_)));
}

QStringList ProxyTask::BuildArguments(const QString &source_filename,
									  int stream_index,
									  const ProxyManager::ProxyParams &params,
									  const QString &output_filename)
{
	const QString scale_filter =
		QStringLiteral("scale=w=%1:h=%2:force_original_aspect_ratio=decrease")
			.arg(QString::number(params.width),
				 QString::number(params.height));

	const QString container_format =
		params.extension.isEmpty() ? QStringLiteral("mp4") : params.extension;

	QStringList args;
	args << QStringLiteral("-y") << QStringLiteral("-i") << source_filename
		 // Map the requested video stream first so it is stream 0 in the proxy
		 << QStringLiteral("-map") << QStringLiteral("0:%1").arg(stream_index);

	if (params.include_audio) {
		// Keep the source audio (if any) so the proxy can also be used for
		// audio preview. Audio streams follow the video stream in source order.
		args << QStringLiteral("-map") << QStringLiteral("0:a?")
			 << QStringLiteral("-c:a") << QStringLiteral("aac")
			 << QStringLiteral("-b:a") << QStringLiteral("128k");
	} else {
		args << QStringLiteral("-an");
	}

	args << QStringLiteral("-vf") << scale_filter << QStringLiteral("-c:v")
		 << QStringLiteral("libx264") << QStringLiteral("-preset")
		 << params.preset << QStringLiteral("-crf")
		 << QString::number(params.crf) << QStringLiteral("-pix_fmt")
		 << QStringLiteral("yuv420p") << QStringLiteral("-movflags")
		 << QStringLiteral("+faststart") << QStringLiteral("-f")
		 << container_format << output_filename;

	return args;
}

bool ProxyTask::Run()
{
	const QString ffmpeg = ProxyManager::FindFFmpegExecutable(
		OLIVE_CONFIG("FFmpegPath").toString());
	if (ffmpeg.isEmpty()) {
		SetError(
			tr("Failed to generate proxy: ffmpeg executable was not found. Set "
			   "the ffmpeg path in Preferences > Disk > Proxy Settings."));
		qWarning() << "ProxyTask: ffmpeg executable not found";
		return false;
	}

	QDir output_dir = QFileInfo(output_filename_).dir();
	if (!output_dir.exists() && !output_dir.mkpath(QStringLiteral("."))) {
		SetError(tr("Failed to create proxy output directory"));
		qWarning() << "ProxyTask: failed to create output directory"
				   << output_dir.absolutePath();
		return false;
	}

	qDebug()
		<< "ProxyTask: starting ffmpeg proxy generation:" << source_filename_
		<< "->" << output_filename_;

	QFile::remove(output_filename_);

	const QStringList args = BuildArguments(source_filename_, stream_index_,
											params_, output_filename_);

	QProcess process;
	process.setProgram(ffmpeg);
	process.setArguments(args);
	process.setProcessChannelMode(QProcess::MergedChannels);
	process.start();

	if (!process.waitForStarted()) {
		SetError(tr("Failed to start ffmpeg for proxy generation"));
		qWarning()
			<< "ProxyTask: failed to start ffmpeg" << process.errorString();
		return false;
	}

	while (!process.waitForFinished(100)) {
		if (IsCancelled()) {
			process.kill();
			process.waitForFinished();
			QFile::remove(output_filename_);
			SetError(tr("Proxy generation was cancelled"));
			return false;
		}
	}

	if (process.exitStatus() != QProcess::NormalExit ||
		process.exitCode() != 0) {
		const QString output = QString::fromUtf8(process.readAll()).trimmed();
		QFile::remove(output_filename_);
		SetError(tr("ffmpeg failed to generate proxy: %1").arg(output));
		qWarning() << "ProxyTask: ffmpeg failed with exit code"
				   << process.exitCode() << "output:" << output;
		return false;
	}

	if (!QFileInfo::exists(output_filename_)) {
		SetError(tr("ffmpeg finished but proxy file was not created"));
		qWarning() << "ProxyTask: ffmpeg finished but output file missing"
				   << output_filename_;
		return false;
	}

	qDebug() << "ProxyTask: proxy generation succeeded:" << output_filename_;
	emit ProgressChanged(1.0);
	return true;
}

}
