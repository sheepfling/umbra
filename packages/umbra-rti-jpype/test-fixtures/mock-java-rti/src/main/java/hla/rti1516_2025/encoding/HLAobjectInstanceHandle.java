package hla.rti1516_2025.encoding;

import hla.rti1516_2025.ObjectInstanceHandle;

public interface HLAobjectInstanceHandle extends DataElement {
   ObjectInstanceHandle getValue();
   HLAobjectInstanceHandle setValue(ObjectInstanceHandle value);
}
