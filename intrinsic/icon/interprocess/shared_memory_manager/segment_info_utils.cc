// Copyright 2023 Intrinsic Innovation LLC

#include "intrinsic/icon/interprocess/shared_memory_manager/segment_info_utils.h"

#include <stddef.h>
#include <string.h>

#include <string>
#include <utility>
#include <vector>

#include "intrinsic/icon/interprocess/shared_memory_manager/segment_info.fbs.h"

namespace intrinsic::icon {

namespace {

static constexpr size_t kMaxStringLength = 255;

}  // namespace

std::vector<std::string> GetNamesFromSegmentInfo(
    const SegmentInfo& segment_info) {
  std::vector<std::string> names;
  names.reserve(segment_info.size());

  for (uint32_t i = 0; i < segment_info.size(); ++i) {
    const SegmentName* name = segment_info.names()->Get(i);
    const char* data = reinterpret_cast<const char*>(name->value()->Data());
    std::string interface_name(data, strnlen(data, kMaxStringLength));
    names.emplace_back(std::move(interface_name));
  }

  return names;
}

}  // namespace intrinsic::icon
