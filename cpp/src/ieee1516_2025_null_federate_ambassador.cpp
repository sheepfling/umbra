#include <RTI/NullFederateAmbassador.h>

namespace
{

// NullFederateAmbassador is an official all-inline API class. For a shared
// library, explicitly instantiate its vtable in the RTI DLL so consumers can
// derive their own callback ambassadors without a MinGW import-library link
// failure. Static builds already emit the vtable in whichever consumer uses
// the class, so keep this anchor limited to the DLL build profile.
#if defined(BUILDING_RTI)
[[maybe_unused]] rti1516_2025::NullFederateAmbassador null_federate_ambassador_export_anchor;
#endif

} // namespace
