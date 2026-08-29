/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team
  Modifications Copyright (C) 2025 mikesolar

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

#include "footage.h"

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QStandardPaths>

#include "codec/decoder.h"
#include "common/filefunctions.h"
#include "common/qtutils.h"
#include "common/xmlutils.h"
#include "config/config.h"
#include "core.h"
#include "render/job/footagejob.h"
#include "ui/icons/icons.h"

namespace olive
{

const QString Footage::kFilenameInput = QStringLiteral("file_in");

#define super ViewerOutput

Footage::Footage(const QString &filename)
	: ViewerOutput(false, false)
	, timestamp_(0)
	, has_source_start_time_(false)
	, proxy_enabled_(false)
	, proxy_state_(ProxyManager::kProxyMissing)
	, proxy_video_stream_index_(-1)
	, proxy_preset_version_(0)
	, has_custom_proxy_params_(false)
	, valid_(false)
	, cancelled_(nullptr)
	, total_stream_count_(0)
{
	SetFlag(kIsItem);

	PrependInput(kFilenameInput, NodeValue::kFile,
				 InputFlags(kInputFlagNotConnectable |
							kInputFlagNotKeyframable));

	Clear();

	if (!filename.isEmpty()) {
		set_filename(filename);
	}

	QTimer *check_timer = new QTimer(this);
	check_timer->setInterval(5000);
	connect(check_timer, &QTimer::timeout, this, &Footage::CheckFootage);
	check_timer->start();

	connect(this->waveform_cache(), &AudioWaveformCache::Validated, this,
			&ViewerOutput::ConnectedWaveformChanged);
}

void Footage::Retranslate()
{
	super::Retranslate();

	SetInputName(kFilenameInput, tr("Filename"));
}

void Footage::InputValueChangedEvent(const QString &input, int element)
{
	if (input == kFilenameInput) {
		// Reset internal stream cache
		Clear();

		Reprobe();
	} else {
		super::InputValueChangedEvent(input, element);
	}
}

rational Footage::VerifyLengthInternal(Track::Type type) const
{
	if (type == Track::kVideo) {
		VideoParams first_stream = GetFirstEnabledVideoStream();

		if (first_stream.is_valid()) {
			return Timecode::timestamp_to_time(first_stream.duration(),
											   first_stream.time_base());
		}
	} else if (type == Track::kAudio) {
		AudioParams first_stream = GetFirstEnabledAudioStream();

		if (first_stream.is_valid()) {
			return Timecode::timestamp_to_time(first_stream.duration(),
											   first_stream.time_base());
		}
	} else if (type == Track::kSubtitle) {
		SubtitleParams first_stream = GetFirstEnabledSubtitleStream();

		if (first_stream.is_valid()) {
			return first_stream.duration();
		}
	}

	return 0;
}

QString Footage::GetColorspaceToUse(const VideoParams &params) const
{
	if (params.colorspace().isEmpty()) {
		return project()->color_manager()->GetDefaultInputColorSpace();
	} else {
		return params.colorspace();
	}
}

void Footage::Clear()
{
	// Clear all dynamically created inputs
	InputArrayResize(kVideoParamsInput, 0);
	InputArrayResize(kAudioParamsInput, 0);
	InputArrayResize(kSubtitleParamsInput, 0);

	// Clear decoder link
	decoder_.clear();

	has_source_start_time_ = false;
	source_start_time_ = rational();
	source_start_time_source_.clear();
	ClearProxy();

	// Clear total stream count
	total_stream_count_ = 0;

	// Reset ready state
	valid_ = false;
}

void Footage::SetValid()
{
	valid_ = true;
}

QString Footage::filename() const
{
	return GetStandardValue(kFilenameInput).toString();
}

void Footage::set_filename(const QString &s)
{
	SetStandardValue(kFilenameInput, s);
}

const qint64 &Footage::timestamp() const
{
	return timestamp_;
}

void Footage::set_timestamp(const qint64 &t)
{
	timestamp_ = t;
}

int Footage::GetStreamIndex(Track::Type type, int index) const
{
	switch (type) {
	case Track::kVideo:
		if (index >= 0 && index < GetVideoStreamCount()) {
			return GetVideoParams(index).stream_index();
		}
		break;
	case Track::kAudio:
		if (index >= 0 && index < GetAudioStreamCount()) {
			return GetAudioParams(index).stream_index();
		}
		break;
	case Track::kSubtitle:
		if (index >= 0 && index < GetSubtitleStreamCount()) {
			return GetSubtitleParams(index).stream_index();
		}
		break;
	case Track::kNone:
	case Track::kCount:
		break;
	}

	return -1;
}

Track::Reference Footage::GetReferenceFromRealIndex(int real_index) const
{
	// Check video streams
	for (int i = 0; i < GetVideoStreamCount(); i++) {
		if (GetVideoParams(i).stream_index() == real_index) {
			return Track::Reference(Track::kVideo, i);
		}
	}

	for (int i = 0; i < GetAudioStreamCount(); i++) {
		if (GetAudioParams(i).stream_index() == real_index) {
			return Track::Reference(Track::kAudio, i);
		}
	}

	for (int i = 0; i < GetSubtitleStreamCount(); i++) {
		if (GetSubtitleParams(i).stream_index() == real_index) {
			return Track::Reference(Track::kSubtitle, i);
		}
	}

	return Track::Reference();
}

const QString &Footage::decoder() const
{
	return decoder_;
}

void Footage::SetSourceStartTime(const rational &time, const QString &source)
{
	source_start_time_ = time;
	source_start_time_source_ = source;
	has_source_start_time_ = true;
}

void Footage::ClearSourceStartTime()
{
	source_start_time_ = rational();
	source_start_time_source_.clear();
	has_source_start_time_ = false;
}

void Footage::set_proxy_enabled(bool enabled)
{
	if (proxy_enabled_ != enabled) {
		qDebug() << "Footage::set_proxy_enabled:" << filename() << enabled;
		proxy_enabled_ = enabled;
		if (Project *p = project()) {
			p->set_modified(true);
		}
		emit ProxySettingsChanged();
	}
}

void Footage::SetProxy(const QString &path, ProxyManager::ProxyState state,
					   int video_stream_index, int preset_version, bool enabled)
{
	qDebug() << "Footage::SetProxy:" << filename() << "enabled=" << enabled
			 << "state=" << ProxyManager::ProxyStateToString(state)
			 << "path=" << path;
	proxy_path_ = path;
	proxy_state_ = state;
	proxy_video_stream_index_ = video_stream_index;
	proxy_preset_version_ = preset_version;
	proxy_enabled_ = enabled;
	if (Project *p = project()) {
		p->set_modified(true);
	}
	emit ProxySettingsChanged();
}

void Footage::ClearProxy()
{
	proxy_enabled_ = false;
	proxy_path_.clear();
	proxy_state_ = ProxyManager::kProxyMissing;
	proxy_video_stream_index_ = -1;
	proxy_preset_version_ = 0;
	emit ProxySettingsChanged();
}

void Footage::SetCustomProxyParams(const ProxyManager::ProxyParams &params)
{
	custom_proxy_params_ = params;
	has_custom_proxy_params_ = true;
	if (Project *p = project()) {
		p->set_modified(true);
	}
	emit ProxySettingsChanged();
}

void Footage::ClearCustomProxyParams()
{
	if (has_custom_proxy_params_) {
		has_custom_proxy_params_ = false;
		custom_proxy_params_ = ProxyManager::ProxyParams();
		if (Project *p = project()) {
			p->set_modified(true);
		}
		emit ProxySettingsChanged();
	}
}

ProxyManager::ProxyParams Footage::GetEffectiveProxyParams() const
{
	if (has_custom_proxy_params_) {
		return custom_proxy_params_;
	}

	return ProxyManager::ProxyParamsFromConfig();
}

QString Footage::DescribeVideoStream(const VideoParams &params)
{
	if (params.video_type() == VideoParams::kVideoTypeStill) {
		return tr("%1: Image - %2x%3")
			.arg(QString::number(params.stream_index()),
				 QString::number(params.width()),
				 QString::number(params.height()));
	} else {
		return tr("%1: Video - %2x%3")
			.arg(QString::number(params.stream_index()),
				 QString::number(params.width()),
				 QString::number(params.height()));
	}
}

QString Footage::DescribeAudioStream(const AudioParams &params)
{
	return tr("%1: Audio - %n Channel(s), %2Hz", nullptr,
			  params.channel_count())
		.arg(QString::number(params.stream_index()),
			 QString::number(params.sample_rate()));
}

QString Footage::DescribeSubtitleStream(const SubtitleParams &params)
{
	return tr("%1: Subtitle").arg(QString::number(params.stream_index()));
}

void Footage::Value(const NodeValueRow &value, const NodeGlobals &globals,
					NodeValueTable *table) const
{
	Q_UNUSED(globals)

	// Pop filename from table
	QString file = value[kFilenameInput].toString();

	// If the file exists and the reference is valid, push a footage job to the renderer
	if (QFileInfo::exists(file)) {
		// Push length
		table->Push(NodeValue::kRational, QVariant::fromValue(GetLength()),
					this, QStringLiteral("length"));

		// Push each stream as a footage job
		for (int i = 0; i < GetTotalStreamCount(); i++) {
			Track::Reference ref = GetReferenceFromRealIndex(i);
			FootageJob job(globals.time(), decoder_, filename(), ref.type(),
						   GetLength(), globals.loop_mode());

			if (ref.type() == Track::kVideo) {
				VideoParams vp = GetVideoParams(ref.index());

				if (proxy_enabled_ && !proxy_path_.isEmpty() &&
					proxy_video_stream_index_ == vp.stream_index() &&
					ProxyManager::GetProxyState(proxy_path_) ==
						ProxyManager::kProxyReady) {
					job.set_proxy(proxy_path_, QStringLiteral("ffmpeg"), 0);
				}

				// Ensure the colorspace is valid and not empty
				vp.set_colorspace(GetColorspaceToUse(vp));

				// Adjust footage job's divider
				if (globals.vparams().divider() > 1) {
					// Use a divider appropriate for this target resolution
					int calculated = VideoParams::GetDividerForTargetResolution(
						vp.width(), vp.height(),
						globals.vparams().effective_width(),
						globals.vparams().effective_height());
					vp.set_divider(
						std::min(calculated, globals.vparams().divider()));
				} else {
					// Render everything at full res
					vp.set_divider(1);
				}

				job.set_video_params(vp);

				table->Push(NodeValue::kTexture, Texture::Job(vp, job), this,
							ref.ToString());
			} else if (ref.type() == Track::kAudio) {
				AudioParams ap = GetAudioParams(ref.index());
				job.set_audio_params(ap);
				job.set_cache_path(project()->cache_path());

				// Proxies generated with audio contain the video stream at
				// index 0 followed by all source audio streams in source order
				if (proxy_enabled_ && !proxy_path_.isEmpty() &&
					ProxyManager::GetProxyState(proxy_path_) ==
						ProxyManager::kProxyReady &&
					ProxyManager::ProxyFilenameHasAudio(proxy_path_)) {
					int audio_rank = 0;
					for (int i = 0; i < GetTotalStreamCount(); i++) {
						const Track::Reference other =
							GetReferenceFromRealIndex(i);
						if (other.type() == Track::kAudio &&
							GetAudioParams(other.index()).stream_index() <
								ap.stream_index()) {
							audio_rank++;
						}
					}
					job.set_proxy(proxy_path_, QStringLiteral("ffmpeg"),
								  audio_rank + 1);
				}

				table->Push(NodeValue::kSamples, QVariant::fromValue(job), this,
							ref.ToString());
			}
		}
	}
}

QString Footage::GetStreamTypeName(Track::Type type)
{
	switch (type) {
	case Track::kVideo:
		return tr("Video");
	case Track::kAudio:
		return tr("Audio");
	case Track::kSubtitle:
		return tr("Subtitle");
	case Track::kNone:
	case Track::kCount:
		break;
	}

	return tr("Unknown");
}

Node *Footage::GetConnectedTextureOutput()
{
	if (GetVideoStreamCount() > 0) {
		return this;
	} else {
		return nullptr;
	}
}

Node *Footage::GetConnectedSampleOutput()
{
	if (GetAudioStreamCount() > 0) {
		return this;
	} else {
		return nullptr;
	}
}

bool TimeIsOutOfBounds(const rational &time, const rational &length)
{
	return time < 0 || time >= length;
}

rational Footage::AdjustTimeByLoopMode(rational time, LoopMode loop_mode,
									   const rational &length,
									   VideoParams::Type type,
									   const rational &timebase)
{
	if (type == VideoParams::kVideoTypeStill) {
		// No looping for still images
		return 0;
	}

	if (TimeIsOutOfBounds(time, length)) {
		switch (loop_mode) {
		case LoopMode::kLoopModeOff:
			// Return no time to indicate no frame should be shown here
			time = rational::NaN;
			break;
		case LoopMode::kLoopModeClamp:
			if (length < timebase) {
				// No full frame fits in the range, so there is nothing to clamp to
				time = rational::NaN;
			} else {
				// Clamp footage time to length
				time = std::clamp(time, rational(0), length - timebase);
			}
			break;
		case LoopMode::kLoopModeLoop:
			if (length <= 0) {
				// Cannot loop around an empty range
				time = rational::NaN;
			} else {
				// Loop footage time around job length
				do {
					if (time >= length) {
						time -= length;
					} else {
						time += length;
					}
				} while (TimeIsOutOfBounds(time, length));
			}
			break;
		}
	}

	return time;
}

QVariant Footage::data(const DataType &d) const
{
	switch (d) {
	case CREATED_TIME: {
		QFileInfo info(filename());

		if (info.exists()) {
			return QtUtils::GetCreationDate(info).toSecsSinceEpoch();
		}
		break;
	}
	case MODIFIED_TIME: {
		QFileInfo info(filename());

		if (info.exists()) {
			return info.lastModified().toSecsSinceEpoch();
		}
		break;
	}
	case ICON: {
		if (valid_ && GetTotalStreamCount()) {
			// Prioritize video > audio > image
			VideoParams s = GetFirstEnabledVideoStream();

			if (s.is_valid() &&
				s.video_type() != VideoParams::kVideoTypeStill) {
				return icon::Video;
			} else if (HasEnabledAudioStreams()) {
				return icon::Audio;
			} else if (s.is_valid() &&
					   s.video_type() == VideoParams::kVideoTypeStill) {
				return icon::Image;
			} else if (HasEnabledSubtitleStreams()) {
				return icon::Subtitles;
			}
		}

		return icon::Error;
	}
	case TOOLTIP: {
		if (valid_) {
			QString tip = tr("Filename: %1").arg(filename());

			int vp_sz = GetVideoStreamCount();
			for (int i = 0; i < vp_sz; i++) {
				VideoParams p = GetVideoParams(i);

				if (p.enabled()) {
					tip.append("\n");
					tip.append(DescribeVideoStream(p));
				}
			}

			int ap_sz = GetAudioStreamCount();
			for (int i = 0; i < ap_sz; i++) {
				AudioParams p = GetAudioParams(i);

				if (p.enabled()) {
					tip.append("\n");
					tip.append(DescribeAudioStream(p));
				}
			}

			int sp_sz = GetSubtitleStreamCount();
			for (int i = 0; i < sp_sz; i++) {
				SubtitleParams p = GetSubtitleParams(i);

				if (p.enabled()) {
					tip.append("\n");
					tip.append(DescribeSubtitleStream(p));
				}
			}

			return tip;
		} else {
			return tr("Invalid");
		}
	}
	default:
		break;
	}

	return super::data(d);
}

bool Footage::LoadCustom(QXmlStreamReader *reader, SerializedData *data)
{
	while (XMLReadNextStartElement(reader)) {
		if (reader->name() == QStringLiteral("timestamp")) {
			this->set_timestamp(reader->readElementText().toLongLong());
		} else if (reader->name() == QStringLiteral("proxy")) {
			bool enabled = false;
			ProxyManager::ProxyState state = ProxyManager::kProxyMissing;
			int stream = -1;
			int preset_version = 0;
			bool has_custom_params = false;
			ProxyManager::ProxyParams custom_params;
			{
				XMLAttributeLoop(reader, attr)
				{
					if (attr.name() == QStringLiteral("enabled")) {
						enabled = (attr.value() == QStringLiteral("1") ||
								   attr.value() == QStringLiteral("true"));
					} else if (attr.name() == QStringLiteral("state")) {
						state = ProxyManager::ProxyStateFromString(
							attr.value().toString());
					} else if (attr.name() == QStringLiteral("stream")) {
						stream = attr.value().toInt();
					} else if (attr.name() == QStringLiteral("preset")) {
						preset_version = attr.value().toInt();
					} else if (attr.name() == QStringLiteral("custom")) {
						has_custom_params =
							(attr.value() == QStringLiteral("1") ||
							 attr.value() == QStringLiteral("true"));
					} else if (attr.name() == QStringLiteral("pwidth")) {
						custom_params.width = attr.value().toInt();
					} else if (attr.name() == QStringLiteral("pheight")) {
						custom_params.height = attr.value().toInt();
					} else if (attr.name() == QStringLiteral("pcrf")) {
						custom_params.crf = attr.value().toInt();
					} else if (attr.name() == QStringLiteral("ppreset")) {
						custom_params.preset = attr.value().toString();
					} else if (attr.name() == QStringLiteral("pext")) {
						custom_params.extension = attr.value().toString();
					} else if (attr.name() == QStringLiteral("paudio")) {
						custom_params.include_audio =
							(attr.value() == QStringLiteral("1") ||
							 attr.value() == QStringLiteral("true"));
					}
				}
			}

			if (has_custom_params) {
				SetCustomProxyParams(custom_params);
			}

			const QString path = reader->readElementText();
			if (!path.isEmpty()) {
				SetProxy(path, state, stream, preset_version, enabled);
			} else if (enabled) {
				set_proxy_enabled(true);
			}
		} else if (reader->name() == QStringLiteral("sourcestarttime")) {
			QString source;
			{
				XMLAttributeLoop(reader, attr)
				{
					if (attr.name() == QStringLiteral("source")) {
						source = attr.value().toString();
					}
				}
			}

			const QStringList split = reader->readElementText().split('/');
			if (split.size() == 2) {
				bool numerator_ok = false;
				bool denominator_ok = false;
				const int numerator = split.at(0).toInt(&numerator_ok);
				const int denominator = split.at(1).toInt(&denominator_ok);
				if (numerator_ok && denominator_ok && denominator) {
					SetSourceStartTime(rational(numerator, denominator),
									   source);
				}
			}
		} else if (reader->name() == QStringLiteral("viewer")) {
			if (!ViewerOutput::LoadCustom(reader, data)) {
				return false;
			}
		} else {
			reader->skipCurrentElement();
		}
	}

	// The cached lengths are not serialized. Recompute them from the stream
	// parameters that were just loaded so that worker processes and any code
	// that reads GetLength() before InvalidateCache() runs sees valid values.
	VerifyLength();

	return true;
}

void Footage::SaveCustom(QXmlStreamWriter *writer) const
{
	writer->writeTextElement(QStringLiteral("timestamp"),
							 QString::number(this->timestamp()));

	if (!proxy_path_.isEmpty() || proxy_enabled_) {
		writer->writeStartElement(QStringLiteral("proxy"));
		writer->writeAttribute(QStringLiteral("enabled"),
							   proxy_enabled_ ? QStringLiteral("1") :
												QStringLiteral("0"));
		writer->writeAttribute(QStringLiteral("state"),
							   ProxyManager::ProxyStateToString(proxy_state_));
		writer->writeAttribute(QStringLiteral("stream"),
							   QString::number(proxy_video_stream_index_));
		writer->writeAttribute(QStringLiteral("preset"),
							   QString::number(proxy_preset_version_));
		if (has_custom_proxy_params_) {
			writer->writeAttribute(QStringLiteral("custom"),
								   QStringLiteral("1"));
			writer->writeAttribute(
				QStringLiteral("pwidth"),
				QString::number(custom_proxy_params_.width));
			writer->writeAttribute(
				QStringLiteral("pheight"),
				QString::number(custom_proxy_params_.height));
			writer->writeAttribute(
				QStringLiteral("pcrf"),
				QString::number(custom_proxy_params_.crf));
			writer->writeAttribute(QStringLiteral("ppreset"),
								   custom_proxy_params_.preset);
			writer->writeAttribute(QStringLiteral("pext"),
								   custom_proxy_params_.extension);
			writer->writeAttribute(
				QStringLiteral("paudio"),
				custom_proxy_params_.include_audio ? QStringLiteral("1") :
													 QStringLiteral("0"));
		}
		writer->writeCharacters(proxy_path_);
		writer->writeEndElement();
	}

	if (has_source_start_time_) {
		writer->writeStartElement(QStringLiteral("sourcestarttime"));
		writer->writeAttribute(QStringLiteral("source"),
							   source_start_time_source_);
		writer->writeCharacters(QStringLiteral("%1/%2").arg(
			QString::number(source_start_time_.numerator()),
			QString::number(source_start_time_.denominator())));
		writer->writeEndElement();
	}

	writer->writeStartElement(QStringLiteral("viewer"));

	ViewerOutput::SaveCustom(writer);

	writer->writeEndElement(); // viewer
}

void Footage::AddedToGraphEvent(Project *p)
{
	connect(p->color_manager(), &ColorManager::DefaultInputChanged, this,
			&Footage::DefaultColorSpaceChanged);
	if (ProxyManager::instance()) {
		connect(ProxyManager::instance(), &ProxyManager::ProxyReady, this,
				&Footage::ProxyReady);
		connect(ProxyManager::instance(), &ProxyManager::ProxyFinished, this,
				&Footage::ProxyFinished);
	}
}

void Footage::RemovedFromGraphEvent(Project *p)
{
	disconnect(p->color_manager(), &ColorManager::DefaultInputChanged, this,
			   &Footage::DefaultColorSpaceChanged);
	if (ProxyManager::instance()) {
		disconnect(ProxyManager::instance(), &ProxyManager::ProxyReady, this,
				   &Footage::ProxyReady);
		disconnect(ProxyManager::instance(), &ProxyManager::ProxyFinished, this,
				   &Footage::ProxyFinished);
	}
}

void Footage::Reprobe()
{
	// Determine if file still exists
	QString filename = this->filename();

	// In case of failure to load file, set timestamp to a value that will always be invalid so we
	// continuously reprobe
	set_timestamp(0);

	if (!filename.isEmpty()) {
		QFileInfo info(filename);

		if (info.exists()) {
			// Grab timestamp
			set_timestamp(info.lastModified().toMSecsSinceEpoch());

			// Determine if we've already cached the metadata of this file
			QString meta_cache_file =
				QDir(QStandardPaths::writableLocation(
						 QStandardPaths::CacheLocation))
					.filePath(FileFunctions::GetUniqueFileIdentifier(filename));

			FootageDescription footage_info;

			// Try to load footage info from cache
			if (!QFileInfo::exists(meta_cache_file) ||
				!footage_info.Load(meta_cache_file)) {
				// Probe and create cache
				QVector<DecoderPtr> decoder_list =
					Decoder::ReceiveListOfAllDecoders();

				foreach (DecoderPtr decoder, decoder_list) {
					footage_info = decoder->Probe(filename, cancelled_);

					if (footage_info.IsValid()) {
						break;
					}
				}

				if (!cancelled_ || !cancelled_->HeardCancel()) {
					// Only cache successful probes; caching a failed probe
					// would make every future load re-use the invalid metadata
					if (footage_info.IsValid() &&
						!footage_info.Save(meta_cache_file)) {
						qWarning()
							<< "Failed to save stream cache, footage will have to be re-probed";
					}
				}
			}

			if (footage_info.IsValid()) {
				decoder_ = footage_info.decoder();

				InputArrayResize(kVideoParamsInput,
								 footage_info.GetVideoStreams().size());
				for (int i = 0; i < footage_info.GetVideoStreams().size();
					 i++) {
					VideoParams video_stream =
						footage_info.GetVideoStreams().at(i);

					if (i < InputArraySize(kVideoParamsInput)) {
						VideoParams existing = this->GetVideoParams(i);
						if (existing.is_valid()) {
							video_stream =
								MergeVideoStream(video_stream, existing);
						}
					}

					SetStream(Track::kVideo, QVariant::fromValue(video_stream),
							  i);
				}

				InputArrayResize(kAudioParamsInput,
								 footage_info.GetAudioStreams().size());
				for (int i = 0; i < footage_info.GetAudioStreams().size();
					 i++) {
					SetStream(Track::kAudio,
							  QVariant::fromValue(
								  footage_info.GetAudioStreams().at(i)),
							  i);
				}

				InputArrayResize(kSubtitleParamsInput,
								 footage_info.GetSubtitleStreams().size());
				for (int i = 0; i < footage_info.GetSubtitleStreams().size();
					 i++) {
					SetStream(Track::kSubtitle,
							  QVariant::fromValue(
								  footage_info.GetSubtitleStreams().at(i)),
							  i);
				}

				total_stream_count_ = footage_info.GetStreamCount();
				if (footage_info.HasSourceStartTime()) {
					SetSourceStartTime(footage_info.source_start_time(),
									   footage_info.source_start_time_source());
				}

				SetValid();
			}
		}
	}
}

VideoParams Footage::MergeVideoStream(const VideoParams &base,
									  const VideoParams &over)
{
	VideoParams merged = base;

	merged.set_pixel_aspect_ratio(over.pixel_aspect_ratio());
	merged.set_interlacing(over.interlacing());
	merged.set_colorspace(over.colorspace());
	merged.set_premultiplied_alpha(over.premultiplied_alpha());
	merged.set_video_type(over.video_type());
	merged.set_color_range(over.color_range());
	if (merged.video_type() == VideoParams::kVideoTypeImageSequence) {
		merged.set_start_time(over.start_time());
		merged.set_duration(over.duration());
		merged.set_frame_rate(over.frame_rate());
		merged.set_time_base(over.time_base());
	}

	return merged;
}

void Footage::CheckFootage()
{
	// Don't check files if not the active window
	if (qApp->activeWindow()) {
		QString fn = filename();

		if (!fn.isEmpty()) {
			QFileInfo info(fn);

			qint64 current_file_timestamp;
			if (!info.lastModified().isValid()) {
				current_file_timestamp = 0;
			} else {
				current_file_timestamp =
					info.lastModified().toMSecsSinceEpoch();
			}

			if (current_file_timestamp != timestamp()) {
				// File has changed!
				Clear();
				Reprobe();
				InvalidateAll(kFilenameInput);
			}
		}
	}
}

void Footage::DefaultColorSpaceChanged()
{
	bool inv = false;
	int sz = GetVideoStreamCount();
	for (int i = 0; i < sz; i++) {
		// Check if any of our streams are using the default colorspace
		if (GetVideoParams(i).colorspace().isEmpty()) {
			inv = true;
			break;
		}
	}

	if (inv) {
		InvalidateAll(kVideoParamsInput);
	}
}

void Footage::ProxyReady(const QString &source_filename, int stream_index,
						 const QString &proxy_filename)
{
	ProxyFinished(source_filename, stream_index, proxy_filename,
				  ProxyManager::kProxyReady);
}

void Footage::ProxyFinished(const QString &source_filename, int stream_index,
							const QString &proxy_filename,
							ProxyManager::ProxyState state)
{
	if (filename() != source_filename ||
		proxy_video_stream_index_ != stream_index ||
		proxy_path_ != proxy_filename) {
		return;
	}

	proxy_state_ = state;
	InvalidateAll(kFilenameInput);
}

}
