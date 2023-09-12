// Copyright 2023 Intrinsic Innovation LLC
// Intrinsic Proprietary and Confidential
// Provided subject to written agreement between the parties.

#ifndef INTRINSIC_SKILLS_TESTING_NO_OP_SKILL_H_
#define INTRINSIC_SKILLS_TESTING_NO_OP_SKILL_H_

#include <memory>
#include <string>

#include "absl/container/flat_hash_map.h"
#include "absl/status/statusor.h"
#include "google/protobuf/descriptor.h"
#include "intrinsic/skills/cc/skill_interface.h"
#include "intrinsic/skills/proto/equipment.pb.h"
#include "intrinsic/skills/proto/skill_service.pb.h"

namespace intrinsic::skills {

class NoOpSkill : public SkillInterface {
 public:
  static std::unique_ptr<SkillInterface> CreateSkill();

  std::string Name() const override;

  absl::flat_hash_map<std::string, intrinsic_proto::skills::EquipmentSelector>
  EquipmentRequired() const override;

  const google::protobuf::Descriptor* GetParameterDescriptor() const override;

  absl::StatusOr<intrinsic_proto::skills::ExecuteResult> Execute(
      const intrinsic_proto::skills::ExecuteRequest& execute_request,
      ExecutionContext& context) override;
};

}  // namespace intrinsic::skills

#endif  // INTRINSIC_SKILLS_TESTING_NO_OP_SKILL_H_