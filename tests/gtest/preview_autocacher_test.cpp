#include <gtest/gtest.h>

#include <QSignalSpy>

#include "codec/conformmanager.h"
#include "node/output/viewer/viewer.h"
#include "node/project.h"
#include "render/diskmanager.h"
#include "render/previewautocacher.h"
#include "render/rendermanager.h"

using namespace olive;

class PreviewAutoCacherTest : public ::testing::Test {
protected:
	void SetUp() override
	{
		ColorManager::SetUpDefaultConfig();

		// Use the dummy render backend so PreviewAutoCacher can be exercised
		// without initializing OpenGL/Vulkan in the unit-test process.
		OLIVE_CONFIG("GraphicsBackend") = QStringLiteral("dummy");

		DiskManager::CreateInstance();
		ConformManager::CreateInstance();
		RenderManager::CreateInstance();

		project_ = std::make_unique<Project>();
		project_->Initialize();
	}

	void TearDown() override
	{
		RenderManager::DestroyInstance();
		ConformManager::DestroyInstance();
		DiskManager::DestroyInstance();
	}

	std::unique_ptr<Project> project_;
};

TEST_F(PreviewAutoCacherTest, ConstructionInitializesDefaultState)
{
	PreviewAutoCacher cacher;
	EXPECT_FALSE(cacher.IsRenderingCustomRange());
}

TEST_F(PreviewAutoCacherTest, SetProjectToNullDoesNotCrash)
{
	PreviewAutoCacher cacher;
	cacher.SetProject(project_.get());
	cacher.SetProject(nullptr);
	EXPECT_FALSE(cacher.IsRenderingCustomRange());
}

TEST_F(PreviewAutoCacherTest, SetRendersPausedTogglesState)
{
	PreviewAutoCacher cacher;
	cacher.SetRendersPaused(true);
	cacher.SetRendersPaused(false);
}

TEST_F(PreviewAutoCacherTest, SetThumbnailsPausedTogglesState)
{
	PreviewAutoCacher cacher;
	cacher.SetThumbnailsPaused(true);
	cacher.SetThumbnailsPaused(false);
}

TEST_F(PreviewAutoCacherTest, SetPlayheadStoresPlayhead)
{
	PreviewAutoCacher cacher;
	cacher.SetPlayhead(rational(42));
}

TEST_F(PreviewAutoCacherTest, ClearSingleFrameRendersDoesNotCrashWhenEmpty)
{
	PreviewAutoCacher cacher;
	cacher.ClearSingleFrameRenders();
}

TEST_F(PreviewAutoCacherTest,
	   ClearSingleFrameRendersThatArentRunningDoesNotCrashWhenEmpty)
{
	PreviewAutoCacher cacher;
	cacher.ClearSingleFrameRendersThatArentRunning();
}

TEST_F(PreviewAutoCacherTest, GetSingleFrameWithoutProjectReturnsTicket)
{
	auto *viewer = new ViewerOutput();
	viewer->setParent(project_.get());

	PreviewAutoCacher cacher;
	RenderTicketPtr ticket = cacher.GetSingleFrame(viewer, rational(0));
	EXPECT_NE(ticket, nullptr);
}

TEST_F(PreviewAutoCacherTest, ForceCacheRangeDoesNotCrash)
{
	auto *viewer = new ViewerOutput();
	viewer->setParent(project_.get());

	PreviewAutoCacher cacher;
	cacher.ForceCacheRange(viewer, TimeRange(rational(0), rational(1)));
	cacher.SetProject(nullptr);
}

TEST_F(PreviewAutoCacherTest, CancelVideoTasksDoesNotCrashWhenIdle)
{
	PreviewAutoCacher cacher;
	cacher.CancelVideoTasks(false);
	cacher.CancelVideoTasks(true);
}

TEST_F(PreviewAutoCacherTest, CancelAudioTasksDoesNotCrashWhenIdle)
{
	PreviewAutoCacher cacher;
	cacher.CancelAudioTasks(false);
	cacher.CancelAudioTasks(true);
}
