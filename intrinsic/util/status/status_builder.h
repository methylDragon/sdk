// Copyright 2023 Intrinsic Innovation LLC

#ifndef INTRINSIC_UTIL_STATUS_STATUS_BUILDER_H_
#define INTRINSIC_UTIL_STATUS_STATUS_BUILDER_H_

#include <memory>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

#include "absl/base/attributes.h"
#include "absl/base/log_severity.h"
#include "absl/log/log_sink.h"
#include "absl/status/status.h"
#include "absl/time/time.h"
#include "grpcpp/support/status.h"
#include "intrinsic/icon/release/source_location.h"

namespace intrinsic {

class ABSL_MUST_USE_RESULT StatusBuilder {
 public:
  // Creates a `StatusBuilder` based on an original status.  If logging is
  // enabled, it will use `location` as the location from which the log message
  // occurs.  A typical user will not specify `location`, allowing it to default
  // to the current location.
  explicit StatusBuilder(const absl::Status& original_status,
                         intrinsic::SourceLocation location =
                             intrinsic::SourceLocation::current());
  explicit StatusBuilder(absl::Status&& original_status,
                         intrinsic::SourceLocation location =
                             intrinsic::SourceLocation::current());
  // Creates a `StatusBuilder` from a status code.  If logging is enabled, it
  // will use `location` as the location from which the log message occurs.  A
  // typical user will not specify `location`, allowing it to default to the
  // current location.
  explicit StatusBuilder(absl::StatusCode code,
                         intrinsic::SourceLocation location =
                             intrinsic::SourceLocation::current());

  StatusBuilder(const StatusBuilder& sb);
  StatusBuilder& operator=(const StatusBuilder& sb);
  StatusBuilder(StatusBuilder&&) = default;
  StatusBuilder& operator=(StatusBuilder&&) = default;

  // Mutates the builder so that the final additional message is prepended to
  // the original error message in the status.  A convenience separator is not
  // placed between the messages.
  //
  // NOTE: Multiple calls to `SetPrepend` and `SetAppend` just adjust the
  // behavior of the final join of the original status with the extra message.
  //
  // Returns `*this` to allow method chaining.
  StatusBuilder& SetPrepend() &;
  StatusBuilder&& SetPrepend() &&;

  // Mutates the builder so that the final additional message is appended to the
  // original error message in the status.  A convenience separator is not
  // placed between the messages.
  //
  // NOTE: Multiple calls to `SetPrepend` and `SetAppend` just adjust the
  // behavior of the final join of the original status with the extra message.
  //
  // Returns `*this` to allow method chaining.
  StatusBuilder& SetAppend() &;
  StatusBuilder&& SetAppend() &&;

  // Mutates the builder to disable any logging that was set using any of the
  // logging functions below.  Returns `*this` to allow method chaining.
  StatusBuilder& SetNoLogging() &;
  StatusBuilder&& SetNoLogging() &&;

  // Mutates the builder so that the result status will be logged (without a
  // stack trace) when this builder is converted to a Status.  This overrides
  // the logging settings from earlier calls to any of the logging mutator
  // functions.  Returns `*this` to allow method chaining.
  StatusBuilder& Log(absl::LogSeverity level) &;
  StatusBuilder&& Log(absl::LogSeverity level) &&;
  StatusBuilder& LogError() & { return Log(absl::LogSeverity::kError); }
  StatusBuilder&& LogError() && { return std::move(LogError()); }
  StatusBuilder& LogWarning() & { return Log(absl::LogSeverity::kWarning); }
  StatusBuilder&& LogWarning() && { return std::move(LogWarning()); }
  StatusBuilder& LogInfo() & { return Log(absl::LogSeverity::kInfo); }
  StatusBuilder&& LogInfo() && { return std::move(LogInfo()); }

  // Mutates the builder so that the result status will be logged every N
  // invocations (without a stack trace) when this builder is converted to a
  // Status.  This overrides the logging settings from earlier calls to any of
  // the logging mutator functions.  Returns `*this` to allow method chaining.
  StatusBuilder& LogEveryN(absl::LogSeverity level, int n) &;
  StatusBuilder&& LogEveryN(absl::LogSeverity level, int n) &&;

