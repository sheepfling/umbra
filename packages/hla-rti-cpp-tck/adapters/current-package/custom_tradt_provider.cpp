#include "internal/time/federation_time_factory_plugin.hpp"

#include <RTI/Exception.h>
#include <RTI/VariableLengthData.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>
#include <RTI/time/HLAinteger64TimeFactory.h>
#include <RTI/time/LogicalTimeFactory.h>

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

namespace {

constexpr wchar_t customImplementationName[] = L"TckTaggedTime";
constexpr std::uint64_t timeKindBit = std::uint64_t{1} << 62U;
constexpr std::uint64_t timeValueMask = timeKindBit - 1U;

class TaggedTime final : public rti1516_2025::HLAinteger64Time {
 public:
  using HLAinteger64Time::HLAinteger64Time;

  void setInitial() override {
    setTime(static_cast<rti1516_2025::Integer64>(timeKind() | 0U));
  }

  [[nodiscard]] bool isInitial() const override { return timeValue() == 0U; }

  void setFinal() override {
    setTime(static_cast<rti1516_2025::Integer64>(timeKind() | timeValueMask));
  }

  [[nodiscard]] bool isFinal() const override {
    return timeValue() == timeValueMask;
  }

  [[nodiscard]] std::wstring implementationName() const override {
    return customImplementationName;
  }

 private:
  [[nodiscard]] std::uint64_t timeKind() const {
    return static_cast<std::uint64_t>(getTime()) & timeKindBit;
  }

  [[nodiscard]] std::uint64_t timeValue() const {
    return static_cast<std::uint64_t>(getTime()) & timeValueMask;
  }
};

class TaggedInterval final : public rti1516_2025::HLAinteger64Interval {
 public:
  using HLAinteger64Interval::HLAinteger64Interval;

  [[nodiscard]] std::wstring implementationName() const override {
    return customImplementationName;
  }
};

class TaggedTimeFactory final : public rti1516_2025::HLAinteger64TimeFactory {
 public:
  std::unique_ptr<rti1516_2025::HLAinteger64Time> makeLogicalTime(
      rti1516_2025::Integer64 value) override {
    return std::make_unique<TaggedTime>(value);
  }

  std::unique_ptr<rti1516_2025::LogicalTime> makeInitial() override {
    auto time = std::make_unique<TaggedTime>();
    time->setInitial();
    return time;
  }

  std::unique_ptr<rti1516_2025::LogicalTime> makeFinal() override {
    auto time = std::make_unique<TaggedTime>();
    time->setFinal();
    return time;
  }

  std::unique_ptr<rti1516_2025::HLAinteger64Interval>
  makeLogicalTimeInterval(rti1516_2025::Integer64 value) override {
    return std::make_unique<TaggedInterval>(value);
  }

  std::unique_ptr<rti1516_2025::LogicalTimeInterval> makeZero() override {
    auto interval = std::make_unique<TaggedInterval>();
    interval->setZero();
    return interval;
  }

  std::unique_ptr<rti1516_2025::LogicalTimeInterval> makeEpsilon() override {
    auto interval = std::make_unique<TaggedInterval>();
    interval->setEpsilon();
    return interval;
  }

  std::unique_ptr<rti1516_2025::LogicalTime> decodeLogicalTime(
      rti1516_2025::VariableLengthData const& encoded) override {
    auto time = std::make_unique<TaggedTime>();
    time->decode(encoded);
    return time;
  }

  std::unique_ptr<rti1516_2025::LogicalTime> decodeLogicalTime(
      void const* buffer,
      std::size_t bufferSize) override {
    auto time = std::make_unique<TaggedTime>();
    time->decode(buffer, bufferSize);
    return time;
  }

  std::unique_ptr<rti1516_2025::LogicalTimeInterval> decodeLogicalTimeInterval(
      rti1516_2025::VariableLengthData const& encoded) override {
    auto interval = std::make_unique<TaggedInterval>();
    interval->decode(encoded);
    return interval;
  }

  std::unique_ptr<rti1516_2025::LogicalTimeInterval> decodeLogicalTimeInterval(
      void const* buffer,
      std::size_t bufferSize) override {
    auto interval = std::make_unique<TaggedInterval>();
    interval->decode(buffer, bufferSize);
    return interval;
  }

  [[nodiscard]] std::wstring getName() const override {
    return customImplementationName;
  }
};

}  // namespace

extern "C" UMBRA_FEDTIME_PROVIDER_EXPORT
rti1516_2025::LogicalTimeFactory* umbra_fedtime_factory_v1(
    wchar_t const* requestedImplementationName) {
  if (requestedImplementationName == nullptr ||
      std::wstring_view(customImplementationName) !=
          requestedImplementationName) {
    return nullptr;
  }
  return new TaggedTimeFactory();
}
