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
 *
 */

//
// Created by mikesolar on 25-10-1.
//

#ifndef OLIVECLIP_H
#define OLIVECLIP_H
#include "image.h"
#include "ofxCore.h"
#include "ofxhClip.h"
#include "render/texture.h"
#include "render/videoparams.h"

#include <QMap>
#include <memory>
namespace olive
{
namespace plugin
{
class OliveClipInstance : public OFX::Host::ImageEffect::ClipInstance {
public:
	OliveClipInstance(OFX::Host::ImageEffect::Instance *effectInstance,
					  OFX::Host::ImageEffect::ClipDescriptor &desc,
					  VideoParams &params)
		: ClipInstance(effectInstance, desc)
		, params_(params)
		, defaultRegionOfDefinitions_{ 0, 0, 0, 0 }
		, name_(desc.getName())
	{
	}
	OFX::Host::ImageEffect::Image *getOutputImage(OfxTime time);

	const std::string &getUnmappedBitDepth() const override;
	const std::string &getUnmappedComponents() const override;
	const std::string &getPremult() const override;
	double getAspectRatio() const override;
	double getFrameRate() const override;
	void getFrameRange(double &startFrame, double &endFrame) const override;
	const std::string &getFieldOrder() const override;
	bool getConnected() const override;
	double getUnmappedFrameRate() const override;
	void getUnmappedFrameRange(double &startFrame,
							   double &endFrame) const override;
	bool getContinuousSamples() const override;
	OFX::Host::ImageEffect::Image *
	getImage(OfxTime time, const OfxRectD *optionalBounds) override;
	OfxRectD getRegionOfDefinition(OfxTime time) const override;

	void setRegionOfDefinition(OfxRectD regionOfDefinition, OfxTime time);
	void setDefaultRegionOfDefinition(OfxRectD regionOfDefinition);
	void setParams(const VideoParams &params);
#ifdef OFX_SUPPORTS_OPENGLRENDER
	OFX::Host::ImageEffect::Texture *
	loadTexture(OfxTime time, const char *format,
				const OfxRectD *optionalBounds) override;
#endif

	void setInputTexture(TexturePtr texture, OfxTime time,
						 bool readback_cpu = true);
	void setOutputTexture(TexturePtr texture, OfxTime time);

	// Get the plugin-preferred VideoParams based on base class _pixelDepth/_components
	VideoParams getPluginPreferredParams() const;

	// Prune old entries from the images_ cache to prevent unbounded growth.
	// Output clip images are not pruned (they are typically single-frame).
	void pruneImagesCache();

	static constexpr int kMaxInputImageCache = 8;

private:
	VideoParams params_;

	QMap<OfxTime, OfxRectD> regionOfDefinitions_;

	OfxRectD defaultRegionOfDefinitions_;

	std::string name_;
	QMap<OfxTime, Image *> images_;
#ifdef OFX_SUPPORTS_OPENGLRENDER
	QMap<OfxTime, TexturePtr> input_textures_;
	QMap<OfxTime, TexturePtr> output_textures_;
#endif
};
}
}

#endif //OLIVECLIP_H
