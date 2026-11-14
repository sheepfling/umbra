#pragma once

#include <RTI/RTI1516.h>

enum class RegionalInteractionAdvance { tar, nmr, nmra, unregulated_timestamp };

struct RegionalInteractionOrderRow {
  bool timestampPreferred;
  bool senderRegulates;
  bool timestampPresent;
  bool receiverTimeConstrained = false;
};

void verifyProcessTsoRegionalInteraction(
    RegionalInteractionAdvance requestKind,
    RegionalInteractionOrderRow const* orderRow = nullptr,
    rti1516_2025::CallbackModel callbackModel = rti1516_2025::HLA_EVOKED);
