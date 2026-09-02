#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/RTIambassadorFactory.h>
#include <RTI/encoding/BasicDataElements.h>
#include <RTI/encoding/EncodingExceptions.h>
#include <RTI/encoding/HLAfixedArray.h>
#include <RTI/encoding/HLAfixedRecord.h>
#include <RTI/encoding/HLAopaqueData.h>
#include <RTI/encoding/HLAvariableArray.h>
#include <RTI/encoding/HLAvariantRecord.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>
#include <RTI/time/HLAinteger64TimeFactory.h>
#include <RTI/time/HLAfloat64Interval.h>
#include <RTI/time/HLAfloat64Time.h>
#include <RTI/time/HLAfloat64TimeFactory.h>

#include "internal/handles/2010_handle_factories.hpp"
#include "internal/exception_mapping_2010.hpp"
#include "internal/utf8.hpp"

#include <pybind11/pybind11.h>

#include <memory>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace py = pybind11;
namespace rti = rti1516e;

namespace {

PyObject* native_rti_error_type = nullptr;

std::string narrow(std::wstring const& value) {
  return umbra::utf8::encode(value);
}

std::wstring widen(std::string const& value) {
  return umbra::utf8::decode(value);
}

[[noreturn]] void raise_native_error(char const* name, std::wstring const& message) {
  std::string detail(name);
  detail += ": ";
  detail += narrow(message);
  PyObject* detailObject = PyUnicode_FromStringAndSize(
      detail.data(), static_cast<Py_ssize_t>(detail.size()));
  if (detailObject == nullptr) {
    throw py::error_already_set();
  }
  PyErr_SetObject(native_rti_error_type, detailObject);
  Py_DECREF(detailObject);
  throw py::error_already_set();
}

[[noreturn]] void raise_native_exception(rti::Exception const& error) {
  std::string const name = umbra::rti1516e_2010::exceptionName(error);
  raise_native_error(name.c_str(), error.what());
}

template <typename Element>
py::bytes encoded_element(Element const& element) {
  auto const value = element.encode();
  return py::bytes(static_cast<char const*>(value.data()), value.size());
}

rti::VariableLengthData variable_length_data(py::bytes const& value) {
  std::string const copied = value;
  return rti::VariableLengthData(copied.data(), copied.size());
}

std::vector<rti::Octet> octets(py::bytes const& value) {
  std::string const copied = value;
  return std::vector<rti::Octet>(copied.begin(), copied.end());
}

py::bytes encoded_time(rti::LogicalTime const& value) {
  auto const encoded = value.encode();
  return py::bytes(static_cast<char const*>(encoded.data()), encoded.size());
}

template <typename Handle>
py::bytes encoded_handle(Handle const& value) {
  auto const encoded = value.encode();
  return py::bytes(static_cast<char const*>(encoded.data()), encoded.size());
}

template <typename Handle>
Handle decode_handle(py::bytes const& value, Handle (*decoder)(rti::VariableLengthData const&)) {
  return decoder(variable_length_data(value));
}

py::bytes encoded_interval(rti::LogicalTimeInterval const& value) {
  auto const encoded = value.encode();
  return py::bytes(static_cast<char const*>(encoded.data()), encoded.size());
}

int char_to_int(char value) {
  return static_cast<int>(static_cast<unsigned char>(value));
}

char int_to_char(int value) {
  if (value < 0 || value > 0xff) {
    raise_native_error("EncoderException", L"character value is outside the octet range.");
  }
  return static_cast<char>(value);
}

int wchar_to_int(wchar_t value) {
  return static_cast<int>(value);
}

wchar_t int_to_wchar(int value) {
  if (value < 0 || value > 0xffff) {
    raise_native_error("EncoderException", L"unicode character value is outside the 16-bit range.");
  }
  return static_cast<wchar_t>(value);
}

std::uint16_t pair_to_int(rti::OctetPair const& value) {
  return static_cast<std::uint16_t>(
      (static_cast<std::uint16_t>(static_cast<unsigned char>(value.first)) << 8U) |
      static_cast<unsigned char>(value.second));
}

rti::OctetPair int_to_pair(std::uint16_t value) {
  return rti::OctetPair(
      static_cast<rti::Octet>((value >> 8U) & 0xffU),
      static_cast<rti::Octet>(value & 0xffU));
}

template <typename T>
T identity_value(T value) {
  return value;
}

template <typename Element>
std::size_t decode_native_from(
    Element& element,
    py::bytes const& value,
    std::size_t index) {
  auto const bytes = octets(value);
  try {
    return element.decodeFrom(bytes, index);
  } catch (rti::EncoderException const& error) {
    raise_native_error("DecoderException", error.what());
  } catch (rti::Exception const& error) {
    raise_native_exception(error);
  }
}

rti::Integer16 int16_from(std::int16_t value) { return static_cast<rti::Integer16>(value); }
std::int16_t int16_to(rti::Integer16 value) { return static_cast<std::int16_t>(value); }
rti::Integer32 int32_from(std::int32_t value) { return static_cast<rti::Integer32>(value); }
std::int32_t int32_to(rti::Integer32 value) { return static_cast<std::int32_t>(value); }
rti::Integer64 int64_from(std::int64_t value) { return static_cast<rti::Integer64>(value); }
std::int64_t int64_to(rti::Integer64 value) { return static_cast<std::int64_t>(value); }
rti::Octet octet_from(std::uint8_t value) { return static_cast<rti::Octet>(value); }
std::uint8_t octet_to(rti::Octet value) { return static_cast<std::uint8_t>(value); }

class Native2010ElementBridge {
 public:
  virtual ~Native2010ElementBridge() = default;
  virtual std::unique_ptr<rti::DataElement> clone_data_element() const = 0;
};

template <typename Element>
std::unique_ptr<rti::DataElement> clone_native_element(Element const& element) {
  std::auto_ptr<rti::DataElement> clone = element.clone();
  if (clone.get() == nullptr) {
    raise_native_error("RTIinternalError", L"The 2010 data-element clone is null.");
  }
  return std::unique_ptr<rti::DataElement>(clone.release());
}

#define UMBRA_2010_DEFINE_NATIVE_VALUE(NAME, NATIVE, PYTYPE, FROM_PYTHON, TO_PYTHON) \
  class NAME : public Native2010ElementBridge { \
   public: \
    NAME() = default; \
    explicit NAME(PYTYPE value) : element_(FROM_PYTHON(value)) {} \
    PYTYPE get_value() const { \
      try { return TO_PYTHON(element_.get()); } \
      catch (rti::EncoderException const& error) { raise_native_error("EncoderException", error.what()); } \
      catch (rti::Exception const& error) { raise_native_exception(error); } \
    } \
    void set_value(PYTYPE value) { \
      try { element_.set(FROM_PYTHON(value)); } \
      catch (rti::EncoderException const& error) { raise_native_error("EncoderException", error.what()); } \
      catch (rti::Exception const& error) { raise_native_exception(error); } \
    } \
    py::bytes to_byte_array() const { \
      try { return encoded_element(element_); } \
      catch (rti::EncoderException const& error) { raise_native_error("EncoderException", error.what()); } \
      catch (rti::Exception const& error) { raise_native_exception(error); } \
    } \
    void decode(py::bytes const& value) { \
      try { element_.decode(variable_length_data(value)); } \
      catch (rti::EncoderException const& error) { raise_native_error("DecoderException", error.what()); } \
      catch (rti::Exception const& error) { raise_native_exception(error); } \
    } \
    std::size_t decode_from(py::bytes const& value, std::size_t index) { \
      return decode_native_from(element_, value, index); \
    } \
    std::size_t get_encoded_length() const { \
      try { return element_.getEncodedLength(); } \
      catch (rti::EncoderException const& error) { raise_native_error("EncoderException", error.what()); } \
      catch (rti::Exception const& error) { raise_native_exception(error); } \
    } \
    unsigned int get_octet_boundary() const { return element_.getOctetBoundary(); } \
    std::unique_ptr<rti::DataElement> clone_data_element() const override { \
      return clone_native_element(element_); \
    } \
   private: \
    NATIVE element_; \
  };

UMBRA_2010_DEFINE_NATIVE_VALUE(
    NativeHLAinteger16BE, rti::HLAinteger16BE, std::int16_t,
    int16_from, int16_to)
UMBRA_2010_DEFINE_NATIVE_VALUE(
    NativeHLAinteger16LE, rti::HLAinteger16LE, std::int16_t,
    int16_from, int16_to)
UMBRA_2010_DEFINE_NATIVE_VALUE(
    NativeHLAinteger32BE, rti::HLAinteger32BE, std::int32_t,
    int32_from, int32_to)
UMBRA_2010_DEFINE_NATIVE_VALUE(
    NativeHLAinteger32LE, rti::HLAinteger32LE, std::int32_t,
    int32_from, int32_to)
UMBRA_2010_DEFINE_NATIVE_VALUE(
    NativeHLAinteger64BE, rti::HLAinteger64BE, std::int64_t,
    int64_from, int64_to)
UMBRA_2010_DEFINE_NATIVE_VALUE(
    NativeHLAinteger64LE, rti::HLAinteger64LE, std::int64_t,
    int64_from, int64_to)
UMBRA_2010_DEFINE_NATIVE_VALUE(
    NativeHLAfloat32BE, rti::HLAfloat32BE, float, identity_value, identity_value)
UMBRA_2010_DEFINE_NATIVE_VALUE(
    NativeHLAfloat32LE, rti::HLAfloat32LE, float, identity_value, identity_value)
UMBRA_2010_DEFINE_NATIVE_VALUE(
    NativeHLAfloat64BE, rti::HLAfloat64BE, double, identity_value, identity_value)
UMBRA_2010_DEFINE_NATIVE_VALUE(
    NativeHLAfloat64LE, rti::HLAfloat64LE, double, identity_value, identity_value)
UMBRA_2010_DEFINE_NATIVE_VALUE(
    NativeHLAbyte, rti::HLAbyte, std::uint8_t,
    octet_from, octet_to)
UMBRA_2010_DEFINE_NATIVE_VALUE(
    NativeHLAoctet, rti::HLAoctet, std::uint8_t,
    octet_from, octet_to)
UMBRA_2010_DEFINE_NATIVE_VALUE(
    NativeHLAASCIIchar, rti::HLAASCIIchar, int, int_to_char, char_to_int)
UMBRA_2010_DEFINE_NATIVE_VALUE(
    NativeHLAunicodeChar, rti::HLAunicodeChar, int, int_to_wchar, wchar_to_int)
UMBRA_2010_DEFINE_NATIVE_VALUE(
    NativeHLAoctetPairBE, rti::HLAoctetPairBE, std::uint16_t, int_to_pair, pair_to_int)
UMBRA_2010_DEFINE_NATIVE_VALUE(
    NativeHLAoctetPairLE, rti::HLAoctetPairLE, std::uint16_t, int_to_pair, pair_to_int)
UMBRA_2010_DEFINE_NATIVE_VALUE(
    NativeHLAboolean, rti::HLAboolean, bool, identity_value, identity_value)
UMBRA_2010_DEFINE_NATIVE_VALUE(
    NativeHLAASCIIstring, rti::HLAASCIIstring, std::string, identity_value, identity_value)
UMBRA_2010_DEFINE_NATIVE_VALUE(
    NativeHLAunicodeString, rti::HLAunicodeString, std::wstring, identity_value, identity_value)

#undef UMBRA_2010_DEFINE_NATIVE_VALUE

class NativeHLAopaqueData : public Native2010ElementBridge {
 public:
  NativeHLAopaqueData() = default;
  explicit NativeHLAopaqueData(py::bytes value) { set_value(value); }

