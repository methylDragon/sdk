// Copyright 2023 Intrinsic Innovation LLC

#ifndef INTRINSIC_ICON_INTERPROCESS_SHARED_MEMORY_MANAGER_SHARED_MEMORY_MANAGER_H_
#define INTRINSIC_ICON_INTERPROCESS_SHARED_MEMORY_MANAGER_SHARED_MEMORY_MANAGER_H_

#include <stddef.h>
#include <stdint.h>

#include <string>
#include <typeinfo>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "intrinsic/icon/interprocess/shared_memory_manager/memory_segment.h"
#include "intrinsic/icon/interprocess/shared_memory_manager/segment_header.h"
#include "intrinsic/icon/interprocess/shared_memory_manager/segment_info.fbs.h"
#include "intrinsic/util/status/status_macros.h"

namespace intrinsic::icon {

// A type `T` is suited for shared memory if it's trivially copyable (no heap
// allocation internally) and is not a pointer type.
template <class T>
inline void AssertSharedMemoryCompatibility() {
  static_assert(
      std::is_trivially_copyable_v<T>,
      "only trivially copyable data types are supported as shm segments");
  static_assert(!std::is_pointer_v<T>,
                "pointer types are not supported as shm segments");
}

// The `SharedMemoryManager` creates and administers a set of shared memory
// segments. It allocates the memory with a given name which has to adhere to a
// POSIX shared memory naming convention (c.f.
// https://man7.org/linux/man-pages/man3/shm_open.3.html).
// Each allocated segmented is prefixed with a `SegmentHeader` to store some
// meta information about the allocated segment such as a reference counting.
// The overall data layout of each segment looks thus like the following:
//
// [SegmentHeader][Payload T]
// ^              ^
// Header()       Value()
//
// The manager additionally maintains a map of all allocated segments for
// further introspection of the segments. Once the manager goes out of scope, it
// unlinks all allocated memory; The kernel then eventually deletes the shared
// memory files once there's no further process using them.
// Once a segment is added via `AddSegment` it is fully initialized with a
// default value or any given value.
class SharedMemoryManager final {
 public:
  struct MemorySegmentInfo {
    uint8_t* data;
    // A value of true indicates that this segment needs to be used
    // by ICON.
    bool must_be_used;
  };

  SharedMemoryManager() = default;

  // This class is move-only.
  SharedMemoryManager(const SharedMemoryManager& other) = delete;
  SharedMemoryManager& operator=(const SharedMemoryManager& other) = delete;
  SharedMemoryManager(SharedMemoryManager&& other) = default;
  SharedMemoryManager& operator=(SharedMemoryManager&& other) = default;
  ~SharedMemoryManager();

  // Allocates a shared memory segment for the type `T` and initializes it with
  // the default value of `T`.
  // The type must be trivially copyable and not a pointer type; other types
  // fail to compile.
  // The name for the segment has to be POSIX conform, in which the length is
  // not to exceed 255 character and it has to contain a leading forward slash
  // `/`. No further slashes are allowed after the first one.
  // The value of `must_be_used` indicates whether this segment needs to be
  // used by ICON.
  // Similarly, one can optionally pass in a type identifier string to uniquely
  // describe the type of the data segment. The string can't exceed a max length
  // of `SegmentHeader::TypeInfo::kMaxSize` and defaults to a compiler generated
  // typeid. Please note that the compiler generated default is not defined by
  // the C++ standard and thus may not conform across process boundaries with
  // different compilers.
  // Returns `absl::InvalidArgumentError` if the name is not POSIX
  // conform, Returns `absl::AlreadyExistsError` if the shared memory segment
  // with this name already exists, Returns `absl::InternalError` if the
  // underlying POSIX call fails, Returns `absl::OkStatus` is the shared memory
  // segment was successfully allocated.
  template <class T>
  absl::Status AddSegmentWithDefaultValue(const MemoryName& name,
                                          bool must_be_used) {
    return AddSegmentWithDefaultValue<T>(name, must_be_used, typeid(T).name());
  }
  template <class T>
  absl::Status AddSegmentWithDefaultValue(const MemoryName& name,
                                          bool must_be_used,
                                          const std::string& type_id) {
    AssertSharedMemoryCompatibility<T>();
    INTR_RETURN_IF_ERROR(InitSegment(name, must_be_used,
                                     SegmentTraits<T>::kSegmentSize, type_id));
    return SetSegmentValue(name, T());
  }

