#pragma once

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#include <RTI/RTI1516.h>

// Shared test fixture entry point for the aggregate deletion coverage and its
// focused process-boundary pending-request case.
void verifyTimestampedDeleteObjectInstanceProcessScenario(
    rti1516_2025::CallbackModel callbackModel,
    bool regulateSender,
    bool constrainReceiver = false,
    bool preferReceiveOrder = false,
    bool gateNmraCallbacks = false,
    bool requestNmr = false,
    bool verifyProducerTimeBoundary = false,
    bool receiverKnowsObject = true,
    bool disableProducerRegulation = false,
    bool verifyDeferredLookaheadDeletionBoundary = false);
#endif