  py::bytes get_value() const {
    auto const* data = element_.get();
    return py::bytes(
        data == nullptr ? "" : reinterpret_cast<char const*>(data),
        element_.dataLength());
  }

  void set_value(py::bytes value) {
    std::string const copied = value;
    try {
      element_.set(
          copied.empty() ? nullptr : reinterpret_cast<rti::Octet const*>(copied.data()),
          copied.size());
    } catch (rti::EncoderException const& error) {
      raise_native_error("EncoderException", error.what());
    } catch (rti::Exception const& error) {
      raise_native_exception(error);
    }
  }

  py::bytes to_byte_array() const {
    try {
      return encoded_element(element_);
    } catch (rti::EncoderException const& error) {
      raise_native_error("EncoderException", error.what());
    } catch (rti::Exception const& error) {
      raise_native_exception(error);
    }
  }

  void decode(py::bytes const& value) {
    try {
      element_.decode(variable_length_data(value));
    } catch (rti::EncoderException const& error) {
      raise_native_error("DecoderException", error.what());
    } catch (rti::Exception const& error) {
      raise_native_exception(error);
    }
  }

  std::size_t decode_from(py::bytes const& value, std::size_t index) {
    return decode_native_from(element_, value, index);
  }

  std::size_t get_encoded_length() const {
    try {
      return element_.getEncodedLength();
    } catch (rti::EncoderException const& error) {
      raise_native_error("EncoderException", error.what());
    } catch (rti::Exception const& error) {
      raise_native_exception(error);
    }
  }

  unsigned int get_octet_boundary() const { return element_.getOctetBoundary(); }
  std::size_t data_length() const { return element_.dataLength(); }
  std::size_t buffer_length() const { return element_.bufferLength(); }
  std::unique_ptr<rti::DataElement> clone_data_element() const override {
    return clone_native_element(element_);
  }

 private:
  rti::HLAopaqueData element_;
};

std::unique_ptr<rti::DataElement> native_prototype(py::object const& prototype) {
  auto* bridge = prototype.cast<Native2010ElementBridge*>();
  if (bridge == nullptr) {
    raise_native_error("EncoderException", L"The 2010 data-element prototype is invalid.");
  }
  return bridge->clone_data_element();
}

std::unique_ptr<rti::DataElement> decoded_native_prototype(
    py::object const& prototype,
    py::bytes const& encoded) {
  auto value = native_prototype(prototype);
  try {
    value->decode(variable_length_data(encoded));
  } catch (rti::EncoderException const& error) {
    raise_native_error("DecoderException", error.what());
  } catch (rti::Exception const& error) {
    raise_native_exception(error);
  }
  return value;
}

class NativeHLAvariableArray : public Native2010ElementBridge {
 public:
  explicit NativeHLAvariableArray(py::object const& prototype)
      : prototype_(native_prototype(prototype)),
        element_(new rti::HLAvariableArray(*prototype_)) {}

  std::size_t size() const { return element_->size(); }

  void add_element(py::bytes const& encoded) {
    auto value = prototype_->clone();
    try {
      value->decode(variable_length_data(encoded));
      element_->addElement(*value);
    } catch (rti::EncoderException const& error) {
      raise_native_error("EncoderException", error.what());
    } catch (rti::Exception const& error) {
      raise_native_exception(error);
    }
  }

  void set_element(std::size_t index, py::bytes const& encoded) {
    auto value = prototype_->clone();
    try {
      value->decode(variable_length_data(encoded));
      element_->set(index, *value);
    } catch (rti::EncoderException const& error) {
      raise_native_error("EncoderException", error.what());
    } catch (rti::Exception const& error) {
      raise_native_exception(error);
    }
  }

  py::bytes get_element_bytes(std::size_t index) const {
    try {
      return encoded_element(element_->get(index));
    } catch (rti::EncoderException const& error) {
      raise_native_error("EncoderException", error.what());
    } catch (rti::Exception const& error) {
      raise_native_exception(error);
    }
  }

  py::bytes to_byte_array() const {
    try {
      return encoded_element(*element_);
    } catch (rti::EncoderException const& error) {
      raise_native_error("EncoderException", error.what());
    } catch (rti::Exception const& error) {
      raise_native_exception(error);
    }
  }

  void decode(py::bytes const& value) {
    try {
      element_->decode(variable_length_data(value));
    } catch (rti::EncoderException const& error) {
      raise_native_error("DecoderException", error.what());
    } catch (rti::Exception const& error) {
      raise_native_exception(error);
    }
  }

  std::size_t decode_from(py::bytes const& value, std::size_t index) {
    return decode_native_from(*element_, value, index);
  }

  std::size_t get_encoded_length() const {
    try {
      return element_->getEncodedLength();
    } catch (rti::EncoderException const& error) {
      raise_native_error("EncoderException", error.what());
    } catch (rti::Exception const& error) {
      raise_native_exception(error);
    }
  }

  unsigned int get_octet_boundary() const { return element_->getOctetBoundary(); }

  std::unique_ptr<rti::DataElement> clone_data_element() const override {
    return clone_native_element(*element_);
  }

 private:
  std::unique_ptr<rti::DataElement> prototype_;
  std::unique_ptr<rti::HLAvariableArray> element_;
};

class NativeHLAfixedArray : public Native2010ElementBridge {
 public:
  NativeHLAfixedArray(py::object const& prototype, std::size_t length)
      : prototype_(native_prototype(prototype)),
        element_(new rti::HLAfixedArray(*prototype_, length)) {}