  // Mutates the builder so that the result status will be logged once per
  // period (without a stack trace) when this builder is converted to a Status.
  // This overrides the logging settings from earlier calls to any of the
  // logging mutator functions.  Returns `*this` to allow method chaining.
  // If period is absl::ZeroDuration() or less, then this is equivalent to
  // calling the Log() method.
  StatusBuilder& LogEvery(absl::LogSeverity level, absl::Duration period) &;
  StatusBuilder&& LogEvery(absl::LogSeverity level, absl::Duration period) &&;

  // Mutates the builder so that a stack trace will be logged if the status is
  // logged. One of the logging setters above should be called as well. If
  // logging is not yet enabled this behaves as if LogInfo().EmitStackTrace()
  // was called. Returns `*this` to allow method chaining.
  StatusBuilder& EmitStackTrace() &;
  StatusBuilder&& EmitStackTrace() &&;

  // Mutates the builder so that the result status will also be logged to the
  // provided `sink` when this builder is converted to a status. Overwrites any
  // sink set prior. The provided `sink` must point to a valid object by the
  // time this builder is converted to a status. Has no effect if this builder
  // is not configured to log by calling any of the LogXXX methods. Returns
  // `*this` to allow method chaining.
  StatusBuilder& AlsoOutputToSink(absl::LogSink* sink) &;
  StatusBuilder&& AlsoOutputToSink(absl::LogSink* sink) &&;

  // Appends to the extra message that will be added to the original status.  By
  // default, the extra message is added to the original message and includes a
  // convenience separator between the original message and the enriched one.
  template <typename T>
  StatusBuilder& operator<<(const T& value) &;
  template <typename T>
  StatusBuilder&& operator<<(const T& value) &&;

  // Sets the status code for the status that will be returned by this
  // StatusBuilder. Returns `*this` to allow method chaining.
  StatusBuilder& SetCode(absl::StatusCode code) &;
  StatusBuilder&& SetCode(absl::StatusCode code) &&;

  ///////////////////////////////// Adaptors /////////////////////////////////
  //
  // A StatusBuilder `adaptor` is a functor which can be included in a builder
  // method chain. There are two common variants:
  //
  // 1. `Pure policy` adaptors modify the StatusBuilder and return the modified
  //    object, which can then be chained with further adaptors or mutations.
  //
  // 2. `Terminal` adaptors consume the builder's Status and return some
  //    other type of object. Alternatively, the consumed Status may be used
  //    for side effects, e.g. by passing it to a side channel. A terminal
  //    adaptor cannot be chained.
  //
  // Careful: The conversion of StatusBuilder to Status has side effects!
  // Adaptors must ensure that this conversion happens at most once in the
  // builder chain. The easiest way to do this is to determine the adaptor type
  // and follow the corresponding guidelines:
  //
  // Pure policy adaptors should:
  // (a) Take a StatusBuilder as input parameter.
  // (b) NEVER convert the StatusBuilder to Status:
  //     - Never assign the builder to a Status variable.
  //     - Never pass the builder to a function whose parameter type is Status,
  //       including by reference (e.g. const Status&).
  //     - Never pass the builder to any function which might convert the
  //       builder to Status (i.e. this restriction is viral).
  // (c) Return a StatusBuilder (usually the input parameter).
  //
  // Terminal adaptors should:
  // (a) Take a Status as input parameter (not a StatusBuilder!).
  // (b) Return a type matching the enclosing function. (This can be `void`.)
  //
  // Adaptors do not necessarily fit into one of these categories. However, any
  // which satisfies the conversion rule can always be decomposed into a pure
  // adaptor chained into a terminal adaptor. (This is recommended.)
  //
  // Examples
  //
  // Pure adaptors allow teams to configure team-specific error handling
  // policies.  For example:
  //
  //   StatusBuilder TeamPolicy(StatusBuilder builder) {
  //     return std::move(builder).Log(absl::LogSeverity::kWarning);
  //   }
  //
  //   INTRINSIC_RETURN_IF_ERROR(foo()).With(TeamPolicy);
  //
  // Because pure policy adaptors return the modified StatusBuilder, they
  // can be chained with further adaptors, e.g.:
  //
  //   INTRINSIC_RETURN_IF_ERROR(foo()).With(TeamPolicy).With(OtherTeamPolicy);
  //
  // Terminal adaptors are often used for type conversion. This allows
  // RETURN_IF_ERROR to be used in functions which do not return Status. For
  // example, a function might want to return some default value on error:
  //
  //   int GetSysCounter() {
  //     int value;
  //     INTRINSIC_RETURN_IF_ERROR(ReadCounterFile(filename, &value))
  //         .LogInfo()
  //         .With([](const absl::Status& unused) { return 0; });
  //     return value;
  //   }
  //
  // For the simple case of returning a constant (e.g. zero, false, nullptr) on
  // error, consider `status_macros::Return` or `status_macros::ReturnVoid`:
  //
  //   bool DoMyThing() {
  //     INTRINSIC_RETURN_IF_ERROR(foo())
  //         .LogWarning().With(status_macros::Return(false));
  //     ...
  //   }
  //
  // A terminal adaptor may instead (or additionally) be used to create side
  // effects that are not supported natively by `StatusBuilder`, such as
  // returning the Status through a side channel.

