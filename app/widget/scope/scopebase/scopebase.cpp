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

#include "scopebase.h"

#include "config/config.h"
#include "render/job/colortransformjob.h"

namespace olive
{

#define super ManagedDisplayWidget

ScopeBase::ScopeBase(QWidget *parent)
	: super(parent)
	, texture_(nullptr)
	, managed_tex_up_to_date_(false)
	, software_image_up_to_date_(false)
{
	EnableDefaultContextMenu();
}

void ScopeBase::SetBuffer(TexturePtr frame)
{
	texture_ = frame;
	managed_tex_up_to_date_ = false;
	software_image_up_to_date_ = false;
	update();
}

void ScopeBase::showEvent(QShowEvent *e)
{
	super::showEvent(e);
}

void ScopeBase::DrawScope(TexturePtr managed_tex, QVariant pipeline)
{
	ShaderJob job;

	job.Insert(QStringLiteral("ove_maintex"),
			   NodeValue(NodeValue::kTexture,
						 QVariant::fromValue(managed_tex)));

	renderer()->Blit(pipeline, job, GetViewportParams());
}

void ScopeBase::UpdateSoftwareImage()
{
	if (!texture_ || texture_->IsDummy() || !renderer()) {
		software_image_ = QImage();
		software_image_up_to_date_ = true;
		return;
	}

	// Backend-neutral widgets each have their own renderer instance (e.g. a
	// separate Vulkan device). The reference texture emitted by the viewer lives
	// in the viewer's renderer, so we must download it to the CPU and re-upload
	// it into this scope's renderer before we can sample it.
	TexturePtr source_tex = texture_;
	if (texture_->renderer() && texture_->renderer() != renderer()) {
		FramePtr temp_frame = Frame::Create();
		temp_frame->set_video_params(texture_->params());
		temp_frame->allocate();
		texture_->Download(temp_frame->data(), temp_frame->linesize_pixels());

		local_texture_ = renderer()->CreateTexture(
			temp_frame->video_params(), temp_frame->data(),
			temp_frame->linesize_pixels());
		source_tex = local_texture_;
	}

	if (!source_tex || source_tex->IsDummy()) {
		software_image_ = QImage();
		software_image_up_to_date_ = true;
		return;
	}

	const int texture_width = static_cast<int>(width() * devicePixelRatioF());
	const int texture_height = static_cast<int>(height() * devicePixelRatioF());

	const VideoParams offscreen_params(texture_width, texture_height,
									   PixelFormat::U8,
									   VideoParams::kRGBAChannelCount);

	if (!software_tex_ || software_tex_->params() != offscreen_params) {
		software_tex_ = renderer()->CreateTexture(offscreen_params);
		software_buffer_.resize(
			texture_width * texture_height *
			VideoParams::GetBytesPerPixel(PixelFormat::U8,
										  VideoParams::kRGBAChannelCount));
	}

	if (!software_tex_ || software_tex_->IsDummy()) {
		software_image_ = QImage();
		software_image_up_to_date_ = true;
		return;
	}

	ColorTransformJob job;
	job.SetColorProcessor(color_service());
	job.SetInputTexture(source_tex);
	job.SetInputAlphaAssociation(kAlphaNone);
	job.SetClearDestinationEnabled(true);
	job.SetForceOpaque(true);

	renderer()->BlitColorManaged(job, software_tex_.get());
	renderer()->DownloadFromTexture(software_tex_->id(),
									software_tex_->params(),
									software_buffer_.data(), 0);

	software_image_ = QImage(
		reinterpret_cast<const uchar *>(software_buffer_.constData()),
		texture_width, texture_height,
		texture_width * VideoParams::GetBytesPerPixel(
							PixelFormat::U8, VideoParams::kRGBAChannelCount),
		QImage::Format_RGBA8888_Premultiplied);
	software_image_.setDevicePixelRatio(devicePixelRatioF());

	software_image_up_to_date_ = true;
}

void ScopeBase::OnInit()
{
	super::OnInit();

	if (!IsBackendNeutral()) {
		pipeline_ = renderer()->CreateNativeShader(GenerateShaderCode());
	}
}

void ScopeBase::OnPaint()
{
	if (IsBackendNeutral()) {
		if (!software_image_up_to_date_) {
			UpdateSoftwareImage();
		}

		QPainter p(paint_device());
		p.fillRect(rect(), Qt::black);

		if (!software_image_.isNull()) {
			DrawScopeSoftware(p, software_image_);
		}
		return;
	}

	// Clear display surface
	renderer()->ClearDestination();

	if (texture_) {
		// Convert reference frame to display space
		if (!managed_tex_ || !managed_tex_up_to_date_ ||
			managed_tex_->params() != texture_->params()) {
			managed_tex_ = renderer()->CreateTexture(texture_->params());

			ColorTransformJob job;
			job.SetColorProcessor(color_service());
			job.SetInputTexture(texture_);
			job.SetInputAlphaAssociation(kAlphaNone);

			renderer()->BlitColorManaged(job, managed_tex_.get());
			managed_tex_up_to_date_ = true;
		}

		DrawScope(managed_tex_, pipeline_);
	}
}

void ScopeBase::OnDestroy()
{
	local_texture_ = nullptr;
	software_tex_ = nullptr;
	software_buffer_.clear();
	software_image_ = QImage();
	managed_tex_ = nullptr;
	texture_ = nullptr;
	pipeline_.clear();

	super::OnDestroy();
}

}
