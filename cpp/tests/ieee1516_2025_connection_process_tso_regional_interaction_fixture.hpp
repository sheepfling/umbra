#pragma once

enum class RegionalInteractionAdvance { tar, nmr, nmra, unregulated_timestamp };

struct RegionalInteractionOrderRow {
  bool timestampPreferred;
  bool senderRegulates;
  bool timestampPresent;
  bool receiverTimeConstrained = false;
};

void verifyProcessTsoRegionalInteraction(
    RegionalInteractionAdvance requestKind,
    RegionalInteractionOrderRow const* orderRow = nullptr);