  // Calls `adaptor` on this status builder to apply policies, type conversions,
  // and/or side effects on the StatusBuilder. Returns the value returned by
  // `adaptor`, which may be any type including `void`. See comments above.
  //
  // Style guide exception for Ref qualified methods and rvalue refs.  This
  // allows us to avoid a copy in the common case.
  template <typename Adaptor>
  auto With(
      Adaptor&& adaptor) & -> decltype(std::forward<Adaptor>(adaptor)(*this)) {
    return std::forward<Adaptor>(adaptor)(*this);
  }
  template <typename Adaptor>
  auto With(Adaptor&& adaptor) && -> decltype(std::forward<Adaptor>(adaptor)(
      std::move(*this))) {
    return std::forward<Adaptor>(adaptor)(std::move(*this));
  }

  // Returns true if the Status created by this builder will be ok().
  bool ok() const;

  // Returns the (canonical) error code for the Status created by this builder.
  absl::StatusCode code() const;

  // Implicit conversion to Status.
  //
  // Careful: this operator has side effects, so it should be called at
  // most once. In particular, do NOT use this conversion operator to inspect
  // the status from an adapter object passed into With().
  //
  // Style guide exception for using Ref qualified methods and for implicit
  // conversions.  This override allows us to implement RETURN_IF_ERROR with
  // 2 move operations in the common case.
  operator absl::Status() const&;  // NOLINT: Builder converts implicitly.
  operator absl::Status() &&;      // NOLINT: Builder converts implicitly.

  operator grpc::Status() const&;  // NOLINT: Builder converts implicitly.
  operator grpc::Status() &&;      // NOLINT: Builder converts implicitly.

  // Returns the source location used to create this builder.
  intrinsic::SourceLocation source_location() const;

 private:
  // Specifies how to join the error message in the original status and any
  // additional message that has been streamed into the builder.
  enum class MessageJoinStyle {
    kAnnotate,
    kAppend,
    kPrepend,
  };

  // Creates a new status based on an old one by joining the message from the
  // original to an additional message.
  static absl::Status JoinMessageToStatus(absl::Status s, std::string_view msg,
                                          MessageJoinStyle style);

  // Creates a Status from this builder and logs it if the builder has been
  // configured to log itself.
  absl::Status CreateStatusAndConditionallyLog() &&;

  // Conditionally logs if the builder has been configured to log.  This method
  // is split from the above to isolate the portability issues around logging
  // into a single place.
  void ConditionallyLog(const absl::Status& status) const;

  // Sets the code of the provided Status (there is no setter for it on
  // absl::Status).
  static void SetStatusCode(absl::StatusCode canonical_code,
                            absl::Status* status);
  // Copies all payloads of a Status to another Status (there is no helper for
  // this in absl::Status).
  static void CopyPayloads(const absl::Status& src, absl::Status* dst);
  // Returns a Status that is the same as the provided `status` but with the
  // message set to `msg`.
  static absl::Status WithMessage(const absl::Status& status,
                                  std::string_view msg);
  // Returns a Status that is identical to `s` except that the error_message()
  // has been augmented by adding `msg` to the end of the original error
  // message.
  //
  // Annotate should be used to add higher-level information to a Status.  E.g.,
  //
  //   absl::Status s = GetFileContents(...);
  //   if (!s.ok()) {
  //     return Annotate(s, "loading summary statistics data");
  //   }
  //
  // Annotate() adds the appropriate separators, so callers should not include a
  // separator in `msg`. The exact formatting is subject to change, so you
  // should not depend on it in your tests.
  //
  // OK status values have no error message and therefore if `s` is OK, the
  // result is unchanged.
  static absl::Status AnnotateStatus(const absl::Status& s,
                                     std::string_view msg);

