#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string_view>

namespace umbra::detail {

// Assigns opaque, federation-execution-wide values to handle-valued FOM names.
// The first assignment is drawn from the operating system's cryptographic
// random source. Copies share the name/value catalog for the lifetime of one
// RTI registry, so the same FDD designator receives the same value in later
// federation executions while distinct designators remain collision-checked.
// Zero remains the invalid-handle value; values 1 and 2 are reserved for the
// two predefined transportation type handles.
class FomHandleAllocator final {
 public:
  using RandomValueProvider = std::function<std::optional<std::uint64_t>()>;

  explicit FomHandleAllocator(RandomValueProvider randomValueProvider = {});
  FomHandleAllocator(FomHandleAllocator const&) = default;
  FomHandleAllocator& operator=(FomHandleAllocator const&) = default;
  FomHandleAllocator(FomHandleAllocator&&) noexcept = default;
  FomHandleAllocator& operator=(FomHandleAllocator&&) noexcept = default;
  ~FomHandleAllocator() = default;

  [[nodiscard]] std::optional<std::uint64_t> allocate(
      std::string_view category,
      std::string_view name,
      std::string_view containingName = {}) noexcept;

 private:
  struct SharedState;
  std::shared_ptr<SharedState> state_;
};

}  // namespace umbra::detail
