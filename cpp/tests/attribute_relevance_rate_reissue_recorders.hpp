#pragma once

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace umbra::test {

// Keep immediate callback writes and test-thread assertions synchronized.
class RateReissueOwnerAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct State {
    std::size_t turnedOnCount = 0U;
    rti1516_2025::ObjectInstanceHandle turnedOnObjectInstance;
    rti1516_2025::AttributeHandleSet turnedOnAttributes;
    std::vector<std::optional<std::wstring>> turnedOnRates;
    std::size_t turnedOffCount = 0U;
    rti1516_2025::ObjectInstanceHandle turnedOffObjectInstance;
    rti1516_2025::AttributeHandleSet turnedOffAttributes;
  };

  void turnUpdatesOnForObjectInstance(
      rti1516_2025::ObjectInstanceHandle const& objectInstance,
      rti1516_2025::AttributeHandleSet const& attributes) override {
    recordOn(objectInstance, attributes, std::nullopt);
  }

  void turnUpdatesOnForObjectInstance(
      rti1516_2025::ObjectInstanceHandle const& objectInstance,
      rti1516_2025::AttributeHandleSet const& attributes,
      std::wstring const& updateRateDesignator) override {
    recordOn(objectInstance, attributes, updateRateDesignator);
  }

  void turnUpdatesOffForObjectInstance(
      rti1516_2025::ObjectInstanceHandle const& objectInstance,
      rti1516_2025::AttributeHandleSet const& attributes) override {
    {
      std::lock_guard lock(mutex_);
      ++state_.turnedOffCount;
      state_.turnedOffObjectInstance = objectInstance;
      state_.turnedOffAttributes = attributes;
    }
    condition_.notify_all();
  }

  [[nodiscard]] State snapshot() const {
    std::lock_guard lock(mutex_);
    return state_;
  }

  [[nodiscard]] bool waitForOnCount(std::size_t count) {
    std::unique_lock lock(mutex_);
    return condition_.wait_for(lock, std::chrono::seconds(2), [&] {
      return state_.turnedOnCount >= count;
    });
  }

  [[nodiscard]] bool waitForOffCount(std::size_t count) {
    std::unique_lock lock(mutex_);
    return condition_.wait_for(lock, std::chrono::seconds(2), [&] {
      return state_.turnedOffCount >= count;
    });
  }

  void resetOnHistory() {
    std::lock_guard lock(mutex_);
    state_.turnedOnCount = 0U;
    state_.turnedOnRates.clear();
    state_.turnedOnAttributes.clear();
  }

 private:
  void recordOn(
      rti1516_2025::ObjectInstanceHandle const& objectInstance,
      rti1516_2025::AttributeHandleSet const& attributes,
      std::optional<std::wstring> rate) {
    {
      std::lock_guard lock(mutex_);
      ++state_.turnedOnCount;
      state_.turnedOnObjectInstance = objectInstance;
      state_.turnedOnAttributes = attributes;
      state_.turnedOnRates.emplace_back(std::move(rate));
    }
    condition_.notify_all();
  }

  mutable std::mutex mutex_;
  std::condition_variable condition_;
  State state_;
};

class RateReissueReceiverAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct State {
    bool discovered = false;
    rti1516_2025::ObjectInstanceHandle discoveredObjectInstance;
    rti1516_2025::ObjectClassHandle discoveredObjectClass;
    std::wstring discoveredObjectInstanceName;
    rti1516_2025::FederateHandle discoveredProducingFederate;
  };

  void discoverObjectInstance(
      rti1516_2025::ObjectInstanceHandle const& objectInstance,
      rti1516_2025::ObjectClassHandle const& objectClass,
      std::wstring const& objectInstanceName,
      rti1516_2025::FederateHandle const& producingFederate) override {
    {
      std::lock_guard lock(mutex_);
      state_.discovered = true;
      state_.discoveredObjectInstance = objectInstance;
      state_.discoveredObjectClass = objectClass;
      state_.discoveredObjectInstanceName = objectInstanceName;
      state_.discoveredProducingFederate = producingFederate;
    }
    condition_.notify_all();
  }

  [[nodiscard]] bool waitForDiscovery() {
    std::unique_lock lock(mutex_);
    return condition_.wait_for(lock, std::chrono::seconds(2), [&] {
      return state_.discovered;
    });
  }

  [[nodiscard]] State snapshot() const {
    std::lock_guard lock(mutex_);
    return state_;
  }

 private:
  mutable std::mutex mutex_;
  std::condition_variable condition_;
  State state_;
};

}  // namespace umbra::test
