#include "tck_api_foundation.hpp"

#include <RTI/auth/AuthorizationResult.h>
#include <RTI/auth/Authorizer.h>
#include <RTI/auth/AuthorizerFactory.h>
#include <RTI/auth/Credentials.h>
#include <RTI/auth/HLAauthorizerFactoryFactory.h>
#include <RTI/auth/HLAnoCredentials.h>
#include <RTI/auth/HLAplainTextPassword.h>
#include <RTI/encoding/BasicDataElements.h>
#include <RTI/encoding/DataElement.h>
#include <RTI/encoding/EncodingConfig.h>
#include <RTI/encoding/EncodingExceptions.h>
#include <RTI/encoding/HLAextendableVariantRecord.h>
#include <RTI/encoding/HLAfixedArray.h>
#include <RTI/encoding/HLAfixedRecord.h>
#include <RTI/encoding/HLAlogicalTime.h>
#include <RTI/encoding/HLAlogicalTimeInterval.h>
#include <RTI/encoding/HLAopaqueData.h>
#include <RTI/encoding/HLAvariableArray.h>
#include <RTI/encoding/HLAvariantRecord.h>
#include <RTI/Enums.h>
#include <RTI/Exception.h>
#include <RTI/FederateAmbassador.h>
#include <RTI/Handle.h>
#include <RTI/libauth/AuthorizerFactoryFactory.h>
#include <RTI/libfedtime/LogicalTimeFactoryFactory.h>
#include <RTI/NullFederateAmbassador.h>
#include <RTI/RangeBounds.h>
#include <RTI/RTI1516.h>
#include <RTI/RTIambassador.h>
#include <RTI/RTIambassadorFactory.h>
#include <RTI/RtiConfiguration.h>
#include <RTI/SpecificConfig.h>
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
#include <RTI/Typedefs.h>
#include <RTI/VariableLengthData.h>

namespace hla_rti_cpp_tck::detail {

void scenarioAnnexCApiHeaderInventory(
    Options const&, rti::CallbackModel) {
  static_assert(sizeof(rti::RTIambassador) > 0);
  static_assert(sizeof(rti::FederateAmbassador) > 0);
  static_assert(sizeof(rti::NullFederateAmbassador) > 0);
  static_assert(sizeof(rti::RTIambassadorFactory) > 0);
  static_assert(sizeof(rti::RangeBounds) > 0);
  static_assert(sizeof(rti::VariableLengthData) > 0);
}

}  // namespace hla_rti_cpp_tck::detail
