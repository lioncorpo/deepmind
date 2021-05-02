// Copyright 2020 DeepMind Technologies Limited.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Copyright 2016-2019 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// ============================================================================
//
// A minimal replacement for "glog"-like functionality. Does not provide output
// in a separate thread nor backtracing.

#ifndef COURIER_PLATFORM_DEFAULT_LOGGING_H_
#define COURIER_PLATFORM_DEFAULT_LOGGING_H_

#include <atomic>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

#ifdef __GNUC__
#define PREDICT_TRUE(x) (__builtin_expect(!!(x), 1))
#define PREDICT_FALSE(x) (__builtin_expect(x, 0))
#define NORETURN __attribute__((noreturn))
#else
#define PREDICT_TRUE(x) (x)
#define PREDICT_FALSE(x) (x)
#define NORETURN
#endif

namespace courier {
namespace internal {

struct CheckOpString {
  explicit CheckOpString(std::string* str) : str_(str) {}
  explicit operator bool() const { return PREDICT_FALSE(str_ != nullptr); }
  std::string* const str_;
};

template <typename T1, typename T2>
CheckOpString MakeCheckOpString(const T1& v1, const T2& v2,
                                const char* exprtext) {
  std::ostringstream oss;
  oss << exprtext << " (" << v1 << " vs. " << v2 << ")";
  return CheckOpString(new std::string(oss.str()));
}

#define DEFINE_CHECK_OP_IMPL(name, op)                                    \
  template <typename T1, typename T2>                                     \
  inline CheckOpString name##Impl(const T1& v1, const T2& v2,             \
                                  const char* exprtext) {                 \
    if (PREDICT_TRUE(v1 op v2)) {                                         \
      return CheckOpString(nullptr);                                      \
    } else {                                                              \
      return (MakeCheckOpString)(v1, v2, exprtext);                       \
    }                                                                     \
  }                                                                       \
  inline CheckOpString name##Impl(int v1, int v2, const char* exprtext) { \
    return (name##Impl<int, int>)(v1, v2, exprtext);                      \
  }
DEFINE_CHECK_OP_IMPL(Check_EQ, ==)
DEFINE_CHECK_OP_IMPL(Check_NE, !=)
DEFINE_CHECK_OP_IMPL(Check_LE, <=)
DEFINE_CHECK_OP_IMPL(Check_LT, <)
DEFINE_CHECK_OP_IMPL(Check_GE, >=)
DEFINE_CHECK_OP_IMPL(Check_GT, >)
#undef DEFINE_CHECK_OP_IMPL

class LogMessage {
 public:
  LogMessage(const char* file, int line) {
    std::clog << "[" << file << ":" << line << "] ";
  }

  ~LogMessage() { std::clog << "\n"; }

  std::ostream& stream() && { return std::clog; }
};

class LogMessageFatal {
 public:
  LogMessageFatal(const char* file, int line) {
    stream_ << "[" << file << ":" << line << "] ";
  }

  LogMessageFatal(const char* file, int line, const CheckOpString& result) {
    stream_ << "[" << file << ":" << line << "] Check failed: " << *result.str_;
  }

  ~LogMessageFatal() NORETURN;

  std::ostream& stream() && { return stream_; }

 private:
  std::ostringstream stream_;
};

inline LogMessageFatal::~LogMessageFatal() {
  std::cerr << stream_.str() << std::endl;
  std::abort();
}

struct NullStream {};

template <typename T>
NullStream&& operator<<(NullStream&& s, T&&) {
  return std::move(s);
}

enum class LogSeverity {
  kFatal,
  kNonFatal,
};

LogMessage LogStream(
    std::integral_constant<LogSeverity, LogSeverity::kNonFatal>);
LogMessageFatal LogStream(
    std::integral_constant<LogSeverity, LogSeverity::kFatal>);

struct Voidify {
  void operator&(std::ostream&) {}
};

class LogEveryNState {
 public:
  bool ShouldLog(int n);
  uint32_t counter() { return counter_.load(std::memory_order_relaxed); }

 private:
  std::atomic<uint32_t> counter_{0};
};

class LogFirstNState {
 public:
  bool ShouldLog(int n);
  uint32_t counter() { return counter_.load(std::memory_order_relaxed); }

 private:
  std::atomic<uint32_t> counter_{0};
};

class LogEveryPow2State {
 public:
  bool ShouldLog(int ignored);
  uint32_t counter() { return counter_.load(std::memory_order_relaxed); }

 private:
  std::atomic<uint32_t> counter_{0};
};

}  // namespace internal
}  // namespace courier

