#include <jni.h>

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/encoding/BasicDataElements.h>
#include <RTI/encoding/HLAfixedArray.h>
#include <RTI/encoding/HLAfixedRecord.h>
#include <RTI/encoding/HLAopaqueData.h>
#include <RTI/encoding/HLAvariableArray.h>
#include <RTI/encoding/HLAvariantRecord.h>
#include <RTI/time/HLAfloat64Interval.h>
#include <RTI/time/HLAfloat64Time.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>

#include "internal/exception_mapping_2010.hpp"
#include "internal/handles/2010_handle_factories.hpp"
#include "internal/utf8.hpp"

#include <memory>
#include <cstdint>
#include <exception>
#include <string>
#include <vector>

namespace {

constexpr char kInternalErrorClass[] =
    "hla/rti1516e/exceptions/RTIinternalError";
constexpr char kBaseExceptionClass[] =
    "hla/rti1516e/exceptions/RTIexception";
constexpr char kExceptionPackage[] = "hla/rti1516e/exceptions/";
constexpr char kEncodingExceptionPackage[] = "hla/rti1516e/encoding/";
constexpr char kNullBindingMessage[] =
    "Umbra IEEE 1516.1-2010 JNI null binding does not implement this service.";

std::string narrow(std::wstring const& value) {
  return umbra::utf8::encode(value);
}

std::wstring widen(JNIEnv* environment, jstring value) {
  if (value == nullptr) {
    return {};
  }
  jsize const length = environment->GetStringLength(value);
  jchar const* characters = environment->GetStringChars(value, nullptr);
  if (characters == nullptr) {
    return {};
  }
  std::wstring result;
  result.reserve(static_cast<std::size_t>(length));
  for (jsize index = 0; index < length; ++index) {
    std::uint32_t codePoint = characters[index];
    if (sizeof(wchar_t) != sizeof(jchar) &&
        umbra::utf8::isHighSurrogate(codePoint) && index + 1 < length &&
        umbra::utf8::isLowSurrogate(characters[index + 1])) {
      codePoint = 0x10000U + ((codePoint - 0xD800U) << 10U) +
          (static_cast<std::uint32_t>(characters[++index]) - 0xDC00U);
    }
    if (sizeof(wchar_t) == sizeof(jchar)) {
      result.push_back(static_cast<wchar_t>(codePoint));
    } else {
      umbra::utf8::appendWideCodePoint(result, codePoint);
    }
  }
  environment->ReleaseStringChars(value, characters);
  return result;
}

std::string narrow(JNIEnv* environment, jstring value) {
  return narrow(widen(environment, value));
}

jstring newJavaString(JNIEnv* environment, std::wstring const& value) {
  std::vector<jchar> characters;
  characters.reserve(value.size());
  for (std::size_t index = 0; index < value.size(); ++index) {
    std::uint32_t codePoint = static_cast<std::uint32_t>(value[index]);
    if (sizeof(wchar_t) == sizeof(jchar)) {
      characters.push_back(static_cast<jchar>(codePoint));
      continue;
    }
    if (codePoint > 0x10FFFFU) {
      codePoint = 0xFFFDU;
    }
    if (codePoint <= 0xFFFFU) {
      characters.push_back(static_cast<jchar>(codePoint));
    } else {
      codePoint -= 0x10000U;
      characters.push_back(static_cast<jchar>(0xD800U | (codePoint >> 10U)));
      characters.push_back(static_cast<jchar>(0xDC00U | (codePoint & 0x3FFU)));
    }
  }
  return environment->NewString(
      characters.empty() ? nullptr : characters.data(),
      static_cast<jsize>(characters.size()));
}

void throwJavaException(
    JNIEnv* environment, char const* className,
    std::wstring const& message) {
  jclass exceptionClass = environment->FindClass(className);
  if (exceptionClass == nullptr) {
    return;
  }
  jmethodID constructor = environment->GetMethodID(
      exceptionClass, "<init>", "(Ljava/lang/String;)V");
  if (constructor == nullptr) {
    environment->DeleteLocalRef(exceptionClass);
    return;
  }
  jstring detail = newJavaString(environment, message);
  if (detail == nullptr) {
    environment->DeleteLocalRef(exceptionClass);
    return;
  }
  jobject exception = environment->NewObject(exceptionClass, constructor, detail);
  if (exception != nullptr) {
    environment->Throw(static_cast<jthrowable>(exception));
    environment->DeleteLocalRef(exception);
  }
  environment->DeleteLocalRef(detail);
  environment->DeleteLocalRef(exceptionClass);
}

void throwInternal(JNIEnv* environment, std::string const& message) {
  throwJavaException(
      environment, kInternalErrorClass, umbra::utf8::decode(message));
}

void throwRtiException(
    JNIEnv* environment, rti1516e::Exception const& error,
    char const* overrideName = nullptr) {
  char const* name = overrideName == nullptr
      ? umbra::rti1516e_2010::exceptionName(error)
      : overrideName;
  std::string className;
  std::wstring message = error.what();
  if (std::string(name) == "EncoderException" ||
      std::string(name) == "DecoderException") {
    className = std::string(kEncodingExceptionPackage) + name;
  } else if (umbra::rti1516e_2010::isJavaExceptionName(name)) {
    className = std::string(kExceptionPackage) + name;
  } else {
    className = kInternalErrorClass;
    message = umbra::utf8::decode(std::string(name) + ": ") + message;
  }
  throwJavaException(environment, className.c_str(), message);
}

struct NativeContext {
  std::auto_ptr<rti1516e::RTIambassador> ambassador;
  rti1516e::NullFederateAmbassador federateAmbassador;
};

void throwNativeInternalError(JNIEnv* environment) {
  try {
    throw rti1516e::RTIinternalError(
        L"Umbra's IEEE 1516.1-2010 C++ null binding does not implement this service.");
  } catch (rti1516e::Exception const& error) {
    throwRtiException(environment, error);
  }
}

std::vector<rti1516e::Octet> readBytes(JNIEnv* environment, jbyteArray value) {
  if (value == nullptr) {
    throwInternal(environment, "The 2010 JNI type probe received a null byte array.");
    return {};
  }
  jsize const length = environment->GetArrayLength(value);
  jbyte* raw = environment->GetByteArrayElements(value, nullptr);
  if (raw == nullptr) {
    return {};
  }
  std::vector<rti1516e::Octet> result(static_cast<std::size_t>(length));
  for (jsize index = 0; index < length; ++index) {
    result[static_cast<std::size_t>(index)] =
        static_cast<rti1516e::Octet>(static_cast<unsigned char>(raw[index]));
  }
  environment->ReleaseByteArrayElements(value, raw, JNI_ABORT);
  return result;
}

jbyteArray writeBytes(
    JNIEnv* environment, std::vector<rti1516e::Octet> const& value) {
  jbyteArray result = environment->NewByteArray(static_cast<jsize>(value.size()));
  if (result == nullptr || value.empty()) {
    return result;
  }
  std::vector<jbyte> signedValue(value.size());
  for (std::size_t index = 0; index < value.size(); ++index) {
    signedValue[index] = static_cast<jbyte>(value[index]);
  }
  environment->SetByteArrayRegion(
      result, 0, static_cast<jsize>(signedValue.size()), signedValue.data());
  return result;
}

rti1516e::VariableLengthData variableData(
    std::vector<rti1516e::Octet> const& value) {
  rti1516e::VariableLengthData result;
  result.setData(value.empty() ? nullptr : value.data(), value.size());
  return result;
}

std::vector<rti1516e::Octet> bytesFrom(
    rti1516e::VariableLengthData const& value) {
  auto const* data = static_cast<rti1516e::Octet const*>(value.data());
  if (data == nullptr || value.size() == 0U) {
    return {};
  }
  return std::vector<rti1516e::Octet>(data, data + value.size());
}

template <typename Element>
std::vector<rti1516e::Octet> encodeElement(Element const& element) {
  return bytesFrom(element.encode());
}

template <typename Element>
std::vector<rti1516e::Octet> seedBasicElement(Element const& element) {
  return encodeElement(element);
}

template <typename Element>
std::vector<rti1516e::Octet> roundTripBasicElement(
    std::vector<rti1516e::Octet> const& input) {
  Element element;
  element.decode(variableData(input));
  return encodeElement(element);
}

std::vector<rti1516e::Octet> seedCompositeElement(std::string const& kind) {
  if (kind == "HLAopaqueData") {
    rti1516e::Octet const value[] = {
        static_cast<rti1516e::Octet>(0x00U), static_cast<rti1516e::Octet>(0x11U),
        static_cast<rti1516e::Octet>(0xFEU), static_cast<rti1516e::Octet>(0x7FU)};
    rti1516e::HLAopaqueData element(value, sizeof(value));
    return encodeElement(element);
  }
  if (kind == "HLAvariableArray") {
    rti1516e::HLAoctet prototype;
    rti1516e::HLAvariableArray element(prototype);
    rti1516e::HLAoctet first(0x12U);
    rti1516e::HLAoctet second(0x34U);
    element.addElement(first);
    element.addElement(second);
    return encodeElement(element);
  }
  if (kind == "HLAfixedArray") {
    rti1516e::HLAoctet prototype;
    rti1516e::HLAfixedArray element(prototype, 3U);
    rti1516e::HLAoctet first(0x12U);
    rti1516e::HLAoctet second(0x34U);
    rti1516e::HLAoctet third(0x56U);
    element.set(0U, first);
    element.set(1U, second);
    element.set(2U, third);
    return encodeElement(element);
  }
  if (kind == "HLAfixedRecord") {
    rti1516e::HLAfixedRecord element;
    rti1516e::HLAinteger32BE integerValue(123456);
    rti1516e::HLAASCIIstring stringValue("fixed-record");
    element.appendElement(integerValue);
    element.appendElement(stringValue);
    return encodeElement(element);
  }
  if (kind == "HLAvariantRecord") {
    rti1516e::HLAinteger32BE discriminant(1);
    rti1516e::HLAASCIIstring value("variant-record");
    rti1516e::HLAvariantRecord element(discriminant);
    element.addVariant(discriminant, value);
    element.setVariant(discriminant, value);
    return encodeElement(element);
  }
  return {};
}

std::vector<rti1516e::Octet> roundTripCompositeElement(
    std::string const& kind, std::vector<rti1516e::Octet> const& input) {
  if (kind == "HLAopaqueData") {
    rti1516e::HLAopaqueData element;
    element.decode(variableData(input));
    return encodeElement(element);
  }
  if (kind == "HLAvariableArray") {
    rti1516e::HLAoctet prototype;
    rti1516e::HLAvariableArray element(prototype);
    element.decode(variableData(input));
    return encodeElement(element);
  }
  if (kind == "HLAfixedArray") {
    rti1516e::HLAoctet prototype;
    rti1516e::HLAfixedArray element(prototype, 3U);
    element.decode(variableData(input));
    return encodeElement(element);
  }
  if (kind == "HLAfixedRecord") {
    rti1516e::HLAfixedRecord element;
    rti1516e::HLAinteger32BE integerValue;
    rti1516e::HLAASCIIstring stringValue;
    element.appendElement(integerValue);
    element.appendElement(stringValue);
    element.decode(variableData(input));
    return encodeElement(element);
  }
  if (kind == "HLAvariantRecord") {
    rti1516e::HLAinteger32BE discriminantPrototype;
    rti1516e::HLAASCIIstring valuePrototype;
    rti1516e::HLAvariantRecord element(discriminantPrototype);
    rti1516e::HLAinteger32BE discriminant(1);
    rti1516e::HLAASCIIstring value;
    element.addVariant(discriminant, valuePrototype);
    element.decode(variableData(input));
    return encodeElement(element);
  }
  return {};
}

template <typename Handle>
std::vector<rti1516e::Octet> encodeHandle(Handle const& handle) {
  return bytesFrom(handle.encode());
}

template <typename Handle>
std::vector<rti1516e::Octet> seedHandle(std::uint64_t value) {
  return encodeHandle(rti1516e::umbra_binding_detail::makeFederateHandle(value));
}

template <>
std::vector<rti1516e::Octet> seedHandle<rti1516e::ObjectClassHandle>(std::uint64_t value) {
  return encodeHandle(rti1516e::umbra_binding_detail::makeObjectClassHandle(value));
}

template <>
std::vector<rti1516e::Octet> seedHandle<rti1516e::InteractionClassHandle>(std::uint64_t value) {
  return encodeHandle(rti1516e::umbra_binding_detail::makeInteractionClassHandle(value));
}

template <>
std::vector<rti1516e::Octet> seedHandle<rti1516e::ObjectInstanceHandle>(std::uint64_t value) {
  return encodeHandle(rti1516e::umbra_binding_detail::makeObjectInstanceHandle(value));
}

template <>
std::vector<rti1516e::Octet> seedHandle<rti1516e::AttributeHandle>(std::uint64_t value) {
  return encodeHandle(rti1516e::umbra_binding_detail::makeAttributeHandle(value));
}

template <>
std::vector<rti1516e::Octet> seedHandle<rti1516e::ParameterHandle>(std::uint64_t value) {
  return encodeHandle(rti1516e::umbra_binding_detail::makeParameterHandle(value));
}

template <>
std::vector<rti1516e::Octet> seedHandle<rti1516e::DimensionHandle>(std::uint64_t value) {
  return encodeHandle(rti1516e::umbra_binding_detail::makeDimensionHandle(value));
}

template <>
std::vector<rti1516e::Octet> seedHandle<rti1516e::MessageRetractionHandle>(std::uint64_t value) {
  return encodeHandle(rti1516e::umbra_binding_detail::makeMessageRetractionHandle(value));
}

template <>
std::vector<rti1516e::Octet> seedHandle<rti1516e::RegionHandle>(std::uint64_t value) {
  return encodeHandle(rti1516e::umbra_binding_detail::makeRegionHandle(value));
}

template <typename Handle>
std::vector<rti1516e::Octet> roundTripHandle(
    std::vector<rti1516e::Octet> const& input,
    Handle (*decoder)(rti1516e::VariableLengthData const&)) {
  return encodeHandle(decoder(variableData(input)));
}

template <typename Handle>
std::vector<std::vector<rti1516e::Octet>> roundTripHandleCollection(
    JNIEnv* environment, jobjectArray values,
    Handle (*decoder)(rti1516e::VariableLengthData const&)) {
  if (values == nullptr) {
    throwInternal(environment, "The 2010 JNI handle collection is null.");
    return {};
  }
  jsize const length = environment->GetArrayLength(values);
  std::vector<std::vector<rti1516e::Octet>> result;
  result.reserve(static_cast<std::size_t>(length));
  for (jsize index = 0; index < length; ++index) {
    auto value = static_cast<jbyteArray>(environment->GetObjectArrayElement(values, index));
    result.push_back(roundTripHandle(readBytes(environment, value), decoder));
    if (value != nullptr) {
      environment->DeleteLocalRef(value);
    }
  }
  return result;
}

template <typename Time>
std::vector<rti1516e::Octet> roundTripTime(
    std::vector<rti1516e::Octet> const& input) {
  Time time;
  time.decode(variableData(input));
  return bytesFrom(time.encode());
}

template <typename Time>
std::vector<rti1516e::Octet> seedTime(Time const& time) {
  return bytesFrom(time.encode());
}

}  // namespace

