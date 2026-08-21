package hla.rti1516_2025.encoding;

import hla.rti1516_2025.ObjectClassHandle;

public interface HLAobjectClassHandle extends DataElement {
   ObjectClassHandle getValue();
   HLAobjectClassHandle setValue(ObjectClassHandle value);
}
