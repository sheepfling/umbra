#include <RTI/Enums.h>
#include <RTI/Exception.h>
#include <RTI/FederateAmbassador.h>
#include <RTI/Handle.h>
#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/RTIambassador.h>
#include <RTI/RTIambassadorFactory.h>
#include <RTI/RangeBounds.h>
#include <RTI/RtiConfiguration.h>
#include <RTI/Typedefs.h>
#include <RTI/VariableLengthData.h>
#include <RTI/auth/AuthorizationResult.h>
#include <RTI/auth/Authorizer.h>
#include <RTI/auth/AuthorizerFactory.h>
#include <RTI/auth/Credentials.h>
#include <RTI/auth/HLAauthorizerFactoryFactory.h>
#include <RTI/auth/HLAnoCredentials.h>
#include <RTI/auth/HLAplainTextPassword.h>
#include <RTI/encoding/BasicDataElements.h>
#include <RTI/encoding/EncodingExceptions.h>
#include <RTI/encoding/HLAextendableVariantRecord.h>
#include <RTI/encoding/HLAfixedArray.h>
#include <RTI/encoding/HLAfixedRecord.h>
#include <RTI/encoding/HLAlogicalTime.h>
#include <RTI/encoding/HLAlogicalTimeInterval.h>
#include <RTI/encoding/HLAopaqueData.h>
#include <RTI/encoding/HLAvariableArray.h>
#include <RTI/encoding/HLAvariantRecord.h>
#include <RTI/libauth/AuthorizerFactoryFactory.h>
#include <RTI/libfedtime/LogicalTimeFactoryFactory.h>
#include <RTI/time/HLAfloat64Interval.h>
#include <RTI/time/HLAfloat64Time.h>
#include <RTI/time/HLAfloat64TimeFactory.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>
#include <RTI/time/HLAinteger64TimeFactory.h>
#include <RTI/time/HLAlogicalTimeFactoryFactory.h>
#include <RTI/time/LogicalTime.h>
#include <RTI/time/LogicalTimeFactory.h>
#include <RTI/time/LogicalTimeInterval.h>

#include <type_traits>

#if __cplusplus < 201402L || __cplusplus >= 201703L
#error "The installed IEEE 1516.1-2025 C++ API headers must compile as C++14"
#endif

static_assert(std::is_class<rti1516_2025::RTIambassador>::value,
              "RTIambassador must be declared by the C++14 API headers");
static_assert(std::is_class<rti1516_2025::RTIambassadorFactory>::value,
              "RTIambassadorFactory must be declared by the C++14 API headers");
static_assert(std::is_class<rti1516_2025::LogicalTime>::value,
              "LogicalTime must be declared by the C++14 API headers");
static_assert(std::is_class<rti1516_2025::LogicalTimeInterval>::value,
              "LogicalTimeInterval must be declared by the C++14 API headers");
static_assert(std::is_class<rti1516_2025::LogicalTimeFactory>::value,
              "LogicalTimeFactory must be declared by the C++14 API headers");
static_assert(std::is_class<rti1516_2025::HLAinteger64Time>::value,
              "HLAinteger64Time must be declared by the C++14 API headers");
static_assert(std::is_class<rti1516_2025::HLAfloat64Time>::value,
              "HLAfloat64Time must be declared by the C++14 API headers");
