#pragma once

// Names owned by the conformance FOM fixtures.  Standard MOM names live in
// internal/fom/hla_names.hpp; keeping these test-only values separate prevents
// a fixture typo from becoming part of the runtime catalog vocabulary.

namespace umbra::test::hla::wide {

namespace fom {

inline constexpr wchar_t customer[] = L"HLAobjectRoot.Customer";
inline constexpr wchar_t employee[] = L"HLAobjectRoot.Employee";
inline constexpr wchar_t employee_server[] = L"HLAobjectRoot.Employee.Server";
inline constexpr wchar_t food_drink[] = L"HLAobjectRoot.Food.Drink";
inline constexpr wchar_t food_drink_soda[] = L"HLAobjectRoot.Food.Drink.Soda";
inline constexpr wchar_t food_drink_soda_light[] =
    L"HLAobjectRoot.Food.Drink.Soda.Light";
inline constexpr wchar_t regional_thing[] = L"HLAobjectRoot.RegionalThing";
inline constexpr wchar_t two_dimensional_regional_object[] =
    L"HLAobjectRoot.UmbraTwoDimensionalRegionObject";

inline constexpr wchar_t attribute_fixture_base[] =
    L"HLAobjectRoot.UmbraAttributeFixtureBase";
inline constexpr wchar_t attribute_fixture_child[] =
    L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild";
inline constexpr wchar_t attribute_fixture_class[] =
    L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureClass";
inline constexpr wchar_t dimension_fixture_object[] =
    L"HLAobjectRoot.UmbraDimensionFixtureObject";
inline constexpr wchar_t directed_fixture_object[] =
    L"HLAobjectRoot.UmbraDirectedFixtureObject";
inline constexpr wchar_t parameter_fixture_base[] =
    L"HLAinteractionRoot.UmbraParameterFixtureBase";
inline constexpr wchar_t parameter_fixture_child[] =
    L"HLAinteractionRoot.UmbraParameterFixtureBase.UmbraParameterFixtureChild";
inline constexpr wchar_t reference_fixture_class[] =
    L"HLAobjectRoot.UmbraReferenceFixtureClass";
inline constexpr wchar_t transportation_fixture_object[] =
    L"HLAobjectRoot.UmbraTransportationFixtureObject";
inline constexpr wchar_t transportation_regional_object[] =
    L"HLAobjectRoot.UmbraTransportationRegionalObject";

inline constexpr wchar_t customer_seated[] =
    L"HLAinteractionRoot.CustomerTransactions.CustomerSeated";
inline constexpr wchar_t food_served[] =
    L"HLAinteractionRoot.CustomerTransactions.FoodServed";
inline constexpr wchar_t main_course_served[] =
    L"HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed";
inline constexpr wchar_t server_take_order[] =
    L"HLAinteractionRoot.ServerAction.TakeOrder";
inline constexpr wchar_t dimension_fixture_interaction[] =
    L"HLAinteractionRoot.UmbraDimensionFixtureInteraction";
inline constexpr wchar_t directed_fixture_interaction[] =
    L"HLAinteractionRoot.UmbraDirectedFixtureInteraction";
inline constexpr wchar_t parameter_fixture_child_interaction[] =
    L"HLAinteractionRoot.UmbraParameterFixtureBase.UmbraParameterFixtureChild";
inline constexpr wchar_t transportation_fixture_interaction[] =
    L"HLAinteractionRoot.UmbraTransportationFixtureInteraction";
inline constexpr wchar_t transportation_regional_interaction[] =
    L"HLAinteractionRoot.UmbraTransportationRegionalInteraction";
inline constexpr wchar_t extended_set_switches[] =
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetSwitches."
    L"UmbraExtendedSetSwitches";

inline constexpr wchar_t missing_object[] = L"HLAobjectRoot.Missing";
inline constexpr wchar_t missing_object_for_interaction[] =
    L"HLAobjectRoot.MissingForInteractionFailure";
inline constexpr wchar_t missing_interaction[] =
    L"HLAinteractionRoot.MissingInteraction";
inline constexpr wchar_t missing_interaction_base[] =
    L"HLAinteractionRoot.Missing";

}  // namespace fom

namespace fixture {

inline constexpr wchar_t bar_quantity[] = L"BarQuantity";
inline constexpr wchar_t best_effort_base[] = L"BestEffortBase";
inline constexpr wchar_t best_effort_child[] = L"BestEffortChild";
inline constexpr wchar_t directed_target_marker[] = L"DirectedTargetMarker";
inline constexpr wchar_t efficiency[] = L"Efficiency";
inline constexpr wchar_t flavor[] = L"Flavor";
inline constexpr wchar_t identifier[] = L"Identifier";
inline constexpr wchar_t missing[] = L"Missing";
inline constexpr wchar_t missing_dimension[] = L"MissingDimension";
inline constexpr wchar_t missing_parameter[] = L"MissingParameter";
inline constexpr wchar_t name[] = L"Name";
inline constexpr wchar_t organic[] = L"Organic";
inline constexpr wchar_t pay_rate[] = L"PayRate";
inline constexpr wchar_t reliable_base_a[] = L"ReliableBaseA";
inline constexpr wchar_t reliable_base_b[] = L"ReliableBaseB";
inline constexpr wchar_t reliable_child[] = L"ReliableChild";
inline constexpr wchar_t server_id[] = L"ServerId";
inline constexpr wchar_t soda_flavor[] = L"SodaFlavor";
inline constexpr wchar_t sweetener[] = L"Sweetener";
inline constexpr wchar_t temperature_ok[] = L"TemperatureOk";
inline constexpr wchar_t umbra_custom_transport[] = L"UmbraCustomTransport";
inline constexpr wchar_t umbra_dimension_fixture[] = L"UmbraDimensionFixture";
inline constexpr wchar_t umbra_extended_set_switches[] =
    L"UmbraExtendedSetSwitches";
inline constexpr wchar_t umbra_extended_switch_payload[] =
    L"UmbraExtendedSwitchPayload";
inline constexpr wchar_t umbra_extension_switch_payload[] =
    L"UmbraExtensionSwitchPayload";
inline constexpr wchar_t umbra_region_x[] = L"UmbraRegionX";
inline constexpr wchar_t umbra_region_y[] = L"UmbraRegionY";
inline constexpr wchar_t umbra_regional_unreserved[] =
    L"UmbraRegionalUnreserved";
inline constexpr wchar_t umbra_transportation_fixture[] =
    L"UmbraTransportationFixture";
inline constexpr wchar_t umbra_transportation_fixture_attribute[] =
    L"UmbraTransportationFixtureAttribute";
inline constexpr wchar_t unowned_child[] = L"UnownedChild";
inline constexpr wchar_t value[] = L"Value";

}  // namespace fixture

}  // namespace umbra::test::hla::wide
