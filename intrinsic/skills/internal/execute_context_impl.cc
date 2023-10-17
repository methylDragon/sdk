// Copyright 2023 Intrinsic Innovation LLC
// Intrinsic Proprietary and Confidential
// Provided subject to written agreement between the parties.

#include "intrinsic/skills/internal/execute_context_impl.h"

#include <memory>
#include <string>
#include <utility>

#include "absl/status/statusor.h"
#include "intrinsic/logging/proto/context.pb.h"
#include "intrinsic/motion_planning/motion_planner_client.h"
#include "intrinsic/motion_planning/proto/motion_planner_service.grpc.pb.h"
#include "intrinsic/skills/cc/equipment_pack.h"
#include "intrinsic/skills/cc/skill_canceller.h"
#include "intrinsic/skills/proto/equipment.pb.h"
#include "intrinsic/skills/proto/footprint.pb.h"
#include "intrinsic/skills/proto/skill_service.pb.h"
#include "intrinsic/skills/proto/skills.pb.h"
#include "intrinsic/world/objects/object_world_client.h"
#include "intrinsic/world/proto/object_world_service.grpc.pb.h"

namespace intrinsic {
namespace skills {

using ::intrinsic::world::ObjectWorldClient;

ExecuteContextImpl::ExecuteContextImpl(
    const intrinsic_proto::skills::ExecuteRequest& request,
    std::shared_ptr<ObjectWorldService::StubInterface> object_world_service,
    std::shared_ptr<MotionPlannerService::StubInterface> motion_planner_service,
    EquipmentPack equipment, std::shared_ptr<SkillCanceller> skill_canceller)
    : world_id_(request.world_id()),
      object_world_service_(std::move(object_world_service)),
      motion_planner_service_(std::move(motion_planner_service)),
      equipment_(std::move(equipment)),
      skill_canceller_(skill_canceller),
      log_context_(request.context()) {}

absl::StatusOr<ObjectWorldClient> ExecuteContextImpl::GetObjectWorld() {
  return ObjectWorldClient(world_id_, object_world_service_);
}

absl::StatusOr<motion_planning::MotionPlannerClient>
ExecuteContextImpl::GetMotionPlanner() {
  return motion_planning::MotionPlannerClient(world_id_,
                                              motion_planner_service_);
}

}  // namespace skills
}  // namespace intrinsic
