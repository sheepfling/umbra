package hla.rti1516_2025.encoding;

import hla.rti1516_2025.AttributeHandle;

public interface HLAattributeHandle extends DataElement {
   AttributeHandle getValue();
   HLAattributeHandle setValue(AttributeHandle value);
}