  // Allocates a shared memory segment for the type `T` and initializes it with
  // the specified value of `T`.
  // Besides the initialized value for the segment, this function behaves
  // exactly like `AddSegment` above.
  template <class T>
  absl::Status AddSegment(const MemoryName& name, bool must_be_used,
                          const T& value) {
    return AddSegment<T>(name, must_be_used, value, typeid(T).name());
  }
  template <class T>
  absl::Status AddSegment(const MemoryName& name, bool must_be_used,
                          const T& value, const std::string& type_id) {
    AssertSharedMemoryCompatibility<T>();
    INTR_RETURN_IF_ERROR(InitSegment(name, must_be_used,
                                     SegmentTraits<T>::kSegmentSize, type_id));
    return SetSegmentValue(name, value);
  }
  template <class T>
  absl::Status AddSegment(const MemoryName& name, bool must_be_used,
                          T&& value) {
    return AddSegment<T>(name, must_be_used, std::forward<T>(value),
                         typeid(T).name());
  }
  template <class T>
  absl::Status AddSegment(const MemoryName& name, bool must_be_used, T&& value,
                          const std::string& type_id) {
    INTR_RETURN_IF_ERROR(InitSegment(name, must_be_used,
                                     SegmentTraits<T>::kSegmentSize, type_id));
    return SetSegmentValue(name, std::forward<T>(value));
  }

  // Allocates a generic memory segment for a byte (uint8_t) array of size `n`.
  absl::Status AddSegment(const MemoryName& name, bool must_be_used, size_t n) {
    return AddSegment(name, must_be_used, n, typeid(uint8_t).name());
  }
  absl::Status AddSegment(const MemoryName& name, bool must_be_used, size_t n,
                          const std::string& type_id) {
    INTR_RETURN_IF_ERROR(InitSegment(name, must_be_used, n, type_id));
    return absl::OkStatus();
  }

  // Returns the `SegmentHeader` belonging to the shared memory segment
  // specified by the given name.
  // Returns null pointer if the segment with the given name does not exist.
  const SegmentHeader* GetSegmentHeader(const MemoryName& name);

  // Returns the value belonging to the shared memory segment specified by the
  // given name.
  // Returns `nullptr` if the segment with the given name does
  // not exist.
  // Note, the type `T` has to match the type with which the segment was
  // originally created. This function leads to undefined behavior otherwise.
  template <class T>
  const T* GetSegmentValue(const MemoryName& name) {
    return reinterpret_cast<T*>(GetRawValue(name));
  }

  // Copies the new value into an existing shared memory segment.
  // Returns `absl::NotFoundError` if the segment with the given name does
  // not exist.
  // Note, the type `T` has to match the type with which the segment was
  // originally created. This function leads to undefined behavior otherwise.
  template <class T>
  absl::Status SetSegmentValue(const MemoryName& name, const T& new_value) {
    uint8_t* value = GetRawValue(name);
    if (value == nullptr) {
      return absl::NotFoundError(
          absl::StrCat("memory segment not found: ", name.GetName()));
    }
    *reinterpret_cast<T*>(value) = new_value;
    return absl::OkStatus();
  }
  template <class T>
  absl::Status SetSegmentValue(const MemoryName& name, T&& new_value) {
    uint8_t* value = GetRawValue(name);
    if (value == nullptr) {
      return absl::NotFoundError(
          absl::StrCat("memory segment not found: ", name.GetName()));
    }
    *reinterpret_cast<T*>(value) = std::forward<T>(new_value);
    return absl::OkStatus();
  }

  // Returns a pointer to the untyped shared memory value.
  // This function might be used when access to the underlying generic memory
  // location is needed, e.g. via `std::memcpy`. One typical use case is to copy
  // a flatbuffer (or any other serialized data struct) into a shared memory
  // segment. Prefer accessing the values via `GetSegmentValue` or
  // `SetSegmentValue` for type safety.
  uint8_t* GetRawValue(const MemoryName& name);

  // Returns a list of names for all registered shared memory segments.
  std::vector<MemoryName> GetRegisteredMemoryNames() const;

  // Returns a SegmentInfo struct containing the list of registered memory
  // segments.
  SegmentInfo GetSegmentInfo() const;

 private:
  absl::Status InitSegment(const MemoryName& name, bool must_be_used,
                           size_t segment_size, const std::string& type_id);

  uint8_t* GetRawHeader(const MemoryName& name);

  uint8_t* GetRawSegment(const MemoryName& name);

  // We not only store the name of the each initialized segment, but also a
  // pointer to its allocated memory. That way we can later on provide
  // introspection tools around all allocated memory in the system.
  absl::flat_hash_map<MemoryName, MemorySegmentInfo> memory_segments_;
};

}  // namespace intrinsic::icon

#endif  // INTRINSIC_ICON_INTERPROCESS_SHARED_MEMORY_MANAGER_SHARED_MEMORY_MANAGER_H_
