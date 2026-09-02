#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace umbra::detail {

// The durable save record contains a route-free, versioned state-image payload
// in addition to the commit identity.  Live callback routes and other
// process-owned endpoints are not serialized by this seam; the registry
// rebinds those endpoints from the currently joined ambassadors during
// restore.  The payload's own format marker allows the state-image schema to
// evolve independently from the public RTI API.
struct FederationSaveCommitDescriptor final {
  std::wstring federationName;
  std::wstring label;
  std::wstring logicalTimeImplementationName;
  std::vector<std::uint64_t> memberFederateIds;
  bool timed = false;
  // Canonical FederationStateImageCodec payload.  An empty value is accepted
  // only for legacy v1 manifests; newly committed records always carry a
  // non-empty versioned image.
  std::string stateImage;
};

class FederationSaveCommitStore {
 public:
  virtual ~FederationSaveCommitStore() = default;
  virtual void commit(FederationSaveCommitDescriptor const& descriptor) = 0;
  // Returns the newest committed envelope for the federation/label pair, or
  // nullopt when no durable commit is present. A malformed persisted record
  // is an error, never an implicit empty image.
  [[nodiscard]] virtual std::optional<FederationSaveCommitDescriptor> load(
      std::wstring const& federationName,
      std::wstring const& label) const = 0;
};

// The default registry seam keeps the current process-local behavior while
// making save durability observable and replaceable in focused unit tests.
class MemoryFederationSaveCommitStore final : public FederationSaveCommitStore {
 public:
  void commit(FederationSaveCommitDescriptor const& descriptor) override;

  [[nodiscard]] std::optional<FederationSaveCommitDescriptor> load(
      std::wstring const& federationName,
      std::wstring const& label) const override;

  [[nodiscard]] std::vector<FederationSaveCommitDescriptor>
  snapshotCommits() const;

 private:
  mutable std::mutex mutex_;
  std::vector<FederationSaveCommitDescriptor> commits_;
};

// Runtime-capable persistence seam.  Each successful commit creates one
// append-only JSON manifest beneath the configured directory.  The manifest
// carries the versioned state-image payload and remains append-only so an
// older label can be audited after a later save.
class FilesystemFederationSaveCommitStore final : public FederationSaveCommitStore {
 public:
  explicit FilesystemFederationSaveCommitStore(std::filesystem::path directory);

  void commit(FederationSaveCommitDescriptor const& descriptor) override;

  [[nodiscard]] std::optional<FederationSaveCommitDescriptor> load(
      std::wstring const& federationName,
      std::wstring const& label) const override;

  [[nodiscard]] std::filesystem::path directory() const;

 private:
  std::filesystem::path directory_;
};

}  // namespace umbra::detail
