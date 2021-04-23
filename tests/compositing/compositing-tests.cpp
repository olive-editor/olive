/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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

#include "testutil.h"

#include "node/distort/crop/cropdistortnode.h"
#include "node/distort/transform/transformdistortnode.h"
#include "node/generator/solid/solid.h"
#include "node/math/merge/merge.h"
#include "node/project/project.h"
#include "render/rendermanager.h"

namespace olive {

OLIVE_ADD_TEST(MergeRGBOptimization)
{
  Project project;

  // Solids generate RGB buffers, which the merge node should optimize by passing through the blend
  SolidGenerator* source_1 = new SolidGenerator();
  source_1->setParent(&project);
  source_1->SetStandardValue(SolidGenerator::kColorInput, QVariant::fromValue(Color(1.0, 0.0, 0.0)));

  SolidGenerator* source_2 = new SolidGenerator();
  source_2->setParent(&project);
  source_2->SetStandardValue(SolidGenerator::kColorInput, QVariant::fromValue(Color(0.0, 1.0, 0.0)));

  // Crops add an alpha channel if there isn't one already
  CropDistortNode* crop = new CropDistortNode();
  crop->setParent(&project);
  crop->SetStandardValue(CropDistortNode::kLeftInput, 0.5);

  MergeNode* merge_1 = new MergeNode();
  merge_1->setParent(&project);

  MergeNode* merge_2 = new MergeNode();
  merge_2->setParent(&project);

  MergeNode* merge_3 = new MergeNode();
  merge_3->setParent(&project);

  MergeNode* merge_4 = new MergeNode();
  merge_4->setParent(&project);

  // Force an alpha channel by passing the solid through a crop
  Node::ConnectEdge(source_2, NodeInput(crop, CropDistortNode::kTextureInput));

  // Merge 1 is source 1 over source 2 - source 1 should be passed through because it's RGB
  Node::ConnectEdge(source_2, NodeInput(merge_1, MergeNode::kBaseIn));
  Node::ConnectEdge(source_1, NodeInput(merge_1, MergeNode::kBlendIn));

  // Merge 2 is source 2 over merge 1 - source 2 should be passed through because it's RGB
  Node::ConnectEdge(merge_1, NodeInput(merge_2, MergeNode::kBaseIn));
  Node::ConnectEdge(source_2, NodeInput(merge_2, MergeNode::kBlendIn));

  // Merge 3 is crop over merge 2 - merge should actually occur because crop is RGBA
  Node::ConnectEdge(merge_2, NodeInput(merge_3, MergeNode::kBaseIn));
  Node::ConnectEdge(crop, NodeInput(merge_3, MergeNode::kBlendIn));

  // Merge 4 is merge 3 over source 2 - merge 3 should be passed through because even though its
  // blend is RGBA, its base is RGB so no alpha channel should have been added
  Node::ConnectEdge(source_2, NodeInput(merge_4, MergeNode::kBaseIn));
  Node::ConnectEdge(merge_3, NodeInput(merge_4, MergeNode::kBlendIn));

  // Generate hashes for all these merges
  VideoParams params(1920, 1080, rational(1, 30), VideoParams::kFormatFloat16, VideoParams::kRGBAChannelCount);

  QByteArray source_1_hash = RenderManager::Hash(source_1, SolidGenerator::kDefaultOutput, params, 0);
  QByteArray source_2_hash = RenderManager::Hash(source_2, SolidGenerator::kDefaultOutput, params, 0);
  QByteArray merge_1_hash = RenderManager::Hash(merge_1, MergeNode::kDefaultOutput, params, 0);
  QByteArray merge_2_hash = RenderManager::Hash(merge_2, MergeNode::kDefaultOutput, params, 0);
  QByteArray merge_3_hash = RenderManager::Hash(merge_3, MergeNode::kDefaultOutput, params, 0);
  QByteArray merge_4_hash = RenderManager::Hash(merge_4, MergeNode::kDefaultOutput, params, 0);
  QByteArray crop_hash = RenderManager::Hash(crop, CropDistortNode::kDefaultOutput, params, 0);

  // Merge 1 should pass through to source 1 since source 1 is RGB and its blend
  OLIVE_ASSERT(source_1_hash == merge_1_hash);

  // Merge 2 should pass through to source 2 since source 2 is RGB and its blend
  OLIVE_ASSERT(source_2_hash == merge_2_hash);

  // Merge 3's blend actually has an alpha channel, so it should be different
  OLIVE_ASSERT(merge_3_hash != crop_hash);
  OLIVE_ASSERT(merge_3_hash != merge_2_hash);

  // Merge 4 should be identical to merge 3 despite going through far less nodes
  OLIVE_ASSERT(merge_4_hash == merge_3_hash);

  OLIVE_TEST_END;
}

}