extern "C" JNIEXPORT jlong JNICALL
Java_org_umbra_jni_rti1516e_NativeBridge_nativeCreate(
    JNIEnv* environment, jclass) {
  try {
    std::auto_ptr<NativeContext> context(new NativeContext());
    rti1516e::RTIambassadorFactory factory;
    context->ambassador =
        factory.createRTIambassador();
    return reinterpret_cast<jlong>(context.release());
  } catch (rti1516e::Exception const& error) {
    throwRtiException(environment, error);
  } catch (...) {
    throwInternal(environment, kNullBindingMessage);
  }
  return 0;
}

extern "C" JNIEXPORT void JNICALL
Java_org_umbra_jni_rti1516e_NativeBridge_nativeDestroy(
    JNIEnv*, jclass, jlong handle) {
  delete reinterpret_cast<NativeContext*>(handle);
}

extern "C" JNIEXPORT void JNICALL
Java_org_umbra_jni_rti1516e_NativeBridge_nativeConnect(
    JNIEnv* environment, jclass, jlong handle, jstring callbackModel,
    jstring localSettingsDesignator) {
  auto* context = reinterpret_cast<NativeContext*>(handle);
  if (context == nullptr || context->ambassador.get() == nullptr) {
    throwInternal(environment, "Umbra IEEE 1516.1-2010 JNI ambassador is closed.");
    return;
  }
  std::string const modelName = narrow(environment, callbackModel);
  rti1516e::CallbackModel const model =
      modelName == "HLA_EVOKED" ? rti1516e::HLA_EVOKED : rti1516e::HLA_IMMEDIATE;
  try {
    context->ambassador->connect(
        context->federateAmbassador, model,
        widen(environment, localSettingsDesignator));
  } catch (rti1516e::Exception const& error) {
    throwRtiException(environment, error);
  } catch (...) {
    throwInternal(environment, kNullBindingMessage);
  }
}

