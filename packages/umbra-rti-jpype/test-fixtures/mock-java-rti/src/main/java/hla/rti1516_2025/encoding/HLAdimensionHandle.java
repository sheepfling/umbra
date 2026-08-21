package hla.rti1516_2025.encoding;

import hla.rti1516_2025.DimensionHandle;

public interface HLAdimensionHandle extends DataElement {
   DimensionHandle getValue();
   HLAdimensionHandle setValue(DimensionHandle value);
}
