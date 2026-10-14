#pragma once

#include "tck_harness.hpp"

namespace hla_rti_cpp_tck::detail {

inline constexpr std::size_t portableLogicalTimeEncodingSize =
    sizeof(std::uint64_t);

class PortableLogicalTimeInterval;

class PortableLogicalTime final : public rti::LogicalTime {
 public:
  PortableLogicalTime();
  explicit PortableLogicalTime(std::int64_t value);

  void setInitial() override;
  bool isInitial() const override;
  void setFinal() override;
  bool isFinal() const override;
  PortableLogicalTime& operator=(rti::LogicalTime const& value) override;
  PortableLogicalTime& operator+=(
      rti::LogicalTimeInterval const& addend) override;
  PortableLogicalTime& operator-=(
      rti::LogicalTimeInterval const& subtrahend) override;
  bool operator>(rti::LogicalTime const& value) const override;
  bool operator<(rti::LogicalTime const& value) const override;
  bool operator==(rti::LogicalTime const& value) const override;
  bool operator>=(rti::LogicalTime const& value) const override;
  bool operator<=(rti::LogicalTime const& value) const override;
  rti::VariableLengthData encode() const override;
  std::size_t encode(void* buffer, std::size_t bufferSize) const override;
  std::size_t encodedLength() const override;
  void decode(rti::VariableLengthData const& encodedLogicalTime) override;
  void decode(void const* buffer, std::size_t bufferSize) override;
  std::wstring toString() const override;
  std::wstring implementationName() const override;
  std::int64_t value() const;

 private:
  std::int64_t value_ = 0;
};

class PortableLogicalTimeInterval final : public rti::LogicalTimeInterval {
 public:
  PortableLogicalTimeInterval();
  explicit PortableLogicalTimeInterval(std::int64_t value);

  void setZero() override;
  bool isZero() const override;
  void setEpsilon() override;
  bool isEpsilon() const override;
  PortableLogicalTimeInterval& operator=(
      rti::LogicalTimeInterval const& value) override;
  PortableLogicalTimeInterval& operator+=(
      rti::LogicalTimeInterval const& addend) override;
  PortableLogicalTimeInterval& operator-=(
      rti::LogicalTimeInterval const& subtrahend) override;
  bool operator>(rti::LogicalTimeInterval const& value) const override;
  bool operator<(rti::LogicalTimeInterval const& value) const override;
  bool operator==(rti::LogicalTimeInterval const& value) const override;
  bool operator>=(rti::LogicalTimeInterval const& value) const override;
  bool operator<=(rti::LogicalTimeInterval const& value) const override;
  void setToDifference(
      rti::LogicalTime const& minuend,
      rti::LogicalTime const& subtrahend) override;
  rti::VariableLengthData encode() const override;
  std::size_t encode(void* buffer, std::size_t bufferSize) const override;
  std::size_t encodedLength() const override;
  void decode(rti::VariableLengthData const& encodedValue) override;
  void decode(void const* buffer, std::size_t bufferSize) override;
  std::wstring toString() const override;
  std::wstring implementationName() const override;
  std::int64_t value() const;

 private:
  std::int64_t value_ = 0;
};

class PortableLogicalTimeFactory final : public rti::LogicalTimeFactory {
 public:
  std::unique_ptr<rti::LogicalTime> makeInitial() override;
  std::unique_ptr<rti::LogicalTime> makeFinal() override;
  std::unique_ptr<rti::LogicalTimeInterval> makeZero() override;
  std::unique_ptr<rti::LogicalTimeInterval> makeEpsilon() override;
  std::unique_ptr<rti::LogicalTime> decodeLogicalTime(
      rti::VariableLengthData const& encodedLogicalTime) override;
  std::unique_ptr<rti::LogicalTime> decodeLogicalTime(
      void const* buffer,
      std::size_t bufferSize) override;
  std::unique_ptr<rti::LogicalTimeInterval> decodeLogicalTimeInterval(
      rti::VariableLengthData const& encodedValue) override;
  std::unique_ptr<rti::LogicalTimeInterval> decodeLogicalTimeInterval(
      void const* buffer,
      std::size_t bufferSize) override;
  std::wstring getName() const override;
};

void verifyReferenceTimeTypes();
void verifyLogicalTimeDataElements(Session& session, TimeContext const& time);

void scenarioLogicalTimeFactoryPortable(
    Options const& options,
    rti::CallbackModel model);
void scenarioLogicalTimePortable(Options const& options, rti::CallbackModel model);
void scenarioLogicalTimeFactoryFactoryPortable(
    Options const& options,
    rti::CallbackModel model);

}  // namespace hla_rti_cpp_tck::detail
