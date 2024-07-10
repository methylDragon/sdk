// Copyright 2023 Intrinsic Innovation LLC

#include "intrinsic/util/proto_time.h"

#include <algorithm>
#include <cstdint>
#include <ctime>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/time/clock.h"
#include "absl/time/time.h"

namespace intrinsic {

namespace {

// Earliest time that can be represented by a google::protobuf::Timestamp.
// Corresponds to 0001-01-01T00:00:00Z.
//
// See google/protobuf/timestamp.proto.
constexpr timespec kMinProtoTimestamp{.tv_sec = -62135596800, .tv_nsec = 0};
// Latest time that can be represented by a google::protobuf::Timestamp.
// Corresponds to 9999-12-31T23:59:59.999999999Z.
//
// See google/protobuf/timestamp.proto.
constexpr timespec kMaxProtoTimestamp{.tv_sec = 253402300799,
                                      .tv_nsec = 999999999};

// Least duration that can be represented by a google::protobuf::Duration.
// Corresponds to -10000 years.
//
// See google/protobuf/duration.proto.
constexpr timespec kMinProtoDuration{.tv_sec = -315576000000,
                                     .tv_nsec = -999999999};
// Greatest duration that can be represented by a google::protobuf::Duration.
// Corresponds to 10000 years.
//
// See google/protobuf/duration.proto.
constexpr timespec kMaxProtoDuration{.tv_sec = 315576000000,
                                     .tv_nsec = 999999999};

// Validation requirements documented in duration.proto
absl::Status Validate(const google::protobuf::Duration& d) {
  const auto sec = d.seconds();
  const auto ns = d.nanos();
  if (sec < kMinProtoDuration.tv_sec || sec > kMaxProtoDuration.tv_sec) {
    return absl::InvalidArgumentError(absl::StrCat("seconds=", sec));
  }
  if (ns < kMinProtoDuration.tv_nsec || ns > kMaxProtoDuration.tv_nsec) {
    return absl::InvalidArgumentError(absl::StrCat("nanos=", ns));
  }
  if ((sec < 0 && ns > 0) || (sec > 0 && ns < 0)) {
    return absl::InvalidArgumentError("sign mismatch");
  }
  return absl::OkStatus();
}

// Documented in google/protobuf/timestamp.proto.
absl::Status Validate(const google::protobuf::Timestamp& timestamp) {
  const auto sec = timestamp.seconds();
  const auto ns = timestamp.nanos();
  // sec must be [0001-01-01T00:00:00Z, 9999-12-31T23:59:59.999999999Z]
  if (sec < kMinProtoTimestamp.tv_sec || sec > kMaxProtoTimestamp.tv_sec) {
    return absl::InvalidArgumentError(absl::StrCat("seconds=", sec));
  }
  if (ns < kMinProtoTimestamp.tv_nsec || ns > kMaxProtoTimestamp.tv_nsec) {
    return absl::InvalidArgumentError(absl::StrCat("nanos=", ns));
  }
  return absl::OkStatus();
}

void FromAbslTimeNoValidation(absl::Time time,
                              google::protobuf::Timestamp* timestamp) {
  const int64_t s = absl::ToUnixSeconds(time);
  timestamp->set_seconds(s);
  timestamp->set_nanos((time - absl::FromUnixSeconds(s)) /
                       absl::Nanoseconds(1));
}

}  // namespace

/// google::protobuf::Timestamp.

google::protobuf::Timestamp GetCurrentTimeProto() {
  absl::StatusOr<google::protobuf::Timestamp> time = FromAbslTime(absl::Now());
  if (!time.ok()) {
    return google::protobuf::Timestamp();
  }
  return *time;
}

absl::Status FromAbslTime(absl::Time time,
                          google::protobuf::Timestamp* timestamp) {
  FromAbslTimeNoValidation(time, timestamp);
  return Validate(*timestamp);
}

absl::StatusOr<google::protobuf::Timestamp> FromAbslTime(absl::Time time) {
  google::protobuf::Timestamp timestamp;
  auto status = FromAbslTime(time, &timestamp);
  if (!status.ok()) {
    return status;
  }
  return timestamp;
}

google::protobuf::Timestamp FromAbslTimeClampToValidRange(absl::Time time) {
  time = std::clamp(time, absl::TimeFromTimespec(kMinProtoTimestamp),
                    absl::TimeFromTimespec(kMaxProtoTimestamp));
  google::protobuf::Timestamp out;
  FromAbslTimeNoValidation(time, &out);
  return out;
}

absl::StatusOr<absl::Time> ToAbslTime(
    const google::protobuf::Timestamp& proto) {
  absl::Status status = Validate(proto);
  if (!status.ok()) return status;
  return absl::FromUnixSeconds(proto.seconds()) +
         absl::Nanoseconds(proto.nanos());
}

/// google::protobuf::Duration.

absl::StatusOr<google::protobuf::Duration> FromAbslDuration(absl::Duration d) {
  google::protobuf::Duration proto;
  absl::Status status = FromAbslDuration(d, &proto);
  if (!status.ok()) return status;
  return proto;
}

absl::Status FromAbslDuration(absl::Duration d,
                              google::protobuf::Duration* proto) {
  // s and n may both be negative, per the Duration proto spec.
  const int64_t s = absl::IDivDuration(d, absl::Seconds(1), &d);
  const int64_t n = absl::IDivDuration(d, absl::Nanoseconds(1), &d);
  proto->set_seconds(s);
  proto->set_nanos(n);
  return Validate(*proto);
}

absl::Duration ToAbslDuration(const google::protobuf::Duration& proto) {
  return absl::Seconds(proto.seconds()) + absl::Nanoseconds(proto.nanos());
}

}  // namespace intrinsic