  std::size_t size() const { return element_->size(); }

  void set_element(std::size_t index, py::bytes const& encoded) {
    auto value = prototype_->clone();
    try {
      value->decode(variable_length_data(encoded));
      element_->set(index, *value);
    } catch (rti::EncoderException const& error) {
      raise_native_error("EncoderException", error.what());
    } catch (rti::Exception const& error) {
      raise_native_exception(error);
    }
  }

  py::bytes get_element_bytes(std::size_t index) const {
    try {
      return encoded_element(element_->get(index));
    } catch (rti::EncoderException const& error) {
      raise_native_error("EncoderException", error.what());
    } catch (rti::Exception const& error) {
      raise_native_exception(error);
    }
  }

  py::bytes to_byte_array() const {
    try {
      return encoded_element(*element_);
    } catch (rti::EncoderException const& error) {
      raise_native_error("EncoderException", error.what());
    } catch (rti::Exception const& error) {
      raise_native_exception(error);
    }
  }

  void decode(py::bytes const& value) {
    try {
      element_->decode(variable_length_data(value));
    } catch (rti::EncoderException const& error) {
      raise_native_error("DecoderException", error.what());
    } catch (rti::Exception const& error) {
      raise_native_exception(error);
    }
  }

  std::size_t decode_from(py::bytes const& value, std::size_t index) {
    return decode_native_from(*element_, value, index);
  }

  std::size_t get_encoded_length() const {
    try {
      return element_->getEncodedLength();
    } catch (rti::EncoderException const& error) {
      raise_native_error("EncoderException", error.what());
    } catch (rti::Exception const& error) {
      raise_native_exception(error);
    }
  }

  unsigned int get_octet_boundary() const { return element_->getOctetBoundary(); }

  std::unique_ptr<rti::DataElement> clone_data_element() const override {
    return clone_native_element(*element_);
  }

 private:
  std::unique_ptr<rti::DataElement> prototype_;
  std::unique_ptr<rti::HLAfixedArray> element_;
};

class NativeHLAfixedRecord : public Native2010ElementBridge {
 public:
  NativeHLAfixedRecord() : element_(new rti::HLAfixedRecord()) {}

  std::size_t size() const { return element_->size(); }

  void append_element(py::object const& prototype, py::bytes const& encoded) {
    auto value = decoded_native_prototype(prototype, encoded);
    try {
      element_->appendElement(*value);
    } catch (rti::EncoderException const& error) {
      raise_native_error("EncoderException", error.what());
    } catch (rti::Exception const& error) {
      raise_native_exception(error);
    }
  }

  void set_element(
      std::size_t index,
      py::object const& prototype,
      py::bytes const& encoded) {
    auto value = decoded_native_prototype(prototype, encoded);
    try {
      element_->set(index, *value);
    } catch (rti::EncoderException const& error) {
      raise_native_error("EncoderException", error.what());
    } catch (rti::Exception const& error) {
      raise_native_exception(error);
    }
  }

  py::bytes get_element_bytes(std::size_t index) const {
    try {
      return encoded_element(element_->get(index));
    } catch (rti::EncoderException const& error) {
      raise_native_error("EncoderException", error.what());
    } catch (rti::Exception const& error) {
      raise_native_exception(error);
    }
  }

  py::bytes to_byte_array() const {
    try {
      return encoded_element(*element_);
    } catch (rti::EncoderException const& error) {
      raise_native_error("EncoderException", error.what());
    } catch (rti::Exception const& error) {
      raise_native_exception(error);
    }
  }

  void decode(py::bytes const& value) {
    try {
      element_->decode(variable_length_data(value));
    } catch (rti::EncoderException const& error) {
      raise_native_error("DecoderException", error.what());
    } catch (rti::Exception const& error) {
      raise_native_exception(error);
    }
  }

  std::size_t decode_from(py::bytes const& value, std::size_t index) {
    return decode_native_from(*element_, value, index);
  }

  std::size_t get_encoded_length() const {
    try {
      return element_->getEncodedLength();
    } catch (rti::EncoderException const& error) {
      raise_native_error("EncoderException", error.what());
    } catch (rti::Exception const& error) {
      raise_native_exception(error);
    }
  }

  unsigned int get_octet_boundary() const { return element_->getOctetBoundary(); }

  std::unique_ptr<rti::DataElement> clone_data_element() const override {
    return clone_native_element(*element_);
  }

 private:
  std::unique_ptr<rti::HLAfixedRecord> element_;
};

class NativeHLAvariantRecord : public Native2010ElementBridge {
 public:
  explicit NativeHLAvariantRecord(py::object const& discriminant_prototype)
      : discriminant_prototype_(native_prototype(discriminant_prototype)),
        element_(new rti::HLAvariantRecord(*discriminant_prototype_)) {}

  void add_variant(
      py::object const& discriminant_prototype,
      py::bytes const& discriminant_encoded,
      py::object const& value_prototype,
      py::bytes const& value_encoded) {
    auto discriminant = decoded_native_prototype(discriminant_prototype, discriminant_encoded);
    auto value = decoded_native_prototype(value_prototype, value_encoded);
    try {
      element_->addVariant(*discriminant, *value);
    } catch (rti::EncoderException const& error) {
      raise_native_error("EncoderException", error.what());
    } catch (rti::Exception const& error) {
      raise_native_exception(error);
    }
  }

  void set_variant(
      py::object const& discriminant_prototype,
      py::bytes const& discriminant_encoded,
      py::object const& value_prototype,
      py::bytes const& value_encoded) {
    auto discriminant = decoded_native_prototype(discriminant_prototype, discriminant_encoded);
    auto value = decoded_native_prototype(value_prototype, value_encoded);
    try {
      element_->setVariant(*discriminant, *value);
    } catch (rti::EncoderException const& error) {
      raise_native_error("EncoderException", error.what());
    } catch (rti::Exception const& error) {
      raise_native_exception(error);
    }
  }

  void set_discriminant(
      py::object const& discriminant_prototype,
      py::bytes const& discriminant_encoded) {
    auto discriminant = decoded_native_prototype(discriminant_prototype, discriminant_encoded);
    try {
      element_->setDiscriminant(*discriminant);
    } catch (rti::EncoderException const& error) {
      raise_native_error("EncoderException", error.what());
    } catch (rti::Exception const& error) {
      raise_native_exception(error);
    }
  }

  py::bytes get_discriminant_bytes() const {
    try {
      return encoded_element(element_->getDiscriminant());
    } catch (rti::EncoderException const& error) {
      raise_native_error("EncoderException", error.what());
    } catch (rti::Exception const& error) {
      raise_native_exception(error);
    }
  }

  py::bytes get_variant_bytes() const {
    try {
      return encoded_element(element_->getVariant());
    } catch (rti::EncoderException const& error) {
      raise_native_error("EncoderException", error.what());
    } catch (rti::Exception const& error) {
      raise_native_exception(error);
    }
  }

  py::bytes to_byte_array() const {
    try {
      return encoded_element(*element_);
    } catch (rti::EncoderException const& error) {
      raise_native_error("EncoderException", error.what());
    } catch (rti::Exception const& error) {
      raise_native_exception(error);
    }
  }

  void decode(py::bytes const& value) {
    try {
      element_->decode(variable_length_data(value));
    } catch (rti::EncoderException const& error) {
      raise_native_error("DecoderException", error.what());
    } catch (rti::Exception const& error) {
      raise_native_exception(error);
    }
  }

  std::size_t decode_from(py::bytes const& value, std::size_t index) {
    return decode_native_from(*element_, value, index);
  }

  std::size_t get_encoded_length() const {
    try {
      return element_->getEncodedLength();
    } catch (rti::EncoderException const& error) {
      raise_native_error("EncoderException", error.what());
    } catch (rti::Exception const& error) {
      raise_native_exception(error);
    }
  }

  unsigned int get_octet_boundary() const { return element_->getOctetBoundary(); }

  std::unique_ptr<rti::DataElement> clone_data_element() const override {
    return clone_native_element(*element_);
  }

 private:
  std::unique_ptr<rti::DataElement> discriminant_prototype_;
  std::unique_ptr<rti::HLAvariantRecord> element_;
};

class Native2010Integer64Interval {
 public:
  Native2010Integer64Interval() = default;
  explicit Native2010Integer64Interval(std::int64_t value) : element_(value) {}

