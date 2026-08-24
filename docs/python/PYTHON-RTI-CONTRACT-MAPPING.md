# Python RTI contract mapping

The Python API is a contract layer, not a third RTI implementation.  Every
abstract method keeps the official Java method spelling so an adapter can be
checked directly against `hla.rti1516_2025`.  The native adapter then forwards
that method to the corresponding C++ `RTI::RTIambassador` operation.  Provider
implementations may translate values and overloads at the edge, but they do
not rename or reinterpret the shared contract.

## Ownership and source surfaces

| Python module | Python role | Java surface | C++ surface |
| --- | --- | --- | --- |
| `hla.rti1516_2025.abstract` | Abstract ambassador, callback, factory, and encoder contracts | `hla.rti1516_2025.RTIambassador`, `FederateAmbassador`, `RtiFactory`, `encoding.DataElement` and `EncoderFactory` | `RTI::RTIambassador`, `RTI::FederateAmbassador`, `RTI::RTIambassadorFactory`, `RTI::encoding::DataElement` |
| `hla.rti1516_2025.values` | Immutable snapshots, handles, sets, maps, time values, and builder values | Java value classes such as `ConfigurationResult`, `TimeQueryReturn`, and handle carriers | `RTI::VariableLengthData`, handle types, `RTI::LogicalTime`, `RTI::LogicalTimeInterval`, and value containers |
| `umbra._native.rti1516_2025.provider` | Concrete pybind11 façade | Adapter target: the same Java-shaped method names | `RTI::RTIambassador` and concrete C++ encoding classes |
| `umbra._java.rti1516_2025.provider` | Concrete JPype façade | Exact Java interfaces and overloads | JNI/native bridge where the selected Java provider is Umbra |
| `umbra._java.rti1516_2025.encoding` | Concrete Java `DataElement` shells | `hla.rti1516_2025.encoding.*` | JNI/native encoding carriers when used by the Umbra Java façade |

The authoritative C++ declarations are in
[`RTIambassador.h`](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h),
[`FederateAmbassador.h`](../../third_party/ieee1516.1-2025/include/RTI/FederateAmbassador.h),
and [`BasicDataElements.h`](../../third_party/ieee1516.1-2025/include/RTI/encoding/BasicDataElements.h).
The Java adapter is validated against the `hla.rti1516_2025` interfaces in its
fixture and against the selected vendor JAR at runtime.

## Method mapping rule

The first identifier in each abstract method is the Java identifier.  For
example:

| Python contract | Java method | C++ method |
| --- | --- | --- |
| `RTIambassador.connect` | `RTIambassador.connect` | `RTIambassador::connect` |
| `RTIambassador.joinFederationExecution` | `RTIambassador.joinFederationExecution` | `RTIambassador::joinFederationExecution` |
| `RTIambassador.sendInteractionWithRegionsWithTime` | `RTIambassador.sendInteractionWithRegions` timestamp overload | `RTIambassador::sendInteraction` timestamp/region overload |
| `FederateAmbassador.reportFederationExecutions` | `FederateAmbassador.reportFederationExecutions` | `FederateAmbassador::reportFederationExecutions` |
| `EncoderFactory.createHLAinteger32BE` | `EncoderFactory.createHLAinteger32BE` | `HLAinteger32BE` construction/helper |
| `DataElement.toByteArray` | `DataElement.toByteArray` | `DataElement::encode` / `VariableLengthData` |

Where Java overloads collapse into one Python signature, the contract
docstring names the overload family and the provider adapter owns overload
selection.  A provider-specific class may expose an additional exact Java or
C++ operation, but that operation is not part of the provider-neutral contract
until both surfaces have a matching value and lifetime model.

## Binary arguments

Read-only payload arguments use `hla.rti1516_2025.BytesLike`.  The provider
boundary accepts `bytes`, `bytearray`, `memoryview`, and other objects that can
be presented through the buffer protocol, then copies them to immutable
`bytes`.  Returned payloads remain `bytes`.  An encode destination uses
`WritableBytes` because the destination must be writable; a read-only
`memoryview` is rejected by the normal slice-assignment operation.

This maps Python values to Java `byte[]`/`ByteWrapper` and C++
`VariableLengthData` without exposing either provider's ownership or cursor
semantics in the shared contract.
