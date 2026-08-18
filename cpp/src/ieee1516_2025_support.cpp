#include <RTI/RTI1516.h>

#include <cstring>
#include <ostream>
#include <utility>
#include <vector>

namespace rti1516_2025 {

class VariableLengthDataImplementation {
 public:
  VariableLengthDataImplementation() = default;

  VariableLengthDataImplementation(VariableLengthDataImplementation const& other) {
    copyFrom(other.data(), other.size());
  }

  VariableLengthDataImplementation& operator=(VariableLengthDataImplementation const& other) {
    if (this != &other) {
      copyFrom(other.data(), other.size());
    }
    return *this;
  }

  ~VariableLengthDataImplementation() {
    releaseExternalData();
  }

  void const* data() const noexcept {
    if (storage_ == Storage::internal) {
      return internal_.empty() ? nullptr : internal_.data();
    }
    return externalData_;
  }

  size_t size() const noexcept {
    return storage_ == Storage::internal ? internal_.size() : externalSize_;
  }

  void setData(void const* source, size_t size) {
    releaseExternalData();
    storage_ = Storage::internal;
    internal_.clear();
    if (source == nullptr || size == 0) {
      return;
    }
    internal_.resize(size);
    std::memcpy(internal_.data(), source, size);
  }

  void setDataPointer(void* source, size_t size) {
    releaseExternalData();
    internal_.clear();
    storage_ = Storage::borrowed;
    externalData_ = source;
    externalSize_ = size;
  }

  void takeDataPointer(
      void* source,
      size_t size,
      VariableLengthDataDeleteFunction deleteFunction) {
    releaseExternalData();
    internal_.clear();
    storage_ = Storage::owned;
    externalData_ = source;
    externalSize_ = size;
    deleteFunction_ = deleteFunction;
  }

 private:
  enum class Storage { internal, borrowed, owned };

  void copyFrom(void const* source, size_t size) {
    setData(source, size);
  }

  void releaseExternalData() noexcept {
    if (storage_ == Storage::owned && externalData_ != nullptr) {
      if (deleteFunction_ != nullptr) {
        deleteFunction_(externalData_);
      } else {
        delete[] static_cast<char*>(externalData_);
      }
    }
    externalData_ = nullptr;
    externalSize_ = 0;
    deleteFunction_ = nullptr;
    if (storage_ != Storage::internal) {
      storage_ = Storage::internal;
    }
  }

  Storage storage_ = Storage::internal;
  std::vector<unsigned char> internal_;
  void* externalData_ = nullptr;
  size_t externalSize_ = 0;
  VariableLengthDataDeleteFunction deleteFunction_ = nullptr;
};

VariableLengthData::VariableLengthData() : _impl(new VariableLengthDataImplementation()) {}

VariableLengthData::VariableLengthData(void const* inData, size_t inSize)
    : _impl(new VariableLengthDataImplementation()) {
  _impl->setData(inData, inSize);
}

VariableLengthData::VariableLengthData(VariableLengthData const& rhs)
    : _impl(new VariableLengthDataImplementation(*rhs._impl)) {}

VariableLengthData::~VariableLengthData() {
  delete _impl;
}

VariableLengthData& VariableLengthData::operator=(VariableLengthData const& rhs) {
  if (this != &rhs) {
    *_impl = *rhs._impl;
  }
  return *this;
}

void const* VariableLengthData::data() const {
  return _impl->data();
}

size_t VariableLengthData::size() const {
  return _impl->size();
}

void VariableLengthData::setData(void const* inData, size_t inSize) {
  _impl->setData(inData, inSize);
}

void VariableLengthData::setDataPointer(void* inData, size_t inSize) {
  _impl->setDataPointer(inData, inSize);
}

void VariableLengthData::takeDataPointer(
    void* inData,
    size_t inSize,
    VariableLengthDataDeleteFunction func) {
  _impl->takeDataPointer(inData, inSize, func);
}

ConfigurationResult::ConfigurationResult()
    : configurationUsed(false),
      addressUsed(false),
      additionalSettingsResult(SETTINGS_IGNORED),
      message() {}

ConfigurationResult::ConfigurationResult(
    bool configurationWasUsed,
    bool addressWasUsed,
    AdditionalSettingsResultCode settingsResultCode,
    std::wstring const& resultMessage)
    : configurationUsed(configurationWasUsed),
      addressUsed(addressWasUsed),
      additionalSettingsResult(settingsResultCode),
      message(resultMessage) {}

RtiConfiguration RtiConfiguration::createConfiguration() {
  return RtiConfiguration();
}

RtiConfiguration& RtiConfiguration::withConfigurationName(std::wstring const& configurationName) {
  _configurationName = configurationName;
  return *this;
}

RtiConfiguration& RtiConfiguration::withRtiAddress(std::wstring const& rtiAddress) {
  _rtiAddress = rtiAddress;
  return *this;
}

RtiConfiguration& RtiConfiguration::withAdditionalSettings(std::wstring const& additionalSettings) {
  _additionalSettings = additionalSettings;
  return *this;
}

std::wstring const& RtiConfiguration::configurationName() const {
  return _configurationName;
}

std::wstring const& RtiConfiguration::rtiAddress() const {
  return _rtiAddress;
}

std::wstring const& RtiConfiguration::additionalSettings() const {
  return _additionalSettings;
}

FederateAmbassador::FederateAmbassador() = default;
FederateAmbassador::~FederateAmbassador() noexcept = default;

FederationExecutionInformation::FederationExecutionInformation() = default;

FederationExecutionInformation::FederationExecutionInformation(
    std::wstring const& federationName,
    std::wstring const& logicalTimeImplementationName)
    : federationExecutionName(federationName),
      logicalTimeImplementationName(logicalTimeImplementationName) {}

FederationExecutionMemberInformation::FederationExecutionMemberInformation() = default;

FederationExecutionMemberInformation::FederationExecutionMemberInformation(
    std::wstring const& federateName,
    std::wstring const& federateType)
    : federateName(federateName),
      federateType(federateType) {}

FederateRestoreStatus::FederateRestoreStatus() = default;

FederateRestoreStatus::FederateRestoreStatus(
    FederateHandle const& preHandle,
    FederateHandle const& postHandle,
    RestoreStatus restoreStatus)
    : preRestoreHandle(preHandle),
      postRestoreHandle(postHandle),
      status(restoreStatus) {}

std::wostream& operator<<(std::wostream& stream, Exception const& exception) {
  stream << exception.name() << L": " << exception.what();
  return stream;
}

}  // namespace rti1516_2025
