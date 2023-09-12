// Copyright 2023 Intrinsic Innovation LLC
// Intrinsic Proprietary and Confidential
// Provided subject to written agreement between the parties.

#ifndef INTRINSIC_SKILLS_INTERNAL_EXECUTION_CONTEXT_IMPL_H_
#define INTRINSIC_SKILLS_INTERNAL_EXECUTION_CONTEXT_IMPL_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "absl/base/attributes.h"
#include "absl/functional/any_invocable.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "absl/time/time.h"
#include "intrinsic/logging/proto/context.pb.h"
#include "intrinsic/motion_planning/motion_planner_client.h"
#include "intrinsic/motion_planning/proto/motion_planner_service.grpc.pb.h"
#include "intrinsic/skills/cc/equipment_pack.h"
#include "intrinsic/skills/cc/skill_interface.h"
#include "intrinsic/skills/internal/canceller.h"
#include "intrinsic/skills/internal/skill_registry_client_interface.h"
#include "intrinsic/skills/proto/equipment.pb.h"
#include "intrinsic/skills/proto/footprint.pb.h"
#include "intrinsic/skills/proto/skill_service.pb.h"
#include "intrinsic/world/objects/object_world_client.h"
#include "intrinsic/world/proto/object_world_service.grpc.pb.h"

namespace intrinsic {
namespace skills {

// Implementation of ExecutionContext as used by the skill service.
class ExecutionContextImpl : public ExecutionContext {
  using ObjectWorldService = ::intrinsic_proto::world::ObjectWorldService;
  using MotionPlannerService =
      ::intrinsic_proto::motion_planning::MotionPlannerService;

 public:
  ExecutionContextImpl(
      const intrinsic_proto::skills::ExecuteRequest& request,
      std::shared_ptr<ObjectWorldService::StubInterface> object_world_service,
      std::shared_ptr<MotionPlannerService::StubInterface>
          motion_planner_service,
      EquipmentPack equipment,
      SkillRegistryClientInterface& skill_registry_client,
      std::shared_ptr<Canceller> skill_canceller);

  absl::StatusOr<world::ObjectWorldClient> GetObjectWorld() override;

  const intrinsic_proto::data_logger::Context& GetLogContext() const override;

  absl::StatusOr<motion_planning::MotionPlannerClient> GetMotionPlanner()
      override;

  const EquipmentPack& GetEquipment() const override { return equipment_; }

  absl::Status RegisterCancellationCallback(
      absl::AnyInvocable<absl::Status() const> cb) override {
    if (skill_canceller_ == nullptr) {
      return absl::FailedPreconditionError("Skill is not cancellable.");
    }
    return skill_canceller_->RegisterCancellationCallback(std::move(cb));
  }

  void NotifyReadyForCancellation() override {
    if (skill_canceller_ == nullptr) return;
    skill_canceller_->NotifyReadyForCancellation();
  }

  bool Cancelled() override {
    if (skill_canceller_ == nullptr) {
      return false;
    }
    return skill_canceller_->Cancelled();
  }

 private:
  std::string world_id_;

  std::shared_ptr<ObjectWorldService::StubInterface> object_world_service_;
  std::shared_ptr<MotionPlannerService::StubInterface> motion_planner_service_;
  EquipmentPack equipment_;
  SkillRegistryClientInterface& skill_registry_client_ ABSL_ATTRIBUTE_UNUSED;

  std::shared_ptr<Canceller> skill_canceller_;

  intrinsic_proto::data_logger::Context log_context_;
};

}  // namespace skills
}  // namespace intrinsic

#endif  // INTRINSIC_SKILLS_INTERNAL_EXECUTION_CONTEXT_IMPL_H_