  std::int64_t get_interval() const { return element_.getInterval(); }
  void set_interval(std::int64_t value) { element_.setInterval(value); }
  bool is_zero() const { return element_.isZero(); }
  bool is_epsilon() const { return element_.isEpsilon(); }
  py::bytes to_byte_array() const { return encoded_interval(element_); }
  std::size_t encoded_length() const { return element_.encodedLength(); }
  void decode(py::bytes const& value) {
    try {
      element_.decode(variable_length_data(value));
    } catch (rti::Exception const& error) {
      raise_native_error("CouldNotDecode", error.what());
    }
  }
  void add_interval(std::int64_t value) {
    try {
      rti::HLAinteger64Interval other(value);
      element_ += other;
    } catch (rti::Exception const& error) {
      raise_native_error("IllegalTimeArithmetic", error.what());
    }
  }
  void subtract_interval(std::int64_t value) {
    try {
      rti::HLAinteger64Interval other(value);
      element_ -= other;
    } catch (rti::Exception const& error) {
      raise_native_error("IllegalTimeArithmetic", error.what());
    }
  }

 private:
  rti::HLAinteger64Interval element_;
};

class Native2010Integer64Time {
 public:
  Native2010Integer64Time() = default;
  explicit Native2010Integer64Time(std::int64_t value) : element_(value) {}

  std::int64_t get_time() const { return element_.getTime(); }
  void set_time(std::int64_t value) { element_.setTime(value); }
  bool is_initial() const { return element_.isInitial(); }
  bool is_final() const { return element_.isFinal(); }
  py::bytes to_byte_array() const { return encoded_time(element_); }
  std::size_t encoded_length() const { return element_.encodedLength(); }
  void decode(py::bytes const& value) {
    try {
      element_.decode(variable_length_data(value));
    } catch (rti::Exception const& error) {
      raise_native_error("CouldNotDecode", error.what());
    }
  }
  void add_interval(std::int64_t value) {
    try {
      rti::HLAinteger64Interval other(value);
      element_ += other;
    } catch (rti::Exception const& error) {
      raise_native_error("IllegalTimeArithmetic", error.what());
    }
  }
  void subtract_interval(std::int64_t value) {
    try {
      rti::HLAinteger64Interval other(value);
      element_ -= other;
    } catch (rti::Exception const& error) {
      raise_native_error("IllegalTimeArithmetic", error.what());
    }
  }
  int compare(std::int64_t value) const {
    rti::HLAinteger64Time other(value);
    return element_.getTime() < other.getTime()
        ? -1
        : element_.getTime() > other.getTime() ? 1 : 0;
  }
  std::int64_t distance(std::int64_t value) const {
    rti::HLAinteger64Time other(value);
    if (other.getTime() > element_.getTime()) {
      raise_native_error("IllegalTimeArithmetic", L"The logical-time distance would be negative.");
    }
    return element_.getTime() - other.getTime();
  }

 private:
  rti::HLAinteger64Time element_;
};

class Native2010Integer64TimeFactory {
 public:
  Native2010Integer64Time make_initial() const { return Native2010Integer64Time(0); }
  Native2010Integer64Time make_final() const {
    return Native2010Integer64Time(std::numeric_limits<std::int64_t>::max());
  }
  Native2010Integer64Interval make_zero() const { return Native2010Integer64Interval(0); }
  Native2010Integer64Interval make_epsilon() const { return Native2010Integer64Interval(1); }
  Native2010Integer64Time make_time(std::int64_t value) const {
    return Native2010Integer64Time(value);
  }
  Native2010Integer64Interval make_interval(std::int64_t value) const {
    return Native2010Integer64Interval(value);
  }
  Native2010Integer64Time decode_time(py::bytes const& value) const {
    Native2010Integer64Time result;
    result.decode(value);
    return result;
  }
  Native2010Integer64Interval decode_interval(py::bytes const& value) const {
    Native2010Integer64Interval result;
    result.decode(value);
    return result;
  }
  std::string name() const { return "HLAinteger64Time"; }
};

class Native2010Float64Interval {
 public:
  Native2010Float64Interval() = default;
  explicit Native2010Float64Interval(double value) : element_(value) {}

  double get_interval() const { return element_.getInterval(); }
  void set_interval(double value) { element_.setInterval(value); }
  bool is_zero() const { return element_.isZero(); }
  bool is_epsilon() const { return element_.isEpsilon(); }
  py::bytes to_byte_array() const { return encoded_interval(element_); }
  std::size_t encoded_length() const { return element_.encodedLength(); }
  void decode(py::bytes const& value) {
    try {
      element_.decode(variable_length_data(value));
    } catch (rti::Exception const& error) {
      raise_native_error("CouldNotDecode", error.what());
    }
  }
  void add_interval(double value) {
    try {
      rti::HLAfloat64Interval other(value);
      element_ += other;
    } catch (rti::Exception const& error) {
      raise_native_error("IllegalTimeArithmetic", error.what());
    }
  }
  void subtract_interval(double value) {
    try {
      rti::HLAfloat64Interval other(value);
      element_ -= other;
    } catch (rti::Exception const& error) {
      raise_native_error("IllegalTimeArithmetic", error.what());
    }
  }

 private:
  rti::HLAfloat64Interval element_;
};

class Native2010Float64Time {
 public:
  Native2010Float64Time() = default;
  explicit Native2010Float64Time(double value) : element_(value) {}

  double get_time() const { return element_.getTime(); }
  void set_time(double value) { element_.setTime(value); }
  bool is_initial() const { return element_.isInitial(); }
  bool is_final() const { return element_.isFinal(); }
  py::bytes to_byte_array() const { return encoded_time(element_); }
  std::size_t encoded_length() const { return element_.encodedLength(); }
  void decode(py::bytes const& value) {
    try {
      element_.decode(variable_length_data(value));
    } catch (rti::Exception const& error) {
      raise_native_error("CouldNotDecode", error.what());
    }
  }
  void add_interval(double value) {
    try {
      rti::HLAfloat64Interval other(value);
      element_ += other;
    } catch (rti::Exception const& error) {
      raise_native_error("IllegalTimeArithmetic", error.what());
    }
  }
  void subtract_interval(double value) {
    try {
      rti::HLAfloat64Interval other(value);
      element_ -= other;
    } catch (rti::Exception const& error) {
      raise_native_error("IllegalTimeArithmetic", error.what());
    }
  }
  int compare(double value) const {
    rti::HLAfloat64Time other(value);
    return element_.getTime() < other.getTime()
        ? -1
        : element_.getTime() > other.getTime() ? 1 : 0;
  }
  double distance(double value) const {
    rti::HLAfloat64Time other(value);
    if (other.getTime() > element_.getTime()) {
      raise_native_error("IllegalTimeArithmetic", L"The logical-time distance would be negative.");
    }
    return element_.getTime() - other.getTime();
  }

 private:
  rti::HLAfloat64Time element_;
};

class Native2010Float64TimeFactory {
 public:
  Native2010Float64Time make_initial() const { return Native2010Float64Time(0.0); }
  Native2010Float64Time make_final() const {
    return Native2010Float64Time((std::numeric_limits<double>::max)());
  }
  Native2010Float64Interval make_zero() const { return Native2010Float64Interval(0.0); }
  Native2010Float64Interval make_epsilon() const {
    return Native2010Float64Interval((std::numeric_limits<double>::denorm_min)());
  }
  Native2010Float64Time make_time(double value) const { return Native2010Float64Time(value); }
  Native2010Float64Interval make_interval(double value) const {
    return Native2010Float64Interval(value);
  }
  Native2010Float64Time decode_time(py::bytes const& value) const {
    Native2010Float64Time result;
    result.decode(value);
    return result;
  }
  Native2010Float64Interval decode_interval(py::bytes const& value) const {
    Native2010Float64Interval result;
    result.decode(value);
    return result;
  }
  std::string name() const { return "HLAfloat64Time"; }
};

class Python2010FederateAmbassador final : public rti::NullFederateAmbassador {
 public:
  explicit Python2010FederateAmbassador(py::object callback_target)
      : callback_target_(std::move(callback_target)) {}

  void discoverObjectInstance(
      rti::ObjectInstanceHandle object,
      rti::ObjectClassHandle object_class,
      std::wstring const& object_name,
      rti::FederateHandle producing_federate) throw(rti::FederateInternalError) override {
    py::gil_scoped_acquire gil;
    try {
      callback_target_.attr("discoverObjectInstance")(
          encoded_handle(object), encoded_handle(object_class), object_name,
          encoded_handle(producing_federate));
    } catch (py::error_already_set const&) {
      throw rti::FederateInternalError(L"Python discoverObjectInstance callback failed.");
    }
  }

