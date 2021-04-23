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

  MergeNode* merge_1 = new MergeNode();
  merge_1->setParent(&project);

  MergeNode* merge_2 = new MergeNode();
  merge_2->setParent(&project);

  Node::ConnectEdge(source_2, NodeInput(merge_1, MergeNode::kBaseIn));
  Node::ConnectEdge(source_1, NodeInput(merge_1, MergeNode::kBlendIn));
  Node::ConnectEdge(source_1, NodeInput(merge_2, MergeNode::kBaseIn));
  Node::ConnectEdge(source_2, NodeInput(merge_2, MergeNode::kBlendIn));

  VideoParams params(1920, 1080, rational(1, 30), VideoParams::kFormatFloat16, VideoParams::kRGBAChannelCount);

  QByteArray source_1_hash = RenderManager::Hash(source_1, SolidGenerator::kDefaultOutput, params, 0);
  QByteArray source_2_hash = RenderManager::Hash(source_2, SolidGenerator::kDefaultOutput, params, 0);
  QByteArray merge_1_hash = RenderManager::Hash(merge_1, MergeNode::kDefaultOutput, params, 0);
  QByteArray merge_2_hash = RenderManager::Hash(merge_2, MergeNode::kDefaultOutput, params, 0);

  // Merge 1 should pass through to source 1 since source 1 is RGB and its blend
  OLIVE_ASSERT(source_1_hash == merge_1_hash);

  // Merge 2 should pass through to source 2 since source 2 is RGB and its blend
  OLIVE_ASSERT(source_2_hash == merge_2_hash);

  OLIVE_TEST_END;
}

}
