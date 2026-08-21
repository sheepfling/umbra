package hla.rti1516_2025.encoding;

import hla.rti1516_2025.RegionHandle;

public interface HLAregionHandle extends DataElement {
   RegionHandle getValue();
   HLAregionHandle setValue(RegionHandle value);
}