  void synchronizationPointRegistrationSucceeded(std::wstring const& label)
      throw(rti::FederateInternalError) override {
    py::gil_scoped_acquire gil;
    try {
      callback_target_.attr("synchronizationPointRegistrationSucceeded")(label);
    } catch (py::error_already_set const&) {
      throw rti::FederateInternalError(
          L"Python synchronizationPointRegistrationSucceeded callback failed.");
    }
  }

  void announceSynchronizationPoint(
      std::wstring const& label,
      rti::VariableLengthData const& tag) throw(rti::FederateInternalError) override {
    py::gil_scoped_acquire gil;
    try {
      callback_target_.attr("announceSynchronizationPoint")(
          label, py::bytes(static_cast<char const*>(tag.data()), tag.size()));
    } catch (py::error_already_set const&) {
      throw rti::FederateInternalError(
          L"Python announceSynchronizationPoint callback failed.");
    }
  }

  void federationSynchronized(
      std::wstring const& label,
      rti::FederateHandleSet const& failed_to_sync) throw(rti::FederateInternalError) override {
    py::gil_scoped_acquire gil;
    try {
      py::set encoded_failed;
      for (auto const& handle : failed_to_sync) {
        encoded_failed.add(encoded_handle(handle));
      }
      callback_target_.attr("federationSynchronized")(label, encoded_failed);
    } catch (py::error_already_set const&) {
      throw rti::FederateInternalError(
          L"Python federationSynchronized callback failed.");
    }
  }

  void reflectAttributeValues(
      rti::ObjectInstanceHandle object,
      rti::AttributeHandleValueMap const& values,
      rti::VariableLengthData const& tag,
      rti::OrderType,
      rti::TransportationType,
      rti::SupplementalReflectInfo info) throw(rti::FederateInternalError) override {
    py::gil_scoped_acquire gil;
    try {
      py::dict encoded_values;
      for (auto const& entry : values) {
        encoded_values[encoded_handle(entry.first)] = py::bytes(
            static_cast<char const*>(entry.second.data()), entry.second.size());
      }
      py::bytes encoded_tag(static_cast<char const*>(tag.data()), tag.size());
      py::bytes encoded_producer = info.hasProducingFederate
          ? encoded_handle(info.producingFederate)
          : py::bytes();
      callback_target_.attr("reflectAttributeValues")(
          encoded_handle(object), encoded_values, encoded_tag, encoded_producer);
    } catch (py::error_already_set const&) {
      throw rti::FederateInternalError(L"Python reflectAttributeValues callback failed.");
    }
  }

  void receiveInteraction(
      rti::InteractionClassHandle interaction,
      rti::ParameterHandleValueMap const& values,
      rti::VariableLengthData const& tag,
      rti::OrderType,
      rti::TransportationType,
      rti::SupplementalReceiveInfo info) throw(rti::FederateInternalError) override {
    py::gil_scoped_acquire gil;
    try {
      py::dict encoded_values;
      for (auto const& entry : values) {
        encoded_values[encoded_handle(entry.first)] = py::bytes(
            static_cast<char const*>(entry.second.data()), entry.second.size());
      }
      py::bytes encoded_tag(static_cast<char const*>(tag.data()), tag.size());
      py::bytes encoded_producer = info.hasProducingFederate
          ? encoded_handle(info.producingFederate)
          : py::bytes();
      callback_target_.attr("receiveInteraction")(
          encoded_handle(interaction), encoded_values, encoded_tag, encoded_producer);
    } catch (py::error_already_set const&) {
      throw rti::FederateInternalError(L"Python receiveInteraction callback failed.");
    }
  }

  void informAttributeOwnership(
      rti::ObjectInstanceHandle object,
      rti::AttributeHandle attribute,
      rti::FederateHandle owner) throw(rti::FederateInternalError) override {
    py::gil_scoped_acquire gil;
    try {
      callback_target_.attr("informAttributeOwnership")(
          encoded_handle(object), encoded_handle(attribute), encoded_handle(owner));
    } catch (py::error_already_set const&) {
      throw rti::FederateInternalError(L"Python informAttributeOwnership callback failed.");
    }
  }

 private:
  py::object callback_target_;
};

class Native2010Ambassador {
 public:
  Native2010Ambassador() {
    rti::RTIambassadorFactory factory;
    ambassador_ = factory.createRTIambassador();
  }

  void connect(py::object callback_target, std::string const& callback_model,
               std::wstring const& local_settings_designator) {
    rti::CallbackModel model;
    if (callback_model == "HLA_EVOKED" || callback_model == "evoked") {
      model = rti::HLA_EVOKED;
    } else if (callback_model == "HLA_IMMEDIATE" || callback_model == "immediate") {
      model = rti::HLA_IMMEDIATE;
    } else {
      raise_native_error(
          "UnsupportedCallbackModel",
          L"Umbra IEEE 1516.1-2010 native binding received an unknown callback model.");
    }
    try {
      federate_ambassador_ = std::make_unique<Python2010FederateAmbassador>(
          std::move(callback_target));
      ambassador_->connect(*federate_ambassador_, model,
                           local_settings_designator);
    } catch (rti::AlreadyConnected const& error) {
      raise_native_error("AlreadyConnected", error.what());
    } catch (rti::Exception const& error) {
      raise_native_exception(error);
    }
  }

  void disconnect() {
    try {
      ambassador_->disconnect();
    } catch (rti::Exception const& error) {
      raise_native_exception(error);
    }
    federate_ambassador_.reset();
  }

  void create_federation_execution(std::wstring const& federation) {
    try {
      ambassador_->createFederationExecution(federation, L"Umbra-2010-reference-fom");
    } catch (rti::FederationExecutionAlreadyExists const& error) {
      raise_native_error("FederationExecutionAlreadyExists", error.what());
    } catch (rti::Exception const& error) {
      raise_native_exception(error);
    }
  }

  void destroy_federation_execution(std::wstring const& federation) {
    try {
      ambassador_->destroyFederationExecution(federation);
    } catch (rti::FederationExecutionDoesNotExist const& error) {
      raise_native_error("FederationExecutionDoesNotExist", error.what());
    } catch (rti::FederatesCurrentlyJoined const& error) {
      raise_native_error("FederatesCurrentlyJoined", error.what());
    } catch (rti::Exception const& error) {
      raise_native_exception(error);
    }
  }

  void register_federation_synchronization_point(
      std::wstring const& label, py::bytes const& tag) {
    try {
      ambassador_->registerFederationSynchronizationPoint(label, variable_length_data(tag));
    } catch (rti::Exception const& error) { raise_native_exception(error); }
  }

  void synchronization_point_achieved(
      std::wstring const& label, bool successfully = true) {
    try {
      ambassador_->synchronizationPointAchieved(label, successfully);
    } catch (rti::Exception const& error) { raise_native_exception(error); }
  }

  py::bytes join_federation_execution(
      std::wstring const& federate_name,
      std::wstring const& federate_type,
      std::wstring const& federation) {
    try {
      return encoded_handle(ambassador_->joinFederationExecution(
          federate_name, federate_type, federation));
    } catch (rti::FederationExecutionDoesNotExist const& error) {
      raise_native_error("FederationExecutionDoesNotExist", error.what());
    } catch (rti::FederateNameAlreadyInUse const& error) {
      raise_native_error("FederateNameAlreadyInUse", error.what());
    } catch (rti::Exception const& error) {
      raise_native_exception(error);
    }
  }

  void resign_federation_execution(std::string const& action) {
    rti::ResignAction resign_action = rti::NO_ACTION;
    if (action == "DELETE_OBJECTS") resign_action = rti::DELETE_OBJECTS;
    try {
      ambassador_->resignFederationExecution(resign_action);
    } catch (rti::Exception const& error) {
      raise_native_exception(error);
    }
  }

  py::bytes get_federate_handle(std::wstring const& name) {
    try { return encoded_handle(ambassador_->getFederateHandle(name)); }
    catch (rti::Exception const& error) { raise_native_exception(error); }
  }

  std::wstring get_federate_name(py::bytes const& encoded) {
    try {
      auto handle = decode_handle(encoded, rti::umbra_binding_detail::decodeFederateHandle);
      return ambassador_->getFederateName(handle);
    } catch (rti::Exception const& error) { raise_native_exception(error); }
  }

  py::bytes get_object_class_handle(std::wstring const& name) {
    try { return encoded_handle(ambassador_->getObjectClassHandle(name)); }
    catch (rti::Exception const& error) { raise_native_exception(error); }
  }

  std::wstring get_object_class_name(py::bytes const& encoded) {
    try {
      auto handle = decode_handle(encoded, rti::umbra_binding_detail::decodeObjectClassHandle);
      return ambassador_->getObjectClassName(handle);
    } catch (rti::Exception const& error) { raise_native_exception(error); }
  }