extern "C" JNIEXPORT void JNICALL
Java_org_umbra_jni_rti1516e_NativeBridge_nativeDisconnect(
    JNIEnv* environment, jclass, jlong handle) {
  auto* context = reinterpret_cast<NativeContext*>(handle);
  if (context == nullptr || context->ambassador.get() == nullptr) {
    throwInternal(environment, "Umbra IEEE 1516.1-2010 JNI ambassador is closed.");
    return;
  }
  try {
    context->ambassador->disconnect();
  } catch (rti1516e::Exception const& error) {
    throwRtiException(environment, error);
  } catch (...) {
    throwInternal(environment, kNullBindingMessage);
  }
}

extern "C" JNIEXPORT void JNICALL
Java_org_umbra_jni_rti1516e_NativeBridge_nativeUnavailable(
    JNIEnv* environment, jclass, jlong) {
  throwNativeInternalError(environment);
}

extern "C" JNIEXPORT void JNICALL
Java_org_umbra_jni_rti1516e_NativeBridge_nativeThrowStandardException(
    JNIEnv* environment, jclass, jstring exceptionName, jstring message) {
  std::string const name = narrow(environment, exceptionName);
  if (name == "RTIexception") {
    throwJavaException(environment, kBaseExceptionClass,
        widen(environment, message));
    return;
  }
  std::wstring const detail = widen(environment, message);
  try {
    // The C++ 1516.1 API uses EncoderException for malformed decode input;
    // the Java 1516e surface distinguishes the direction, so the probe uses
    // the corresponding Java DecoderException class explicitly.
    if (name == "DecoderException") {
      throw rti1516e::EncoderException(detail);
    }
    umbra::rti1516e_2010::throwNamedException(name, detail);
  } catch (rti1516e::EncoderException const& error) {
    throwRtiException(
        environment, error, name == "DecoderException" ? "DecoderException" : nullptr);
  } catch (rti1516e::Exception const& error) {
    throwRtiException(environment, error);
  } catch (std::exception const& error) {
    throwInternal(environment, error.what());
  } catch (...) {
    throwInternal(environment, kNullBindingMessage);
  }
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_org_umbra_jni_rti1516e_NativeBridge_nativeRoundTripBytes(
    JNIEnv* environment, jclass, jbyteArray value) {
  return writeBytes(environment, readBytes(environment, value));
}

extern "C" JNIEXPORT jstring JNICALL
Java_org_umbra_jni_rti1516e_NativeBridge_nativeRoundTripString(
    JNIEnv* environment, jclass, jstring value) {
  if (value == nullptr) {
    return nullptr;
  }
  jsize const length = environment->GetStringLength(value);
  jchar const* characters = environment->GetStringChars(value, nullptr);
  if (characters == nullptr) {
    return nullptr;
  }
  jstring result = environment->NewString(characters, length);
  environment->ReleaseStringChars(value, characters);
  return result;
}

extern "C" JNIEXPORT jbyte JNICALL
Java_org_umbra_jni_rti1516e_NativeBridge_nativeRoundTripByte(
    JNIEnv*, jclass, jbyte value) {
  return value;
}

extern "C" JNIEXPORT jshort JNICALL
Java_org_umbra_jni_rti1516e_NativeBridge_nativeRoundTripShort(
    JNIEnv*, jclass, jshort value) {
  return value;
}

extern "C" JNIEXPORT jint JNICALL
Java_org_umbra_jni_rti1516e_NativeBridge_nativeRoundTripInt(
    JNIEnv*, jclass, jint value) {
  return value;
}

extern "C" JNIEXPORT jlong JNICALL
Java_org_umbra_jni_rti1516e_NativeBridge_nativeRoundTripLong(
    JNIEnv*, jclass, jlong value) {
  return value;
}

extern "C" JNIEXPORT jfloat JNICALL
Java_org_umbra_jni_rti1516e_NativeBridge_nativeRoundTripFloat(
    JNIEnv*, jclass, jfloat value) {
  return value;
}

extern "C" JNIEXPORT jdouble JNICALL
Java_org_umbra_jni_rti1516e_NativeBridge_nativeRoundTripDouble(
    JNIEnv*, jclass, jdouble value) {
  return value;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_org_umbra_jni_rti1516e_NativeBridge_nativeRoundTripBoolean(
    JNIEnv*, jclass, jboolean value) {
  return value;
}

extern "C" JNIEXPORT jobject JNICALL
Java_org_umbra_jni_rti1516e_NativeBridge_nativeRoundTripObject(
    JNIEnv*, jclass, jobject value) {
  // Java-only carriers (enums, callback records, and exception objects) have
  // no corresponding C++ standard value. Returning the same local reference
  // still exercises the JNI object-reference boundary without inventing a
  // vendor serialization format for them.
  return value;
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_org_umbra_jni_rti1516e_NativeBridge_nativeSeedDataElement(
    JNIEnv* environment, jclass, jstring kind) {
  try {
    std::string const name = narrow(environment, kind);
    std::vector<rti1516e::Octet> value;
    if (name == "HLAASCIIchar") value = seedBasicElement(rti1516e::HLAASCIIchar('A'));
    else if (name == "HLAASCIIstring") value = seedBasicElement(rti1516e::HLAASCIIstring("ASCII"));
    else if (name == "HLAboolean") value = seedBasicElement(rti1516e::HLAboolean(true));
    else if (name == "HLAbyte") value = seedBasicElement(rti1516e::HLAbyte(0x7FU));
    else if (name == "HLAfloat32BE") value = seedBasicElement(rti1516e::HLAfloat32BE(1.25F));
    else if (name == "HLAfloat32LE") value = seedBasicElement(rti1516e::HLAfloat32LE(-2.5F));
    else if (name == "HLAfloat64BE") value = seedBasicElement(rti1516e::HLAfloat64BE(1.25));
    else if (name == "HLAfloat64LE") value = seedBasicElement(rti1516e::HLAfloat64LE(-2.5));
    else if (name == "HLAinteger16BE") value = seedBasicElement(rti1516e::HLAinteger16BE(static_cast<rti1516e::Integer16>(-1234)));
    else if (name == "HLAinteger16LE") value = seedBasicElement(rti1516e::HLAinteger16LE(static_cast<rti1516e::Integer16>(-1234)));
    else if (name == "HLAinteger32BE") value = seedBasicElement(rti1516e::HLAinteger32BE(static_cast<rti1516e::Integer32>(-123456)));
    else if (name == "HLAinteger32LE") value = seedBasicElement(rti1516e::HLAinteger32LE(static_cast<rti1516e::Integer32>(-123456)));
    else if (name == "HLAinteger64BE") value = seedBasicElement(rti1516e::HLAinteger64BE(static_cast<rti1516e::Integer64>(-1234567890123LL)));
    else if (name == "HLAinteger64LE") value = seedBasicElement(rti1516e::HLAinteger64LE(static_cast<rti1516e::Integer64>(-1234567890123LL)));
    else if (name == "HLAoctet") value = seedBasicElement(rti1516e::HLAoctet(0xFEU));
    else if (name == "HLAoctetPairBE") value = seedBasicElement(rti1516e::HLAoctetPairBE(rti1516e::OctetPair(0x12U, 0x34U)));
    else if (name == "HLAoctetPairLE") value = seedBasicElement(rti1516e::HLAoctetPairLE(rti1516e::OctetPair(0x12U, 0x34U)));
    else if (name == "HLAunicodeChar") value = seedBasicElement(rti1516e::HLAunicodeChar(static_cast<wchar_t>(0x03A9)));
    else if (name == "HLAunicodeString") value = seedBasicElement(rti1516e::HLAunicodeString(L"Unicode-\u03A9"));
    else value = seedCompositeElement(name);
    if (value.empty()) {
      throwInternal(environment, "Unknown IEEE 1516.1-2010 data-element kind.");
      return nullptr;
    }
    return writeBytes(environment, value);
  } catch (rti1516e::Exception const& error) {
    throwRtiException(environment, error);
  } catch (std::exception const& error) {
    throwInternal(environment, error.what());
  } catch (...) {
    throwInternal(environment, kNullBindingMessage);
  }
  return nullptr;
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_org_umbra_jni_rti1516e_NativeBridge_nativeRoundTripDataElement(
    JNIEnv* environment, jclass, jstring kind, jbyteArray input) {
  try {
    std::string const name = narrow(environment, kind);
    std::vector<rti1516e::Octet> const value = readBytes(environment, input);
    std::vector<rti1516e::Octet> result;
    if (name == "HLAASCIIchar") result = roundTripBasicElement<rti1516e::HLAASCIIchar>(value);
    else if (name == "HLAASCIIstring") result = roundTripBasicElement<rti1516e::HLAASCIIstring>(value);
    else if (name == "HLAboolean") result = roundTripBasicElement<rti1516e::HLAboolean>(value);
    else if (name == "HLAbyte") result = roundTripBasicElement<rti1516e::HLAbyte>(value);
    else if (name == "HLAfloat32BE") result = roundTripBasicElement<rti1516e::HLAfloat32BE>(value);
    else if (name == "HLAfloat32LE") result = roundTripBasicElement<rti1516e::HLAfloat32LE>(value);
    else if (name == "HLAfloat64BE") result = roundTripBasicElement<rti1516e::HLAfloat64BE>(value);
    else if (name == "HLAfloat64LE") result = roundTripBasicElement<rti1516e::HLAfloat64LE>(value);
    else if (name == "HLAinteger16BE") result = roundTripBasicElement<rti1516e::HLAinteger16BE>(value);
    else if (name == "HLAinteger16LE") result = roundTripBasicElement<rti1516e::HLAinteger16LE>(value);
    else if (name == "HLAinteger32BE") result = roundTripBasicElement<rti1516e::HLAinteger32BE>(value);
    else if (name == "HLAinteger32LE") result = roundTripBasicElement<rti1516e::HLAinteger32LE>(value);
    else if (name == "HLAinteger64BE") result = roundTripBasicElement<rti1516e::HLAinteger64BE>(value);
    else if (name == "HLAinteger64LE") result = roundTripBasicElement<rti1516e::HLAinteger64LE>(value);
    else if (name == "HLAoctet") result = roundTripBasicElement<rti1516e::HLAoctet>(value);
    else if (name == "HLAoctetPairBE") result = roundTripBasicElement<rti1516e::HLAoctetPairBE>(value);
    else if (name == "HLAoctetPairLE") result = roundTripBasicElement<rti1516e::HLAoctetPairLE>(value);
    else if (name == "HLAunicodeChar") result = roundTripBasicElement<rti1516e::HLAunicodeChar>(value);
    else if (name == "HLAunicodeString") result = roundTripBasicElement<rti1516e::HLAunicodeString>(value);
    else result = roundTripCompositeElement(name, value);
    if (result.empty()) {
      throwInternal(environment, "Unknown IEEE 1516.1-2010 data-element kind.");
      return nullptr;
    }
    return writeBytes(environment, result);
  } catch (rti1516e::EncoderException const& error) {
    throwRtiException(environment, error, "DecoderException");
  } catch (rti1516e::Exception const& error) {
    // The C++ 1516.1 carrier reports malformed decode input as
    // ``CouldNotDecode``.  The standard Java encoding surface exposes the
    // direction-specific ``DecoderException`` for DataElement.decode, so
    // preserve that Java contract instead of leaking an RTI service error.
    char const* name = umbra::rti1516e_2010::exceptionName(error);
    throwRtiException(
        environment, error,
        std::string(name) == "CouldNotDecode" ? "DecoderException" : nullptr);
  } catch (std::exception const& error) {
    throwInternal(environment, error.what());
  } catch (...) {
    throwInternal(environment, kNullBindingMessage);
  }
  return nullptr;
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_org_umbra_jni_rti1516e_NativeBridge_nativeSeedHandle(
    JNIEnv* environment, jclass, jstring kind, jlong value) {
  try {
    std::string const name = narrow(environment, kind);
    std::vector<rti1516e::Octet> result;
    if (name == "FederateHandle") result = seedHandle<rti1516e::FederateHandle>(static_cast<std::uint64_t>(value));
    else if (name == "ObjectClassHandle") result = seedHandle<rti1516e::ObjectClassHandle>(static_cast<std::uint64_t>(value));
    else if (name == "InteractionClassHandle") result = seedHandle<rti1516e::InteractionClassHandle>(static_cast<std::uint64_t>(value));
    else if (name == "ObjectInstanceHandle") result = seedHandle<rti1516e::ObjectInstanceHandle>(static_cast<std::uint64_t>(value));
    else if (name == "AttributeHandle") result = seedHandle<rti1516e::AttributeHandle>(static_cast<std::uint64_t>(value));
    else if (name == "ParameterHandle") result = seedHandle<rti1516e::ParameterHandle>(static_cast<std::uint64_t>(value));
    else if (name == "DimensionHandle") result = seedHandle<rti1516e::DimensionHandle>(static_cast<std::uint64_t>(value));
    else if (name == "MessageRetractionHandle") result = seedHandle<rti1516e::MessageRetractionHandle>(static_cast<std::uint64_t>(value));
    else if (name == "RegionHandle") result = seedHandle<rti1516e::RegionHandle>(static_cast<std::uint64_t>(value));
    else {
      throwInternal(environment, "Unknown IEEE 1516.1-2010 handle kind.");
      return nullptr;
    }
    return writeBytes(environment, result);
  } catch (rti1516e::Exception const& error) {
    throwRtiException(environment, error);
  } catch (std::exception const& error) {
    throwInternal(environment, error.what());
  } catch (...) {
    throwInternal(environment, kNullBindingMessage);
  }
  return nullptr;
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_org_umbra_jni_rti1516e_NativeBridge_nativeRoundTripHandle(
    JNIEnv* environment, jclass, jstring kind, jbyteArray input) {
  try {
    std::string const name = narrow(environment, kind);
    std::vector<rti1516e::Octet> const value = readBytes(environment, input);
    std::vector<rti1516e::Octet> result;
    if (name == "FederateHandle") result = roundTripHandle(value, rti1516e::umbra_binding_detail::decodeFederateHandle);
    else if (name == "ObjectClassHandle") result = roundTripHandle(value, rti1516e::umbra_binding_detail::decodeObjectClassHandle);
    else if (name == "InteractionClassHandle") result = roundTripHandle(value, rti1516e::umbra_binding_detail::decodeInteractionClassHandle);
    else if (name == "ObjectInstanceHandle") result = roundTripHandle(value, rti1516e::umbra_binding_detail::decodeObjectInstanceHandle);
    else if (name == "AttributeHandle") result = roundTripHandle(value, rti1516e::umbra_binding_detail::decodeAttributeHandle);
    else if (name == "ParameterHandle") result = roundTripHandle(value, rti1516e::umbra_binding_detail::decodeParameterHandle);
    else if (name == "DimensionHandle") result = roundTripHandle(value, rti1516e::umbra_binding_detail::decodeDimensionHandle);
    else if (name == "MessageRetractionHandle") result = roundTripHandle(value, rti1516e::umbra_binding_detail::decodeMessageRetractionHandle);
    else if (name == "RegionHandle") result = roundTripHandle(value, rti1516e::umbra_binding_detail::decodeRegionHandle);
    else {
      throwInternal(environment, "Unknown IEEE 1516.1-2010 handle kind.");
      return nullptr;
    }
    return writeBytes(environment, result);
  } catch (rti1516e::Exception const& error) {
    throwRtiException(environment, error);
  } catch (std::exception const& error) {
    throwInternal(environment, error.what());
  } catch (...) {
    throwInternal(environment, kNullBindingMessage);
  }
  return nullptr;
}

extern "C" JNIEXPORT jobjectArray JNICALL
Java_org_umbra_jni_rti1516e_NativeBridge_nativeRoundTripHandleCollection(
    JNIEnv* environment, jclass, jstring kind, jobjectArray values) {
  try {
    std::string const name = narrow(environment, kind);
    std::vector<std::vector<rti1516e::Octet>> result;
    if (name == "FederateHandle") result = roundTripHandleCollection(environment, values, rti1516e::umbra_binding_detail::decodeFederateHandle);
    else if (name == "ObjectClassHandle") result = roundTripHandleCollection(environment, values, rti1516e::umbra_binding_detail::decodeObjectClassHandle);
    else if (name == "InteractionClassHandle") result = roundTripHandleCollection(environment, values, rti1516e::umbra_binding_detail::decodeInteractionClassHandle);
    else if (name == "ObjectInstanceHandle") result = roundTripHandleCollection(environment, values, rti1516e::umbra_binding_detail::decodeObjectInstanceHandle);
    else if (name == "AttributeHandle") result = roundTripHandleCollection(environment, values, rti1516e::umbra_binding_detail::decodeAttributeHandle);
    else if (name == "ParameterHandle") result = roundTripHandleCollection(environment, values, rti1516e::umbra_binding_detail::decodeParameterHandle);
    else if (name == "DimensionHandle") result = roundTripHandleCollection(environment, values, rti1516e::umbra_binding_detail::decodeDimensionHandle);
    else if (name == "MessageRetractionHandle") result = roundTripHandleCollection(environment, values, rti1516e::umbra_binding_detail::decodeMessageRetractionHandle);
    else if (name == "RegionHandle") result = roundTripHandleCollection(environment, values, rti1516e::umbra_binding_detail::decodeRegionHandle);
    else {
      throwInternal(environment, "Unknown IEEE 1516.1-2010 handle kind.");
      return nullptr;
    }
    jclass byteArrayClass = environment->FindClass("[B");
    if (byteArrayClass == nullptr) return nullptr;
    jobjectArray output = environment->NewObjectArray(
        static_cast<jsize>(result.size()), byteArrayClass, nullptr);
    for (std::size_t index = 0; index < result.size(); ++index) {
      jbyteArray encoded = writeBytes(environment, result[index]);
      environment->SetObjectArrayElement(output, static_cast<jsize>(index), encoded);
      environment->DeleteLocalRef(encoded);
    }
    environment->DeleteLocalRef(byteArrayClass);
    return output;
  } catch (rti1516e::Exception const& error) {
    throwRtiException(environment, error);
  } catch (std::exception const& error) {
    throwInternal(environment, error.what());
  } catch (...) {
    throwInternal(environment, kNullBindingMessage);
  }
  return nullptr;
}

extern "C" JNIEXPORT jobjectArray JNICALL
Java_org_umbra_jni_rti1516e_NativeBridge_nativeRoundTripByteMatrix(
    JNIEnv* environment, jclass, jobjectArray values) {
  try {
    if (values == nullptr) {
      throwInternal(environment, "The 2010 JNI type probe received a null byte matrix.");
      return nullptr;
    }
    jsize const length = environment->GetArrayLength(values);
    std::vector<std::vector<rti1516e::Octet>> result;
    result.reserve(static_cast<std::size_t>(length));
    for (jsize index = 0; index < length; ++index) {
      jobject row = environment->GetObjectArrayElement(values, index);
      result.push_back(readBytes(environment, static_cast<jbyteArray>(row)));
      if (row != nullptr) environment->DeleteLocalRef(row);
      if (environment->ExceptionCheck()) return nullptr;
    }
    jclass byteArrayClass = environment->FindClass("[B");
    if (byteArrayClass == nullptr) return nullptr;
    jobjectArray output = environment->NewObjectArray(length, byteArrayClass, nullptr);
    if (output == nullptr) {
      environment->DeleteLocalRef(byteArrayClass);
      return nullptr;
    }
    for (jsize index = 0; index < length; ++index) {
      jbyteArray encoded = writeBytes(
          environment, result[static_cast<std::size_t>(index)]);
      environment->SetObjectArrayElement(output, index, encoded);
      environment->DeleteLocalRef(encoded);
      if (environment->ExceptionCheck()) {
        environment->DeleteLocalRef(byteArrayClass);
        return nullptr;
      }
    }
    environment->DeleteLocalRef(byteArrayClass);
    return output;
  } catch (rti1516e::Exception const& error) {
    throwRtiException(environment, error);
  } catch (std::exception const& error) {
    throwInternal(environment, error.what());
  } catch (...) {
    throwInternal(environment, kNullBindingMessage);
  }
  return nullptr;
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_org_umbra_jni_rti1516e_NativeBridge_nativeSeedLogicalTime(
    JNIEnv* environment, jclass, jstring kind) {
  try {
    std::string const name = narrow(environment, kind);
    std::vector<rti1516e::Octet> result;
    if (name == "HLAinteger64Time") result = seedTime(rti1516e::HLAinteger64Time(static_cast<rti1516e::Integer64>(123456789)));
    else if (name == "HLAinteger64Interval") result = seedTime(rti1516e::HLAinteger64Interval(static_cast<rti1516e::Integer64>(1234)));
    else if (name == "HLAfloat64Time") result = seedTime(rti1516e::HLAfloat64Time(12.5));
    else if (name == "HLAfloat64Interval") result = seedTime(rti1516e::HLAfloat64Interval(0.25));
    else {
      throwInternal(environment, "Unknown IEEE 1516.1-2010 logical-time kind.");
      return nullptr;
    }
    return writeBytes(environment, result);
  } catch (rti1516e::Exception const& error) {
    throwRtiException(environment, error);
  } catch (std::exception const& error) {
    throwInternal(environment, error.what());
  } catch (...) {
    throwInternal(environment, kNullBindingMessage);
  }
  return nullptr;
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_org_umbra_jni_rti1516e_NativeBridge_nativeRoundTripLogicalTime(
    JNIEnv* environment, jclass, jstring kind, jbyteArray input) {
  try {
    std::string const name = narrow(environment, kind);
    std::vector<rti1516e::Octet> const value = readBytes(environment, input);
    std::vector<rti1516e::Octet> result;
    if (name == "HLAinteger64Time") result = roundTripTime<rti1516e::HLAinteger64Time>(value);
    else if (name == "HLAinteger64Interval") result = roundTripTime<rti1516e::HLAinteger64Interval>(value);
    else if (name == "HLAfloat64Time") result = roundTripTime<rti1516e::HLAfloat64Time>(value);
    else if (name == "HLAfloat64Interval") result = roundTripTime<rti1516e::HLAfloat64Interval>(value);
    else {
      throwInternal(environment, "Unknown IEEE 1516.1-2010 logical-time kind.");
      return nullptr;
    }
    return writeBytes(environment, result);
  } catch (rti1516e::Exception const& error) {
    throwRtiException(environment, error);
  } catch (std::exception const& error) {
    throwInternal(environment, error.what());
  } catch (...) {
    throwInternal(environment, kNullBindingMessage);
  }
  return nullptr;
}
