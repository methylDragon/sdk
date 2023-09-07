// Copyright 2023 Intrinsic Innovation LLC
// Intrinsic Proprietary and Confidential
// Provided subject to written agreement between the parties.

#ifndef INTRINSIC_MATH_PROTO_CONVERSION_H_
#define INTRINSIC_MATH_PROTO_CONVERSION_H_

#include "absl/status/statusor.h"
#include "intrinsic/eigenmath/types.h"
#include "intrinsic/math/pose3.h"
#include "intrinsic/math/proto/point.pb.h"
#include "intrinsic/math/proto/pose.pb.h"
#include "intrinsic/math/proto/quaternion.pb.h"

// All conversions from INTRINSIC protos to their respective C++ types should be
// declared in namespace intrinsic_proto. This makes it possible to make
// unqualified calls to FromProto() throughout our code base.
namespace intrinsic_proto {

intrinsic::eigenmath::Vector3d FromProto(const Point& point);
intrinsic::eigenmath::Quaterniond FromProto(const Quaternion& quaternion);
absl::StatusOr<intrinsic::Pose> FromProto(const Pose& pose);

}  // namespace intrinsic_proto

// To enable unqualified calls to ToProto() throughout our code base, we delcare
// functions which convert C++ types to protos in namespace intrinsic.
namespace intrinsic {

intrinsic_proto::Pose ToProto(const Pose& pose);
intrinsic_proto::Point ToProto(const eigenmath::Vector3d& point);
intrinsic_proto::Quaternion ToProto(const eigenmath::Quaterniond& quaternion);

}  // namespace intrinsic

#endif  // INTRINSIC_MATH_PROTO_CONVERSION_H_
