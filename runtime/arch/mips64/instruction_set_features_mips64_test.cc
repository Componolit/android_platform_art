/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "instruction_set_features_mips64.h"

#include <gtest/gtest.h>

namespace art {

TEST(Mips64InstructionSetFeaturesTest, Mips64Features) {
  std::string error_msg;
  std::unique_ptr<const InstructionSetFeatures> mips64_features(
      InstructionSetFeatures::FromVariant(kMips64, "default", &error_msg));
  ASSERT_TRUE(mips64_features.get() != nullptr) << error_msg;
  EXPECT_EQ(mips64_features->GetInstructionSet(), kMips64);
  EXPECT_TRUE(mips64_features->Equals(mips64_features.get()));
  EXPECT_STREQ("default", mips64_features->GetFeatureString().c_str());
  EXPECT_EQ(mips64_features->AsBitmap(), 0U);
}

}  // namespace art
