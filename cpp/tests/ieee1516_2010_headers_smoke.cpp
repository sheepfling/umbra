#include <type_traits>

#include <RTI/RTI1516.h>
#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTIambassadorFactory.h>
#include <RTI/encoding/BasicDataElements.h>
#include <RTI/encoding/DataElement.h>
#include <RTI/encoding/EncodingConfig.h>
#include <RTI/encoding/EncodingExceptions.h>
#include <RTI/encoding/HLAfixedArray.h>
#include <RTI/encoding/HLAfixedRecord.h>
#include <RTI/encoding/HLAopaqueData.h>
#include <RTI/encoding/HLAvariableArray.h>
#include <RTI/encoding/HLAvariantRecord.h>
#include <RTI/time/HLAfloat64Interval.h>
#include <RTI/time/HLAfloat64Time.h>
#include <RTI/time/HLAfloat64TimeFactory.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>
#include <RTI/time/HLAinteger64TimeFactory.h>

static_assert(HLA_API_MAJOR_VERSION == 2, "IEEE 1516e major version drifted");
static_assert(HLA_API_MINOR_VERSION == 0, "IEEE 1516e minor version drifted");
static_assert(std::has_virtual_destructor<rti1516e::RTIambassador>::value,
              "RTIambassador must be polymorphic");
static_assert(std::is_abstract<rti1516e::RTIambassador>::value,
              "the official RTIambassador must remain abstract");

int main() {
  return 0;
}

