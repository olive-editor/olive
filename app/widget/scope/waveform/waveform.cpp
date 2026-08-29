
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

#include "waveform.h"

#include <QPainter>
#include <QtMath>
#include <QDebug>
#include <QVector2D>
#include <QVector3D>

#include "common/qtutils.h"
#include "config/config.h"
#include "node/node.h"

namespace olive
{

#define super ScopeBase

WaveformScope::WaveformScope(QWidget *parent)
	: super(parent)
{
}

ShaderCode WaveformScope::GenerateShaderCode()
{
	return ShaderCode(
		FileFunctions::ReadFileAsString(":/shaders/rgbwaveform.frag"),
		FileFunctions::ReadFileAsString(":/shaders/rgbwaveform.vert"));
}

void WaveformScope::DrawScope(TexturePtr managed_tex, QVariant pipeline)
{
	float waveform_scale = 0.80f;

	// Draw waveform through shader
	ShaderJob job;

	// Set viewport size
	job.Insert(QStringLiteral("viewport"),
			   NodeValue(NodeValue::kVec2, QVector2D(width(), height())));

	// Set luma coefficients
	double luma_coeffs[3] = { 0.0f, 0.0f, 0.0f };
	color_manager()->GetDefaultLumaCoefs(luma_coeffs);
	job.Insert(
		QStringLiteral("luma_coeffs"),
		NodeValue(NodeValue::kVec3,
				  QVector3D(luma_coeffs[0], luma_coeffs[1], luma_coeffs[2])));

	// Scale of the waveform relative to the viewport surface.
	job.Insert(QStringLiteral("waveform_scale"),
			   NodeValue(NodeValue::kFloat, waveform_scale));

	// Insert source texture
	job.Insert(QStringLiteral("ove_maintex"),
			   NodeValue(NodeValue::kTexture,
						 QVariant::fromValue(managed_tex)));

	renderer()->Blit(pipeline, job, GetViewportParams());

	float waveform_dim_x = ceil((width() - 1.0) * waveform_scale);
	float waveform_dim_y = ceil((height() - 1.0) * waveform_scale);
	float waveform_start_dim_x = ((width() - 1.0) - waveform_dim_x) / 2.0f;
	float waveform_start_dim_y = ((height() - 1.0) - waveform_dim_y) / 2.0f;
	float waveform_end_dim_x = (width() - 1.0) - waveform_start_dim_x;

	// Draw line overlays
	QPainter p(paint_device());
	QFont font;
	font.setPixelSize(10);
	QFontMetrics font_metrics = QFontMetrics(font);
	QString label;
	float ire_increment = 0.1f;
	int ire_steps = qRound(1.0 / ire_increment);
	QVector<QLine> ire_lines(ire_steps + 1);
	int font_x_offset = 0;
	int font_y_offset = font_metrics.capHeight() / 2.0f;

	p.setCompositionMode(QPainter::CompositionMode_Plus);

	p.setPen(QColor(0.0, 0.6 * 255.0, 0.0));
	p.setFont(font);

	for (int i = 0; i <= ire_steps; i++) {
		ire_lines[i].setLine(
			waveform_start_dim_x,
			(waveform_dim_y * (i * ire_increment)) + waveform_start_dim_y,
			waveform_end_dim_x,
			(waveform_dim_y * (i * ire_increment)) + waveform_start_dim_y);
		label = QString::number(1.0 - (i * ire_increment), 'f', 1);
		font_x_offset = QtUtils::QFontMetricsWidth(font_metrics, label) + 4;

		p.drawText(waveform_start_dim_x - font_x_offset,
				   (waveform_dim_y * (i * ire_increment)) +
					   waveform_start_dim_y + font_y_offset,
				   label);
	}

	p.drawLines(ire_lines);
}

void WaveformScope::DrawScopeSoftware(QPainter &p, const QImage &image)
{
	const float waveform_scale = 0.80f;
	const int waveform_dim_x = qCeil((width() - 1.0) * waveform_scale);
	const int waveform_dim_y = qCeil((height() - 1.0) * waveform_scale);
	const int waveform_start_dim_x =
		qFloor(((width() - 1.0) - waveform_dim_x) / 2.0f);
	const int waveform_start_dim_y =
		qFloor(((height() - 1.0) - waveform_dim_y) / 2.0f);
	const int waveform_end_dim_x = width() - 1 - waveform_start_dim_x;

	QImage buf(width(), height(), QImage::Format_ARGB32_Premultiplied);
	buf.fill(Qt::transparent);

	const int src_w = image.width();
	const int src_h = image.height();

	// Limit analysis resolution to keep CPU usage reasonable on large frames.
	const int step_x = qMax(1, src_w / 512);
	const int step_y = qMax(1, src_h / 512);

	for (int sy = 0; sy < src_h; sy += step_y) {
		const uchar *src_line = image.constScanLine(sy);
		for (int sx = 0; sx < src_w; sx += step_x) {
			const uchar *src = src_line + sx * 4;
			float r = src[0] / 255.0f;
			float g = src[1] / 255.0f;
			float b = src[2] / 255.0f;

			int scope_x = waveform_start_dim_x +
						  int((float(sx) / float(src_w)) * waveform_dim_x);
			if (scope_x < waveform_start_dim_x ||
				scope_x >= waveform_end_dim_x) {
				continue;
			}

			auto mark = [&](float value, int add_r, int add_g, int add_b) {
				int scope_y =
					waveform_start_dim_y + int((1.0f - value) * waveform_dim_y);
				if (scope_y < waveform_start_dim_y ||
					scope_y >= waveform_start_dim_y + waveform_dim_y) {
					return;
				}
				QRgb *dst_line =
					reinterpret_cast<QRgb *>(buf.scanLine(scope_y));
				QRgb cur = dst_line[scope_x];
				int nr = qMin(255, qRed(cur) + add_r);
				int ng = qMin(255, qGreen(cur) + add_g);
				int nb = qMin(255, qBlue(cur) + add_b);
				int na = qMax(qMax(nr, ng), nb);
				dst_line[scope_x] = qRgba(nr, ng, nb, na);
			};

			mark(r, 30, 0, 0);
			mark(g, 0, 30, 0);
			mark(b, 0, 0, 30);
		}
	}

	p.setCompositionMode(QPainter::CompositionMode_Plus);
	p.drawImage(0, 0, buf);

	// Draw IRE line overlays
	QFont font;
	font.setPixelSize(10);
	QFontMetrics font_metrics = QFontMetrics(font);
	QString label;
	float ire_increment = 0.1f;
	int ire_steps = qRound(1.0 / ire_increment);
	QVector<QLine> ire_lines(ire_steps + 1);
	int font_x_offset = 0;
	int font_y_offset = font_metrics.capHeight() / 2.0f;

	p.setPen(QColor(0.0, 0.6 * 255.0, 0.0));
	p.setFont(font);

	for (int i = 0; i <= ire_steps; i++) {
		ire_lines[i].setLine(
			waveform_start_dim_x,
			(waveform_dim_y * (i * ire_increment)) + waveform_start_dim_y,
			waveform_end_dim_x,
			(waveform_dim_y * (i * ire_increment)) + waveform_start_dim_y);
		label = QString::number(1.0 - (i * ire_increment), 'f', 1);
		font_x_offset = QtUtils::QFontMetricsWidth(font_metrics, label) + 4;

		p.drawText(waveform_start_dim_x - font_x_offset,
				   (waveform_dim_y * (i * ire_increment)) +
					   waveform_start_dim_y + font_y_offset,
				   label);
	}

	p.drawLines(ire_lines);
}

}
