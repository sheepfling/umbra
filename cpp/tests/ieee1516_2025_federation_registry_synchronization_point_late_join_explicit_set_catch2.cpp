#include "ieee1516_2025_federation_registry_test_support.hpp"

TEST_CASE(
    "Synchronization-point late-join expansion respects an explicit synchronization set",
    "[unit][kernel][federation-management][federation-registry][synchronization][late-join]") {
  EmbeddedFederationRegistry registry;
  REQUIRE(registry.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::applied);

  auto first = registry.join(
      L"exercise", L"trainer", L"first", noOpCallbackRoute());
  REQUIRE(first.status == FederationRegistryStatus::applied);
  REQUIRE(first.membership);
  auto const firstId = first.membership->id;

  auto explicitRegistration = registry.registerSynchronizationPoint(
      L"exercise",
      firstId,
      L"explicit-scope",
      {0x01U},
      {firstId},
      true);
  REQUIRE(explicitRegistration.status ==
      umbra::detail::SynchronizationPointRegistrationStatus::applied);
  REQUIRE(explicitRegistration.succeeded);

  auto second = registry.join(
      L"exercise", L"trainer", L"second", noOpCallbackRoute());
  REQUIRE(second.status == FederationRegistryStatus::applied);
  REQUIRE(second.membership);
  auto const secondId = second.membership->id;
  auto explicitAnnouncements = registry.announcePendingSynchronizationPoints(
      L"exercise", secondId);
  REQUIRE(explicitAnnouncements.status ==
      umbra::detail::SynchronizationPointAnnouncementStatus::applied);
  REQUIRE(explicitAnnouncements.announcements.empty());

  auto defaultRegistration = registry.registerSynchronizationPoint(
      L"exercise",
      firstId,
      L"default-scope",
      {0x02U},
      {},
      false);
  REQUIRE(defaultRegistration.status ==
      umbra::detail::SynchronizationPointRegistrationStatus::applied);
  REQUIRE(defaultRegistration.succeeded);

  auto third = registry.join(
      L"exercise", L"trainer", L"third", noOpCallbackRoute());
  REQUIRE(third.status == FederationRegistryStatus::applied);
  REQUIRE(third.membership);
  auto const thirdId = third.membership->id;
  auto announcements = registry.announcePendingSynchronizationPoints(
      L"exercise", thirdId);
  REQUIRE(announcements.status ==
      umbra::detail::SynchronizationPointAnnouncementStatus::applied);
  REQUIRE(announcements.announcements.size() == 1U);
  REQUIRE(announcements.announcements.front().label == L"default-scope");
  REQUIRE(announcements.announcements.front().receivingFederateId == thirdId);
  REQUIRE(announcements.announcements.front().userSuppliedTag ==
      std::vector<unsigned char>{0x02U});
}