  // Infrequently set builder options, instantiated lazily. This reduces
  // average construction/destruction time (e.g. the `stream` is fairly
  // expensive). Stacks can also be blown if StatusBuilder grows too large.
  // This is primarily an issue for debug builds, which do not necessarily
  // re-use stack space within a function across the sub-scopes used by
  // status macros.
  struct Rep {
    explicit Rep() = default;
    Rep(const Rep& r);

    enum class LoggingMode { kDisabled, kLog, kLogEveryN, kLogEveryPeriod };
    LoggingMode logging_mode = LoggingMode::kDisabled;

    // Corresponds to the levels in `absl::LogSeverity`.
    absl::LogSeverity log_severity;

    // Only log every N invocations.
    // Only used when `logging_mode == LoggingMode::kLogEveryN`.
    int n;

    // Only log once per period.
    // Only used when `logging_mode == LoggingMode::kLogEveryPeriod`.
    absl::Duration period;

    // Gathers additional messages added with `<<` for use in the final status.
    std::ostringstream stream;

    // Whether to log stack trace.  Only used when `logging_mode !=
    // LoggingMode::kDisabled`.
    bool should_log_stack_trace = false;

    // Specifies how to join the message in `status_` and `stream`.
    MessageJoinStyle message_join_style = MessageJoinStyle::kAnnotate;

    // If not nullptr, specifies the log sink where log output should be also
    // sent to.  Only used when `logging_mode != LoggingMode::kDisabled`.
    absl::LogSink* sink = nullptr;
  };

  // The status that the result will be based on.
  absl::Status status_;

  // The location to record if this status is logged.
  intrinsic::SourceLocation loc_;

  // nullptr if the result status will be OK.  Extra fields moved to the heap to
  // minimize stack space.
  std::unique_ptr<Rep> rep_;
};

// Implicitly converts `builder` to `Status` and write it to `os`.
std::ostream& operator<<(std::ostream& os, const StatusBuilder& builder);
std::ostream& operator<<(std::ostream& os, StatusBuilder&& builder);

// Each of the functions below creates StatusBuilder with a canonical error.
// The error code of the StatusBuilder matches the name of the function.
StatusBuilder AbortedErrorBuilder(
    intrinsic::SourceLocation location = INTRINSIC_LOC);
StatusBuilder AlreadyExistsErrorBuilder(
    intrinsic::SourceLocation location = INTRINSIC_LOC);
StatusBuilder CancelledErrorBuilder(
    intrinsic::SourceLocation location = INTRINSIC_LOC);
StatusBuilder DataLossErrorBuilder(
    intrinsic::SourceLocation location = INTRINSIC_LOC);
StatusBuilder DeadlineExceededErrorBuilder(
    intrinsic::SourceLocation location = INTRINSIC_LOC);
StatusBuilder FailedPreconditionErrorBuilder(
    intrinsic::SourceLocation location = INTRINSIC_LOC);
StatusBuilder InternalErrorBuilder(
    intrinsic::SourceLocation location = INTRINSIC_LOC);
StatusBuilder InvalidArgumentErrorBuilder(
    intrinsic::SourceLocation location = INTRINSIC_LOC);
StatusBuilder NotFoundErrorBuilder(
    intrinsic::SourceLocation location = INTRINSIC_LOC);
StatusBuilder OutOfRangeErrorBuilder(
    intrinsic::SourceLocation location = INTRINSIC_LOC);
StatusBuilder PermissionDeniedErrorBuilder(
    intrinsic::SourceLocation location = INTRINSIC_LOC);
StatusBuilder UnauthenticatedErrorBuilder(
    intrinsic::SourceLocation location = INTRINSIC_LOC);
StatusBuilder ResourceExhaustedErrorBuilder(
    intrinsic::SourceLocation location = INTRINSIC_LOC);
StatusBuilder UnavailableErrorBuilder(
    intrinsic::SourceLocation location = INTRINSIC_LOC);
StatusBuilder UnimplementedErrorBuilder(
    intrinsic::SourceLocation location = INTRINSIC_LOC);
StatusBuilder UnknownErrorBuilder(
    intrinsic::SourceLocation location = INTRINSIC_LOC);

// StatusBuilder policy to append an extra message to the original status.
//
// This is most useful with adaptors such as util::TaskReturn that otherwise
// would prevent use of operator<<.  For example:
//
//   INTR_INTRINSIC_RETURN_IF_ERROR(foo(val))
//       .With(intrinsic::ExtraMessage("when calling foo()"))
//       .With([]() { return "bar"; });
//
// or
//
//   INTR_INTRINSIC_RETURN_IF_ERROR(foo(val))
//       .With(intrinsic::ExtraMessage() << "val: " << val)
//       .With([]() { return "bar"; });
//
// Note in the above example, the INTR_RETURN_IF_ERROR macro ensures the
// ExtraMessage expression is evaluated only in the error case, so efficiency of
// constructing the message is not a concern in the success case.
//
// The return in the last With lambda respectively causes the entire expression
// to return a "const char*" return value.
//
class ExtraMessage {
 public:
  ExtraMessage() = default;
  explicit ExtraMessage(std::string msg) : stream_(std::move(msg)) {}
  ExtraMessage(const ExtraMessage& m) : stream_(m.stream_.str()) {}
  ExtraMessage(const ExtraMessage&& m) : stream_(std::move(m.stream_).str()) {}

