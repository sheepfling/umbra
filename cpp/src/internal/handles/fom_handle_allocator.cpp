#include "internal/handles/fom_handle_allocator.hpp"
#include "internal/handles/transportation_type_handle.hpp"

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <utility>

#if defined(_WIN32)
#include <windows.h>
#include <bcrypt.h>
#elif defined(__linux__)
#include <sys/random.h>
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
#include <cstdlib>
#elif defined(__unix__)
#include <fcntl.h>
#include <unistd.h>
#endif

namespace umbra::detail {
namespace {

bool fillWithSystemRandomBytes(void* output, std::size_t size) noexcept {
#if defined(_WIN32)
  if (size > (std::numeric_limits<ULONG>::max)()) {
    return false;
  }
  return BCryptGenRandom(
             nullptr,
             static_cast<PUCHAR>(output),
             static_cast<ULONG>(size),
             BCRYPT_USE_SYSTEM_PREFERRED_RNG) >= 0;
#elif defined(__linux__)
  auto* cursor = static_cast<unsigned char*>(output);
  std::size_t remaining = size;
  while (remaining != 0U) {
    auto const received = getrandom(cursor, remaining, 0);
    if (received < 0 && errno == EINTR) {
      continue;
    }
    if (received <= 0) {
      return false;
    }
    auto const count = static_cast<std::size_t>(received);
    cursor += count;
    remaining -= count;
  }
  return true;
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
  arc4random_buf(output, size);
  return true;
#elif defined(__unix__)
  int const descriptor = open("/dev/urandom", O_RDONLY);
  if (descriptor < 0) {
    return false;
  }
  auto* cursor = static_cast<unsigned char*>(output);
  std::size_t remaining = size;
  while (remaining != 0U) {
    auto const received = read(descriptor, cursor, remaining);
    if (received < 0 && errno == EINTR) {
      continue;
    }
    if (received <= 0) {
      close(descriptor);
      return false;
    }
    auto const count = static_cast<std::size_t>(received);
    cursor += count;
    remaining -= count;
  }
  close(descriptor);
  return true;
#else
  static_cast<void>(output);
  static_cast<void>(size);
  return false;
#endif
}

std::optional<std::uint64_t> secureRandomHandleValue() noexcept {
  std::uint64_t value{};
  if (!fillWithSystemRandomBytes(&value, sizeof(value))) {
    return std::nullopt;
  }
  return value;
}

void appendIdentityField(std::string& identity, std::string_view field) {
  identity.append(std::to_string(field.size()));
  identity.push_back(':');
  identity.append(field);
}

}  // namespace

struct FomHandleAllocator::SharedState final {
  explicit SharedState(RandomValueProvider provider)
      : randomValueProvider(
            provider ? std::move(provider)
                     : RandomValueProvider{secureRandomHandleValue}) {}

  std::mutex mutex;
  RandomValueProvider randomValueProvider;
  std::map<std::string, std::uint64_t, std::less<>> handlesByIdentity;
  // 0 is invalid; 1 and 2 are reserved for Umbra's predefined transportation
  // handle representations and must not be assigned to FOM names.
  std::set<std::uint64_t> allocated{
      0U,
      rti1516_2025::umbra_binding_detail::kHlaReliableTransportationTypeValue,
      rti1516_2025::umbra_binding_detail::kHlaBestEffortTransportationTypeValue};
};

FomHandleAllocator::FomHandleAllocator(RandomValueProvider randomValueProvider)
    : state_(std::make_shared<SharedState>(std::move(randomValueProvider))) {}

std::optional<std::uint64_t> FomHandleAllocator::allocate(
    std::string_view category,
    std::string_view name,
    std::string_view containingName) noexcept {
  if (!state_ || category.empty() || name.empty()) {
    return std::nullopt;
  }

  try {
    std::string identity;
    identity.reserve(
        category.size() + name.size() + containingName.size() + 48U);
    appendIdentityField(identity, category);
    appendIdentityField(identity, containingName);
    appendIdentityField(identity, name);

    std::scoped_lock lock(state_->mutex);
    auto const existing = state_->handlesByIdentity.find(identity);
    if (existing != state_->handlesByIdentity.end()) {
      return existing->second;
    }

    // A fresh OS-CSPRNG sample is required after a collision. Do not fall back
    // to a counter: entropy-source failure must fail FOM reconciliation. The
    // RTI registry retains successful name assignments across executions.
    for (std::size_t attempt = 0U; attempt < 256U; ++attempt) {
      auto const candidate = state_->randomValueProvider();
      if (!candidate) {
        return std::nullopt;
      }
      if (*candidate == 0U) {
        continue;
      }

      auto [allocatedPosition, insertedHandle] =
          state_->allocated.insert(*candidate);
      if (!insertedHandle) {
        continue;
      }
      try {
        auto [identityPosition, insertedIdentity] =
            state_->handlesByIdentity.emplace(identity, *candidate);
        if (!insertedIdentity) {
          state_->allocated.erase(allocatedPosition);
          return identityPosition->second;
        }
      } catch (...) {
        state_->allocated.erase(allocatedPosition);
        throw;
      }
      return candidate;
    }
  } catch (...) {
    return std::nullopt;
  }
  return std::nullopt;
}

}  // namespace umbra::detail