#define COURIER_CHECK_OP_LOG(name, op, val1, val2, log)                \
  while (::courier::internal::CheckOpString _result =                  \
             ::courier::internal::name##Impl(val1, val2,               \
                                             #val1 " " #op " " #val2)) \
  log(__FILE__, __LINE__, _result).stream()

#define COURIER_CHECK_OP(name, op, val1, val2) \
  COURIER_CHECK_OP_LOG(name, op, val1, val2,   \
                       ::courier::internal::LogMessageFatal)

#define COURIER_CHECK_EQ(val1, val2) COURIER_CHECK_OP(Check_EQ, ==, val1, val2)
#define COURIER_CHECK_NE(val1, val2) COURIER_CHECK_OP(Check_NE, !=, val1, val2)
#define COURIER_CHECK_LE(val1, val2) COURIER_CHECK_OP(Check_LE, <=, val1, val2)
#define COURIER_CHECK_LT(val1, val2) COURIER_CHECK_OP(Check_LT, <, val1, val2)
#define COURIER_CHECK_GE(val1, val2) COURIER_CHECK_OP(Check_GE, >=, val1, val2)
#define COURIER_CHECK_GT(val1, val2) COURIER_CHECK_OP(Check_GT, >, val1, val2)

#define COURIER_QCHECK_EQ(val1, val2) COURIER_CHECK_OP(Check_EQ, ==, val1, val2)
#define COURIER_QCHECK_NE(val1, val2) COURIER_CHECK_OP(Check_NE, !=, val1, val2)
#define COURIER_QCHECK_LE(val1, val2) COURIER_CHECK_OP(Check_LE, <=, val1, val2)
#define COURIER_QCHECK_LT(val1, val2) COURIER_CHECK_OP(Check_LT, <, val1, val2)
#define COURIER_QCHECK_GE(val1, val2) COURIER_CHECK_OP(Check_GE, >=, val1, val2)
#define COURIER_QCHECK_GT(val1, val2) COURIER_CHECK_OP(Check_GT, >, val1, val2)

#define COURIER_CHECK(condition)                                   \
  while (auto _result = ::courier::internal::CheckOpString(        \
             (condition) ? nullptr : new std::string(#condition))) \
  ::courier::internal::LogMessageFatal(__FILE__, __LINE__, _result).stream()

#define COURIER_QCHECK(condition) COURIER_CHECK(condition)

#define COURIER_FATAL ::courier::internal::LogSeverity::kFatal
#define COURIER_QFATAL ::courier::internal::LogSeverity::kFatal
#define COURIER_INFO ::courier::internal::LogSeverity::kNonFatal
#define COURIER_WARNING ::courier::internal::LogSeverity::kNonFatal
#define COURIER_ERROR ::courier::internal::LogSeverity::kNonFatal

#define COURIER_LOG(level)                                                 \
  decltype(::courier::internal::LogStream(                                 \
      std::integral_constant<::courier::internal::LogSeverity, level>()))( \
      __FILE__, __LINE__)                                                  \
      .stream()

#define COURIER_VLOG(level) ::courier::internal::NullStream()

#define COURIER_LOG_IF(level, condition)                                 \
  !(condition)                                                           \
      ? static_cast<void>(0)                                             \
      : ::courier::internal::Voidify() &                                 \
            decltype(::courier::internal::LogStream(                     \
                std::integral_constant<::courier::internal::LogSeverity, \
                                       level>()))(__FILE__, __LINE__)    \
                .stream()

#define COURIER_LOGGING_INTERNAL_STATEFUL_CONDITION(kind, condition, arg) \
  for (bool logging_internal_stateful_condition_do_log(condition);        \
       logging_internal_stateful_condition_do_log;                        \
       logging_internal_stateful_condition_do_log = false)                \
    for (static ::courier::internal::Log##kind##State                     \
             logging_internal_stateful_condition_state;                   \
         logging_internal_stateful_condition_do_log &&                    \
         logging_internal_stateful_condition_state.ShouldLog(arg);        \
         logging_internal_stateful_condition_do_log = false)              \
      for (const uint32_t COUNTER ABSL_ATTRIBUTE_UNUSED =                 \
               logging_internal_stateful_condition_state.counter();       \
           logging_internal_stateful_condition_do_log;                    \
           logging_internal_stateful_condition_do_log = false)

#define COURIER_LOG_EVERY_N(level, n)                          \
  COURIER_LOGGING_INTERNAL_STATEFUL_CONDITION(EveryN, true, n) \
  COURIER_LOG(level)

#define COURIER_LOG_FIRST_N(level, n)                          \
  COURIER_LOGGING_INTERNAL_STATEFUL_CONDITION(FirstN, true, n) \
  COURIER_LOG(level)

#define COURIER_LOG_EVERY_POW_2(level)                            \
  COURIER_LOGGING_INTERNAL_STATEFUL_CONDITION(EveryPow2, true, 0) \
  COURIER_LOG(level)

#endif  // COURIER_PLATFORM_DEFAULT_LOGGING_H_
