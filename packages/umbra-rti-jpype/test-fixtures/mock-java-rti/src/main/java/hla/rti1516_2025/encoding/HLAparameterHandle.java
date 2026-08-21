package hla.rti1516_2025.encoding;

import hla.rti1516_2025.ParameterHandle;

public interface HLAparameterHandle extends DataElement {
   ParameterHandle getValue();
   HLAparameterHandle setValue(ParameterHandle value);
}
