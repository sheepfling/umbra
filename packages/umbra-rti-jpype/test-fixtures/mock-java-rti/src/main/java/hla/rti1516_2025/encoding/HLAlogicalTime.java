package hla.rti1516_2025.encoding;

import hla.rti1516_2025.LogicalTime;

public interface HLAlogicalTime extends DataElement {
   LogicalTime getValue();
   HLAlogicalTime setValue(LogicalTime value);
}
