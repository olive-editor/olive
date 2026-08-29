#include <gtest/gtest.h>

#include "node/traverser.h"
#include "node/value.h"
#include "render/job/pluginjob.h"
#include "render/texture.h"
#include "render/videoparams.h"

namespace
{

class PluginJobTraverser : public olive::NodeTraverser {
public:
	void Resolve(olive::NodeValue &value)
	{
		ResolveJobs(value);
	}

	bool called() const
	{
		return called_;
	}

protected:
	olive::TexturePtr ProcessPluginJob(olive::TexturePtr /*texture*/,
									   olive::TexturePtr destination,
									   const olive::Node * /*node*/) override
	{
		called_ = true;
		return destination;
	}

private:
	bool called_ = false;
};

} // namespace

TEST(PluginRenderPipeline, PluginJobIsResolved)
{
	olive::VideoParams params(320, 240, olive::core::PixelFormat::U8, 4);

	olive::plugin::PluginJob job(nullptr, nullptr, olive::NodeValueRow());
	olive::TexturePtr job_tex = olive::Texture::Job(params, job);

	olive::NodeValue val(olive::NodeValue::kTexture, job_tex);

	PluginJobTraverser traverser;
	traverser.SetCacheVideoParams(params);
	traverser.Resolve(val);

	EXPECT_TRUE(traverser.called());
	ASSERT_TRUE(val.toTexture());
	EXPECT_NE(val.toTexture().get(), job_tex.get());
}
