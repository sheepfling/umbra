#include <jni.h>

#include "internal/runtime/umbra_rti_ambassador.hpp"
#include "internal/runtime/reference_authorizer.hpp"
#include "internal/handles/attribute_handle.hpp"
#include "internal/handles/dimension_handle.hpp"
#include "internal/handles/federate_handle.hpp"
#include "internal/handles/interaction_class_handle.hpp"
#include "internal/handles/object_class_handle.hpp"
#include "internal/handles/object_instance_handle.hpp"
#include "internal/handles/parameter_handle.hpp"
#include "internal/handles/region_handle.hpp"
#include "internal/handles/transportation_type_handle.hpp"

#include <RTI/Exception.h>
#include <RTI/NullFederateAmbassador.h>
#include <RTI/RtiConfiguration.h>
#include <RTI/auth/HLAnoCredentials.h>
#include <RTI/auth/AuthorizationResult.h>
#include <RTI/auth/Authorizer.h>
#include <RTI/auth/Credentials.h>
#include <RTI/encoding/BasicDataElements.h>
#include <RTI/encoding/HLAopaqueData.h>
#include <RTI/encoding/HLAfixedArray.h>
#include <RTI/encoding/HLAfixedRecord.h>
#include <RTI/encoding/HLAextendableVariantRecord.h>
#include <RTI/encoding/HLAvariantRecord.h>
#include <RTI/encoding/HLAvariableArray.h>
#include <RTI/time/LogicalTime.h>
#include <RTI/time/LogicalTimeFactory.h>
#include <RTI/time/HLAinteger64TimeFactory.h>
#include <RTI/time/HLAinteger64Time.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAfloat64TimeFactory.h>
#include <RTI/time/HLAfloat64Time.h>
#include <RTI/time/HLAfloat64Interval.h>

#include <climits>
#include <algorithm>
#include <cstdint>
#include <mutex>
#include <limits>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace rti = rti1516_2025;
namespace binding = rti1516_2025::umbra_binding_detail;

namespace {

std::string narrowAscii(std::wstring const& value) {
  std::string result;
  result.reserve(value.size());
  for (wchar_t character : value) {
    result.push_back(character >= 0 && character <= 0x7f ? static_cast<char>(character) : '?');
  }
  return result;
}

std::string asciiString(JNIEnv* environment, jstring value) {
  if (value == nullptr) return {};
  jsize const length = environment->GetStringLength(value);
  jchar const* characters = environment->GetStringChars(value, nullptr);
  if (characters == nullptr) {
    throw std::runtime_error("Could not read a Java ASCII string");
  }
  std::string result;
  result.reserve(static_cast<std::size_t>(length));
  for (jsize index = 0; index < length; ++index) {
    // Preserve every representable byte so the C++ HLAASCIIstring encoder,
    // rather than the bridge, rejects non-ASCII input at encode time.
    result.push_back(characters[index] <= 0xffU
        ? static_cast<char>(characters[index])
        : static_cast<char>(0x80));
  }
  environment->ReleaseStringChars(value, characters);
  return result;
}

std::wstring wideString(JNIEnv* environment, jstring value) {
  if (value == nullptr) {
    return {};
  }

  jsize const length = environment->GetStringLength(value);
  jchar const* characters = environment->GetStringChars(value, nullptr);
  if (characters == nullptr) {
    throw std::runtime_error("Could not read a Java string");
  }

  std::wstring result;
  result.reserve(static_cast<std::size_t>(length));
#if WCHAR_MAX <= 0xffff
  for (jsize index = 0; index < length; ++index) {
    result.push_back(static_cast<wchar_t>(characters[index]));
  }
#else
  for (jsize index = 0; index < length; ++index) {
    std::uint32_t codePoint = characters[index];
    if (codePoint >= 0xd800 && codePoint <= 0xdbff && index + 1 < length) {
      std::uint32_t const lowSurrogate = characters[index + 1];
      if (lowSurrogate >= 0xdc00 && lowSurrogate <= 0xdfff) {
        codePoint = 0x10000 + ((codePoint - 0xd800) << 10) + (lowSurrogate - 0xdc00);
        ++index;
      }
    }
    result.push_back(static_cast<wchar_t>(codePoint));
  }
#endif
  environment->ReleaseStringChars(value, characters);
  return result;
}

std::vector<std::wstring> wideStringVector(JNIEnv* environment, jobjectArray values) {
  if (values == nullptr) {
    throw std::runtime_error("A federation execution requires at least one FOM module");
  }
  jsize const length = environment->GetArrayLength(values);
  std::vector<std::wstring> result;
  result.reserve(static_cast<std::size_t>(length));
  for (jsize index = 0; index < length; ++index) {
    auto value = static_cast<jstring>(environment->GetObjectArrayElement(values, index));
    if (value == nullptr) {
      throw std::runtime_error("A federation execution FOM module must not be null");
    }
    result.push_back(wideString(environment, value));
    environment->DeleteLocalRef(value);
  }
  return result;
}

jstring javaString(JNIEnv* environment, std::wstring const& value) {
  std::vector<jchar> characters;
  characters.reserve(value.size());
#if WCHAR_MAX <= 0xffff
  for (wchar_t character : value) {
    characters.push_back(static_cast<jchar>(character));
  }
#else
  for (wchar_t character : value) {
    std::uint32_t const codePoint = static_cast<std::uint32_t>(character);
    if (codePoint <= 0xffff) {
      characters.push_back(static_cast<jchar>(codePoint));
    } else {
      std::uint32_t const offset = codePoint - 0x10000;
      characters.push_back(static_cast<jchar>(0xd800 + (offset >> 10)));
      characters.push_back(static_cast<jchar>(0xdc00 + (offset & 0x3ff)));
    }
  }
#endif
  return environment->NewString(
      characters.empty() ? nullptr : characters.data(), static_cast<jsize>(characters.size()));
}

jstring javaAsciiString(JNIEnv* environment, std::string const& value) {
  std::wstring wide;
  wide.reserve(value.size());
  for (char character : value) {
    wide.push_back(static_cast<wchar_t>(static_cast<unsigned char>(character)));
  }
  return javaString(environment, wide);
}

void clearJavaException(JNIEnv* environment) {
  if (environment->ExceptionCheck()) {
    environment->ExceptionClear();
  }
}

void throwJavaException(
    JNIEnv* environment, char const* className, std::string const& message) {
  clearJavaException(environment);
  jclass exceptionClass = environment->FindClass(className);
  if (exceptionClass == nullptr) {
    clearJavaException(environment);
    exceptionClass = environment->FindClass("hla/rti1516_2025/exceptions/RTIinternalError");
  }
  if (exceptionClass != nullptr) {
    environment->ThrowNew(exceptionClass, message.c_str());
    environment->DeleteLocalRef(exceptionClass);
  }
}

void throwJavaRtiException(JNIEnv* environment, rti::Exception const& exception) {
  auto const exceptionName = narrowAscii(exception.name());
  if (exceptionName == "EncoderException") {
    throwJavaException(
        environment,
        "hla/rti1516_2025/encoding/EncoderException",
        narrowAscii(exception.what()));
    return;
  }
  auto const className = "hla/rti1516_2025/exceptions/" + exceptionName;
  throwJavaException(environment, className.c_str(), narrowAscii(exception.what()));
}

void throwJavaDecoderException(JNIEnv* environment, rti::Exception const& exception) {
  if (narrowAscii(exception.name()) == "EncoderException") {
    throwJavaException(
        environment,
        "hla/rti1516_2025/encoding/DecoderException",
        narrowAscii(exception.what()));
    return;
  }
  throwJavaRtiException(environment, exception);
}

jbyteArray javaByteArray(JNIEnv* environment, rti::VariableLengthData const& value) {
  if (value.size() > static_cast<std::size_t>(std::numeric_limits<jsize>::max())) {
    throw std::runtime_error("The encoded RTI value is too large for a Java byte array");
  }
  auto result = environment->NewByteArray(static_cast<jsize>(value.size()));
  if (result == nullptr) {
    clearJavaException(environment);
    throw std::runtime_error("Could not create a Java byte array for an RTI value");
  }
  if (value.size() != 0) {
    environment->SetByteArrayRegion(
        result,
        0,
        static_cast<jsize>(value.size()),
        static_cast<jbyte const*>(value.data()));
    if (environment->ExceptionCheck()) {
      environment->DeleteLocalRef(result);
      clearJavaException(environment);
      throw std::runtime_error("Could not copy an encoded RTI value into Java");
    }
  }
  return result;
}

template <typename HandleSet>
jobjectArray javaEncodedHandleSet(JNIEnv* environment, HandleSet const& values) {
  if (values.size() > static_cast<std::size_t>(std::numeric_limits<jsize>::max())) {
    throw std::runtime_error("The RTI handle set is too large for a Java array");
  }
  jclass byteArrayClass = environment->FindClass("[B");
  if (byteArrayClass == nullptr) {
    clearJavaException(environment);
    throw std::runtime_error("Could not resolve the Java byte-array class");
  }
  jobjectArray result = environment->NewObjectArray(
      static_cast<jsize>(values.size()), byteArrayClass, nullptr);
  environment->DeleteLocalRef(byteArrayClass);
  if (result == nullptr) {
    clearJavaException(environment);
    throw std::runtime_error("Could not create a Java RTI handle array");
  }
  try {
    jsize index = 0;
    for (auto const& value : values) {
      jbyteArray encoded = javaByteArray(environment, value.encode());
      environment->SetObjectArrayElement(result, index++, encoded);
      environment->DeleteLocalRef(encoded);
      if (environment->ExceptionCheck()) {
        clearJavaException(environment);
        throw std::runtime_error("Could not add an RTI handle to a Java array");
      }
    }
    return result;
  } catch (...) {
    environment->DeleteLocalRef(result);
    throw;
  }
}

jlongArray javaRangeBounds(JNIEnv* environment, rti::RangeBounds const& bounds) {
  jlongArray result = environment->NewLongArray(2);
  if (result == nullptr) {
    clearJavaException(environment);
    throw std::runtime_error("Could not create a Java range-bounds array");
  }
  jlong values[] = {
      static_cast<jlong>(bounds.getLowerBound()),
      static_cast<jlong>(bounds.getUpperBound())};
  environment->SetLongArrayRegion(result, 0, 2, values);
  if (environment->ExceptionCheck()) {
    environment->DeleteLocalRef(result);
    clearJavaException(environment);
    throw std::runtime_error("Could not copy RTI range bounds into Java");
  }
  return result;
}

jbyteArray javaTimeQueryByteArray(
    JNIEnv* environment,
    bool valid,
    rti::VariableLengthData const& encodedTime) {
  if (encodedTime.size() >= static_cast<std::size_t>(std::numeric_limits<jsize>::max())) {
    throw std::runtime_error("The encoded RTI time-query value is too large for a Java byte array");
  }
  auto result = environment->NewByteArray(static_cast<jsize>(encodedTime.size() + 1));
  if (result == nullptr) {
    clearJavaException(environment);
    throw std::runtime_error("Could not create a Java byte array for an RTI time-query value");
  }
  jbyte const validity = valid ? 1 : 0;
  environment->SetByteArrayRegion(result, 0, 1, &validity);
  if (encodedTime.size() != 0) {
    environment->SetByteArrayRegion(
        result,
        1,
        static_cast<jsize>(encodedTime.size()),
        static_cast<jbyte const*>(encodedTime.data()));
  }
  if (environment->ExceptionCheck()) {
    environment->DeleteLocalRef(result);
    clearJavaException(environment);
    throw std::runtime_error("Could not copy an RTI time-query value into Java");
  }
  return result;
}

rti::VariableLengthData variableLengthData(JNIEnv* environment, jbyteArray value) {
  if (value == nullptr) {
    throw std::runtime_error("An encoded RTI value must not be null");
  }
  jsize const size = environment->GetArrayLength(value);
  jbyte* bytes = environment->GetByteArrayElements(value, nullptr);
  if (bytes == nullptr) {
    clearJavaException(environment);
    throw std::runtime_error("Could not read an encoded Java byte array");
  }
  rti::VariableLengthData result(bytes, static_cast<std::size_t>(size));
  environment->ReleaseByteArrayElements(value, bytes, JNI_ABORT);
  return result;
}

std::vector<rti::Octet> octetVector(JNIEnv* environment, jbyteArray value) {
  auto const data = variableLengthData(environment, value);
  auto const* bytes = static_cast<rti::Octet const*>(data.data());
  return bytes == nullptr
      ? std::vector<rti::Octet>{}
      : std::vector<rti::Octet>(bytes, bytes + data.size());
}

struct JavaDataElementType final {
  char const* javaInterface;
  char const* nativeName;
};

jmethodID requiredMethod(JNIEnv* environment, jclass owner, char const* name, char const* signature);
void ensureJavaCallSucceeded(JNIEnv* environment, char const* operation);

std::string nativeDataElementType(JNIEnv* environment, jobject value) {
  if (value == nullptr) throw std::runtime_error("A Java data element must not be null");
  static constexpr JavaDataElementType types[] = {
      {"hla/rti1516_2025/encoding/HLAinteger16BE", "HLAinteger16BE"},
      {"hla/rti1516_2025/encoding/HLAinteger16LE", "HLAinteger16LE"},
      {"hla/rti1516_2025/encoding/HLAinteger32BE", "HLAinteger32BE"},
      {"hla/rti1516_2025/encoding/HLAinteger32LE", "HLAinteger32LE"},
      {"hla/rti1516_2025/encoding/HLAinteger64BE", "HLAinteger64BE"},
      {"hla/rti1516_2025/encoding/HLAinteger64LE", "HLAinteger64LE"},
      {"hla/rti1516_2025/encoding/HLAunsignedInteger16BE", "HLAunsignedInteger16BE"},
      {"hla/rti1516_2025/encoding/HLAunsignedInteger16LE", "HLAunsignedInteger16LE"},
      {"hla/rti1516_2025/encoding/HLAunsignedInteger32BE", "HLAunsignedInteger32BE"},
      {"hla/rti1516_2025/encoding/HLAunsignedInteger32LE", "HLAunsignedInteger32LE"},
      {"hla/rti1516_2025/encoding/HLAunsignedInteger64BE", "HLAunsignedInteger64BE"},
      {"hla/rti1516_2025/encoding/HLAunsignedInteger64LE", "HLAunsignedInteger64LE"},
      {"hla/rti1516_2025/encoding/HLAfloat32BE", "HLAfloat32BE"},
      {"hla/rti1516_2025/encoding/HLAfloat32LE", "HLAfloat32LE"},
      {"hla/rti1516_2025/encoding/HLAfloat64BE", "HLAfloat64BE"},
      {"hla/rti1516_2025/encoding/HLAfloat64LE", "HLAfloat64LE"},
      {"hla/rti1516_2025/encoding/HLAbyte", "HLAbyte"},
      {"hla/rti1516_2025/encoding/HLAoctet", "HLAoctet"},
      {"hla/rti1516_2025/encoding/HLAASCIIchar", "HLAASCIIchar"},
      {"hla/rti1516_2025/encoding/HLAASCIIstring", "HLAASCIIstring"},
      {"hla/rti1516_2025/encoding/HLAunicodeChar", "HLAunicodeChar"},
      {"hla/rti1516_2025/encoding/HLAunicodeString", "HLAunicodeString"},
      {"hla/rti1516_2025/encoding/HLAoctetPairBE", "HLAoctetPairBE"},
      {"hla/rti1516_2025/encoding/HLAoctetPairLE", "HLAoctetPairLE"},
      {"hla/rti1516_2025/encoding/HLAopaqueData", "HLAopaqueData"},
      {"hla/rti1516_2025/encoding/HLAboolean", "HLAboolean"},
  };
  for (auto const& candidate : types) {
    jclass type = environment->FindClass(candidate.javaInterface);
    if (type == nullptr) {
      clearJavaException(environment);
      throw std::runtime_error(std::string("Could not load Java data-element interface ") +
                               candidate.javaInterface);
    }
    bool const matches = environment->IsInstanceOf(value, type) == JNI_TRUE;
    environment->DeleteLocalRef(type);
    if (matches) return candidate.nativeName;
  }
  throw std::runtime_error("The Java data element is not a C++-backed Umbra primitive type");
}

std::unique_ptr<rti::DataElement> nativeDataElementPrototype(std::string const& type) {
  if (type == "HLAinteger16BE") return std::make_unique<rti::HLAinteger16BE>();
  if (type == "HLAinteger16LE") return std::make_unique<rti::HLAinteger16LE>();
  if (type == "HLAinteger32BE") return std::make_unique<rti::HLAinteger32BE>();
  if (type == "HLAinteger32LE") return std::make_unique<rti::HLAinteger32LE>();
  if (type == "HLAinteger64BE") return std::make_unique<rti::HLAinteger64BE>();
  if (type == "HLAinteger64LE") return std::make_unique<rti::HLAinteger64LE>();
  if (type == "HLAunsignedInteger16BE") return std::make_unique<rti::HLAunsignedInteger16BE>();
  if (type == "HLAunsignedInteger16LE") return std::make_unique<rti::HLAunsignedInteger16LE>();
  if (type == "HLAunsignedInteger32BE") return std::make_unique<rti::HLAunsignedInteger32BE>();
  if (type == "HLAunsignedInteger32LE") return std::make_unique<rti::HLAunsignedInteger32LE>();
  if (type == "HLAunsignedInteger64BE") return std::make_unique<rti::HLAunsignedInteger64BE>();
  if (type == "HLAunsignedInteger64LE") return std::make_unique<rti::HLAunsignedInteger64LE>();
  if (type == "HLAfloat32BE") return std::make_unique<rti::HLAfloat32BE>();
  if (type == "HLAfloat32LE") return std::make_unique<rti::HLAfloat32LE>();
  if (type == "HLAfloat64BE") return std::make_unique<rti::HLAfloat64BE>();
  if (type == "HLAfloat64LE") return std::make_unique<rti::HLAfloat64LE>();
  if (type == "HLAbyte") return std::make_unique<rti::HLAbyte>();
  if (type == "HLAoctet") return std::make_unique<rti::HLAoctet>();
  if (type == "HLAASCIIchar") return std::make_unique<rti::HLAASCIIchar>();
  if (type == "HLAASCIIstring") return std::make_unique<rti::HLAASCIIstring>();
  if (type == "HLAunicodeChar") return std::make_unique<rti::HLAunicodeChar>();
  if (type == "HLAunicodeString") return std::make_unique<rti::HLAunicodeString>();
  if (type == "HLAoctetPairBE") return std::make_unique<rti::HLAoctetPairBE>();
  if (type == "HLAoctetPairLE") return std::make_unique<rti::HLAoctetPairLE>();
  if (type == "HLAopaqueData") return std::make_unique<rti::HLAopaqueData>();
  if (type == "HLAboolean") return std::make_unique<rti::HLAboolean>();
  throw std::runtime_error("The Java data-element type is not implemented by Umbra C++");
}

rti::VariableLengthData encodedJavaDataElement(JNIEnv* environment, jobject value) {
  jclass type = environment->GetObjectClass(value);
  if (type == nullptr) {
    clearJavaException(environment);
    throw std::runtime_error("Could not inspect a Java data element");
  }
  try {
    auto const encode = requiredMethod(environment, type, "toByteArray", "()[B");
    auto encoded = static_cast<jbyteArray>(environment->CallObjectMethod(value, encode));
    ensureJavaCallSucceeded(environment, "data-element toByteArray");
    if (encoded == nullptr) throw std::runtime_error("A Java data element returned null encoding");
    auto result = variableLengthData(environment, encoded);
    environment->DeleteLocalRef(encoded);
    environment->DeleteLocalRef(type);
    return result;
  } catch (...) {
    environment->DeleteLocalRef(type);
    throw;
  }
}

std::unique_ptr<rti::DataElement> nativeDataElementFromJava(
    JNIEnv* environment,
    jobject value,
    std::string const& expectedType) {
  auto const actualType = nativeDataElementType(environment, value);
  if (actualType != expectedType) {
    throw rti::EncoderException(L"The Java data element does not match the native C++ array prototype.");
  }
  auto result = nativeDataElementPrototype(actualType);
  result->decode(encodedJavaDataElement(environment, value));
  return result;
}

jbyteArray javaByteArray(JNIEnv* environment, std::vector<rti::Octet> const& value) {
  auto result = environment->NewByteArray(static_cast<jsize>(value.size()));
  if (result == nullptr) return nullptr;
  if (!value.empty()) {
    environment->SetByteArrayRegion(
        result,
        0,
        static_cast<jsize>(value.size()),
        reinterpret_cast<jbyte const*>(value.data()));
  }
  return result;
}

rti::FederateHandleSet federateHandleSet(JNIEnv* environment, jobjectArray values) {
  rti::FederateHandleSet result;
  if (values == nullptr) return result;
  jsize const length = environment->GetArrayLength(values);
  for (jsize index = 0; index < length; ++index) {
    auto value = static_cast<jbyteArray>(environment->GetObjectArrayElement(values, index));
    if (value == nullptr) {
      throw std::runtime_error("A synchronization-set federate handle must not be null");
    }
    result.insert(rti::umbra_binding_detail::decodeFederateHandle(
        variableLengthData(environment, value)));
    environment->DeleteLocalRef(value);
  }
  return result;
}

jmethodID requiredMethod(
    JNIEnv* environment, jclass owner, char const* name, char const* signature);

void ensureJavaCallSucceeded(JNIEnv* environment, char const* operation) {
  if (environment->ExceptionCheck()) {
    clearJavaException(environment);
    throw std::runtime_error(std::string("Java ") + operation + " failed while crossing the JNI bridge");
  }
}

rti::VariableLengthData encodedJavaHandle(JNIEnv* environment, jobject handle) {
  if (handle == nullptr) {
    throw std::runtime_error("A Java RTI handle must not be null");
  }
  jclass type = environment->GetObjectClass(handle);
  if (type == nullptr) {
    clearJavaException(environment);
    throw std::runtime_error("Could not inspect a Java RTI handle");
  }
  try {
    auto const encodedLength = requiredMethod(environment, type, "encodedLength", "()I");
    auto const encode = requiredMethod(environment, type, "encode", "([BI)V");
    jint const length = environment->CallIntMethod(handle, encodedLength);
    ensureJavaCallSucceeded(environment, "handle encodedLength");
    if (length < 0) {
      throw std::runtime_error("A Java RTI handle reported a negative encoded length");
    }
    jbyteArray encoded = environment->NewByteArray(length);
    if (encoded == nullptr) {
      clearJavaException(environment);
      throw std::runtime_error("Could not allocate a Java RTI handle encoding");
    }
    try {
      environment->CallVoidMethod(handle, encode, encoded, 0);
      ensureJavaCallSucceeded(environment, "handle encode");
      auto result = variableLengthData(environment, encoded);
      environment->DeleteLocalRef(encoded);
      environment->DeleteLocalRef(type);
      return result;
    } catch (...) {
      environment->DeleteLocalRef(encoded);
      throw;
    }
  } catch (...) {
    environment->DeleteLocalRef(type);
    throw;
  }
}

rti::ParameterHandleValueMap parameterHandleValueMap(JNIEnv* environment, jobject values) {
  if (values == nullptr) {
    throw std::runtime_error("A Java parameter-handle value map must not be null");
  }
  jclass mapClass = environment->GetObjectClass(values);
  if (mapClass == nullptr) {
    clearJavaException(environment);
    throw std::runtime_error("Could not inspect a Java parameter-handle value map");
  }
  jobject entries = nullptr;
  jobject iterator = nullptr;
  try {
    auto const entrySet = requiredMethod(environment, mapClass, "entrySet", "()Ljava/util/Set;");
    entries = environment->CallObjectMethod(values, entrySet);
    ensureJavaCallSucceeded(environment, "parameter map entrySet");
    if (entries == nullptr) throw std::runtime_error("A Java parameter map returned null entrySet");
    jclass setClass = environment->GetObjectClass(entries);
    if (setClass == nullptr) {
      clearJavaException(environment);
      throw std::runtime_error("Could not inspect Java parameter-map entries");
    }
    auto const iteratorMethod = requiredMethod(environment, setClass, "iterator", "()Ljava/util/Iterator;");
    iterator = environment->CallObjectMethod(entries, iteratorMethod);
    environment->DeleteLocalRef(setClass);
    ensureJavaCallSucceeded(environment, "parameter map iterator");
    if (iterator == nullptr) throw std::runtime_error("A Java parameter map returned null iterator");
    jclass iteratorClass = environment->GetObjectClass(iterator);
    if (iteratorClass == nullptr) {
      clearJavaException(environment);
      throw std::runtime_error("Could not inspect Java parameter-map iterator");
    }
    auto const hasNext = requiredMethod(environment, iteratorClass, "hasNext", "()Z");
    auto const next = requiredMethod(environment, iteratorClass, "next", "()Ljava/lang/Object;");
    environment->DeleteLocalRef(iteratorClass);
    rti::ParameterHandleValueMap result;
    while (environment->CallBooleanMethod(iterator, hasNext) == JNI_TRUE) {
      ensureJavaCallSucceeded(environment, "parameter map iterator hasNext");
      jobject entry = environment->CallObjectMethod(iterator, next);
      ensureJavaCallSucceeded(environment, "parameter map iterator next");
      if (entry == nullptr) throw std::runtime_error("A Java parameter map contained a null entry");
      try {
        jclass entryClass = environment->GetObjectClass(entry);
        if (entryClass == nullptr) {
          clearJavaException(environment);
          throw std::runtime_error("Could not inspect a Java parameter-map entry");
        }
        auto const getKey = requiredMethod(environment, entryClass, "getKey", "()Ljava/lang/Object;");
        auto const getValue = requiredMethod(environment, entryClass, "getValue", "()Ljava/lang/Object;");
        jobject key = environment->CallObjectMethod(entry, getKey);
        ensureJavaCallSucceeded(environment, "parameter map entry key");
        auto value = static_cast<jbyteArray>(environment->CallObjectMethod(entry, getValue));
        ensureJavaCallSucceeded(environment, "parameter map entry value");
        environment->DeleteLocalRef(entryClass);
        if (key == nullptr || value == nullptr) {
          if (key != nullptr) environment->DeleteLocalRef(key);
          if (value != nullptr) environment->DeleteLocalRef(value);
          throw std::runtime_error("A Java parameter map requires non-null handles and values");
        }
        try {
          auto handle = rti::umbra_binding_detail::decodeParameterHandle(
              encodedJavaHandle(environment, key));
          auto data = variableLengthData(environment, value);
          result.insert_or_assign(std::move(handle), std::move(data));
          environment->DeleteLocalRef(key);
          environment->DeleteLocalRef(value);
        } catch (...) {
          environment->DeleteLocalRef(key);
          environment->DeleteLocalRef(value);
          throw;
        }
      } catch (...) {
        environment->DeleteLocalRef(entry);
        throw;
      }
      environment->DeleteLocalRef(entry);
    }
    ensureJavaCallSucceeded(environment, "parameter map iterator hasNext");
    environment->DeleteLocalRef(iterator);
    environment->DeleteLocalRef(entries);
    environment->DeleteLocalRef(mapClass);
    return result;
  } catch (...) {
    if (iterator != nullptr) environment->DeleteLocalRef(iterator);
    if (entries != nullptr) environment->DeleteLocalRef(entries);
    environment->DeleteLocalRef(mapClass);
    throw;
  }
}

rti::AttributeHandleSet attributeHandleSet(JNIEnv* environment, jobject values) {
  if (values == nullptr) {
    throw std::runtime_error("A Java attribute-handle set must not be null");
  }
  jclass setClass = environment->GetObjectClass(values);
  if (setClass == nullptr) {
    clearJavaException(environment);
    throw std::runtime_error("Could not inspect a Java attribute-handle set");
  }
  jobject iterator = nullptr;
  try {
    auto const iteratorMethod = requiredMethod(environment, setClass, "iterator", "()Ljava/util/Iterator;");
    iterator = environment->CallObjectMethod(values, iteratorMethod);
    environment->DeleteLocalRef(setClass);
    setClass = nullptr;
    ensureJavaCallSucceeded(environment, "attribute set iterator");
    if (iterator == nullptr) throw std::runtime_error("A Java attribute set returned null iterator");
    jclass iteratorClass = environment->GetObjectClass(iterator);
    if (iteratorClass == nullptr) {
      clearJavaException(environment);
      throw std::runtime_error("Could not inspect a Java attribute-set iterator");
    }
    auto const hasNext = requiredMethod(environment, iteratorClass, "hasNext", "()Z");
    auto const next = requiredMethod(environment, iteratorClass, "next", "()Ljava/lang/Object;");
    environment->DeleteLocalRef(iteratorClass);
    rti::AttributeHandleSet result;
    while (environment->CallBooleanMethod(iterator, hasNext) == JNI_TRUE) {
      ensureJavaCallSucceeded(environment, "attribute set iterator hasNext");
      jobject value = environment->CallObjectMethod(iterator, next);
      ensureJavaCallSucceeded(environment, "attribute set iterator next");
      if (value == nullptr) throw std::runtime_error("A Java attribute set contained a null handle");
      try {
        result.insert(rti::umbra_binding_detail::decodeAttributeHandle(
            encodedJavaHandle(environment, value)));
        environment->DeleteLocalRef(value);
      } catch (...) {
        environment->DeleteLocalRef(value);
        throw;
      }
    }
    ensureJavaCallSucceeded(environment, "attribute set iterator hasNext");
    environment->DeleteLocalRef(iterator);
    return result;
  } catch (...) {
    if (iterator != nullptr) environment->DeleteLocalRef(iterator);
    if (setClass != nullptr) environment->DeleteLocalRef(setClass);
    throw;
  }
}

template <typename HandleSet, typename Decode>
HandleSet opaqueHandleSet(
    JNIEnv* environment,
    jobject values,
    Decode decode,
    char const* description) {
  if (values == nullptr) {
    throw std::runtime_error(std::string("A Java ") + description + " set must not be null");
  }
  jclass setClass = environment->GetObjectClass(values);
  if (setClass == nullptr) {
    clearJavaException(environment);
    throw std::runtime_error(std::string("Could not inspect a Java ") + description + " set");
  }
  jobject iterator = nullptr;
  try {
    auto const iteratorMethod = requiredMethod(environment, setClass, "iterator", "()Ljava/util/Iterator;");
    iterator = environment->CallObjectMethod(values, iteratorMethod);
    environment->DeleteLocalRef(setClass);
    setClass = nullptr;
    ensureJavaCallSucceeded(environment, "RTI handle-set iterator");
    if (iterator == nullptr) {
      throw std::runtime_error(std::string("A Java ") + description + " set returned a null iterator");
    }
    jclass iteratorClass = environment->GetObjectClass(iterator);
    if (iteratorClass == nullptr) {
      clearJavaException(environment);
      throw std::runtime_error("Could not inspect a Java RTI handle-set iterator");
    }
    auto const hasNext = requiredMethod(environment, iteratorClass, "hasNext", "()Z");
    auto const next = requiredMethod(environment, iteratorClass, "next", "()Ljava/lang/Object;");
    environment->DeleteLocalRef(iteratorClass);
    HandleSet result;
    while (environment->CallBooleanMethod(iterator, hasNext) == JNI_TRUE) {
      ensureJavaCallSucceeded(environment, "RTI handle-set iterator hasNext");
      jobject value = environment->CallObjectMethod(iterator, next);
      ensureJavaCallSucceeded(environment, "RTI handle-set iterator next");
      if (value == nullptr) {
        throw std::runtime_error(std::string("A Java ") + description + " set contained a null handle");
      }
      try {
        result.insert(decode(encodedJavaHandle(environment, value)));
        environment->DeleteLocalRef(value);
      } catch (...) {
        environment->DeleteLocalRef(value);
        throw;
      }
    }
    ensureJavaCallSucceeded(environment, "RTI handle-set iterator hasNext");
    environment->DeleteLocalRef(iterator);
    return result;
  } catch (...) {
    if (iterator != nullptr) environment->DeleteLocalRef(iterator);
    if (setClass != nullptr) environment->DeleteLocalRef(setClass);
    throw;
  }
}

rti::DimensionHandleSet dimensionHandleSet(JNIEnv* environment, jobject values) {
  return opaqueHandleSet<rti::DimensionHandleSet>(
      environment,
      values,
      [](rti::VariableLengthData const& encoded) {
        return rti::umbra_binding_detail::decodeDimensionHandle(encoded);
      },
      "dimension-handle");
}

rti::RegionHandleSet regionHandleSet(JNIEnv* environment, jobject values) {
  return opaqueHandleSet<rti::RegionHandleSet>(
      environment,
      values,
      [](rti::VariableLengthData const& encoded) {
        return rti::umbra_binding_detail::decodeRegionHandle(encoded);
      },
      "region-handle");
}

rti::InteractionClassHandleSet interactionClassHandleSet(
    JNIEnv* environment, jobject values) {
  return opaqueHandleSet<rti::InteractionClassHandleSet>(
      environment,
      values,
      [](rti::VariableLengthData const& encoded) {
        return rti::umbra_binding_detail::decodeInteractionClassHandle(encoded);
      },
      "interaction-class-handle");
}

rti::AttributeHandleSetRegionHandleSetPairVector attributeRegionPairs(
    JNIEnv* environment, jobject values) {
  if (values == nullptr) {
    throw std::runtime_error("A Java attribute/region pair list must not be null");
  }
  jclass listClass = environment->GetObjectClass(values);
  if (listClass == nullptr) {
    clearJavaException(environment);
    throw std::runtime_error("Could not inspect a Java attribute/region pair list");
  }
  jobject iterator = nullptr;
  try {
    auto const iteratorMethod = requiredMethod(
        environment, listClass, "iterator", "()Ljava/util/Iterator;");
    iterator = environment->CallObjectMethod(values, iteratorMethod);
    environment->DeleteLocalRef(listClass);
    listClass = nullptr;
    ensureJavaCallSucceeded(environment, "attribute/region pair-list iterator");
    if (iterator == nullptr) {
      throw std::runtime_error("A Java attribute/region pair list returned a null iterator");
    }
    jclass iteratorClass = environment->GetObjectClass(iterator);
    if (iteratorClass == nullptr) {
      clearJavaException(environment);
      throw std::runtime_error("Could not inspect a Java attribute/region pair-list iterator");
    }
    auto const hasNext = requiredMethod(environment, iteratorClass, "hasNext", "()Z");
    auto const next = requiredMethod(environment, iteratorClass, "next", "()Ljava/lang/Object;");
    environment->DeleteLocalRef(iteratorClass);
    rti::AttributeHandleSetRegionHandleSetPairVector result;
    while (environment->CallBooleanMethod(iterator, hasNext) == JNI_TRUE) {
      ensureJavaCallSucceeded(environment, "attribute/region pair-list iterator hasNext");
      jobject pair = environment->CallObjectMethod(iterator, next);
      ensureJavaCallSucceeded(environment, "attribute/region pair-list iterator next");
      if (pair == nullptr) {
        throw std::runtime_error("A Java attribute/region pair list contained a null pair");
      }
      jobject attributes = nullptr;
      jobject regions = nullptr;
      jclass pairClass = nullptr;
      try {
        pairClass = environment->GetObjectClass(pair);
        if (pairClass == nullptr) {
          clearJavaException(environment);
          throw std::runtime_error("Could not inspect a Java attribute/region pair");
        }
        auto const getAttributes = environment->GetMethodID(
            pairClass, "getAttributes", "()Lhla/rti1516_2025/AttributeHandleSet;");
        if (getAttributes == nullptr) clearJavaException(environment);
        auto const getRegions = environment->GetMethodID(
            pairClass, "getRegions", "()Lhla/rti1516_2025/RegionHandleSet;");
        if (getRegions == nullptr) clearJavaException(environment);
        if (getAttributes != nullptr && getRegions != nullptr) {
          // The compact fixture's historical value object exposes accessors.
          attributes = environment->CallObjectMethod(pair, getAttributes);
          ensureJavaCallSucceeded(environment, "attribute/region pair attributes");
          regions = environment->CallObjectMethod(pair, getRegions);
          ensureJavaCallSucceeded(environment, "attribute/region pair regions");
        } else {
          // IEEE 1516.1-2025 uses AttributeRegionAssociation with public
          // final ahset/rhset fields rather than accessor methods.
          auto const ahset = environment->GetFieldID(
              pairClass, "ahset", "Lhla/rti1516_2025/AttributeHandleSet;");
          if (ahset == nullptr) {
            clearJavaException(environment);
            throw std::runtime_error(
                "A Java attribute/region association exposes neither getAttributes nor ahset");
          }
          auto const rhset = environment->GetFieldID(
              pairClass, "rhset", "Lhla/rti1516_2025/RegionHandleSet;");
          if (rhset == nullptr) {
            clearJavaException(environment);
            throw std::runtime_error(
                "A Java attribute/region association exposes neither getRegions nor rhset");
          }
          attributes = environment->GetObjectField(pair, ahset);
          ensureJavaCallSucceeded(environment, "attribute/region association ahset");
          regions = environment->GetObjectField(pair, rhset);
          ensureJavaCallSucceeded(environment, "attribute/region association rhset");
        }
        environment->DeleteLocalRef(pairClass);
        pairClass = nullptr;
        if (attributes == nullptr || regions == nullptr) {
          throw std::runtime_error("A Java attribute/region pair requires attributes and regions");
        }
        result.emplace_back(
            attributeHandleSet(environment, attributes),
            regionHandleSet(environment, regions));
        environment->DeleteLocalRef(attributes);
        environment->DeleteLocalRef(regions);
        environment->DeleteLocalRef(pair);
      } catch (...) {
        if (pairClass != nullptr) environment->DeleteLocalRef(pairClass);
        if (attributes != nullptr) environment->DeleteLocalRef(attributes);
        if (regions != nullptr) environment->DeleteLocalRef(regions);
        environment->DeleteLocalRef(pair);
        throw;
      }
    }
    ensureJavaCallSucceeded(environment, "attribute/region pair-list iterator hasNext");
    environment->DeleteLocalRef(iterator);
    return result;
  } catch (...) {
    if (iterator != nullptr) environment->DeleteLocalRef(iterator);
    if (listClass != nullptr) environment->DeleteLocalRef(listClass);
    throw;
  }
}

rti::AttributeHandleValueMap attributeHandleValueMap(JNIEnv* environment, jobject values) {
  if (values == nullptr) {
    throw std::runtime_error("A Java attribute-handle value map must not be null");
  }
  jclass mapClass = environment->GetObjectClass(values);
  if (mapClass == nullptr) {
    clearJavaException(environment);
    throw std::runtime_error("Could not inspect a Java attribute-handle value map");
  }
  jobject entries = nullptr;
  jobject iterator = nullptr;
  try {
    auto const entrySet = requiredMethod(environment, mapClass, "entrySet", "()Ljava/util/Set;");
    entries = environment->CallObjectMethod(values, entrySet);
    ensureJavaCallSucceeded(environment, "attribute map entrySet");
    if (entries == nullptr) throw std::runtime_error("A Java attribute map returned null entrySet");
    jclass setClass = environment->GetObjectClass(entries);
    if (setClass == nullptr) {
      clearJavaException(environment);
      throw std::runtime_error("Could not inspect Java attribute-map entries");
    }
    auto const iteratorMethod = requiredMethod(environment, setClass, "iterator", "()Ljava/util/Iterator;");
    iterator = environment->CallObjectMethod(entries, iteratorMethod);
    environment->DeleteLocalRef(setClass);
    ensureJavaCallSucceeded(environment, "attribute map iterator");
    if (iterator == nullptr) throw std::runtime_error("A Java attribute map returned null iterator");
    jclass iteratorClass = environment->GetObjectClass(iterator);
    if (iteratorClass == nullptr) {
      clearJavaException(environment);
      throw std::runtime_error("Could not inspect Java attribute-map iterator");
    }
    auto const hasNext = requiredMethod(environment, iteratorClass, "hasNext", "()Z");
    auto const next = requiredMethod(environment, iteratorClass, "next", "()Ljava/lang/Object;");
    environment->DeleteLocalRef(iteratorClass);
    rti::AttributeHandleValueMap result;
    while (environment->CallBooleanMethod(iterator, hasNext) == JNI_TRUE) {
      ensureJavaCallSucceeded(environment, "attribute map iterator hasNext");
      jobject entry = environment->CallObjectMethod(iterator, next);
      ensureJavaCallSucceeded(environment, "attribute map iterator next");
      if (entry == nullptr) throw std::runtime_error("A Java attribute map contained a null entry");
      try {
        jclass entryClass = environment->GetObjectClass(entry);
        if (entryClass == nullptr) {
          clearJavaException(environment);
          throw std::runtime_error("Could not inspect a Java attribute-map entry");
        }
        auto const getKey = requiredMethod(environment, entryClass, "getKey", "()Ljava/lang/Object;");
        auto const getValue = requiredMethod(environment, entryClass, "getValue", "()Ljava/lang/Object;");
        jobject key = environment->CallObjectMethod(entry, getKey);
        ensureJavaCallSucceeded(environment, "attribute map entry key");
        auto value = static_cast<jbyteArray>(environment->CallObjectMethod(entry, getValue));
        ensureJavaCallSucceeded(environment, "attribute map entry value");
        environment->DeleteLocalRef(entryClass);
        if (key == nullptr || value == nullptr) {
          if (key != nullptr) environment->DeleteLocalRef(key);
          if (value != nullptr) environment->DeleteLocalRef(value);
          throw std::runtime_error("A Java attribute map requires non-null handles and values");
        }
        try {
          auto handle = rti::umbra_binding_detail::decodeAttributeHandle(
              encodedJavaHandle(environment, key));
          auto data = variableLengthData(environment, value);
          result.insert_or_assign(std::move(handle), std::move(data));
          environment->DeleteLocalRef(key);
          environment->DeleteLocalRef(value);
        } catch (...) {
          environment->DeleteLocalRef(key);
          environment->DeleteLocalRef(value);
          throw;
        }
      } catch (...) {
        environment->DeleteLocalRef(entry);
        throw;
      }
      environment->DeleteLocalRef(entry);
    }
    ensureJavaCallSucceeded(environment, "attribute map iterator hasNext");
    environment->DeleteLocalRef(iterator);
    environment->DeleteLocalRef(entries);
    environment->DeleteLocalRef(mapClass);
    return result;
  } catch (...) {
    if (iterator != nullptr) environment->DeleteLocalRef(iterator);
    if (entries != nullptr) environment->DeleteLocalRef(entries);
    environment->DeleteLocalRef(mapClass);
    throw;
  }
}

std::set<std::wstring> objectInstanceNameSet(JNIEnv* environment, jobject values) {
  if (values == nullptr) {
    throw std::runtime_error("A Java object-instance name set must not be null");
  }
  jclass setClass = environment->GetObjectClass(values);
  if (setClass == nullptr) {
    clearJavaException(environment);
    throw std::runtime_error("Could not inspect a Java object-instance name set");
  }
  jobject iterator = nullptr;
  try {
    auto const iteratorMethod = requiredMethod(environment, setClass, "iterator", "()Ljava/util/Iterator;");
    iterator = environment->CallObjectMethod(values, iteratorMethod);
    environment->DeleteLocalRef(setClass);
    setClass = nullptr;
    ensureJavaCallSucceeded(environment, "object-name set iterator");
    if (iterator == nullptr) throw std::runtime_error("A Java object-name set returned null iterator");
    jclass iteratorClass = environment->GetObjectClass(iterator);
    if (iteratorClass == nullptr) {
      clearJavaException(environment);
      throw std::runtime_error("Could not inspect a Java object-name-set iterator");
    }
    auto const hasNext = requiredMethod(environment, iteratorClass, "hasNext", "()Z");
    auto const next = requiredMethod(environment, iteratorClass, "next", "()Ljava/lang/Object;");
    environment->DeleteLocalRef(iteratorClass);
    std::set<std::wstring> result;
    while (environment->CallBooleanMethod(iterator, hasNext) == JNI_TRUE) {
      ensureJavaCallSucceeded(environment, "object-name set iterator hasNext");
      auto value = static_cast<jstring>(environment->CallObjectMethod(iterator, next));
      ensureJavaCallSucceeded(environment, "object-name set iterator next");
      if (value == nullptr) throw std::runtime_error("A Java object-name set contained null");
      try {
        result.insert(wideString(environment, value));
        environment->DeleteLocalRef(value);
      } catch (...) {
        environment->DeleteLocalRef(value);
        throw;
      }
    }
    ensureJavaCallSucceeded(environment, "object-name set iterator hasNext");
    environment->DeleteLocalRef(iterator);
    return result;
  } catch (...) {
    if (iterator != nullptr) environment->DeleteLocalRef(iterator);
    if (setClass != nullptr) environment->DeleteLocalRef(setClass);
    throw;
  }
}

class NativeHLAinteger32BE final {
 public:
  explicit NativeHLAinteger32BE(std::int32_t value) : element(value) {}

  rti::HLAinteger32BE element;
};

class NativeHLAinteger32LE final {
 public:
  explicit NativeHLAinteger32LE(std::int32_t value) : element(value) {}

  rti::HLAinteger32LE element;
};

class NativeHLAinteger64BE final {
 public:
  explicit NativeHLAinteger64BE(std::int64_t value) : element(value) {}

  rti::HLAinteger64BE element;
};

class NativeHLAinteger64LE final {
 public:
  explicit NativeHLAinteger64LE(std::int64_t value) : element(value) {}

  rti::HLAinteger64LE element;
};

class NativeHLAinteger16BE final {
 public:
  explicit NativeHLAinteger16BE(std::int16_t value) : element(value) {}

  rti::HLAinteger16BE element;
};

class NativeHLAinteger16LE final {
 public:
  explicit NativeHLAinteger16LE(std::int16_t value) : element(value) {}

  rti::HLAinteger16LE element;
};

template <typename Element, typename Value>
class NativeScalarElement final {
 public:
  explicit NativeScalarElement(Value value) : element(value) {}

  Value get() const { return element.get(); }
  void set(Value value) { element.set(value); }

  Element element;
};

template <typename Element>
class NativeOctetPairElement final {
 public:
  explicit NativeOctetPairElement(std::uint16_t value) { set(value); }

  std::uint16_t get() const {
    auto const pair = element.get();
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(static_cast<std::uint8_t>(pair.first)) << 8U) |
        static_cast<std::uint8_t>(pair.second));
  }

  void set(std::uint16_t value) {
    element.set(rti::OctetPair{
        static_cast<rti::Octet>((value >> 8U) & 0xffU),
        static_cast<rti::Octet>(value & 0xffU)});
  }

  Element element;
};

class NativeHLAASCIIstring final {
 public:
  explicit NativeHLAASCIIstring(std::string value) : element(std::move(value)) {}

  std::string get() const { return element.get(); }
  void set(std::string value) { element.set(std::move(value)); }

  rti::HLAASCIIstring element;
};

class NativeHLAunicodeString final {
 public:
  explicit NativeHLAunicodeString(std::wstring value) : element(std::move(value)) {}

  std::wstring get() const { return element.get(); }
  void set(std::wstring value) { element.set(std::move(value)); }

  rti::HLAunicodeString element;
};

class NativeHLAopaqueData final {
 public:
  explicit NativeHLAopaqueData(std::vector<rti::Octet> value) { set(std::move(value)); }

  std::vector<rti::Octet> get() const {
    auto const length = element.dataLength();
    auto const* data = element.get();
    return data == nullptr ? std::vector<rti::Octet>{}
                           : std::vector<rti::Octet>(data, data + length);
  }

  void set(std::vector<rti::Octet> value) {
    element.set(value.empty() ? nullptr : value.data(), value.size());
  }

  rti::Octet get(std::size_t index) const {
    auto const value = get();
    if (index >= value.size()) throw std::out_of_range("HLAopaqueData index is out of range");
    return value[index];
  }

  rti::HLAopaqueData element;
};

class NativeHLAvariableArray final {
 public:
  explicit NativeHLAvariableArray(std::unique_ptr<rti::DataElement> prototype)
      : prototype(std::move(prototype)),
        element(std::make_unique<rti::HLAvariableArray>(*this->prototype)) {}

  void add(std::unique_ptr<rti::DataElement> value) { element->addElement(*value); }

  void resize(std::size_t requestedSize) {
    auto replacement = std::make_unique<rti::HLAvariableArray>(*prototype);
    auto const retained = std::min(requestedSize, element->size());
    for (std::size_t index = 0; index < retained; ++index) {
      replacement->addElement(element->get(index));
    }
    for (std::size_t index = retained; index < requestedSize; ++index) {
      replacement->addElement(*prototype);
    }
    element = std::move(replacement);
  }

  std::vector<rti::Octet> encodedElement(std::size_t index) const {
    auto const encoded = element->get(index).encode();
    auto const* bytes = static_cast<rti::Octet const*>(encoded.data());
    return bytes == nullptr ? std::vector<rti::Octet>{}
                            : std::vector<rti::Octet>(bytes, bytes + encoded.size());
  }

  std::unique_ptr<rti::DataElement> prototype;
  std::unique_ptr<rti::HLAvariableArray> element;
};

class NativeHLAfixedArray final {
 public:
  NativeHLAfixedArray(std::unique_ptr<rti::DataElement> prototype, std::size_t size)
      : prototype(std::move(prototype)),
        element(std::make_unique<rti::HLAfixedArray>(*this->prototype, size)) {}

  void set(std::size_t index, std::unique_ptr<rti::DataElement> value) {
    element->set(index, *value);
  }

  std::vector<rti::Octet> encodedElement(std::size_t index) const {
    auto const encoded = element->get(index).encode();
    auto const* bytes = static_cast<rti::Octet const*>(encoded.data());
    return bytes == nullptr ? std::vector<rti::Octet>{}
                            : std::vector<rti::Octet>(bytes, bytes + encoded.size());
  }

  std::unique_ptr<rti::DataElement> prototype;
  std::unique_ptr<rti::HLAfixedArray> element;
};

class NativeHLAfixedRecord final {
 public:
  void append(std::unique_ptr<rti::DataElement> value) {
    prototypes.push_back(value->clone());
    element.appendElement(*value);
  }
  void set(std::size_t index, std::unique_ptr<rti::DataElement> value) {
    if (!value->isSameTypeAs(*prototypes.at(index))) {
      throw rti::EncoderException(L"HLAfixedRecord replacement does not match its component type.");
    }
    element.set(index, *value);
  }
  std::vector<rti::Octet> encodedElement(std::size_t index) const {
    auto const encoded = element.get(index).encode();
    auto const* bytes = static_cast<rti::Octet const*>(encoded.data());
    return bytes == nullptr ? std::vector<rti::Octet>{} : std::vector<rti::Octet>(bytes, bytes + encoded.size());
  }
  rti::HLAfixedRecord element;
  std::vector<std::unique_ptr<rti::DataElement>> prototypes;
};

class NativeHLAvariantRecord final {
 public:
  explicit NativeHLAvariantRecord(std::unique_ptr<rti::DataElement> discriminantPrototype)
      : discriminantPrototype(std::move(discriminantPrototype)),
        element(std::make_unique<rti::HLAvariantRecord>(*this->discriminantPrototype)) {}

  rti::DataElement const* valuePrototypeFor(std::vector<rti::Octet> const& discriminant) const {
    for (auto const& variant : variants) {
      if (variant.discriminant == discriminant) return variant.valuePrototype.get();
    }
    return nullptr;
  }

  void addVariant(
      std::vector<rti::Octet> discriminantEncoding,
      std::unique_ptr<rti::DataElement> discriminant,
      std::unique_ptr<rti::DataElement> value) {
    auto valuePrototype = value->clone();
    element->addVariant(*discriminant, *value);
    variants.push_back({std::move(discriminantEncoding), std::move(valuePrototype)});
  }

  void setVariant(
      std::unique_ptr<rti::DataElement> discriminant,
      std::unique_ptr<rti::DataElement> value) {
    element->setVariant(*discriminant, *value);
  }

  std::vector<rti::Octet> discriminantEncoding() const {
    return encodedElement(element->getDiscriminant());
  }

  std::vector<rti::Octet> valueEncoding() const {
    return encodedElement(element->getVariant());
  }

  struct VariantType final {
    std::vector<rti::Octet> discriminant;
    std::unique_ptr<rti::DataElement> valuePrototype;
  };

  static std::vector<rti::Octet> encodedElement(rti::DataElement const& value) {
    auto const encoded = value.encode();
    auto const* bytes = static_cast<rti::Octet const*>(encoded.data());
    return bytes == nullptr ? std::vector<rti::Octet>{}
                            : std::vector<rti::Octet>(bytes, bytes + encoded.size());
  }

  std::unique_ptr<rti::DataElement> discriminantPrototype;
  std::unique_ptr<rti::HLAvariantRecord> element;
  std::vector<VariantType> variants;
};

class NativeHLAextendableVariantRecord final {
 public:
  explicit NativeHLAextendableVariantRecord(
      std::unique_ptr<rti::DataElement> discriminantPrototype)
      : discriminantPrototype(std::move(discriminantPrototype)),
        element(std::make_unique<rti::HLAextendableVariantRecord>(*this->discriminantPrototype)) {}

  rti::DataElement const* valuePrototypeFor(
      std::vector<rti::Octet> const& discriminant) const {
    for (auto const& variant : variants) {
      if (variant.discriminant == discriminant) return variant.valuePrototype.get();
    }
    return nullptr;
  }

  void addVariant(
      std::vector<rti::Octet> discriminantEncoding,
      std::unique_ptr<rti::DataElement> discriminant,
      std::unique_ptr<rti::DataElement> value) {
    auto valuePrototype = value->clone();
    element->addVariant(*discriminant, *value);
    variants.push_back({std::move(discriminantEncoding), std::move(valuePrototype)});
  }

  static std::vector<rti::Octet> encodedElement(rti::DataElement const& value) {
    auto const encoded = value.encode();
    auto const* bytes = static_cast<rti::Octet const*>(encoded.data());
    return bytes == nullptr ? std::vector<rti::Octet>{}
                            : std::vector<rti::Octet>(bytes, bytes + encoded.size());
  }

  std::vector<rti::Octet> discriminantEncoding() const {
    return encodedElement(element->getDiscriminant());
  }

  std::vector<rti::Octet> valueEncoding() const {
    return encodedElement(element->getVariant());
  }

  struct VariantType final {
    std::vector<rti::Octet> discriminant;
    std::unique_ptr<rti::DataElement> valuePrototype;
  };

  std::unique_ptr<rti::DataElement> discriminantPrototype;
  std::unique_ptr<rti::HLAextendableVariantRecord> element;
  std::vector<VariantType> variants;
};

jlong nativeCompositionHandle(JNIEnv* environment, jobject value, char const* className) {
  auto type = environment->FindClass(className);
  if (type == nullptr) {
    clearJavaException(environment);
    return 0;
  }
  bool const matches = environment->IsInstanceOf(value, type) == JNI_TRUE;
  if (!matches) {
    environment->DeleteLocalRef(type);
    return 0;
  }
  auto const method = environment->GetMethodID(type, "nativeHandleForComposition", "()J");
  environment->DeleteLocalRef(type);
  if (method == nullptr) {
    clearJavaException(environment);
    throw std::runtime_error("Umbra native composite does not expose its JNI handle");
  }
  auto const handle = environment->CallLongMethod(value, method);
  if (environment->ExceptionCheck()) {
    clearJavaException(environment);
    throw rti::EncoderException(L"The native composite data element is closed.");
  }
  return handle;
}

std::unique_ptr<rti::DataElement> cloneNativeCompositeDataElement(JNIEnv* environment, jobject value) {
  if (value == nullptr) {
    throw rti::EncoderException(L"A composite data element must not be null.");
  }
  // The enclosing data-element conversion continues with primitive types
  // when this Java object is not one of Umbra's native composites.
  if (auto const handle = nativeCompositionHandle(
          environment, value, "org/umbra/jni/rti1516_2025/NativeHLAvariableArray")) {
    auto* state = reinterpret_cast<NativeHLAvariableArray*>(handle);
    return state->element->clone();
  }
  if (auto const handle = nativeCompositionHandle(
          environment, value, "org/umbra/jni/rti1516_2025/NativeHLAfixedArray")) {
    auto* state = reinterpret_cast<NativeHLAfixedArray*>(handle);
    return state->element->clone();
  }
  if (auto const handle = nativeCompositionHandle(
          environment, value, "org/umbra/jni/rti1516_2025/NativeHLAfixedRecord")) {
    auto* state = reinterpret_cast<NativeHLAfixedRecord*>(handle);
    return state->element.clone();
  }
  if (auto const handle = nativeCompositionHandle(
          environment, value, "org/umbra/jni/rti1516_2025/NativeHLAvariantRecord")) {
    auto* state = reinterpret_cast<NativeHLAvariantRecord*>(handle);
    return state->element->clone();
  }
  if (auto const handle = nativeCompositionHandle(
          environment, value, "org/umbra/jni/rti1516_2025/NativeCompositionDataElement")) {
    auto* state = reinterpret_cast<NativeHLAextendableVariantRecord*>(handle);
    return state->element->clone();
  }
  return nullptr;
}

std::unique_ptr<rti::DataElement> nativeDataElementFromJava(JNIEnv* environment, jobject value) {
  if (auto composite = cloneNativeCompositeDataElement(environment, value)) return composite;
  auto const type = nativeDataElementType(environment, value);
  return nativeDataElementFromJava(environment, value, type);
}

std::unique_ptr<rti::DataElement> nativeDataElementFromJava(
    JNIEnv* environment, jobject value, rti::DataElement const& prototype) {
  auto result = nativeDataElementFromJava(environment, value);
  if (result != nullptr && !result->isSameTypeAs(prototype)) {
    throw rti::EncoderException(L"The Java data element does not match the native C++ prototype.");
  }
  return result;
}

using NativeHLAfloat32BE = NativeScalarElement<rti::HLAfloat32BE, float>;
using NativeHLAfloat32LE = NativeScalarElement<rti::HLAfloat32LE, float>;
using NativeHLAfloat64BE = NativeScalarElement<rti::HLAfloat64BE, double>;
using NativeHLAfloat64LE = NativeScalarElement<rti::HLAfloat64LE, double>;
using NativeHLAunsignedInteger16BE =
    NativeScalarElement<rti::HLAunsignedInteger16BE, std::uint16_t>;
using NativeHLAunsignedInteger16LE =
    NativeScalarElement<rti::HLAunsignedInteger16LE, std::uint16_t>;
using NativeHLAunsignedInteger32BE =
    NativeScalarElement<rti::HLAunsignedInteger32BE, std::uint32_t>;
using NativeHLAunsignedInteger32LE =
    NativeScalarElement<rti::HLAunsignedInteger32LE, std::uint32_t>;
using NativeHLAunsignedInteger64BE =
    NativeScalarElement<rti::HLAunsignedInteger64BE, std::uint64_t>;
using NativeHLAunsignedInteger64LE =
    NativeScalarElement<rti::HLAunsignedInteger64LE, std::uint64_t>;
using NativeHLAbyte = NativeScalarElement<rti::HLAbyte, rti::Octet>;
using NativeHLAoctet = NativeScalarElement<rti::HLAoctet, rti::Octet>;
using NativeHLAoctetPairBE = NativeOctetPairElement<rti::HLAoctetPairBE>;
using NativeHLAoctetPairLE = NativeOctetPairElement<rti::HLAoctetPairLE>;
using NativeHLAASCIIchar = NativeScalarElement<rti::HLAASCIIchar, std::uint8_t>;
using NativeHLAunicodeChar = NativeScalarElement<rti::HLAunicodeChar, std::uint16_t>;
using NativeHLAboolean = NativeScalarElement<rti::HLAboolean, bool>;

template <typename State>
State* scalarStateFor(JNIEnv* environment, jlong handle, char const* typeName) {
  if (handle == 0) {
    auto const message = std::string("The native ") + typeName + " value is closed.";
    throwJavaException(
        environment,
        "hla/rti1516_2025/exceptions/RTIinternalError",
        message.c_str());
    return nullptr;
  }
  return reinterpret_cast<State*>(handle);
}

NativeHLAinteger32BE* integer32StateFor(JNIEnv* environment, jlong handle) {
  if (handle == 0) {
    throwJavaException(
        environment,
        "hla/rti1516_2025/exceptions/RTIinternalError",
        "The native HLAinteger32BE value is closed.");
    return nullptr;
  }
  return reinterpret_cast<NativeHLAinteger32BE*>(handle);
}

NativeHLAinteger32LE* integer32LEStateFor(JNIEnv* environment, jlong handle) {
  if (handle == 0) {
    throwJavaException(
        environment,
        "hla/rti1516_2025/exceptions/RTIinternalError",
        "The native HLAinteger32LE value is closed.");
    return nullptr;
  }
  return reinterpret_cast<NativeHLAinteger32LE*>(handle);
}

NativeHLAinteger64BE* integer64StateFor(JNIEnv* environment, jlong handle) {
  if (handle == 0) {
    throwJavaException(
        environment,
        "hla/rti1516_2025/exceptions/RTIinternalError",
        "The native HLAinteger64BE value is closed.");
    return nullptr;
  }
  return reinterpret_cast<NativeHLAinteger64BE*>(handle);
}

NativeHLAinteger64LE* integer64LEStateFor(JNIEnv* environment, jlong handle) {
  if (handle == 0) {
    throwJavaException(
        environment,
        "hla/rti1516_2025/exceptions/RTIinternalError",
        "The native HLAinteger64LE value is closed.");
    return nullptr;
  }
  return reinterpret_cast<NativeHLAinteger64LE*>(handle);
}

NativeHLAinteger16BE* integer16StateFor(JNIEnv* environment, jlong handle) {
  if (handle == 0) {
    throwJavaException(
        environment,
        "hla/rti1516_2025/exceptions/RTIinternalError",
        "The native HLAinteger16BE value is closed.");
    return nullptr;
  }
  return reinterpret_cast<NativeHLAinteger16BE*>(handle);
}

NativeHLAinteger16LE* integer16LEStateFor(JNIEnv* environment, jlong handle) {
  if (handle == 0) {
    throwJavaException(
        environment,
        "hla/rti1516_2025/exceptions/RTIinternalError",
        "The native HLAinteger16LE value is closed.");
    return nullptr;
  }
  return reinterpret_cast<NativeHLAinteger16LE*>(handle);
}

class ScopedJNIEnvironment final {
 public:
  explicit ScopedJNIEnvironment(JavaVM* virtualMachine) : virtualMachine_(virtualMachine) {
    void* rawEnvironment = nullptr;
    auto const status = virtualMachine_->GetEnv(&rawEnvironment, JNI_VERSION_1_8);
    if (status == JNI_OK) {
      environment_ = static_cast<JNIEnv*>(rawEnvironment);
      return;
    }
    if (status != JNI_EDETACHED ||
        virtualMachine_->AttachCurrentThread(reinterpret_cast<void**>(&environment_), nullptr) != JNI_OK) {
      throw rti::FederateInternalError(L"Umbra could not attach the JNI callback thread.");
    }
    attached_ = true;
  }

  ~ScopedJNIEnvironment() {
    if (attached_) {
      virtualMachine_->DetachCurrentThread();
    }
  }

  [[nodiscard]] JNIEnv* get() const noexcept { return environment_; }

 private:
  JavaVM* virtualMachine_;
  JNIEnv* environment_ = nullptr;
  bool attached_ = false;
};

jclass globalClass(JNIEnv* environment, char const* name) {
  jclass local = environment->FindClass(name);
  if (local == nullptr) {
    clearJavaException(environment);
    throw std::runtime_error(std::string("Could not resolve Java class ") + name);
  }
  auto global = static_cast<jclass>(environment->NewGlobalRef(local));
  environment->DeleteLocalRef(local);
  if (global == nullptr) {
    clearJavaException(environment);
    throw std::runtime_error(std::string("Could not retain Java class ") + name);
  }
  return global;
}

jmethodID requiredMethod(
    JNIEnv* environment, jclass owner, char const* name, char const* signature) {
  jmethodID method = environment->GetMethodID(owner, name, signature);
  if (method == nullptr) {
    clearJavaException(environment);
    throw std::runtime_error(std::string("Could not resolve Java method ") + name);
  }
  return method;
}

jmethodID requiredStaticMethod(
    JNIEnv* environment, jclass owner, char const* name, char const* signature) {
  jmethodID method = environment->GetStaticMethodID(owner, name, signature);
  if (method == nullptr) {
    clearJavaException(environment);
    throw std::runtime_error(std::string("Could not resolve Java static method ") + name);
  }
  return method;
}

class JavaFederateAmbassador final : public rti::NullFederateAmbassador {
 public:
  JavaFederateAmbassador(JNIEnv* environment, JavaVM* virtualMachine)
      : virtualMachine_(virtualMachine),
        executionInformationSetClass_(globalClass(
            environment,
            "org/umbra/jni/rti1516_2025/NativeFederationExecutionInformationSet")),
        executionInformationClass_(globalClass(
            environment,
            "hla/rti1516_2025/FederationExecutionInformation")),
        memberInformationSetClass_(globalClass(
            environment,
            "org/umbra/jni/rti1516_2025/NativeFederationExecutionMemberInformationSet")),
        memberInformationClass_(globalClass(
            environment,
            "hla/rti1516_2025/FederationExecutionMemberInformation")),
        federateHandleClass_(globalClass(
            environment,
            "org/umbra/jni/rti1516_2025/NativeFederateHandle")),
        federateHandleSetClass_(globalClass(
            environment,
            "org/umbra/jni/rti1516_2025/NativeFederateHandleSet")),
        objectInstanceHandleClass_(globalClass(
            environment,
            "org/umbra/jni/rti1516_2025/NativeObjectInstanceHandle")),
        objectClassHandleClass_(globalClass(
            environment,
            "org/umbra/jni/rti1516_2025/NativeObjectClassHandle")),
        attributeHandleClass_(globalClass(
            environment,
            "org/umbra/jni/rti1516_2025/NativeAttributeHandle")),
        attributeHandleSetClass_(globalClass(
            environment,
            "org/umbra/jni/rti1516_2025/NativeAttributeHandleSet")),
        regionHandleClass_(globalClass(
            environment,
            "org/umbra/jni/rti1516_2025/NativeRegionHandle")),
        regionHandleSetClass_(globalClass(
            environment,
            "org/umbra/jni/rti1516_2025/NativeRegionHandleSet")),
        attributeHandleValueMapClass_(globalClass(
            environment,
            "org/umbra/jni/rti1516_2025/NativeAttributeHandleValueMap")),
        objectInstanceNameSetClass_(globalClass(environment, "java/util/HashSet")),
        interactionClassHandleClass_(globalClass(
            environment,
            "org/umbra/jni/rti1516_2025/NativeInteractionClassHandle")),
        parameterHandleClass_(globalClass(
            environment,
            "org/umbra/jni/rti1516_2025/NativeParameterHandle")),
        parameterHandleValueMapClass_(globalClass(
            environment,
            "org/umbra/jni/rti1516_2025/NativeParameterHandleValueMap")),
        transportationTypeHandleClass_(globalClass(
            environment,
            "org/umbra/jni/rti1516_2025/NativeTransportationTypeHandle")),
        saveStatusClass_(globalClass(environment, "hla/rti1516_2025/SaveStatus")),
        saveFailureReasonClass_(globalClass(
            environment,
            "hla/rti1516_2025/SaveFailureReason")),
        synchronizationPointFailureReasonClass_(globalClass(
            environment,
            "hla/rti1516_2025/SynchronizationPointFailureReason")),
        restoreStatusClass_(globalClass(environment, "hla/rti1516_2025/RestoreStatus")),
        restoreFailureReasonClass_(globalClass(
            environment,
            "hla/rti1516_2025/RestoreFailureReason")),
        federateHandleSaveStatusPairClass_(globalClass(
            environment,
            "hla/rti1516_2025/FederateHandleSaveStatusPair")),
        federateRestoreStatusClass_(globalClass(
            environment,
            "hla/rti1516_2025/FederateRestoreStatus")) {
    executionInformationSetConstructor_ = requiredMethod(
        environment, executionInformationSetClass_, "<init>", "()V");
    executionInformationConstructor_ = requiredMethod(
        environment,
        executionInformationClass_,
        "<init>",
        "(Ljava/lang/String;Ljava/lang/String;)V");
    memberInformationSetConstructor_ = requiredMethod(
        environment, memberInformationSetClass_, "<init>", "()V");
    memberInformationConstructor_ = requiredMethod(
        environment,
        memberInformationClass_,
        "<init>",
        "(Ljava/lang/String;Ljava/lang/String;)V");
    federateHandleConstructor_ = requiredMethod(
        environment, federateHandleClass_, "<init>", "([B)V");
    federateHandleSetConstructor_ = requiredMethod(
        environment, federateHandleSetClass_, "<init>", "()V");
    objectInstanceHandleConstructor_ = requiredMethod(
        environment, objectInstanceHandleClass_, "<init>", "([B)V");
    objectClassHandleConstructor_ = requiredMethod(
        environment, objectClassHandleClass_, "<init>", "([B)V");
    attributeHandleConstructor_ = requiredMethod(
        environment, attributeHandleClass_, "<init>", "([B)V");
    attributeHandleSetConstructor_ = requiredMethod(
        environment, attributeHandleSetClass_, "<init>", "()V");
    regionHandleConstructor_ = requiredMethod(
        environment, regionHandleClass_, "<init>", "([B)V");
    regionHandleSetConstructor_ = requiredMethod(
        environment, regionHandleSetClass_, "<init>", "()V");
    attributeHandleValueMapConstructor_ = requiredMethod(
        environment, attributeHandleValueMapClass_, "<init>", "()V");
    objectInstanceNameSetConstructor_ = requiredMethod(
        environment, objectInstanceNameSetClass_, "<init>", "()V");
    interactionClassHandleConstructor_ = requiredMethod(
        environment, interactionClassHandleClass_, "<init>", "([B)V");
    parameterHandleConstructor_ = requiredMethod(
        environment, parameterHandleClass_, "<init>", "([B)V");
    parameterHandleValueMapConstructor_ = requiredMethod(
        environment, parameterHandleValueMapClass_, "<init>", "()V");
    transportationTypeHandleConstructor_ = requiredMethod(
        environment, transportationTypeHandleClass_, "<init>", "([B)V");
    setAdd_ = requiredMethod(environment, executionInformationSetClass_, "add", "(Ljava/lang/Object;)Z");
    memberSetAdd_ = requiredMethod(environment, memberInformationSetClass_, "add", "(Ljava/lang/Object;)Z");
    federateHandleSetAdd_ = requiredMethod(
        environment, federateHandleSetClass_, "add", "(Ljava/lang/Object;)Z");
    attributeHandleSetAdd_ = requiredMethod(
        environment, attributeHandleSetClass_, "add", "(Ljava/lang/Object;)Z");
    regionHandleSetAdd_ = requiredMethod(
        environment, regionHandleSetClass_, "add", "(Ljava/lang/Object;)Z");
    attributeHandleValueMapPut_ = requiredMethod(
        environment, attributeHandleValueMapClass_, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
    objectInstanceNameSetAdd_ = requiredMethod(
        environment, objectInstanceNameSetClass_, "add", "(Ljava/lang/Object;)Z");
    parameterHandleValueMapPut_ = requiredMethod(
        environment, parameterHandleValueMapClass_, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
    saveStatusValueOf_ = requiredStaticMethod(
        environment,
        saveStatusClass_,
        "valueOf",
        "(Ljava/lang/String;)Lhla/rti1516_2025/SaveStatus;");
    saveFailureReasonValueOf_ = requiredStaticMethod(
        environment,
        saveFailureReasonClass_,
        "valueOf",
        "(Ljava/lang/String;)Lhla/rti1516_2025/SaveFailureReason;");
    synchronizationPointFailureReasonValueOf_ = requiredStaticMethod(
        environment,
        synchronizationPointFailureReasonClass_,
        "valueOf",
        "(Ljava/lang/String;)Lhla/rti1516_2025/SynchronizationPointFailureReason;");
    restoreStatusValueOf_ = requiredStaticMethod(
        environment,
        restoreStatusClass_,
        "valueOf",
        "(Ljava/lang/String;)Lhla/rti1516_2025/RestoreStatus;");
    restoreFailureReasonValueOf_ = requiredStaticMethod(
        environment,
        restoreFailureReasonClass_,
        "valueOf",
        "(Ljava/lang/String;)Lhla/rti1516_2025/RestoreFailureReason;");
    federateHandleSaveStatusPairConstructor_ = requiredMethod(
        environment,
        federateHandleSaveStatusPairClass_,
        "<init>",
        "(Lhla/rti1516_2025/FederateHandle;Lhla/rti1516_2025/SaveStatus;)V");
    federateRestoreStatusConstructor_ = requiredMethod(
        environment,
        federateRestoreStatusClass_,
        "<init>",
        "(Lhla/rti1516_2025/FederateHandle;Lhla/rti1516_2025/FederateHandle;Lhla/rti1516_2025/RestoreStatus;)V");
  }

  ~JavaFederateAmbassador() noexcept override = default;

  // The Java time-factory registry uses the native ambassador handle so that
  // callback values are decoded by the same C++ RTI time factory that owns
  // arithmetic for the active federation.
  void setNativeHandle(jlong nativeHandle) { nativeHandle_ = nativeHandle; }

  void setTarget(JNIEnv* environment, jobject target) {
    if (target == nullptr) {
      throw rti::FederateInternalError(L"Umbra requires a Java FederateAmbassador callback target.");
    }
    jclass targetClass = environment->GetObjectClass(target);
    if (targetClass == nullptr) {
      clearJavaException(environment);
      throw rti::FederateInternalError(L"Umbra could not inspect the Java callback target.");
    }
    auto const connectionLost = requiredMethod(
        environment, targetClass, "connectionLost", "(Ljava/lang/String;)V");
    auto const reportExecutions = requiredMethod(
        environment,
        targetClass,
        "reportFederationExecutions",
        "(Lhla/rti1516_2025/FederationExecutionInformationSet;)V");
    auto const reportMembers = requiredMethod(
        environment,
        targetClass,
        "reportFederationExecutionMembers",
        "(Ljava/lang/String;Lhla/rti1516_2025/FederationExecutionMemberInformationSet;)V");
    auto const missingFederation = requiredMethod(
        environment,
        targetClass,
        "reportFederationExecutionDoesNotExist",
        "(Ljava/lang/String;)V");
    auto const federateResigned = requiredMethod(
        environment, targetClass, "federateResigned", "(Ljava/lang/String;)V");
    auto const synchronizationRegistrationSucceeded = requiredMethod(
        environment,
        targetClass,
        "synchronizationPointRegistrationSucceeded",
        "(Ljava/lang/String;)V");
    auto const synchronizationRegistrationFailed = requiredMethod(
        environment,
        targetClass,
        "synchronizationPointRegistrationFailed",
        "(Ljava/lang/String;Lhla/rti1516_2025/SynchronizationPointFailureReason;)V");
    auto const announceSynchronizationPoint = requiredMethod(
        environment,
        targetClass,
        "announceSynchronizationPoint",
        "(Ljava/lang/String;[B)V");
    auto const federationSynchronized = requiredMethod(
        environment,
        targetClass,
        "federationSynchronized",
        "(Ljava/lang/String;Lhla/rti1516_2025/FederateHandleSet;)V");
    auto const federationSaveStatusResponse = requiredMethod(
        environment,
        targetClass,
        "federationSaveStatusResponse",
        "([Lhla/rti1516_2025/FederateHandleSaveStatusPair;)V");
    auto const initiateFederateSave = requiredMethod(
        environment, targetClass, "initiateFederateSave", "(Ljava/lang/String;)V");
    auto const initiateFederateSaveWithTime = requiredMethod(
        environment,
        targetClass,
        "initiateFederateSave",
        "(Ljava/lang/String;Lhla/rti1516_2025/time/LogicalTime;)V");
    auto const federationSaved = requiredMethod(
        environment, targetClass, "federationSaved", "()V");
    auto const federationNotSaved = requiredMethod(
        environment,
        targetClass,
        "federationNotSaved",
        "(Lhla/rti1516_2025/SaveFailureReason;)V");
    auto const federationRestoreStatusResponse = requiredMethod(
        environment,
        targetClass,
        "federationRestoreStatusResponse",
        "([Lhla/rti1516_2025/FederateRestoreStatus;)V");
    auto const requestFederationRestoreSucceeded = requiredMethod(
        environment,
        targetClass,
        "requestFederationRestoreSucceeded",
        "(Ljava/lang/String;)V");
    auto const requestFederationRestoreFailed = requiredMethod(
        environment,
        targetClass,
        "requestFederationRestoreFailed",
        "(Ljava/lang/String;)V");
    auto const federationRestoreBegun = requiredMethod(
        environment, targetClass, "federationRestoreBegun", "()V");
    auto const initiateFederateRestore = requiredMethod(
        environment,
        targetClass,
        "initiateFederateRestore",
        "(Ljava/lang/String;Ljava/lang/String;Lhla/rti1516_2025/FederateHandle;)V");
    auto const federationRestored = requiredMethod(
        environment, targetClass, "federationRestored", "()V");
    auto const federationNotRestored = requiredMethod(
        environment,
        targetClass,
        "federationNotRestored",
        "(Lhla/rti1516_2025/RestoreFailureReason;)V");
    auto const startRegistrationForObjectClass = requiredMethod(
        environment,
        targetClass,
        "startRegistrationForObjectClass",
        "(Lhla/rti1516_2025/ObjectClassHandle;)V");
    auto const stopRegistrationForObjectClass = requiredMethod(
        environment,
        targetClass,
        "stopRegistrationForObjectClass",
        "(Lhla/rti1516_2025/ObjectClassHandle;)V");
    auto const turnInteractionsOn = requiredMethod(
        environment,
        targetClass,
        "turnInteractionsOn",
        "(Lhla/rti1516_2025/InteractionClassHandle;)V");
    auto const turnInteractionsOff = requiredMethod(
        environment,
        targetClass,
        "turnInteractionsOff",
        "(Lhla/rti1516_2025/InteractionClassHandle;)V");
    auto const turnUpdatesOnForObjectInstance = requiredMethod(
        environment,
        targetClass,
        "turnUpdatesOnForObjectInstance",
        "(Lhla/rti1516_2025/ObjectInstanceHandle;Lhla/rti1516_2025/AttributeHandleSet;)V");
    auto const turnUpdatesOnForObjectInstanceWithRate = requiredMethod(
        environment,
        targetClass,
        "turnUpdatesOnForObjectInstance",
        "(Lhla/rti1516_2025/ObjectInstanceHandle;Lhla/rti1516_2025/AttributeHandleSet;Ljava/lang/String;)V");
    auto const turnUpdatesOffForObjectInstance = requiredMethod(
        environment,
        targetClass,
        "turnUpdatesOffForObjectInstance",
        "(Lhla/rti1516_2025/ObjectInstanceHandle;Lhla/rti1516_2025/AttributeHandleSet;)V");
    auto const receiveInteraction = requiredMethod(
        environment,
        targetClass,
        "receiveInteraction",
        "(Lhla/rti1516_2025/InteractionClassHandle;Lhla/rti1516_2025/ParameterHandleValueMap;[BLhla/rti1516_2025/TransportationTypeHandle;Lhla/rti1516_2025/FederateHandle;Lhla/rti1516_2025/RegionHandleSet;)V");
    auto const receiveInteractionWithTime = requiredMethod(
        environment,
        targetClass,
        "receiveInteraction",
        "(Lhla/rti1516_2025/InteractionClassHandle;Lhla/rti1516_2025/ParameterHandleValueMap;[BLhla/rti1516_2025/TransportationTypeHandle;Lhla/rti1516_2025/FederateHandle;Lhla/rti1516_2025/RegionHandleSet;Lhla/rti1516_2025/time/LogicalTime;Lhla/rti1516_2025/OrderType;Lhla/rti1516_2025/OrderType;Lhla/rti1516_2025/MessageRetractionHandle;)V");
    auto const receiveDirectedInteraction = requiredMethod(
        environment,
        targetClass,
        "receiveDirectedInteraction",
        "(Lhla/rti1516_2025/InteractionClassHandle;Lhla/rti1516_2025/ObjectInstanceHandle;Lhla/rti1516_2025/ParameterHandleValueMap;[BLhla/rti1516_2025/TransportationTypeHandle;Lhla/rti1516_2025/FederateHandle;)V");
    auto const receiveDirectedInteractionWithTime = requiredMethod(
        environment,
        targetClass,
        "receiveDirectedInteraction",
        "(Lhla/rti1516_2025/InteractionClassHandle;Lhla/rti1516_2025/ObjectInstanceHandle;Lhla/rti1516_2025/ParameterHandleValueMap;[BLhla/rti1516_2025/TransportationTypeHandle;Lhla/rti1516_2025/FederateHandle;Lhla/rti1516_2025/time/LogicalTime;Lhla/rti1516_2025/OrderType;Lhla/rti1516_2025/OrderType;Lhla/rti1516_2025/MessageRetractionHandle;)V");
    auto const requestAttributeOwnershipAssumption = requiredMethod(
        environment,
        targetClass,
        "requestAttributeOwnershipAssumption",
        "(Lhla/rti1516_2025/ObjectInstanceHandle;Lhla/rti1516_2025/AttributeHandleSet;[B)V");
    auto const requestDivestitureConfirmation = requiredMethod(
        environment,
        targetClass,
        "requestDivestitureConfirmation",
        "(Lhla/rti1516_2025/ObjectInstanceHandle;Lhla/rti1516_2025/AttributeHandleSet;[B)V");
    auto const attributeOwnershipAcquisitionNotification = requiredMethod(
        environment,
        targetClass,
        "attributeOwnershipAcquisitionNotification",
        "(Lhla/rti1516_2025/ObjectInstanceHandle;Lhla/rti1516_2025/AttributeHandleSet;[B)V");
    auto const attributeOwnershipUnavailable = requiredMethod(
        environment,
        targetClass,
        "attributeOwnershipUnavailable",
        "(Lhla/rti1516_2025/ObjectInstanceHandle;Lhla/rti1516_2025/AttributeHandleSet;[B)V");
    auto const requestAttributeOwnershipRelease = requiredMethod(
        environment,
        targetClass,
        "requestAttributeOwnershipRelease",
        "(Lhla/rti1516_2025/ObjectInstanceHandle;Lhla/rti1516_2025/AttributeHandleSet;[B)V");
    auto const confirmAttributeOwnershipAcquisitionCancellation = requiredMethod(
        environment,
        targetClass,
        "confirmAttributeOwnershipAcquisitionCancellation",
        "(Lhla/rti1516_2025/ObjectInstanceHandle;Lhla/rti1516_2025/AttributeHandleSet;)V");
    auto const informAttributeOwnership = requiredMethod(
        environment,
        targetClass,
        "informAttributeOwnership",
        "(Lhla/rti1516_2025/ObjectInstanceHandle;Lhla/rti1516_2025/AttributeHandleSet;Lhla/rti1516_2025/FederateHandle;)V");
    auto const attributeIsNotOwned = requiredMethod(
        environment,
        targetClass,
        "attributeIsNotOwned",
        "(Lhla/rti1516_2025/ObjectInstanceHandle;Lhla/rti1516_2025/AttributeHandleSet;)V");
    auto const attributeIsOwnedByRTI = requiredMethod(
        environment,
        targetClass,
        "attributeIsOwnedByRTI",
        "(Lhla/rti1516_2025/ObjectInstanceHandle;Lhla/rti1516_2025/AttributeHandleSet;)V");
    auto const discoverObjectInstance = requiredMethod(
        environment,
        targetClass,
        "discoverObjectInstance",
        "(Lhla/rti1516_2025/ObjectInstanceHandle;Lhla/rti1516_2025/ObjectClassHandle;Ljava/lang/String;Lhla/rti1516_2025/FederateHandle;)V");
    auto const reflectAttributeValues = requiredMethod(
        environment,
        targetClass,
        "reflectAttributeValues",
        "(Lhla/rti1516_2025/ObjectInstanceHandle;Lhla/rti1516_2025/AttributeHandleValueMap;[BLhla/rti1516_2025/TransportationTypeHandle;Lhla/rti1516_2025/FederateHandle;Lhla/rti1516_2025/RegionHandleSet;)V");
    auto const reflectAttributeValuesWithTime = requiredMethod(
        environment,
        targetClass,
        "reflectAttributeValues",
        "(Lhla/rti1516_2025/ObjectInstanceHandle;Lhla/rti1516_2025/AttributeHandleValueMap;[BLhla/rti1516_2025/TransportationTypeHandle;Lhla/rti1516_2025/FederateHandle;Lhla/rti1516_2025/RegionHandleSet;Lhla/rti1516_2025/time/LogicalTime;Lhla/rti1516_2025/OrderType;Lhla/rti1516_2025/OrderType;Lhla/rti1516_2025/MessageRetractionHandle;)V");
    auto const provideAttributeValueUpdate = requiredMethod(
        environment,
        targetClass,
        "provideAttributeValueUpdate",
        "(Lhla/rti1516_2025/ObjectInstanceHandle;Lhla/rti1516_2025/AttributeHandleSet;[B)V");
    auto const reportAttributeTransportationType = requiredMethod(
        environment,
        targetClass,
        "reportAttributeTransportationType",
        "(Lhla/rti1516_2025/ObjectInstanceHandle;Lhla/rti1516_2025/AttributeHandle;Lhla/rti1516_2025/TransportationTypeHandle;)V");
    auto const reportInteractionTransportationType = requiredMethod(
        environment,
        targetClass,
        "reportInteractionTransportationType",
        "(Lhla/rti1516_2025/FederateHandle;Lhla/rti1516_2025/InteractionClassHandle;Lhla/rti1516_2025/TransportationTypeHandle;)V");
    auto const confirmAttributeTransportationTypeChange = requiredMethod(
        environment,
        targetClass,
        "confirmAttributeTransportationTypeChange",
        "(Lhla/rti1516_2025/ObjectInstanceHandle;Lhla/rti1516_2025/AttributeHandleSet;Lhla/rti1516_2025/TransportationTypeHandle;)V");
    auto const confirmInteractionTransportationTypeChange = requiredMethod(
        environment,
        targetClass,
        "confirmInteractionTransportationTypeChange",
        "(Lhla/rti1516_2025/InteractionClassHandle;Lhla/rti1516_2025/TransportationTypeHandle;)V");
    auto const attributesInScope = requiredMethod(
        environment,
        targetClass,
        "attributesInScope",
        "(Lhla/rti1516_2025/ObjectInstanceHandle;Lhla/rti1516_2025/AttributeHandleSet;)V");
    auto const attributesOutOfScope = requiredMethod(
        environment,
        targetClass,
        "attributesOutOfScope",
        "(Lhla/rti1516_2025/ObjectInstanceHandle;Lhla/rti1516_2025/AttributeHandleSet;)V");
    auto const timeAdvanceGrant = requiredMethod(
        environment,
        targetClass,
        "timeAdvanceGrant",
        "(Lhla/rti1516_2025/time/LogicalTime;)V");
    auto const flushQueueGrant = requiredMethod(
        environment,
        targetClass,
        "flushQueueGrant",
        "(Lhla/rti1516_2025/time/LogicalTime;Lhla/rti1516_2025/time/LogicalTime;)V");
    auto const timeConstrainedEnabled = requiredMethod(
        environment,
        targetClass,
        "timeConstrainedEnabled",
        "(Lhla/rti1516_2025/time/LogicalTime;)V");
    auto const timeRegulationEnabled = requiredMethod(
        environment,
        targetClass,
        "timeRegulationEnabled",
        "(Lhla/rti1516_2025/time/LogicalTime;)V");
    auto const requestRetraction = requiredMethod(
        environment,
        targetClass,
        "requestRetraction",
        "(Lhla/rti1516_2025/MessageRetractionHandle;)V");
    auto const removeObjectInstance = requiredMethod(
        environment,
        targetClass,
        "removeObjectInstance",
        "(Lhla/rti1516_2025/ObjectInstanceHandle;[BLhla/rti1516_2025/FederateHandle;)V");
    auto const removeObjectInstanceWithTime = requiredMethod(
        environment,
        targetClass,
        "removeObjectInstance",
        "(Lhla/rti1516_2025/ObjectInstanceHandle;[BLhla/rti1516_2025/FederateHandle;Lhla/rti1516_2025/time/LogicalTime;Lhla/rti1516_2025/OrderType;Lhla/rti1516_2025/OrderType;Lhla/rti1516_2025/MessageRetractionHandle;)V");
    auto const objectInstanceNameReservationSucceeded = requiredMethod(
        environment,
        targetClass,
        "objectInstanceNameReservationSucceeded",
        "(Ljava/lang/String;)V");
    auto const objectInstanceNameReservationFailed = requiredMethod(
        environment,
        targetClass,
        "objectInstanceNameReservationFailed",
        "(Ljava/lang/String;)V");
    auto const multipleObjectInstanceNameReservationSucceeded = requiredMethod(
        environment,
        targetClass,
        "multipleObjectInstanceNameReservationSucceeded",
        "(Ljava/util/Set;)V");
    auto const multipleObjectInstanceNameReservationFailed = requiredMethod(
        environment,
        targetClass,
        "multipleObjectInstanceNameReservationFailed",
        "(Ljava/util/Set;)V");
    environment->DeleteLocalRef(targetClass);

    jobject retained = environment->NewGlobalRef(target);
    if (retained == nullptr) {
      clearJavaException(environment);
      throw rti::FederateInternalError(L"Umbra could not retain the Java callback target.");
    }

    jobject previous = nullptr;
    {
      std::scoped_lock lock(mutex_);
      previous = target_;
      target_ = retained;
      connectionLost_ = connectionLost;
      reportExecutions_ = reportExecutions;
      reportMembers_ = reportMembers;
      missingFederation_ = missingFederation;
      federateResigned_ = federateResigned;
      synchronizationRegistrationSucceeded_ = synchronizationRegistrationSucceeded;
      synchronizationRegistrationFailed_ = synchronizationRegistrationFailed;
      announceSynchronizationPoint_ = announceSynchronizationPoint;
      federationSynchronized_ = federationSynchronized;
      federationSaveStatusResponse_ = federationSaveStatusResponse;
      initiateFederateSave_ = initiateFederateSave;
      initiateFederateSaveWithTime_ = initiateFederateSaveWithTime;
      federationSaved_ = federationSaved;
      federationNotSaved_ = federationNotSaved;
      federationRestoreStatusResponse_ = federationRestoreStatusResponse;
      requestFederationRestoreSucceeded_ = requestFederationRestoreSucceeded;
      requestFederationRestoreFailed_ = requestFederationRestoreFailed;
      federationRestoreBegun_ = federationRestoreBegun;
      initiateFederateRestore_ = initiateFederateRestore;
      federationRestored_ = federationRestored;
      federationNotRestored_ = federationNotRestored;
      startRegistrationForObjectClass_ = startRegistrationForObjectClass;
      stopRegistrationForObjectClass_ = stopRegistrationForObjectClass;
      turnInteractionsOn_ = turnInteractionsOn;
      turnInteractionsOff_ = turnInteractionsOff;
      turnUpdatesOnForObjectInstance_ = turnUpdatesOnForObjectInstance;
      turnUpdatesOnForObjectInstanceWithRate_ = turnUpdatesOnForObjectInstanceWithRate;
      turnUpdatesOffForObjectInstance_ = turnUpdatesOffForObjectInstance;
      receiveInteraction_ = receiveInteraction;
      receiveInteractionWithTime_ = receiveInteractionWithTime;
      receiveDirectedInteraction_ = receiveDirectedInteraction;
      receiveDirectedInteractionWithTime_ = receiveDirectedInteractionWithTime;
      requestAttributeOwnershipAssumption_ = requestAttributeOwnershipAssumption;
      requestDivestitureConfirmation_ = requestDivestitureConfirmation;
      attributeOwnershipAcquisitionNotification_ = attributeOwnershipAcquisitionNotification;
      attributeOwnershipUnavailable_ = attributeOwnershipUnavailable;
      requestAttributeOwnershipRelease_ = requestAttributeOwnershipRelease;
      confirmAttributeOwnershipAcquisitionCancellation_ =
          confirmAttributeOwnershipAcquisitionCancellation;
      informAttributeOwnership_ = informAttributeOwnership;
      attributeIsNotOwned_ = attributeIsNotOwned;
      attributeIsOwnedByRTI_ = attributeIsOwnedByRTI;
      discoverObjectInstance_ = discoverObjectInstance;
      reflectAttributeValues_ = reflectAttributeValues;
      reflectAttributeValuesWithTime_ = reflectAttributeValuesWithTime;
      provideAttributeValueUpdate_ = provideAttributeValueUpdate;
      reportAttributeTransportationType_ = reportAttributeTransportationType;
      reportInteractionTransportationType_ = reportInteractionTransportationType;
      confirmAttributeTransportationTypeChange_ = confirmAttributeTransportationTypeChange;
      confirmInteractionTransportationTypeChange_ = confirmInteractionTransportationTypeChange;
      attributesInScope_ = attributesInScope;
      attributesOutOfScope_ = attributesOutOfScope;
      timeAdvanceGrant_ = timeAdvanceGrant;
      flushQueueGrant_ = flushQueueGrant;
      timeConstrainedEnabled_ = timeConstrainedEnabled;
      timeRegulationEnabled_ = timeRegulationEnabled;
      requestRetraction_ = requestRetraction;
      removeObjectInstance_ = removeObjectInstance;
      removeObjectInstanceWithTime_ = removeObjectInstanceWithTime;
      objectInstanceNameReservationSucceeded_ = objectInstanceNameReservationSucceeded;
      objectInstanceNameReservationFailed_ = objectInstanceNameReservationFailed;
      multipleObjectInstanceNameReservationSucceeded_ = multipleObjectInstanceNameReservationSucceeded;
      multipleObjectInstanceNameReservationFailed_ = multipleObjectInstanceNameReservationFailed;
    }
    if (previous != nullptr) {
      environment->DeleteGlobalRef(previous);
    }
  }

  void clearTarget(JNIEnv* environment) {
    jobject previous = nullptr;
    {
      std::scoped_lock lock(mutex_);
      previous = target_;
      target_ = nullptr;
      connectionLost_ = nullptr;
      reportExecutions_ = nullptr;
      reportMembers_ = nullptr;
      missingFederation_ = nullptr;
      federateResigned_ = nullptr;
      synchronizationRegistrationSucceeded_ = nullptr;
      synchronizationRegistrationFailed_ = nullptr;
      announceSynchronizationPoint_ = nullptr;
      federationSynchronized_ = nullptr;
      federationSaveStatusResponse_ = nullptr;
      initiateFederateSave_ = nullptr;
      initiateFederateSaveWithTime_ = nullptr;
      federationSaved_ = nullptr;
      federationNotSaved_ = nullptr;
      federationRestoreStatusResponse_ = nullptr;
      requestFederationRestoreSucceeded_ = nullptr;
      requestFederationRestoreFailed_ = nullptr;
      federationRestoreBegun_ = nullptr;
      initiateFederateRestore_ = nullptr;
      federationRestored_ = nullptr;
      federationNotRestored_ = nullptr;
      startRegistrationForObjectClass_ = nullptr;
      stopRegistrationForObjectClass_ = nullptr;
      turnInteractionsOn_ = nullptr;
      turnInteractionsOff_ = nullptr;
      turnUpdatesOnForObjectInstance_ = nullptr;
      turnUpdatesOnForObjectInstanceWithRate_ = nullptr;
      turnUpdatesOffForObjectInstance_ = nullptr;
      receiveInteraction_ = nullptr;
      receiveInteractionWithTime_ = nullptr;
      receiveDirectedInteraction_ = nullptr;
      receiveDirectedInteractionWithTime_ = nullptr;
      requestAttributeOwnershipAssumption_ = nullptr;
      requestDivestitureConfirmation_ = nullptr;
      attributeOwnershipAcquisitionNotification_ = nullptr;
      attributeOwnershipUnavailable_ = nullptr;
      requestAttributeOwnershipRelease_ = nullptr;
      confirmAttributeOwnershipAcquisitionCancellation_ = nullptr;
      informAttributeOwnership_ = nullptr;
      attributeIsNotOwned_ = nullptr;
      attributeIsOwnedByRTI_ = nullptr;
      discoverObjectInstance_ = nullptr;
      reflectAttributeValues_ = nullptr;
      reflectAttributeValuesWithTime_ = nullptr;
      provideAttributeValueUpdate_ = nullptr;
      reportAttributeTransportationType_ = nullptr;
      reportInteractionTransportationType_ = nullptr;
      confirmAttributeTransportationTypeChange_ = nullptr;
      confirmInteractionTransportationTypeChange_ = nullptr;
      attributesInScope_ = nullptr;
      attributesOutOfScope_ = nullptr;
      timeAdvanceGrant_ = nullptr;
      flushQueueGrant_ = nullptr;
      timeConstrainedEnabled_ = nullptr;
      timeRegulationEnabled_ = nullptr;
      requestRetraction_ = nullptr;
      removeObjectInstance_ = nullptr;
      removeObjectInstanceWithTime_ = nullptr;
      objectInstanceNameReservationSucceeded_ = nullptr;
      objectInstanceNameReservationFailed_ = nullptr;
      multipleObjectInstanceNameReservationSucceeded_ = nullptr;
      multipleObjectInstanceNameReservationFailed_ = nullptr;
    }
    if (previous != nullptr) {
      environment->DeleteGlobalRef(previous);
    }
  }

  void dispose(JNIEnv* environment) {
    clearTarget(environment);
    for (jclass klass : {
             executionInformationSetClass_,
             executionInformationClass_,
             memberInformationSetClass_,
             memberInformationClass_,
             federateHandleClass_,
             federateHandleSetClass_,
             objectInstanceHandleClass_,
             objectClassHandleClass_,
             attributeHandleClass_,
             attributeHandleSetClass_,
             regionHandleClass_,
             regionHandleSetClass_,
             attributeHandleValueMapClass_,
             objectInstanceNameSetClass_,
             interactionClassHandleClass_,
             parameterHandleClass_,
             parameterHandleValueMapClass_,
             transportationTypeHandleClass_,
             saveStatusClass_,
             saveFailureReasonClass_,
             synchronizationPointFailureReasonClass_,
             restoreStatusClass_,
             restoreFailureReasonClass_,
             federateHandleSaveStatusPairClass_,
             federateRestoreStatusClass_}) {
      if (klass != nullptr) {
        environment->DeleteGlobalRef(klass);
      }
    }
    executionInformationSetClass_ = nullptr;
    executionInformationClass_ = nullptr;
    memberInformationSetClass_ = nullptr;
    memberInformationClass_ = nullptr;
    federateHandleClass_ = nullptr;
    federateHandleSetClass_ = nullptr;
    objectInstanceHandleClass_ = nullptr;
    objectClassHandleClass_ = nullptr;
    attributeHandleClass_ = nullptr;
    attributeHandleSetClass_ = nullptr;
    regionHandleClass_ = nullptr;
    regionHandleSetClass_ = nullptr;
    attributeHandleValueMapClass_ = nullptr;
    objectInstanceNameSetClass_ = nullptr;
    interactionClassHandleClass_ = nullptr;
    parameterHandleClass_ = nullptr;
    parameterHandleValueMapClass_ = nullptr;
    transportationTypeHandleClass_ = nullptr;
    saveStatusClass_ = nullptr;
    saveFailureReasonClass_ = nullptr;
    synchronizationPointFailureReasonClass_ = nullptr;
    restoreStatusClass_ = nullptr;
    restoreFailureReasonClass_ = nullptr;
    federateHandleSaveStatusPairClass_ = nullptr;
    federateRestoreStatusClass_ = nullptr;
  }

  void connectionLost(std::wstring const& faultDescription) override {
    invokeString(connectionLost_, faultDescription);
  }

  void reportFederationExecutions(
      rti::FederationExecutionInformationVector const& report) override {
    ScopedJNIEnvironment attachment(virtualMachine_);
    JNIEnv* environment = attachment.get();
    jmethodID method = nullptr;
    jobject target = retainedTarget(environment, method, reportExecutions_);
    if (target == nullptr || method == nullptr) {
      return;
    }

    jobject result = environment->NewObject(
        executionInformationSetClass_, executionInformationSetConstructor_);
    if (result == nullptr) {
      environment->DeleteLocalRef(target);
      clearJavaException(environment);
      throw rti::FederateInternalError(L"Umbra could not create a Java federation report set.");
    }
    try {
      for (auto const& information : report) {
        jstring name = javaString(environment, information.federationExecutionName);
        jstring logicalTime = javaString(environment, information.logicalTimeImplementationName);
        jobject entry = environment->NewObject(
            executionInformationClass_, executionInformationConstructor_, name, logicalTime);
        if (name != nullptr) {
          environment->DeleteLocalRef(name);
        }
        if (logicalTime != nullptr) {
          environment->DeleteLocalRef(logicalTime);
        }
        if (entry == nullptr) {
          clearJavaException(environment);
          throw rti::FederateInternalError(L"Umbra could not create a Java federation report entry.");
        }
        environment->CallBooleanMethod(result, setAdd_, entry);
        environment->DeleteLocalRef(entry);
        ensureNoJavaException(environment);
      }
      environment->CallVoidMethod(target, method, result);
      ensureNoJavaException(environment);
    } catch (...) {
      environment->DeleteLocalRef(result);
      environment->DeleteLocalRef(target);
      throw;
    }
    environment->DeleteLocalRef(result);
    environment->DeleteLocalRef(target);
  }

  void reportFederationExecutionMembers(
      std::wstring const& federationName,
      rti::FederationExecutionMemberInformationVector const& report) override {
    ScopedJNIEnvironment attachment(virtualMachine_);
    JNIEnv* environment = attachment.get();
    jmethodID method = nullptr;
    jobject target = retainedTarget(environment, method, reportMembers_);
    if (target == nullptr || method == nullptr) {
      return;
    }

    jobject result = environment->NewObject(
        memberInformationSetClass_, memberInformationSetConstructor_);
    jstring name = javaString(environment, federationName);
    if (result == nullptr || name == nullptr) {
      if (result != nullptr) {
        environment->DeleteLocalRef(result);
      }
      if (name != nullptr) {
        environment->DeleteLocalRef(name);
      }
      environment->DeleteLocalRef(target);
      clearJavaException(environment);
      throw rti::FederateInternalError(L"Umbra could not create a Java federation member report.");
    }
    try {
      for (auto const& information : report) {
        jstring federateName = javaString(environment, information.federateName);
        jstring federateType = javaString(environment, information.federateType);
        jobject entry = environment->NewObject(
            memberInformationClass_, memberInformationConstructor_, federateName, federateType);
        if (federateName != nullptr) {
          environment->DeleteLocalRef(federateName);
        }
        if (federateType != nullptr) {
          environment->DeleteLocalRef(federateType);
        }
        if (entry == nullptr) {
          clearJavaException(environment);
          throw rti::FederateInternalError(L"Umbra could not create a Java federation member entry.");
        }
        environment->CallBooleanMethod(result, memberSetAdd_, entry);
        environment->DeleteLocalRef(entry);
        ensureNoJavaException(environment);
      }
      environment->CallVoidMethod(target, method, name, result);
      ensureNoJavaException(environment);
    } catch (...) {
      environment->DeleteLocalRef(name);
      environment->DeleteLocalRef(result);
      environment->DeleteLocalRef(target);
      throw;
    }
    environment->DeleteLocalRef(name);
    environment->DeleteLocalRef(result);
    environment->DeleteLocalRef(target);
  }

  void reportFederationExecutionDoesNotExist(
      std::wstring const& federationName) override {
    invokeString(missingFederation_, federationName);
  }

  void federateResigned(std::wstring const& reasonForResignDescription) override {
    invokeString(federateResigned_, reasonForResignDescription);
  }

  void startRegistrationForObjectClass(
      rti::ObjectClassHandle const& objectClass) override {
    invokeObjectClass(startRegistrationForObjectClass_, objectClass);
  }

  void stopRegistrationForObjectClass(
      rti::ObjectClassHandle const& objectClass) override {
    invokeObjectClass(stopRegistrationForObjectClass_, objectClass);
  }

  void turnInteractionsOn(
      rti::InteractionClassHandle const& interactionClass) override {
    invokeInteractionClass(turnInteractionsOn_, interactionClass);
  }

  void turnInteractionsOff(
      rti::InteractionClassHandle const& interactionClass) override {
    invokeInteractionClass(turnInteractionsOff_, interactionClass);
  }

  void synchronizationPointRegistrationSucceeded(
      std::wstring const& synchronizationPointLabel) override {
    invokeString(synchronizationRegistrationSucceeded_, synchronizationPointLabel);
  }

  void synchronizationPointRegistrationFailed(
      std::wstring const& synchronizationPointLabel,
      rti::SynchronizationPointFailureReason reason) override {
    ScopedJNIEnvironment attachment(virtualMachine_);
    JNIEnv* environment = attachment.get();
    jmethodID method = nullptr;
    jobject target = retainedTarget(environment, method, synchronizationRegistrationFailed_);
    if (target == nullptr || method == nullptr) return;
    jstring label = javaString(environment, synchronizationPointLabel);
    jstring reasonName = javaString(
        environment, synchronizationPointFailureReasonName(reason));
    jobject javaReason = reasonName == nullptr
        ? nullptr
        : environment->CallStaticObjectMethod(
              synchronizationPointFailureReasonClass_,
              synchronizationPointFailureReasonValueOf_,
              reasonName);
    if (label == nullptr || javaReason == nullptr) {
      if (label != nullptr) environment->DeleteLocalRef(label);
      if (reasonName != nullptr) environment->DeleteLocalRef(reasonName);
      if (javaReason != nullptr) environment->DeleteLocalRef(javaReason);
      environment->DeleteLocalRef(target);
      clearJavaException(environment);
      throw rti::FederateInternalError(
          L"Umbra could not create a Java synchronization registration failure.");
    }
    environment->CallVoidMethod(target, method, label, javaReason);
    environment->DeleteLocalRef(label);
    environment->DeleteLocalRef(reasonName);
    environment->DeleteLocalRef(javaReason);
    environment->DeleteLocalRef(target);
    ensureNoJavaException(environment);
  }

  void announceSynchronizationPoint(
      std::wstring const& synchronizationPointLabel,
      rti::VariableLengthData const& userSuppliedTag) override {
    ScopedJNIEnvironment attachment(virtualMachine_);
    JNIEnv* environment = attachment.get();
    jmethodID method = nullptr;
    jobject target = retainedTarget(environment, method, announceSynchronizationPoint_);
    if (target == nullptr || method == nullptr) return;
    jstring label = javaString(environment, synchronizationPointLabel);
    jbyteArray tag = javaByteArray(environment, userSuppliedTag);
    if (label == nullptr || tag == nullptr) {
      if (label != nullptr) environment->DeleteLocalRef(label);
      if (tag != nullptr) environment->DeleteLocalRef(tag);
      environment->DeleteLocalRef(target);
      clearJavaException(environment);
      throw rti::FederateInternalError(L"Umbra could not create a Java synchronization announcement.");
    }
    environment->CallVoidMethod(target, method, label, tag);
    environment->DeleteLocalRef(label);
    environment->DeleteLocalRef(tag);
    environment->DeleteLocalRef(target);
    ensureNoJavaException(environment);
  }

  void federationSynchronized(
      std::wstring const& synchronizationPointLabel,
      rti::FederateHandleSet const& failedToSyncSet) override {
    ScopedJNIEnvironment attachment(virtualMachine_);
    JNIEnv* environment = attachment.get();
    jmethodID method = nullptr;
    jobject target = retainedTarget(environment, method, federationSynchronized_);
    if (target == nullptr || method == nullptr) return;
    jstring label = javaString(environment, synchronizationPointLabel);
    jobject failures = javaFederateHandleSet(environment, failedToSyncSet);
    if (label == nullptr || failures == nullptr) {
      if (label != nullptr) environment->DeleteLocalRef(label);
      if (failures != nullptr) environment->DeleteLocalRef(failures);
      environment->DeleteLocalRef(target);
      clearJavaException(environment);
      throw rti::FederateInternalError(L"Umbra could not create a Java synchronization result.");
    }
    environment->CallVoidMethod(target, method, label, failures);
    environment->DeleteLocalRef(label);
    environment->DeleteLocalRef(failures);
    environment->DeleteLocalRef(target);
    ensureNoJavaException(environment);
  }

  void federationSaveStatusResponse(
      rti::FederateHandleSaveStatusPairVector const& response) override {
    ScopedJNIEnvironment attachment(virtualMachine_);
    JNIEnv* environment = attachment.get();
    jmethodID method = nullptr;
    jobject target = retainedTarget(environment, method, federationSaveStatusResponse_);
    if (target == nullptr || method == nullptr) return;
    jobjectArray result = environment->NewObjectArray(
        static_cast<jsize>(response.size()), federateHandleSaveStatusPairClass_, nullptr);
    if (result == nullptr) {
      environment->DeleteLocalRef(target);
      clearJavaException(environment);
      throw rti::FederateInternalError(L"Umbra could not create a Java save-status response.");
    }
    try {
      for (jsize index = 0; index < static_cast<jsize>(response.size()); ++index) {
        auto const& [federate, status] = response[static_cast<std::size_t>(index)];
        jbyteArray encoded = javaByteArray(environment, federate.encode());
        jobject handle = environment->NewObject(
            federateHandleClass_, federateHandleConstructor_, encoded);
        if (encoded != nullptr) environment->DeleteLocalRef(encoded);
        jstring statusName = javaString(environment, saveStatusName(status));
        jobject javaStatus = environment->CallStaticObjectMethod(
            saveStatusClass_, saveStatusValueOf_, statusName);
        if (statusName != nullptr) environment->DeleteLocalRef(statusName);
        if (handle == nullptr || javaStatus == nullptr) {
          if (handle != nullptr) environment->DeleteLocalRef(handle);
          if (javaStatus != nullptr) environment->DeleteLocalRef(javaStatus);
          clearJavaException(environment);
          throw rti::FederateInternalError(L"Umbra could not create a Java save-status record.");
        }
        jobject pair = environment->NewObject(
            federateHandleSaveStatusPairClass_,
            federateHandleSaveStatusPairConstructor_,
            handle,
            javaStatus);
        environment->DeleteLocalRef(handle);
        environment->DeleteLocalRef(javaStatus);
        if (pair == nullptr) {
          clearJavaException(environment);
          throw rti::FederateInternalError(L"Umbra could not create a Java save-status pair.");
        }
        environment->SetObjectArrayElement(result, index, pair);
        environment->DeleteLocalRef(pair);
        ensureNoJavaException(environment);
      }
      environment->CallVoidMethod(target, method, result);
      ensureNoJavaException(environment);
    } catch (...) {
      environment->DeleteLocalRef(result);
      environment->DeleteLocalRef(target);
      throw;
    }
    environment->DeleteLocalRef(result);
    environment->DeleteLocalRef(target);
  }

  void initiateFederateSave(std::wstring const& label) override {
    invokeString(initiateFederateSave_, label);
  }

  void initiateFederateSave(
      std::wstring const& label,
      rti::LogicalTime const& time) override {
    ScopedJNIEnvironment attachment(virtualMachine_);
    JNIEnv* environment = attachment.get();
    jmethodID method = nullptr;
    jobject target = retainedTarget(environment, method, initiateFederateSaveWithTime_);
    if (target == nullptr || method == nullptr) return;
    jstring javaLabel = javaString(environment, label);
    jobject javaTime = javaLogicalTime(environment, time);
    if (javaLabel == nullptr || javaTime == nullptr) {
      if (javaLabel != nullptr) environment->DeleteLocalRef(javaLabel);
      if (javaTime != nullptr) environment->DeleteLocalRef(javaTime);
      environment->DeleteLocalRef(target);
      clearJavaException(environment);
      throw rti::FederateInternalError(
          L"Umbra could not create a Java timestamped save callback.");
    }
    environment->CallVoidMethod(target, method, javaLabel, javaTime);
    environment->DeleteLocalRef(javaLabel);
    environment->DeleteLocalRef(javaTime);
    environment->DeleteLocalRef(target);
    ensureNoJavaException(environment);
  }

  void federationSaved() override { invokeVoid(federationSaved_); }

  void federationNotSaved(rti::SaveFailureReason reason) override {
    ScopedJNIEnvironment attachment(virtualMachine_);
    JNIEnv* environment = attachment.get();
    jmethodID method = nullptr;
    jobject target = retainedTarget(environment, method, federationNotSaved_);
    if (target == nullptr || method == nullptr) return;
    jstring reasonName = javaString(environment, saveFailureReasonName(reason));
    jobject javaReason = environment->CallStaticObjectMethod(
        saveFailureReasonClass_, saveFailureReasonValueOf_, reasonName);
    if (reasonName != nullptr) environment->DeleteLocalRef(reasonName);
    if (javaReason == nullptr) {
      environment->DeleteLocalRef(target);
      clearJavaException(environment);
      throw rti::FederateInternalError(L"Umbra could not create a Java save-failure reason.");
    }
    environment->CallVoidMethod(target, method, javaReason);
    environment->DeleteLocalRef(javaReason);
    environment->DeleteLocalRef(target);
    ensureNoJavaException(environment);
  }

  void federationRestoreStatusResponse(
      rti::FederateRestoreStatusVector const& response) override {
    ScopedJNIEnvironment attachment(virtualMachine_);
    JNIEnv* environment = attachment.get();
    jmethodID method = nullptr;
    jobject target = retainedTarget(environment, method, federationRestoreStatusResponse_);
    if (target == nullptr || method == nullptr) return;
    jobjectArray result = environment->NewObjectArray(
        static_cast<jsize>(response.size()), federateRestoreStatusClass_, nullptr);
    if (result == nullptr) {
      environment->DeleteLocalRef(target);
      clearJavaException(environment);
      throw rti::FederateInternalError(L"Umbra could not create a Java restore-status response.");
    }
    try {
      for (jsize index = 0; index < static_cast<jsize>(response.size()); ++index) {
        auto const& item = response[static_cast<std::size_t>(index)];
        jobject pre = javaFederateHandle(environment, item.preRestoreHandle);
        jobject post = javaFederateHandle(environment, item.postRestoreHandle);
        jstring statusName = javaString(environment, restoreStatusName(item.status));
        jobject status = environment->CallStaticObjectMethod(
            restoreStatusClass_, restoreStatusValueOf_, statusName);
        if (statusName != nullptr) environment->DeleteLocalRef(statusName);
        if (pre == nullptr || post == nullptr || status == nullptr) {
          if (pre != nullptr) environment->DeleteLocalRef(pre);
          if (post != nullptr) environment->DeleteLocalRef(post);
          if (status != nullptr) environment->DeleteLocalRef(status);
          clearJavaException(environment);
          throw rti::FederateInternalError(L"Umbra could not create a Java restore-status record.");
        }
        jobject entry = environment->NewObject(
            federateRestoreStatusClass_, federateRestoreStatusConstructor_, pre, post, status);
        environment->DeleteLocalRef(pre);
        environment->DeleteLocalRef(post);
        environment->DeleteLocalRef(status);
        if (entry == nullptr) {
          clearJavaException(environment);
          throw rti::FederateInternalError(L"Umbra could not create a Java restore-status entry.");
        }
        environment->SetObjectArrayElement(result, index, entry);
        environment->DeleteLocalRef(entry);
        ensureNoJavaException(environment);
      }
      environment->CallVoidMethod(target, method, result);
      ensureNoJavaException(environment);
    } catch (...) {
      environment->DeleteLocalRef(result);
      environment->DeleteLocalRef(target);
      throw;
    }
    environment->DeleteLocalRef(result);
    environment->DeleteLocalRef(target);
  }

  void requestFederationRestoreSucceeded(std::wstring const& label) override {
    invokeString(requestFederationRestoreSucceeded_, label);
  }

  void requestFederationRestoreFailed(std::wstring const& label) override {
    invokeString(requestFederationRestoreFailed_, label);
  }

  void federationRestoreBegun() override { invokeVoid(federationRestoreBegun_); }

  void initiateFederateRestore(
      std::wstring const& label,
      std::wstring const& federateName,
      rti::FederateHandle const& postRestoreFederateHandle) override {
    ScopedJNIEnvironment attachment(virtualMachine_);
    JNIEnv* environment = attachment.get();
    jmethodID method = nullptr;
    jobject target = retainedTarget(environment, method, initiateFederateRestore_);
    if (target == nullptr || method == nullptr) return;
    jstring javaLabel = javaString(environment, label);
    jstring javaFederateName = javaString(environment, federateName);
    jobject javaHandle = javaFederateHandle(environment, postRestoreFederateHandle);
    if (javaLabel == nullptr || javaFederateName == nullptr || javaHandle == nullptr) {
      if (javaLabel != nullptr) environment->DeleteLocalRef(javaLabel);
      if (javaFederateName != nullptr) environment->DeleteLocalRef(javaFederateName);
      if (javaHandle != nullptr) environment->DeleteLocalRef(javaHandle);
      environment->DeleteLocalRef(target);
      clearJavaException(environment);
      throw rti::FederateInternalError(L"Umbra could not create a Java restore initiation.");
    }
    environment->CallVoidMethod(target, method, javaLabel, javaFederateName, javaHandle);
    environment->DeleteLocalRef(javaLabel);
    environment->DeleteLocalRef(javaFederateName);
    environment->DeleteLocalRef(javaHandle);
    environment->DeleteLocalRef(target);
    ensureNoJavaException(environment);
  }

  void federationRestored() override { invokeVoid(federationRestored_); }

  void federationNotRestored(rti::RestoreFailureReason reason) override {
    ScopedJNIEnvironment attachment(virtualMachine_);
    JNIEnv* environment = attachment.get();
    jmethodID method = nullptr;
    jobject target = retainedTarget(environment, method, federationNotRestored_);
    if (target == nullptr || method == nullptr) return;
    jstring reasonName = javaString(environment, restoreFailureReasonName(reason));
    jobject javaReason = environment->CallStaticObjectMethod(
        restoreFailureReasonClass_, restoreFailureReasonValueOf_, reasonName);
    if (reasonName != nullptr) environment->DeleteLocalRef(reasonName);
    if (javaReason == nullptr) {
      environment->DeleteLocalRef(target);
      clearJavaException(environment);
      throw rti::FederateInternalError(L"Umbra could not create a Java restore-failure reason.");
    }
    environment->CallVoidMethod(target, method, javaReason);
    environment->DeleteLocalRef(javaReason);
    environment->DeleteLocalRef(target);
    ensureNoJavaException(environment);
  }

  void receiveInteraction(
      rti::InteractionClassHandle const& interactionClass,
      rti::ParameterHandleValueMap const& parameterValues,
      rti::VariableLengthData const& userSuppliedTag,
      rti::TransportationTypeHandle const& transportationType,
      rti::FederateHandle const& producingFederate,
      rti::RegionHandleSet const* optionalSentRegions) override {
    ScopedJNIEnvironment attachment(virtualMachine_);
    JNIEnv* environment = attachment.get();
    jmethodID method = nullptr;
    jobject target = retainedTarget(environment, method, receiveInteraction_);
    if (target == nullptr || method == nullptr) return;
    jobject javaInteraction = javaInteractionClassHandle(environment, interactionClass);
    jobject javaParameters = javaParameterHandleValueMap(environment, parameterValues);
    jbyteArray javaTag = javaByteArray(environment, userSuppliedTag);
    jobject javaTransportation = javaTransportationTypeHandle(environment, transportationType);
    jobject javaProducingFederate = javaFederateHandle(environment, producingFederate);
    jobject javaRegions = optionalSentRegions == nullptr
        ? nullptr
        : javaRegionHandleSet(environment, *optionalSentRegions);
    if (javaInteraction == nullptr || javaParameters == nullptr || javaTag == nullptr ||
        javaTransportation == nullptr || javaProducingFederate == nullptr ||
        (optionalSentRegions != nullptr && javaRegions == nullptr)) {
      if (javaInteraction != nullptr) environment->DeleteLocalRef(javaInteraction);
      if (javaParameters != nullptr) environment->DeleteLocalRef(javaParameters);
      if (javaTag != nullptr) environment->DeleteLocalRef(javaTag);
      if (javaTransportation != nullptr) environment->DeleteLocalRef(javaTransportation);
      if (javaProducingFederate != nullptr) environment->DeleteLocalRef(javaProducingFederate);
      if (javaRegions != nullptr) environment->DeleteLocalRef(javaRegions);
      environment->DeleteLocalRef(target);
      clearJavaException(environment);
      throw rti::FederateInternalError(L"Umbra could not create a Java interaction callback.");
    }
    environment->CallVoidMethod(
        target,
        method,
        javaInteraction,
        javaParameters,
        javaTag,
        javaTransportation,
        javaProducingFederate,
        javaRegions);
    environment->DeleteLocalRef(javaInteraction);
    environment->DeleteLocalRef(javaParameters);
    environment->DeleteLocalRef(javaTag);
    environment->DeleteLocalRef(javaTransportation);
    environment->DeleteLocalRef(javaProducingFederate);
    if (javaRegions != nullptr) environment->DeleteLocalRef(javaRegions);
    environment->DeleteLocalRef(target);
    ensureNoJavaException(environment);
  }

  void receiveInteraction(
      rti::InteractionClassHandle const& interactionClass,
      rti::ParameterHandleValueMap const& parameterValues,
      rti::VariableLengthData const& userSuppliedTag,
      rti::TransportationTypeHandle const& transportationType,
      rti::FederateHandle const& producingFederate,
      rti::RegionHandleSet const* optionalSentRegions,
      rti::LogicalTime const& time,
      rti::OrderType sentOrderType,
      rti::OrderType receivedOrderType,
      rti::MessageRetractionHandle const* optionalRetraction) override {
    ScopedJNIEnvironment attachment(virtualMachine_);
    JNIEnv* environment = attachment.get();
    jmethodID method = nullptr;
    jobject target = retainedTarget(environment, method, receiveInteractionWithTime_);
    if (target == nullptr || method == nullptr) return;
    jobject javaInteraction = javaInteractionClassHandle(environment, interactionClass);
    jobject javaParameters = javaParameterHandleValueMap(environment, parameterValues);
    jbyteArray javaTag = javaByteArray(environment, userSuppliedTag);
    jobject javaTransportation = javaTransportationTypeHandle(environment, transportationType);
    jobject javaProducingFederate = javaFederateHandle(environment, producingFederate);
    jobject javaRegions = optionalSentRegions == nullptr
        ? nullptr
        : javaRegionHandleSet(environment, *optionalSentRegions);
    jobject javaTime = javaLogicalTime(environment, time);
    jobject javaSentOrder = javaOrderType(environment, sentOrderType);
    jobject javaReceivedOrder = javaOrderType(environment, receivedOrderType);
    jobject javaRetraction = optionalRetraction == nullptr
        ? nullptr
        : javaMessageRetractionHandle(environment, *optionalRetraction);
    if (javaInteraction == nullptr || javaParameters == nullptr || javaTag == nullptr ||
        javaTransportation == nullptr || javaProducingFederate == nullptr || javaTime == nullptr ||
        javaSentOrder == nullptr || javaReceivedOrder == nullptr ||
        (optionalSentRegions != nullptr && javaRegions == nullptr) ||
        (optionalRetraction != nullptr && javaRetraction == nullptr)) {
      if (javaInteraction != nullptr) environment->DeleteLocalRef(javaInteraction);
      if (javaParameters != nullptr) environment->DeleteLocalRef(javaParameters);
      if (javaTag != nullptr) environment->DeleteLocalRef(javaTag);
      if (javaTransportation != nullptr) environment->DeleteLocalRef(javaTransportation);
      if (javaProducingFederate != nullptr) environment->DeleteLocalRef(javaProducingFederate);
      if (javaRegions != nullptr) environment->DeleteLocalRef(javaRegions);
      if (javaTime != nullptr) environment->DeleteLocalRef(javaTime);
      if (javaSentOrder != nullptr) environment->DeleteLocalRef(javaSentOrder);
      if (javaReceivedOrder != nullptr) environment->DeleteLocalRef(javaReceivedOrder);
      if (javaRetraction != nullptr) environment->DeleteLocalRef(javaRetraction);
      environment->DeleteLocalRef(target);
      clearJavaException(environment);
      throw rti::FederateInternalError(L"Umbra could not create a Java timestamped interaction callback.");
    }
    environment->CallVoidMethod(
        target,
        method,
        javaInteraction,
        javaParameters,
        javaTag,
        javaTransportation,
        javaProducingFederate,
        javaRegions,
        javaTime,
        javaSentOrder,
        javaReceivedOrder,
        javaRetraction);
    environment->DeleteLocalRef(javaInteraction);
    environment->DeleteLocalRef(javaParameters);
    environment->DeleteLocalRef(javaTag);
    environment->DeleteLocalRef(javaTransportation);
    environment->DeleteLocalRef(javaProducingFederate);
    if (javaRegions != nullptr) environment->DeleteLocalRef(javaRegions);
    environment->DeleteLocalRef(javaTime);
    environment->DeleteLocalRef(javaSentOrder);
    environment->DeleteLocalRef(javaReceivedOrder);
    if (javaRetraction != nullptr) environment->DeleteLocalRef(javaRetraction);
    environment->DeleteLocalRef(target);
    ensureNoJavaException(environment);
  }

  void receiveDirectedInteraction(
      rti::InteractionClassHandle const& interactionClass,
      rti::ObjectInstanceHandle const& objectInstance,
      rti::ParameterHandleValueMap const& parameterValues,
      rti::VariableLengthData const& userSuppliedTag,
      rti::TransportationTypeHandle const& transportationType,
      rti::FederateHandle const& producingFederate) override {
    ScopedJNIEnvironment attachment(virtualMachine_);
    JNIEnv* environment = attachment.get();
    jmethodID method = nullptr;
    jobject target = retainedTarget(environment, method, receiveDirectedInteraction_);
    if (target == nullptr || method == nullptr) return;
    jobject javaInteraction = javaInteractionClassHandle(environment, interactionClass);
    jobject javaObjectInstance = javaObjectInstanceHandle(environment, objectInstance);
    jobject javaParameters = javaParameterHandleValueMap(environment, parameterValues);
    jbyteArray javaTag = javaByteArray(environment, userSuppliedTag);
    jobject javaTransportation = javaTransportationTypeHandle(environment, transportationType);
    jobject javaProducingFederate = javaFederateHandle(environment, producingFederate);
    if (javaInteraction == nullptr || javaObjectInstance == nullptr ||
        javaParameters == nullptr || javaTag == nullptr || javaTransportation == nullptr ||
        javaProducingFederate == nullptr) {
      if (javaInteraction != nullptr) environment->DeleteLocalRef(javaInteraction);
      if (javaObjectInstance != nullptr) environment->DeleteLocalRef(javaObjectInstance);
      if (javaParameters != nullptr) environment->DeleteLocalRef(javaParameters);
      if (javaTag != nullptr) environment->DeleteLocalRef(javaTag);
      if (javaTransportation != nullptr) environment->DeleteLocalRef(javaTransportation);
      if (javaProducingFederate != nullptr) environment->DeleteLocalRef(javaProducingFederate);
      environment->DeleteLocalRef(target);
      clearJavaException(environment);
      throw rti::FederateInternalError(
          L"Umbra could not create a Java directed-interaction callback.");
    }
    environment->CallVoidMethod(
        target,
        method,
        javaInteraction,
        javaObjectInstance,
        javaParameters,
        javaTag,
        javaTransportation,
        javaProducingFederate);
    environment->DeleteLocalRef(javaInteraction);
    environment->DeleteLocalRef(javaObjectInstance);
    environment->DeleteLocalRef(javaParameters);
    environment->DeleteLocalRef(javaTag);
    environment->DeleteLocalRef(javaTransportation);
    environment->DeleteLocalRef(javaProducingFederate);
    environment->DeleteLocalRef(target);
    ensureNoJavaException(environment);
  }

  void receiveDirectedInteraction(
      rti::InteractionClassHandle const& interactionClass,
      rti::ObjectInstanceHandle const& objectInstance,
      rti::ParameterHandleValueMap const& parameterValues,
      rti::VariableLengthData const& userSuppliedTag,
      rti::TransportationTypeHandle const& transportationType,
      rti::FederateHandle const& producingFederate,
      rti::LogicalTime const& time,
      rti::OrderType sentOrderType,
      rti::OrderType receivedOrderType,
      rti::MessageRetractionHandle const* optionalRetraction) override {
    ScopedJNIEnvironment attachment(virtualMachine_);
    JNIEnv* environment = attachment.get();
    jmethodID method = nullptr;
    jobject target = retainedTarget(environment, method, receiveDirectedInteractionWithTime_);
    if (target == nullptr || method == nullptr) return;
    jobject javaInteraction = javaInteractionClassHandle(environment, interactionClass);
    jobject javaObjectInstance = javaObjectInstanceHandle(environment, objectInstance);
    jobject javaParameters = javaParameterHandleValueMap(environment, parameterValues);
    jbyteArray javaTag = javaByteArray(environment, userSuppliedTag);
    jobject javaTransportation = javaTransportationTypeHandle(environment, transportationType);
    jobject javaProducingFederate = javaFederateHandle(environment, producingFederate);
    jobject javaTime = javaLogicalTime(environment, time);
    jobject javaSentOrder = javaOrderType(environment, sentOrderType);
    jobject javaReceivedOrder = javaOrderType(environment, receivedOrderType);
    jobject javaRetraction = optionalRetraction == nullptr
        ? nullptr
        : javaMessageRetractionHandle(environment, *optionalRetraction);
    if (javaInteraction == nullptr || javaObjectInstance == nullptr ||
        javaParameters == nullptr || javaTag == nullptr || javaTransportation == nullptr ||
        javaProducingFederate == nullptr || javaTime == nullptr || javaSentOrder == nullptr ||
        javaReceivedOrder == nullptr ||
        (optionalRetraction != nullptr && javaRetraction == nullptr)) {
      if (javaInteraction != nullptr) environment->DeleteLocalRef(javaInteraction);
      if (javaObjectInstance != nullptr) environment->DeleteLocalRef(javaObjectInstance);
      if (javaParameters != nullptr) environment->DeleteLocalRef(javaParameters);
      if (javaTag != nullptr) environment->DeleteLocalRef(javaTag);
      if (javaTransportation != nullptr) environment->DeleteLocalRef(javaTransportation);
      if (javaProducingFederate != nullptr) environment->DeleteLocalRef(javaProducingFederate);
      if (javaTime != nullptr) environment->DeleteLocalRef(javaTime);
      if (javaSentOrder != nullptr) environment->DeleteLocalRef(javaSentOrder);
      if (javaReceivedOrder != nullptr) environment->DeleteLocalRef(javaReceivedOrder);
      if (javaRetraction != nullptr) environment->DeleteLocalRef(javaRetraction);
      environment->DeleteLocalRef(target);
      clearJavaException(environment);
      throw rti::FederateInternalError(
          L"Umbra could not create a Java timestamped directed-interaction callback.");
    }
    environment->CallVoidMethod(
        target,
        method,
        javaInteraction,
        javaObjectInstance,
        javaParameters,
        javaTag,
        javaTransportation,
        javaProducingFederate,
        javaTime,
        javaSentOrder,
        javaReceivedOrder,
        javaRetraction);
    environment->DeleteLocalRef(javaInteraction);
    environment->DeleteLocalRef(javaObjectInstance);
    environment->DeleteLocalRef(javaParameters);
    environment->DeleteLocalRef(javaTag);
    environment->DeleteLocalRef(javaTransportation);
    environment->DeleteLocalRef(javaProducingFederate);
    environment->DeleteLocalRef(javaTime);
    environment->DeleteLocalRef(javaSentOrder);
    environment->DeleteLocalRef(javaReceivedOrder);
    if (javaRetraction != nullptr) environment->DeleteLocalRef(javaRetraction);
    environment->DeleteLocalRef(target);
    ensureNoJavaException(environment);
  }

  void requestAttributeOwnershipAssumption(
      rti::ObjectInstanceHandle const& objectInstance,
      rti::AttributeHandleSet const& offeredAttributes,
      rti::VariableLengthData const& userSuppliedTag) override {
    invokeOwnershipSetCallback(
        requestAttributeOwnershipAssumption_,
        objectInstance,
        offeredAttributes,
        &userSuppliedTag,
        nullptr);
  }

  void requestDivestitureConfirmation(
      rti::ObjectInstanceHandle const& objectInstance,
      rti::AttributeHandleSet const& releasedAttributes,
      rti::VariableLengthData const& userSuppliedTag) override {
    invokeOwnershipSetCallback(
        requestDivestitureConfirmation_,
        objectInstance,
        releasedAttributes,
        &userSuppliedTag,
        nullptr);
  }

  void attributeOwnershipAcquisitionNotification(
      rti::ObjectInstanceHandle const& objectInstance,
      rti::AttributeHandleSet const& securedAttributes,
      rti::VariableLengthData const& userSuppliedTag) override {
    invokeOwnershipSetCallback(
        attributeOwnershipAcquisitionNotification_,
        objectInstance,
        securedAttributes,
        &userSuppliedTag,
        nullptr);
  }

  void attributeOwnershipUnavailable(
      rti::ObjectInstanceHandle const& objectInstance,
      rti::AttributeHandleSet const& attributes,
      rti::VariableLengthData const& userSuppliedTag) override {
    invokeOwnershipSetCallback(
        attributeOwnershipUnavailable_, objectInstance, attributes, &userSuppliedTag, nullptr);
  }

  void requestAttributeOwnershipRelease(
      rti::ObjectInstanceHandle const& objectInstance,
      rti::AttributeHandleSet const& candidateAttributes,
      rti::VariableLengthData const& userSuppliedTag) override {
    invokeOwnershipSetCallback(
        requestAttributeOwnershipRelease_,
        objectInstance,
        candidateAttributes,
        &userSuppliedTag,
        nullptr);
  }

  void confirmAttributeOwnershipAcquisitionCancellation(
      rti::ObjectInstanceHandle const& objectInstance,
      rti::AttributeHandleSet const& attributes) override {
    invokeOwnershipSetCallback(
        confirmAttributeOwnershipAcquisitionCancellation_,
        objectInstance,
        attributes,
        nullptr,
        nullptr);
  }

  void informAttributeOwnership(
      rti::ObjectInstanceHandle const& objectInstance,
      rti::AttributeHandleSet const& attributes,
      rti::FederateHandle const& owner) override {
    invokeOwnershipSetCallback(
        informAttributeOwnership_, objectInstance, attributes, nullptr, &owner);
  }

  void attributeIsNotOwned(
      rti::ObjectInstanceHandle const& objectInstance,
      rti::AttributeHandleSet const& attributes) override {
    invokeOwnershipSetCallback(
        attributeIsNotOwned_, objectInstance, attributes, nullptr, nullptr);
  }

  void attributeIsOwnedByRTI(
      rti::ObjectInstanceHandle const& objectInstance,
      rti::AttributeHandleSet const& attributes) override {
    invokeOwnershipSetCallback(
        attributeIsOwnedByRTI_, objectInstance, attributes, nullptr, nullptr);
  }

  void discoverObjectInstance(
      rti::ObjectInstanceHandle const& objectInstance,
      rti::ObjectClassHandle const& objectClass,
      std::wstring const& objectInstanceName,
      rti::FederateHandle const& producingFederate) override {
    ScopedJNIEnvironment attachment(virtualMachine_);
    JNIEnv* environment = attachment.get();
    jmethodID method = nullptr;
    jobject target = retainedTarget(environment, method, discoverObjectInstance_);
    if (target == nullptr || method == nullptr) return;
    jobject javaInstance = javaObjectInstanceHandle(environment, objectInstance);
    jobject javaClass = javaObjectClassHandle(environment, objectClass);
    jstring javaName = javaString(environment, objectInstanceName);
    jobject javaProducingFederate = javaFederateHandle(environment, producingFederate);
    if (javaInstance == nullptr || javaClass == nullptr || javaName == nullptr ||
        javaProducingFederate == nullptr) {
      if (javaInstance != nullptr) environment->DeleteLocalRef(javaInstance);
      if (javaClass != nullptr) environment->DeleteLocalRef(javaClass);
      if (javaName != nullptr) environment->DeleteLocalRef(javaName);
      if (javaProducingFederate != nullptr) environment->DeleteLocalRef(javaProducingFederate);
      environment->DeleteLocalRef(target);
      clearJavaException(environment);
      throw rti::FederateInternalError(L"Umbra could not create a Java object discovery callback.");
    }
    environment->CallVoidMethod(
        target, method, javaInstance, javaClass, javaName, javaProducingFederate);
    environment->DeleteLocalRef(javaInstance);
    environment->DeleteLocalRef(javaClass);
    environment->DeleteLocalRef(javaName);
    environment->DeleteLocalRef(javaProducingFederate);
    environment->DeleteLocalRef(target);
    ensureNoJavaException(environment);
  }

  void objectInstanceNameReservationSucceeded(
      std::wstring const& objectInstanceName) override {
    invokeString(objectInstanceNameReservationSucceeded_, objectInstanceName);
  }

  void objectInstanceNameReservationFailed(
      std::wstring const& objectInstanceName) override {
    invokeString(objectInstanceNameReservationFailed_, objectInstanceName);
  }

  void multipleObjectInstanceNameReservationSucceeded(
      std::set<std::wstring> const& objectInstanceNames) override {
    invokeObjectInstanceNameSet(
        multipleObjectInstanceNameReservationSucceeded_, objectInstanceNames);
  }

  void multipleObjectInstanceNameReservationFailed(
      std::set<std::wstring> const& objectInstanceNames) override {
    invokeObjectInstanceNameSet(
        multipleObjectInstanceNameReservationFailed_, objectInstanceNames);
  }

  void reflectAttributeValues(
      rti::ObjectInstanceHandle const& objectInstance,
      rti::AttributeHandleValueMap const& attributeValues,
      rti::VariableLengthData const& userSuppliedTag,
      rti::TransportationTypeHandle const& transportationType,
      rti::FederateHandle const& producingFederate,
      rti::RegionHandleSet const* optionalSentRegions) override {
    ScopedJNIEnvironment attachment(virtualMachine_);
    JNIEnv* environment = attachment.get();
    jmethodID method = nullptr;
    jobject target = retainedTarget(environment, method, reflectAttributeValues_);
    if (target == nullptr || method == nullptr) return;
    jobject javaInstance = javaObjectInstanceHandle(environment, objectInstance);
    jobject javaAttributes = javaAttributeHandleValueMap(environment, attributeValues);
    jbyteArray javaTag = javaByteArray(environment, userSuppliedTag);
    jobject javaTransportation = javaTransportationTypeHandle(environment, transportationType);
    jobject javaProducingFederate = javaFederateHandle(environment, producingFederate);
    jobject javaRegions = optionalSentRegions == nullptr
        ? nullptr
        : javaRegionHandleSet(environment, *optionalSentRegions);
    if (javaInstance == nullptr || javaAttributes == nullptr || javaTag == nullptr ||
        javaTransportation == nullptr || javaProducingFederate == nullptr ||
        (optionalSentRegions != nullptr && javaRegions == nullptr)) {
      if (javaInstance != nullptr) environment->DeleteLocalRef(javaInstance);
      if (javaAttributes != nullptr) environment->DeleteLocalRef(javaAttributes);
      if (javaTag != nullptr) environment->DeleteLocalRef(javaTag);
      if (javaTransportation != nullptr) environment->DeleteLocalRef(javaTransportation);
      if (javaProducingFederate != nullptr) environment->DeleteLocalRef(javaProducingFederate);
      if (javaRegions != nullptr) environment->DeleteLocalRef(javaRegions);
      environment->DeleteLocalRef(target);
      clearJavaException(environment);
      throw rti::FederateInternalError(L"Umbra could not create a Java attribute-reflection callback.");
    }
    environment->CallVoidMethod(
        target,
        method,
        javaInstance,
        javaAttributes,
        javaTag,
        javaTransportation,
        javaProducingFederate,
        javaRegions);
    environment->DeleteLocalRef(javaInstance);
    environment->DeleteLocalRef(javaAttributes);
    environment->DeleteLocalRef(javaTag);
    environment->DeleteLocalRef(javaTransportation);
    environment->DeleteLocalRef(javaProducingFederate);
    if (javaRegions != nullptr) environment->DeleteLocalRef(javaRegions);
    environment->DeleteLocalRef(target);
    ensureNoJavaException(environment);
  }

  void reflectAttributeValues(
      rti::ObjectInstanceHandle const& objectInstance,
      rti::AttributeHandleValueMap const& attributeValues,
      rti::VariableLengthData const& userSuppliedTag,
      rti::TransportationTypeHandle const& transportationType,
      rti::FederateHandle const& producingFederate,
      rti::RegionHandleSet const* optionalSentRegions,
      rti::LogicalTime const& time,
      rti::OrderType sentOrderType,
      rti::OrderType receivedOrderType,
      rti::MessageRetractionHandle const* optionalRetraction) override {
    ScopedJNIEnvironment attachment(virtualMachine_);
    JNIEnv* environment = attachment.get();
    jmethodID method = nullptr;
    jobject target = retainedTarget(environment, method, reflectAttributeValuesWithTime_);
    if (target == nullptr || method == nullptr) return;
    jobject javaInstance = javaObjectInstanceHandle(environment, objectInstance);
    jobject javaAttributes = javaAttributeHandleValueMap(environment, attributeValues);
    jbyteArray javaTag = javaByteArray(environment, userSuppliedTag);
    jobject javaTransportation = javaTransportationTypeHandle(environment, transportationType);
    jobject javaProducingFederate = javaFederateHandle(environment, producingFederate);
    jobject javaRegions = optionalSentRegions == nullptr
        ? nullptr
        : javaRegionHandleSet(environment, *optionalSentRegions);
    jobject javaTime = javaLogicalTime(environment, time);
    jobject javaSentOrder = javaOrderType(environment, sentOrderType);
    jobject javaReceivedOrder = javaOrderType(environment, receivedOrderType);
    jobject javaRetraction = optionalRetraction == nullptr
        ? nullptr
        : javaMessageRetractionHandle(environment, *optionalRetraction);
    if (javaInstance == nullptr || javaAttributes == nullptr || javaTag == nullptr ||
        javaTransportation == nullptr || javaProducingFederate == nullptr || javaTime == nullptr ||
        javaSentOrder == nullptr || javaReceivedOrder == nullptr ||
        (optionalSentRegions != nullptr && javaRegions == nullptr) ||
        (optionalRetraction != nullptr && javaRetraction == nullptr)) {
      if (javaInstance != nullptr) environment->DeleteLocalRef(javaInstance);
      if (javaAttributes != nullptr) environment->DeleteLocalRef(javaAttributes);
      if (javaTag != nullptr) environment->DeleteLocalRef(javaTag);
      if (javaTransportation != nullptr) environment->DeleteLocalRef(javaTransportation);
      if (javaProducingFederate != nullptr) environment->DeleteLocalRef(javaProducingFederate);
      if (javaRegions != nullptr) environment->DeleteLocalRef(javaRegions);
      if (javaTime != nullptr) environment->DeleteLocalRef(javaTime);
      if (javaSentOrder != nullptr) environment->DeleteLocalRef(javaSentOrder);
      if (javaReceivedOrder != nullptr) environment->DeleteLocalRef(javaReceivedOrder);
      if (javaRetraction != nullptr) environment->DeleteLocalRef(javaRetraction);
      environment->DeleteLocalRef(target);
      clearJavaException(environment);
      throw rti::FederateInternalError(L"Umbra could not create a Java timestamped reflection callback.");
    }
    environment->CallVoidMethod(
        target,
        method,
        javaInstance,
        javaAttributes,
        javaTag,
        javaTransportation,
        javaProducingFederate,
        javaRegions,
        javaTime,
        javaSentOrder,
        javaReceivedOrder,
        javaRetraction);
    environment->DeleteLocalRef(javaInstance);
    environment->DeleteLocalRef(javaAttributes);
    environment->DeleteLocalRef(javaTag);
    environment->DeleteLocalRef(javaTransportation);
    environment->DeleteLocalRef(javaProducingFederate);
    if (javaRegions != nullptr) environment->DeleteLocalRef(javaRegions);
    environment->DeleteLocalRef(javaTime);
    environment->DeleteLocalRef(javaSentOrder);
    environment->DeleteLocalRef(javaReceivedOrder);
    if (javaRetraction != nullptr) environment->DeleteLocalRef(javaRetraction);
    environment->DeleteLocalRef(target);
    ensureNoJavaException(environment);
  }

  void removeObjectInstance(
      rti::ObjectInstanceHandle const& objectInstance,
      rti::VariableLengthData const& userSuppliedTag,
      rti::FederateHandle const& producingFederate) override {
    ScopedJNIEnvironment attachment(virtualMachine_);
    JNIEnv* environment = attachment.get();
    jmethodID method = nullptr;
    jobject target = retainedTarget(environment, method, removeObjectInstance_);
    if (target == nullptr || method == nullptr) return;
    jobject javaInstance = javaObjectInstanceHandle(environment, objectInstance);
    jbyteArray javaTag = javaByteArray(environment, userSuppliedTag);
    jobject javaProducingFederate = javaFederateHandle(environment, producingFederate);
    if (javaInstance == nullptr || javaTag == nullptr || javaProducingFederate == nullptr) {
      if (javaInstance != nullptr) environment->DeleteLocalRef(javaInstance);
      if (javaTag != nullptr) environment->DeleteLocalRef(javaTag);
      if (javaProducingFederate != nullptr) environment->DeleteLocalRef(javaProducingFederate);
      environment->DeleteLocalRef(target);
      clearJavaException(environment);
      throw rti::FederateInternalError(L"Umbra could not create a Java object-removal callback.");
    }
    environment->CallVoidMethod(
        target, method, javaInstance, javaTag, javaProducingFederate);
    environment->DeleteLocalRef(javaInstance);
    environment->DeleteLocalRef(javaTag);
    environment->DeleteLocalRef(javaProducingFederate);
    environment->DeleteLocalRef(target);
    ensureNoJavaException(environment);
  }

  void removeObjectInstance(
      rti::ObjectInstanceHandle const& objectInstance,
      rti::VariableLengthData const& userSuppliedTag,
      rti::FederateHandle const& producingFederate,
      rti::LogicalTime const& time,
      rti::OrderType sentOrderType,
      rti::OrderType receivedOrderType,
      rti::MessageRetractionHandle const* optionalRetraction) override {
    ScopedJNIEnvironment attachment(virtualMachine_);
    JNIEnv* environment = attachment.get();
    jmethodID method = nullptr;
    jobject target = retainedTarget(environment, method, removeObjectInstanceWithTime_);
    if (target == nullptr || method == nullptr) return;
    jobject javaInstance = javaObjectInstanceHandle(environment, objectInstance);
    jbyteArray javaTag = javaByteArray(environment, userSuppliedTag);
    jobject javaProducingFederate = javaFederateHandle(environment, producingFederate);
    jobject javaTime = javaLogicalTime(environment, time);
    jobject javaSentOrder = javaOrderType(environment, sentOrderType);
    jobject javaReceivedOrder = javaOrderType(environment, receivedOrderType);
    jobject javaRetraction = optionalRetraction == nullptr
        ? nullptr
        : javaMessageRetractionHandle(environment, *optionalRetraction);
    if (javaInstance == nullptr || javaTag == nullptr || javaProducingFederate == nullptr ||
        javaTime == nullptr || javaSentOrder == nullptr || javaReceivedOrder == nullptr ||
        (optionalRetraction != nullptr && javaRetraction == nullptr)) {
      if (javaInstance != nullptr) environment->DeleteLocalRef(javaInstance);
      if (javaTag != nullptr) environment->DeleteLocalRef(javaTag);
      if (javaProducingFederate != nullptr) environment->DeleteLocalRef(javaProducingFederate);
      if (javaTime != nullptr) environment->DeleteLocalRef(javaTime);
      if (javaSentOrder != nullptr) environment->DeleteLocalRef(javaSentOrder);
      if (javaReceivedOrder != nullptr) environment->DeleteLocalRef(javaReceivedOrder);
      if (javaRetraction != nullptr) environment->DeleteLocalRef(javaRetraction);
      environment->DeleteLocalRef(target);
      clearJavaException(environment);
      throw rti::FederateInternalError(L"Umbra could not create a Java timestamped removal callback.");
    }
    environment->CallVoidMethod(
        target,
        method,
        javaInstance,
        javaTag,
        javaProducingFederate,
        javaTime,
        javaSentOrder,
        javaReceivedOrder,
        javaRetraction);
    environment->DeleteLocalRef(javaInstance);
    environment->DeleteLocalRef(javaTag);
    environment->DeleteLocalRef(javaProducingFederate);
    environment->DeleteLocalRef(javaTime);
    environment->DeleteLocalRef(javaSentOrder);
    environment->DeleteLocalRef(javaReceivedOrder);
    if (javaRetraction != nullptr) environment->DeleteLocalRef(javaRetraction);
    environment->DeleteLocalRef(target);
    ensureNoJavaException(environment);
  }

  void provideAttributeValueUpdate(
      rti::ObjectInstanceHandle const& objectInstance,
      rti::AttributeHandleSet const& attributes,
      rti::VariableLengthData const& userSuppliedTag) override {
    ScopedJNIEnvironment attachment(virtualMachine_);
    JNIEnv* environment = attachment.get();
    jmethodID method = nullptr;
    jobject target = retainedTarget(environment, method, provideAttributeValueUpdate_);
    if (target == nullptr || method == nullptr) return;
    jobject javaInstance = javaObjectInstanceHandle(environment, objectInstance);
    jobject javaAttributes = javaAttributeHandleSet(environment, attributes);
    jbyteArray javaTag = javaByteArray(environment, userSuppliedTag);
    if (javaInstance == nullptr || javaAttributes == nullptr || javaTag == nullptr) {
      if (javaInstance != nullptr) environment->DeleteLocalRef(javaInstance);
      if (javaAttributes != nullptr) environment->DeleteLocalRef(javaAttributes);
      if (javaTag != nullptr) environment->DeleteLocalRef(javaTag);
      environment->DeleteLocalRef(target);
      clearJavaException(environment);
      throw rti::FederateInternalError(L"Umbra could not create a Java attribute-update request callback.");
    }
    environment->CallVoidMethod(target, method, javaInstance, javaAttributes, javaTag);
    environment->DeleteLocalRef(javaInstance);
    environment->DeleteLocalRef(javaAttributes);
    environment->DeleteLocalRef(javaTag);
    environment->DeleteLocalRef(target);
    ensureNoJavaException(environment);
  }

  void reportAttributeTransportationType(
      rti::ObjectInstanceHandle const& objectInstance,
      rti::AttributeHandle const& attribute,
      rti::TransportationTypeHandle const& transportationType) override {
    ScopedJNIEnvironment attachment(virtualMachine_);
    JNIEnv* environment = attachment.get();
    jmethodID method = nullptr;
    jobject target = retainedTarget(environment, method, reportAttributeTransportationType_);
    if (target == nullptr || method == nullptr) return;
    jobject javaInstance = javaObjectInstanceHandle(environment, objectInstance);
    jobject javaAttribute = javaAttributeHandle(environment, attribute);
    jobject javaTransportation = javaTransportationTypeHandle(environment, transportationType);
    if (javaInstance == nullptr || javaAttribute == nullptr || javaTransportation == nullptr) {
      if (javaInstance != nullptr) environment->DeleteLocalRef(javaInstance);
      if (javaAttribute != nullptr) environment->DeleteLocalRef(javaAttribute);
      if (javaTransportation != nullptr) environment->DeleteLocalRef(javaTransportation);
      environment->DeleteLocalRef(target);
      clearJavaException(environment);
      throw rti::FederateInternalError(L"Umbra could not create a Java attribute-transportation callback.");
    }
    environment->CallVoidMethod(
        target, method, javaInstance, javaAttribute, javaTransportation);
    environment->DeleteLocalRef(javaInstance);
    environment->DeleteLocalRef(javaAttribute);
    environment->DeleteLocalRef(javaTransportation);
    environment->DeleteLocalRef(target);
    ensureNoJavaException(environment);
  }

  void reportInteractionTransportationType(
      rti::FederateHandle const& federate,
      rti::InteractionClassHandle const& interactionClass,
      rti::TransportationTypeHandle const& transportationType) override {
    ScopedJNIEnvironment attachment(virtualMachine_);
    JNIEnv* environment = attachment.get();
    jmethodID method = nullptr;
    jobject target = retainedTarget(environment, method, reportInteractionTransportationType_);
    if (target == nullptr || method == nullptr) return;
    jobject javaFederate = javaFederateHandle(environment, federate);
    jobject javaInteraction = javaInteractionClassHandle(environment, interactionClass);
    jobject javaTransportation = javaTransportationTypeHandle(environment, transportationType);
    if (javaFederate == nullptr || javaInteraction == nullptr || javaTransportation == nullptr) {
      if (javaFederate != nullptr) environment->DeleteLocalRef(javaFederate);
      if (javaInteraction != nullptr) environment->DeleteLocalRef(javaInteraction);
      if (javaTransportation != nullptr) environment->DeleteLocalRef(javaTransportation);
      environment->DeleteLocalRef(target);
      clearJavaException(environment);
      throw rti::FederateInternalError(L"Umbra could not create a Java interaction-transportation callback.");
    }
    environment->CallVoidMethod(
        target, method, javaFederate, javaInteraction, javaTransportation);
    environment->DeleteLocalRef(javaFederate);
    environment->DeleteLocalRef(javaInteraction);
    environment->DeleteLocalRef(javaTransportation);
    environment->DeleteLocalRef(target);
    ensureNoJavaException(environment);
  }

  void confirmAttributeTransportationTypeChange(
      rti::ObjectInstanceHandle const& objectInstance,
      rti::AttributeHandleSet const& attributes,
      rti::TransportationTypeHandle const& transportationType) override {
    ScopedJNIEnvironment attachment(virtualMachine_);
    JNIEnv* environment = attachment.get();
    jmethodID method = nullptr;
    jobject target = retainedTarget(environment, method, confirmAttributeTransportationTypeChange_);
    if (target == nullptr || method == nullptr) return;
    jobject javaInstance = javaObjectInstanceHandle(environment, objectInstance);
    jobject javaAttributes = javaAttributeHandleSet(environment, attributes);
    jobject javaTransportation = javaTransportationTypeHandle(environment, transportationType);
    if (javaInstance == nullptr || javaAttributes == nullptr || javaTransportation == nullptr) {
      if (javaInstance != nullptr) environment->DeleteLocalRef(javaInstance);
      if (javaAttributes != nullptr) environment->DeleteLocalRef(javaAttributes);
      if (javaTransportation != nullptr) environment->DeleteLocalRef(javaTransportation);
      environment->DeleteLocalRef(target);
      clearJavaException(environment);
      throw rti::FederateInternalError(L"Umbra could not create a Java attribute-transportation confirmation.");
    }
    environment->CallVoidMethod(
        target, method, javaInstance, javaAttributes, javaTransportation);
    environment->DeleteLocalRef(javaInstance);
    environment->DeleteLocalRef(javaAttributes);
    environment->DeleteLocalRef(javaTransportation);
    environment->DeleteLocalRef(target);
    ensureNoJavaException(environment);
  }

  void confirmInteractionTransportationTypeChange(
      rti::InteractionClassHandle const& interactionClass,
      rti::TransportationTypeHandle const& transportationType) override {
    ScopedJNIEnvironment attachment(virtualMachine_);
    JNIEnv* environment = attachment.get();
    jmethodID method = nullptr;
    jobject target = retainedTarget(environment, method, confirmInteractionTransportationTypeChange_);
    if (target == nullptr || method == nullptr) return;
    jobject javaInteraction = javaInteractionClassHandle(environment, interactionClass);
    jobject javaTransportation = javaTransportationTypeHandle(environment, transportationType);
    if (javaInteraction == nullptr || javaTransportation == nullptr) {
      if (javaInteraction != nullptr) environment->DeleteLocalRef(javaInteraction);
      if (javaTransportation != nullptr) environment->DeleteLocalRef(javaTransportation);
      environment->DeleteLocalRef(target);
      clearJavaException(environment);
      throw rti::FederateInternalError(L"Umbra could not create a Java interaction-transportation confirmation.");
    }
    environment->CallVoidMethod(target, method, javaInteraction, javaTransportation);
    environment->DeleteLocalRef(javaInteraction);
    environment->DeleteLocalRef(javaTransportation);
    environment->DeleteLocalRef(target);
    ensureNoJavaException(environment);
  }

  void attributesInScope(
      rti::ObjectInstanceHandle const& objectInstance,
      rti::AttributeHandleSet const& attributes) override {
    invokeObjectInstanceAttributeSet(attributesInScope_, objectInstance, attributes);
  }

  void attributesOutOfScope(
      rti::ObjectInstanceHandle const& objectInstance,
      rti::AttributeHandleSet const& attributes) override {
    invokeObjectInstanceAttributeSet(attributesOutOfScope_, objectInstance, attributes);
  }

  void turnUpdatesOnForObjectInstance(
      rti::ObjectInstanceHandle const& objectInstance,
      rti::AttributeHandleSet const& attributes) override {
    invokeObjectInstanceAttributeSet(
        turnUpdatesOnForObjectInstance_, objectInstance, attributes);
  }

  void turnUpdatesOnForObjectInstance(
      rti::ObjectInstanceHandle const& objectInstance,
      rti::AttributeHandleSet const& attributes,
      std::wstring const& updateRateDesignator) override {
    invokeObjectInstanceAttributeSetWithString(
        turnUpdatesOnForObjectInstanceWithRate_,
        objectInstance,
        attributes,
        updateRateDesignator);
  }

  void turnUpdatesOffForObjectInstance(
      rti::ObjectInstanceHandle const& objectInstance,
      rti::AttributeHandleSet const& attributes) override {
    invokeObjectInstanceAttributeSet(
        turnUpdatesOffForObjectInstance_, objectInstance, attributes);
  }

  void timeAdvanceGrant(rti::LogicalTime const& time) override {
    ScopedJNIEnvironment attachment(virtualMachine_);
    JNIEnv* environment = attachment.get();
    jmethodID method = nullptr;
    jobject target = retainedTarget(environment, method, timeAdvanceGrant_);
    if (target == nullptr || method == nullptr) return;
    jobject javaTime = javaLogicalTime(environment, time);
    if (javaTime == nullptr) {
      environment->DeleteLocalRef(target);
      clearJavaException(environment);
      throw rti::FederateInternalError(L"Umbra could not create a Java time-advance grant.");
    }
    environment->CallVoidMethod(target, method, javaTime);
    environment->DeleteLocalRef(javaTime);
    environment->DeleteLocalRef(target);
    ensureNoJavaException(environment);
  }

  void flushQueueGrant(
      rti::LogicalTime const& time,
      rti::LogicalTime const& optimisticTime) override {
    ScopedJNIEnvironment attachment(virtualMachine_);
    JNIEnv* environment = attachment.get();
    jmethodID method = nullptr;
    jobject target = retainedTarget(environment, method, flushQueueGrant_);
    if (target == nullptr || method == nullptr) return;
    jobject javaTime = javaLogicalTime(environment, time);
    jobject javaOptimisticTime = javaLogicalTime(environment, optimisticTime);
    if (javaTime == nullptr || javaOptimisticTime == nullptr) {
      if (javaTime != nullptr) environment->DeleteLocalRef(javaTime);
      if (javaOptimisticTime != nullptr) environment->DeleteLocalRef(javaOptimisticTime);
      environment->DeleteLocalRef(target);
      clearJavaException(environment);
      throw rti::FederateInternalError(L"Umbra could not create a Java flush-queue grant.");
    }
    environment->CallVoidMethod(target, method, javaTime, javaOptimisticTime);
    environment->DeleteLocalRef(javaTime);
    environment->DeleteLocalRef(javaOptimisticTime);
    environment->DeleteLocalRef(target);
    ensureNoJavaException(environment);
  }

  void timeConstrainedEnabled(rti::LogicalTime const& time) override {
    ScopedJNIEnvironment attachment(virtualMachine_);
    JNIEnv* environment = attachment.get();
    jmethodID method = nullptr;
    jobject target = retainedTarget(environment, method, timeConstrainedEnabled_);
    if (target == nullptr || method == nullptr) return;
    jobject javaTime = javaLogicalTime(environment, time);
    if (javaTime == nullptr) {
      environment->DeleteLocalRef(target);
      clearJavaException(environment);
      throw rti::FederateInternalError(L"Umbra could not create a Java time-constrained callback.");
    }
    environment->CallVoidMethod(target, method, javaTime);
    environment->DeleteLocalRef(javaTime);
    environment->DeleteLocalRef(target);
    ensureNoJavaException(environment);
  }

  void timeRegulationEnabled(rti::LogicalTime const& time) override {
    ScopedJNIEnvironment attachment(virtualMachine_);
    JNIEnv* environment = attachment.get();
    jmethodID method = nullptr;
    jobject target = retainedTarget(environment, method, timeRegulationEnabled_);
    if (target == nullptr || method == nullptr) return;
    jobject javaTime = javaLogicalTime(environment, time);
    if (javaTime == nullptr) {
      environment->DeleteLocalRef(target);
      clearJavaException(environment);
      throw rti::FederateInternalError(L"Umbra could not create a Java time-regulation callback.");
    }
    environment->CallVoidMethod(target, method, javaTime);
    environment->DeleteLocalRef(javaTime);
    environment->DeleteLocalRef(target);
    ensureNoJavaException(environment);
  }

  void requestRetraction(rti::MessageRetractionHandle const& retraction) override {
    ScopedJNIEnvironment attachment(virtualMachine_);
    JNIEnv* environment = attachment.get();
    jmethodID method = nullptr;
    jobject target = retainedTarget(environment, method, requestRetraction_);
    if (target == nullptr || method == nullptr) return;
    jobject javaRetraction = javaMessageRetractionHandle(environment, retraction);
    if (javaRetraction == nullptr) {
      environment->DeleteLocalRef(target);
      clearJavaException(environment);
      throw rti::FederateInternalError(L"Umbra could not create a Java retraction callback.");
    }
    environment->CallVoidMethod(target, method, javaRetraction);
    environment->DeleteLocalRef(javaRetraction);
    environment->DeleteLocalRef(target);
    ensureNoJavaException(environment);
  }

 private:
  static std::wstring saveStatusName(rti::SaveStatus status) {
    switch (status) {
      case rti::NO_SAVE_IN_PROGRESS:
        return L"NO_SAVE_IN_PROGRESS";
      case rti::FEDERATE_INSTRUCTED_TO_SAVE:
        return L"FEDERATE_INSTRUCTED_TO_SAVE";
      case rti::FEDERATE_SAVING:
        return L"FEDERATE_SAVING";
      case rti::FEDERATE_WAITING_FOR_FEDERATION_TO_SAVE:
        return L"FEDERATE_WAITING_FOR_FEDERATION_TO_SAVE";
    }
    throw rti::FederateInternalError(L"Umbra could not translate a save-status enum value.");
  }

  static std::wstring synchronizationPointFailureReasonName(
      rti::SynchronizationPointFailureReason reason) {
    switch (reason) {
      case rti::SYNCHRONIZATION_POINT_LABEL_NOT_UNIQUE:
        return L"SYNCHRONIZATION_POINT_LABEL_NOT_UNIQUE";
      case rti::SYNCHRONIZATION_SET_MEMBER_NOT_JOINED:
        return L"SYNCHRONIZATION_SET_MEMBER_NOT_JOINED";
    }
    throw rti::FederateInternalError(
        L"Umbra could not translate a synchronization-point failure enum value.");
  }

  static std::wstring saveFailureReasonName(rti::SaveFailureReason reason) {
    switch (reason) {
      case rti::RTI_UNABLE_TO_SAVE:
        return L"RTI_UNABLE_TO_SAVE";
      case rti::FEDERATE_REPORTED_FAILURE_DURING_SAVE:
        return L"FEDERATE_REPORTED_FAILURE_DURING_SAVE";
      case rti::FEDERATE_RESIGNED_DURING_SAVE:
        return L"FEDERATE_RESIGNED_DURING_SAVE";
      case rti::RTI_DETECTED_FAILURE_DURING_SAVE:
        return L"RTI_DETECTED_FAILURE_DURING_SAVE";
      case rti::SAVE_TIME_CANNOT_BE_HONORED:
        return L"SAVE_TIME_CANNOT_BE_HONORED";
      case rti::SAVE_ABORTED:
        return L"SAVE_ABORTED";
    }
    throw rti::FederateInternalError(L"Umbra could not translate a save-failure enum value.");
  }

  static std::wstring restoreStatusName(rti::RestoreStatus status) {
    switch (status) {
      case rti::NO_RESTORE_IN_PROGRESS: return L"NO_RESTORE_IN_PROGRESS";
      case rti::FEDERATE_RESTORE_REQUEST_PENDING: return L"FEDERATE_RESTORE_REQUEST_PENDING";
      case rti::FEDERATE_WAITING_FOR_RESTORE_TO_BEGIN: return L"FEDERATE_WAITING_FOR_RESTORE_TO_BEGIN";
      case rti::FEDERATE_PREPARED_TO_RESTORE: return L"FEDERATE_PREPARED_TO_RESTORE";
      case rti::FEDERATE_RESTORING: return L"FEDERATE_RESTORING";
      case rti::FEDERATE_WAITING_FOR_FEDERATION_TO_RESTORE: return L"FEDERATE_WAITING_FOR_FEDERATION_TO_RESTORE";
    }
    throw rti::FederateInternalError(L"Umbra could not translate a restore-status enum value.");
  }

  static std::wstring restoreFailureReasonName(rti::RestoreFailureReason reason) {
    switch (reason) {
      case rti::RTI_UNABLE_TO_RESTORE:
        return L"RTI_UNABLE_TO_RESTORE";
      case rti::FEDERATE_REPORTED_FAILURE_DURING_RESTORE:
        return L"FEDERATE_REPORTED_FAILURE_DURING_RESTORE";
      case rti::FEDERATE_RESIGNED_DURING_RESTORE:
        return L"FEDERATE_RESIGNED_DURING_RESTORE";
      case rti::RTI_DETECTED_FAILURE_DURING_RESTORE:
        return L"RTI_DETECTED_FAILURE_DURING_RESTORE";
      case rti::RESTORE_ABORTED:
        return L"RESTORE_ABORTED";
    }
    throw rti::FederateInternalError(L"Umbra could not translate a restore-failure enum value.");
  }

  jobject retainedTarget(JNIEnv* environment, jmethodID& method, jmethodID storedMethod) {
    std::scoped_lock lock(mutex_);
    method = storedMethod;
    return target_ == nullptr ? nullptr : environment->NewLocalRef(target_);
  }

  void invokeString(jmethodID storedMethod, std::wstring const& value) {
    ScopedJNIEnvironment attachment(virtualMachine_);
    JNIEnv* environment = attachment.get();
    jmethodID method = nullptr;
    jobject target = retainedTarget(environment, method, storedMethod);
    if (target == nullptr || method == nullptr) {
      return;
    }
    jstring argument = javaString(environment, value);
    if (argument == nullptr) {
      environment->DeleteLocalRef(target);
      clearJavaException(environment);
      throw rti::FederateInternalError(L"Umbra could not create a Java callback string.");
    }
    environment->CallVoidMethod(target, method, argument);
    environment->DeleteLocalRef(argument);
    environment->DeleteLocalRef(target);
    ensureNoJavaException(environment);
  }

  void invokeObjectInstanceNameSet(
      jmethodID storedMethod, std::set<std::wstring> const& values) {
    ScopedJNIEnvironment attachment(virtualMachine_);
    JNIEnv* environment = attachment.get();
    jmethodID method = nullptr;
    jobject target = retainedTarget(environment, method, storedMethod);
    if (target == nullptr || method == nullptr) return;
    jobject javaNames = javaObjectInstanceNameSet(environment, values);
    if (javaNames == nullptr) {
      environment->DeleteLocalRef(target);
      clearJavaException(environment);
      throw rti::FederateInternalError(L"Umbra could not create a Java object-name set callback.");
    }
    environment->CallVoidMethod(target, method, javaNames);
    environment->DeleteLocalRef(javaNames);
    environment->DeleteLocalRef(target);
    ensureNoJavaException(environment);
  }

  void invokeObjectClass(
      jmethodID storedMethod,
      rti::ObjectClassHandle const& objectClass) {
    ScopedJNIEnvironment attachment(virtualMachine_);
    JNIEnv* environment = attachment.get();
    jmethodID method = nullptr;
    jobject target = retainedTarget(environment, method, storedMethod);
    if (target == nullptr || method == nullptr) return;
    jobject javaObjectClass = javaObjectClassHandle(environment, objectClass);
    if (javaObjectClass == nullptr) {
      environment->DeleteLocalRef(target);
      clearJavaException(environment);
      throw rti::FederateInternalError(L"Umbra could not create a Java declaration advisory.");
    }
    environment->CallVoidMethod(target, method, javaObjectClass);
    environment->DeleteLocalRef(javaObjectClass);
    environment->DeleteLocalRef(target);
    ensureNoJavaException(environment);
  }

  void invokeInteractionClass(
      jmethodID storedMethod,
      rti::InteractionClassHandle const& interactionClass) {
    ScopedJNIEnvironment attachment(virtualMachine_);
    JNIEnv* environment = attachment.get();
    jmethodID method = nullptr;
    jobject target = retainedTarget(environment, method, storedMethod);
    if (target == nullptr || method == nullptr) return;
    jobject javaInteractionClass = javaInteractionClassHandle(environment, interactionClass);
    if (javaInteractionClass == nullptr) {
      environment->DeleteLocalRef(target);
      clearJavaException(environment);
      throw rti::FederateInternalError(L"Umbra could not create a Java declaration advisory.");
    }
    environment->CallVoidMethod(target, method, javaInteractionClass);
    environment->DeleteLocalRef(javaInteractionClass);
    environment->DeleteLocalRef(target);
    ensureNoJavaException(environment);
  }

  void invokeObjectInstanceAttributeSet(
      jmethodID storedMethod,
      rti::ObjectInstanceHandle const& objectInstance,
      rti::AttributeHandleSet const& attributes) {
    ScopedJNIEnvironment attachment(virtualMachine_);
    JNIEnv* environment = attachment.get();
    jmethodID method = nullptr;
    jobject target = retainedTarget(environment, method, storedMethod);
    if (target == nullptr || method == nullptr) return;
    jobject javaInstance = javaObjectInstanceHandle(environment, objectInstance);
    jobject javaAttributes = javaAttributeHandleSet(environment, attributes);
    if (javaInstance == nullptr || javaAttributes == nullptr) {
      if (javaInstance != nullptr) environment->DeleteLocalRef(javaInstance);
      if (javaAttributes != nullptr) environment->DeleteLocalRef(javaAttributes);
      environment->DeleteLocalRef(target);
      clearJavaException(environment);
      throw rti::FederateInternalError(L"Umbra could not create a Java attribute-scope callback.");
    }
    environment->CallVoidMethod(target, method, javaInstance, javaAttributes);
    environment->DeleteLocalRef(javaInstance);
    environment->DeleteLocalRef(javaAttributes);
    environment->DeleteLocalRef(target);
    ensureNoJavaException(environment);
  }

  void invokeObjectInstanceAttributeSetWithString(
      jmethodID storedMethod,
      rti::ObjectInstanceHandle const& objectInstance,
      rti::AttributeHandleSet const& attributes,
      std::wstring const& value) {
    ScopedJNIEnvironment attachment(virtualMachine_);
    JNIEnv* environment = attachment.get();
    jmethodID method = nullptr;
    jobject target = retainedTarget(environment, method, storedMethod);
    if (target == nullptr || method == nullptr) return;
    jobject javaInstance = javaObjectInstanceHandle(environment, objectInstance);
    jobject javaAttributes = javaAttributeHandleSet(environment, attributes);
    jstring javaValue = javaString(environment, value);
    if (javaInstance == nullptr || javaAttributes == nullptr || javaValue == nullptr) {
      if (javaInstance != nullptr) environment->DeleteLocalRef(javaInstance);
      if (javaAttributes != nullptr) environment->DeleteLocalRef(javaAttributes);
      if (javaValue != nullptr) environment->DeleteLocalRef(javaValue);
      environment->DeleteLocalRef(target);
      clearJavaException(environment);
      throw rti::FederateInternalError(
          L"Umbra could not create a Java update-relevance advisory.");
    }
    environment->CallVoidMethod(target, method, javaInstance, javaAttributes, javaValue);
    environment->DeleteLocalRef(javaInstance);
    environment->DeleteLocalRef(javaAttributes);
    environment->DeleteLocalRef(javaValue);
    environment->DeleteLocalRef(target);
    ensureNoJavaException(environment);
  }

  void invokeOwnershipSetCallback(
      jmethodID storedMethod,
      rti::ObjectInstanceHandle const& objectInstance,
      rti::AttributeHandleSet const& attributes,
      rti::VariableLengthData const* optionalTag,
      rti::FederateHandle const* optionalOwner) {
    ScopedJNIEnvironment attachment(virtualMachine_);
    JNIEnv* environment = attachment.get();
    jmethodID method = nullptr;
    jobject target = retainedTarget(environment, method, storedMethod);
    if (target == nullptr || method == nullptr) return;
    jobject javaInstance = javaObjectInstanceHandle(environment, objectInstance);
    jobject javaAttributes = javaAttributeHandleSet(environment, attributes);
    jbyteArray javaTag = optionalTag == nullptr
        ? nullptr
        : javaByteArray(environment, *optionalTag);
    jobject javaOwner = optionalOwner == nullptr
        ? nullptr
        : javaFederateHandle(environment, *optionalOwner);
    if (javaInstance == nullptr || javaAttributes == nullptr ||
        (optionalTag != nullptr && javaTag == nullptr) ||
        (optionalOwner != nullptr && javaOwner == nullptr)) {
      if (javaInstance != nullptr) environment->DeleteLocalRef(javaInstance);
      if (javaAttributes != nullptr) environment->DeleteLocalRef(javaAttributes);
      if (javaTag != nullptr) environment->DeleteLocalRef(javaTag);
      if (javaOwner != nullptr) environment->DeleteLocalRef(javaOwner);
      environment->DeleteLocalRef(target);
      clearJavaException(environment);
      throw rti::FederateInternalError(L"Umbra could not create a Java ownership callback.");
    }
    if (optionalTag != nullptr) {
      environment->CallVoidMethod(target, method, javaInstance, javaAttributes, javaTag);
    } else if (optionalOwner != nullptr) {
      environment->CallVoidMethod(target, method, javaInstance, javaAttributes, javaOwner);
    } else {
      environment->CallVoidMethod(target, method, javaInstance, javaAttributes);
    }
    environment->DeleteLocalRef(javaInstance);
    environment->DeleteLocalRef(javaAttributes);
    if (javaTag != nullptr) environment->DeleteLocalRef(javaTag);
    if (javaOwner != nullptr) environment->DeleteLocalRef(javaOwner);
    environment->DeleteLocalRef(target);
    ensureNoJavaException(environment);
  }

  void invokeVoid(jmethodID storedMethod) {
    ScopedJNIEnvironment attachment(virtualMachine_);
    JNIEnv* environment = attachment.get();
    jmethodID method = nullptr;
    jobject target = retainedTarget(environment, method, storedMethod);
    if (target == nullptr || method == nullptr) return;
    environment->CallVoidMethod(target, method);
    environment->DeleteLocalRef(target);
    ensureNoJavaException(environment);
  }

  jobject javaFederateHandleSet(
      JNIEnv* environment, rti::FederateHandleSet const& values) {
    jobject result = environment->NewObject(
        federateHandleSetClass_, federateHandleSetConstructor_);
    if (result == nullptr) {
      clearJavaException(environment);
      throw rti::FederateInternalError(L"Umbra could not create a Java federate-handle set.");
    }
    try {
      for (auto const& value : values) {
        jbyteArray encoded = javaByteArray(environment, value.encode());
        jobject handle = environment->NewObject(
            federateHandleClass_, federateHandleConstructor_, encoded);
        if (encoded != nullptr) environment->DeleteLocalRef(encoded);
        if (handle == nullptr) {
          clearJavaException(environment);
          throw rti::FederateInternalError(L"Umbra could not create a Java federate handle.");
        }
        environment->CallBooleanMethod(result, federateHandleSetAdd_, handle);
        environment->DeleteLocalRef(handle);
        ensureNoJavaException(environment);
      }
    } catch (...) {
      environment->DeleteLocalRef(result);
      throw;
    }
    return result;
  }

  jobject javaObjectInstanceNameSet(
      JNIEnv* environment, std::set<std::wstring> const& values) {
    jobject result = environment->NewObject(
        objectInstanceNameSetClass_, objectInstanceNameSetConstructor_);
    if (result == nullptr) {
      clearJavaException(environment);
      throw rti::FederateInternalError(L"Umbra could not create a Java object-name set.");
    }
    try {
      for (auto const& value : values) {
        jstring name = javaString(environment, value);
        if (name == nullptr) {
          clearJavaException(environment);
          throw rti::FederateInternalError(L"Umbra could not create a Java object name.");
        }
        environment->CallBooleanMethod(result, objectInstanceNameSetAdd_, name);
        environment->DeleteLocalRef(name);
        ensureNoJavaException(environment);
      }
    } catch (...) {
      environment->DeleteLocalRef(result);
      throw;
    }
    return result;
  }

  jobject javaAttributeHandleSet(
      JNIEnv* environment, rti::AttributeHandleSet const& values) {
    jobject result = environment->NewObject(
        attributeHandleSetClass_, attributeHandleSetConstructor_);
    if (result == nullptr) {
      clearJavaException(environment);
      throw rti::FederateInternalError(L"Umbra could not create a Java attribute-handle set.");
    }
    try {
      for (auto const& value : values) {
        jobject handle = javaAttributeHandle(environment, value);
        if (handle == nullptr) {
          clearJavaException(environment);
          throw rti::FederateInternalError(L"Umbra could not create a Java attribute handle.");
        }
        environment->CallBooleanMethod(result, attributeHandleSetAdd_, handle);
        environment->DeleteLocalRef(handle);
        ensureNoJavaException(environment);
      }
    } catch (...) {
      environment->DeleteLocalRef(result);
      throw;
    }
    return result;
  }

  jobject javaRegionHandleSet(
      JNIEnv* environment, rti::RegionHandleSet const& values) {
    jobject result = environment->NewObject(
        regionHandleSetClass_, regionHandleSetConstructor_);
    if (result == nullptr) {
      clearJavaException(environment);
      throw rti::FederateInternalError(L"Umbra could not create a Java region-handle set.");
    }
    try {
      for (auto const& value : values) {
        jbyteArray encoded = javaByteArray(environment, value.encode());
        jobject handle = environment->NewObject(
            regionHandleClass_, regionHandleConstructor_, encoded);
        if (encoded != nullptr) environment->DeleteLocalRef(encoded);
        if (handle == nullptr) {
          clearJavaException(environment);
          throw rti::FederateInternalError(L"Umbra could not create a Java region handle.");
        }
        environment->CallBooleanMethod(result, regionHandleSetAdd_, handle);
        environment->DeleteLocalRef(handle);
        ensureNoJavaException(environment);
      }
    } catch (...) {
      environment->DeleteLocalRef(result);
      throw;
    }
    return result;
  }

  jobject javaFederateHandle(JNIEnv* environment, rti::FederateHandle const& value) {
    jbyteArray encoded = javaByteArray(environment, value.encode());
    jobject result = environment->NewObject(
        federateHandleClass_, federateHandleConstructor_, encoded);
    if (encoded != nullptr) environment->DeleteLocalRef(encoded);
    return result;
  }

  jobject javaObjectInstanceHandle(
      JNIEnv* environment, rti::ObjectInstanceHandle const& value) {
    jbyteArray encoded = javaByteArray(environment, value.encode());
    jobject result = environment->NewObject(
        objectInstanceHandleClass_, objectInstanceHandleConstructor_, encoded);
    if (encoded != nullptr) environment->DeleteLocalRef(encoded);
    return result;
  }

  jobject javaObjectClassHandle(
      JNIEnv* environment, rti::ObjectClassHandle const& value) {
    jbyteArray encoded = javaByteArray(environment, value.encode());
    jobject result = environment->NewObject(
        objectClassHandleClass_, objectClassHandleConstructor_, encoded);
    if (encoded != nullptr) environment->DeleteLocalRef(encoded);
    return result;
  }

  jobject javaAttributeHandle(JNIEnv* environment, rti::AttributeHandle const& value) {
    jbyteArray encoded = javaByteArray(environment, value.encode());
    jobject result = environment->NewObject(
        attributeHandleClass_, attributeHandleConstructor_, encoded);
    if (encoded != nullptr) environment->DeleteLocalRef(encoded);
    return result;
  }

  jobject javaLogicalTime(JNIEnv* environment, rti::LogicalTime const& value) {
    std::wstring factoryName;
    if (value.implementationName() == L"HLAinteger64Time") {
      factoryName = L"HLAinteger64Time";
    } else if (value.implementationName() == L"HLAfloat64Time") {
      factoryName = L"HLAfloat64Time";
    } else {
      throw rti::FederateInternalError(
          L"Umbra JNI received a callback time from an unsupported logical-time implementation.");
    }
    jclass factoriesClass = environment->FindClass(
        "org/umbra/jni/rti1516_2025/NativeLogicalTimeFactories");
    if (factoriesClass == nullptr) {
      clearJavaException(environment);
      throw rti::FederateInternalError(
          L"Umbra could not find the Java bridge logical-time factories.");
    }
    jclass factoryInterface = nullptr;
    jmethodID forName = nullptr;
    jmethodID decode = nullptr;
    jobject factory = nullptr;
    jstring javaFactoryName = nullptr;
    jbyteArray encoded = nullptr;
    try {
      forName = requiredStaticMethod(
          environment,
          factoriesClass,
          "forName",
          "(Ljava/lang/String;J)Lhla/rti1516_2025/time/LogicalTimeFactory;");
      factoryInterface = environment->FindClass("hla/rti1516_2025/time/LogicalTimeFactory");
      if (factoryInterface == nullptr) {
        clearJavaException(environment);
        throw rti::FederateInternalError(L"Umbra could not find the standard Java logical-time factory interface.");
      }
      decode = requiredMethod(
          environment,
          factoryInterface,
          "decodeTime",
          "([BI)Lhla/rti1516_2025/time/LogicalTime;");
      javaFactoryName = javaString(environment, factoryName);
      factory = environment->CallStaticObjectMethod(
          factoriesClass, forName, javaFactoryName, nativeHandle_);
      environment->DeleteLocalRef(javaFactoryName);
      javaFactoryName = nullptr;
      encoded = javaByteArray(environment, value.encode());
      if (factory == nullptr || encoded == nullptr) {
        clearJavaException(environment);
        throw rti::FederateInternalError(L"Umbra could not create a Java logical-time value.");
      }
      jobject result = environment->CallObjectMethod(factory, decode, encoded, 0);
      environment->DeleteLocalRef(encoded);
      encoded = nullptr;
      environment->DeleteLocalRef(factory);
      factory = nullptr;
      environment->DeleteLocalRef(factoryInterface);
      factoryInterface = nullptr;
      environment->DeleteLocalRef(factoriesClass);
      factoriesClass = nullptr;
      ensureNoJavaException(environment);
      if (result == nullptr) {
        throw rti::FederateInternalError(L"The Java time factory returned null logical time.");
      }
      return result;
    } catch (...) {
      if (encoded != nullptr) environment->DeleteLocalRef(encoded);
      if (javaFactoryName != nullptr) environment->DeleteLocalRef(javaFactoryName);
      if (factory != nullptr) environment->DeleteLocalRef(factory);
      if (factoryInterface != nullptr) environment->DeleteLocalRef(factoryInterface);
      if (factoriesClass != nullptr) environment->DeleteLocalRef(factoriesClass);
      throw;
    }
  }

  jobject javaOrderType(JNIEnv* environment, rti::OrderType value) {
    jclass orderTypeClass = environment->FindClass("hla/rti1516_2025/OrderType");
    if (orderTypeClass == nullptr) {
      clearJavaException(environment);
      throw rti::FederateInternalError(L"Umbra could not find the Java order-type enum.");
    }
    jstring name = nullptr;
    try {
      jmethodID valueOf = environment->GetStaticMethodID(
          orderTypeClass,
          "valueOf",
          "(Ljava/lang/String;)Lhla/rti1516_2025/OrderType;");
      if (valueOf == nullptr) {
        clearJavaException(environment);
        throw rti::FederateInternalError(L"Umbra could not resolve Java OrderType.valueOf.");
      }
      std::wstring enumName;
      switch (value) {
        case rti::RECEIVE:
          enumName = L"RECEIVE";
          break;
        case rti::TIMESTAMP:
          enumName = L"TIMESTAMP";
          break;
        default:
          throw rti::InvalidOrderType(
              L"The C++ RTI returned an unknown callback order type.");
      }
      name = javaString(environment, enumName);
      if (name == nullptr) {
        clearJavaException(environment);
        throw rti::FederateInternalError(L"Umbra could not create a Java order-type name.");
      }
      jobject result = environment->CallStaticObjectMethod(orderTypeClass, valueOf, name);
      environment->DeleteLocalRef(name);
      name = nullptr;
      environment->DeleteLocalRef(orderTypeClass);
      orderTypeClass = nullptr;
      ensureNoJavaException(environment);
      if (result == nullptr) {
        throw rti::FederateInternalError(L"The Java order-type enum returned null.");
      }
      return result;
    } catch (...) {
      if (name != nullptr) environment->DeleteLocalRef(name);
      if (orderTypeClass != nullptr) environment->DeleteLocalRef(orderTypeClass);
      throw;
    }
  }

  jobject javaMessageRetractionHandle(
      JNIEnv* environment, rti::MessageRetractionHandle const& value) {
    jclass handleClass = environment->FindClass(
        "org/umbra/jni/rti1516_2025/NativeMessageRetractionHandle");
    if (handleClass == nullptr) {
      clearJavaException(environment);
      throw rti::FederateInternalError(L"Umbra could not find the Java retraction-handle carrier.");
    }
    jbyteArray encoded = nullptr;
    try {
      jmethodID constructor = requiredMethod(environment, handleClass, "<init>", "([B)V");
      encoded = javaByteArray(environment, value.encode());
      jobject result = environment->NewObject(handleClass, constructor, encoded);
      environment->DeleteLocalRef(encoded);
      encoded = nullptr;
      environment->DeleteLocalRef(handleClass);
      handleClass = nullptr;
      ensureNoJavaException(environment);
      if (result == nullptr) {
        throw rti::FederateInternalError(L"Umbra could not create a Java retraction handle.");
      }
      return result;
    } catch (...) {
      if (encoded != nullptr) environment->DeleteLocalRef(encoded);
      if (handleClass != nullptr) environment->DeleteLocalRef(handleClass);
      throw;
    }
  }

  jobject javaInteractionClassHandle(
      JNIEnv* environment, rti::InteractionClassHandle const& value) {
    jbyteArray encoded = javaByteArray(environment, value.encode());
    jobject result = environment->NewObject(
        interactionClassHandleClass_, interactionClassHandleConstructor_, encoded);
    if (encoded != nullptr) environment->DeleteLocalRef(encoded);
    return result;
  }

  jobject javaTransportationTypeHandle(
      JNIEnv* environment, rti::TransportationTypeHandle const& value) {
    jbyteArray encoded = javaByteArray(environment, value.encode());
    jobject result = environment->NewObject(
        transportationTypeHandleClass_, transportationTypeHandleConstructor_, encoded);
    if (encoded != nullptr) environment->DeleteLocalRef(encoded);
    return result;
  }

  jobject javaParameterHandleValueMap(
      JNIEnv* environment, rti::ParameterHandleValueMap const& values) {
    jobject result = environment->NewObject(
        parameterHandleValueMapClass_, parameterHandleValueMapConstructor_);
    if (result == nullptr) {
      clearJavaException(environment);
      throw rti::FederateInternalError(L"Umbra could not create a Java parameter-value map.");
    }
    try {
      for (auto const& [parameter, value] : values) {
        jbyteArray encoded = javaByteArray(environment, parameter.encode());
        jobject javaParameter = environment->NewObject(
            parameterHandleClass_, parameterHandleConstructor_, encoded);
        if (encoded != nullptr) environment->DeleteLocalRef(encoded);
        jbyteArray javaValue = javaByteArray(environment, value);
        if (javaParameter == nullptr || javaValue == nullptr) {
          if (javaParameter != nullptr) environment->DeleteLocalRef(javaParameter);
          if (javaValue != nullptr) environment->DeleteLocalRef(javaValue);
          clearJavaException(environment);
          throw rti::FederateInternalError(L"Umbra could not create a Java interaction parameter.");
        }
        jobject previous = environment->CallObjectMethod(
            result, parameterHandleValueMapPut_, javaParameter, javaValue);
        if (previous != nullptr) environment->DeleteLocalRef(previous);
        environment->DeleteLocalRef(javaParameter);
        environment->DeleteLocalRef(javaValue);
        ensureNoJavaException(environment);
      }
    } catch (...) {
      environment->DeleteLocalRef(result);
      throw;
    }
    return result;
  }

  jobject javaAttributeHandleValueMap(
      JNIEnv* environment, rti::AttributeHandleValueMap const& values) {
    jobject result = environment->NewObject(
        attributeHandleValueMapClass_, attributeHandleValueMapConstructor_);
    if (result == nullptr) {
      clearJavaException(environment);
      throw rti::FederateInternalError(L"Umbra could not create a Java attribute-value map.");
    }
    try {
      for (auto const& [attribute, value] : values) {
        jbyteArray encoded = javaByteArray(environment, attribute.encode());
        jobject javaAttribute = environment->NewObject(
            attributeHandleClass_, attributeHandleConstructor_, encoded);
        if (encoded != nullptr) environment->DeleteLocalRef(encoded);
        jbyteArray javaValue = javaByteArray(environment, value);
        if (javaAttribute == nullptr || javaValue == nullptr) {
          if (javaAttribute != nullptr) environment->DeleteLocalRef(javaAttribute);
          if (javaValue != nullptr) environment->DeleteLocalRef(javaValue);
          clearJavaException(environment);
          throw rti::FederateInternalError(L"Umbra could not create a Java object attribute.");
        }
        jobject previous = environment->CallObjectMethod(
            result, attributeHandleValueMapPut_, javaAttribute, javaValue);
        if (previous != nullptr) environment->DeleteLocalRef(previous);
        environment->DeleteLocalRef(javaAttribute);
        environment->DeleteLocalRef(javaValue);
        ensureNoJavaException(environment);
      }
    } catch (...) {
      environment->DeleteLocalRef(result);
      throw;
    }
    return result;
  }

  static void ensureNoJavaException(JNIEnv* environment) {
    if (environment->ExceptionCheck()) {
      environment->ExceptionClear();
      throw rti::FederateInternalError(L"The Java federate callback raised an exception.");
    }
  }

  JavaVM* virtualMachine_;
  jlong nativeHandle_ = 0;
  std::mutex mutex_;
  jobject target_ = nullptr;
  jmethodID connectionLost_ = nullptr;
  jmethodID reportExecutions_ = nullptr;
  jmethodID reportMembers_ = nullptr;
  jmethodID missingFederation_ = nullptr;
  jmethodID federateResigned_ = nullptr;
  jmethodID synchronizationRegistrationSucceeded_ = nullptr;
  jmethodID synchronizationRegistrationFailed_ = nullptr;
  jmethodID announceSynchronizationPoint_ = nullptr;
  jmethodID federationSynchronized_ = nullptr;
  jmethodID federationSaveStatusResponse_ = nullptr;
  jmethodID initiateFederateSave_ = nullptr;
  jmethodID initiateFederateSaveWithTime_ = nullptr;
  jmethodID federationSaved_ = nullptr;
  jmethodID federationNotSaved_ = nullptr;
  jmethodID federationRestoreStatusResponse_ = nullptr;
  jmethodID requestFederationRestoreSucceeded_ = nullptr;
  jmethodID requestFederationRestoreFailed_ = nullptr;
  jmethodID federationRestoreBegun_ = nullptr;
  jmethodID initiateFederateRestore_ = nullptr;
  jmethodID federationRestored_ = nullptr;
  jmethodID federationNotRestored_ = nullptr;
  jmethodID startRegistrationForObjectClass_ = nullptr;
  jmethodID stopRegistrationForObjectClass_ = nullptr;
  jmethodID turnInteractionsOn_ = nullptr;
  jmethodID turnInteractionsOff_ = nullptr;
  jmethodID turnUpdatesOnForObjectInstance_ = nullptr;
  jmethodID turnUpdatesOnForObjectInstanceWithRate_ = nullptr;
  jmethodID turnUpdatesOffForObjectInstance_ = nullptr;
  jmethodID receiveInteraction_ = nullptr;
  jmethodID receiveInteractionWithTime_ = nullptr;
  jmethodID receiveDirectedInteraction_ = nullptr;
  jmethodID receiveDirectedInteractionWithTime_ = nullptr;
  jmethodID requestAttributeOwnershipAssumption_ = nullptr;
  jmethodID requestDivestitureConfirmation_ = nullptr;
  jmethodID attributeOwnershipAcquisitionNotification_ = nullptr;
  jmethodID attributeOwnershipUnavailable_ = nullptr;
  jmethodID requestAttributeOwnershipRelease_ = nullptr;
  jmethodID confirmAttributeOwnershipAcquisitionCancellation_ = nullptr;
  jmethodID informAttributeOwnership_ = nullptr;
  jmethodID attributeIsNotOwned_ = nullptr;
  jmethodID attributeIsOwnedByRTI_ = nullptr;
  jmethodID discoverObjectInstance_ = nullptr;
  jmethodID reflectAttributeValues_ = nullptr;
  jmethodID reflectAttributeValuesWithTime_ = nullptr;
  jmethodID provideAttributeValueUpdate_ = nullptr;
  jmethodID reportAttributeTransportationType_ = nullptr;
  jmethodID reportInteractionTransportationType_ = nullptr;
  jmethodID confirmAttributeTransportationTypeChange_ = nullptr;
  jmethodID confirmInteractionTransportationTypeChange_ = nullptr;
  jmethodID attributesInScope_ = nullptr;
  jmethodID attributesOutOfScope_ = nullptr;
  jmethodID timeAdvanceGrant_ = nullptr;
  jmethodID flushQueueGrant_ = nullptr;
  jmethodID timeConstrainedEnabled_ = nullptr;
  jmethodID timeRegulationEnabled_ = nullptr;
  jmethodID requestRetraction_ = nullptr;
  jmethodID removeObjectInstance_ = nullptr;
  jmethodID removeObjectInstanceWithTime_ = nullptr;
  jmethodID objectInstanceNameReservationSucceeded_ = nullptr;
  jmethodID objectInstanceNameReservationFailed_ = nullptr;
  jmethodID multipleObjectInstanceNameReservationSucceeded_ = nullptr;
  jmethodID multipleObjectInstanceNameReservationFailed_ = nullptr;
  jclass executionInformationSetClass_ = nullptr;
  jclass executionInformationClass_ = nullptr;
  jclass memberInformationSetClass_ = nullptr;
  jclass memberInformationClass_ = nullptr;
  jclass federateHandleClass_ = nullptr;
  jclass federateHandleSetClass_ = nullptr;
  jclass objectInstanceHandleClass_ = nullptr;
  jclass objectClassHandleClass_ = nullptr;
  jclass attributeHandleClass_ = nullptr;
  jclass attributeHandleSetClass_ = nullptr;
  jclass regionHandleClass_ = nullptr;
  jclass regionHandleSetClass_ = nullptr;
  jclass attributeHandleValueMapClass_ = nullptr;
  jclass objectInstanceNameSetClass_ = nullptr;
  jclass interactionClassHandleClass_ = nullptr;
  jclass parameterHandleClass_ = nullptr;
  jclass parameterHandleValueMapClass_ = nullptr;
  jclass transportationTypeHandleClass_ = nullptr;
  jclass saveStatusClass_ = nullptr;
  jclass saveFailureReasonClass_ = nullptr;
  jclass synchronizationPointFailureReasonClass_ = nullptr;
  jclass restoreStatusClass_ = nullptr;
  jclass restoreFailureReasonClass_ = nullptr;
  jclass federateHandleSaveStatusPairClass_ = nullptr;
  jclass federateRestoreStatusClass_ = nullptr;
  jmethodID executionInformationSetConstructor_ = nullptr;
  jmethodID executionInformationConstructor_ = nullptr;
  jmethodID memberInformationSetConstructor_ = nullptr;
  jmethodID memberInformationConstructor_ = nullptr;
  jmethodID setAdd_ = nullptr;
  jmethodID memberSetAdd_ = nullptr;
  jmethodID federateHandleConstructor_ = nullptr;
  jmethodID federateHandleSetConstructor_ = nullptr;
  jmethodID objectInstanceHandleConstructor_ = nullptr;
  jmethodID objectClassHandleConstructor_ = nullptr;
  jmethodID attributeHandleConstructor_ = nullptr;
  jmethodID attributeHandleSetConstructor_ = nullptr;
  jmethodID regionHandleConstructor_ = nullptr;
  jmethodID regionHandleSetConstructor_ = nullptr;
  jmethodID attributeHandleValueMapConstructor_ = nullptr;
  jmethodID objectInstanceNameSetConstructor_ = nullptr;
  jmethodID interactionClassHandleConstructor_ = nullptr;
  jmethodID parameterHandleConstructor_ = nullptr;
  jmethodID parameterHandleValueMapConstructor_ = nullptr;
  jmethodID transportationTypeHandleConstructor_ = nullptr;
  jmethodID federateHandleSetAdd_ = nullptr;
  jmethodID attributeHandleSetAdd_ = nullptr;
  jmethodID regionHandleSetAdd_ = nullptr;
  jmethodID attributeHandleValueMapPut_ = nullptr;
  jmethodID objectInstanceNameSetAdd_ = nullptr;
  jmethodID parameterHandleValueMapPut_ = nullptr;
  jmethodID saveStatusValueOf_ = nullptr;
  jmethodID saveFailureReasonValueOf_ = nullptr;
  jmethodID synchronizationPointFailureReasonValueOf_ = nullptr;
  jmethodID restoreStatusValueOf_ = nullptr;
  jmethodID restoreFailureReasonValueOf_ = nullptr;
  jmethodID federateHandleSaveStatusPairConstructor_ = nullptr;
  jmethodID federateRestoreStatusConstructor_ = nullptr;
};

rti::CallbackModel callbackModel(std::wstring const& value) {
  if (value == L"HLA_IMMEDIATE") {
    return rti::HLA_IMMEDIATE;
  }
  if (value == L"HLA_EVOKED") {
    return rti::HLA_EVOKED;
  }
  throw rti::UnsupportedCallbackModel(L"The Java JNI bridge received an unknown callback model.");
}

rti::OrderType orderTypeFromJava(std::wstring const& value) {
  if (value == L"RECEIVE" || value == L"Receive") return rti::RECEIVE;
  if (value == L"TIMESTAMP" || value == L"TimeStamp") return rti::TIMESTAMP;
  throw rti::InvalidOrderName(L"The Java JNI bridge received an unknown order type.");
}

std::wstring orderTypeName(rti::OrderType value) {
  switch (value) {
    case rti::RECEIVE: return L"RECEIVE";
    case rti::TIMESTAMP: return L"TIMESTAMP";
  }
  throw rti::InvalidOrderType(L"The C++ RTI returned an unknown order type.");
}

std::optional<rti::RtiConfiguration> configurationFromJava(
    JNIEnv* environment, jobject configuration) {
  if (configuration == nullptr) {
    return std::nullopt;
  }
  jclass type = environment->GetObjectClass(configuration);
  if (type == nullptr) {
    clearJavaException(environment);
    throw rti::RTIinternalError(L"Umbra could not inspect the Java RTI configuration.");
  }
  auto const configurationName = requiredMethod(environment, type, "configurationName", "()Ljava/lang/String;");
  auto const rtiAddress = requiredMethod(environment, type, "rtiAddress", "()Ljava/lang/String;");
  auto const additionalSettings = requiredMethod(
      environment, type, "additionalSettings", "()Ljava/lang/String;");
  jstring name = static_cast<jstring>(environment->CallObjectMethod(configuration, configurationName));
  jstring address = static_cast<jstring>(environment->CallObjectMethod(configuration, rtiAddress));
  jstring settings = static_cast<jstring>(environment->CallObjectMethod(configuration, additionalSettings));
  environment->DeleteLocalRef(type);
  if (environment->ExceptionCheck()) {
    if (name != nullptr) environment->DeleteLocalRef(name);
    if (address != nullptr) environment->DeleteLocalRef(address);
    if (settings != nullptr) environment->DeleteLocalRef(settings);
    clearJavaException(environment);
    throw rti::RTIinternalError(L"Umbra could not read the Java RTI configuration.");
  }
  rti::RtiConfiguration result = rti::RtiConfiguration::createConfiguration();
  result.withConfigurationName(wideString(environment, name));
  result.withRtiAddress(wideString(environment, address));
  result.withAdditionalSettings(wideString(environment, settings));
  if (name != nullptr) environment->DeleteLocalRef(name);
  if (address != nullptr) environment->DeleteLocalRef(address);
  if (settings != nullptr) environment->DeleteLocalRef(settings);
  return result;
}

std::optional<rti::Credentials> credentialsFromJava(
    JNIEnv* environment, jobject credentials) {
  if (credentials == nullptr) {
    return std::nullopt;
  }
  jclass type = environment->GetObjectClass(credentials);
  if (type == nullptr) {
    clearJavaException(environment);
    throw rti::RTIinternalError(L"Umbra could not inspect the Java credentials.");
  }
  auto const getType = requiredMethod(environment, type, "getType", "()Ljava/lang/String;");
  auto const getData = requiredMethod(environment, type, "getData", "()[B");
  jstring credentialType = static_cast<jstring>(environment->CallObjectMethod(credentials, getType));
  auto data = static_cast<jbyteArray>(environment->CallObjectMethod(credentials, getData));
  environment->DeleteLocalRef(type);
  if (environment->ExceptionCheck()) {
    if (credentialType != nullptr) environment->DeleteLocalRef(credentialType);
    if (data != nullptr) environment->DeleteLocalRef(data);
    clearJavaException(environment);
    throw rti::RTIinternalError(L"Umbra could not read the Java credentials.");
  }
  std::wstring typeName = wideString(environment, credentialType);
  std::vector<rti::Octet> encoded;
  if (data != nullptr) {
    encoded = octetVector(environment, data);
  }
  if (credentialType != nullptr) environment->DeleteLocalRef(credentialType);
  if (data != nullptr) environment->DeleteLocalRef(data);
  rti::VariableLengthData value(
      encoded.empty() ? nullptr : encoded.data(), encoded.size());
  return rti::Credentials(std::move(typeName), value);
}

jobject configurationResultToJava(
    JNIEnv* environment, rti::ConfigurationResult const& result) {
  char const* resultName = "SETTINGS_IGNORED";
  switch (result.additionalSettingsResult) {
    case rti::SETTINGS_IGNORED:
      break;
    case rti::SETTINGS_FAILED_TO_PARSE:
      resultName = "SETTINGS_FAILED_TO_PARSE";
      break;
    case rti::SETTINGS_APPLIED:
      resultName = "SETTINGS_APPLIED";
      break;
  }
  jclass enumClass = environment->FindClass("hla/rti1516_2025/AdditionalSettingsResultCode");
  jclass resultClass = environment->FindClass("hla/rti1516_2025/ConfigurationResult");
  if (enumClass == nullptr || resultClass == nullptr) {
    if (enumClass != nullptr) environment->DeleteLocalRef(enumClass);
    if (resultClass != nullptr) environment->DeleteLocalRef(resultClass);
    clearJavaException(environment);
    throw std::runtime_error("Could not construct Java ConfigurationResult");
  }
  auto const resultField = environment->GetStaticFieldID(
      enumClass,
      resultName,
      "Lhla/rti1516_2025/AdditionalSettingsResultCode;");
  auto const constructor = environment->GetMethodID(
      resultClass,
      "<init>",
      "(ZZLhla/rti1516_2025/AdditionalSettingsResultCode;Ljava/lang/String;)V");
  if (resultField == nullptr || constructor == nullptr) {
    environment->DeleteLocalRef(enumClass);
    environment->DeleteLocalRef(resultClass);
    clearJavaException(environment);
    throw std::runtime_error("Could not resolve Java ConfigurationResult members");
  }
  jobject enumValue = environment->GetStaticObjectField(enumClass, resultField);
  jstring message = javaString(environment, result.message);
  jobject javaResult = environment->NewObject(
      resultClass,
      constructor,
      static_cast<jboolean>(result.configurationUsed),
      static_cast<jboolean>(result.addressUsed),
      enumValue,
      message);
  if (enumValue != nullptr) environment->DeleteLocalRef(enumValue);
  if (message != nullptr) environment->DeleteLocalRef(message);
  environment->DeleteLocalRef(enumClass);
  environment->DeleteLocalRef(resultClass);
  if (javaResult == nullptr) {
    clearJavaException(environment);
    throw std::runtime_error("Could not create Java ConfigurationResult");
  }
  return javaResult;
}

jobject authorizationResultToJava(
    JNIEnv* environment,
    rti::AuthorizationResult const& result) {
  char const* codeName = "AUTHORIZATION_ERROR";
  switch (result.getCode()) {
    case rti::AuthorizationResult::AUTHORIZED:
      codeName = "AUTHORIZED";
      break;
    case rti::AuthorizationResult::UNAUTHORIZED:
      codeName = "UNAUTHORIZED";
      break;
    case rti::AuthorizationResult::INVALID_CREDENTIALS:
      codeName = "INVALID_CREDENTIALS";
      break;
    case rti::AuthorizationResult::AUTHORIZATION_ERROR:
      break;
  }
  jclass codeClass = environment->FindClass(
      "hla/rti1516_2025/auth/AuthorizationResult$Code");
  jclass resultClass = environment->FindClass(
      "hla/rti1516_2025/auth/AuthorizationResult");
  if (codeClass == nullptr || resultClass == nullptr) {
    if (codeClass != nullptr) environment->DeleteLocalRef(codeClass);
    if (resultClass != nullptr) environment->DeleteLocalRef(resultClass);
    clearJavaException(environment);
    throw std::runtime_error("Could not construct Java AuthorizationResult");
  }
  auto const codeField = environment->GetStaticFieldID(
      codeClass,
      codeName,
      "Lhla/rti1516_2025/auth/AuthorizationResult$Code;");
  auto const constructor = environment->GetMethodID(
      resultClass,
      "<init>",
      "(Lhla/rti1516_2025/auth/AuthorizationResult$Code;Ljava/lang/String;)V");
  if (codeField == nullptr || constructor == nullptr) {
    environment->DeleteLocalRef(codeClass);
    environment->DeleteLocalRef(resultClass);
    clearJavaException(environment);
    throw std::runtime_error("Could not resolve Java AuthorizationResult members");
  }
  jobject code = environment->GetStaticObjectField(codeClass, codeField);
  jstring message = javaString(environment, result.getMessage());
  jobject javaResult = environment->NewObject(resultClass, constructor, code, message);
  if (code != nullptr) environment->DeleteLocalRef(code);
  if (message != nullptr) environment->DeleteLocalRef(message);
  environment->DeleteLocalRef(codeClass);
  environment->DeleteLocalRef(resultClass);
  if (javaResult == nullptr) {
    clearJavaException(environment);
    throw std::runtime_error("Could not create Java AuthorizationResult");
  }
  return javaResult;
}

class NativeState final {
 public:
  NativeState(JNIEnv* environment, JavaVM* virtualMachine)
      : callbacks_(environment, virtualMachine) {
    callbacks_.setNativeHandle(reinterpret_cast<jlong>(this));
  }

  rti::ConfigurationResult connect(
      JNIEnv* environment,
      jobject callbackTarget,
      std::wstring const& callbackModelName,
      jobject javaConfiguration,
      jobject javaCredentials) {
    auto const selectedCallbackModel = callbackModel(callbackModelName);
    auto const configuration = configurationFromJava(environment, javaConfiguration);
    auto const credentials = credentialsFromJava(environment, javaCredentials);
    if (connected_) {
      return connectNative(selectedCallbackModel, configuration, credentials);
    }
    callbacks_.setTarget(environment, callbackTarget);
    try {
      auto result = connectNative(selectedCallbackModel, configuration, credentials);
      connected_ = true;
      return result;
    } catch (...) {
      callbacks_.clearTarget(environment);
      throw;
    }
  }

  void disconnect(JNIEnv* environment) {
    ambassador_.disconnect();
    connected_ = false;
    callbacks_.clearTarget(environment);
  }

  bool failEmbeddedTransportConnectionForTesting(std::wstring const& faultDescription) {
    return umbra::detail::failEmbeddedTransportConnectionForTesting(
        ambassador_, faultDescription);
  }

  bool forceEmbeddedFederateResignationForTesting(std::wstring const& reason) {
    return umbra::detail::forceEmbeddedFederateResignationForTesting(
        ambassador_, reason);
  }

  bool evokeCallback(double minimumTime) { return ambassador_.evokeCallback(minimumTime); }

  bool evokeMultipleCallbacks(double minimumTime, double maximumTime) {
    return ambassador_.evokeMultipleCallbacks(minimumTime, maximumTime);
  }

  void enableCallbacks() { ambassador_.enableCallbacks(); }

  void disableCallbacks() { ambassador_.disableCallbacks(); }

  void listFederationExecutions() { ambassador_.listFederationExecutions(); }

  void listFederationExecutionMembers(std::wstring const& federationName) {
    ambassador_.listFederationExecutionMembers(federationName);
  }

  void createFederationExecution(
      std::wstring const& federationName,
      std::vector<std::wstring> const& fomModules,
      std::wstring const& logicalTimeImplementationName) {
    ambassador_.createFederationExecution(
        federationName, fomModules, logicalTimeImplementationName);
  }

  void createFederationExecutionWithMIM(
      std::wstring const& federationName,
      std::vector<std::wstring> const& fomModules,
      std::wstring const& mimModule,
      std::wstring const& logicalTimeImplementationName) {
    ambassador_.createFederationExecutionWithMIM(
        federationName, fomModules, mimModule, logicalTimeImplementationName);
  }

  void destroyFederationExecution(std::wstring const& federationName) {
    ambassador_.destroyFederationExecution(federationName);
  }

  rti::FederateHandle joinFederationExecution(
      std::optional<std::wstring> federateName,
      std::wstring const& federateType,
      std::wstring const& federationName,
      std::vector<std::wstring> const& additionalFomModules) {
    if (federateName) {
      return ambassador_.joinFederationExecution(
          *federateName, federateType, federationName, additionalFomModules);
    }
    return ambassador_.joinFederationExecution(
        federateType, federationName, additionalFomModules);
  }

  void resignFederationExecution(rti::ResignAction resignAction) {
    ambassador_.resignFederationExecution(resignAction);
  }

  rti::FederateHandle getFederateHandle(std::wstring const& federateName) {
    return ambassador_.getFederateHandle(federateName);
  }

  std::wstring getFederateName(rti::VariableLengthData const& encodedFederateHandle) {
    return ambassador_.getFederateName(
        ambassador_.decodeFederateHandle(encodedFederateHandle));
  }

  unsigned long normalizeFederateHandle(
      rti::VariableLengthData const& encodedFederateHandle) {
    return ambassador_.normalizeFederateHandle(
        ambassador_.decodeFederateHandle(encodedFederateHandle));
  }

  rti::ObjectClassHandle getObjectClassHandle(std::wstring const& objectClassName) {
    return ambassador_.getObjectClassHandle(objectClassName);
  }

  std::wstring getObjectClassName(
      rti::VariableLengthData const& encodedObjectClassHandle) {
    return ambassador_.getObjectClassName(
        ambassador_.decodeObjectClassHandle(encodedObjectClassHandle));
  }

  rti::ObjectClassHandle getKnownObjectClassHandle(
      rti::VariableLengthData const& encodedObjectInstanceHandle) {
    return ambassador_.getKnownObjectClassHandle(
        ambassador_.decodeObjectInstanceHandle(encodedObjectInstanceHandle));
  }

  unsigned long normalizeObjectClassHandle(
      rti::VariableLengthData const& encodedObjectClassHandle) {
    return ambassador_.normalizeObjectClassHandle(
        ambassador_.decodeObjectClassHandle(encodedObjectClassHandle));
  }

  rti::AttributeHandle getAttributeHandle(
      rti::VariableLengthData const& encodedObjectClassHandle,
      std::wstring const& attributeName) {
    return ambassador_.getAttributeHandle(
        ambassador_.decodeObjectClassHandle(encodedObjectClassHandle), attributeName);
  }

  std::wstring getAttributeName(
      rti::VariableLengthData const& encodedObjectClassHandle,
      rti::VariableLengthData const& encodedAttributeHandle) {
    return ambassador_.getAttributeName(
        ambassador_.decodeObjectClassHandle(encodedObjectClassHandle),
        ambassador_.decodeAttributeHandle(encodedAttributeHandle));
  }

  rti::DimensionHandle getDimensionHandle(std::wstring const& dimensionName) {
    return ambassador_.getDimensionHandle(dimensionName);
  }

  std::wstring getDimensionName(rti::VariableLengthData const& encodedDimensionHandle) {
    return ambassador_.getDimensionName(
        rti::umbra_binding_detail::decodeDimensionHandle(encodedDimensionHandle));
  }

  rti::DimensionHandleSet getAvailableDimensionsForObjectClass(
      rti::VariableLengthData const& encodedObjectClassHandle) {
    return ambassador_.getAvailableDimensionsForObjectClass(
        ambassador_.decodeObjectClassHandle(encodedObjectClassHandle));
  }

  rti::DimensionHandleSet getAvailableDimensionsForInteractionClass(
      rti::VariableLengthData const& encodedInteractionClassHandle) {
    return ambassador_.getAvailableDimensionsForInteractionClass(
        ambassador_.decodeInteractionClassHandle(encodedInteractionClassHandle));
  }

  unsigned long getDimensionUpperBound(
      rti::VariableLengthData const& encodedDimensionHandle) {
    return ambassador_.getDimensionUpperBound(
        rti::umbra_binding_detail::decodeDimensionHandle(encodedDimensionHandle));
  }

  rti::RegionHandle createRegion(rti::DimensionHandleSet const& dimensions) {
    return ambassador_.createRegion(dimensions);
  }

  void commitRegionModifications(rti::RegionHandleSet const& regions) {
    ambassador_.commitRegionModifications(regions);
  }

  void deleteRegion(rti::VariableLengthData const& encodedRegionHandle) {
    ambassador_.deleteRegion(
        rti::umbra_binding_detail::decodeRegionHandle(encodedRegionHandle));
  }

  rti::DimensionHandleSet getDimensionHandleSet(
      rti::VariableLengthData const& encodedRegionHandle) {
    return ambassador_.getDimensionHandleSet(
        rti::umbra_binding_detail::decodeRegionHandle(encodedRegionHandle));
  }

  rti::RangeBounds getRangeBounds(
      rti::VariableLengthData const& encodedRegionHandle,
      rti::VariableLengthData const& encodedDimensionHandle) {
    return ambassador_.getRangeBounds(
        rti::umbra_binding_detail::decodeRegionHandle(encodedRegionHandle),
        rti::umbra_binding_detail::decodeDimensionHandle(encodedDimensionHandle));
  }

  void setRangeBounds(
      rti::VariableLengthData const& encodedRegionHandle,
      rti::VariableLengthData const& encodedDimensionHandle,
      jlong lowerBound,
      jlong upperBound) {
    if (lowerBound < 0 || upperBound < 0 ||
        static_cast<unsigned long long>(lowerBound) >
            static_cast<unsigned long long>(std::numeric_limits<unsigned long>::max()) ||
        static_cast<unsigned long long>(upperBound) >
            static_cast<unsigned long long>(std::numeric_limits<unsigned long>::max())) {
      throw rti::InvalidRangeBound(L"Java range bounds are outside the C++ unsigned-long domain.");
    }
    ambassador_.setRangeBounds(
        rti::umbra_binding_detail::decodeRegionHandle(encodedRegionHandle),
        rti::umbra_binding_detail::decodeDimensionHandle(encodedDimensionHandle),
        rti::RangeBounds(
            static_cast<unsigned long>(lowerBound), static_cast<unsigned long>(upperBound)));
  }

  void publishObjectClassAttributes(
      rti::VariableLengthData const& encodedObjectClassHandle,
      rti::AttributeHandleSet const& attributes) {
    ambassador_.publishObjectClassAttributes(
        ambassador_.decodeObjectClassHandle(encodedObjectClassHandle), attributes);
  }

  void unpublishObjectClass(rti::VariableLengthData const& encodedObjectClassHandle) {
    ambassador_.unpublishObjectClass(
        ambassador_.decodeObjectClassHandle(encodedObjectClassHandle));
  }

  void unpublishObjectClassAttributes(
      rti::VariableLengthData const& encodedObjectClassHandle,
      rti::AttributeHandleSet const& attributes) {
    ambassador_.unpublishObjectClassAttributes(
        ambassador_.decodeObjectClassHandle(encodedObjectClassHandle), attributes);
  }

  void subscribeObjectClassAttributes(
      rti::VariableLengthData const& encodedObjectClassHandle,
      rti::AttributeHandleSet const& attributes,
      bool active,
      std::wstring const& updateRateDesignator) {
    ambassador_.subscribeObjectClassAttributes(
        ambassador_.decodeObjectClassHandle(encodedObjectClassHandle),
        attributes,
        active,
        updateRateDesignator);
  }

  void unsubscribeObjectClass(rti::VariableLengthData const& encodedObjectClassHandle) {
    ambassador_.unsubscribeObjectClass(
        ambassador_.decodeObjectClassHandle(encodedObjectClassHandle));
  }

  void unsubscribeObjectClassAttributes(
      rti::VariableLengthData const& encodedObjectClassHandle,
      rti::AttributeHandleSet const& attributes) {
    ambassador_.unsubscribeObjectClassAttributes(
        ambassador_.decodeObjectClassHandle(encodedObjectClassHandle), attributes);
  }

  void subscribeObjectClassAttributesWithRegions(
      rti::VariableLengthData const& encodedObjectClassHandle,
      rti::AttributeHandleSetRegionHandleSetPairVector const& attributesAndRegions,
      bool active,
      std::wstring const& updateRateDesignator) {
    ambassador_.subscribeObjectClassAttributesWithRegions(
        ambassador_.decodeObjectClassHandle(encodedObjectClassHandle),
        attributesAndRegions,
        active,
        updateRateDesignator);
  }

  void unsubscribeObjectClassAttributesWithRegions(
      rti::VariableLengthData const& encodedObjectClassHandle,
      rti::AttributeHandleSetRegionHandleSetPairVector const& attributesAndRegions) {
    ambassador_.unsubscribeObjectClassAttributesWithRegions(
        ambassador_.decodeObjectClassHandle(encodedObjectClassHandle), attributesAndRegions);
  }

  void reserveObjectInstanceName(std::wstring const& objectInstanceName) {
    ambassador_.reserveObjectInstanceName(objectInstanceName);
  }

  void releaseObjectInstanceName(std::wstring const& objectInstanceName) {
    ambassador_.releaseObjectInstanceName(objectInstanceName);
  }

  void reserveMultipleObjectInstanceNames(
      std::set<std::wstring> const& objectInstanceNames) {
    ambassador_.reserveMultipleObjectInstanceNames(objectInstanceNames);
  }

  void releaseMultipleObjectInstanceNames(
      std::set<std::wstring> const& objectInstanceNames) {
    ambassador_.releaseMultipleObjectInstanceNames(objectInstanceNames);
  }

  rti::ObjectInstanceHandle registerObjectInstance(
      rti::VariableLengthData const& encodedObjectClassHandle,
      std::optional<std::wstring> objectInstanceName) {
    auto const objectClass = ambassador_.decodeObjectClassHandle(encodedObjectClassHandle);
    if (objectInstanceName) {
      return ambassador_.registerObjectInstance(objectClass, *objectInstanceName);
    }
    return ambassador_.registerObjectInstance(objectClass);
  }

  rti::ObjectInstanceHandle registerObjectInstanceWithRegions(
      rti::VariableLengthData const& encodedObjectClassHandle,
      rti::AttributeHandleSetRegionHandleSetPairVector const& attributesAndRegions,
      std::optional<std::wstring> objectInstanceName) {
    auto const objectClass = ambassador_.decodeObjectClassHandle(encodedObjectClassHandle);
    if (objectInstanceName) {
      return ambassador_.registerObjectInstanceWithRegions(
          objectClass, attributesAndRegions, *objectInstanceName);
    }
    return ambassador_.registerObjectInstanceWithRegions(objectClass, attributesAndRegions);
  }

  void associateRegionsForUpdates(
      rti::VariableLengthData const& encodedObjectInstanceHandle,
      rti::AttributeHandleSetRegionHandleSetPairVector const& attributesAndRegions) {
    ambassador_.associateRegionsForUpdates(
        ambassador_.decodeObjectInstanceHandle(encodedObjectInstanceHandle), attributesAndRegions);
  }

  void unassociateRegionsForUpdates(
      rti::VariableLengthData const& encodedObjectInstanceHandle,
      rti::AttributeHandleSetRegionHandleSetPairVector const& attributesAndRegions) {
    ambassador_.unassociateRegionsForUpdates(
        ambassador_.decodeObjectInstanceHandle(encodedObjectInstanceHandle), attributesAndRegions);
  }

  rti::ObjectInstanceHandle getObjectInstanceHandle(
      std::wstring const& objectInstanceName) {
    return ambassador_.getObjectInstanceHandle(objectInstanceName);
  }

  std::wstring getObjectInstanceName(
      rti::VariableLengthData const& encodedObjectInstanceHandle) {
    return ambassador_.getObjectInstanceName(
        ambassador_.decodeObjectInstanceHandle(encodedObjectInstanceHandle));
  }

  unsigned long normalizeObjectInstanceHandle(
      rti::VariableLengthData const& encodedObjectInstanceHandle) {
    return ambassador_.normalizeObjectInstanceHandle(
        ambassador_.decodeObjectInstanceHandle(encodedObjectInstanceHandle));
  }

  void updateAttributeValues(
      rti::VariableLengthData const& encodedObjectInstanceHandle,
      rti::AttributeHandleValueMap const& attributeValues,
      rti::VariableLengthData const& userSuppliedTag) {
    ambassador_.updateAttributeValues(
        ambassador_.decodeObjectInstanceHandle(encodedObjectInstanceHandle),
        attributeValues,
        userSuppliedTag);
  }

  rti::VariableLengthData updateAttributeValuesWithTime(
      rti::VariableLengthData const& encodedObjectInstanceHandle,
      rti::AttributeHandleValueMap const& attributeValues,
      rti::VariableLengthData const& userSuppliedTag,
      rti::VariableLengthData const& encodedLogicalTime) {
    auto factory = ambassador_.getTimeFactory();
    auto time = factory->decodeLogicalTime(encodedLogicalTime);
    auto retraction = ambassador_.updateAttributeValues(
        ambassador_.decodeObjectInstanceHandle(encodedObjectInstanceHandle),
        attributeValues,
        userSuppliedTag,
        *time);
    return retraction.isValid() ? retraction.encode() : rti::VariableLengthData();
  }

  void deleteObjectInstance(
      rti::VariableLengthData const& encodedObjectInstanceHandle,
      rti::VariableLengthData const& userSuppliedTag) {
    ambassador_.deleteObjectInstance(
        ambassador_.decodeObjectInstanceHandle(encodedObjectInstanceHandle),
        userSuppliedTag);
  }

  rti::VariableLengthData deleteObjectInstanceWithTime(
      rti::VariableLengthData const& encodedObjectInstanceHandle,
      rti::VariableLengthData const& userSuppliedTag,
      rti::VariableLengthData const& encodedLogicalTime) {
    auto factory = ambassador_.getTimeFactory();
    auto time = factory->decodeLogicalTime(encodedLogicalTime);
    auto retraction = ambassador_.deleteObjectInstance(
        ambassador_.decodeObjectInstanceHandle(encodedObjectInstanceHandle),
        userSuppliedTag,
        *time);
    return retraction.isValid() ? retraction.encode() : rti::VariableLengthData();
  }

  void localDeleteObjectInstance(
      rti::VariableLengthData const& encodedObjectInstanceHandle) {
    ambassador_.localDeleteObjectInstance(
        ambassador_.decodeObjectInstanceHandle(encodedObjectInstanceHandle));
  }

  void requestAttributeValueUpdateForObjectInstance(
      rti::VariableLengthData const& encodedObjectInstanceHandle,
      rti::AttributeHandleSet const& attributes,
      rti::VariableLengthData const& userSuppliedTag) {
    ambassador_.requestAttributeValueUpdate(
        ambassador_.decodeObjectInstanceHandle(encodedObjectInstanceHandle),
        attributes,
        userSuppliedTag);
  }

  void requestAttributeValueUpdateForObjectClass(
      rti::VariableLengthData const& encodedObjectClassHandle,
      rti::AttributeHandleSet const& attributes,
      rti::VariableLengthData const& userSuppliedTag) {
    ambassador_.requestAttributeValueUpdate(
        ambassador_.decodeObjectClassHandle(encodedObjectClassHandle),
        attributes,
        userSuppliedTag);
  }

  void requestAttributeValueUpdateWithRegions(
      rti::VariableLengthData const& encodedObjectClassHandle,
      rti::AttributeHandleSetRegionHandleSetPairVector const& attributesAndRegions,
      rti::VariableLengthData const& userSuppliedTag) {
    ambassador_.requestAttributeValueUpdateWithRegions(
        ambassador_.decodeObjectClassHandle(encodedObjectClassHandle),
        attributesAndRegions,
        userSuppliedTag);
  }

  void queryAttributeOwnership(
      rti::VariableLengthData const& encodedObjectInstanceHandle,
      rti::AttributeHandleSet const& attributes) {
    ambassador_.queryAttributeOwnership(
        ambassador_.decodeObjectInstanceHandle(encodedObjectInstanceHandle), attributes);
  }

  bool isAttributeOwnedByFederate(
      rti::VariableLengthData const& encodedObjectInstanceHandle,
      rti::VariableLengthData const& encodedAttributeHandle) {
    return ambassador_.isAttributeOwnedByFederate(
        ambassador_.decodeObjectInstanceHandle(encodedObjectInstanceHandle),
        ambassador_.decodeAttributeHandle(encodedAttributeHandle));
  }

  void unconditionalAttributeOwnershipDivestiture(
      rti::VariableLengthData const& encodedObjectInstanceHandle,
      rti::AttributeHandleSet const& attributes,
      rti::VariableLengthData const& userSuppliedTag) {
    ambassador_.unconditionalAttributeOwnershipDivestiture(
        ambassador_.decodeObjectInstanceHandle(encodedObjectInstanceHandle),
        attributes,
        userSuppliedTag);
  }

  void negotiatedAttributeOwnershipDivestiture(
      rti::VariableLengthData const& encodedObjectInstanceHandle,
      rti::AttributeHandleSet const& attributes,
      rti::VariableLengthData const& userSuppliedTag) {
    ambassador_.negotiatedAttributeOwnershipDivestiture(
        ambassador_.decodeObjectInstanceHandle(encodedObjectInstanceHandle),
        attributes,
        userSuppliedTag);
  }

  void confirmDivestiture(
      rti::VariableLengthData const& encodedObjectInstanceHandle,
      rti::AttributeHandleSet const& attributes,
      rti::VariableLengthData const& userSuppliedTag) {
    ambassador_.confirmDivestiture(
        ambassador_.decodeObjectInstanceHandle(encodedObjectInstanceHandle),
        attributes,
        userSuppliedTag);
  }

  void cancelNegotiatedAttributeOwnershipDivestiture(
      rti::VariableLengthData const& encodedObjectInstanceHandle,
      rti::AttributeHandleSet const& attributes) {
    ambassador_.cancelNegotiatedAttributeOwnershipDivestiture(
        ambassador_.decodeObjectInstanceHandle(encodedObjectInstanceHandle), attributes);
  }

  void attributeOwnershipAcquisition(
      rti::VariableLengthData const& encodedObjectInstanceHandle,
      rti::AttributeHandleSet const& attributes,
      rti::VariableLengthData const& userSuppliedTag) {
    ambassador_.attributeOwnershipAcquisition(
        ambassador_.decodeObjectInstanceHandle(encodedObjectInstanceHandle),
        attributes,
        userSuppliedTag);
  }

  void attributeOwnershipAcquisitionIfAvailable(
      rti::VariableLengthData const& encodedObjectInstanceHandle,
      rti::AttributeHandleSet const& attributes,
      rti::VariableLengthData const& userSuppliedTag) {
    ambassador_.attributeOwnershipAcquisitionIfAvailable(
        ambassador_.decodeObjectInstanceHandle(encodedObjectInstanceHandle),
        attributes,
        userSuppliedTag);
  }

  void cancelAttributeOwnershipAcquisition(
      rti::VariableLengthData const& encodedObjectInstanceHandle,
      rti::AttributeHandleSet const& attributes) {
    ambassador_.cancelAttributeOwnershipAcquisition(
        ambassador_.decodeObjectInstanceHandle(encodedObjectInstanceHandle), attributes);
  }

  void attributeOwnershipReleaseDenied(
      rti::VariableLengthData const& encodedObjectInstanceHandle,
      rti::AttributeHandleSet const& attributes,
      rti::VariableLengthData const& userSuppliedTag) {
    ambassador_.attributeOwnershipReleaseDenied(
        ambassador_.decodeObjectInstanceHandle(encodedObjectInstanceHandle),
        attributes,
        userSuppliedTag);
  }

  rti::AttributeHandleSet attributeOwnershipDivestitureIfWanted(
      rti::VariableLengthData const& encodedObjectInstanceHandle,
      rti::AttributeHandleSet const& attributes,
      rti::VariableLengthData const& userSuppliedTag) {
    rti::AttributeHandleSet divestedAttributes;
    ambassador_.attributeOwnershipDivestitureIfWanted(
        ambassador_.decodeObjectInstanceHandle(encodedObjectInstanceHandle),
        attributes,
        userSuppliedTag,
        divestedAttributes);
    return divestedAttributes;
  }

  void queryAttributeTransportationType(
      rti::VariableLengthData const& encodedObjectInstanceHandle,
      rti::VariableLengthData const& encodedAttributeHandle) {
    ambassador_.queryAttributeTransportationType(
        ambassador_.decodeObjectInstanceHandle(encodedObjectInstanceHandle),
        ambassador_.decodeAttributeHandle(encodedAttributeHandle));
  }

  void queryInteractionTransportationType(
      rti::VariableLengthData const& encodedFederateHandle,
      rti::VariableLengthData const& encodedInteractionClassHandle) {
    ambassador_.queryInteractionTransportationType(
        ambassador_.decodeFederateHandle(encodedFederateHandle),
        ambassador_.decodeInteractionClassHandle(encodedInteractionClassHandle));
  }

  void requestAttributeTransportationTypeChange(
      rti::VariableLengthData const& encodedObjectInstanceHandle,
      rti::AttributeHandleSet const& attributes,
      rti::VariableLengthData const& encodedTransportationTypeHandle) {
    ambassador_.requestAttributeTransportationTypeChange(
        ambassador_.decodeObjectInstanceHandle(encodedObjectInstanceHandle),
        attributes,
        rti::umbra_binding_detail::decodeTransportationTypeHandle(
            encodedTransportationTypeHandle));
  }

  void requestInteractionTransportationTypeChange(
      rti::VariableLengthData const& encodedInteractionClassHandle,
      rti::VariableLengthData const& encodedTransportationTypeHandle) {
    ambassador_.requestInteractionTransportationTypeChange(
        ambassador_.decodeInteractionClassHandle(encodedInteractionClassHandle),
        rti::umbra_binding_detail::decodeTransportationTypeHandle(
            encodedTransportationTypeHandle));
  }

  void setAttributeScopeAdvisorySwitch(bool enabled) {
    ambassador_.setAttributeScopeAdvisorySwitch(enabled);
  }

  bool getObjectClassRelevanceAdvisorySwitch() const {
    return ambassador_.getObjectClassRelevanceAdvisorySwitch();
  }

  void setObjectClassRelevanceAdvisorySwitch(bool enabled) {
    ambassador_.setObjectClassRelevanceAdvisorySwitch(enabled);
  }

  bool getAttributeRelevanceAdvisorySwitch() const {
    return ambassador_.getAttributeRelevanceAdvisorySwitch();
  }

  void setAttributeRelevanceAdvisorySwitch(bool enabled) {
    ambassador_.setAttributeRelevanceAdvisorySwitch(enabled);
  }

  bool getAttributeScopeAdvisorySwitch() const {
    return ambassador_.getAttributeScopeAdvisorySwitch();
  }

  bool getInteractionRelevanceAdvisorySwitch() const {
    return ambassador_.getInteractionRelevanceAdvisorySwitch();
  }

  void setInteractionRelevanceAdvisorySwitch(bool enabled) {
    ambassador_.setInteractionRelevanceAdvisorySwitch(enabled);
  }

  rti::ResignAction getAutomaticResignDirective() {
    return ambassador_.getAutomaticResignDirective();
  }

  void setAutomaticResignDirective(rti::ResignAction resignAction) {
    ambassador_.setAutomaticResignDirective(resignAction);
  }

  bool getServiceReportingSwitch() const {
    return ambassador_.getServiceReportingSwitch();
  }

  void setServiceReportingSwitch(bool enabled) {
    ambassador_.setServiceReportingSwitch(enabled);
  }

  bool getExceptionReportingSwitch() const {
    return ambassador_.getExceptionReportingSwitch();
  }

  void setExceptionReportingSwitch(bool enabled) {
    ambassador_.setExceptionReportingSwitch(enabled);
  }

  bool getSendServiceReportsToFileSwitch() const {
    return ambassador_.getSendServiceReportsToFileSwitch();
  }

  void setSendServiceReportsToFileSwitch(bool enabled) {
    ambassador_.setSendServiceReportsToFileSwitch(enabled);
  }

  std::wstring getHLAversion() const {
    return L"IEEE 1516.1-2025";
  }

  bool getAutoProvideSwitch() const {
    return ambassador_.getAutoProvideSwitch();
  }

  bool getDelaySubscriptionEvaluationSwitch() const {
    return ambassador_.getDelaySubscriptionEvaluationSwitch();
  }

  bool getAdvisoriesUseKnownClassSwitch() const {
    return ambassador_.getAdvisoriesUseKnownClassSwitch();
  }

  bool getAllowRelaxedDDMSwitch() const {
    return ambassador_.getAllowRelaxedDDMSwitch();
  }

  bool getNonRegulatedGrantSwitch() const {
    return ambassador_.getNonRegulatedGrantSwitch();
  }

  unsigned long normalizeServiceGroup(rti::ServiceGroup serviceGroup) {
    return ambassador_.normalizeServiceGroup(serviceGroup);
  }

  std::wstring getOrderType(std::wstring const& orderTypeNameValue) {
    return orderTypeName(ambassador_.getOrderType(orderTypeNameValue));
  }

  std::wstring getOrderName(std::wstring const& orderTypeNameValue) {
    return ambassador_.getOrderName(orderTypeFromJava(orderTypeNameValue));
  }

  void changeAttributeOrderType(
      rti::VariableLengthData const& encodedObjectInstanceHandle,
      rti::AttributeHandleSet const& attributes,
      std::wstring const& orderTypeNameValue) {
    ambassador_.changeAttributeOrderType(
        ambassador_.decodeObjectInstanceHandle(encodedObjectInstanceHandle),
        attributes,
        orderTypeFromJava(orderTypeNameValue));
  }

  void changeDefaultAttributeOrderType(
      rti::VariableLengthData const& encodedObjectClassHandle,
      rti::AttributeHandleSet const& attributes,
      std::wstring const& orderTypeNameValue) {
    ambassador_.changeDefaultAttributeOrderType(
        ambassador_.decodeObjectClassHandle(encodedObjectClassHandle),
        attributes,
        orderTypeFromJava(orderTypeNameValue));
  }

  void changeInteractionOrderType(
      rti::VariableLengthData const& encodedInteractionClassHandle,
      std::wstring const& orderTypeNameValue) {
    ambassador_.changeInteractionOrderType(
        ambassador_.decodeInteractionClassHandle(encodedInteractionClassHandle),
        orderTypeFromJava(orderTypeNameValue));
  }

  double getUpdateRateValue(std::wstring const& updateRateDesignator) {
    return ambassador_.getUpdateRateValue(updateRateDesignator);
  }

  double getUpdateRateValueForAttribute(
      rti::VariableLengthData const& encodedObjectInstanceHandle,
      rti::VariableLengthData const& encodedAttributeHandle) {
    return ambassador_.getUpdateRateValueForAttribute(
        ambassador_.decodeObjectInstanceHandle(encodedObjectInstanceHandle),
        ambassador_.decodeAttributeHandle(encodedAttributeHandle));
  }

  void changeDefaultAttributeTransportationType(
      rti::VariableLengthData const& encodedObjectClassHandle,
      rti::AttributeHandleSet const& attributes,
      rti::VariableLengthData const& encodedTransportationTypeHandle) {
    ambassador_.changeDefaultAttributeTransportationType(
        ambassador_.decodeObjectClassHandle(encodedObjectClassHandle),
        attributes,
        rti::umbra_binding_detail::decodeTransportationTypeHandle(
            encodedTransportationTypeHandle));
  }

  std::wstring getTimeFactoryName() const {
    return ambassador_.getTimeFactory()->getName();
  }

  void validateLogicalTimeImplementation(
      std::wstring const& implementationName, bool interval) const {
    auto const selectedName = ambassador_.getTimeFactory()->getName();
    if (selectedName == implementationName) return;
    std::wstring message = L"The supplied logical-time implementation '" + implementationName +
        L"' does not match the federation's selected implementation '" + selectedName + L"'.";
    if (interval) throw rti::InvalidLookahead(message);
    throw rti::InvalidLogicalTime(message);
  }

  rti::VariableLengthData makeIntegerLogicalTime(std::int64_t value) {
    auto factory = ambassador_.getTimeFactory();
    auto* typedFactory = dynamic_cast<rti::HLAinteger64TimeFactory*>(factory.get());
    if (typedFactory == nullptr) {
      throw rti::RTIinternalError(L"The selected C++ logical-time factory is not integer64.");
    }
    return typedFactory->makeLogicalTime(static_cast<rti::Integer64>(value))->encode();
  }

  rti::VariableLengthData makeIntegerLogicalTimeInterval(std::int64_t value) {
    auto factory = ambassador_.getTimeFactory();
    auto* typedFactory = dynamic_cast<rti::HLAinteger64TimeFactory*>(factory.get());
    if (typedFactory == nullptr) {
      throw rti::RTIinternalError(L"The selected C++ logical-time factory is not integer64.");
    }
    return typedFactory->makeLogicalTimeInterval(static_cast<rti::Integer64>(value))->encode();
  }

  rti::VariableLengthData makeFloatLogicalTime(double value) {
    auto factory = ambassador_.getTimeFactory();
    auto* typedFactory = dynamic_cast<rti::HLAfloat64TimeFactory*>(factory.get());
    if (typedFactory == nullptr) {
      throw rti::RTIinternalError(L"The selected C++ logical-time factory is not float64.");
    }
    return typedFactory->makeLogicalTime(value)->encode();
  }

  rti::VariableLengthData makeFloatLogicalTimeInterval(double value) {
    auto factory = ambassador_.getTimeFactory();
    auto* typedFactory = dynamic_cast<rti::HLAfloat64TimeFactory*>(factory.get());
    if (typedFactory == nullptr) {
      throw rti::RTIinternalError(L"The selected C++ logical-time factory is not float64.");
    }
    return typedFactory->makeLogicalTimeInterval(value)->encode();
  }

  rti::VariableLengthData decodeLogicalTime(rti::VariableLengthData const& encoded) {
    return ambassador_.getTimeFactory()->decodeLogicalTime(encoded)->encode();
  }

  rti::VariableLengthData decodeLogicalTimeInterval(rti::VariableLengthData const& encoded) {
    return ambassador_.getTimeFactory()->decodeLogicalTimeInterval(encoded)->encode();
  }

  // Logical-time arithmetic is an RTI semantic operation.  Keep the Java
  // carrier objects as encoded views only and perform add/subtract/difference
  // with the selected C++ reference implementation.
  rti::VariableLengthData addLogicalTime(
      rti::VariableLengthData const& encodedTime,
      rti::VariableLengthData const& encodedInterval) {
    auto factory = ambassador_.getTimeFactory();
    auto time = factory->decodeLogicalTime(encodedTime);
    auto interval = factory->decodeLogicalTimeInterval(encodedInterval);
    *time += *interval;
    return time->encode();
  }

  rti::VariableLengthData subtractLogicalTime(
      rti::VariableLengthData const& encodedTime,
      rti::VariableLengthData const& encodedInterval) {
    auto factory = ambassador_.getTimeFactory();
    auto time = factory->decodeLogicalTime(encodedTime);
    auto interval = factory->decodeLogicalTimeInterval(encodedInterval);
    *time -= *interval;
    return time->encode();
  }

  rti::VariableLengthData addLogicalTimeInterval(
      rti::VariableLengthData const& encodedInterval,
      rti::VariableLengthData const& encodedAddend) {
    auto factory = ambassador_.getTimeFactory();
    auto interval = factory->decodeLogicalTimeInterval(encodedInterval);
    auto addend = factory->decodeLogicalTimeInterval(encodedAddend);
    *interval += *addend;
    return interval->encode();
  }

  rti::VariableLengthData subtractLogicalTimeInterval(
      rti::VariableLengthData const& encodedInterval,
      rti::VariableLengthData const& encodedSubtrahend) {
    auto factory = ambassador_.getTimeFactory();
    auto interval = factory->decodeLogicalTimeInterval(encodedInterval);
    auto subtrahend = factory->decodeLogicalTimeInterval(encodedSubtrahend);
    *interval -= *subtrahend;
    return interval->encode();
  }

  rti::VariableLengthData differenceLogicalTime(
      rti::VariableLengthData const& encodedMinuend,
      rti::VariableLengthData const& encodedSubtrahend) {
    auto factory = ambassador_.getTimeFactory();
    auto minuend = factory->decodeLogicalTime(encodedMinuend);
    auto subtrahend = factory->decodeLogicalTime(encodedSubtrahend);
    auto difference = factory->makeZero();
    difference->setToDifference(*minuend, *subtrahend);
    return difference->encode();
  }

  void enableTimeRegulation(
      rti::VariableLengthData const& encodedLogicalTimeInterval) {
    auto factory = ambassador_.getTimeFactory();
    auto interval = factory->decodeLogicalTimeInterval(encodedLogicalTimeInterval);
    ambassador_.enableTimeRegulation(*interval);
  }

  void disableTimeRegulation() { ambassador_.disableTimeRegulation(); }

  void enableTimeConstrained() { ambassador_.enableTimeConstrained(); }

  void disableTimeConstrained() { ambassador_.disableTimeConstrained(); }

  void enableAsynchronousDelivery() { ambassador_.enableAsynchronousDelivery(); }

  void disableAsynchronousDelivery() { ambassador_.disableAsynchronousDelivery(); }

  void modifyLookahead(rti::VariableLengthData const& encodedLogicalTimeInterval) {
    auto factory = ambassador_.getTimeFactory();
    auto interval = factory->decodeLogicalTimeInterval(encodedLogicalTimeInterval);
    ambassador_.modifyLookahead(*interval);
  }

  rti::VariableLengthData queryLookahead() {
    auto factory = ambassador_.getTimeFactory();
    auto interval = factory->makeZero();
    ambassador_.queryLookahead(*interval);
    return interval->encode();
  }

  void timeAdvanceRequest(rti::VariableLengthData const& encodedLogicalTime) {
    auto factory = ambassador_.getTimeFactory();
    auto time = factory->decodeLogicalTime(encodedLogicalTime);
    ambassador_.timeAdvanceRequest(*time);
  }

  void timeAdvanceRequestAvailable(rti::VariableLengthData const& encodedLogicalTime) {
    auto factory = ambassador_.getTimeFactory();
    auto time = factory->decodeLogicalTime(encodedLogicalTime);
    ambassador_.timeAdvanceRequestAvailable(*time);
  }

  void nextMessageRequest(rti::VariableLengthData const& encodedLogicalTime) {
    auto factory = ambassador_.getTimeFactory();
    auto time = factory->decodeLogicalTime(encodedLogicalTime);
    ambassador_.nextMessageRequest(*time);
  }

  void nextMessageRequestAvailable(rti::VariableLengthData const& encodedLogicalTime) {
    auto factory = ambassador_.getTimeFactory();
    auto time = factory->decodeLogicalTime(encodedLogicalTime);
    ambassador_.nextMessageRequestAvailable(*time);
  }

  void flushQueueRequest(rti::VariableLengthData const& encodedLogicalTime) {
    auto factory = ambassador_.getTimeFactory();
    auto time = factory->decodeLogicalTime(encodedLogicalTime);
    ambassador_.flushQueueRequest(*time);
  }

  rti::VariableLengthData queryLogicalTime() {
    auto factory = ambassador_.getTimeFactory();
    auto time = factory->makeInitial();
    ambassador_.queryLogicalTime(*time);
    return time->encode();
  }

  std::pair<bool, rti::VariableLengthData> queryGALT() {
    auto factory = ambassador_.getTimeFactory();
    auto time = factory->makeInitial();
    bool const valid = ambassador_.queryGALT(*time);
    return {valid, time->encode()};
  }

  std::pair<bool, rti::VariableLengthData> queryLITS() {
    auto factory = ambassador_.getTimeFactory();
    auto time = factory->makeInitial();
    bool const valid = ambassador_.queryLITS(*time);
    return {valid, time->encode()};
  }

  rti::VariableLengthData sendInteractionWithTime(
      rti::VariableLengthData const& encodedInteractionClassHandle,
      rti::ParameterHandleValueMap const& parameterValues,
      rti::VariableLengthData const& userSuppliedTag,
      rti::VariableLengthData const& encodedLogicalTime) {
    auto factory = ambassador_.getTimeFactory();
    auto time = factory->decodeLogicalTime(encodedLogicalTime);
    auto retraction = ambassador_.sendInteraction(
        ambassador_.decodeInteractionClassHandle(encodedInteractionClassHandle),
        parameterValues,
        userSuppliedTag,
        *time);
    return retraction.isValid() ? retraction.encode() : rti::VariableLengthData();
  }

  void retract(rti::VariableLengthData const& encodedMessageRetractionHandle) {
    ambassador_.retract(
        ambassador_.decodeMessageRetractionHandle(encodedMessageRetractionHandle));
  }

  rti::VariableLengthData decodeMessageRetractionHandle(
      rti::VariableLengthData const& encodedMessageRetractionHandle) {
    return ambassador_.decodeMessageRetractionHandle(encodedMessageRetractionHandle).encode();
  }

  rti::InteractionClassHandle getInteractionClassHandle(
      std::wstring const& interactionClassName) {
    return ambassador_.getInteractionClassHandle(interactionClassName);
  }

  std::wstring getInteractionClassName(
      rti::VariableLengthData const& encodedInteractionClassHandle) {
    return ambassador_.getInteractionClassName(
        ambassador_.decodeInteractionClassHandle(encodedInteractionClassHandle));
  }

  unsigned long normalizeInteractionClassHandle(
      rti::VariableLengthData const& encodedInteractionClassHandle) {
    return ambassador_.normalizeInteractionClassHandle(
        ambassador_.decodeInteractionClassHandle(encodedInteractionClassHandle));
  }

  rti::ParameterHandle getParameterHandle(
      rti::VariableLengthData const& encodedInteractionClassHandle,
      std::wstring const& parameterName) {
    return ambassador_.getParameterHandle(
        ambassador_.decodeInteractionClassHandle(encodedInteractionClassHandle), parameterName);
  }

  std::wstring getParameterName(
      rti::VariableLengthData const& encodedInteractionClassHandle,
      rti::VariableLengthData const& encodedParameterHandle) {
    return ambassador_.getParameterName(
        ambassador_.decodeInteractionClassHandle(encodedInteractionClassHandle),
        ambassador_.decodeParameterHandle(encodedParameterHandle));
  }

  rti::TransportationTypeHandle getTransportationTypeHandle(
      std::wstring const& transportationTypeName) {
    return ambassador_.getTransportationTypeHandle(transportationTypeName);
  }

  std::wstring getTransportationTypeName(
      rti::VariableLengthData const& encodedTransportationTypeHandle) {
    return ambassador_.getTransportationTypeName(
        rti::umbra_binding_detail::decodeTransportationTypeHandle(
            encodedTransportationTypeHandle));
  }

  void publishInteractionClass(
      rti::VariableLengthData const& encodedInteractionClassHandle) {
    ambassador_.publishInteractionClass(
        ambassador_.decodeInteractionClassHandle(encodedInteractionClassHandle));
  }

  void unpublishInteractionClass(
      rti::VariableLengthData const& encodedInteractionClassHandle) {
    ambassador_.unpublishInteractionClass(
        ambassador_.decodeInteractionClassHandle(encodedInteractionClassHandle));
  }

  void publishObjectClassDirectedInteractions(
      rti::VariableLengthData const& encodedObjectClassHandle,
      rti::InteractionClassHandleSet const& interactionClasses) {
    ambassador_.publishObjectClassDirectedInteractions(
        ambassador_.decodeObjectClassHandle(encodedObjectClassHandle), interactionClasses);
  }

  void unpublishObjectClassDirectedInteractions(
      rti::VariableLengthData const& encodedObjectClassHandle) {
    ambassador_.unpublishObjectClassDirectedInteractions(
        ambassador_.decodeObjectClassHandle(encodedObjectClassHandle));
  }

  void unpublishObjectClassDirectedInteractions(
      rti::VariableLengthData const& encodedObjectClassHandle,
      rti::InteractionClassHandleSet const& interactionClasses) {
    ambassador_.unpublishObjectClassDirectedInteractions(
        ambassador_.decodeObjectClassHandle(encodedObjectClassHandle), interactionClasses);
  }

  void subscribeInteractionClass(
      rti::VariableLengthData const& encodedInteractionClassHandle, bool active) {
    ambassador_.subscribeInteractionClass(
        ambassador_.decodeInteractionClassHandle(encodedInteractionClassHandle), active);
  }

  void unsubscribeInteractionClass(
      rti::VariableLengthData const& encodedInteractionClassHandle) {
    ambassador_.unsubscribeInteractionClass(
        ambassador_.decodeInteractionClassHandle(encodedInteractionClassHandle));
  }

  void subscribeObjectClassDirectedInteractions(
      rti::VariableLengthData const& encodedObjectClassHandle,
      rti::InteractionClassHandleSet const& interactionClasses,
      bool universally) {
    ambassador_.subscribeObjectClassDirectedInteractions(
        ambassador_.decodeObjectClassHandle(encodedObjectClassHandle),
        interactionClasses,
        universally);
  }

  void unsubscribeObjectClassDirectedInteractions(
      rti::VariableLengthData const& encodedObjectClassHandle) {
    ambassador_.unsubscribeObjectClassDirectedInteractions(
        ambassador_.decodeObjectClassHandle(encodedObjectClassHandle));
  }

  void unsubscribeObjectClassDirectedInteractions(
      rti::VariableLengthData const& encodedObjectClassHandle,
      rti::InteractionClassHandleSet const& interactionClasses) {
    ambassador_.unsubscribeObjectClassDirectedInteractions(
        ambassador_.decodeObjectClassHandle(encodedObjectClassHandle), interactionClasses);
  }

  void subscribeInteractionClassWithRegions(
      rti::VariableLengthData const& encodedInteractionClassHandle,
      rti::RegionHandleSet const& regions,
      bool active) {
    ambassador_.subscribeInteractionClassWithRegions(
        ambassador_.decodeInteractionClassHandle(encodedInteractionClassHandle),
        regions,
        active);
  }

  void unsubscribeInteractionClassWithRegions(
      rti::VariableLengthData const& encodedInteractionClassHandle,
      rti::RegionHandleSet const& regions) {
    ambassador_.unsubscribeInteractionClassWithRegions(
        ambassador_.decodeInteractionClassHandle(encodedInteractionClassHandle), regions);
  }

  void sendInteraction(
      rti::VariableLengthData const& encodedInteractionClassHandle,
      rti::ParameterHandleValueMap const& parameterValues,
      rti::VariableLengthData const& userSuppliedTag) {
    ambassador_.sendInteraction(
        ambassador_.decodeInteractionClassHandle(encodedInteractionClassHandle),
        parameterValues,
        userSuppliedTag);
  }

  void sendDirectedInteraction(
      rti::VariableLengthData const& encodedInteractionClassHandle,
      rti::VariableLengthData const& encodedObjectInstanceHandle,
      rti::ParameterHandleValueMap const& parameterValues,
      rti::VariableLengthData const& userSuppliedTag) {
    ambassador_.sendDirectedInteraction(
        ambassador_.decodeInteractionClassHandle(encodedInteractionClassHandle),
        ambassador_.decodeObjectInstanceHandle(encodedObjectInstanceHandle),
        parameterValues,
        userSuppliedTag);
  }

  rti::VariableLengthData sendDirectedInteractionWithTime(
      rti::VariableLengthData const& encodedInteractionClassHandle,
      rti::VariableLengthData const& encodedObjectInstanceHandle,
      rti::ParameterHandleValueMap const& parameterValues,
      rti::VariableLengthData const& userSuppliedTag,
      rti::VariableLengthData const& encodedLogicalTime) {
    auto factory = ambassador_.getTimeFactory();
    auto time = factory->decodeLogicalTime(encodedLogicalTime);
    auto retraction = ambassador_.sendDirectedInteraction(
        ambassador_.decodeInteractionClassHandle(encodedInteractionClassHandle),
        ambassador_.decodeObjectInstanceHandle(encodedObjectInstanceHandle),
        parameterValues,
        userSuppliedTag,
        *time);
    return retraction.isValid() ? retraction.encode() : rti::VariableLengthData();
  }

  void sendInteractionWithRegions(
      rti::VariableLengthData const& encodedInteractionClassHandle,
      rti::ParameterHandleValueMap const& parameterValues,
      rti::RegionHandleSet const& regions,
      rti::VariableLengthData const& userSuppliedTag) {
    ambassador_.sendInteractionWithRegions(
        ambassador_.decodeInteractionClassHandle(encodedInteractionClassHandle),
        parameterValues,
        regions,
        userSuppliedTag);
  }

  rti::VariableLengthData sendInteractionWithRegionsWithTime(
      rti::VariableLengthData const& encodedInteractionClassHandle,
      rti::ParameterHandleValueMap const& parameterValues,
      rti::RegionHandleSet const& regions,
      rti::VariableLengthData const& userSuppliedTag,
      rti::VariableLengthData const& encodedLogicalTime) {
    auto factory = ambassador_.getTimeFactory();
    auto time = factory->decodeLogicalTime(encodedLogicalTime);
    auto retraction = ambassador_.sendInteractionWithRegions(
        ambassador_.decodeInteractionClassHandle(encodedInteractionClassHandle),
        parameterValues,
        regions,
        userSuppliedTag,
        *time);
    return retraction.isValid() ? retraction.encode() : rti::VariableLengthData();
  }

  bool getConveyRegionDesignatorSetsSwitch() const {
    return ambassador_.getConveyRegionDesignatorSetsSwitch();
  }

  void setConveyRegionDesignatorSetsSwitch(bool enabled) {
    ambassador_.setConveyRegionDesignatorSetsSwitch(enabled);
  }

  void registerFederationSynchronizationPoint(
      std::wstring const& synchronizationPointLabel,
      rti::VariableLengthData const& userSuppliedTag,
      std::optional<rti::FederateHandleSet> synchronizationSet) {
    if (synchronizationSet) {
      ambassador_.registerFederationSynchronizationPoint(
          synchronizationPointLabel, userSuppliedTag, *synchronizationSet);
    } else {
      ambassador_.registerFederationSynchronizationPoint(
          synchronizationPointLabel, userSuppliedTag);
    }
  }

  void synchronizationPointAchieved(
      std::wstring const& synchronizationPointLabel, bool successfully) {
    ambassador_.synchronizationPointAchieved(synchronizationPointLabel, successfully);
  }

  void queryFederationSaveStatus() { ambassador_.queryFederationSaveStatus(); }

  void requestFederationSave(std::wstring const& label) {
    ambassador_.requestFederationSave(label);
  }

  void requestFederationSave(
      std::wstring const& label,
      rti::VariableLengthData const& encodedLogicalTime) {
    auto factory = ambassador_.getTimeFactory();
    auto time = factory->decodeLogicalTime(encodedLogicalTime);
    ambassador_.requestFederationSave(label, *time);
  }

  void federateSaveBegun() { ambassador_.federateSaveBegun(); }

  void federateSaveComplete() { ambassador_.federateSaveComplete(); }

  void federateSaveNotComplete() { ambassador_.federateSaveNotComplete(); }

  void abortFederationSave() { ambassador_.abortFederationSave(); }

  void queryFederationRestoreStatus() { ambassador_.queryFederationRestoreStatus(); }

  void requestFederationRestore(std::wstring const& label) {
    ambassador_.requestFederationRestore(label);
  }

  void federateRestoreComplete() { ambassador_.federateRestoreComplete(); }

  void federateRestoreNotComplete() { ambassador_.federateRestoreNotComplete(); }

  void abortFederationRestore() { ambassador_.abortFederationRestore(); }

  void close(JNIEnv* environment) noexcept {
    if (connected_) {
      try {
        ambassador_.disconnect();
      } catch (...) {
      }
      connected_ = false;
    }
    callbacks_.dispose(environment);
  }

 private:
  rti::ConfigurationResult connectNative(
      rti::CallbackModel callbackModelValue,
      std::optional<rti::RtiConfiguration> const& configuration,
      std::optional<rti::Credentials> const& credentials) {
    if (configuration && credentials) {
      return ambassador_.connect(callbacks_, callbackModelValue, *configuration, *credentials);
    }
    if (configuration) {
      return ambassador_.connect(callbacks_, callbackModelValue, *configuration);
    }
    if (credentials) {
      return ambassador_.connect(callbacks_, callbackModelValue, *credentials);
    }
    return ambassador_.connect(callbacks_, callbackModelValue);
  }

  JavaFederateAmbassador callbacks_;
  binding::UmbraRtiAmbassador ambassador_;
  bool connected_ = false;
};

NativeState* stateFor(JNIEnv* environment, jlong handle) {
  if (handle == 0) {
    throwJavaException(
        environment,
        "hla/rti1516_2025/exceptions/RTIinternalError",
        "The Umbra JNI RTI ambassador is closed.");
    return nullptr;
  }
  return reinterpret_cast<NativeState*>(handle);
}

jlong nativeCreate(JNIEnv* environment, jclass) {
  try {
    JavaVM* virtualMachine = nullptr;
    if (environment->GetJavaVM(&virtualMachine) != JNI_OK || virtualMachine == nullptr) {
      throw std::runtime_error("Could not resolve the JVM for the Umbra JNI bridge");
    }
    return reinterpret_cast<jlong>(new NativeState(environment, virtualMachine));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(
        environment,
        "hla/rti1516_2025/exceptions/RTIinternalError",
        exception.what());
  }
  return 0;
}

void nativeDestroy(JNIEnv* environment, jclass, jlong handle) {
  if (handle == 0) {
    return;
  }
  auto* state = reinterpret_cast<NativeState*>(handle);
  state->close(environment);
  delete state;
}

jobject nativeAuthorize(
    JNIEnv* environment,
    jclass,
    jstring password,
    jobject credentials,
    jstring federationName,
    jstring federateName,
    jstring federateType) {
  try {
    std::optional<std::wstring> configuredPassword;
    if (password != nullptr) {
      configuredPassword = wideString(environment, password);
    }
    auto const suppliedCredentials = credentialsFromJava(environment, credentials);
    rti::VariableLengthData emptyCredentialData;
    rti::Credentials nativeCredentials = suppliedCredentials
        ? *suppliedCredentials
        : rti::Credentials(rti::HLAnoCredentialsType, emptyCredentialData);
    umbra::detail::ReferenceAuthorizerConfiguration configuration;
    configuration.globalPlainTextPassword = std::move(configuredPassword);
    auto authorizer = umbra::detail::makeReferenceAuthorizer(std::move(configuration));
    rti::AuthorizationResult result(rti::AuthorizationResult::AUTHORIZATION_ERROR);
    if (federationName != nullptr && federateName != nullptr && federateType != nullptr) {
      result = authorizer->authorizeFederateOperation(
          nativeCredentials,
          wideString(environment, federationName),
          wideString(environment, federateName),
          wideString(environment, federateType));
    } else if (federationName != nullptr) {
      result = authorizer->authorizeFederationOperation(
          nativeCredentials, wideString(environment, federationName));
    } else {
      result = authorizer->authorizeRtiOperation(nativeCredentials);
    }
    return authorizationResultToJava(environment, result);
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(
        environment,
        "hla/rti1516_2025/exceptions/RTIinternalError",
        exception.what());
  }
  return nullptr;
}

jobject nativeConnect(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jobject callbackTarget,
    jstring callbackModelName,
    jobject configuration,
    jobject credentials) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) {
    return nullptr;
  }
  try {
    auto const result = state->connect(
        environment,
        callbackTarget,
        wideString(environment, callbackModelName),
        configuration,
        credentials);
    return configurationResultToJava(environment, result);
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(
        environment,
        "hla/rti1516_2025/exceptions/RTIinternalError",
        exception.what());
  }
  return nullptr;
}

void nativeDisconnect(JNIEnv* environment, jclass, jlong handle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) {
    return;
  }
  try {
    state->disconnect(environment);
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(
        environment,
        "hla/rti1516_2025/exceptions/RTIinternalError",
        exception.what());
  }
}

jboolean nativeFailEmbeddedTransportConnectionForTesting(
    JNIEnv* environment, jclass, jlong handle, jstring faultDescription) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return JNI_FALSE;
  try {
    return state->failEmbeddedTransportConnectionForTesting(
        wideString(environment, faultDescription)) ? JNI_TRUE : JNI_FALSE;
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(
        environment,
        "hla/rti1516_2025/exceptions/RTIinternalError",
        exception.what());
  }
  return JNI_FALSE;
}

jboolean nativeForceEmbeddedFederateResignationForTesting(
    JNIEnv* environment, jclass, jlong handle, jstring reason) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return JNI_FALSE;
  try {
    return state->forceEmbeddedFederateResignationForTesting(
        wideString(environment, reason)) ? JNI_TRUE : JNI_FALSE;
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(
        environment,
        "hla/rti1516_2025/exceptions/RTIinternalError",
        exception.what());
  }
  return JNI_FALSE;
}

jboolean nativeEvokeCallback(JNIEnv* environment, jclass, jlong handle, jdouble minimumTime) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) {
    return JNI_FALSE;
  }
  try {
    return state->evokeCallback(minimumTime) ? JNI_TRUE : JNI_FALSE;
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(
        environment,
        "hla/rti1516_2025/exceptions/RTIinternalError",
        exception.what());
  }
  return JNI_FALSE;
}

jboolean nativeEvokeMultipleCallbacks(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jdouble minimumTime,
    jdouble maximumTime) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) {
    return JNI_FALSE;
  }
  try {
    return state->evokeMultipleCallbacks(minimumTime, maximumTime) ? JNI_TRUE : JNI_FALSE;
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(
        environment,
        "hla/rti1516_2025/exceptions/RTIinternalError",
        exception.what());
  }
  return JNI_FALSE;
}

void nativeEnableCallbacks(JNIEnv* environment, jclass, jlong handle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->enableCallbacks();
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeDisableCallbacks(JNIEnv* environment, jclass, jlong handle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->disableCallbacks();
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeListFederationExecutions(JNIEnv* environment, jclass, jlong handle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->listFederationExecutions();
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeListFederationExecutionMembers(
    JNIEnv* environment, jclass, jlong handle, jstring federationName) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->listFederationExecutionMembers(wideString(environment, federationName));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeCreateFederationExecution(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jstring federationName,
    jobjectArray fomModules,
    jstring logicalTimeImplementationName) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->createFederationExecution(
        wideString(environment, federationName),
        wideStringVector(environment, fomModules),
        wideString(environment, logicalTimeImplementationName));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeCreateFederationExecutionWithMIM(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jstring federationName,
    jobjectArray fomModules,
    jstring mimModule,
    jstring logicalTimeImplementationName) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->createFederationExecutionWithMIM(
        wideString(environment, federationName),
        wideStringVector(environment, fomModules),
        wideString(environment, mimModule),
        wideString(environment, logicalTimeImplementationName));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeDestroyFederationExecution(
    JNIEnv* environment, jclass, jlong handle, jstring federationName) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->destroyFederationExecution(wideString(environment, federationName));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

rti::ResignAction resignAction(std::wstring const& value) {
  if (value == L"UNCONDITIONALLY_DIVEST_ATTRIBUTES") return rti::UNCONDITIONALLY_DIVEST_ATTRIBUTES;
  if (value == L"DELETE_OBJECTS") return rti::DELETE_OBJECTS;
  if (value == L"CANCEL_PENDING_OWNERSHIP_ACQUISITIONS") return rti::CANCEL_PENDING_OWNERSHIP_ACQUISITIONS;
  if (value == L"DELETE_OBJECTS_THEN_DIVEST") return rti::DELETE_OBJECTS_THEN_DIVEST;
  if (value == L"CANCEL_THEN_DELETE_THEN_DIVEST") return rti::CANCEL_THEN_DELETE_THEN_DIVEST;
  if (value == L"NO_ACTION") return rti::NO_ACTION;
  throw rti::InvalidResignAction(L"The Java JNI bridge received an unknown resignation action.");
}

std::wstring resignActionName(rti::ResignAction value) {
  switch (value) {
    case rti::UNCONDITIONALLY_DIVEST_ATTRIBUTES:
      return L"UNCONDITIONALLY_DIVEST_ATTRIBUTES";
    case rti::DELETE_OBJECTS: return L"DELETE_OBJECTS";
    case rti::CANCEL_PENDING_OWNERSHIP_ACQUISITIONS:
      return L"CANCEL_PENDING_OWNERSHIP_ACQUISITIONS";
    case rti::DELETE_OBJECTS_THEN_DIVEST: return L"DELETE_OBJECTS_THEN_DIVEST";
    case rti::CANCEL_THEN_DELETE_THEN_DIVEST:
      return L"CANCEL_THEN_DELETE_THEN_DIVEST";
    case rti::NO_ACTION: return L"NO_ACTION";
  }
  throw rti::InvalidResignAction(L"The C++ RTI returned an unknown resignation action.");
}

rti::ServiceGroup serviceGroup(std::wstring const& value) {
  if (value == L"FEDERATION_MANAGEMENT") return rti::FEDERATION_MANAGEMENT;
  if (value == L"DECLARATION_MANAGEMENT") return rti::DECLARATION_MANAGEMENT;
  if (value == L"OBJECT_MANAGEMENT") return rti::OBJECT_MANAGEMENT;
  if (value == L"OWNERSHIP_MANAGEMENT") return rti::OWNERSHIP_MANAGEMENT;
  if (value == L"TIME_MANAGEMENT") return rti::TIME_MANAGEMENT;
  if (value == L"DATA_DISTRIBUTION_MANAGEMENT") return rti::DATA_DISTRIBUTION_MANAGEMENT;
  if (value == L"SUPPORT_SERVICES") return rti::SUPPORT_SERVICES;
  throw rti::InvalidServiceGroup(L"The Java JNI bridge received an unknown service group.");
}

jbyteArray nativeJoinFederationExecution(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jstring federateName,
    jstring federateType,
    jstring federationName,
    jobjectArray additionalFomModules) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return nullptr;
  try {
    std::optional<std::wstring> requestedName;
    if (federateName != nullptr) requestedName = wideString(environment, federateName);
    auto joined = state->joinFederationExecution(
        std::move(requestedName),
        wideString(environment, federateType),
        wideString(environment, federationName),
        wideStringVector(environment, additionalFomModules));
    return javaByteArray(environment, joined.encode());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

void nativeResignFederationExecution(
    JNIEnv* environment, jclass, jlong handle, jstring resignActionName) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->resignFederationExecution(resignAction(wideString(environment, resignActionName)));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

jbyteArray nativeGetFederateHandle(
    JNIEnv* environment, jclass, jlong handle, jstring federateName) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return nullptr;
  try {
    return javaByteArray(environment, state->getFederateHandle(
        wideString(environment, federateName)).encode());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

jstring nativeGetFederateName(
    JNIEnv* environment, jclass, jlong handle, jbyteArray encodedFederateHandle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return nullptr;
  try {
    return javaString(environment, state->getFederateName(
        variableLengthData(environment, encodedFederateHandle)));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

jlong nativeNormalizeFederateHandle(
    JNIEnv* environment, jclass, jlong handle, jbyteArray encodedFederateHandle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return 0;
  try {
    return static_cast<jlong>(state->normalizeFederateHandle(
        variableLengthData(environment, encodedFederateHandle)));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return 0;
}

jbyteArray nativeDecodeObjectClassHandle(JNIEnv* environment, jclass, jbyteArray encodedValue) {
  try {
    auto decoded = rti::umbra_binding_detail::decodeObjectClassHandle(
        variableLengthData(environment, encodedValue));
    return javaByteArray(environment, decoded.encode());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

jbyteArray nativeGetObjectClassHandle(
    JNIEnv* environment, jclass, jlong handle, jstring objectClassName) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return nullptr;
  try {
    return javaByteArray(environment, state->getObjectClassHandle(
        wideString(environment, objectClassName)).encode());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

jstring nativeGetObjectClassName(
    JNIEnv* environment, jclass, jlong handle, jbyteArray encodedObjectClassHandle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return nullptr;
  try {
    return javaString(environment, state->getObjectClassName(
        variableLengthData(environment, encodedObjectClassHandle)));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

jbyteArray nativeGetKnownObjectClassHandle(
    JNIEnv* environment, jclass, jlong handle, jbyteArray encodedObjectInstanceHandle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return nullptr;
  try {
    return javaByteArray(environment, state->getKnownObjectClassHandle(
        variableLengthData(environment, encodedObjectInstanceHandle)).encode());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

jlong nativeNormalizeObjectClassHandle(
    JNIEnv* environment, jclass, jlong handle, jbyteArray encodedObjectClassHandle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return 0;
  try {
    return static_cast<jlong>(state->normalizeObjectClassHandle(
        variableLengthData(environment, encodedObjectClassHandle)));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return 0;
}

jbyteArray nativeDecodeAttributeHandle(JNIEnv* environment, jclass, jbyteArray encodedValue) {
  try {
    auto decoded = rti::umbra_binding_detail::decodeAttributeHandle(
        variableLengthData(environment, encodedValue));
    return javaByteArray(environment, decoded.encode());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

jbyteArray nativeGetAttributeHandle(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedObjectClassHandle,
    jstring attributeName) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return nullptr;
  try {
    return javaByteArray(environment, state->getAttributeHandle(
        variableLengthData(environment, encodedObjectClassHandle),
        wideString(environment, attributeName)).encode());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

jstring nativeGetAttributeName(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedObjectClassHandle,
    jbyteArray encodedAttributeHandle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return nullptr;
  try {
    return javaString(environment, state->getAttributeName(
        variableLengthData(environment, encodedObjectClassHandle),
        variableLengthData(environment, encodedAttributeHandle)));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

jbyteArray nativeDecodeDimensionHandle(JNIEnv* environment, jclass, jbyteArray encodedValue) {
  try {
    auto decoded = rti::umbra_binding_detail::decodeDimensionHandle(
        variableLengthData(environment, encodedValue));
    return javaByteArray(environment, decoded.encode());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

jbyteArray nativeGetDimensionHandle(
    JNIEnv* environment, jclass, jlong handle, jstring dimensionName) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return nullptr;
  try {
    return javaByteArray(environment, state->getDimensionHandle(
        wideString(environment, dimensionName)).encode());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

jstring nativeGetDimensionName(
    JNIEnv* environment, jclass, jlong handle, jbyteArray encodedDimensionHandle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return nullptr;
  try {
    return javaString(environment, state->getDimensionName(
        variableLengthData(environment, encodedDimensionHandle)));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

jobjectArray nativeGetAvailableDimensionsForObjectClass(
    JNIEnv* environment, jclass, jlong handle, jbyteArray encodedObjectClassHandle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return nullptr;
  try {
    return javaEncodedHandleSet(environment, state->getAvailableDimensionsForObjectClass(
        variableLengthData(environment, encodedObjectClassHandle)));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

jobjectArray nativeGetAvailableDimensionsForInteractionClass(
    JNIEnv* environment, jclass, jlong handle, jbyteArray encodedInteractionClassHandle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return nullptr;
  try {
    return javaEncodedHandleSet(environment, state->getAvailableDimensionsForInteractionClass(
        variableLengthData(environment, encodedInteractionClassHandle)));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

jlong nativeGetDimensionUpperBound(
    JNIEnv* environment, jclass, jlong handle, jbyteArray encodedDimensionHandle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return 0;
  try {
    return static_cast<jlong>(state->getDimensionUpperBound(
        variableLengthData(environment, encodedDimensionHandle)));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return 0;
}

jbyteArray nativeDecodeRegionHandle(JNIEnv* environment, jclass, jbyteArray encodedValue) {
  try {
    auto decoded = rti::umbra_binding_detail::decodeRegionHandle(
        variableLengthData(environment, encodedValue));
    return javaByteArray(environment, decoded.encode());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

jbyteArray nativeCreateRegion(
    JNIEnv* environment, jclass, jlong handle, jobject dimensions) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return nullptr;
  try {
    return javaByteArray(environment, state->createRegion(
        dimensionHandleSet(environment, dimensions)).encode());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

void nativeCommitRegionModifications(
    JNIEnv* environment, jclass, jlong handle, jobject regions) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->commitRegionModifications(regionHandleSet(environment, regions));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeDeleteRegion(
    JNIEnv* environment, jclass, jlong handle, jbyteArray encodedRegionHandle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->deleteRegion(variableLengthData(environment, encodedRegionHandle));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

jobjectArray nativeGetDimensionHandleSet(
    JNIEnv* environment, jclass, jlong handle, jbyteArray encodedRegionHandle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return nullptr;
  try {
    return javaEncodedHandleSet(environment, state->getDimensionHandleSet(
        variableLengthData(environment, encodedRegionHandle)));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

jlongArray nativeGetRangeBounds(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedRegionHandle,
    jbyteArray encodedDimensionHandle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return nullptr;
  try {
    return javaRangeBounds(environment, state->getRangeBounds(
        variableLengthData(environment, encodedRegionHandle),
        variableLengthData(environment, encodedDimensionHandle)));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

void nativeSetRangeBounds(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedRegionHandle,
    jbyteArray encodedDimensionHandle,
    jlong lowerBound,
    jlong upperBound) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->setRangeBounds(
        variableLengthData(environment, encodedRegionHandle),
        variableLengthData(environment, encodedDimensionHandle),
        lowerBound,
        upperBound);
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

jbyteArray nativeDecodeObjectInstanceHandle(JNIEnv* environment, jclass, jbyteArray encodedValue) {
  try {
    auto decoded = rti::umbra_binding_detail::decodeObjectInstanceHandle(
        variableLengthData(environment, encodedValue));
    return javaByteArray(environment, decoded.encode());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

void nativePublishObjectClassAttributes(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedObjectClassHandle,
    jobject attributes) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->publishObjectClassAttributes(
        variableLengthData(environment, encodedObjectClassHandle),
        attributeHandleSet(environment, attributes));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeUnpublishObjectClass(
    JNIEnv* environment, jclass, jlong handle, jbyteArray encodedObjectClassHandle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->unpublishObjectClass(variableLengthData(environment, encodedObjectClassHandle));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeUnpublishObjectClassAttributes(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedObjectClassHandle,
    jobject attributes) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->unpublishObjectClassAttributes(
        variableLengthData(environment, encodedObjectClassHandle),
        attributeHandleSet(environment, attributes));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativePublishObjectClassDirectedInteractions(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedObjectClassHandle,
    jobject interactionClasses) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->publishObjectClassDirectedInteractions(
        variableLengthData(environment, encodedObjectClassHandle),
        interactionClassHandleSet(environment, interactionClasses));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeUnpublishObjectClassDirectedInteractions(
    JNIEnv* environment, jclass, jlong handle, jbyteArray encodedObjectClassHandle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->unpublishObjectClassDirectedInteractions(
        variableLengthData(environment, encodedObjectClassHandle));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeUnpublishObjectClassDirectedInteractionsWithClasses(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedObjectClassHandle,
    jobject interactionClasses) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->unpublishObjectClassDirectedInteractions(
        variableLengthData(environment, encodedObjectClassHandle),
        interactionClassHandleSet(environment, interactionClasses));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeSubscribeObjectClassAttributes(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedObjectClassHandle,
    jobject attributes,
    jboolean active,
    jstring updateRateDesignator) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->subscribeObjectClassAttributes(
        variableLengthData(environment, encodedObjectClassHandle),
        attributeHandleSet(environment, attributes),
        active == JNI_TRUE,
        wideString(environment, updateRateDesignator));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeUnsubscribeObjectClass(
    JNIEnv* environment, jclass, jlong handle, jbyteArray encodedObjectClassHandle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->unsubscribeObjectClass(variableLengthData(environment, encodedObjectClassHandle));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeUnsubscribeObjectClassAttributes(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedObjectClassHandle,
    jobject attributes) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->unsubscribeObjectClassAttributes(
        variableLengthData(environment, encodedObjectClassHandle),
        attributeHandleSet(environment, attributes));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeSubscribeObjectClassDirectedInteractions(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedObjectClassHandle,
    jobject interactionClasses,
    jboolean universally) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->subscribeObjectClassDirectedInteractions(
        variableLengthData(environment, encodedObjectClassHandle),
        interactionClassHandleSet(environment, interactionClasses),
        universally == JNI_TRUE);
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeUnsubscribeObjectClassDirectedInteractions(
    JNIEnv* environment, jclass, jlong handle, jbyteArray encodedObjectClassHandle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->unsubscribeObjectClassDirectedInteractions(
        variableLengthData(environment, encodedObjectClassHandle));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeUnsubscribeObjectClassDirectedInteractionsWithClasses(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedObjectClassHandle,
    jobject interactionClasses) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->unsubscribeObjectClassDirectedInteractions(
        variableLengthData(environment, encodedObjectClassHandle),
        interactionClassHandleSet(environment, interactionClasses));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeSubscribeObjectClassAttributesWithRegions(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedObjectClassHandle,
    jobject attributesAndRegions,
    jboolean active,
    jstring updateRateDesignator) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->subscribeObjectClassAttributesWithRegions(
        variableLengthData(environment, encodedObjectClassHandle),
        attributeRegionPairs(environment, attributesAndRegions),
        active == JNI_TRUE,
        wideString(environment, updateRateDesignator));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeUnsubscribeObjectClassAttributesWithRegions(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedObjectClassHandle,
    jobject attributesAndRegions) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->unsubscribeObjectClassAttributesWithRegions(
        variableLengthData(environment, encodedObjectClassHandle),
        attributeRegionPairs(environment, attributesAndRegions));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeReserveObjectInstanceName(
    JNIEnv* environment, jclass, jlong handle, jstring objectInstanceName) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->reserveObjectInstanceName(wideString(environment, objectInstanceName));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeReleaseObjectInstanceName(
    JNIEnv* environment, jclass, jlong handle, jstring objectInstanceName) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->releaseObjectInstanceName(wideString(environment, objectInstanceName));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeReserveMultipleObjectInstanceNames(
    JNIEnv* environment, jclass, jlong handle, jobject objectInstanceNames) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->reserveMultipleObjectInstanceNames(
        objectInstanceNameSet(environment, objectInstanceNames));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeReleaseMultipleObjectInstanceNames(
    JNIEnv* environment, jclass, jlong handle, jobject objectInstanceNames) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->releaseMultipleObjectInstanceNames(
        objectInstanceNameSet(environment, objectInstanceNames));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

jbyteArray nativeRegisterObjectInstance(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedObjectClassHandle,
    jstring objectInstanceName) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return nullptr;
  try {
    std::optional<std::wstring> requestedName;
    if (objectInstanceName != nullptr) requestedName = wideString(environment, objectInstanceName);
    return javaByteArray(environment, state->registerObjectInstance(
        variableLengthData(environment, encodedObjectClassHandle), std::move(requestedName)).encode());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

jbyteArray nativeRegisterObjectInstanceWithRegions(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedObjectClassHandle,
    jobject attributesAndRegions,
    jstring objectInstanceName) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return nullptr;
  try {
    std::optional<std::wstring> requestedName;
    if (objectInstanceName != nullptr) requestedName = wideString(environment, objectInstanceName);
    return javaByteArray(environment, state->registerObjectInstanceWithRegions(
        variableLengthData(environment, encodedObjectClassHandle),
        attributeRegionPairs(environment, attributesAndRegions),
        std::move(requestedName)).encode());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

void nativeAssociateRegionsForUpdates(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedObjectInstanceHandle,
    jobject attributesAndRegions) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->associateRegionsForUpdates(
        variableLengthData(environment, encodedObjectInstanceHandle),
        attributeRegionPairs(environment, attributesAndRegions));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeUnassociateRegionsForUpdates(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedObjectInstanceHandle,
    jobject attributesAndRegions) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->unassociateRegionsForUpdates(
        variableLengthData(environment, encodedObjectInstanceHandle),
        attributeRegionPairs(environment, attributesAndRegions));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

jbyteArray nativeGetObjectInstanceHandle(
    JNIEnv* environment, jclass, jlong handle, jstring objectInstanceName) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return nullptr;
  try {
    return javaByteArray(environment, state->getObjectInstanceHandle(
        wideString(environment, objectInstanceName)).encode());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

jstring nativeGetObjectInstanceName(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedObjectInstanceHandle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return nullptr;
  try {
    return javaString(environment, state->getObjectInstanceName(
        variableLengthData(environment, encodedObjectInstanceHandle)));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

jlong nativeNormalizeObjectInstanceHandle(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedObjectInstanceHandle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return 0;
  try {
    return static_cast<jlong>(state->normalizeObjectInstanceHandle(
        variableLengthData(environment, encodedObjectInstanceHandle)));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return 0;
}

void nativeUpdateAttributeValues(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedObjectInstanceHandle,
    jobject attributeValues,
    jbyteArray userSuppliedTag) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->updateAttributeValues(
        variableLengthData(environment, encodedObjectInstanceHandle),
        attributeHandleValueMap(environment, attributeValues),
        variableLengthData(environment, userSuppliedTag));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

jbyteArray nativeUpdateAttributeValuesWithTime(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedObjectInstanceHandle,
    jobject attributeValues,
    jbyteArray userSuppliedTag,
    jbyteArray encodedLogicalTime) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return nullptr;
  try {
    return javaByteArray(
        environment,
        state->updateAttributeValuesWithTime(
            variableLengthData(environment, encodedObjectInstanceHandle),
            attributeHandleValueMap(environment, attributeValues),
            variableLengthData(environment, userSuppliedTag),
            variableLengthData(environment, encodedLogicalTime)));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

void nativeDeleteObjectInstance(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedObjectInstanceHandle,
    jbyteArray userSuppliedTag) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->deleteObjectInstance(
        variableLengthData(environment, encodedObjectInstanceHandle),
        variableLengthData(environment, userSuppliedTag));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

jbyteArray nativeDeleteObjectInstanceWithTime(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedObjectInstanceHandle,
    jbyteArray userSuppliedTag,
    jbyteArray encodedLogicalTime) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return nullptr;
  try {
    return javaByteArray(
        environment,
        state->deleteObjectInstanceWithTime(
            variableLengthData(environment, encodedObjectInstanceHandle),
            variableLengthData(environment, userSuppliedTag),
            variableLengthData(environment, encodedLogicalTime)));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

void nativeLocalDeleteObjectInstance(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedObjectInstanceHandle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->localDeleteObjectInstance(
        variableLengthData(environment, encodedObjectInstanceHandle));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeRequestAttributeValueUpdateForObjectInstance(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedObjectInstanceHandle,
    jobject attributes,
    jbyteArray userSuppliedTag) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->requestAttributeValueUpdateForObjectInstance(
        variableLengthData(environment, encodedObjectInstanceHandle),
        attributeHandleSet(environment, attributes),
        variableLengthData(environment, userSuppliedTag));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeRequestAttributeValueUpdateForObjectClass(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedObjectClassHandle,
    jobject attributes,
    jbyteArray userSuppliedTag) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->requestAttributeValueUpdateForObjectClass(
        variableLengthData(environment, encodedObjectClassHandle),
        attributeHandleSet(environment, attributes),
        variableLengthData(environment, userSuppliedTag));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeRequestAttributeValueUpdateWithRegions(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedObjectClassHandle,
    jobject attributesAndRegions,
    jbyteArray userSuppliedTag) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->requestAttributeValueUpdateWithRegions(
        variableLengthData(environment, encodedObjectClassHandle),
        attributeRegionPairs(environment, attributesAndRegions),
        variableLengthData(environment, userSuppliedTag));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeQueryAttributeOwnership(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedObjectInstanceHandle,
    jobject attributes) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->queryAttributeOwnership(
        variableLengthData(environment, encodedObjectInstanceHandle),
        attributeHandleSet(environment, attributes));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

jboolean nativeIsAttributeOwnedByFederate(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedObjectInstanceHandle,
    jbyteArray encodedAttributeHandle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return JNI_FALSE;
  try {
    return state->isAttributeOwnedByFederate(
        variableLengthData(environment, encodedObjectInstanceHandle),
        variableLengthData(environment, encodedAttributeHandle))
        ? JNI_TRUE
        : JNI_FALSE;
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return JNI_FALSE;
}

void nativeUnconditionalAttributeOwnershipDivestiture(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedObjectInstanceHandle,
    jobject attributes,
    jbyteArray userSuppliedTag) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->unconditionalAttributeOwnershipDivestiture(
        variableLengthData(environment, encodedObjectInstanceHandle),
        attributeHandleSet(environment, attributes),
        variableLengthData(environment, userSuppliedTag));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeNegotiatedAttributeOwnershipDivestiture(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedObjectInstanceHandle,
    jobject attributes,
    jbyteArray userSuppliedTag) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->negotiatedAttributeOwnershipDivestiture(
        variableLengthData(environment, encodedObjectInstanceHandle),
        attributeHandleSet(environment, attributes),
        variableLengthData(environment, userSuppliedTag));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeConfirmDivestiture(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedObjectInstanceHandle,
    jobject attributes,
    jbyteArray userSuppliedTag) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->confirmDivestiture(
        variableLengthData(environment, encodedObjectInstanceHandle),
        attributeHandleSet(environment, attributes),
        variableLengthData(environment, userSuppliedTag));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeCancelNegotiatedAttributeOwnershipDivestiture(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedObjectInstanceHandle,
    jobject attributes) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->cancelNegotiatedAttributeOwnershipDivestiture(
        variableLengthData(environment, encodedObjectInstanceHandle),
        attributeHandleSet(environment, attributes));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeAttributeOwnershipAcquisition(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedObjectInstanceHandle,
    jobject attributes,
    jbyteArray userSuppliedTag) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->attributeOwnershipAcquisition(
        variableLengthData(environment, encodedObjectInstanceHandle),
        attributeHandleSet(environment, attributes),
        variableLengthData(environment, userSuppliedTag));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeAttributeOwnershipAcquisitionIfAvailable(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedObjectInstanceHandle,
    jobject attributes,
    jbyteArray userSuppliedTag) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->attributeOwnershipAcquisitionIfAvailable(
        variableLengthData(environment, encodedObjectInstanceHandle),
        attributeHandleSet(environment, attributes),
        variableLengthData(environment, userSuppliedTag));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeCancelAttributeOwnershipAcquisition(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedObjectInstanceHandle,
    jobject attributes) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->cancelAttributeOwnershipAcquisition(
        variableLengthData(environment, encodedObjectInstanceHandle),
        attributeHandleSet(environment, attributes));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeAttributeOwnershipReleaseDenied(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedObjectInstanceHandle,
    jobject attributes,
    jbyteArray userSuppliedTag) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->attributeOwnershipReleaseDenied(
        variableLengthData(environment, encodedObjectInstanceHandle),
        attributeHandleSet(environment, attributes),
        variableLengthData(environment, userSuppliedTag));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

jobjectArray nativeAttributeOwnershipDivestitureIfWanted(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedObjectInstanceHandle,
    jobject attributes,
    jbyteArray userSuppliedTag) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return nullptr;
  try {
    return javaEncodedHandleSet(
        environment,
        state->attributeOwnershipDivestitureIfWanted(
            variableLengthData(environment, encodedObjectInstanceHandle),
            attributeHandleSet(environment, attributes),
            variableLengthData(environment, userSuppliedTag)));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

void nativeQueryAttributeTransportationType(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedObjectInstanceHandle,
    jbyteArray encodedAttributeHandle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->queryAttributeTransportationType(
        variableLengthData(environment, encodedObjectInstanceHandle),
        variableLengthData(environment, encodedAttributeHandle));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeQueryInteractionTransportationType(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedFederateHandle,
    jbyteArray encodedInteractionClassHandle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->queryInteractionTransportationType(
        variableLengthData(environment, encodedFederateHandle),
        variableLengthData(environment, encodedInteractionClassHandle));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeRequestAttributeTransportationTypeChange(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedObjectInstanceHandle,
    jobject attributes,
    jbyteArray encodedTransportationTypeHandle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->requestAttributeTransportationTypeChange(
        variableLengthData(environment, encodedObjectInstanceHandle),
        attributeHandleSet(environment, attributes),
        variableLengthData(environment, encodedTransportationTypeHandle));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeRequestInteractionTransportationTypeChange(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedInteractionClassHandle,
    jbyteArray encodedTransportationTypeHandle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->requestInteractionTransportationTypeChange(
        variableLengthData(environment, encodedInteractionClassHandle),
        variableLengthData(environment, encodedTransportationTypeHandle));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeSetAttributeScopeAdvisorySwitch(
    JNIEnv* environment, jclass, jlong handle, jboolean enabled) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->setAttributeScopeAdvisorySwitch(enabled == JNI_TRUE);
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

jboolean nativeGetObjectClassRelevanceAdvisorySwitch(
    JNIEnv* environment, jclass, jlong handle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return JNI_FALSE;
  try {
    return state->getObjectClassRelevanceAdvisorySwitch() ? JNI_TRUE : JNI_FALSE;
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return JNI_FALSE;
}

void nativeSetObjectClassRelevanceAdvisorySwitch(
    JNIEnv* environment, jclass, jlong handle, jboolean enabled) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->setObjectClassRelevanceAdvisorySwitch(enabled == JNI_TRUE);
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

jboolean nativeGetAttributeRelevanceAdvisorySwitch(
    JNIEnv* environment, jclass, jlong handle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return JNI_FALSE;
  try {
    return state->getAttributeRelevanceAdvisorySwitch() ? JNI_TRUE : JNI_FALSE;
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return JNI_FALSE;
}

void nativeSetAttributeRelevanceAdvisorySwitch(
    JNIEnv* environment, jclass, jlong handle, jboolean enabled) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->setAttributeRelevanceAdvisorySwitch(enabled == JNI_TRUE);
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

jboolean nativeGetAttributeScopeAdvisorySwitch(
    JNIEnv* environment, jclass, jlong handle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return JNI_FALSE;
  try {
    return state->getAttributeScopeAdvisorySwitch() ? JNI_TRUE : JNI_FALSE;
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return JNI_FALSE;
}

jboolean nativeGetInteractionRelevanceAdvisorySwitch(
    JNIEnv* environment, jclass, jlong handle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return JNI_FALSE;
  try {
    return state->getInteractionRelevanceAdvisorySwitch() ? JNI_TRUE : JNI_FALSE;
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return JNI_FALSE;
}

void nativeSetInteractionRelevanceAdvisorySwitch(
    JNIEnv* environment, jclass, jlong handle, jboolean enabled) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->setInteractionRelevanceAdvisorySwitch(enabled == JNI_TRUE);
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

jstring nativeGetAutomaticResignDirective(
    JNIEnv* environment, jclass, jlong handle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return nullptr;
  try {
    return javaString(environment, resignActionName(state->getAutomaticResignDirective()));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

void nativeSetAutomaticResignDirective(
    JNIEnv* environment, jclass, jlong handle, jstring resignActionNameValue) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->setAutomaticResignDirective(
        resignAction(wideString(environment, resignActionNameValue)));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

jboolean nativeGetServiceReportingSwitch(JNIEnv* environment, jclass, jlong handle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return JNI_FALSE;
  try {
    return state->getServiceReportingSwitch() ? JNI_TRUE : JNI_FALSE;
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return JNI_FALSE;
}

void nativeSetServiceReportingSwitch(
    JNIEnv* environment, jclass, jlong handle, jboolean enabled) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->setServiceReportingSwitch(enabled == JNI_TRUE);
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

jboolean nativeGetExceptionReportingSwitch(JNIEnv* environment, jclass, jlong handle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return JNI_FALSE;
  try {
    return state->getExceptionReportingSwitch() ? JNI_TRUE : JNI_FALSE;
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return JNI_FALSE;
}

void nativeSetExceptionReportingSwitch(
    JNIEnv* environment, jclass, jlong handle, jboolean enabled) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->setExceptionReportingSwitch(enabled == JNI_TRUE);
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

jboolean nativeGetSendServiceReportsToFileSwitch(
    JNIEnv* environment, jclass, jlong handle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return JNI_FALSE;
  try {
    return state->getSendServiceReportsToFileSwitch() ? JNI_TRUE : JNI_FALSE;
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return JNI_FALSE;
}

void nativeSetSendServiceReportsToFileSwitch(
    JNIEnv* environment, jclass, jlong handle, jboolean enabled) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->setSendServiceReportsToFileSwitch(enabled == JNI_TRUE);
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

jstring nativeGetHLAversion(JNIEnv* environment, jclass, jlong handle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return nullptr;
  try {
    return javaString(environment, state->getHLAversion());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

jboolean nativeGetAutoProvideSwitch(JNIEnv* environment, jclass, jlong handle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return JNI_FALSE;
  try {
    return state->getAutoProvideSwitch() ? JNI_TRUE : JNI_FALSE;
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return JNI_FALSE;
}

jboolean nativeGetDelaySubscriptionEvaluationSwitch(
    JNIEnv* environment, jclass, jlong handle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return JNI_FALSE;
  try {
    return state->getDelaySubscriptionEvaluationSwitch() ? JNI_TRUE : JNI_FALSE;
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return JNI_FALSE;
}

jboolean nativeGetAdvisoriesUseKnownClassSwitch(
    JNIEnv* environment, jclass, jlong handle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return JNI_FALSE;
  try {
    return state->getAdvisoriesUseKnownClassSwitch() ? JNI_TRUE : JNI_FALSE;
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return JNI_FALSE;
}

jboolean nativeGetAllowRelaxedDDMSwitch(JNIEnv* environment, jclass, jlong handle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return JNI_FALSE;
  try {
    return state->getAllowRelaxedDDMSwitch() ? JNI_TRUE : JNI_FALSE;
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return JNI_FALSE;
}

jboolean nativeGetNonRegulatedGrantSwitch(JNIEnv* environment, jclass, jlong handle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return JNI_FALSE;
  try {
    return state->getNonRegulatedGrantSwitch() ? JNI_TRUE : JNI_FALSE;
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return JNI_FALSE;
}

jlong nativeNormalizeServiceGroup(
    JNIEnv* environment, jclass, jlong handle, jstring serviceGroupName) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return 0;
  try {
    return static_cast<jlong>(state->normalizeServiceGroup(
        serviceGroup(wideString(environment, serviceGroupName))));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return 0;
}

jboolean nativeGetConveyRegionDesignatorSetsSwitch(
    JNIEnv* environment, jclass, jlong handle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return JNI_FALSE;
  try {
    return state->getConveyRegionDesignatorSetsSwitch() ? JNI_TRUE : JNI_FALSE;
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return JNI_FALSE;
}

void nativeSetConveyRegionDesignatorSetsSwitch(
    JNIEnv* environment, jclass, jlong handle, jboolean enabled) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->setConveyRegionDesignatorSetsSwitch(enabled == JNI_TRUE);
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

jstring nativeGetOrderType(
    JNIEnv* environment, jclass, jlong handle, jstring orderTypeNameValue) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return nullptr;
  try {
    return javaString(environment, state->getOrderType(
        wideString(environment, orderTypeNameValue)));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

jstring nativeGetOrderName(
    JNIEnv* environment, jclass, jlong handle, jstring orderTypeNameValue) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return nullptr;
  try {
    return javaString(environment, state->getOrderName(
        wideString(environment, orderTypeNameValue)));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

void nativeChangeAttributeOrderType(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedObjectInstanceHandle,
    jobject attributes,
    jstring orderTypeNameValue) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->changeAttributeOrderType(
        variableLengthData(environment, encodedObjectInstanceHandle),
        attributeHandleSet(environment, attributes),
        wideString(environment, orderTypeNameValue));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeChangeDefaultAttributeOrderType(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedObjectClassHandle,
    jobject attributes,
    jstring orderTypeNameValue) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->changeDefaultAttributeOrderType(
        variableLengthData(environment, encodedObjectClassHandle),
        attributeHandleSet(environment, attributes),
        wideString(environment, orderTypeNameValue));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeChangeInteractionOrderType(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedInteractionClassHandle,
    jstring orderTypeNameValue) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->changeInteractionOrderType(
        variableLengthData(environment, encodedInteractionClassHandle),
        wideString(environment, orderTypeNameValue));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

jdouble nativeGetUpdateRateValue(
    JNIEnv* environment, jclass, jlong handle, jstring updateRateDesignator) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return 0.0;
  try {
    return static_cast<jdouble>(state->getUpdateRateValue(
        wideString(environment, updateRateDesignator)));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return 0.0;
}

jdouble nativeGetUpdateRateValueForAttribute(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedObjectInstanceHandle,
    jbyteArray encodedAttributeHandle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return 0.0;
  try {
    return static_cast<jdouble>(state->getUpdateRateValueForAttribute(
        variableLengthData(environment, encodedObjectInstanceHandle),
        variableLengthData(environment, encodedAttributeHandle)));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return 0.0;
}

void nativeChangeDefaultAttributeTransportationType(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedObjectClassHandle,
    jobject attributes,
    jbyteArray encodedTransportationTypeHandle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->changeDefaultAttributeTransportationType(
        variableLengthData(environment, encodedObjectClassHandle),
        attributeHandleSet(environment, attributes),
        variableLengthData(environment, encodedTransportationTypeHandle));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

jstring nativeGetTimeFactoryName(JNIEnv* environment, jclass, jlong handle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return nullptr;
  try {
    return javaString(environment, state->getTimeFactoryName());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

void nativeValidateLogicalTimeImplementation(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jstring implementationName,
    jboolean interval) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->validateLogicalTimeImplementation(
        wideString(environment, implementationName), interval == JNI_TRUE);
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

jbyteArray nativeMakeIntegerLogicalTime(
    JNIEnv* environment, jclass, jlong handle, jlong value) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return nullptr;
  try {
    return javaByteArray(
        environment, state->makeIntegerLogicalTime(static_cast<std::int64_t>(value)));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

jbyteArray nativeMakeIntegerLogicalTimeInterval(
    JNIEnv* environment, jclass, jlong handle, jlong value) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return nullptr;
  try {
    return javaByteArray(
        environment, state->makeIntegerLogicalTimeInterval(static_cast<std::int64_t>(value)));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

jbyteArray nativeMakeFloatLogicalTime(
    JNIEnv* environment, jclass, jlong handle, jdouble value) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return nullptr;
  try {
    return javaByteArray(environment, state->makeFloatLogicalTime(static_cast<double>(value)));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

jbyteArray nativeMakeFloatLogicalTimeInterval(
    JNIEnv* environment, jclass, jlong handle, jdouble value) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return nullptr;
  try {
    return javaByteArray(
        environment, state->makeFloatLogicalTimeInterval(static_cast<double>(value)));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

jbyteArray nativeDecodeLogicalTime(
    JNIEnv* environment, jclass, jlong handle, jbyteArray encodedValue) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return nullptr;
  try {
    return javaByteArray(
        environment, state->decodeLogicalTime(variableLengthData(environment, encodedValue)));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

jbyteArray nativeDecodeLogicalTimeInterval(
    JNIEnv* environment, jclass, jlong handle, jbyteArray encodedValue) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return nullptr;
  try {
    return javaByteArray(
        environment,
        state->decodeLogicalTimeInterval(variableLengthData(environment, encodedValue)));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

jbyteArray nativeAddLogicalTime(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedTime,
    jbyteArray encodedInterval) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return nullptr;
  try {
    return javaByteArray(
        environment,
        state->addLogicalTime(
            variableLengthData(environment, encodedTime),
            variableLengthData(environment, encodedInterval)));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

jbyteArray nativeSubtractLogicalTime(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedTime,
    jbyteArray encodedInterval) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return nullptr;
  try {
    return javaByteArray(
        environment,
        state->subtractLogicalTime(
            variableLengthData(environment, encodedTime),
            variableLengthData(environment, encodedInterval)));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

jbyteArray nativeAddLogicalTimeInterval(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedInterval,
    jbyteArray encodedAddend) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return nullptr;
  try {
    return javaByteArray(
        environment,
        state->addLogicalTimeInterval(
            variableLengthData(environment, encodedInterval),
            variableLengthData(environment, encodedAddend)));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

jbyteArray nativeSubtractLogicalTimeInterval(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedInterval,
    jbyteArray encodedSubtrahend) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return nullptr;
  try {
    return javaByteArray(
        environment,
        state->subtractLogicalTimeInterval(
            variableLengthData(environment, encodedInterval),
            variableLengthData(environment, encodedSubtrahend)));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

jbyteArray nativeDifferenceLogicalTime(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedMinuend,
    jbyteArray encodedSubtrahend) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return nullptr;
  try {
    return javaByteArray(
        environment,
        state->differenceLogicalTime(
            variableLengthData(environment, encodedMinuend),
            variableLengthData(environment, encodedSubtrahend)));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

void nativeEnableTimeRegulation(
    JNIEnv* environment, jclass, jlong handle, jbyteArray encodedLogicalTimeInterval) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->enableTimeRegulation(
        variableLengthData(environment, encodedLogicalTimeInterval));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeDisableTimeRegulation(JNIEnv* environment, jclass, jlong handle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->disableTimeRegulation();
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeEnableTimeConstrained(JNIEnv* environment, jclass, jlong handle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->enableTimeConstrained();
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeDisableTimeConstrained(JNIEnv* environment, jclass, jlong handle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->disableTimeConstrained();
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeEnableAsynchronousDelivery(JNIEnv* environment, jclass, jlong handle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->enableAsynchronousDelivery();
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeDisableAsynchronousDelivery(JNIEnv* environment, jclass, jlong handle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->disableAsynchronousDelivery();
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeModifyLookahead(
    JNIEnv* environment, jclass, jlong handle, jbyteArray encodedLogicalTimeInterval) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->modifyLookahead(
        variableLengthData(environment, encodedLogicalTimeInterval));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

jbyteArray nativeQueryLookahead(JNIEnv* environment, jclass, jlong handle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return nullptr;
  try {
    return javaByteArray(environment, state->queryLookahead());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

void nativeTimeAdvanceRequest(
    JNIEnv* environment, jclass, jlong handle, jbyteArray encodedLogicalTime) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->timeAdvanceRequest(variableLengthData(environment, encodedLogicalTime));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeTimeAdvanceRequestAvailable(
    JNIEnv* environment, jclass, jlong handle, jbyteArray encodedLogicalTime) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->timeAdvanceRequestAvailable(
        variableLengthData(environment, encodedLogicalTime));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeNextMessageRequest(
    JNIEnv* environment, jclass, jlong handle, jbyteArray encodedLogicalTime) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->nextMessageRequest(variableLengthData(environment, encodedLogicalTime));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeNextMessageRequestAvailable(
    JNIEnv* environment, jclass, jlong handle, jbyteArray encodedLogicalTime) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->nextMessageRequestAvailable(
        variableLengthData(environment, encodedLogicalTime));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeFlushQueueRequest(
    JNIEnv* environment, jclass, jlong handle, jbyteArray encodedLogicalTime) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->flushQueueRequest(variableLengthData(environment, encodedLogicalTime));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

jbyteArray nativeQueryLogicalTime(JNIEnv* environment, jclass, jlong handle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return nullptr;
  try {
    return javaByteArray(environment, state->queryLogicalTime());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

jbyteArray nativeQueryGALT(JNIEnv* environment, jclass, jlong handle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return nullptr;
  try {
    auto const [valid, encodedTime] = state->queryGALT();
    return javaTimeQueryByteArray(environment, valid, encodedTime);
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

jbyteArray nativeQueryLITS(JNIEnv* environment, jclass, jlong handle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return nullptr;
  try {
    auto const [valid, encodedTime] = state->queryLITS();
    return javaTimeQueryByteArray(environment, valid, encodedTime);
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

jbyteArray nativeSendInteractionWithTime(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedInteractionClassHandle,
    jobject parameterValues,
    jbyteArray userSuppliedTag,
    jbyteArray encodedLogicalTime) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return nullptr;
  try {
    return javaByteArray(
        environment,
        state->sendInteractionWithTime(
            variableLengthData(environment, encodedInteractionClassHandle),
            parameterHandleValueMap(environment, parameterValues),
            variableLengthData(environment, userSuppliedTag),
            variableLengthData(environment, encodedLogicalTime)));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

void nativeRetract(
    JNIEnv* environment, jclass, jlong handle, jbyteArray encodedMessageRetractionHandle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->retract(variableLengthData(environment, encodedMessageRetractionHandle));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

jbyteArray nativeDecodeInteractionClassHandle(
    JNIEnv* environment, jclass, jbyteArray encodedValue) {
  try {
    auto decoded = rti::umbra_binding_detail::decodeInteractionClassHandle(
        variableLengthData(environment, encodedValue));
    return javaByteArray(environment, decoded.encode());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

jbyteArray nativeDecodeMessageRetractionHandle(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedMessageRetractionHandle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return nullptr;
  try {
    return javaByteArray(environment, state->decodeMessageRetractionHandle(
        variableLengthData(environment, encodedMessageRetractionHandle)));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

jbyteArray nativeGetInteractionClassHandle(
    JNIEnv* environment, jclass, jlong handle, jstring interactionClassName) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return nullptr;
  try {
    return javaByteArray(environment, state->getInteractionClassHandle(
        wideString(environment, interactionClassName)).encode());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

jstring nativeGetInteractionClassName(
    JNIEnv* environment, jclass, jlong handle, jbyteArray encodedInteractionClassHandle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return nullptr;
  try {
    return javaString(environment, state->getInteractionClassName(
        variableLengthData(environment, encodedInteractionClassHandle)));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

jlong nativeNormalizeInteractionClassHandle(
    JNIEnv* environment, jclass, jlong handle, jbyteArray encodedInteractionClassHandle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return 0;
  try {
    return static_cast<jlong>(state->normalizeInteractionClassHandle(
        variableLengthData(environment, encodedInteractionClassHandle)));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return 0;
}

jbyteArray nativeDecodeParameterHandle(JNIEnv* environment, jclass, jbyteArray encodedValue) {
  try {
    auto decoded = rti::umbra_binding_detail::decodeParameterHandle(
        variableLengthData(environment, encodedValue));
    return javaByteArray(environment, decoded.encode());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

jbyteArray nativeGetParameterHandle(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedInteractionClassHandle,
    jstring parameterName) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return nullptr;
  try {
    return javaByteArray(environment, state->getParameterHandle(
        variableLengthData(environment, encodedInteractionClassHandle),
        wideString(environment, parameterName)).encode());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

jstring nativeGetParameterName(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedInteractionClassHandle,
    jbyteArray encodedParameterHandle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return nullptr;
  try {
    return javaString(environment, state->getParameterName(
        variableLengthData(environment, encodedInteractionClassHandle),
        variableLengthData(environment, encodedParameterHandle)));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

void nativePublishInteractionClass(
    JNIEnv* environment, jclass, jlong handle, jbyteArray encodedInteractionClassHandle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->publishInteractionClass(
        variableLengthData(environment, encodedInteractionClassHandle));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeUnpublishInteractionClass(
    JNIEnv* environment, jclass, jlong handle, jbyteArray encodedInteractionClassHandle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->unpublishInteractionClass(
        variableLengthData(environment, encodedInteractionClassHandle));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeSubscribeInteractionClass(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedInteractionClassHandle,
    jboolean active) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->subscribeInteractionClass(
        variableLengthData(environment, encodedInteractionClassHandle), active == JNI_TRUE);
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeUnsubscribeInteractionClass(
    JNIEnv* environment, jclass, jlong handle, jbyteArray encodedInteractionClassHandle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->unsubscribeInteractionClass(
        variableLengthData(environment, encodedInteractionClassHandle));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeSubscribeInteractionClassWithRegions(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedInteractionClassHandle,
    jobject regions,
    jboolean active) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->subscribeInteractionClassWithRegions(
        variableLengthData(environment, encodedInteractionClassHandle),
        regionHandleSet(environment, regions),
        active == JNI_TRUE);
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeUnsubscribeInteractionClassWithRegions(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedInteractionClassHandle,
    jobject regions) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->unsubscribeInteractionClassWithRegions(
        variableLengthData(environment, encodedInteractionClassHandle),
        regionHandleSet(environment, regions));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

jbyteArray nativeDecodeTransportationTypeHandle(
    JNIEnv* environment, jclass, jbyteArray encodedValue) {
  try {
    auto decoded = rti::umbra_binding_detail::decodeTransportationTypeHandle(
        variableLengthData(environment, encodedValue));
    return javaByteArray(environment, decoded.encode());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

jbyteArray nativeGetTransportationTypeHandle(
    JNIEnv* environment, jclass, jlong handle, jstring transportationTypeName) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return nullptr;
  try {
    return javaByteArray(environment, state->getTransportationTypeHandle(
        wideString(environment, transportationTypeName)).encode());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

jstring nativeGetTransportationTypeName(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedTransportationTypeHandle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return nullptr;
  try {
    return javaString(environment, state->getTransportationTypeName(
        variableLengthData(environment, encodedTransportationTypeHandle)));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

void nativeSendInteraction(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedInteractionClassHandle,
    jobject parameterValues,
    jbyteArray userSuppliedTag) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->sendInteraction(
        variableLengthData(environment, encodedInteractionClassHandle),
        parameterHandleValueMap(environment, parameterValues),
        variableLengthData(environment, userSuppliedTag));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeSendDirectedInteraction(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedInteractionClassHandle,
    jbyteArray encodedObjectInstanceHandle,
    jobject parameterValues,
    jbyteArray userSuppliedTag) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->sendDirectedInteraction(
        variableLengthData(environment, encodedInteractionClassHandle),
        variableLengthData(environment, encodedObjectInstanceHandle),
        parameterHandleValueMap(environment, parameterValues),
        variableLengthData(environment, userSuppliedTag));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

jbyteArray nativeSendDirectedInteractionWithTime(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedInteractionClassHandle,
    jbyteArray encodedObjectInstanceHandle,
    jobject parameterValues,
    jbyteArray userSuppliedTag,
    jbyteArray encodedLogicalTime) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return nullptr;
  try {
    return javaByteArray(
        environment,
        state->sendDirectedInteractionWithTime(
            variableLengthData(environment, encodedInteractionClassHandle),
            variableLengthData(environment, encodedObjectInstanceHandle),
            parameterHandleValueMap(environment, parameterValues),
            variableLengthData(environment, userSuppliedTag),
            variableLengthData(environment, encodedLogicalTime)));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

void nativeSendInteractionWithRegions(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedInteractionClassHandle,
    jobject parameterValues,
    jobject regions,
    jbyteArray userSuppliedTag) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->sendInteractionWithRegions(
        variableLengthData(environment, encodedInteractionClassHandle),
        parameterHandleValueMap(environment, parameterValues),
        regionHandleSet(environment, regions),
        variableLengthData(environment, userSuppliedTag));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

jbyteArray nativeSendInteractionWithRegionsWithTime(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jbyteArray encodedInteractionClassHandle,
    jobject parameterValues,
    jobject regions,
    jbyteArray userSuppliedTag,
    jbyteArray encodedLogicalTime) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return nullptr;
  try {
    return javaByteArray(
        environment,
        state->sendInteractionWithRegionsWithTime(
            variableLengthData(environment, encodedInteractionClassHandle),
            parameterHandleValueMap(environment, parameterValues),
            regionHandleSet(environment, regions),
            variableLengthData(environment, userSuppliedTag),
            variableLengthData(environment, encodedLogicalTime)));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

jlong nativeCreateHLAinteger32BE(JNIEnv* environment, jclass, jint value) {
  try {
    return reinterpret_cast<jlong>(new NativeHLAinteger32BE(static_cast<std::int32_t>(value)));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return 0;
}

void nativeDestroyHLAinteger32BE(JNIEnv*, jclass, jlong handle) {
  delete reinterpret_cast<NativeHLAinteger32BE*>(handle);
}

jint nativeGetHLAinteger32BE(JNIEnv* environment, jclass, jlong handle) {
  auto* state = integer32StateFor(environment, handle);
  if (state == nullptr) return 0;
  try {
    return static_cast<jint>(state->element.get());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  }
  return 0;
}

void nativeSetHLAinteger32BE(JNIEnv* environment, jclass, jlong handle, jint value) {
  auto* state = integer32StateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->element.set(static_cast<std::int32_t>(value));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  }
}

jint nativeHLAinteger32BEOctetBoundary(JNIEnv* environment, jclass, jlong handle) {
  auto* state = integer32StateFor(environment, handle);
  if (state == nullptr) return 0;
  try {
    return static_cast<jint>(state->element.getOctetBoundary());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  }
  return 0;
}

jint nativeHLAinteger32BEEncodedLength(JNIEnv* environment, jclass, jlong handle) {
  auto* state = integer32StateFor(environment, handle);
  if (state == nullptr) return 0;
  try {
    return static_cast<jint>(state->element.getEncodedLength());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  }
  return 0;
}

jbyteArray nativeHLAinteger32BEToByteArray(JNIEnv* environment, jclass, jlong handle) {
  auto* state = integer32StateFor(environment, handle);
  if (state == nullptr) return nullptr;
  try {
    return javaByteArray(environment, state->element.encode());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

void nativeDecodeHLAinteger32BE(JNIEnv* environment, jclass, jlong handle, jbyteArray bytes) {
  auto* state = integer32StateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->element.decode(variableLengthData(environment, bytes));
  } catch (rti::Exception const& exception) {
    throwJavaDecoderException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

jlong nativeCreateHLAinteger32LE(JNIEnv* environment, jclass, jint value) {
  try {
    return reinterpret_cast<jlong>(new NativeHLAinteger32LE(static_cast<std::int32_t>(value)));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return 0;
}

void nativeDestroyHLAinteger32LE(JNIEnv*, jclass, jlong handle) {
  delete reinterpret_cast<NativeHLAinteger32LE*>(handle);
}

jint nativeGetHLAinteger32LE(JNIEnv* environment, jclass, jlong handle) {
  auto* state = integer32LEStateFor(environment, handle);
  if (state == nullptr) return 0;
  try {
    return static_cast<jint>(state->element.get());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  }
  return 0;
}

void nativeSetHLAinteger32LE(JNIEnv* environment, jclass, jlong handle, jint value) {
  auto* state = integer32LEStateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->element.set(static_cast<std::int32_t>(value));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  }
}

jint nativeHLAinteger32LEOctetBoundary(JNIEnv* environment, jclass, jlong handle) {
  auto* state = integer32LEStateFor(environment, handle);
  if (state == nullptr) return 0;
  try {
    return static_cast<jint>(state->element.getOctetBoundary());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  }
  return 0;
}

jint nativeHLAinteger32LEEncodedLength(JNIEnv* environment, jclass, jlong handle) {
  auto* state = integer32LEStateFor(environment, handle);
  if (state == nullptr) return 0;
  try {
    return static_cast<jint>(state->element.getEncodedLength());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  }
  return 0;
}

jbyteArray nativeHLAinteger32LEToByteArray(JNIEnv* environment, jclass, jlong handle) {
  auto* state = integer32LEStateFor(environment, handle);
  if (state == nullptr) return nullptr;
  try {
    return javaByteArray(environment, state->element.encode());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

void nativeDecodeHLAinteger32LE(JNIEnv* environment, jclass, jlong handle, jbyteArray bytes) {
  auto* state = integer32LEStateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->element.decode(variableLengthData(environment, bytes));
  } catch (rti::Exception const& exception) {
    throwJavaDecoderException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

#define UMBRA_DEFINE_SCALAR_ELEMENT_BINDING(NAME, STATE, JNI_TYPE, CPP_TYPE)                        \
  jlong nativeCreate##NAME(JNIEnv* environment, jclass, JNI_TYPE value) {                            \
    try {                                                                                               \
      return reinterpret_cast<jlong>(new STATE(static_cast<CPP_TYPE>(value)));                        \
    } catch (rti::Exception const& exception) {                                                        \
      throwJavaRtiException(environment, exception);                                                   \
    } catch (std::exception const& exception) {                                                        \
      throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what()); \
    }                                                                                                   \
    return 0;                                                                                           \
  }                                                                                                     \
  void nativeDestroy##NAME(JNIEnv*, jclass, jlong handle) {                                            \
    delete reinterpret_cast<STATE*>(handle);                                                           \
  }                                                                                                     \
  JNI_TYPE nativeGet##NAME(JNIEnv* environment, jclass, jlong handle) {                               \
    auto* state = scalarStateFor<STATE>(environment, handle, #NAME);                                  \
    if (state == nullptr) return 0;                                                                    \
    try {                                                                                               \
      return static_cast<JNI_TYPE>(state->get());                                                      \
    } catch (rti::Exception const& exception) {                                                        \
      throwJavaRtiException(environment, exception);                                                   \
    }                                                                                                   \
    return 0;                                                                                           \
  }                                                                                                     \
  void nativeSet##NAME(JNIEnv* environment, jclass, jlong handle, JNI_TYPE value) {                   \
    auto* state = scalarStateFor<STATE>(environment, handle, #NAME);                                  \
    if (state == nullptr) return;                                                                      \
    try {                                                                                               \
      state->set(static_cast<CPP_TYPE>(value));                                                        \
    } catch (rti::Exception const& exception) {                                                        \
      throwJavaRtiException(environment, exception);                                                   \
    }                                                                                                   \
  }                                                                                                     \
  jint native##NAME##OctetBoundary(JNIEnv* environment, jclass, jlong handle) {                       \
    auto* state = scalarStateFor<STATE>(environment, handle, #NAME);                                  \
    if (state == nullptr) return 0;                                                                    \
    try {                                                                                               \
      return static_cast<jint>(state->element.getOctetBoundary());                                    \
    } catch (rti::Exception const& exception) {                                                        \
      throwJavaRtiException(environment, exception);                                                   \
    }                                                                                                   \
    return 0;                                                                                           \
  }                                                                                                     \
  jint native##NAME##EncodedLength(JNIEnv* environment, jclass, jlong handle) {                       \
    auto* state = scalarStateFor<STATE>(environment, handle, #NAME);                                  \
    if (state == nullptr) return 0;                                                                    \
    try {                                                                                               \
      return static_cast<jint>(state->element.getEncodedLength());                                    \
    } catch (rti::Exception const& exception) {                                                        \
      throwJavaRtiException(environment, exception);                                                   \
    }                                                                                                   \
    return 0;                                                                                           \
  }                                                                                                     \
  jbyteArray native##NAME##ToByteArray(JNIEnv* environment, jclass, jlong handle) {                   \
    auto* state = scalarStateFor<STATE>(environment, handle, #NAME);                                  \
    if (state == nullptr) return nullptr;                                                              \
    try {                                                                                               \
      return javaByteArray(environment, state->element.encode());                                     \
    } catch (rti::Exception const& exception) {                                                        \
      throwJavaRtiException(environment, exception);                                                   \
    } catch (std::exception const& exception) {                                                        \
      throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what()); \
    }                                                                                                   \
    return nullptr;                                                                                     \
  }                                                                                                     \
  void nativeDecode##NAME(JNIEnv* environment, jclass, jlong handle, jbyteArray bytes) {              \
    auto* state = scalarStateFor<STATE>(environment, handle, #NAME);                                  \
    if (state == nullptr) return;                                                                      \
    try {                                                                                               \
      state->element.decode(variableLengthData(environment, bytes));                                  \
    } catch (rti::Exception const& exception) {                                                        \
      throwJavaDecoderException(environment, exception);                                               \
    } catch (std::exception const& exception) {                                                        \
      throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what()); \
    }                                                                                                   \
  }

UMBRA_DEFINE_SCALAR_ELEMENT_BINDING(HLAfloat32BE, NativeHLAfloat32BE, jfloat, float)
UMBRA_DEFINE_SCALAR_ELEMENT_BINDING(HLAfloat32LE, NativeHLAfloat32LE, jfloat, float)
UMBRA_DEFINE_SCALAR_ELEMENT_BINDING(HLAfloat64BE, NativeHLAfloat64BE, jdouble, double)
UMBRA_DEFINE_SCALAR_ELEMENT_BINDING(HLAfloat64LE, NativeHLAfloat64LE, jdouble, double)
UMBRA_DEFINE_SCALAR_ELEMENT_BINDING(
    HLAunsignedInteger16BE, NativeHLAunsignedInteger16BE, jshort, std::uint16_t)
UMBRA_DEFINE_SCALAR_ELEMENT_BINDING(
    HLAunsignedInteger16LE, NativeHLAunsignedInteger16LE, jshort, std::uint16_t)
UMBRA_DEFINE_SCALAR_ELEMENT_BINDING(
    HLAunsignedInteger32BE, NativeHLAunsignedInteger32BE, jint, std::uint32_t)
UMBRA_DEFINE_SCALAR_ELEMENT_BINDING(
    HLAunsignedInteger32LE, NativeHLAunsignedInteger32LE, jint, std::uint32_t)
UMBRA_DEFINE_SCALAR_ELEMENT_BINDING(
    HLAunsignedInteger64BE, NativeHLAunsignedInteger64BE, jlong, std::uint64_t)
UMBRA_DEFINE_SCALAR_ELEMENT_BINDING(
    HLAunsignedInteger64LE, NativeHLAunsignedInteger64LE, jlong, std::uint64_t)
UMBRA_DEFINE_SCALAR_ELEMENT_BINDING(HLAbyte, NativeHLAbyte, jbyte, rti::Octet)
UMBRA_DEFINE_SCALAR_ELEMENT_BINDING(HLAoctet, NativeHLAoctet, jbyte, rti::Octet)
UMBRA_DEFINE_SCALAR_ELEMENT_BINDING(
    HLAoctetPairBE, NativeHLAoctetPairBE, jshort, std::uint16_t)
UMBRA_DEFINE_SCALAR_ELEMENT_BINDING(
    HLAoctetPairLE, NativeHLAoctetPairLE, jshort, std::uint16_t)
UMBRA_DEFINE_SCALAR_ELEMENT_BINDING(
    HLAASCIIchar, NativeHLAASCIIchar, jbyte, std::uint8_t)
UMBRA_DEFINE_SCALAR_ELEMENT_BINDING(
    HLAunicodeChar, NativeHLAunicodeChar, jshort, std::uint16_t)
UMBRA_DEFINE_SCALAR_ELEMENT_BINDING(HLAboolean, NativeHLAboolean, jboolean, bool)

#undef UMBRA_DEFINE_SCALAR_ELEMENT_BINDING

#define UMBRA_DEFINE_STRING_ELEMENT_BINDING(NAME, STATE, FROM_JAVA, TO_JAVA)                        \
  jlong nativeCreate##NAME(JNIEnv* environment, jclass, jstring value) {                             \
    try {                                                                                               \
      return reinterpret_cast<jlong>(new STATE(FROM_JAVA(environment, value)));                       \
    } catch (rti::Exception const& exception) {                                                        \
      throwJavaRtiException(environment, exception);                                                   \
    } catch (std::exception const& exception) {                                                        \
      throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what()); \
    }                                                                                                   \
    return 0;                                                                                           \
  }                                                                                                     \
  void nativeDestroy##NAME(JNIEnv*, jclass, jlong handle) {                                            \
    delete reinterpret_cast<STATE*>(handle);                                                           \
  }                                                                                                     \
  jstring nativeGet##NAME(JNIEnv* environment, jclass, jlong handle) {                                \
    auto* state = scalarStateFor<STATE>(environment, handle, #NAME);                                  \
    if (state == nullptr) return nullptr;                                                              \
    try {                                                                                               \
      return TO_JAVA(environment, state->get());                                                       \
    } catch (rti::Exception const& exception) {                                                        \
      throwJavaRtiException(environment, exception);                                                   \
    } catch (std::exception const& exception) {                                                        \
      throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what()); \
    }                                                                                                   \
    return nullptr;                                                                                     \
  }                                                                                                     \
  void nativeSet##NAME(JNIEnv* environment, jclass, jlong handle, jstring value) {                    \
    auto* state = scalarStateFor<STATE>(environment, handle, #NAME);                                  \
    if (state == nullptr) return;                                                                      \
    try {                                                                                               \
      state->set(FROM_JAVA(environment, value));                                                       \
    } catch (rti::Exception const& exception) {                                                        \
      throwJavaRtiException(environment, exception);                                                   \
    } catch (std::exception const& exception) {                                                        \
      throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what()); \
    }                                                                                                   \
  }                                                                                                     \
  jint native##NAME##OctetBoundary(JNIEnv* environment, jclass, jlong handle) {                       \
    auto* state = scalarStateFor<STATE>(environment, handle, #NAME);                                  \
    if (state == nullptr) return 0;                                                                    \
    try {                                                                                               \
      return static_cast<jint>(state->element.getOctetBoundary());                                    \
    } catch (rti::Exception const& exception) {                                                        \
      throwJavaRtiException(environment, exception);                                                   \
    }                                                                                                   \
    return 0;                                                                                           \
  }                                                                                                     \
  jint native##NAME##EncodedLength(JNIEnv* environment, jclass, jlong handle) {                       \
    auto* state = scalarStateFor<STATE>(environment, handle, #NAME);                                  \
    if (state == nullptr) return 0;                                                                    \
    try {                                                                                               \
      return static_cast<jint>(state->element.getEncodedLength());                                    \
    } catch (rti::Exception const& exception) {                                                        \
      throwJavaRtiException(environment, exception);                                                   \
    }                                                                                                   \
    return 0;                                                                                           \
  }                                                                                                     \
  jbyteArray native##NAME##ToByteArray(JNIEnv* environment, jclass, jlong handle) {                   \
    auto* state = scalarStateFor<STATE>(environment, handle, #NAME);                                  \
    if (state == nullptr) return nullptr;                                                              \
    try {                                                                                               \
      return javaByteArray(environment, state->element.encode());                                     \
    } catch (rti::Exception const& exception) {                                                        \
      throwJavaRtiException(environment, exception);                                                   \
    } catch (std::exception const& exception) {                                                        \
      throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what()); \
    }                                                                                                   \
    return nullptr;                                                                                     \
  }                                                                                                     \
  void nativeDecode##NAME(JNIEnv* environment, jclass, jlong handle, jbyteArray bytes) {              \
    auto* state = scalarStateFor<STATE>(environment, handle, #NAME);                                  \
    if (state == nullptr) return;                                                                      \
    try {                                                                                               \
      state->element.decode(variableLengthData(environment, bytes));                                  \
    } catch (rti::Exception const& exception) {                                                        \
      throwJavaDecoderException(environment, exception);                                               \
    } catch (std::exception const& exception) {                                                        \
      throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what()); \
    }                                                                                                   \
  }

UMBRA_DEFINE_STRING_ELEMENT_BINDING(
    HLAASCIIstring, NativeHLAASCIIstring, asciiString, javaAsciiString)
UMBRA_DEFINE_STRING_ELEMENT_BINDING(
    HLAunicodeString, NativeHLAunicodeString, wideString, javaString)

#undef UMBRA_DEFINE_STRING_ELEMENT_BINDING

jlong nativeCreateHLAopaqueData(JNIEnv* environment, jclass, jbyteArray value) {
  try {
    return reinterpret_cast<jlong>(new NativeHLAopaqueData(octetVector(environment, value)));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return 0;
}

void nativeDestroyHLAopaqueData(JNIEnv*, jclass, jlong handle) {
  delete reinterpret_cast<NativeHLAopaqueData*>(handle);
}

jint nativeHLAopaqueDataSize(JNIEnv* environment, jclass, jlong handle) {
  auto* state = scalarStateFor<NativeHLAopaqueData>(environment, handle, "HLAopaqueData");
  if (state == nullptr) return 0;
  try {
    return static_cast<jint>(state->element.dataLength());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  }
  return 0;
}

jbyte nativeGetHLAopaqueData(JNIEnv* environment, jclass, jlong handle, jint index) {
  auto* state = scalarStateFor<NativeHLAopaqueData>(environment, handle, "HLAopaqueData");
  if (state == nullptr) return 0;
  if (index < 0 || static_cast<std::size_t>(index) >= state->element.dataLength()) {
    throwJavaException(environment, "java/lang/IndexOutOfBoundsException", "HLAopaqueData index is out of range");
    return 0;
  }
  try {
    return static_cast<jbyte>(state->get(static_cast<std::size_t>(index)));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  }
  return 0;
}

jbyteArray nativeGetHLAopaqueDataValue(JNIEnv* environment, jclass, jlong handle) {
  auto* state = scalarStateFor<NativeHLAopaqueData>(environment, handle, "HLAopaqueData");
  if (state == nullptr) return nullptr;
  try {
    return javaByteArray(environment, state->get());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

void nativeSetHLAopaqueData(JNIEnv* environment, jclass, jlong handle, jbyteArray value) {
  auto* state = scalarStateFor<NativeHLAopaqueData>(environment, handle, "HLAopaqueData");
  if (state == nullptr) return;
  try {
    state->set(octetVector(environment, value));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

jint nativeHLAopaqueDataOctetBoundary(JNIEnv* environment, jclass, jlong handle) {
  auto* state = scalarStateFor<NativeHLAopaqueData>(environment, handle, "HLAopaqueData");
  if (state == nullptr) return 0;
  try {
    return static_cast<jint>(state->element.getOctetBoundary());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  }
  return 0;
}

jint nativeHLAopaqueDataEncodedLength(JNIEnv* environment, jclass, jlong handle) {
  auto* state = scalarStateFor<NativeHLAopaqueData>(environment, handle, "HLAopaqueData");
  if (state == nullptr) return 0;
  try {
    return static_cast<jint>(state->element.getEncodedLength());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  }
  return 0;
}

jbyteArray nativeHLAopaqueDataToByteArray(JNIEnv* environment, jclass, jlong handle) {
  auto* state = scalarStateFor<NativeHLAopaqueData>(environment, handle, "HLAopaqueData");
  if (state == nullptr) return nullptr;
  try {
    return javaByteArray(environment, state->element.encode());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

void nativeDecodeHLAopaqueData(JNIEnv* environment, jclass, jlong handle, jbyteArray bytes) {
  auto* state = scalarStateFor<NativeHLAopaqueData>(environment, handle, "HLAopaqueData");
  if (state == nullptr) return;
  try {
    state->element.decode(variableLengthData(environment, bytes));
  } catch (rti::Exception const& exception) {
    throwJavaDecoderException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

jlong nativeCreateHLAvariableArray(JNIEnv* environment, jclass, jobject prototype) {
  try {
    auto nativePrototype = nativeDataElementFromJava(environment, prototype);
    if (nativePrototype == nullptr) {
      if (!environment->ExceptionCheck()) {
        throwJavaException(
            environment,
            "hla/rti1516_2025/encoding/EncoderException",
            "HLAvariableArray prototype is not a supported native data element");
      }
      return 0;
    }
    return reinterpret_cast<jlong>(new NativeHLAvariableArray(std::move(nativePrototype)));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return 0;
}

void nativeDestroyHLAvariableArray(JNIEnv*, jclass, jlong handle) {
  delete reinterpret_cast<NativeHLAvariableArray*>(handle);
}

void nativeAddHLAvariableArrayElement(JNIEnv* environment, jclass, jlong handle, jobject value) {
  auto* state = scalarStateFor<NativeHLAvariableArray>(environment, handle, "HLAvariableArray");
  if (state == nullptr) return;
  try {
    state->add(nativeDataElementFromJava(environment, value, *state->prototype));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeResizeHLAvariableArray(JNIEnv* environment, jclass, jlong handle, jint size) {
  auto* state = scalarStateFor<NativeHLAvariableArray>(environment, handle, "HLAvariableArray");
  if (state == nullptr) return;
  if (size < 0) {
    throwJavaException(environment, "java/lang/IllegalArgumentException", "HLAvariableArray size is negative");
    return;
  }
  try {
    state->resize(static_cast<std::size_t>(size));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

jint nativeHLAvariableArraySize(JNIEnv* environment, jclass, jlong handle) {
  auto* state = scalarStateFor<NativeHLAvariableArray>(environment, handle, "HLAvariableArray");
  if (state == nullptr) return 0;
  try {
    return static_cast<jint>(state->element->size());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  }
  return 0;
}

jbyteArray nativeHLAvariableArrayElementEncoding(
    JNIEnv* environment, jclass, jlong handle, jint index) {
  auto* state = scalarStateFor<NativeHLAvariableArray>(environment, handle, "HLAvariableArray");
  if (state == nullptr) return nullptr;
  if (index < 0 || static_cast<std::size_t>(index) >= state->element->size()) {
    throwJavaException(environment, "java/lang/IndexOutOfBoundsException", "HLAvariableArray index is out of range");
    return nullptr;
  }
  try {
    return javaByteArray(environment, state->encodedElement(static_cast<std::size_t>(index)));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

jint nativeHLAvariableArrayOctetBoundary(JNIEnv* environment, jclass, jlong handle) {
  auto* state = scalarStateFor<NativeHLAvariableArray>(environment, handle, "HLAvariableArray");
  if (state == nullptr) return 0;
  try {
    return static_cast<jint>(state->element->getOctetBoundary());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  }
  return 0;
}

jint nativeHLAvariableArrayEncodedLength(JNIEnv* environment, jclass, jlong handle) {
  auto* state = scalarStateFor<NativeHLAvariableArray>(environment, handle, "HLAvariableArray");
  if (state == nullptr) return 0;
  try {
    return static_cast<jint>(state->element->getEncodedLength());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  }
  return 0;
}

jbyteArray nativeHLAvariableArrayToByteArray(JNIEnv* environment, jclass, jlong handle) {
  auto* state = scalarStateFor<NativeHLAvariableArray>(environment, handle, "HLAvariableArray");
  if (state == nullptr) return nullptr;
  try {
    return javaByteArray(environment, state->element->encode());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

void nativeDecodeHLAvariableArray(JNIEnv* environment, jclass, jlong handle, jbyteArray bytes) {
  auto* state = scalarStateFor<NativeHLAvariableArray>(environment, handle, "HLAvariableArray");
  if (state == nullptr) return;
  try {
    state->element->decode(variableLengthData(environment, bytes));
  } catch (rti::Exception const& exception) {
    throwJavaDecoderException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

jlong nativeCreateHLAfixedArray(JNIEnv* environment, jclass, jobject prototype, jint size) {
  if (size < 0) {
    throwJavaException(environment, "hla/rti1516_2025/encoding/EncoderException", "HLAfixedArray size is negative");
    return 0;
  }
  try {
    auto nativePrototype = nativeDataElementFromJava(environment, prototype);
    if (nativePrototype == nullptr) {
      if (!environment->ExceptionCheck()) {
        throwJavaException(
            environment,
            "hla/rti1516_2025/encoding/EncoderException",
            "HLAfixedArray prototype is not a supported native data element");
      }
      return 0;
    }
    return reinterpret_cast<jlong>(new NativeHLAfixedArray(
        std::move(nativePrototype), static_cast<std::size_t>(size)));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return 0;
}

void nativeDestroyHLAfixedArray(JNIEnv*, jclass, jlong handle) {
  delete reinterpret_cast<NativeHLAfixedArray*>(handle);
}

void nativeSetHLAfixedArrayElement(JNIEnv* environment, jclass, jlong handle, jint index, jobject value) {
  auto* state = scalarStateFor<NativeHLAfixedArray>(environment, handle, "HLAfixedArray");
  if (state == nullptr) return;
  if (index < 0 || static_cast<std::size_t>(index) >= state->element->size()) {
    throwJavaException(environment, "java/lang/IndexOutOfBoundsException", "HLAfixedArray index is out of range");
    return;
  }
  try {
    state->set(static_cast<std::size_t>(index), nativeDataElementFromJava(environment, value, *state->prototype));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

jint nativeHLAfixedArraySize(JNIEnv* environment, jclass, jlong handle) {
  auto* state = scalarStateFor<NativeHLAfixedArray>(environment, handle, "HLAfixedArray");
  if (state == nullptr) return 0;
  try { return static_cast<jint>(state->element->size()); }
  catch (rti::Exception const& exception) { throwJavaRtiException(environment, exception); }
  return 0;
}

jbyteArray nativeHLAfixedArrayElementEncoding(JNIEnv* environment, jclass, jlong handle, jint index) {
  auto* state = scalarStateFor<NativeHLAfixedArray>(environment, handle, "HLAfixedArray");
  if (state == nullptr) return nullptr;
  if (index < 0 || static_cast<std::size_t>(index) >= state->element->size()) {
    throwJavaException(environment, "java/lang/IndexOutOfBoundsException", "HLAfixedArray index is out of range");
    return nullptr;
  }
  try { return javaByteArray(environment, state->encodedElement(static_cast<std::size_t>(index))); }
  catch (rti::Exception const& exception) { throwJavaRtiException(environment, exception); }
  catch (std::exception const& exception) { throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what()); }
  return nullptr;
}

jint nativeHLAfixedArrayOctetBoundary(JNIEnv* environment, jclass, jlong handle) {
  auto* state = scalarStateFor<NativeHLAfixedArray>(environment, handle, "HLAfixedArray");
  if (state == nullptr) return 0;
  try { return static_cast<jint>(state->element->getOctetBoundary()); }
  catch (rti::Exception const& exception) { throwJavaRtiException(environment, exception); }
  return 0;
}

jint nativeHLAfixedArrayEncodedLength(JNIEnv* environment, jclass, jlong handle) {
  auto* state = scalarStateFor<NativeHLAfixedArray>(environment, handle, "HLAfixedArray");
  if (state == nullptr) return 0;
  try { return static_cast<jint>(state->element->getEncodedLength()); }
  catch (rti::Exception const& exception) { throwJavaRtiException(environment, exception); }
  return 0;
}

jbyteArray nativeHLAfixedArrayToByteArray(JNIEnv* environment, jclass, jlong handle) {
  auto* state = scalarStateFor<NativeHLAfixedArray>(environment, handle, "HLAfixedArray");
  if (state == nullptr) return nullptr;
  try { return javaByteArray(environment, state->element->encode()); }
  catch (rti::Exception const& exception) { throwJavaRtiException(environment, exception); }
  catch (std::exception const& exception) { throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what()); }
  return nullptr;
}

void nativeDecodeHLAfixedArray(JNIEnv* environment, jclass, jlong handle, jbyteArray bytes) {
  auto* state = scalarStateFor<NativeHLAfixedArray>(environment, handle, "HLAfixedArray");
  if (state == nullptr) return;
  try { state->element->decode(variableLengthData(environment, bytes)); }
  catch (rti::Exception const& exception) { throwJavaDecoderException(environment, exception); }
  catch (std::exception const& exception) { throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what()); }
}

jlong nativeCreateHLAfixedRecord(JNIEnv*, jclass) { return reinterpret_cast<jlong>(new NativeHLAfixedRecord()); }
void nativeDestroyHLAfixedRecord(JNIEnv*, jclass, jlong handle) { delete reinterpret_cast<NativeHLAfixedRecord*>(handle); }
void nativeAppendHLAfixedRecordElement(JNIEnv* environment, jclass, jlong handle, jobject value) {
  auto* state = scalarStateFor<NativeHLAfixedRecord>(environment, handle, "HLAfixedRecord"); if (state == nullptr) return;
  try { state->append(nativeDataElementFromJava(environment, value)); }
  catch (rti::Exception const& exception) { throwJavaRtiException(environment, exception); }
  catch (std::exception const& exception) { throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what()); }
}
void nativeSetHLAfixedRecordElement(JNIEnv* environment, jclass, jlong handle, jint index, jobject value) {
  auto* state = scalarStateFor<NativeHLAfixedRecord>(environment, handle, "HLAfixedRecord"); if (state == nullptr) return;
  if (index < 0 || static_cast<std::size_t>(index) >= state->prototypes.size()) { throwJavaException(environment, "java/lang/IndexOutOfBoundsException", "HLAfixedRecord index is out of range"); return; }
  try { state->set(static_cast<std::size_t>(index), nativeDataElementFromJava(environment, value)); }
  catch (rti::Exception const& exception) { throwJavaRtiException(environment, exception); }
  catch (std::exception const& exception) { throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what()); }
}
jint nativeHLAfixedRecordSize(JNIEnv* environment, jclass, jlong handle) { auto* state = scalarStateFor<NativeHLAfixedRecord>(environment, handle, "HLAfixedRecord"); return state == nullptr ? 0 : static_cast<jint>(state->element.size()); }
jbyteArray nativeHLAfixedRecordElementEncoding(JNIEnv* environment, jclass, jlong handle, jint index) {
  auto* state = scalarStateFor<NativeHLAfixedRecord>(environment, handle, "HLAfixedRecord"); if (state == nullptr) return nullptr;
  if (index < 0 || static_cast<std::size_t>(index) >= state->prototypes.size()) { throwJavaException(environment, "java/lang/IndexOutOfBoundsException", "HLAfixedRecord index is out of range"); return nullptr; }
  try { return javaByteArray(environment, state->encodedElement(static_cast<std::size_t>(index))); }
  catch (rti::Exception const& exception) { throwJavaRtiException(environment, exception); }
  return nullptr;
}
jint nativeHLAfixedRecordOctetBoundary(JNIEnv* environment, jclass, jlong handle) { auto* state = scalarStateFor<NativeHLAfixedRecord>(environment, handle, "HLAfixedRecord"); return state == nullptr ? 0 : static_cast<jint>(state->element.getOctetBoundary()); }
jint nativeHLAfixedRecordEncodedLength(JNIEnv* environment, jclass, jlong handle) { auto* state = scalarStateFor<NativeHLAfixedRecord>(environment, handle, "HLAfixedRecord"); if (state == nullptr) return 0; try { return static_cast<jint>(state->element.getEncodedLength()); } catch (rti::Exception const& exception) { throwJavaRtiException(environment, exception); } return 0; }
jbyteArray nativeHLAfixedRecordToByteArray(JNIEnv* environment, jclass, jlong handle) { auto* state = scalarStateFor<NativeHLAfixedRecord>(environment, handle, "HLAfixedRecord"); if (state == nullptr) return nullptr; try { return javaByteArray(environment, state->element.encode()); } catch (rti::Exception const& exception) { throwJavaRtiException(environment, exception); } return nullptr; }
void nativeDecodeHLAfixedRecord(JNIEnv* environment, jclass, jlong handle, jbyteArray bytes) { auto* state = scalarStateFor<NativeHLAfixedRecord>(environment, handle, "HLAfixedRecord"); if (state == nullptr) return; try { state->element.decode(variableLengthData(environment, bytes)); } catch (rti::Exception const& exception) { throwJavaDecoderException(environment, exception); } }

jlong nativeCreateHLAvariantRecord(JNIEnv* environment, jclass, jobject discriminantPrototype) {
  try {
    return reinterpret_cast<jlong>(new NativeHLAvariantRecord(
        nativeDataElementFromJava(environment, discriminantPrototype)));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return 0;
}

void nativeDestroyHLAvariantRecord(JNIEnv*, jclass, jlong handle) {
  delete reinterpret_cast<NativeHLAvariantRecord*>(handle);
}

void nativeSetHLAvariantRecordVariant(
    JNIEnv* environment, jclass, jlong handle, jobject discriminantValue, jobject value) {
  auto* state = scalarStateFor<NativeHLAvariantRecord>(environment, handle, "HLAvariantRecord");
  if (state == nullptr) return;
  try {
    auto discriminant = nativeDataElementFromJava(
        environment, discriminantValue, *state->discriminantPrototype);
    auto discriminantEncoding = NativeHLAvariantRecord::encodedElement(*discriminant);
    auto nativeValue = nativeDataElementFromJava(environment, value);
    auto const* expectedPrototype = state->valuePrototypeFor(discriminantEncoding);
    if (expectedPrototype == nullptr) {
      state->addVariant(
          std::move(discriminantEncoding), std::move(discriminant), std::move(nativeValue));
    } else if (nativeValue->isSameTypeAs(*expectedPrototype)) {
      state->setVariant(std::move(discriminant), std::move(nativeValue));
    } else {
      throwJavaException(
          environment,
          "hla/rti1516_2025/encoding/EncoderException",
          "HLAvariantRecord replacement value type does not match its mapped variant");
    }
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeSetHLAvariantRecordDiscriminant(
    JNIEnv* environment, jclass, jlong handle, jobject discriminantValue) {
  auto* state = scalarStateFor<NativeHLAvariantRecord>(environment, handle, "HLAvariantRecord");
  if (state == nullptr) return;
  try {
    state->element->setDiscriminant(*nativeDataElementFromJava(
        environment, discriminantValue, *state->discriminantPrototype));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

jbyteArray nativeHLAvariantRecordDiscriminantEncoding(JNIEnv* environment, jclass, jlong handle) {
  auto* state = scalarStateFor<NativeHLAvariantRecord>(environment, handle, "HLAvariantRecord");
  if (state == nullptr) return nullptr;
  try {
    return javaByteArray(environment, state->discriminantEncoding());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  }
  return nullptr;
}

jbyteArray nativeHLAvariantRecordValueEncoding(JNIEnv* environment, jclass, jlong handle) {
  auto* state = scalarStateFor<NativeHLAvariantRecord>(environment, handle, "HLAvariantRecord");
  if (state == nullptr) return nullptr;
  try {
    return javaByteArray(environment, state->valueEncoding());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  }
  return nullptr;
}

jint nativeHLAvariantRecordOctetBoundary(JNIEnv* environment, jclass, jlong handle) {
  auto* state = scalarStateFor<NativeHLAvariantRecord>(environment, handle, "HLAvariantRecord");
  if (state == nullptr) return 0;
  try {
    return static_cast<jint>(state->element->getOctetBoundary());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  }
  return 0;
}

jint nativeHLAvariantRecordEncodedLength(JNIEnv* environment, jclass, jlong handle) {
  auto* state = scalarStateFor<NativeHLAvariantRecord>(environment, handle, "HLAvariantRecord");
  if (state == nullptr) return 0;
  try {
    return static_cast<jint>(state->element->getEncodedLength());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  }
  return 0;
}

jbyteArray nativeHLAvariantRecordToByteArray(JNIEnv* environment, jclass, jlong handle) {
  auto* state = scalarStateFor<NativeHLAvariantRecord>(environment, handle, "HLAvariantRecord");
  if (state == nullptr) return nullptr;
  try {
    return javaByteArray(environment, state->element->encode());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  }
  return nullptr;
}

void nativeDecodeHLAvariantRecord(JNIEnv* environment, jclass, jlong handle, jbyteArray bytes) {
  auto* state = scalarStateFor<NativeHLAvariantRecord>(environment, handle, "HLAvariantRecord");
  if (state == nullptr) return;
  try {
    state->element->decode(variableLengthData(environment, bytes));
  } catch (rti::Exception const& exception) {
    throwJavaDecoderException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

jlong nativeCreateHLAextendableVariantRecord(JNIEnv* environment, jclass, jobject discriminantPrototype) {
  try {
    return reinterpret_cast<jlong>(new NativeHLAextendableVariantRecord(
        nativeDataElementFromJava(environment, discriminantPrototype)));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return 0;
}

void nativeDestroyHLAextendableVariantRecord(JNIEnv*, jclass, jlong handle) {
  delete reinterpret_cast<NativeHLAextendableVariantRecord*>(handle);
}

void nativeAddHLAextendableVariantRecordVariant(
    JNIEnv* environment, jclass, jlong handle, jobject discriminantValue, jobject value) {
  auto* state = scalarStateFor<NativeHLAextendableVariantRecord>(
      environment, handle, "HLAextendableVariantRecord");
  if (state == nullptr) return;
  try {
    auto discriminant = nativeDataElementFromJava(
        environment, discriminantValue, *state->discriminantPrototype);
    auto discriminantEncoding = NativeHLAextendableVariantRecord::encodedElement(*discriminant);
    if (state->valuePrototypeFor(discriminantEncoding) != nullptr) {
      throw rti::EncoderException(L"HLAextendableVariantRecord discriminant is already mapped.");
    }
    state->addVariant(
        std::move(discriminantEncoding),
        std::move(discriminant),
        nativeDataElementFromJava(environment, value));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeSetHLAextendableVariantRecordVariant(
    JNIEnv* environment, jclass, jlong handle, jobject discriminantValue, jobject value) {
  auto* state = scalarStateFor<NativeHLAextendableVariantRecord>(
      environment, handle, "HLAextendableVariantRecord");
  if (state == nullptr) return;
  try {
    auto discriminant = nativeDataElementFromJava(
        environment, discriminantValue, *state->discriminantPrototype);
    auto nativeValue = nativeDataElementFromJava(environment, value);
    state->element->setVariant(*discriminant, *nativeValue);
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeSetHLAextendableVariantRecordDiscriminant(
    JNIEnv* environment, jclass, jlong handle, jobject discriminantValue) {
  auto* state = scalarStateFor<NativeHLAextendableVariantRecord>(
      environment, handle, "HLAextendableVariantRecord");
  if (state == nullptr) return;
  try {
    state->element->setDiscriminant(*nativeDataElementFromJava(
        environment, discriminantValue, *state->discriminantPrototype));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

jbyteArray nativeHLAextendableVariantRecordDiscriminantEncoding(JNIEnv* environment, jclass, jlong handle) {
  auto* state = scalarStateFor<NativeHLAextendableVariantRecord>(
      environment, handle, "HLAextendableVariantRecord");
  if (state == nullptr) return nullptr;
  try {
    return javaByteArray(environment, state->discriminantEncoding());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  }
  return nullptr;
}

jbyteArray nativeHLAextendableVariantRecordValueEncoding(JNIEnv* environment, jclass, jlong handle) {
  auto* state = scalarStateFor<NativeHLAextendableVariantRecord>(
      environment, handle, "HLAextendableVariantRecord");
  if (state == nullptr) return nullptr;
  try {
    return javaByteArray(environment, state->valueEncoding());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  }
  return nullptr;
}

jint nativeHLAextendableVariantRecordOctetBoundary(JNIEnv* environment, jclass, jlong handle) {
  auto* state = scalarStateFor<NativeHLAextendableVariantRecord>(
      environment, handle, "HLAextendableVariantRecord");
  if (state == nullptr) return 0;
  try {
    return static_cast<jint>(state->element->getOctetBoundary());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  }
  return 0;
}

jint nativeHLAextendableVariantRecordEncodedLength(JNIEnv* environment, jclass, jlong handle) {
  auto* state = scalarStateFor<NativeHLAextendableVariantRecord>(
      environment, handle, "HLAextendableVariantRecord");
  if (state == nullptr) return 0;
  try {
    return static_cast<jint>(state->element->getEncodedLength());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  }
  return 0;
}

jbyteArray nativeHLAextendableVariantRecordToByteArray(JNIEnv* environment, jclass, jlong handle) {
  auto* state = scalarStateFor<NativeHLAextendableVariantRecord>(
      environment, handle, "HLAextendableVariantRecord");
  if (state == nullptr) return nullptr;
  try {
    return javaByteArray(environment, state->element->encode());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  }
  return nullptr;
}

void nativeDecodeHLAextendableVariantRecord(JNIEnv* environment, jclass, jlong handle, jbyteArray bytes) {
  auto* state = scalarStateFor<NativeHLAextendableVariantRecord>(
      environment, handle, "HLAextendableVariantRecord");
  if (state == nullptr) return;
  try {
    state->element->decode(variableLengthData(environment, bytes));
  } catch (rti::Exception const& exception) {
    throwJavaDecoderException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

jlong nativeCreateHLAinteger64BE(JNIEnv* environment, jclass, jlong value) {
  try {
    return reinterpret_cast<jlong>(new NativeHLAinteger64BE(static_cast<std::int64_t>(value)));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return 0;
}

void nativeDestroyHLAinteger64BE(JNIEnv*, jclass, jlong handle) {
  delete reinterpret_cast<NativeHLAinteger64BE*>(handle);
}

jlong nativeGetHLAinteger64BE(JNIEnv* environment, jclass, jlong handle) {
  auto* state = integer64StateFor(environment, handle);
  if (state == nullptr) return 0;
  try {
    return static_cast<jlong>(state->element.get());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  }
  return 0;
}

void nativeSetHLAinteger64BE(JNIEnv* environment, jclass, jlong handle, jlong value) {
  auto* state = integer64StateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->element.set(static_cast<std::int64_t>(value));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  }
}

jint nativeHLAinteger64BEOctetBoundary(JNIEnv* environment, jclass, jlong handle) {
  auto* state = integer64StateFor(environment, handle);
  if (state == nullptr) return 0;
  try {
    return static_cast<jint>(state->element.getOctetBoundary());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  }
  return 0;
}

jint nativeHLAinteger64BEEncodedLength(JNIEnv* environment, jclass, jlong handle) {
  auto* state = integer64StateFor(environment, handle);
  if (state == nullptr) return 0;
  try {
    return static_cast<jint>(state->element.getEncodedLength());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  }
  return 0;
}

jbyteArray nativeHLAinteger64BEToByteArray(JNIEnv* environment, jclass, jlong handle) {
  auto* state = integer64StateFor(environment, handle);
  if (state == nullptr) return nullptr;
  try {
    return javaByteArray(environment, state->element.encode());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

void nativeDecodeHLAinteger64BE(JNIEnv* environment, jclass, jlong handle, jbyteArray bytes) {
  auto* state = integer64StateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->element.decode(variableLengthData(environment, bytes));
  } catch (rti::Exception const& exception) {
    throwJavaDecoderException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

jlong nativeCreateHLAinteger64LE(JNIEnv* environment, jclass, jlong value) {
  try {
    return reinterpret_cast<jlong>(new NativeHLAinteger64LE(static_cast<std::int64_t>(value)));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return 0;
}

void nativeDestroyHLAinteger64LE(JNIEnv*, jclass, jlong handle) {
  delete reinterpret_cast<NativeHLAinteger64LE*>(handle);
}

jlong nativeGetHLAinteger64LE(JNIEnv* environment, jclass, jlong handle) {
  auto* state = integer64LEStateFor(environment, handle);
  if (state == nullptr) return 0;
  try {
    return static_cast<jlong>(state->element.get());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  }
  return 0;
}

void nativeSetHLAinteger64LE(JNIEnv* environment, jclass, jlong handle, jlong value) {
  auto* state = integer64LEStateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->element.set(static_cast<std::int64_t>(value));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  }
}

jint nativeHLAinteger64LEOctetBoundary(JNIEnv* environment, jclass, jlong handle) {
  auto* state = integer64LEStateFor(environment, handle);
  if (state == nullptr) return 0;
  try {
    return static_cast<jint>(state->element.getOctetBoundary());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  }
  return 0;
}

jint nativeHLAinteger64LEEncodedLength(JNIEnv* environment, jclass, jlong handle) {
  auto* state = integer64LEStateFor(environment, handle);
  if (state == nullptr) return 0;
  try {
    return static_cast<jint>(state->element.getEncodedLength());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  }
  return 0;
}

jbyteArray nativeHLAinteger64LEToByteArray(JNIEnv* environment, jclass, jlong handle) {
  auto* state = integer64LEStateFor(environment, handle);
  if (state == nullptr) return nullptr;
  try {
    return javaByteArray(environment, state->element.encode());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

void nativeDecodeHLAinteger64LE(JNIEnv* environment, jclass, jlong handle, jbyteArray bytes) {
  auto* state = integer64LEStateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->element.decode(variableLengthData(environment, bytes));
  } catch (rti::Exception const& exception) {
    throwJavaDecoderException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

jlong nativeCreateHLAinteger16BE(JNIEnv* environment, jclass, jshort value) {
  try {
    return reinterpret_cast<jlong>(new NativeHLAinteger16BE(static_cast<std::int16_t>(value)));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return 0;
}

void nativeDestroyHLAinteger16BE(JNIEnv*, jclass, jlong handle) {
  delete reinterpret_cast<NativeHLAinteger16BE*>(handle);
}

jshort nativeGetHLAinteger16BE(JNIEnv* environment, jclass, jlong handle) {
  auto* state = integer16StateFor(environment, handle);
  if (state == nullptr) return 0;
  try {
    return static_cast<jshort>(state->element.get());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  }
  return 0;
}

void nativeSetHLAinteger16BE(JNIEnv* environment, jclass, jlong handle, jshort value) {
  auto* state = integer16StateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->element.set(static_cast<std::int16_t>(value));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  }
}

jint nativeHLAinteger16BEOctetBoundary(JNIEnv* environment, jclass, jlong handle) {
  auto* state = integer16StateFor(environment, handle);
  if (state == nullptr) return 0;
  try {
    return static_cast<jint>(state->element.getOctetBoundary());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  }
  return 0;
}

jint nativeHLAinteger16BEEncodedLength(JNIEnv* environment, jclass, jlong handle) {
  auto* state = integer16StateFor(environment, handle);
  if (state == nullptr) return 0;
  try {
    return static_cast<jint>(state->element.getEncodedLength());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  }
  return 0;
}

jbyteArray nativeHLAinteger16BEToByteArray(JNIEnv* environment, jclass, jlong handle) {
  auto* state = integer16StateFor(environment, handle);
  if (state == nullptr) return nullptr;
  try {
    return javaByteArray(environment, state->element.encode());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

void nativeDecodeHLAinteger16BE(JNIEnv* environment, jclass, jlong handle, jbyteArray bytes) {
  auto* state = integer16StateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->element.decode(variableLengthData(environment, bytes));
  } catch (rti::Exception const& exception) {
    throwJavaDecoderException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

jlong nativeCreateHLAinteger16LE(JNIEnv* environment, jclass, jshort value) {
  try {
    return reinterpret_cast<jlong>(new NativeHLAinteger16LE(static_cast<std::int16_t>(value)));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return 0;
}

void nativeDestroyHLAinteger16LE(JNIEnv*, jclass, jlong handle) {
  delete reinterpret_cast<NativeHLAinteger16LE*>(handle);
}

jshort nativeGetHLAinteger16LE(JNIEnv* environment, jclass, jlong handle) {
  auto* state = integer16LEStateFor(environment, handle);
  if (state == nullptr) return 0;
  try {
    return static_cast<jshort>(state->element.get());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  }
  return 0;
}

void nativeSetHLAinteger16LE(JNIEnv* environment, jclass, jlong handle, jshort value) {
  auto* state = integer16LEStateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->element.set(static_cast<std::int16_t>(value));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  }
}

jint nativeHLAinteger16LEOctetBoundary(JNIEnv* environment, jclass, jlong handle) {
  auto* state = integer16LEStateFor(environment, handle);
  if (state == nullptr) return 0;
  try {
    return static_cast<jint>(state->element.getOctetBoundary());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  }
  return 0;
}

jint nativeHLAinteger16LEEncodedLength(JNIEnv* environment, jclass, jlong handle) {
  auto* state = integer16LEStateFor(environment, handle);
  if (state == nullptr) return 0;
  try {
    return static_cast<jint>(state->element.getEncodedLength());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  }
  return 0;
}

jbyteArray nativeHLAinteger16LEToByteArray(JNIEnv* environment, jclass, jlong handle) {
  auto* state = integer16LEStateFor(environment, handle);
  if (state == nullptr) return nullptr;
  try {
    return javaByteArray(environment, state->element.encode());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

void nativeDecodeHLAinteger16LE(JNIEnv* environment, jclass, jlong handle, jbyteArray bytes) {
  auto* state = integer16LEStateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->element.decode(variableLengthData(environment, bytes));
  } catch (rti::Exception const& exception) {
    throwJavaDecoderException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

jbyteArray nativeDecodeFederateHandle(JNIEnv* environment, jclass, jbyteArray encodedValue) {
  try {
    auto decoded = rti::umbra_binding_detail::decodeFederateHandle(
        variableLengthData(environment, encodedValue));
    return javaByteArray(environment, decoded.encode());
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
  return nullptr;
}

void nativeRegisterFederationSynchronizationPoint(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jstring synchronizationPointLabel,
    jbyteArray userSuppliedTag,
    jobjectArray synchronizationSet) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    std::optional<rti::FederateHandleSet> decodedSet;
    if (synchronizationSet != nullptr) decodedSet = federateHandleSet(environment, synchronizationSet);
    state->registerFederationSynchronizationPoint(
        wideString(environment, synchronizationPointLabel),
        variableLengthData(environment, userSuppliedTag),
        std::move(decodedSet));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeSynchronizationPointAchieved(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jstring synchronizationPointLabel,
    jboolean successfully) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->synchronizationPointAchieved(
        wideString(environment, synchronizationPointLabel), successfully == JNI_TRUE);
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeQueryFederationSaveStatus(JNIEnv* environment, jclass, jlong handle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->queryFederationSaveStatus();
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeRequestFederationSave(JNIEnv* environment, jclass, jlong handle, jstring label) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->requestFederationSave(wideString(environment, label));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeRequestFederationSaveWithTime(
    JNIEnv* environment,
    jclass,
    jlong handle,
    jstring label,
    jbyteArray encodedLogicalTime) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->requestFederationSave(
        wideString(environment, label),
        variableLengthData(environment, encodedLogicalTime));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeFederateSaveBegun(JNIEnv* environment, jclass, jlong handle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->federateSaveBegun();
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeFederateSaveComplete(JNIEnv* environment, jclass, jlong handle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->federateSaveComplete();
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeFederateSaveNotComplete(JNIEnv* environment, jclass, jlong handle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->federateSaveNotComplete();
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeAbortFederationSave(JNIEnv* environment, jclass, jlong handle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->abortFederationSave();
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeQueryFederationRestoreStatus(JNIEnv* environment, jclass, jlong handle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->queryFederationRestoreStatus();
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeRequestFederationRestore(JNIEnv* environment, jclass, jlong handle, jstring label) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->requestFederationRestore(wideString(environment, label));
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeFederateRestoreComplete(JNIEnv* environment, jclass, jlong handle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->federateRestoreComplete();
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeFederateRestoreNotComplete(JNIEnv* environment, jclass, jlong handle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->federateRestoreNotComplete();
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

void nativeAbortFederationRestore(JNIEnv* environment, jclass, jlong handle) {
  auto* state = stateFor(environment, handle);
  if (state == nullptr) return;
  try {
    state->abortFederationRestore();
  } catch (rti::Exception const& exception) {
    throwJavaRtiException(environment, exception);
  } catch (std::exception const& exception) {
    throwJavaException(environment, "hla/rti1516_2025/exceptions/RTIinternalError", exception.what());
  }
}

JNINativeMethod const nativeMethods[] = {
    {const_cast<char*>("nativeCreate"), const_cast<char*>("()J"), reinterpret_cast<void*>(nativeCreate)},
    {const_cast<char*>("nativeDestroy"), const_cast<char*>("(J)V"), reinterpret_cast<void*>(nativeDestroy)},
    {const_cast<char*>("nativeAuthorize"),
     const_cast<char*>(
         "(Ljava/lang/String;Lhla/rti1516_2025/auth/Credentials;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)Lhla/rti1516_2025/auth/AuthorizationResult;"),
     reinterpret_cast<void*>(nativeAuthorize)},
    {const_cast<char*>("nativeConnect"),
     const_cast<char*>(
         "(JLhla/rti1516_2025/FederateAmbassador;Ljava/lang/String;Lhla/rti1516_2025/RtiConfiguration;Lhla/rti1516_2025/auth/Credentials;)Lhla/rti1516_2025/ConfigurationResult;"),
     reinterpret_cast<void*>(nativeConnect)},
    {const_cast<char*>("nativeDisconnect"), const_cast<char*>("(J)V"), reinterpret_cast<void*>(nativeDisconnect)},
    {const_cast<char*>("nativeFailEmbeddedTransportConnectionForTesting"), const_cast<char*>("(JLjava/lang/String;)Z"), reinterpret_cast<void*>(nativeFailEmbeddedTransportConnectionForTesting)},
    {const_cast<char*>("nativeForceEmbeddedFederateResignationForTesting"), const_cast<char*>("(JLjava/lang/String;)Z"), reinterpret_cast<void*>(nativeForceEmbeddedFederateResignationForTesting)},
    {const_cast<char*>("nativeEvokeCallback"), const_cast<char*>("(JD)Z"), reinterpret_cast<void*>(nativeEvokeCallback)},
    {const_cast<char*>("nativeEvokeMultipleCallbacks"), const_cast<char*>("(JDD)Z"), reinterpret_cast<void*>(nativeEvokeMultipleCallbacks)},
    {const_cast<char*>("nativeEnableCallbacks"), const_cast<char*>("(J)V"), reinterpret_cast<void*>(nativeEnableCallbacks)},
    {const_cast<char*>("nativeDisableCallbacks"), const_cast<char*>("(J)V"), reinterpret_cast<void*>(nativeDisableCallbacks)},
    {const_cast<char*>("nativeListFederationExecutions"), const_cast<char*>("(J)V"), reinterpret_cast<void*>(nativeListFederationExecutions)},
    {const_cast<char*>("nativeListFederationExecutionMembers"), const_cast<char*>("(JLjava/lang/String;)V"), reinterpret_cast<void*>(nativeListFederationExecutionMembers)},
    {const_cast<char*>("nativeCreateFederationExecution"), const_cast<char*>("(JLjava/lang/String;[Ljava/lang/String;Ljava/lang/String;)V"), reinterpret_cast<void*>(nativeCreateFederationExecution)},
    {const_cast<char*>("nativeCreateFederationExecutionWithMIM"), const_cast<char*>("(JLjava/lang/String;[Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V"), reinterpret_cast<void*>(nativeCreateFederationExecutionWithMIM)},
    {const_cast<char*>("nativeDestroyFederationExecution"), const_cast<char*>("(JLjava/lang/String;)V"), reinterpret_cast<void*>(nativeDestroyFederationExecution)},
    {const_cast<char*>("nativeJoinFederationExecution"), const_cast<char*>("(JLjava/lang/String;Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;)[B"), reinterpret_cast<void*>(nativeJoinFederationExecution)},
    {const_cast<char*>("nativeResignFederationExecution"), const_cast<char*>("(JLjava/lang/String;)V"), reinterpret_cast<void*>(nativeResignFederationExecution)},
    {const_cast<char*>("nativeGetFederateHandle"), const_cast<char*>("(JLjava/lang/String;)[B"), reinterpret_cast<void*>(nativeGetFederateHandle)},
    {const_cast<char*>("nativeGetFederateName"), const_cast<char*>("(J[B)Ljava/lang/String;"), reinterpret_cast<void*>(nativeGetFederateName)},
    {const_cast<char*>("nativeNormalizeFederateHandle"), const_cast<char*>("(J[B)J"), reinterpret_cast<void*>(nativeNormalizeFederateHandle)},
    {const_cast<char*>("nativeDecodeObjectClassHandle"), const_cast<char*>("([B)[B"), reinterpret_cast<void*>(nativeDecodeObjectClassHandle)},
    {const_cast<char*>("nativeGetObjectClassHandle"), const_cast<char*>("(JLjava/lang/String;)[B"), reinterpret_cast<void*>(nativeGetObjectClassHandle)},
    {const_cast<char*>("nativeGetObjectClassName"), const_cast<char*>("(J[B)Ljava/lang/String;"), reinterpret_cast<void*>(nativeGetObjectClassName)},
    {const_cast<char*>("nativeGetKnownObjectClassHandle"), const_cast<char*>("(J[B)[B"), reinterpret_cast<void*>(nativeGetKnownObjectClassHandle)},
    {const_cast<char*>("nativeNormalizeObjectClassHandle"), const_cast<char*>("(J[B)J"), reinterpret_cast<void*>(nativeNormalizeObjectClassHandle)},
    {const_cast<char*>("nativeDecodeAttributeHandle"), const_cast<char*>("([B)[B"), reinterpret_cast<void*>(nativeDecodeAttributeHandle)},
    {const_cast<char*>("nativeGetAttributeHandle"), const_cast<char*>("(J[BLjava/lang/String;)[B"), reinterpret_cast<void*>(nativeGetAttributeHandle)},
    {const_cast<char*>("nativeGetAttributeName"), const_cast<char*>("(J[B[B)Ljava/lang/String;"), reinterpret_cast<void*>(nativeGetAttributeName)},
    {const_cast<char*>("nativeDecodeDimensionHandle"), const_cast<char*>("([B)[B"), reinterpret_cast<void*>(nativeDecodeDimensionHandle)},
    {const_cast<char*>("nativeGetDimensionHandle"), const_cast<char*>("(JLjava/lang/String;)[B"), reinterpret_cast<void*>(nativeGetDimensionHandle)},
    {const_cast<char*>("nativeGetDimensionName"), const_cast<char*>("(J[B)Ljava/lang/String;"), reinterpret_cast<void*>(nativeGetDimensionName)},
    {const_cast<char*>("nativeGetAvailableDimensionsForObjectClass"), const_cast<char*>("(J[B)[[B"), reinterpret_cast<void*>(nativeGetAvailableDimensionsForObjectClass)},
    {const_cast<char*>("nativeGetAvailableDimensionsForInteractionClass"), const_cast<char*>("(J[B)[[B"), reinterpret_cast<void*>(nativeGetAvailableDimensionsForInteractionClass)},
    {const_cast<char*>("nativeGetDimensionUpperBound"), const_cast<char*>("(J[B)J"), reinterpret_cast<void*>(nativeGetDimensionUpperBound)},
    {const_cast<char*>("nativeDecodeRegionHandle"), const_cast<char*>("([B)[B"), reinterpret_cast<void*>(nativeDecodeRegionHandle)},
    {const_cast<char*>("nativeCreateRegion"), const_cast<char*>("(JLhla/rti1516_2025/DimensionHandleSet;)[B"), reinterpret_cast<void*>(nativeCreateRegion)},
    {const_cast<char*>("nativeCommitRegionModifications"), const_cast<char*>("(JLhla/rti1516_2025/RegionHandleSet;)V"), reinterpret_cast<void*>(nativeCommitRegionModifications)},
    {const_cast<char*>("nativeDeleteRegion"), const_cast<char*>("(J[B)V"), reinterpret_cast<void*>(nativeDeleteRegion)},
    {const_cast<char*>("nativeGetDimensionHandleSet"), const_cast<char*>("(J[B)[[B"), reinterpret_cast<void*>(nativeGetDimensionHandleSet)},
    {const_cast<char*>("nativeGetRangeBounds"), const_cast<char*>("(J[B[B)[J"), reinterpret_cast<void*>(nativeGetRangeBounds)},
    {const_cast<char*>("nativeSetRangeBounds"), const_cast<char*>("(J[B[BJJ)V"), reinterpret_cast<void*>(nativeSetRangeBounds)},
    {const_cast<char*>("nativeDecodeObjectInstanceHandle"), const_cast<char*>("([B)[B"), reinterpret_cast<void*>(nativeDecodeObjectInstanceHandle)},
    {const_cast<char*>("nativePublishObjectClassAttributes"), const_cast<char*>("(J[BLhla/rti1516_2025/AttributeHandleSet;)V"), reinterpret_cast<void*>(nativePublishObjectClassAttributes)},
    {const_cast<char*>("nativeUnpublishObjectClass"), const_cast<char*>("(J[B)V"), reinterpret_cast<void*>(nativeUnpublishObjectClass)},
    {const_cast<char*>("nativeUnpublishObjectClassAttributes"), const_cast<char*>("(J[BLhla/rti1516_2025/AttributeHandleSet;)V"), reinterpret_cast<void*>(nativeUnpublishObjectClassAttributes)},
    {const_cast<char*>("nativePublishObjectClassDirectedInteractions"), const_cast<char*>("(J[BLjava/util/Set;)V"), reinterpret_cast<void*>(nativePublishObjectClassDirectedInteractions)},
    {const_cast<char*>("nativeUnpublishObjectClassDirectedInteractions"), const_cast<char*>("(J[B)V"), reinterpret_cast<void*>(nativeUnpublishObjectClassDirectedInteractions)},
    {const_cast<char*>("nativeUnpublishObjectClassDirectedInteractionsWithClasses"), const_cast<char*>("(J[BLjava/util/Set;)V"), reinterpret_cast<void*>(nativeUnpublishObjectClassDirectedInteractionsWithClasses)},
    {const_cast<char*>("nativeSubscribeObjectClassAttributes"), const_cast<char*>("(J[BLhla/rti1516_2025/AttributeHandleSet;ZLjava/lang/String;)V"), reinterpret_cast<void*>(nativeSubscribeObjectClassAttributes)},
    {const_cast<char*>("nativeUnsubscribeObjectClass"), const_cast<char*>("(J[B)V"), reinterpret_cast<void*>(nativeUnsubscribeObjectClass)},
    {const_cast<char*>("nativeUnsubscribeObjectClassAttributes"), const_cast<char*>("(J[BLhla/rti1516_2025/AttributeHandleSet;)V"), reinterpret_cast<void*>(nativeUnsubscribeObjectClassAttributes)},
    {const_cast<char*>("nativeSubscribeObjectClassDirectedInteractions"), const_cast<char*>("(J[BLjava/util/Set;Z)V"), reinterpret_cast<void*>(nativeSubscribeObjectClassDirectedInteractions)},
    {const_cast<char*>("nativeUnsubscribeObjectClassDirectedInteractions"), const_cast<char*>("(J[B)V"), reinterpret_cast<void*>(nativeUnsubscribeObjectClassDirectedInteractions)},
    {const_cast<char*>("nativeUnsubscribeObjectClassDirectedInteractionsWithClasses"), const_cast<char*>("(J[BLjava/util/Set;)V"), reinterpret_cast<void*>(nativeUnsubscribeObjectClassDirectedInteractionsWithClasses)},
    {const_cast<char*>("nativeSubscribeObjectClassAttributesWithRegions"), const_cast<char*>("(J[BLhla/rti1516_2025/AttributeSetRegionSetPairList;ZLjava/lang/String;)V"), reinterpret_cast<void*>(nativeSubscribeObjectClassAttributesWithRegions)},
    {const_cast<char*>("nativeUnsubscribeObjectClassAttributesWithRegions"), const_cast<char*>("(J[BLhla/rti1516_2025/AttributeSetRegionSetPairList;)V"), reinterpret_cast<void*>(nativeUnsubscribeObjectClassAttributesWithRegions)},
    {const_cast<char*>("nativeReserveObjectInstanceName"), const_cast<char*>("(JLjava/lang/String;)V"), reinterpret_cast<void*>(nativeReserveObjectInstanceName)},
    {const_cast<char*>("nativeReleaseObjectInstanceName"), const_cast<char*>("(JLjava/lang/String;)V"), reinterpret_cast<void*>(nativeReleaseObjectInstanceName)},
    {const_cast<char*>("nativeReserveMultipleObjectInstanceNames"), const_cast<char*>("(JLjava/util/Set;)V"), reinterpret_cast<void*>(nativeReserveMultipleObjectInstanceNames)},
    {const_cast<char*>("nativeReleaseMultipleObjectInstanceNames"), const_cast<char*>("(JLjava/util/Set;)V"), reinterpret_cast<void*>(nativeReleaseMultipleObjectInstanceNames)},
    {const_cast<char*>("nativeRegisterObjectInstance"), const_cast<char*>("(J[BLjava/lang/String;)[B"), reinterpret_cast<void*>(nativeRegisterObjectInstance)},
    {const_cast<char*>("nativeRegisterObjectInstanceWithRegions"), const_cast<char*>("(J[BLhla/rti1516_2025/AttributeSetRegionSetPairList;Ljava/lang/String;)[B"), reinterpret_cast<void*>(nativeRegisterObjectInstanceWithRegions)},
    {const_cast<char*>("nativeAssociateRegionsForUpdates"), const_cast<char*>("(J[BLhla/rti1516_2025/AttributeSetRegionSetPairList;)V"), reinterpret_cast<void*>(nativeAssociateRegionsForUpdates)},
    {const_cast<char*>("nativeUnassociateRegionsForUpdates"), const_cast<char*>("(J[BLhla/rti1516_2025/AttributeSetRegionSetPairList;)V"), reinterpret_cast<void*>(nativeUnassociateRegionsForUpdates)},
    {const_cast<char*>("nativeGetObjectInstanceHandle"), const_cast<char*>("(JLjava/lang/String;)[B"), reinterpret_cast<void*>(nativeGetObjectInstanceHandle)},
    {const_cast<char*>("nativeGetObjectInstanceName"), const_cast<char*>("(J[B)Ljava/lang/String;"), reinterpret_cast<void*>(nativeGetObjectInstanceName)},
    {const_cast<char*>("nativeNormalizeObjectInstanceHandle"), const_cast<char*>("(J[B)J"), reinterpret_cast<void*>(nativeNormalizeObjectInstanceHandle)},
    {const_cast<char*>("nativeUpdateAttributeValues"), const_cast<char*>("(J[BLhla/rti1516_2025/AttributeHandleValueMap;[B)V"), reinterpret_cast<void*>(nativeUpdateAttributeValues)},
    {const_cast<char*>("nativeUpdateAttributeValuesWithTime"), const_cast<char*>("(J[BLhla/rti1516_2025/AttributeHandleValueMap;[B[B)[B"), reinterpret_cast<void*>(nativeUpdateAttributeValuesWithTime)},
    {const_cast<char*>("nativeDeleteObjectInstance"), const_cast<char*>("(J[B[B)V"), reinterpret_cast<void*>(nativeDeleteObjectInstance)},
    {const_cast<char*>("nativeDeleteObjectInstanceWithTime"), const_cast<char*>("(J[B[B[B)[B"), reinterpret_cast<void*>(nativeDeleteObjectInstanceWithTime)},
    {const_cast<char*>("nativeLocalDeleteObjectInstance"), const_cast<char*>("(J[B)V"), reinterpret_cast<void*>(nativeLocalDeleteObjectInstance)},
    {const_cast<char*>("nativeRequestAttributeValueUpdateForObjectInstance"), const_cast<char*>("(J[BLhla/rti1516_2025/AttributeHandleSet;[B)V"), reinterpret_cast<void*>(nativeRequestAttributeValueUpdateForObjectInstance)},
    {const_cast<char*>("nativeRequestAttributeValueUpdateForObjectClass"), const_cast<char*>("(J[BLhla/rti1516_2025/AttributeHandleSet;[B)V"), reinterpret_cast<void*>(nativeRequestAttributeValueUpdateForObjectClass)},
    {const_cast<char*>("nativeRequestAttributeValueUpdateWithRegions"), const_cast<char*>("(J[BLhla/rti1516_2025/AttributeSetRegionSetPairList;[B)V"), reinterpret_cast<void*>(nativeRequestAttributeValueUpdateWithRegions)},
    {const_cast<char*>("nativeQueryAttributeOwnership"), const_cast<char*>("(J[BLhla/rti1516_2025/AttributeHandleSet;)V"), reinterpret_cast<void*>(nativeQueryAttributeOwnership)},
    {const_cast<char*>("nativeIsAttributeOwnedByFederate"), const_cast<char*>("(J[B[B)Z"), reinterpret_cast<void*>(nativeIsAttributeOwnedByFederate)},
    {const_cast<char*>("nativeUnconditionalAttributeOwnershipDivestiture"), const_cast<char*>("(J[BLhla/rti1516_2025/AttributeHandleSet;[B)V"), reinterpret_cast<void*>(nativeUnconditionalAttributeOwnershipDivestiture)},
    {const_cast<char*>("nativeNegotiatedAttributeOwnershipDivestiture"), const_cast<char*>("(J[BLhla/rti1516_2025/AttributeHandleSet;[B)V"), reinterpret_cast<void*>(nativeNegotiatedAttributeOwnershipDivestiture)},
    {const_cast<char*>("nativeConfirmDivestiture"), const_cast<char*>("(J[BLhla/rti1516_2025/AttributeHandleSet;[B)V"), reinterpret_cast<void*>(nativeConfirmDivestiture)},
    {const_cast<char*>("nativeCancelNegotiatedAttributeOwnershipDivestiture"), const_cast<char*>("(J[BLhla/rti1516_2025/AttributeHandleSet;)V"), reinterpret_cast<void*>(nativeCancelNegotiatedAttributeOwnershipDivestiture)},
    {const_cast<char*>("nativeAttributeOwnershipAcquisition"), const_cast<char*>("(J[BLhla/rti1516_2025/AttributeHandleSet;[B)V"), reinterpret_cast<void*>(nativeAttributeOwnershipAcquisition)},
    {const_cast<char*>("nativeAttributeOwnershipAcquisitionIfAvailable"), const_cast<char*>("(J[BLhla/rti1516_2025/AttributeHandleSet;[B)V"), reinterpret_cast<void*>(nativeAttributeOwnershipAcquisitionIfAvailable)},
    {const_cast<char*>("nativeCancelAttributeOwnershipAcquisition"), const_cast<char*>("(J[BLhla/rti1516_2025/AttributeHandleSet;)V"), reinterpret_cast<void*>(nativeCancelAttributeOwnershipAcquisition)},
    {const_cast<char*>("nativeAttributeOwnershipReleaseDenied"), const_cast<char*>("(J[BLhla/rti1516_2025/AttributeHandleSet;[B)V"), reinterpret_cast<void*>(nativeAttributeOwnershipReleaseDenied)},
    {const_cast<char*>("nativeAttributeOwnershipDivestitureIfWanted"), const_cast<char*>("(J[BLhla/rti1516_2025/AttributeHandleSet;[B)[[B"), reinterpret_cast<void*>(nativeAttributeOwnershipDivestitureIfWanted)},
    {const_cast<char*>("nativeQueryAttributeTransportationType"), const_cast<char*>("(J[B[B)V"), reinterpret_cast<void*>(nativeQueryAttributeTransportationType)},
    {const_cast<char*>("nativeQueryInteractionTransportationType"), const_cast<char*>("(J[B[B)V"), reinterpret_cast<void*>(nativeQueryInteractionTransportationType)},
    {const_cast<char*>("nativeRequestAttributeTransportationTypeChange"), const_cast<char*>("(J[BLhla/rti1516_2025/AttributeHandleSet;[B)V"), reinterpret_cast<void*>(nativeRequestAttributeTransportationTypeChange)},
    {const_cast<char*>("nativeRequestInteractionTransportationTypeChange"), const_cast<char*>("(J[B[B)V"), reinterpret_cast<void*>(nativeRequestInteractionTransportationTypeChange)},
    {const_cast<char*>("nativeSetAttributeScopeAdvisorySwitch"), const_cast<char*>("(JZ)V"), reinterpret_cast<void*>(nativeSetAttributeScopeAdvisorySwitch)},
    {const_cast<char*>("nativeGetObjectClassRelevanceAdvisorySwitch"), const_cast<char*>("(J)Z"), reinterpret_cast<void*>(nativeGetObjectClassRelevanceAdvisorySwitch)},
    {const_cast<char*>("nativeSetObjectClassRelevanceAdvisorySwitch"), const_cast<char*>("(JZ)V"), reinterpret_cast<void*>(nativeSetObjectClassRelevanceAdvisorySwitch)},
    {const_cast<char*>("nativeGetAttributeRelevanceAdvisorySwitch"), const_cast<char*>("(J)Z"), reinterpret_cast<void*>(nativeGetAttributeRelevanceAdvisorySwitch)},
    {const_cast<char*>("nativeSetAttributeRelevanceAdvisorySwitch"), const_cast<char*>("(JZ)V"), reinterpret_cast<void*>(nativeSetAttributeRelevanceAdvisorySwitch)},
    {const_cast<char*>("nativeGetAttributeScopeAdvisorySwitch"), const_cast<char*>("(J)Z"), reinterpret_cast<void*>(nativeGetAttributeScopeAdvisorySwitch)},
    {const_cast<char*>("nativeGetInteractionRelevanceAdvisorySwitch"), const_cast<char*>("(J)Z"), reinterpret_cast<void*>(nativeGetInteractionRelevanceAdvisorySwitch)},
    {const_cast<char*>("nativeSetInteractionRelevanceAdvisorySwitch"), const_cast<char*>("(JZ)V"), reinterpret_cast<void*>(nativeSetInteractionRelevanceAdvisorySwitch)},
    {const_cast<char*>("nativeGetAutomaticResignDirective"), const_cast<char*>("(J)Ljava/lang/String;"), reinterpret_cast<void*>(nativeGetAutomaticResignDirective)},
    {const_cast<char*>("nativeSetAutomaticResignDirective"), const_cast<char*>("(JLjava/lang/String;)V"), reinterpret_cast<void*>(nativeSetAutomaticResignDirective)},
    {const_cast<char*>("nativeGetServiceReportingSwitch"), const_cast<char*>("(J)Z"), reinterpret_cast<void*>(nativeGetServiceReportingSwitch)},
    {const_cast<char*>("nativeSetServiceReportingSwitch"), const_cast<char*>("(JZ)V"), reinterpret_cast<void*>(nativeSetServiceReportingSwitch)},
    {const_cast<char*>("nativeGetExceptionReportingSwitch"), const_cast<char*>("(J)Z"), reinterpret_cast<void*>(nativeGetExceptionReportingSwitch)},
    {const_cast<char*>("nativeSetExceptionReportingSwitch"), const_cast<char*>("(JZ)V"), reinterpret_cast<void*>(nativeSetExceptionReportingSwitch)},
    {const_cast<char*>("nativeGetSendServiceReportsToFileSwitch"), const_cast<char*>("(J)Z"), reinterpret_cast<void*>(nativeGetSendServiceReportsToFileSwitch)},
    {const_cast<char*>("nativeSetSendServiceReportsToFileSwitch"), const_cast<char*>("(JZ)V"), reinterpret_cast<void*>(nativeSetSendServiceReportsToFileSwitch)},
    {const_cast<char*>("nativeGetHLAversion"), const_cast<char*>("(J)Ljava/lang/String;"), reinterpret_cast<void*>(nativeGetHLAversion)},
    {const_cast<char*>("nativeGetAutoProvideSwitch"), const_cast<char*>("(J)Z"), reinterpret_cast<void*>(nativeGetAutoProvideSwitch)},
    {const_cast<char*>("nativeGetDelaySubscriptionEvaluationSwitch"), const_cast<char*>("(J)Z"), reinterpret_cast<void*>(nativeGetDelaySubscriptionEvaluationSwitch)},
    {const_cast<char*>("nativeGetAdvisoriesUseKnownClassSwitch"), const_cast<char*>("(J)Z"), reinterpret_cast<void*>(nativeGetAdvisoriesUseKnownClassSwitch)},
    {const_cast<char*>("nativeGetAllowRelaxedDDMSwitch"), const_cast<char*>("(J)Z"), reinterpret_cast<void*>(nativeGetAllowRelaxedDDMSwitch)},
    {const_cast<char*>("nativeGetNonRegulatedGrantSwitch"), const_cast<char*>("(J)Z"), reinterpret_cast<void*>(nativeGetNonRegulatedGrantSwitch)},
    {const_cast<char*>("nativeNormalizeServiceGroup"), const_cast<char*>("(JLjava/lang/String;)J"), reinterpret_cast<void*>(nativeNormalizeServiceGroup)},
    {const_cast<char*>("nativeGetConveyRegionDesignatorSetsSwitch"), const_cast<char*>("(J)Z"), reinterpret_cast<void*>(nativeGetConveyRegionDesignatorSetsSwitch)},
    {const_cast<char*>("nativeSetConveyRegionDesignatorSetsSwitch"), const_cast<char*>("(JZ)V"), reinterpret_cast<void*>(nativeSetConveyRegionDesignatorSetsSwitch)},
    {const_cast<char*>("nativeGetOrderType"), const_cast<char*>("(JLjava/lang/String;)Ljava/lang/String;"), reinterpret_cast<void*>(nativeGetOrderType)},
    {const_cast<char*>("nativeGetOrderName"), const_cast<char*>("(JLjava/lang/String;)Ljava/lang/String;"), reinterpret_cast<void*>(nativeGetOrderName)},
    {const_cast<char*>("nativeChangeAttributeOrderType"), const_cast<char*>("(J[BLhla/rti1516_2025/AttributeHandleSet;Ljava/lang/String;)V"), reinterpret_cast<void*>(nativeChangeAttributeOrderType)},
    {const_cast<char*>("nativeChangeDefaultAttributeOrderType"), const_cast<char*>("(J[BLhla/rti1516_2025/AttributeHandleSet;Ljava/lang/String;)V"), reinterpret_cast<void*>(nativeChangeDefaultAttributeOrderType)},
    {const_cast<char*>("nativeChangeInteractionOrderType"), const_cast<char*>("(J[BLjava/lang/String;)V"), reinterpret_cast<void*>(nativeChangeInteractionOrderType)},
    {const_cast<char*>("nativeGetUpdateRateValue"), const_cast<char*>("(JLjava/lang/String;)D"), reinterpret_cast<void*>(nativeGetUpdateRateValue)},
    {const_cast<char*>("nativeGetUpdateRateValueForAttribute"), const_cast<char*>("(J[B[B)D"), reinterpret_cast<void*>(nativeGetUpdateRateValueForAttribute)},
    {const_cast<char*>("nativeChangeDefaultAttributeTransportationType"), const_cast<char*>("(J[BLhla/rti1516_2025/AttributeHandleSet;[B)V"), reinterpret_cast<void*>(nativeChangeDefaultAttributeTransportationType)},
    {const_cast<char*>("nativeGetTimeFactoryName"), const_cast<char*>("(J)Ljava/lang/String;"), reinterpret_cast<void*>(nativeGetTimeFactoryName)},
    {const_cast<char*>("nativeValidateLogicalTimeImplementation"), const_cast<char*>("(JLjava/lang/String;Z)V"), reinterpret_cast<void*>(nativeValidateLogicalTimeImplementation)},
    {const_cast<char*>("nativeMakeIntegerLogicalTime"), const_cast<char*>("(JJ)[B"), reinterpret_cast<void*>(nativeMakeIntegerLogicalTime)},
    {const_cast<char*>("nativeMakeIntegerLogicalTimeInterval"), const_cast<char*>("(JJ)[B"), reinterpret_cast<void*>(nativeMakeIntegerLogicalTimeInterval)},
    {const_cast<char*>("nativeMakeFloatLogicalTime"), const_cast<char*>("(JD)[B"), reinterpret_cast<void*>(nativeMakeFloatLogicalTime)},
    {const_cast<char*>("nativeMakeFloatLogicalTimeInterval"), const_cast<char*>("(JD)[B"), reinterpret_cast<void*>(nativeMakeFloatLogicalTimeInterval)},
    {const_cast<char*>("nativeDecodeLogicalTime"), const_cast<char*>("(J[B)[B"), reinterpret_cast<void*>(nativeDecodeLogicalTime)},
    {const_cast<char*>("nativeDecodeLogicalTimeInterval"), const_cast<char*>("(J[B)[B"), reinterpret_cast<void*>(nativeDecodeLogicalTimeInterval)},
    {const_cast<char*>("nativeAddLogicalTime"), const_cast<char*>("(J[B[B)[B"), reinterpret_cast<void*>(nativeAddLogicalTime)},
    {const_cast<char*>("nativeSubtractLogicalTime"), const_cast<char*>("(J[B[B)[B"), reinterpret_cast<void*>(nativeSubtractLogicalTime)},
    {const_cast<char*>("nativeAddLogicalTimeInterval"), const_cast<char*>("(J[B[B)[B"), reinterpret_cast<void*>(nativeAddLogicalTimeInterval)},
    {const_cast<char*>("nativeSubtractLogicalTimeInterval"), const_cast<char*>("(J[B[B)[B"), reinterpret_cast<void*>(nativeSubtractLogicalTimeInterval)},
    {const_cast<char*>("nativeDifferenceLogicalTime"), const_cast<char*>("(J[B[B)[B"), reinterpret_cast<void*>(nativeDifferenceLogicalTime)},
    {const_cast<char*>("nativeEnableTimeRegulation"), const_cast<char*>("(J[B)V"), reinterpret_cast<void*>(nativeEnableTimeRegulation)},
    {const_cast<char*>("nativeDisableTimeRegulation"), const_cast<char*>("(J)V"), reinterpret_cast<void*>(nativeDisableTimeRegulation)},
    {const_cast<char*>("nativeEnableTimeConstrained"), const_cast<char*>("(J)V"), reinterpret_cast<void*>(nativeEnableTimeConstrained)},
    {const_cast<char*>("nativeDisableTimeConstrained"), const_cast<char*>("(J)V"), reinterpret_cast<void*>(nativeDisableTimeConstrained)},
    {const_cast<char*>("nativeEnableAsynchronousDelivery"), const_cast<char*>("(J)V"), reinterpret_cast<void*>(nativeEnableAsynchronousDelivery)},
    {const_cast<char*>("nativeDisableAsynchronousDelivery"), const_cast<char*>("(J)V"), reinterpret_cast<void*>(nativeDisableAsynchronousDelivery)},
    {const_cast<char*>("nativeModifyLookahead"), const_cast<char*>("(J[B)V"), reinterpret_cast<void*>(nativeModifyLookahead)},
    {const_cast<char*>("nativeQueryLookahead"), const_cast<char*>("(J)[B"), reinterpret_cast<void*>(nativeQueryLookahead)},
    {const_cast<char*>("nativeTimeAdvanceRequest"), const_cast<char*>("(J[B)V"), reinterpret_cast<void*>(nativeTimeAdvanceRequest)},
    {const_cast<char*>("nativeTimeAdvanceRequestAvailable"), const_cast<char*>("(J[B)V"), reinterpret_cast<void*>(nativeTimeAdvanceRequestAvailable)},
    {const_cast<char*>("nativeNextMessageRequest"), const_cast<char*>("(J[B)V"), reinterpret_cast<void*>(nativeNextMessageRequest)},
    {const_cast<char*>("nativeNextMessageRequestAvailable"), const_cast<char*>("(J[B)V"), reinterpret_cast<void*>(nativeNextMessageRequestAvailable)},
    {const_cast<char*>("nativeFlushQueueRequest"), const_cast<char*>("(J[B)V"), reinterpret_cast<void*>(nativeFlushQueueRequest)},
    {const_cast<char*>("nativeQueryLogicalTime"), const_cast<char*>("(J)[B"), reinterpret_cast<void*>(nativeQueryLogicalTime)},
    {const_cast<char*>("nativeQueryGALT"), const_cast<char*>("(J)[B"), reinterpret_cast<void*>(nativeQueryGALT)},
    {const_cast<char*>("nativeQueryLITS"), const_cast<char*>("(J)[B"), reinterpret_cast<void*>(nativeQueryLITS)},
    {const_cast<char*>("nativeSendInteractionWithTime"), const_cast<char*>("(J[BLhla/rti1516_2025/ParameterHandleValueMap;[B[B)[B"), reinterpret_cast<void*>(nativeSendInteractionWithTime)},
    {const_cast<char*>("nativeRetract"), const_cast<char*>("(J[B)V"), reinterpret_cast<void*>(nativeRetract)},
    {const_cast<char*>("nativeDecodeMessageRetractionHandle"), const_cast<char*>("(J[B)[B"), reinterpret_cast<void*>(nativeDecodeMessageRetractionHandle)},
    {const_cast<char*>("nativeDecodeInteractionClassHandle"), const_cast<char*>("([B)[B"), reinterpret_cast<void*>(nativeDecodeInteractionClassHandle)},
    {const_cast<char*>("nativeGetInteractionClassHandle"), const_cast<char*>("(JLjava/lang/String;)[B"), reinterpret_cast<void*>(nativeGetInteractionClassHandle)},
    {const_cast<char*>("nativeGetInteractionClassName"), const_cast<char*>("(J[B)Ljava/lang/String;"), reinterpret_cast<void*>(nativeGetInteractionClassName)},
    {const_cast<char*>("nativeNormalizeInteractionClassHandle"), const_cast<char*>("(J[B)J"), reinterpret_cast<void*>(nativeNormalizeInteractionClassHandle)},
    {const_cast<char*>("nativeDecodeParameterHandle"), const_cast<char*>("([B)[B"), reinterpret_cast<void*>(nativeDecodeParameterHandle)},
    {const_cast<char*>("nativeGetParameterHandle"), const_cast<char*>("(J[BLjava/lang/String;)[B"), reinterpret_cast<void*>(nativeGetParameterHandle)},
    {const_cast<char*>("nativeGetParameterName"), const_cast<char*>("(J[B[B)Ljava/lang/String;"), reinterpret_cast<void*>(nativeGetParameterName)},
    {const_cast<char*>("nativePublishInteractionClass"), const_cast<char*>("(J[B)V"), reinterpret_cast<void*>(nativePublishInteractionClass)},
    {const_cast<char*>("nativeUnpublishInteractionClass"), const_cast<char*>("(J[B)V"), reinterpret_cast<void*>(nativeUnpublishInteractionClass)},
    {const_cast<char*>("nativeSubscribeInteractionClass"), const_cast<char*>("(J[BZ)V"), reinterpret_cast<void*>(nativeSubscribeInteractionClass)},
    {const_cast<char*>("nativeUnsubscribeInteractionClass"), const_cast<char*>("(J[B)V"), reinterpret_cast<void*>(nativeUnsubscribeInteractionClass)},
    {const_cast<char*>("nativeSubscribeInteractionClassWithRegions"), const_cast<char*>("(J[BLhla/rti1516_2025/RegionHandleSet;Z)V"), reinterpret_cast<void*>(nativeSubscribeInteractionClassWithRegions)},
    {const_cast<char*>("nativeUnsubscribeInteractionClassWithRegions"), const_cast<char*>("(J[BLhla/rti1516_2025/RegionHandleSet;)V"), reinterpret_cast<void*>(nativeUnsubscribeInteractionClassWithRegions)},
    {const_cast<char*>("nativeDecodeTransportationTypeHandle"), const_cast<char*>("([B)[B"), reinterpret_cast<void*>(nativeDecodeTransportationTypeHandle)},
    {const_cast<char*>("nativeGetTransportationTypeHandle"), const_cast<char*>("(JLjava/lang/String;)[B"), reinterpret_cast<void*>(nativeGetTransportationTypeHandle)},
    {const_cast<char*>("nativeGetTransportationTypeName"), const_cast<char*>("(J[B)Ljava/lang/String;"), reinterpret_cast<void*>(nativeGetTransportationTypeName)},
    {const_cast<char*>("nativeSendInteraction"), const_cast<char*>("(J[BLhla/rti1516_2025/ParameterHandleValueMap;[B)V"), reinterpret_cast<void*>(nativeSendInteraction)},
    {const_cast<char*>("nativeSendDirectedInteraction"), const_cast<char*>("(J[B[BLhla/rti1516_2025/ParameterHandleValueMap;[B)V"), reinterpret_cast<void*>(nativeSendDirectedInteraction)},
    {const_cast<char*>("nativeSendDirectedInteractionWithTime"), const_cast<char*>("(J[B[BLhla/rti1516_2025/ParameterHandleValueMap;[B[B)[B"), reinterpret_cast<void*>(nativeSendDirectedInteractionWithTime)},
    {const_cast<char*>("nativeSendInteractionWithRegions"), const_cast<char*>("(J[BLhla/rti1516_2025/ParameterHandleValueMap;Lhla/rti1516_2025/RegionHandleSet;[B)V"), reinterpret_cast<void*>(nativeSendInteractionWithRegions)},
    {const_cast<char*>("nativeSendInteractionWithRegionsWithTime"), const_cast<char*>("(J[BLhla/rti1516_2025/ParameterHandleValueMap;Lhla/rti1516_2025/RegionHandleSet;[B[B)[B"), reinterpret_cast<void*>(nativeSendInteractionWithRegionsWithTime)},
    {const_cast<char*>("nativeCreateHLAinteger32BE"), const_cast<char*>("(I)J"), reinterpret_cast<void*>(nativeCreateHLAinteger32BE)},
    {const_cast<char*>("nativeDestroyHLAinteger32BE"), const_cast<char*>("(J)V"), reinterpret_cast<void*>(nativeDestroyHLAinteger32BE)},
    {const_cast<char*>("nativeGetHLAinteger32BE"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeGetHLAinteger32BE)},
    {const_cast<char*>("nativeSetHLAinteger32BE"), const_cast<char*>("(JI)V"), reinterpret_cast<void*>(nativeSetHLAinteger32BE)},
    {const_cast<char*>("nativeHLAinteger32BEOctetBoundary"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAinteger32BEOctetBoundary)},
    {const_cast<char*>("nativeHLAinteger32BEEncodedLength"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAinteger32BEEncodedLength)},
    {const_cast<char*>("nativeHLAinteger32BEToByteArray"), const_cast<char*>("(J)[B"), reinterpret_cast<void*>(nativeHLAinteger32BEToByteArray)},
    {const_cast<char*>("nativeDecodeHLAinteger32BE"), const_cast<char*>("(J[B)V"), reinterpret_cast<void*>(nativeDecodeHLAinteger32BE)},
    {const_cast<char*>("nativeCreateHLAinteger32LE"), const_cast<char*>("(I)J"), reinterpret_cast<void*>(nativeCreateHLAinteger32LE)},
    {const_cast<char*>("nativeDestroyHLAinteger32LE"), const_cast<char*>("(J)V"), reinterpret_cast<void*>(nativeDestroyHLAinteger32LE)},
    {const_cast<char*>("nativeGetHLAinteger32LE"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeGetHLAinteger32LE)},
    {const_cast<char*>("nativeSetHLAinteger32LE"), const_cast<char*>("(JI)V"), reinterpret_cast<void*>(nativeSetHLAinteger32LE)},
    {const_cast<char*>("nativeHLAinteger32LEOctetBoundary"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAinteger32LEOctetBoundary)},
    {const_cast<char*>("nativeHLAinteger32LEEncodedLength"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAinteger32LEEncodedLength)},
    {const_cast<char*>("nativeHLAinteger32LEToByteArray"), const_cast<char*>("(J)[B"), reinterpret_cast<void*>(nativeHLAinteger32LEToByteArray)},
    {const_cast<char*>("nativeDecodeHLAinteger32LE"), const_cast<char*>("(J[B)V"), reinterpret_cast<void*>(nativeDecodeHLAinteger32LE)},
    {const_cast<char*>("nativeCreateHLAinteger64BE"), const_cast<char*>("(J)J"), reinterpret_cast<void*>(nativeCreateHLAinteger64BE)},
    {const_cast<char*>("nativeDestroyHLAinteger64BE"), const_cast<char*>("(J)V"), reinterpret_cast<void*>(nativeDestroyHLAinteger64BE)},
    {const_cast<char*>("nativeGetHLAinteger64BE"), const_cast<char*>("(J)J"), reinterpret_cast<void*>(nativeGetHLAinteger64BE)},
    {const_cast<char*>("nativeSetHLAinteger64BE"), const_cast<char*>("(JJ)V"), reinterpret_cast<void*>(nativeSetHLAinteger64BE)},
    {const_cast<char*>("nativeHLAinteger64BEOctetBoundary"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAinteger64BEOctetBoundary)},
    {const_cast<char*>("nativeHLAinteger64BEEncodedLength"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAinteger64BEEncodedLength)},
    {const_cast<char*>("nativeHLAinteger64BEToByteArray"), const_cast<char*>("(J)[B"), reinterpret_cast<void*>(nativeHLAinteger64BEToByteArray)},
    {const_cast<char*>("nativeDecodeHLAinteger64BE"), const_cast<char*>("(J[B)V"), reinterpret_cast<void*>(nativeDecodeHLAinteger64BE)},
    {const_cast<char*>("nativeCreateHLAinteger64LE"), const_cast<char*>("(J)J"), reinterpret_cast<void*>(nativeCreateHLAinteger64LE)},
    {const_cast<char*>("nativeDestroyHLAinteger64LE"), const_cast<char*>("(J)V"), reinterpret_cast<void*>(nativeDestroyHLAinteger64LE)},
    {const_cast<char*>("nativeGetHLAinteger64LE"), const_cast<char*>("(J)J"), reinterpret_cast<void*>(nativeGetHLAinteger64LE)},
    {const_cast<char*>("nativeSetHLAinteger64LE"), const_cast<char*>("(JJ)V"), reinterpret_cast<void*>(nativeSetHLAinteger64LE)},
    {const_cast<char*>("nativeHLAinteger64LEOctetBoundary"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAinteger64LEOctetBoundary)},
    {const_cast<char*>("nativeHLAinteger64LEEncodedLength"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAinteger64LEEncodedLength)},
    {const_cast<char*>("nativeHLAinteger64LEToByteArray"), const_cast<char*>("(J)[B"), reinterpret_cast<void*>(nativeHLAinteger64LEToByteArray)},
    {const_cast<char*>("nativeDecodeHLAinteger64LE"), const_cast<char*>("(J[B)V"), reinterpret_cast<void*>(nativeDecodeHLAinteger64LE)},
    {const_cast<char*>("nativeCreateHLAinteger16BE"), const_cast<char*>("(S)J"), reinterpret_cast<void*>(nativeCreateHLAinteger16BE)},
    {const_cast<char*>("nativeDestroyHLAinteger16BE"), const_cast<char*>("(J)V"), reinterpret_cast<void*>(nativeDestroyHLAinteger16BE)},
    {const_cast<char*>("nativeGetHLAinteger16BE"), const_cast<char*>("(J)S"), reinterpret_cast<void*>(nativeGetHLAinteger16BE)},
    {const_cast<char*>("nativeSetHLAinteger16BE"), const_cast<char*>("(JS)V"), reinterpret_cast<void*>(nativeSetHLAinteger16BE)},
    {const_cast<char*>("nativeHLAinteger16BEOctetBoundary"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAinteger16BEOctetBoundary)},
    {const_cast<char*>("nativeHLAinteger16BEEncodedLength"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAinteger16BEEncodedLength)},
    {const_cast<char*>("nativeHLAinteger16BEToByteArray"), const_cast<char*>("(J)[B"), reinterpret_cast<void*>(nativeHLAinteger16BEToByteArray)},
    {const_cast<char*>("nativeDecodeHLAinteger16BE"), const_cast<char*>("(J[B)V"), reinterpret_cast<void*>(nativeDecodeHLAinteger16BE)},
    {const_cast<char*>("nativeCreateHLAinteger16LE"), const_cast<char*>("(S)J"), reinterpret_cast<void*>(nativeCreateHLAinteger16LE)},
    {const_cast<char*>("nativeDestroyHLAinteger16LE"), const_cast<char*>("(J)V"), reinterpret_cast<void*>(nativeDestroyHLAinteger16LE)},
    {const_cast<char*>("nativeGetHLAinteger16LE"), const_cast<char*>("(J)S"), reinterpret_cast<void*>(nativeGetHLAinteger16LE)},
    {const_cast<char*>("nativeSetHLAinteger16LE"), const_cast<char*>("(JS)V"), reinterpret_cast<void*>(nativeSetHLAinteger16LE)},
    {const_cast<char*>("nativeHLAinteger16LEOctetBoundary"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAinteger16LEOctetBoundary)},
    {const_cast<char*>("nativeHLAinteger16LEEncodedLength"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAinteger16LEEncodedLength)},
    {const_cast<char*>("nativeHLAinteger16LEToByteArray"), const_cast<char*>("(J)[B"), reinterpret_cast<void*>(nativeHLAinteger16LEToByteArray)},
    {const_cast<char*>("nativeDecodeHLAinteger16LE"), const_cast<char*>("(J[B)V"), reinterpret_cast<void*>(nativeDecodeHLAinteger16LE)},
    {const_cast<char*>("nativeCreateHLAfloat32BE"), const_cast<char*>("(F)J"), reinterpret_cast<void*>(nativeCreateHLAfloat32BE)},
    {const_cast<char*>("nativeDestroyHLAfloat32BE"), const_cast<char*>("(J)V"), reinterpret_cast<void*>(nativeDestroyHLAfloat32BE)},
    {const_cast<char*>("nativeGetHLAfloat32BE"), const_cast<char*>("(J)F"), reinterpret_cast<void*>(nativeGetHLAfloat32BE)},
    {const_cast<char*>("nativeSetHLAfloat32BE"), const_cast<char*>("(JF)V"), reinterpret_cast<void*>(nativeSetHLAfloat32BE)},
    {const_cast<char*>("nativeHLAfloat32BEOctetBoundary"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAfloat32BEOctetBoundary)},
    {const_cast<char*>("nativeHLAfloat32BEEncodedLength"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAfloat32BEEncodedLength)},
    {const_cast<char*>("nativeHLAfloat32BEToByteArray"), const_cast<char*>("(J)[B"), reinterpret_cast<void*>(nativeHLAfloat32BEToByteArray)},
    {const_cast<char*>("nativeDecodeHLAfloat32BE"), const_cast<char*>("(J[B)V"), reinterpret_cast<void*>(nativeDecodeHLAfloat32BE)},
    {const_cast<char*>("nativeCreateHLAfloat32LE"), const_cast<char*>("(F)J"), reinterpret_cast<void*>(nativeCreateHLAfloat32LE)},
    {const_cast<char*>("nativeDestroyHLAfloat32LE"), const_cast<char*>("(J)V"), reinterpret_cast<void*>(nativeDestroyHLAfloat32LE)},
    {const_cast<char*>("nativeGetHLAfloat32LE"), const_cast<char*>("(J)F"), reinterpret_cast<void*>(nativeGetHLAfloat32LE)},
    {const_cast<char*>("nativeSetHLAfloat32LE"), const_cast<char*>("(JF)V"), reinterpret_cast<void*>(nativeSetHLAfloat32LE)},
    {const_cast<char*>("nativeHLAfloat32LEOctetBoundary"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAfloat32LEOctetBoundary)},
    {const_cast<char*>("nativeHLAfloat32LEEncodedLength"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAfloat32LEEncodedLength)},
    {const_cast<char*>("nativeHLAfloat32LEToByteArray"), const_cast<char*>("(J)[B"), reinterpret_cast<void*>(nativeHLAfloat32LEToByteArray)},
    {const_cast<char*>("nativeDecodeHLAfloat32LE"), const_cast<char*>("(J[B)V"), reinterpret_cast<void*>(nativeDecodeHLAfloat32LE)},
    {const_cast<char*>("nativeCreateHLAfloat64BE"), const_cast<char*>("(D)J"), reinterpret_cast<void*>(nativeCreateHLAfloat64BE)},
    {const_cast<char*>("nativeDestroyHLAfloat64BE"), const_cast<char*>("(J)V"), reinterpret_cast<void*>(nativeDestroyHLAfloat64BE)},
    {const_cast<char*>("nativeGetHLAfloat64BE"), const_cast<char*>("(J)D"), reinterpret_cast<void*>(nativeGetHLAfloat64BE)},
    {const_cast<char*>("nativeSetHLAfloat64BE"), const_cast<char*>("(JD)V"), reinterpret_cast<void*>(nativeSetHLAfloat64BE)},
    {const_cast<char*>("nativeHLAfloat64BEOctetBoundary"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAfloat64BEOctetBoundary)},
    {const_cast<char*>("nativeHLAfloat64BEEncodedLength"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAfloat64BEEncodedLength)},
    {const_cast<char*>("nativeHLAfloat64BEToByteArray"), const_cast<char*>("(J)[B"), reinterpret_cast<void*>(nativeHLAfloat64BEToByteArray)},
    {const_cast<char*>("nativeDecodeHLAfloat64BE"), const_cast<char*>("(J[B)V"), reinterpret_cast<void*>(nativeDecodeHLAfloat64BE)},
    {const_cast<char*>("nativeCreateHLAfloat64LE"), const_cast<char*>("(D)J"), reinterpret_cast<void*>(nativeCreateHLAfloat64LE)},
    {const_cast<char*>("nativeDestroyHLAfloat64LE"), const_cast<char*>("(J)V"), reinterpret_cast<void*>(nativeDestroyHLAfloat64LE)},
    {const_cast<char*>("nativeGetHLAfloat64LE"), const_cast<char*>("(J)D"), reinterpret_cast<void*>(nativeGetHLAfloat64LE)},
    {const_cast<char*>("nativeSetHLAfloat64LE"), const_cast<char*>("(JD)V"), reinterpret_cast<void*>(nativeSetHLAfloat64LE)},
    {const_cast<char*>("nativeHLAfloat64LEOctetBoundary"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAfloat64LEOctetBoundary)},
    {const_cast<char*>("nativeHLAfloat64LEEncodedLength"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAfloat64LEEncodedLength)},
    {const_cast<char*>("nativeHLAfloat64LEToByteArray"), const_cast<char*>("(J)[B"), reinterpret_cast<void*>(nativeHLAfloat64LEToByteArray)},
    {const_cast<char*>("nativeDecodeHLAfloat64LE"), const_cast<char*>("(J[B)V"), reinterpret_cast<void*>(nativeDecodeHLAfloat64LE)},
    {const_cast<char*>("nativeCreateHLAunsignedInteger16BE"), const_cast<char*>("(S)J"), reinterpret_cast<void*>(nativeCreateHLAunsignedInteger16BE)},
    {const_cast<char*>("nativeDestroyHLAunsignedInteger16BE"), const_cast<char*>("(J)V"), reinterpret_cast<void*>(nativeDestroyHLAunsignedInteger16BE)},
    {const_cast<char*>("nativeGetHLAunsignedInteger16BE"), const_cast<char*>("(J)S"), reinterpret_cast<void*>(nativeGetHLAunsignedInteger16BE)},
    {const_cast<char*>("nativeSetHLAunsignedInteger16BE"), const_cast<char*>("(JS)V"), reinterpret_cast<void*>(nativeSetHLAunsignedInteger16BE)},
    {const_cast<char*>("nativeHLAunsignedInteger16BEOctetBoundary"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAunsignedInteger16BEOctetBoundary)},
    {const_cast<char*>("nativeHLAunsignedInteger16BEEncodedLength"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAunsignedInteger16BEEncodedLength)},
    {const_cast<char*>("nativeHLAunsignedInteger16BEToByteArray"), const_cast<char*>("(J)[B"), reinterpret_cast<void*>(nativeHLAunsignedInteger16BEToByteArray)},
    {const_cast<char*>("nativeDecodeHLAunsignedInteger16BE"), const_cast<char*>("(J[B)V"), reinterpret_cast<void*>(nativeDecodeHLAunsignedInteger16BE)},
    {const_cast<char*>("nativeCreateHLAunsignedInteger16LE"), const_cast<char*>("(S)J"), reinterpret_cast<void*>(nativeCreateHLAunsignedInteger16LE)},
    {const_cast<char*>("nativeDestroyHLAunsignedInteger16LE"), const_cast<char*>("(J)V"), reinterpret_cast<void*>(nativeDestroyHLAunsignedInteger16LE)},
    {const_cast<char*>("nativeGetHLAunsignedInteger16LE"), const_cast<char*>("(J)S"), reinterpret_cast<void*>(nativeGetHLAunsignedInteger16LE)},
    {const_cast<char*>("nativeSetHLAunsignedInteger16LE"), const_cast<char*>("(JS)V"), reinterpret_cast<void*>(nativeSetHLAunsignedInteger16LE)},
    {const_cast<char*>("nativeHLAunsignedInteger16LEOctetBoundary"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAunsignedInteger16LEOctetBoundary)},
    {const_cast<char*>("nativeHLAunsignedInteger16LEEncodedLength"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAunsignedInteger16LEEncodedLength)},
    {const_cast<char*>("nativeHLAunsignedInteger16LEToByteArray"), const_cast<char*>("(J)[B"), reinterpret_cast<void*>(nativeHLAunsignedInteger16LEToByteArray)},
    {const_cast<char*>("nativeDecodeHLAunsignedInteger16LE"), const_cast<char*>("(J[B)V"), reinterpret_cast<void*>(nativeDecodeHLAunsignedInteger16LE)},
    {const_cast<char*>("nativeCreateHLAunsignedInteger32BE"), const_cast<char*>("(I)J"), reinterpret_cast<void*>(nativeCreateHLAunsignedInteger32BE)},
    {const_cast<char*>("nativeDestroyHLAunsignedInteger32BE"), const_cast<char*>("(J)V"), reinterpret_cast<void*>(nativeDestroyHLAunsignedInteger32BE)},
    {const_cast<char*>("nativeGetHLAunsignedInteger32BE"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeGetHLAunsignedInteger32BE)},
    {const_cast<char*>("nativeSetHLAunsignedInteger32BE"), const_cast<char*>("(JI)V"), reinterpret_cast<void*>(nativeSetHLAunsignedInteger32BE)},
    {const_cast<char*>("nativeHLAunsignedInteger32BEOctetBoundary"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAunsignedInteger32BEOctetBoundary)},
    {const_cast<char*>("nativeHLAunsignedInteger32BEEncodedLength"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAunsignedInteger32BEEncodedLength)},
    {const_cast<char*>("nativeHLAunsignedInteger32BEToByteArray"), const_cast<char*>("(J)[B"), reinterpret_cast<void*>(nativeHLAunsignedInteger32BEToByteArray)},
    {const_cast<char*>("nativeDecodeHLAunsignedInteger32BE"), const_cast<char*>("(J[B)V"), reinterpret_cast<void*>(nativeDecodeHLAunsignedInteger32BE)},
    {const_cast<char*>("nativeCreateHLAunsignedInteger32LE"), const_cast<char*>("(I)J"), reinterpret_cast<void*>(nativeCreateHLAunsignedInteger32LE)},
    {const_cast<char*>("nativeDestroyHLAunsignedInteger32LE"), const_cast<char*>("(J)V"), reinterpret_cast<void*>(nativeDestroyHLAunsignedInteger32LE)},
    {const_cast<char*>("nativeGetHLAunsignedInteger32LE"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeGetHLAunsignedInteger32LE)},
    {const_cast<char*>("nativeSetHLAunsignedInteger32LE"), const_cast<char*>("(JI)V"), reinterpret_cast<void*>(nativeSetHLAunsignedInteger32LE)},
    {const_cast<char*>("nativeHLAunsignedInteger32LEOctetBoundary"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAunsignedInteger32LEOctetBoundary)},
    {const_cast<char*>("nativeHLAunsignedInteger32LEEncodedLength"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAunsignedInteger32LEEncodedLength)},
    {const_cast<char*>("nativeHLAunsignedInteger32LEToByteArray"), const_cast<char*>("(J)[B"), reinterpret_cast<void*>(nativeHLAunsignedInteger32LEToByteArray)},
    {const_cast<char*>("nativeDecodeHLAunsignedInteger32LE"), const_cast<char*>("(J[B)V"), reinterpret_cast<void*>(nativeDecodeHLAunsignedInteger32LE)},
    {const_cast<char*>("nativeCreateHLAunsignedInteger64BE"), const_cast<char*>("(J)J"), reinterpret_cast<void*>(nativeCreateHLAunsignedInteger64BE)},
    {const_cast<char*>("nativeDestroyHLAunsignedInteger64BE"), const_cast<char*>("(J)V"), reinterpret_cast<void*>(nativeDestroyHLAunsignedInteger64BE)},
    {const_cast<char*>("nativeGetHLAunsignedInteger64BE"), const_cast<char*>("(J)J"), reinterpret_cast<void*>(nativeGetHLAunsignedInteger64BE)},
    {const_cast<char*>("nativeSetHLAunsignedInteger64BE"), const_cast<char*>("(JJ)V"), reinterpret_cast<void*>(nativeSetHLAunsignedInteger64BE)},
    {const_cast<char*>("nativeHLAunsignedInteger64BEOctetBoundary"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAunsignedInteger64BEOctetBoundary)},
    {const_cast<char*>("nativeHLAunsignedInteger64BEEncodedLength"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAunsignedInteger64BEEncodedLength)},
    {const_cast<char*>("nativeHLAunsignedInteger64BEToByteArray"), const_cast<char*>("(J)[B"), reinterpret_cast<void*>(nativeHLAunsignedInteger64BEToByteArray)},
    {const_cast<char*>("nativeDecodeHLAunsignedInteger64BE"), const_cast<char*>("(J[B)V"), reinterpret_cast<void*>(nativeDecodeHLAunsignedInteger64BE)},
    {const_cast<char*>("nativeCreateHLAunsignedInteger64LE"), const_cast<char*>("(J)J"), reinterpret_cast<void*>(nativeCreateHLAunsignedInteger64LE)},
    {const_cast<char*>("nativeDestroyHLAunsignedInteger64LE"), const_cast<char*>("(J)V"), reinterpret_cast<void*>(nativeDestroyHLAunsignedInteger64LE)},
    {const_cast<char*>("nativeGetHLAunsignedInteger64LE"), const_cast<char*>("(J)J"), reinterpret_cast<void*>(nativeGetHLAunsignedInteger64LE)},
    {const_cast<char*>("nativeSetHLAunsignedInteger64LE"), const_cast<char*>("(JJ)V"), reinterpret_cast<void*>(nativeSetHLAunsignedInteger64LE)},
    {const_cast<char*>("nativeHLAunsignedInteger64LEOctetBoundary"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAunsignedInteger64LEOctetBoundary)},
    {const_cast<char*>("nativeHLAunsignedInteger64LEEncodedLength"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAunsignedInteger64LEEncodedLength)},
    {const_cast<char*>("nativeHLAunsignedInteger64LEToByteArray"), const_cast<char*>("(J)[B"), reinterpret_cast<void*>(nativeHLAunsignedInteger64LEToByteArray)},
    {const_cast<char*>("nativeDecodeHLAunsignedInteger64LE"), const_cast<char*>("(J[B)V"), reinterpret_cast<void*>(nativeDecodeHLAunsignedInteger64LE)},
    {const_cast<char*>("nativeCreateHLAbyte"), const_cast<char*>("(B)J"), reinterpret_cast<void*>(nativeCreateHLAbyte)},
    {const_cast<char*>("nativeDestroyHLAbyte"), const_cast<char*>("(J)V"), reinterpret_cast<void*>(nativeDestroyHLAbyte)},
    {const_cast<char*>("nativeGetHLAbyte"), const_cast<char*>("(J)B"), reinterpret_cast<void*>(nativeGetHLAbyte)},
    {const_cast<char*>("nativeSetHLAbyte"), const_cast<char*>("(JB)V"), reinterpret_cast<void*>(nativeSetHLAbyte)},
    {const_cast<char*>("nativeHLAbyteOctetBoundary"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAbyteOctetBoundary)},
    {const_cast<char*>("nativeHLAbyteEncodedLength"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAbyteEncodedLength)},
    {const_cast<char*>("nativeHLAbyteToByteArray"), const_cast<char*>("(J)[B"), reinterpret_cast<void*>(nativeHLAbyteToByteArray)},
    {const_cast<char*>("nativeDecodeHLAbyte"), const_cast<char*>("(J[B)V"), reinterpret_cast<void*>(nativeDecodeHLAbyte)},
    {const_cast<char*>("nativeCreateHLAoctet"), const_cast<char*>("(B)J"), reinterpret_cast<void*>(nativeCreateHLAoctet)},
    {const_cast<char*>("nativeDestroyHLAoctet"), const_cast<char*>("(J)V"), reinterpret_cast<void*>(nativeDestroyHLAoctet)},
    {const_cast<char*>("nativeGetHLAoctet"), const_cast<char*>("(J)B"), reinterpret_cast<void*>(nativeGetHLAoctet)},
    {const_cast<char*>("nativeSetHLAoctet"), const_cast<char*>("(JB)V"), reinterpret_cast<void*>(nativeSetHLAoctet)},
    {const_cast<char*>("nativeHLAoctetOctetBoundary"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAoctetOctetBoundary)},
    {const_cast<char*>("nativeHLAoctetEncodedLength"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAoctetEncodedLength)},
    {const_cast<char*>("nativeHLAoctetToByteArray"), const_cast<char*>("(J)[B"), reinterpret_cast<void*>(nativeHLAoctetToByteArray)},
    {const_cast<char*>("nativeDecodeHLAoctet"), const_cast<char*>("(J[B)V"), reinterpret_cast<void*>(nativeDecodeHLAoctet)},
    {const_cast<char*>("nativeCreateHLAoctetPairBE"), const_cast<char*>("(S)J"), reinterpret_cast<void*>(nativeCreateHLAoctetPairBE)},
    {const_cast<char*>("nativeDestroyHLAoctetPairBE"), const_cast<char*>("(J)V"), reinterpret_cast<void*>(nativeDestroyHLAoctetPairBE)},
    {const_cast<char*>("nativeGetHLAoctetPairBE"), const_cast<char*>("(J)S"), reinterpret_cast<void*>(nativeGetHLAoctetPairBE)},
    {const_cast<char*>("nativeSetHLAoctetPairBE"), const_cast<char*>("(JS)V"), reinterpret_cast<void*>(nativeSetHLAoctetPairBE)},
    {const_cast<char*>("nativeHLAoctetPairBEOctetBoundary"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAoctetPairBEOctetBoundary)},
    {const_cast<char*>("nativeHLAoctetPairBEEncodedLength"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAoctetPairBEEncodedLength)},
    {const_cast<char*>("nativeHLAoctetPairBEToByteArray"), const_cast<char*>("(J)[B"), reinterpret_cast<void*>(nativeHLAoctetPairBEToByteArray)},
    {const_cast<char*>("nativeDecodeHLAoctetPairBE"), const_cast<char*>("(J[B)V"), reinterpret_cast<void*>(nativeDecodeHLAoctetPairBE)},
    {const_cast<char*>("nativeCreateHLAoctetPairLE"), const_cast<char*>("(S)J"), reinterpret_cast<void*>(nativeCreateHLAoctetPairLE)},
    {const_cast<char*>("nativeDestroyHLAoctetPairLE"), const_cast<char*>("(J)V"), reinterpret_cast<void*>(nativeDestroyHLAoctetPairLE)},
    {const_cast<char*>("nativeGetHLAoctetPairLE"), const_cast<char*>("(J)S"), reinterpret_cast<void*>(nativeGetHLAoctetPairLE)},
    {const_cast<char*>("nativeSetHLAoctetPairLE"), const_cast<char*>("(JS)V"), reinterpret_cast<void*>(nativeSetHLAoctetPairLE)},
    {const_cast<char*>("nativeHLAoctetPairLEOctetBoundary"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAoctetPairLEOctetBoundary)},
    {const_cast<char*>("nativeHLAoctetPairLEEncodedLength"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAoctetPairLEEncodedLength)},
    {const_cast<char*>("nativeHLAoctetPairLEToByteArray"), const_cast<char*>("(J)[B"), reinterpret_cast<void*>(nativeHLAoctetPairLEToByteArray)},
    {const_cast<char*>("nativeDecodeHLAoctetPairLE"), const_cast<char*>("(J[B)V"), reinterpret_cast<void*>(nativeDecodeHLAoctetPairLE)},
    {const_cast<char*>("nativeCreateHLAASCIIchar"), const_cast<char*>("(B)J"), reinterpret_cast<void*>(nativeCreateHLAASCIIchar)},
    {const_cast<char*>("nativeDestroyHLAASCIIchar"), const_cast<char*>("(J)V"), reinterpret_cast<void*>(nativeDestroyHLAASCIIchar)},
    {const_cast<char*>("nativeGetHLAASCIIchar"), const_cast<char*>("(J)B"), reinterpret_cast<void*>(nativeGetHLAASCIIchar)},
    {const_cast<char*>("nativeSetHLAASCIIchar"), const_cast<char*>("(JB)V"), reinterpret_cast<void*>(nativeSetHLAASCIIchar)},
    {const_cast<char*>("nativeHLAASCIIcharOctetBoundary"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAASCIIcharOctetBoundary)},
    {const_cast<char*>("nativeHLAASCIIcharEncodedLength"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAASCIIcharEncodedLength)},
    {const_cast<char*>("nativeHLAASCIIcharToByteArray"), const_cast<char*>("(J)[B"), reinterpret_cast<void*>(nativeHLAASCIIcharToByteArray)},
    {const_cast<char*>("nativeDecodeHLAASCIIchar"), const_cast<char*>("(J[B)V"), reinterpret_cast<void*>(nativeDecodeHLAASCIIchar)},
    {const_cast<char*>("nativeCreateHLAunicodeChar"), const_cast<char*>("(S)J"), reinterpret_cast<void*>(nativeCreateHLAunicodeChar)},
    {const_cast<char*>("nativeDestroyHLAunicodeChar"), const_cast<char*>("(J)V"), reinterpret_cast<void*>(nativeDestroyHLAunicodeChar)},
    {const_cast<char*>("nativeGetHLAunicodeChar"), const_cast<char*>("(J)S"), reinterpret_cast<void*>(nativeGetHLAunicodeChar)},
    {const_cast<char*>("nativeSetHLAunicodeChar"), const_cast<char*>("(JS)V"), reinterpret_cast<void*>(nativeSetHLAunicodeChar)},
    {const_cast<char*>("nativeHLAunicodeCharOctetBoundary"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAunicodeCharOctetBoundary)},
    {const_cast<char*>("nativeHLAunicodeCharEncodedLength"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAunicodeCharEncodedLength)},
    {const_cast<char*>("nativeHLAunicodeCharToByteArray"), const_cast<char*>("(J)[B"), reinterpret_cast<void*>(nativeHLAunicodeCharToByteArray)},
    {const_cast<char*>("nativeDecodeHLAunicodeChar"), const_cast<char*>("(J[B)V"), reinterpret_cast<void*>(nativeDecodeHLAunicodeChar)},
    {const_cast<char*>("nativeCreateHLAboolean"), const_cast<char*>("(Z)J"), reinterpret_cast<void*>(nativeCreateHLAboolean)},
    {const_cast<char*>("nativeDestroyHLAboolean"), const_cast<char*>("(J)V"), reinterpret_cast<void*>(nativeDestroyHLAboolean)},
    {const_cast<char*>("nativeGetHLAboolean"), const_cast<char*>("(J)Z"), reinterpret_cast<void*>(nativeGetHLAboolean)},
    {const_cast<char*>("nativeSetHLAboolean"), const_cast<char*>("(JZ)V"), reinterpret_cast<void*>(nativeSetHLAboolean)},
    {const_cast<char*>("nativeHLAbooleanOctetBoundary"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAbooleanOctetBoundary)},
    {const_cast<char*>("nativeHLAbooleanEncodedLength"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAbooleanEncodedLength)},
    {const_cast<char*>("nativeHLAbooleanToByteArray"), const_cast<char*>("(J)[B"), reinterpret_cast<void*>(nativeHLAbooleanToByteArray)},
    {const_cast<char*>("nativeDecodeHLAboolean"), const_cast<char*>("(J[B)V"), reinterpret_cast<void*>(nativeDecodeHLAboolean)},
    {const_cast<char*>("nativeCreateHLAASCIIstring"), const_cast<char*>("(Ljava/lang/String;)J"), reinterpret_cast<void*>(nativeCreateHLAASCIIstring)},
    {const_cast<char*>("nativeDestroyHLAASCIIstring"), const_cast<char*>("(J)V"), reinterpret_cast<void*>(nativeDestroyHLAASCIIstring)},
    {const_cast<char*>("nativeGetHLAASCIIstring"), const_cast<char*>("(J)Ljava/lang/String;"), reinterpret_cast<void*>(nativeGetHLAASCIIstring)},
    {const_cast<char*>("nativeSetHLAASCIIstring"), const_cast<char*>("(JLjava/lang/String;)V"), reinterpret_cast<void*>(nativeSetHLAASCIIstring)},
    {const_cast<char*>("nativeHLAASCIIstringOctetBoundary"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAASCIIstringOctetBoundary)},
    {const_cast<char*>("nativeHLAASCIIstringEncodedLength"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAASCIIstringEncodedLength)},
    {const_cast<char*>("nativeHLAASCIIstringToByteArray"), const_cast<char*>("(J)[B"), reinterpret_cast<void*>(nativeHLAASCIIstringToByteArray)},
    {const_cast<char*>("nativeDecodeHLAASCIIstring"), const_cast<char*>("(J[B)V"), reinterpret_cast<void*>(nativeDecodeHLAASCIIstring)},
    {const_cast<char*>("nativeCreateHLAunicodeString"), const_cast<char*>("(Ljava/lang/String;)J"), reinterpret_cast<void*>(nativeCreateHLAunicodeString)},
    {const_cast<char*>("nativeDestroyHLAunicodeString"), const_cast<char*>("(J)V"), reinterpret_cast<void*>(nativeDestroyHLAunicodeString)},
    {const_cast<char*>("nativeGetHLAunicodeString"), const_cast<char*>("(J)Ljava/lang/String;"), reinterpret_cast<void*>(nativeGetHLAunicodeString)},
    {const_cast<char*>("nativeSetHLAunicodeString"), const_cast<char*>("(JLjava/lang/String;)V"), reinterpret_cast<void*>(nativeSetHLAunicodeString)},
    {const_cast<char*>("nativeHLAunicodeStringOctetBoundary"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAunicodeStringOctetBoundary)},
    {const_cast<char*>("nativeHLAunicodeStringEncodedLength"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAunicodeStringEncodedLength)},
    {const_cast<char*>("nativeHLAunicodeStringToByteArray"), const_cast<char*>("(J)[B"), reinterpret_cast<void*>(nativeHLAunicodeStringToByteArray)},
    {const_cast<char*>("nativeDecodeHLAunicodeString"), const_cast<char*>("(J[B)V"), reinterpret_cast<void*>(nativeDecodeHLAunicodeString)},
    {const_cast<char*>("nativeCreateHLAopaqueData"), const_cast<char*>("([B)J"), reinterpret_cast<void*>(nativeCreateHLAopaqueData)},
    {const_cast<char*>("nativeDestroyHLAopaqueData"), const_cast<char*>("(J)V"), reinterpret_cast<void*>(nativeDestroyHLAopaqueData)},
    {const_cast<char*>("nativeHLAopaqueDataSize"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAopaqueDataSize)},
    {const_cast<char*>("nativeGetHLAopaqueData"), const_cast<char*>("(JI)B"), reinterpret_cast<void*>(nativeGetHLAopaqueData)},
    {const_cast<char*>("nativeGetHLAopaqueDataValue"), const_cast<char*>("(J)[B"), reinterpret_cast<void*>(nativeGetHLAopaqueDataValue)},
    {const_cast<char*>("nativeSetHLAopaqueData"), const_cast<char*>("(J[B)V"), reinterpret_cast<void*>(nativeSetHLAopaqueData)},
    {const_cast<char*>("nativeHLAopaqueDataOctetBoundary"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAopaqueDataOctetBoundary)},
    {const_cast<char*>("nativeHLAopaqueDataEncodedLength"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAopaqueDataEncodedLength)},
    {const_cast<char*>("nativeHLAopaqueDataToByteArray"), const_cast<char*>("(J)[B"), reinterpret_cast<void*>(nativeHLAopaqueDataToByteArray)},
    {const_cast<char*>("nativeDecodeHLAopaqueData"), const_cast<char*>("(J[B)V"), reinterpret_cast<void*>(nativeDecodeHLAopaqueData)},
    {const_cast<char*>("nativeCreateHLAvariableArray"), const_cast<char*>("(Lhla/rti1516_2025/encoding/DataElement;)J"), reinterpret_cast<void*>(nativeCreateHLAvariableArray)},
    {const_cast<char*>("nativeDestroyHLAvariableArray"), const_cast<char*>("(J)V"), reinterpret_cast<void*>(nativeDestroyHLAvariableArray)},
    {const_cast<char*>("nativeAddHLAvariableArrayElement"), const_cast<char*>("(JLhla/rti1516_2025/encoding/DataElement;)V"), reinterpret_cast<void*>(nativeAddHLAvariableArrayElement)},
    {const_cast<char*>("nativeResizeHLAvariableArray"), const_cast<char*>("(JI)V"), reinterpret_cast<void*>(nativeResizeHLAvariableArray)},
    {const_cast<char*>("nativeHLAvariableArraySize"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAvariableArraySize)},
    {const_cast<char*>("nativeHLAvariableArrayElementEncoding"), const_cast<char*>("(JI)[B"), reinterpret_cast<void*>(nativeHLAvariableArrayElementEncoding)},
    {const_cast<char*>("nativeHLAvariableArrayOctetBoundary"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAvariableArrayOctetBoundary)},
    {const_cast<char*>("nativeHLAvariableArrayEncodedLength"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAvariableArrayEncodedLength)},
    {const_cast<char*>("nativeHLAvariableArrayToByteArray"), const_cast<char*>("(J)[B"), reinterpret_cast<void*>(nativeHLAvariableArrayToByteArray)},
    {const_cast<char*>("nativeDecodeHLAvariableArray"), const_cast<char*>("(J[B)V"), reinterpret_cast<void*>(nativeDecodeHLAvariableArray)},
    {const_cast<char*>("nativeCreateHLAfixedArray"), const_cast<char*>("(Lhla/rti1516_2025/encoding/DataElement;I)J"), reinterpret_cast<void*>(nativeCreateHLAfixedArray)},
    {const_cast<char*>("nativeDestroyHLAfixedArray"), const_cast<char*>("(J)V"), reinterpret_cast<void*>(nativeDestroyHLAfixedArray)},
    {const_cast<char*>("nativeSetHLAfixedArrayElement"), const_cast<char*>("(JILhla/rti1516_2025/encoding/DataElement;)V"), reinterpret_cast<void*>(nativeSetHLAfixedArrayElement)},
    {const_cast<char*>("nativeHLAfixedArraySize"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAfixedArraySize)},
    {const_cast<char*>("nativeHLAfixedArrayElementEncoding"), const_cast<char*>("(JI)[B"), reinterpret_cast<void*>(nativeHLAfixedArrayElementEncoding)},
    {const_cast<char*>("nativeHLAfixedArrayOctetBoundary"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAfixedArrayOctetBoundary)},
    {const_cast<char*>("nativeHLAfixedArrayEncodedLength"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAfixedArrayEncodedLength)},
    {const_cast<char*>("nativeHLAfixedArrayToByteArray"), const_cast<char*>("(J)[B"), reinterpret_cast<void*>(nativeHLAfixedArrayToByteArray)},
    {const_cast<char*>("nativeDecodeHLAfixedArray"), const_cast<char*>("(J[B)V"), reinterpret_cast<void*>(nativeDecodeHLAfixedArray)},
    {const_cast<char*>("nativeCreateHLAfixedRecord"), const_cast<char*>("()J"), reinterpret_cast<void*>(nativeCreateHLAfixedRecord)},
    {const_cast<char*>("nativeDestroyHLAfixedRecord"), const_cast<char*>("(J)V"), reinterpret_cast<void*>(nativeDestroyHLAfixedRecord)},
    {const_cast<char*>("nativeAppendHLAfixedRecordElement"), const_cast<char*>("(JLhla/rti1516_2025/encoding/DataElement;)V"), reinterpret_cast<void*>(nativeAppendHLAfixedRecordElement)},
    {const_cast<char*>("nativeSetHLAfixedRecordElement"), const_cast<char*>("(JILhla/rti1516_2025/encoding/DataElement;)V"), reinterpret_cast<void*>(nativeSetHLAfixedRecordElement)},
    {const_cast<char*>("nativeHLAfixedRecordSize"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAfixedRecordSize)},
    {const_cast<char*>("nativeHLAfixedRecordElementEncoding"), const_cast<char*>("(JI)[B"), reinterpret_cast<void*>(nativeHLAfixedRecordElementEncoding)},
    {const_cast<char*>("nativeHLAfixedRecordOctetBoundary"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAfixedRecordOctetBoundary)},
    {const_cast<char*>("nativeHLAfixedRecordEncodedLength"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAfixedRecordEncodedLength)},
    {const_cast<char*>("nativeHLAfixedRecordToByteArray"), const_cast<char*>("(J)[B"), reinterpret_cast<void*>(nativeHLAfixedRecordToByteArray)},
    {const_cast<char*>("nativeDecodeHLAfixedRecord"), const_cast<char*>("(J[B)V"), reinterpret_cast<void*>(nativeDecodeHLAfixedRecord)},
    {const_cast<char*>("nativeCreateHLAvariantRecord"), const_cast<char*>("(Lhla/rti1516_2025/encoding/DataElement;)J"), reinterpret_cast<void*>(nativeCreateHLAvariantRecord)},
    {const_cast<char*>("nativeDestroyHLAvariantRecord"), const_cast<char*>("(J)V"), reinterpret_cast<void*>(nativeDestroyHLAvariantRecord)},
    {const_cast<char*>("nativeSetHLAvariantRecordVariant"), const_cast<char*>("(JLhla/rti1516_2025/encoding/DataElement;Lhla/rti1516_2025/encoding/DataElement;)V"), reinterpret_cast<void*>(nativeSetHLAvariantRecordVariant)},
    {const_cast<char*>("nativeSetHLAvariantRecordDiscriminant"), const_cast<char*>("(JLhla/rti1516_2025/encoding/DataElement;)V"), reinterpret_cast<void*>(nativeSetHLAvariantRecordDiscriminant)},
    {const_cast<char*>("nativeHLAvariantRecordDiscriminantEncoding"), const_cast<char*>("(J)[B"), reinterpret_cast<void*>(nativeHLAvariantRecordDiscriminantEncoding)},
    {const_cast<char*>("nativeHLAvariantRecordValueEncoding"), const_cast<char*>("(J)[B"), reinterpret_cast<void*>(nativeHLAvariantRecordValueEncoding)},
    {const_cast<char*>("nativeHLAvariantRecordOctetBoundary"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAvariantRecordOctetBoundary)},
    {const_cast<char*>("nativeHLAvariantRecordEncodedLength"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAvariantRecordEncodedLength)},
    {const_cast<char*>("nativeHLAvariantRecordToByteArray"), const_cast<char*>("(J)[B"), reinterpret_cast<void*>(nativeHLAvariantRecordToByteArray)},
    {const_cast<char*>("nativeDecodeHLAvariantRecord"), const_cast<char*>("(J[B)V"), reinterpret_cast<void*>(nativeDecodeHLAvariantRecord)},
    {const_cast<char*>("nativeCreateHLAextendableVariantRecord"), const_cast<char*>("(Lhla/rti1516_2025/encoding/DataElement;)J"), reinterpret_cast<void*>(nativeCreateHLAextendableVariantRecord)},
    {const_cast<char*>("nativeDestroyHLAextendableVariantRecord"), const_cast<char*>("(J)V"), reinterpret_cast<void*>(nativeDestroyHLAextendableVariantRecord)},
    {const_cast<char*>("nativeAddHLAextendableVariantRecordVariant"), const_cast<char*>("(JLhla/rti1516_2025/encoding/DataElement;Lhla/rti1516_2025/encoding/DataElement;)V"), reinterpret_cast<void*>(nativeAddHLAextendableVariantRecordVariant)},
    {const_cast<char*>("nativeSetHLAextendableVariantRecordVariant"), const_cast<char*>("(JLhla/rti1516_2025/encoding/DataElement;Lhla/rti1516_2025/encoding/DataElement;)V"), reinterpret_cast<void*>(nativeSetHLAextendableVariantRecordVariant)},
    {const_cast<char*>("nativeSetHLAextendableVariantRecordDiscriminant"), const_cast<char*>("(JLhla/rti1516_2025/encoding/DataElement;)V"), reinterpret_cast<void*>(nativeSetHLAextendableVariantRecordDiscriminant)},
    {const_cast<char*>("nativeHLAextendableVariantRecordDiscriminantEncoding"), const_cast<char*>("(J)[B"), reinterpret_cast<void*>(nativeHLAextendableVariantRecordDiscriminantEncoding)},
    {const_cast<char*>("nativeHLAextendableVariantRecordValueEncoding"), const_cast<char*>("(J)[B"), reinterpret_cast<void*>(nativeHLAextendableVariantRecordValueEncoding)},
    {const_cast<char*>("nativeHLAextendableVariantRecordOctetBoundary"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAextendableVariantRecordOctetBoundary)},
    {const_cast<char*>("nativeHLAextendableVariantRecordEncodedLength"), const_cast<char*>("(J)I"), reinterpret_cast<void*>(nativeHLAextendableVariantRecordEncodedLength)},
    {const_cast<char*>("nativeHLAextendableVariantRecordToByteArray"), const_cast<char*>("(J)[B"), reinterpret_cast<void*>(nativeHLAextendableVariantRecordToByteArray)},
    {const_cast<char*>("nativeDecodeHLAextendableVariantRecord"), const_cast<char*>("(J[B)V"), reinterpret_cast<void*>(nativeDecodeHLAextendableVariantRecord)},
    {const_cast<char*>("nativeDecodeFederateHandle"), const_cast<char*>("([B)[B"), reinterpret_cast<void*>(nativeDecodeFederateHandle)},
    {const_cast<char*>("nativeRegisterFederationSynchronizationPoint"), const_cast<char*>("(JLjava/lang/String;[B[[B)V"), reinterpret_cast<void*>(nativeRegisterFederationSynchronizationPoint)},
    {const_cast<char*>("nativeSynchronizationPointAchieved"), const_cast<char*>("(JLjava/lang/String;Z)V"), reinterpret_cast<void*>(nativeSynchronizationPointAchieved)},
    {const_cast<char*>("nativeQueryFederationSaveStatus"), const_cast<char*>("(J)V"), reinterpret_cast<void*>(nativeQueryFederationSaveStatus)},
    {const_cast<char*>("nativeRequestFederationSave"), const_cast<char*>("(JLjava/lang/String;)V"), reinterpret_cast<void*>(nativeRequestFederationSave)},
    {const_cast<char*>("nativeRequestFederationSaveWithTime"), const_cast<char*>("(JLjava/lang/String;[B)V"), reinterpret_cast<void*>(nativeRequestFederationSaveWithTime)},
    {const_cast<char*>("nativeFederateSaveBegun"), const_cast<char*>("(J)V"), reinterpret_cast<void*>(nativeFederateSaveBegun)},
    {const_cast<char*>("nativeFederateSaveComplete"), const_cast<char*>("(J)V"), reinterpret_cast<void*>(nativeFederateSaveComplete)},
    {const_cast<char*>("nativeFederateSaveNotComplete"), const_cast<char*>("(J)V"), reinterpret_cast<void*>(nativeFederateSaveNotComplete)},
    {const_cast<char*>("nativeAbortFederationSave"), const_cast<char*>("(J)V"), reinterpret_cast<void*>(nativeAbortFederationSave)},
    {const_cast<char*>("nativeQueryFederationRestoreStatus"), const_cast<char*>("(J)V"), reinterpret_cast<void*>(nativeQueryFederationRestoreStatus)},
    {const_cast<char*>("nativeRequestFederationRestore"), const_cast<char*>("(JLjava/lang/String;)V"), reinterpret_cast<void*>(nativeRequestFederationRestore)},
    {const_cast<char*>("nativeFederateRestoreComplete"), const_cast<char*>("(J)V"), reinterpret_cast<void*>(nativeFederateRestoreComplete)},
    {const_cast<char*>("nativeFederateRestoreNotComplete"), const_cast<char*>("(J)V"), reinterpret_cast<void*>(nativeFederateRestoreNotComplete)},
    {const_cast<char*>("nativeAbortFederationRestore"), const_cast<char*>("(J)V"), reinterpret_cast<void*>(nativeAbortFederationRestore)},
};

}  // namespace

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* virtualMachine, void*) {
  JNIEnv* environment = nullptr;
  if (virtualMachine->GetEnv(reinterpret_cast<void**>(&environment), JNI_VERSION_1_8) != JNI_OK ||
      environment == nullptr) {
    return JNI_ERR;
  }
  jclass bridge = environment->FindClass("org/umbra/jni/rti1516_2025/NativeBridge");
  if (bridge == nullptr) {
    clearJavaException(environment);
    return JNI_ERR;
  }
  auto const status = environment->RegisterNatives(
      bridge,
      nativeMethods,
      static_cast<jint>(sizeof(nativeMethods) / sizeof(nativeMethods[0])));
  environment->DeleteLocalRef(bridge);
  return status == JNI_OK ? JNI_VERSION_1_8 : JNI_ERR;
}