  py::bytes get_attribute_handle(py::bytes const& class_encoded, std::wstring const& name) {
    try {
      auto object_class = decode_handle(class_encoded, rti::umbra_binding_detail::decodeObjectClassHandle);
      return encoded_handle(ambassador_->getAttributeHandle(object_class, name));
    } catch (rti::Exception const& error) { raise_native_exception(error); }
  }

  std::wstring get_attribute_name(py::bytes const& class_encoded, py::bytes const& attribute_encoded) {
    try {
      auto object_class = decode_handle(class_encoded, rti::umbra_binding_detail::decodeObjectClassHandle);
      auto attribute = decode_handle(attribute_encoded, rti::umbra_binding_detail::decodeAttributeHandle);
      return ambassador_->getAttributeName(object_class, attribute);
    } catch (rti::Exception const& error) { raise_native_exception(error); }
  }

  py::bytes get_interaction_class_handle(std::wstring const& name) {
    try { return encoded_handle(ambassador_->getInteractionClassHandle(name)); }
    catch (rti::Exception const& error) { raise_native_exception(error); }
  }

  std::wstring get_interaction_class_name(py::bytes const& encoded) {
    try {
      auto handle = decode_handle(encoded, rti::umbra_binding_detail::decodeInteractionClassHandle);
      return ambassador_->getInteractionClassName(handle);
    } catch (rti::Exception const& error) { raise_native_exception(error); }
  }

  py::bytes get_parameter_handle(
      py::bytes const& class_encoded, std::wstring const& name) {
    try {
      auto interaction = decode_handle(
          class_encoded, rti::umbra_binding_detail::decodeInteractionClassHandle);
      return encoded_handle(ambassador_->getParameterHandle(interaction, name));
    } catch (rti::Exception const& error) { raise_native_exception(error); }
  }

  std::wstring get_parameter_name(
      py::bytes const& class_encoded, py::bytes const& parameter_encoded) {
    try {
      auto interaction = decode_handle(
          class_encoded, rti::umbra_binding_detail::decodeInteractionClassHandle);
      auto parameter = decode_handle(
          parameter_encoded, rti::umbra_binding_detail::decodeParameterHandle);
      return ambassador_->getParameterName(interaction, parameter);
    } catch (rti::Exception const& error) { raise_native_exception(error); }
  }

  void publish_interaction_class(py::bytes const& class_encoded) {
    try {
      auto interaction = decode_handle(
          class_encoded, rti::umbra_binding_detail::decodeInteractionClassHandle);
      ambassador_->publishInteractionClass(interaction);
    } catch (rti::Exception const& error) { raise_native_exception(error); }
  }

  void unpublish_interaction_class(py::bytes const& class_encoded) {
    try {
      auto interaction = decode_handle(
          class_encoded, rti::umbra_binding_detail::decodeInteractionClassHandle);
      ambassador_->unpublishInteractionClass(interaction);
    } catch (rti::Exception const& error) { raise_native_exception(error); }
  }

  void subscribe_interaction_class(py::bytes const& class_encoded) {
    try {
      auto interaction = decode_handle(
          class_encoded, rti::umbra_binding_detail::decodeInteractionClassHandle);
      ambassador_->subscribeInteractionClass(interaction);
    } catch (rti::Exception const& error) { raise_native_exception(error); }
  }

  void unsubscribe_interaction_class(py::bytes const& class_encoded) {
    try {
      auto interaction = decode_handle(
          class_encoded, rti::umbra_binding_detail::decodeInteractionClassHandle);
      ambassador_->unsubscribeInteractionClass(interaction);
    } catch (rti::Exception const& error) { raise_native_exception(error); }
  }

  void publish_object_class_attributes(py::bytes const& class_encoded, py::set const& attributes) {
    try {
      auto object_class = decode_handle(class_encoded, rti::umbra_binding_detail::decodeObjectClassHandle);
      rti::AttributeHandleSet decoded;
      for (py::handle value : attributes) decoded.insert(decode_handle(value.cast<py::bytes>(), rti::umbra_binding_detail::decodeAttributeHandle));
      ambassador_->publishObjectClassAttributes(object_class, decoded);
    } catch (rti::Exception const& error) { raise_native_exception(error); }
  }

  void unpublish_object_class_attributes(py::bytes const& class_encoded, py::set const& attributes) {
    try {
      auto object_class = decode_handle(class_encoded, rti::umbra_binding_detail::decodeObjectClassHandle);
      rti::AttributeHandleSet decoded;
      for (py::handle value : attributes) decoded.insert(decode_handle(value.cast<py::bytes>(), rti::umbra_binding_detail::decodeAttributeHandle));
      ambassador_->unpublishObjectClassAttributes(object_class, decoded);
    } catch (rti::Exception const& error) { raise_native_exception(error); }
  }

  void subscribe_object_class_attributes(py::bytes const& class_encoded, py::set const& attributes) {
    try {
      auto object_class = decode_handle(class_encoded, rti::umbra_binding_detail::decodeObjectClassHandle);
      rti::AttributeHandleSet decoded;
      for (py::handle value : attributes) decoded.insert(decode_handle(value.cast<py::bytes>(), rti::umbra_binding_detail::decodeAttributeHandle));
      ambassador_->subscribeObjectClassAttributes(object_class, decoded);
    } catch (rti::Exception const& error) { raise_native_exception(error); }
  }

  void unsubscribe_object_class_attributes(py::bytes const& class_encoded, py::set const& attributes) {
    try {
      auto object_class = decode_handle(class_encoded, rti::umbra_binding_detail::decodeObjectClassHandle);
      rti::AttributeHandleSet decoded;
      for (py::handle value : attributes) decoded.insert(decode_handle(value.cast<py::bytes>(), rti::umbra_binding_detail::decodeAttributeHandle));
      ambassador_->unsubscribeObjectClassAttributes(object_class, decoded);
    } catch (rti::Exception const& error) { raise_native_exception(error); }
  }

  py::bytes register_object_instance(py::bytes const& class_encoded, py::object const& name) {
    try {
      auto object_class = decode_handle(class_encoded, rti::umbra_binding_detail::decodeObjectClassHandle);
      rti::ObjectInstanceHandle object = name.is_none()
          ? ambassador_->registerObjectInstance(object_class)
          : ambassador_->registerObjectInstance(object_class, name.cast<std::wstring>());
      return encoded_handle(object);
    } catch (rti::Exception const& error) { raise_native_exception(error); }
  }

  std::wstring get_object_instance_name(py::bytes const& object_encoded) {
    try {
      auto object = decode_handle(object_encoded, rti::umbra_binding_detail::decodeObjectInstanceHandle);
      return ambassador_->getObjectInstanceName(object);
    } catch (rti::Exception const& error) { raise_native_exception(error); }
  }

  void update_attribute_values(
      py::bytes const& object_encoded, py::dict const& values, py::bytes const& tag) {
    try {
      auto object = decode_handle(object_encoded, rti::umbra_binding_detail::decodeObjectInstanceHandle);
      rti::AttributeHandleValueMap decoded;
      for (auto const& entry : values) {
        auto attribute = decode_handle(entry.first.cast<py::bytes>(), rti::umbra_binding_detail::decodeAttributeHandle);
        decoded.emplace(attribute, variable_length_data(entry.second.cast<py::bytes>()));
      }
      ambassador_->updateAttributeValues(object, decoded, variable_length_data(tag));
    } catch (rti::Exception const& error) { raise_native_exception(error); }
  }

  void send_interaction(
      py::bytes const& class_encoded, py::dict const& values, py::bytes const& tag) {
    try {
      auto interaction = decode_handle(
          class_encoded, rti::umbra_binding_detail::decodeInteractionClassHandle);
      rti::ParameterHandleValueMap decoded;
      for (auto const& entry : values) {
        auto parameter = decode_handle(
            entry.first.cast<py::bytes>(), rti::umbra_binding_detail::decodeParameterHandle);
        decoded.emplace(parameter, variable_length_data(entry.second.cast<py::bytes>()));
      }
      ambassador_->sendInteraction(interaction, decoded, variable_length_data(tag));
    } catch (rti::Exception const& error) { raise_native_exception(error); }
  }

  void query_attribute_ownership(
      py::bytes const& object_encoded, py::bytes const& attribute_encoded) {
    try {
      auto object = decode_handle(object_encoded, rti::umbra_binding_detail::decodeObjectInstanceHandle);
      auto attribute = decode_handle(attribute_encoded, rti::umbra_binding_detail::decodeAttributeHandle);
      ambassador_->queryAttributeOwnership(object, attribute);
    } catch (rti::Exception const& error) { raise_native_exception(error); }
  }

