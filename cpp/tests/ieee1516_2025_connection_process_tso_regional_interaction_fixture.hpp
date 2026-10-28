#pragma once

enum class RegionalInteractionAdvance { tar, nmr, nmra, unregulated_timestamp };

void verifyProcessTsoRegionalInteraction(RegionalInteractionAdvance requestKind);