  // Appends to the extra message that will be added to the original status.  By
  // default, the extra message is added to the original message, which includes
  // a convenience separator between the original message and the enriched one.
  template <typename T>
  ExtraMessage& operator<<(const T& value) & {
    stream_ << value;
    return *this;
  }

  // As above, preserving the rvalue-ness of the ExtraMessage object.
  template <typename T>
  ExtraMessage&& operator<<(const T& value) && {
    *this << value;
    return std::move(*this);
  }

  // Appends to the extra message that will be added to the original status.  By
  // default, the extra message is added to the original message, which includes
  // a convenience separator between the original message and the enriched one.
  StatusBuilder operator()(StatusBuilder builder) const {
    builder << stream_.str();
    return builder;
  }

 private:
  std::ostringstream stream_;
};

// Implementation details follow; clients should ignore.

inline StatusBuilder::StatusBuilder(const absl::Status& original_status,
                                    intrinsic::SourceLocation location)
    : status_(original_status), loc_(location) {}

inline StatusBuilder::StatusBuilder(absl::Status&& original_status,
                                    intrinsic::SourceLocation location)
    : status_(std::move(original_status)), loc_(location) {}

inline StatusBuilder::StatusBuilder(absl::StatusCode code,
                                    intrinsic::SourceLocation location)
    : status_(code, ""), loc_(location) {}

inline StatusBuilder::StatusBuilder(const StatusBuilder& sb)
    : status_(sb.status_), loc_(sb.loc_) {
  if (sb.rep_ != nullptr) {
    rep_ = std::make_unique<Rep>(*sb.rep_);
  }
}

inline StatusBuilder& StatusBuilder::operator=(const StatusBuilder& sb) {
  status_ = sb.status_;
  loc_ = sb.loc_;
  if (sb.rep_ != nullptr) {
    rep_ = std::make_unique<Rep>(*sb.rep_);
  } else {
    rep_ = nullptr;
  }
  return *this;
}

inline StatusBuilder& StatusBuilder::SetPrepend() & {
  if (status_.ok()) {
    return *this;
  }
  if (rep_ == nullptr) {
    rep_ = std::make_unique<Rep>();
  }

  rep_->message_join_style = MessageJoinStyle::kPrepend;
  return *this;
}

inline StatusBuilder&& StatusBuilder::SetPrepend() && {
  return std::move(SetPrepend());
}

inline StatusBuilder& StatusBuilder::SetAppend() & {
  if (status_.ok()) {
    return *this;
  }
  if (rep_ == nullptr) {
    rep_ = std::make_unique<Rep>();
  }
  rep_->message_join_style = MessageJoinStyle::kAppend;
  return *this;
}

inline StatusBuilder&& StatusBuilder::SetAppend() && {
  return std::move(SetAppend());
}

inline StatusBuilder& StatusBuilder::SetNoLogging() & {
  if (rep_ != nullptr) {
    rep_->logging_mode = Rep::LoggingMode::kDisabled;
    rep_->should_log_stack_trace = false;
  }
  return *this;
}

inline StatusBuilder&& StatusBuilder::SetNoLogging() && {
  return std::move(SetNoLogging());
}

inline StatusBuilder& StatusBuilder::Log(absl::LogSeverity level) & {
  if (status_.ok()) {
    return *this;
  }
  if (rep_ == nullptr) {
    rep_ = std::make_unique<Rep>();
  }
  rep_->logging_mode = Rep::LoggingMode::kLog;
  rep_->log_severity = level;
  return *this;
}

inline StatusBuilder&& StatusBuilder::Log(absl::LogSeverity level) && {
  return std::move(Log(level));
}

inline StatusBuilder& StatusBuilder::LogEveryN(absl::LogSeverity level,
                                               int n) & {
  if (status_.ok()) {
    return *this;
  }
  if (n < 1) {
    return Log(level);
  }
  if (rep_ == nullptr) {
    rep_ = std::make_unique<Rep>();
  }
  rep_->logging_mode = Rep::LoggingMode::kLogEveryN;
  rep_->log_severity = level;
  rep_->n = n;
  return *this;
}
inline StatusBuilder&& StatusBuilder::LogEveryN(absl::LogSeverity level,
                                                int n) && {
  return std::move(LogEveryN(level, n));
}

inline StatusBuilder& StatusBuilder::LogEvery(absl::LogSeverity level,
                                              absl::Duration period) & {
  if (status_.ok()) {
    return *this;
  }
  if (period <= absl::ZeroDuration()) {
    return Log(level);
  }
  if (rep_ == nullptr) {
    rep_ = std::make_unique<Rep>();
  }
  rep_->logging_mode = Rep::LoggingMode::kLogEveryPeriod;
  rep_->log_severity = level;
  rep_->period = period;
  return *this;
}

inline StatusBuilder&& StatusBuilder::LogEvery(absl::LogSeverity level,
                                               absl::Duration period) && {
  return std::move(LogEvery(level, period));
}

inline StatusBuilder& StatusBuilder::EmitStackTrace() & {
  if (status_.ok()) {
    return *this;
  }
  if (rep_ == nullptr) {
    rep_ = std::make_unique<Rep>();
    // Default to INFO logging, otherwise nothing would be emitted.
    rep_->logging_mode = Rep::LoggingMode::kLog;
    rep_->log_severity = absl::LogSeverity::kInfo;
  }
  rep_->should_log_stack_trace = true;
  return *this;
}

inline StatusBuilder&& StatusBuilder::EmitStackTrace() && {
  return std::move(EmitStackTrace());
}

inline StatusBuilder& StatusBuilder::AlsoOutputToSink(absl::LogSink* sink) & {
  if (status_.ok()) {
    return *this;
  }
  if (rep_ == nullptr) {
    rep_ = std::make_unique<Rep>();
  }
  rep_->sink = sink;
  return *this;
}
inline StatusBuilder&& StatusBuilder::AlsoOutputToSink(absl::LogSink* sink) && {
  return std::move(AlsoOutputToSink(sink));
}

template <typename T>
StatusBuilder& StatusBuilder::operator<<(const T& value) & {
  if (status_.ok()) {
    return *this;
  }
  if (rep_ == nullptr) {
    rep_ = std::make_unique<Rep>();
  }
  rep_->stream << value;
  return *this;
}

template <typename T>
StatusBuilder&& StatusBuilder::operator<<(const T& value) && {
  return std::move(operator<<(value));
}

inline StatusBuilder& StatusBuilder::SetCode(absl::StatusCode code) & {
  SetStatusCode(code, &status_);
  return *this;
}

inline StatusBuilder&& StatusBuilder::SetCode(absl::StatusCode code) && {
  return std::move(SetCode(code));
}

inline bool StatusBuilder::ok() const { return status_.ok(); }

inline absl::StatusCode StatusBuilder::code() const { return status_.code(); }

inline StatusBuilder::operator absl::Status() const& {
  if (rep_ == nullptr) {
    return status_;
  }
  return StatusBuilder(*this).CreateStatusAndConditionallyLog();
}

inline StatusBuilder::operator absl::Status() && {
  if (rep_ == nullptr) {
    return std::move(status_);
  }
  return std::move(*this).CreateStatusAndConditionallyLog();
}

inline StatusBuilder::operator grpc::Status() const& {
  if (rep_ == nullptr) {
    return status_;
  }
  return grpc::Status(static_cast<absl::Status>(*this));
}

inline StatusBuilder::operator grpc::Status() && {
  if (rep_ == nullptr) {
    return std::move(status_);
  }
  return grpc::Status(static_cast<absl::Status>(std::move(*this)));
}

inline intrinsic::SourceLocation StatusBuilder::source_location() const {
  return loc_;
}

}  // namespace intrinsic

#endif  // INTRINSIC_UTIL_STATUS_STATUS_BUILDER_H_
