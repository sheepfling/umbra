package hla.rti1516_2025.encoding;

import hla.rti1516_2025.LogicalTimeInterval;

public interface HLAlogicalTimeInterval extends DataElement {
   LogicalTimeInterval getValue();
   HLAlogicalTimeInterval setValue(LogicalTimeInterval value);
}
