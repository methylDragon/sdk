// Copyright 2023 Intrinsic Innovation LLC
// Intrinsic Proprietary and Confidential
// Provided subject to written agreement between the parties.

#ifndef INTRINSIC_ICON_EXAMPLES_BLENDED_JOINT_MOVE_LIB_H_
#define INTRINSIC_ICON_EXAMPLES_BLENDED_JOINT_MOVE_LIB_H_

#include <memory>

#include "intrinsic/util/grpc/channel_interface.h"

namespace intrinsic::icon::examples {

// Executes a blended joint move for the part `part_name`. A valid connection to
// an ICON server is passed in using the parameter `icon_channel`.
absl::Status RunBlendedJointMove(
    absl::string_view part_name,
    std::shared_ptr<intrinsic::icon::ChannelInterface> icon_channel);

}  // namespace intrinsic::icon::examples

#endif  // INTRINSIC_ICON_EXAMPLES_BLENDED_JOINT_MOVE_LIB_H_