  bool is_attribute_owned_by_federate(
      py::bytes const& object_encoded, py::bytes const& attribute_encoded) {
    try {
      auto object = decode_handle(object_encoded, rti::umbra_binding_detail::decodeObjectInstanceHandle);
      auto attribute = decode_handle(attribute_encoded, rti::umbra_binding_detail::decodeAttributeHandle);
      return ambassador_->isAttributeOwnedByFederate(object, attribute);
    } catch (rti::Exception const& error) { raise_native_exception(error); }
  }

 private:
  std::auto_ptr<rti::RTIambassador> ambassador_;
  std::unique_ptr<Python2010FederateAmbassador> federate_ambassador_;
};

}  // namespace

PYBIND11_MODULE(_native_2010, module) {
  native_rti_error_type = PyErr_NewException(
      "umbra._native.rti1516e._native_2010.Native2010RtiError",
      PyExc_RuntimeError, nullptr);
  module.add_object("Native2010RtiError",
                    py::reinterpret_steal<py::object>(native_rti_error_type));

  py::class_<Native2010Ambassador>(module, "Native2010Ambassador")
      .def(py::init<>())
      .def("connect", &Native2010Ambassador::connect,
           py::arg("federate_ambassador"), py::arg("callback_model"),
           py::arg("local_settings_designator") = L"")
      .def("disconnect", &Native2010Ambassador::disconnect)
      .def("create_federation_execution", &Native2010Ambassador::create_federation_execution)
      .def("destroy_federation_execution", &Native2010Ambassador::destroy_federation_execution)
      .def("register_federation_synchronization_point",
           &Native2010Ambassador::register_federation_synchronization_point)
      .def("synchronization_point_achieved",
           &Native2010Ambassador::synchronization_point_achieved,
           py::arg("label"), py::arg("successfully") = true)
      .def("join_federation_execution", &Native2010Ambassador::join_federation_execution)
      .def("resign_federation_execution", &Native2010Ambassador::resign_federation_execution)
      .def("get_federate_handle", &Native2010Ambassador::get_federate_handle)
      .def("get_federate_name", &Native2010Ambassador::get_federate_name)
      .def("get_object_class_handle", &Native2010Ambassador::get_object_class_handle)
      .def("get_object_class_name", &Native2010Ambassador::get_object_class_name)
      .def("get_attribute_handle", &Native2010Ambassador::get_attribute_handle)
      .def("get_attribute_name", &Native2010Ambassador::get_attribute_name)
      .def("get_interaction_class_handle", &Native2010Ambassador::get_interaction_class_handle)
      .def("get_interaction_class_name", &Native2010Ambassador::get_interaction_class_name)
      .def("get_parameter_handle", &Native2010Ambassador::get_parameter_handle)
      .def("get_parameter_name", &Native2010Ambassador::get_parameter_name)
      .def("publish_object_class_attributes", &Native2010Ambassador::publish_object_class_attributes)
      .def("unpublish_object_class_attributes", &Native2010Ambassador::unpublish_object_class_attributes)
      .def("subscribe_object_class_attributes", &Native2010Ambassador::subscribe_object_class_attributes)
      .def("unsubscribe_object_class_attributes", &Native2010Ambassador::unsubscribe_object_class_attributes)
      .def("publish_interaction_class", &Native2010Ambassador::publish_interaction_class)
      .def("unpublish_interaction_class", &Native2010Ambassador::unpublish_interaction_class)
      .def("subscribe_interaction_class", &Native2010Ambassador::subscribe_interaction_class)
      .def("unsubscribe_interaction_class", &Native2010Ambassador::unsubscribe_interaction_class)
      .def("register_object_instance", &Native2010Ambassador::register_object_instance,
           py::arg("object_class"), py::arg("name") = py::none())
      .def("get_object_instance_name", &Native2010Ambassador::get_object_instance_name)
      .def("update_attribute_values", &Native2010Ambassador::update_attribute_values)
      .def("send_interaction", &Native2010Ambassador::send_interaction)
      .def("query_attribute_ownership", &Native2010Ambassador::query_attribute_ownership)
      .def("is_attribute_owned_by_federate", &Native2010Ambassador::is_attribute_owned_by_federate);

  py::class_<Native2010Integer64Interval>(module, "Native2010Integer64Interval")
      .def(py::init<>())
      .def(py::init<std::int64_t>())
      .def("get_interval", &Native2010Integer64Interval::get_interval)
      .def("set_interval", &Native2010Integer64Interval::set_interval)
      .def("is_zero", &Native2010Integer64Interval::is_zero)
      .def("is_epsilon", &Native2010Integer64Interval::is_epsilon)
      .def("to_byte_array", &Native2010Integer64Interval::to_byte_array)
      .def("encoded_length", &Native2010Integer64Interval::encoded_length)
      .def("decode", &Native2010Integer64Interval::decode)
      .def("add_interval", &Native2010Integer64Interval::add_interval)
      .def("subtract_interval", &Native2010Integer64Interval::subtract_interval);

  py::class_<Native2010Integer64Time>(module, "Native2010Integer64Time")
      .def(py::init<>())
      .def(py::init<std::int64_t>())
      .def("get_time", &Native2010Integer64Time::get_time)
      .def("set_time", &Native2010Integer64Time::set_time)
      .def("is_initial", &Native2010Integer64Time::is_initial)
      .def("is_final", &Native2010Integer64Time::is_final)
      .def("to_byte_array", &Native2010Integer64Time::to_byte_array)
      .def("encoded_length", &Native2010Integer64Time::encoded_length)
      .def("decode", &Native2010Integer64Time::decode)
      .def("add_interval", &Native2010Integer64Time::add_interval)
      .def("subtract_interval", &Native2010Integer64Time::subtract_interval)
      .def("compare", &Native2010Integer64Time::compare)
      .def("distance", &Native2010Integer64Time::distance);

  py::class_<Native2010Integer64TimeFactory>(module, "Native2010Integer64TimeFactory")
      .def(py::init<>())
      .def("make_initial", &Native2010Integer64TimeFactory::make_initial)
      .def("make_final", &Native2010Integer64TimeFactory::make_final)
      .def("make_zero", &Native2010Integer64TimeFactory::make_zero)
      .def("make_epsilon", &Native2010Integer64TimeFactory::make_epsilon)
      .def("make_time", &Native2010Integer64TimeFactory::make_time)
      .def("make_interval", &Native2010Integer64TimeFactory::make_interval)
      .def("decode_time", &Native2010Integer64TimeFactory::decode_time)
      .def("decode_interval", &Native2010Integer64TimeFactory::decode_interval)
      .def("name", &Native2010Integer64TimeFactory::name);

  py::class_<Native2010Float64Interval>(module, "Native2010Float64Interval")
      .def(py::init<>())
      .def(py::init<double>())
      .def("get_interval", &Native2010Float64Interval::get_interval)
      .def("set_interval", &Native2010Float64Interval::set_interval)
      .def("is_zero", &Native2010Float64Interval::is_zero)
      .def("is_epsilon", &Native2010Float64Interval::is_epsilon)
      .def("to_byte_array", &Native2010Float64Interval::to_byte_array)
      .def("encoded_length", &Native2010Float64Interval::encoded_length)
      .def("decode", &Native2010Float64Interval::decode)
      .def("add_interval", &Native2010Float64Interval::add_interval)
      .def("subtract_interval", &Native2010Float64Interval::subtract_interval);

  py::class_<Native2010Float64Time>(module, "Native2010Float64Time")
      .def(py::init<>())
      .def(py::init<double>())
      .def("get_time", &Native2010Float64Time::get_time)
      .def("set_time", &Native2010Float64Time::set_time)
      .def("is_initial", &Native2010Float64Time::is_initial)
      .def("is_final", &Native2010Float64Time::is_final)
      .def("to_byte_array", &Native2010Float64Time::to_byte_array)
      .def("encoded_length", &Native2010Float64Time::encoded_length)
      .def("decode", &Native2010Float64Time::decode)
      .def("add_interval", &Native2010Float64Time::add_interval)
      .def("subtract_interval", &Native2010Float64Time::subtract_interval)
      .def("compare", &Native2010Float64Time::compare)
      .def("distance", &Native2010Float64Time::distance);

  py::class_<Native2010Float64TimeFactory>(module, "Native2010Float64TimeFactory")
      .def(py::init<>())
      .def("make_initial", &Native2010Float64TimeFactory::make_initial)
      .def("make_final", &Native2010Float64TimeFactory::make_final)
      .def("make_zero", &Native2010Float64TimeFactory::make_zero)
      .def("make_epsilon", &Native2010Float64TimeFactory::make_epsilon)
      .def("make_time", &Native2010Float64TimeFactory::make_time)
      .def("make_interval", &Native2010Float64TimeFactory::make_interval)
      .def("decode_time", &Native2010Float64TimeFactory::decode_time)
      .def("decode_interval", &Native2010Float64TimeFactory::decode_interval)
      .def("name", &Native2010Float64TimeFactory::name);

  py::class_<Native2010ElementBridge>(module, "_Native2010ElementBridge");

  py::class_<NativeHLAopaqueData, Native2010ElementBridge>(module, "NativeHLAopaqueData")
      .def(py::init<>())
      .def(py::init<py::bytes>())
      .def("get_value", &NativeHLAopaqueData::get_value)
      .def("set_value", &NativeHLAopaqueData::set_value)
      .def("to_byte_array", &NativeHLAopaqueData::to_byte_array)
      .def("decode", &NativeHLAopaqueData::decode)
      .def("decode_from", &NativeHLAopaqueData::decode_from)
      .def("get_encoded_length", &NativeHLAopaqueData::get_encoded_length)
      .def("get_octet_boundary", &NativeHLAopaqueData::get_octet_boundary)
      .def("data_length", &NativeHLAopaqueData::data_length)
      .def("buffer_length", &NativeHLAopaqueData::buffer_length);

  py::class_<NativeHLAvariableArray, Native2010ElementBridge>(module, "NativeHLAvariableArray")
      .def(py::init<py::object>())
      .def("size", &NativeHLAvariableArray::size)
      .def("add_element", &NativeHLAvariableArray::add_element)
      .def("set_element", &NativeHLAvariableArray::set_element)
      .def("get_element_bytes", &NativeHLAvariableArray::get_element_bytes)
      .def("to_byte_array", &NativeHLAvariableArray::to_byte_array)
      .def("decode", &NativeHLAvariableArray::decode)
      .def("decode_from", &NativeHLAvariableArray::decode_from)
      .def("get_encoded_length", &NativeHLAvariableArray::get_encoded_length)
      .def("get_octet_boundary", &NativeHLAvariableArray::get_octet_boundary);

  py::class_<NativeHLAfixedArray, Native2010ElementBridge>(module, "NativeHLAfixedArray")
      .def(py::init<py::object, std::size_t>())
      .def("size", &NativeHLAfixedArray::size)
      .def("set_element", &NativeHLAfixedArray::set_element)
      .def("get_element_bytes", &NativeHLAfixedArray::get_element_bytes)
      .def("to_byte_array", &NativeHLAfixedArray::to_byte_array)
      .def("decode", &NativeHLAfixedArray::decode)
      .def("decode_from", &NativeHLAfixedArray::decode_from)
      .def("get_encoded_length", &NativeHLAfixedArray::get_encoded_length)
      .def("get_octet_boundary", &NativeHLAfixedArray::get_octet_boundary);

  py::class_<NativeHLAfixedRecord, Native2010ElementBridge>(module, "NativeHLAfixedRecord")
      .def(py::init<>())
      .def("size", &NativeHLAfixedRecord::size)
      .def("append_element", &NativeHLAfixedRecord::append_element)
      .def("set_element", &NativeHLAfixedRecord::set_element)
      .def("get_element_bytes", &NativeHLAfixedRecord::get_element_bytes)
      .def("to_byte_array", &NativeHLAfixedRecord::to_byte_array)
      .def("decode", &NativeHLAfixedRecord::decode)
      .def("decode_from", &NativeHLAfixedRecord::decode_from)
      .def("get_encoded_length", &NativeHLAfixedRecord::get_encoded_length)
      .def("get_octet_boundary", &NativeHLAfixedRecord::get_octet_boundary);

  py::class_<NativeHLAvariantRecord, Native2010ElementBridge>(module, "NativeHLAvariantRecord")
      .def(py::init<py::object>())
      .def("add_variant", &NativeHLAvariantRecord::add_variant)
      .def("set_variant", &NativeHLAvariantRecord::set_variant)
      .def("set_discriminant", &NativeHLAvariantRecord::set_discriminant)
      .def("get_discriminant_bytes", &NativeHLAvariantRecord::get_discriminant_bytes)
      .def("get_variant_bytes", &NativeHLAvariantRecord::get_variant_bytes)
      .def("to_byte_array", &NativeHLAvariantRecord::to_byte_array)
      .def("decode", &NativeHLAvariantRecord::decode)
      .def("decode_from", &NativeHLAvariantRecord::decode_from)
      .def("get_encoded_length", &NativeHLAvariantRecord::get_encoded_length)
      .def("get_octet_boundary", &NativeHLAvariantRecord::get_octet_boundary);

#define UMBRA_2010_BIND_NATIVE_VALUE(NAME, PYTYPE) \
  py::class_<NAME, Native2010ElementBridge>(module, #NAME) \
      .def(py::init<>()) \
      .def(py::init<PYTYPE>()) \
      .def("get_value", &NAME::get_value) \
      .def("set_value", &NAME::set_value) \
      .def("to_byte_array", &NAME::to_byte_array) \
      .def("decode", &NAME::decode) \
      .def("decode_from", &NAME::decode_from) \
      .def("get_encoded_length", &NAME::get_encoded_length) \
      .def("get_octet_boundary", &NAME::get_octet_boundary);

  UMBRA_2010_BIND_NATIVE_VALUE(NativeHLAinteger16BE, std::int16_t)
  UMBRA_2010_BIND_NATIVE_VALUE(NativeHLAinteger16LE, std::int16_t)
  UMBRA_2010_BIND_NATIVE_VALUE(NativeHLAinteger32BE, std::int32_t)
  UMBRA_2010_BIND_NATIVE_VALUE(NativeHLAinteger32LE, std::int32_t)
  UMBRA_2010_BIND_NATIVE_VALUE(NativeHLAinteger64BE, std::int64_t)
  UMBRA_2010_BIND_NATIVE_VALUE(NativeHLAinteger64LE, std::int64_t)
  UMBRA_2010_BIND_NATIVE_VALUE(NativeHLAfloat32BE, float)
  UMBRA_2010_BIND_NATIVE_VALUE(NativeHLAfloat32LE, float)
  UMBRA_2010_BIND_NATIVE_VALUE(NativeHLAfloat64BE, double)
  UMBRA_2010_BIND_NATIVE_VALUE(NativeHLAfloat64LE, double)
  UMBRA_2010_BIND_NATIVE_VALUE(NativeHLAbyte, std::uint8_t)
  UMBRA_2010_BIND_NATIVE_VALUE(NativeHLAoctet, std::uint8_t)
  UMBRA_2010_BIND_NATIVE_VALUE(NativeHLAASCIIchar, int)
  UMBRA_2010_BIND_NATIVE_VALUE(NativeHLAunicodeChar, int)
  UMBRA_2010_BIND_NATIVE_VALUE(NativeHLAoctetPairBE, std::uint16_t)
  UMBRA_2010_BIND_NATIVE_VALUE(NativeHLAoctetPairLE, std::uint16_t)
  UMBRA_2010_BIND_NATIVE_VALUE(NativeHLAboolean, bool)
  UMBRA_2010_BIND_NATIVE_VALUE(NativeHLAASCIIstring, std::string)
  UMBRA_2010_BIND_NATIVE_VALUE(NativeHLAunicodeString, std::wstring)

#undef UMBRA_2010_BIND_NATIVE_VALUE

  module.def("rti_name", [] { return narrow(rti::rtiName()); });
  module.def("rti_version", [] { return narrow(rti::rtiVersion()); });
  module.def(
      "raise_standard_exception",
      [](std::string const& name, std::string const& message) {
        if (name == "RTIexception") {
          raise_native_error("RTIexception", widen(message));
        }
        try {
          // The C++ 1516.1 API has one encoding exception type; the Java and
          // Python surfaces distinguish encode and decode failures.
          if (name == "DecoderException") {
            throw rti::EncoderException(widen(message));
          }
          umbra::rti1516e_2010::throwNamedException(name, widen(message));
        } catch (rti::EncoderException const& error) {
          if (name == "DecoderException") {
            raise_native_error("DecoderException", error.what());
          }
          raise_native_exception(error);
        } catch (rti::Exception const& error) {
          raise_native_exception(error);
        }
      },
      py::arg("name"), py::arg("message") = "native exception probe");
}
