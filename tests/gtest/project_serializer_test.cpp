#include <gtest/gtest.h>

#include <QBuffer>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "node/factory.h"
#include "node/project.h"
#include "node/input/time/timeinput.h"
#include "node/project/serializer/serializer.h"
#include "node/color/colormanager/colormanager.h"
#include "render/diskmanager.h"

TEST(ProjectSerializer, SaveLoadProjectRoundTrip)
{
	const bool created_disk_manager =
		(olive::DiskManager::instance() == nullptr);
	if (created_disk_manager) {
		olive::DiskManager::CreateInstance();
	}

	olive::ColorManager::SetUpDefaultConfig();
	olive::NodeFactory::Initialize();
	olive::ProjectSerializer::Initialize();

	olive::Project project;
	project.Initialize();

	auto *node = new olive::TimeInput();
	node->SetLabel(QStringLiteral("TimeInput"));
	node->setParent(&project);

	olive::ProjectSerializer::SaveData save_data(
		olive::ProjectSerializer::kProject, &project, QString());

	QByteArray xml;
	QBuffer buffer(&xml);
	buffer.open(QIODevice::WriteOnly);
	QXmlStreamWriter writer(&buffer);
	olive::ProjectSerializer::Result save_result =
		olive::ProjectSerializer::Save(&writer, save_data);
	EXPECT_EQ(save_result.code(), olive::ProjectSerializer::kSuccess);
	buffer.close();

	olive::Project loaded_project;
	QBuffer read_buffer(&xml);
	read_buffer.open(QIODevice::ReadOnly);
	QXmlStreamReader reader(&read_buffer);
	olive::ProjectSerializer::Result result = olive::ProjectSerializer::Load(
		&loaded_project, &reader, olive::ProjectSerializer::kProject);
	EXPECT_EQ(result.code(), olive::ProjectSerializer::kSuccess);
	EXPECT_FALSE(loaded_project.nodes().isEmpty());
	ASSERT_TRUE(result.GetLoadData().node_ptrs.contains(
		reinterpret_cast<quintptr>(node)));
	EXPECT_TRUE(
		loaded_project.nodes().contains(result.GetLoadData().node_ptrs.value(
			reinterpret_cast<quintptr>(node))));

	olive::ProjectSerializer::Destroy();
	if (created_disk_manager) {
		olive::DiskManager::DestroyInstance();
	}
}
