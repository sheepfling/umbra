#include "tck_main_scenarios.hpp"

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace hla_rti_cpp_tck::detail {
namespace {

std::wstring additionalSettingsWithServiceReportDirectory(Options const& options) {
  std::wstring const settings = toWide(options.additionalSettings);
  constexpr std::wstring_view directorySetting = L"serviceReportDirectory=";
  std::wstring result;
  auto append = [&](std::wstring_view token) {
    if (token.empty()) {
      return;
    }
    if (!result.empty()) {
      result.push_back(L';');
    }
    result.append(token);
  };

  std::size_t begin = 0U;
  while (begin <= settings.size()) {
    auto const end = settings.find(L';', begin);
    auto const token = std::wstring_view(settings).substr(
        begin,
        end == std::wstring::npos ? std::wstring::npos : end - begin);
    if (!token.starts_with(directorySetting)) {
      append(token);
    }
    if (end == std::wstring::npos) {
      break;
    }
    begin = end + 1U;
  }

  append(directorySetting);
  result.append(options.serviceReportDirectory.wstring());
  return result;
}

void connectWithServiceReportDirectory(Session& session, Options const& options) {
  auto configuration = rti::RtiConfiguration::createConfiguration();
  auto const configurationName = options.memberConfigurationName.empty()
      ? options.configurationName
      : options.memberConfigurationName;
  if (!configurationName.empty()) {
    configuration.withConfigurationName(toWide(configurationName));
  }
  if (!options.rtiAddress.empty()) {
    configuration.withRtiAddress(toWide(options.rtiAddress));
  }
  configuration.withAdditionalSettings(
      additionalSettingsWithServiceReportDirectory(options));
  static_cast<void>(session.connectWithConfiguration(configuration));
}

std::optional<std::filesystem::path> advertisedServiceReportFile(
    Recorder const& recorder,
    rti::AttributeHandle const& federateNameAttribute,
    rti::AttributeHandle const& reportFileAttribute,
    std::wstring const& federateName) {
  for (auto const& reflection : recorder.reflections()) {
    auto const name = std::find_if(
        reflection.values.begin(), reflection.values.end(),
        [&](auto const& value) { return value.first == federateNameAttribute; });
    auto const reportFile = std::find_if(
        reflection.values.begin(), reflection.values.end(),
        [&](auto const& value) { return value.first == reportFileAttribute; });
    if (name == reflection.values.end() || reportFile == reflection.values.end()) {
      continue;
    }
    rti::HLAunicodeString decodedName;
    decodedName.decode(name->second);
    if (decodedName.get() != federateName) {
      continue;
    }
    rti::HLAunicodeString decodedReportFile;
    decodedReportFile.decode(reportFile->second);
    if (decodedReportFile.get().empty()) {
      throw std::runtime_error(
          "Publisher Federate MOM advertised an empty HLAreportServiceFile value");
    }
    return std::filesystem::path(decodedReportFile.get());
  }
  return std::nullopt;
}

std::string readServiceReportFile(std::filesystem::path const& file) {
  std::ifstream input(file, std::ios::binary);
  if (!input) {
    throw std::runtime_error(
        "Unable to read the file advertised by HLAreportServiceFile");
  }
  return std::string(
      std::istreambuf_iterator<char>{input},
      std::istreambuf_iterator<char>{});
}

std::optional<std::vector<std::string_view>> braceDelimitedObjectRecords(
    std::string_view contents) {
  std::vector<std::string_view> records;
  std::size_t recordBegin = 0U;
  std::size_t braceDepth = 0U;
  bool inString = false;
  bool escaped = false;
  for (std::size_t index = 0U; index < contents.size(); ++index) {
    auto const value = contents[index];
    if (inString) {
      if (escaped) {
        escaped = false;
      } else if (value == '\\') {
        escaped = true;
      } else if (value == '"') {
        inString = false;
      }
      continue;
    }
    if (braceDepth == 0U) {
      if (value != '{') {
        return std::nullopt;
      }
      recordBegin = index;
      braceDepth = 1U;
      continue;
    }
    if (value == '"') {
      inString = true;
    } else if (value == '{') {
      ++braceDepth;
    } else if (value == '}') {
      if (braceDepth == 0U) {
        return std::nullopt;
      }
      --braceDepth;
      if (braceDepth == 0U) {
        records.push_back(contents.substr(recordBegin, index - recordBegin + 1U));
      }
    }
  }
  if (braceDepth != 0U || inString || records.empty()) {
    return std::nullopt;
  }
  return records;
}

std::optional<std::string_view> jsonStringToken(
    std::string_view object,
    std::string_view key) {
  std::string const marker = std::string{"\""} + std::string{key} + "\":";
  auto const member = object.find(marker);
  if (member == std::string_view::npos) return std::nullopt;
  auto const begin = member + marker.size();
  if (begin >= object.size() || object[begin] != '"') return std::nullopt;
  bool escaped = false;
  for (std::size_t index = begin + 1U; index < object.size(); ++index) {
    if (escaped) {
      escaped = false;
    } else if (object[index] == '\\') {
      escaped = true;
    } else if (object[index] == '"') {
      return object.substr(begin, index - begin + 1U);
    }
  }
  return std::nullopt;
}

std::optional<std::size_t> jsonStringArrayLength(
    std::string_view object,
    std::string_view key) {
  std::string const marker = std::string{"\""} + std::string{key} + "\":";
  auto const member = object.find(marker);
  if (member == std::string_view::npos) return std::nullopt;
  std::size_t index = member + marker.size();
  if (index >= object.size() || object[index++] != '[') return std::nullopt;
  auto skipWhitespace = [&] {
    while (index < object.size() &&
           (object[index] == ' ' || object[index] == '\t' ||
            object[index] == '\r' || object[index] == '\n')) {
      ++index;
    }
  };
  std::size_t count = 0U;
  skipWhitespace();
  if (index < object.size() && object[index] == ']') return 0U;
  std::vector<std::string_view> values;
  while (index < object.size()) {
    if (object[index++] != '"') return std::nullopt;
    auto const tokenBegin = index - 1U;
    bool escaped = false;
    bool closed = false;
    while (index < object.size()) {
      char const character = object[index++];
      if (escaped) {
        escaped = false;
      } else if (character == '\\') {
        escaped = true;
      } else if (character == '"') {
        closed = true;
        break;
      }
    }
    if (!closed) return std::nullopt;
    auto const token = object.substr(tokenBegin, index - tokenBegin);
    if (std::find(values.begin(), values.end(), token) != values.end()) {
      return std::nullopt;
    }
    values.push_back(token);
    ++count;
    skipWhitespace();
    if (index >= object.size()) return std::nullopt;
    if (object[index] == ']') return count;
    if (object[index++] != ',') return std::nullopt;
    skipWhitespace();
  }
  return std::nullopt;
}

std::optional<bool> jsonBooleanValue(
    std::string_view object,
    std::string_view key) {
  std::string const marker = std::string{"\""} + std::string{key} + "\":";
  auto const member = object.find(marker);
  if (member == std::string_view::npos) return std::nullopt;
  auto const begin = member + marker.size();
  if (object.substr(begin, 4U) == "true") return true;
  if (object.substr(begin, 5U) == "false") return false;
  return std::nullopt;
}

std::optional<std::string_view> jsonObjectRecord(
    std::string_view document,
    std::string_view marker) {
  auto const member = document.find(marker);
  if (member == std::string_view::npos) return std::nullopt;
  auto const begin = member + marker.size() - 1U;
  std::size_t depth = 0U;
  bool inString = false;
  bool escaped = false;
  for (std::size_t index = begin; index < document.size(); ++index) {
    char const character = document[index];
    if (inString) {
      if (escaped) {
        escaped = false;
      } else if (character == '\\') {
        escaped = true;
      } else if (character == '"') {
        inString = false;
      }
      continue;
    }
    if (character == '"') {
      inString = true;
    } else if (character == '{') {
      ++depth;
    } else if (character == '}') {
      if (depth == 0U) return std::nullopt;
      --depth;
      if (depth == 0U) return document.substr(begin, index - begin + 1U);
    }
  }
  return std::nullopt;
}

std::optional<std::string_view> firstJsonObjectRecord(
    std::string_view document) {
  if (document.empty() || document.front() != '{') {
    return std::nullopt;
  }
  std::size_t depth = 0U;
  bool inString = false;
  bool escaped = false;
  for (std::size_t index = 0U; index < document.size(); ++index) {
    char const character = document[index];
    if (inString) {
      if (escaped) {
        escaped = false;
      } else if (character == '\\') {
        escaped = true;
      } else if (character == '"') {
        inString = false;
      }
      continue;
    }
    if (character == '"') {
      inString = true;
    } else if (character == '{') {
      ++depth;
    } else if (character == '}') {
      if (depth == 0U) return std::nullopt;
      --depth;
      if (depth == 0U) return document.substr(0U, index + 1U);
    }
  }
  return std::nullopt;
}

std::string jsonStringLiteral(std::string_view value) {
  constexpr std::string_view hexadecimalDigits{"0123456789abcdef"};
  std::string result{"\""};
  for (char const rawCharacter : value) {
    auto const character = static_cast<unsigned char>(rawCharacter);
    switch (character) {
      case '\'': result.append("\\'"); break;
      case '"':
      case '\\':
        result.push_back('\\');
        result.push_back(static_cast<char>(character));
        break;
      case '\b': result.append("\\b"); break;
      case '\f': result.append("\\f"); break;
      case '\n': result.append("\\n"); break;
      case '\r': result.append("\\r"); break;
      case '\t': result.append("\\t"); break;
      default:
        if (character < 0x20U) {
          result.append("\\u00");
          result.push_back(hexadecimalDigits[character >> 4U]);
          result.push_back(hexadecimalDigits[character & 0x0fU]);
        } else {
          result.push_back(static_cast<char>(character));
        }
        break;
    }
  }
  result.push_back('"');
  return result;
}

std::optional<std::string_view> configurationRecord(
    std::string_view initialRecord) {
  return jsonObjectRecord(initialRecord, "\"Configuration\":{");
}

std::optional<std::string_view> federationAttributesRecord(
    std::string_view initialRecord) {
  return jsonObjectRecord(initialRecord, "\"HLAmanager.HLAfederation\":{");
}

std::optional<std::string_view> federateAttributesRecord(
    std::string_view initialRecord) {
  return jsonObjectRecord(initialRecord, "\"HLAmanager.HLAfederate\":{");
}

}  // namespace

void scenarioServiceReportInteraction(Options const& options, rti::CallbackModel model) {
  require(
      !options.mimFom.empty(),
      "Service-report interaction testing requires an adapter-supplied standard MIM");

  Session subject(options, model, "service-report-subject");
  Session observer(options, model, "service-report-observer");
  auto const federation = federationName(options, "service-report-interaction");
  subject.connect();
  observer.connect();
  subject.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(
      options.ownerFederateName + L"-service-report-subject",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-service-report-observer",
      options.federateType,
      federation);

  auto const reportClass = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(
      reportClass.isValid(),
      "Service-report interaction lookup returned an invalid HLAreportServiceInvocation handle");
  std::vector<rti::ParameterHandle> reportParameters;
  for (auto const& name : {
           L"HLAservice",
           L"HLAserviceType",
           L"HLAsuccessIndicator",
           L"HLAsuppliedArguments",
           L"HLAreturnedArgument",
           L"HLAexception",
           L"HLAserialNumber",
           L"HLAfederate"}) {
    auto const parameter = observer.rtiAmbassador().getParameterHandle(reportClass, name);
    require(
        parameter.isValid(),
        "Service-report interaction parameter lookup returned an invalid handle");
    reportParameters.push_back(parameter);
  }

  auto const interactionClass = subject.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const parameter = subject.rtiAmbassador().getParameterHandle(
      interactionClass,
      options.parameterName);
  require(
      interactionClass.isValid() && parameter.isValid(),
      "Service-report source interaction lookup returned an invalid handle");

  // Suppress reports for setup calls. The source service invocation below is
  // the only operation whose report is asserted by this scenario.
  subject.rtiAmbassador().setServiceReportingSwitch(false);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().subscribeInteractionClass(reportClass);
  subject.rtiAmbassador().publishInteractionClass(interactionClass);
  observer.recorder().clearInteraction();

  subject.rtiAmbassador().setServiceReportingSwitch(true);
  std::vector<std::uint8_t> const valueBytes{0x53U, 0x52U};
  std::vector<std::uint8_t> const tagBytes{0x4DU, 0x4FU, 0x4DU};
  rti::ParameterHandleValueMap values;
  values.emplace(
      parameter,
      rti::VariableLengthData(valueBytes.data(), valueBytes.size()));
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  subject.rtiAmbassador().sendInteraction(interactionClass, values, tag);

  waitFor(
      observer,
      [&] { return observer.recorder().interaction().present; },
      options,
      "the HLAreportServiceInvocation callback");
  auto const report = observer.recorder().interaction();
  require(
      report.interaction == reportClass &&
          report.parameters.size() == reportParameters.size() &&
          !report.producer.isValid(),
      "Service-report callback returned the wrong interaction identity or producer metadata");
  require(
      report.tag.empty() && !report.regions.has_value(),
      "Service-report callback returned user tag or region metadata");
  require(
      report.transportation ==
          observer.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable"),
      "Service-report callback returned the wrong transportation type");
  for (auto const parameterHandle : reportParameters) {
    require(
        report.parameters.count(parameterHandle) == 1U,
        "Service-report callback omitted a standard report parameter");
  }
  auto const reportedFederate = observer.rtiAmbassador().decodeFederateHandle(
      report.parameters.at(reportParameters.at(7U)));
  require(
      reportedFederate == subject.federateHandle(),
      "Service-report callback identified the wrong joined federate");

  rti::HLAunicodeString service;
  service.decode(report.parameters.at(reportParameters[0]));
  require(
      service.get() == L"SendInteraction",
      "Service-report callback returned the wrong service name");
  rti::HLAinteger16BE serviceType;
  serviceType.decode(report.parameters.at(reportParameters[1]));
  require(
      serviceType.get() == 2,
      "Service-report callback returned the wrong interaction service type");
  rti::HLAboolean success;
  success.decode(report.parameters.at(reportParameters[2]));
  require(success.get(), "Service-report callback did not report a successful invocation");

  auto const quoted = [](std::wstring const& value) {
    return std::wstring{L"\""} + value + L"\"";
  };
  rti::HLAfixedRecord argumentPrototype;
  argumentPrototype.appendElement(rti::HLAinteger32BE{})
      .appendElement(rti::HLAunicodeString{})
      .appendElement(rti::HLAunicodeString{});
  rti::HLAvariableArray suppliedArguments{argumentPrototype};
  suppliedArguments.decode(report.parameters.at(reportParameters[3]));
  require(
      suppliedArguments.size() == 4U,
      "Successful SendInteraction service report omitted its four supplied arguments");
  auto const verifyArgument = [&](std::size_t index,
                                  rti::Integer32 type,
                                  std::wstring const& name,
                                  std::wstring const& value) {
    auto const& argument = dynamic_cast<rti::HLAfixedRecord const&>(
        suppliedArguments.get(index));
    require(
        dynamic_cast<rti::HLAinteger32BE const&>(argument.get(0U)).get() == type &&
            dynamic_cast<rti::HLAunicodeString const&>(argument.get(1U)).get() == name &&
            dynamic_cast<rti::HLAunicodeString const&>(argument.get(2U)).get() == value,
        "Successful SendInteraction service report returned a wrong supplied argument");
  };
  verifyArgument(
      0U,
      27,
      L"Interaction class designator",
      quoted(interactionClass.toString()));
  verifyArgument(
      1U,
      40,
      L"Constrained set of interaction parameter designator and value pairs",
      L"{\"" + parameter.toString() + L"\":\"U1I=\"}");
  verifyArgument(2U, 60, L"User-supplied tag", L"\"TU9N\"");
  verifyArgument(3U, 34, L"Optional timestamp", L"null");

  // IEEE 1516.1-2025 §11.5.1 requires HLAargumentType=Null when a service
  // has no return value. In the standard MIM, Null is enumerator 34.
  rti::HLAfixedRecord returnedArgument;
  returnedArgument.appendElement(rti::HLAinteger32BE{})
      .appendElement(rti::HLAunicodeString{})
      .appendElement(rti::HLAunicodeString{});
  returnedArgument.decode(report.parameters.at(reportParameters[4]));
  require(
      returnedArgument.size() == 3U &&
          dynamic_cast<rti::HLAinteger32BE const&>(returnedArgument.get(0U)).get() == 34 &&
          dynamic_cast<rti::HLAunicodeString const&>(returnedArgument.get(1U)).get().empty() &&
          dynamic_cast<rti::HLAunicodeString const&>(returnedArgument.get(2U)).get() == L"null",
      "Successful void SendInteraction service report did not use the Null returned-argument type");
  rti::HLAunicodeString exception;
  exception.decode(report.parameters.at(reportParameters[5]));
  require(exception.get().empty(), "Successful service report carried an exception");
  rti::HLAinteger32BE serial;
  serial.decode(report.parameters.at(reportParameters[6]));
  require(serial.get() == 0, "First service report did not use serial number zero");

  observer.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  subject.rtiAmbassador().setServiceReportingSwitch(false);
  observer.resign(rti::NO_ACTION);
  subject.resign(rti::DELETE_OBJECTS);
  subject.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  subject.disconnect();
}

void scenarioFederateServiceReportingMomSwitch(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty() && !options.mimFom.empty() &&
          !options.logicalTimeImplementationName.empty() &&
          !options.interactionClassName.empty() && !options.parameterName.empty(),
      "Federate MOM Service Reporting testing requires adapter-supplied FOM, standard MIM, and logical-time configuration");

  Session federate(options, model, "owner");
  Session observer(options, model, "service-report-observer");
  auto const federation = federationName(
      options,
      "federate-service-reporting-mom-switch");
  federate.connect();
  observer.connect();
  federate.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  federate.join(
      options.ownerFederateName + L"-service-reporting-mom-switch",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-service-reporting-mom-switch-observer",
      options.federateType,
      federation);

  auto& federateAmbassador = federate.rtiAmbassador();
  auto& observerAmbassador = observer.rtiAmbassador();
  require(
      !federateAmbassador.getServiceReportingSwitch() &&
          !observerAmbassador.getServiceReportingSwitch(),
      "Standard MIM Service Reporting switch did not start false for both joined federates");
  require(
      !federateAmbassador.getSendServiceReportsToFileSwitch(),
      "Service-report interaction coverage requires file reporting to remain disabled");

  auto const reportClass = observerAmbassador.getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  auto const reportService = observerAmbassador.getParameterHandle(
      reportClass,
      L"HLAservice");
  auto const reportServiceType = observerAmbassador.getParameterHandle(
      reportClass,
      L"HLAserviceType");
  auto const reportSuccess = observerAmbassador.getParameterHandle(
      reportClass,
      L"HLAsuccessIndicator");
  auto const reportArguments = observerAmbassador.getParameterHandle(
      reportClass,
      L"HLAsuppliedArguments");
  auto const reportSerial = observerAmbassador.getParameterHandle(
      reportClass,
      L"HLAserialNumber");
  auto const reportFederate = observerAmbassador.getParameterHandle(
      reportClass,
      L"HLAfederate");
  auto const reliable = observerAmbassador.getTransportationTypeHandle(
      L"HLAreliable");
  require(
      reportClass.isValid() && reportService.isValid() &&
          reportServiceType.isValid() && reportSuccess.isValid() &&
          reportArguments.isValid() && reportSerial.isValid() &&
          reportFederate.isValid() && reliable.isValid(),
      "Standard MIM Service Reporting observer lookup returned an invalid report handle");
  observerAmbassador.subscribeInteractionClass(reportClass);

  auto const interactionClass = federateAmbassador.getInteractionClassHandle(
      options.interactionClassName);
  auto const interactionParameter = federateAmbassador.getParameterHandle(
      interactionClass,
      options.parameterName);
  require(
      interactionClass.isValid() && interactionParameter.isValid(),
      "Service Reporting source interaction lookup returned an invalid handle");
  federateAmbassador.publishInteractionClass(interactionClass);

  auto const setSwitches = federateAmbassador.getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetSwitches");
  auto const serviceReporting = federateAmbassador.getParameterHandle(
      setSwitches,
      L"HLAserviceReporting");
  require(
      setSwitches.isValid() && serviceReporting.isValid(),
      "Standard federate-MOM Service Reporting lookup returned an invalid handle");
  auto setReportingThroughMom = [&](bool const enabled) {
    rti::ParameterHandleValueMap const parameters{
        {serviceReporting, rti::HLAinteger32BE{enabled ? 1 : 0}.encode()}};
    federateAmbassador.sendInteraction(
        setSwitches,
        parameters,
        rti::VariableLengthData{});
    require(
        federateAmbassador.getServiceReportingSwitch() == enabled,
        std::string("HLAsetSwitches HLAserviceReporting did not set the switch to ") +
            (enabled ? "true" : "false"));
    require(
        !observerAmbassador.getServiceReportingSwitch(),
        "HLAsetSwitches HLAserviceReporting changed the observer's joined-federate switch");
  };
  setReportingThroughMom(true);
  if (model == rti::HLA_EVOKED) {
    while (observerAmbassador.evokeCallback(0.0)) {
    }
  }

  observer.recorder().clearInteraction();
  std::vector<std::uint8_t> const valueBytes{0x01U, 0x02U};
  std::vector<std::uint8_t> const tagBytes{0x72U, 0x6FU, 0x69U};
  rti::ParameterHandleValueMap values;
  values.emplace(
      interactionParameter,
      rti::VariableLengthData(valueBytes.data(), valueBytes.size()));
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  federateAmbassador.sendInteraction(interactionClass, values, tag);
  waitFor(
      observer,
      [&] { return observer.recorder().interaction().present; },
      options,
      "the HLAreportServiceInvocation generated while HLAserviceReporting is enabled through HLAsetSwitches");

  auto const report = observer.recorder().interaction();
  require(
      report.interaction == reportClass && report.parameters.size() == 8U &&
          report.transportation == reliable && report.tag.empty() &&
          !report.regions.has_value() && !report.producer.isValid(),
      "HLAsetSwitches-enabled service report had non-standard interaction metadata");
  rti::HLAunicodeString service;
  service.decode(report.parameters.at(reportService));
  require(
      service.get() == L"SendInteraction",
      "HLAsetSwitches-enabled service report did not identify SendInteraction");
  rti::HLAinteger16BE serviceType;
  serviceType.decode(report.parameters.at(reportServiceType));
  require(
      serviceType.get() == 2,
      "HLAsetSwitches-enabled SendInteraction report used the wrong service type");
  rti::HLAboolean success;
  success.decode(report.parameters.at(reportSuccess));
  require(
      success.get(),
      "HLAsetSwitches-enabled service report did not identify the successful invocation");
  require(
      observerAmbassador.decodeFederateHandle(
          report.parameters.at(reportFederate)) == federate.federateHandle(),
      "HLAsetSwitches-enabled service report identified a different joined federate");
  rti::HLAinteger32BE serial;
  serial.decode(report.parameters.at(reportSerial));
  require(
      serial.get() == 0,
      "The first HLAsetSwitches-enabled service report did not use serial number zero");

  rti::HLAfixedRecord suppliedArgumentPrototype;
  suppliedArgumentPrototype.appendElement(rti::HLAinteger32BE{})
      .appendElement(rti::HLAunicodeString{})
      .appendElement(rti::HLAunicodeString{});
  rti::HLAvariableArray suppliedArguments{suppliedArgumentPrototype};
  suppliedArguments.decode(report.parameters.at(reportArguments));
  require(
      suppliedArguments.size() == 4U,
      "HLAsetSwitches-enabled SendInteraction report returned the wrong supplied-argument count");
  std::array<std::pair<int, std::wstring>, 4U> const expectedArguments{{
      {27, L"\"" + interactionClass.toString() + L"\""},
      {40,
       L"{\"" + interactionParameter.toString() + L"\":\"AQI=\"}"},
      {60, L"\"cm9p\""},
      {34, L"null"},
  }};
  std::array<std::wstring, 4U> const expectedArgumentNames{
      L"Interaction class designator",
      L"Constrained set of interaction parameter designator and value pairs",
      L"User-supplied tag",
      L"Optional timestamp"};
  for (std::size_t index = 0U; index < suppliedArguments.size(); ++index) {
    auto const& argument = dynamic_cast<rti::HLAfixedRecord const&>(
        suppliedArguments.get(index));
    require(
        dynamic_cast<rti::HLAinteger32BE const&>(argument.get(0U)).get() ==
                expectedArguments[index].first &&
            dynamic_cast<rti::HLAunicodeString const&>(argument.get(1U)).get() ==
                expectedArgumentNames[index] &&
            dynamic_cast<rti::HLAunicodeString const&>(argument.get(2U)).get() ==
                expectedArguments[index].second,
        "HLAsetSwitches-enabled service report did not preserve the tested SendInteraction argument");
  }

  setReportingThroughMom(false);
  require(
      !federateAmbassador.getServiceReportingSwitch(),
      "HLAsetSwitches did not disable Service Reporting after the report witness");

  observerAmbassador.unsubscribeInteractionClass(reportClass);
  observer.resign(rti::NO_ACTION);
  federate.resign(rti::NO_ACTION);
  federateAmbassador.destroyFederationExecution(federation);
  observer.disconnect();
  federate.disconnect();

  scenarioFederateServiceReportingFddInitial(options, model);
}

void scenarioFederateServiceReportingMomSwitchContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateServiceReportingMomSwitch(options, model);
}

void scenarioFederateExceptionReportingMomSwitch(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty() && !options.mimFom.empty() &&
          !options.logicalTimeImplementationName.empty(),
      "Federate MOM Exception Reporting testing requires adapter-supplied FOM, standard MIM, and logical-time configuration");

  Session federate(options, model, "owner");
  Session peer(options, model, "exception-reporting-peer");
  auto const federation = federationName(
      options,
      "federate-exception-reporting-mom-switch");
  federate.connect();
  peer.connect();
  federate.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  federate.join(
      options.ownerFederateName + L"-exception-reporting-mom-switch",
      options.federateType,
      federation);
  peer.join(
      options.memberFederateName + L"-exception-reporting-mom-peer",
      options.federateType,
      federation);

  auto& federateAmbassador = federate.rtiAmbassador();
  auto& peerAmbassador = peer.rtiAmbassador();
  require(
      !federateAmbassador.getExceptionReportingSwitch() &&
          !peerAmbassador.getExceptionReportingSwitch(),
      "Standard MIM Exception Reporting switch did not start false for both joined federates");

  auto const setSwitches = federateAmbassador.getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetSwitches");
  auto const exceptionReporting = federateAmbassador.getParameterHandle(
      setSwitches,
      L"HLAexceptionReporting");
  require(
      setSwitches.isValid() && exceptionReporting.isValid(),
      "Standard federate-MOM Exception Reporting lookup returned an invalid handle");
  auto const initialServiceReporting =
      federateAmbassador.getServiceReportingSwitch();
  auto const initialResignAction =
      federateAmbassador.getAutomaticResignDirective();
  auto const initialFileReporting =
      federateAmbassador.getSendServiceReportsToFileSwitch();
  auto setReportingThroughMom = [&](bool const enabled) {
    rti::ParameterHandleValueMap const parameters{
        {exceptionReporting, rti::HLAinteger32BE{enabled ? 1 : 0}.encode()}};
    federateAmbassador.sendInteraction(
        setSwitches,
        parameters,
        rti::VariableLengthData{});
    require(
        federateAmbassador.getExceptionReportingSwitch() == enabled,
        std::string("HLAsetSwitches HLAexceptionReporting did not set the switch to ") +
            (enabled ? "true" : "false"));
    require(
        !peerAmbassador.getExceptionReportingSwitch(),
        "HLAsetSwitches HLAexceptionReporting changed another joined federate's switch");
    require(
        federateAmbassador.getServiceReportingSwitch() ==
                initialServiceReporting &&
            federateAmbassador.getAutomaticResignDirective() ==
                initialResignAction &&
            federateAmbassador.getSendServiceReportsToFileSwitch() ==
                initialFileReporting,
        "HLAsetSwitches HLAexceptionReporting changed a neighboring switch");
  };
  setReportingThroughMom(true);
  setReportingThroughMom(false);

  peer.resign(rti::NO_ACTION);
  federate.resign(rti::NO_ACTION);
  federateAmbassador.destroyFederationExecution(federation);
  peer.disconnect();
  federate.disconnect();
}

void scenarioFederateExceptionReportingMomSwitchContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateExceptionReportingMomSwitch(options, model);
}

void scenarioFederateSendServiceReportsToFileMomSwitch(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty() && !options.mimFom.empty() &&
          !options.logicalTimeImplementationName.empty(),
      "Federate MOM report-destination testing requires adapter-supplied FOM, standard MIM, and logical-time configuration");

  Session federate(options, model, "owner");
  Session peer(options, model, "report-file-switch-peer");
  auto const federation = federationName(
      options,
      "federate-send-service-reports-to-file-mom-switch");
  federate.connect();
  peer.connect();
  federate.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  federate.join(
      options.ownerFederateName + L"-report-file-destination-mom-switch",
      options.federateType,
      federation);
  peer.join(
      options.memberFederateName + L"-report-file-destination-mom-peer",
      options.federateType,
      federation);

  auto& ambassador = federate.rtiAmbassador();
  auto& peerAmbassador = peer.rtiAmbassador();
  require(
      !ambassador.getServiceReportingSwitch(),
      "Standard MIM Service Reporting switch did not start false; report destination must remain inactive in this state-only scenario");
  auto const initialFileReporting =
      ambassador.getSendServiceReportsToFileSwitch();
  auto const initialPeerFileReporting =
      peerAmbassador.getSendServiceReportsToFileSwitch();
  auto const initialExceptionReporting =
      ambassador.getExceptionReportingSwitch();
  auto const initialResignAction = ambassador.getAutomaticResignDirective();
  auto const setSwitches = ambassador.getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetSwitches");
  auto const sendReportsToFile = ambassador.getParameterHandle(
      setSwitches,
      L"HLAsendServiceReportsToFile");
  require(
      setSwitches.isValid() && sendReportsToFile.isValid(),
      "Standard federate-MOM report-destination lookup returned an invalid handle");

  auto setReportDestinationThroughMom = [&](bool const enabled) {
    rti::ParameterHandleValueMap const parameters{
        {sendReportsToFile, rti::HLAinteger32BE{enabled ? 1 : 0}.encode()}};
    ambassador.sendInteraction(
        setSwitches,
        parameters,
        rti::VariableLengthData{});
    require(
        ambassador.getSendServiceReportsToFileSwitch() == enabled,
        std::string("HLAsetSwitches HLAsendServiceReportsToFile did not set the switch to ") +
            (enabled ? "true" : "false"));
    require(
        peerAmbassador.getSendServiceReportsToFileSwitch() ==
            initialPeerFileReporting,
        "HLAsetSwitches HLAsendServiceReportsToFile changed another joined federate's switch");
    require(
        !ambassador.getServiceReportingSwitch() &&
            ambassador.getExceptionReportingSwitch() ==
                initialExceptionReporting &&
            ambassador.getAutomaticResignDirective() == initialResignAction,
        "HLAsetSwitches HLAsendServiceReportsToFile changed a neighboring federate switch");
  };
  setReportDestinationThroughMom(false);
  setReportDestinationThroughMom(true);
  setReportDestinationThroughMom(initialFileReporting);

  peer.resign(rti::NO_ACTION);
  federate.resign(rti::NO_ACTION);
  ambassador.destroyFederationExecution(federation);
  peer.disconnect();
  federate.disconnect();
}

void scenarioFederateSendServiceReportsToFileMomSwitchContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateSendServiceReportsToFileMomSwitch(options, model);
}

void scenarioServiceReportReceiveOrderInteraction(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Receive-order interaction service-report testing requires an adapter-supplied FOM");
  require(
      !options.mimFom.empty(),
      "Receive-order interaction service-report testing requires an adapter-supplied standard MIM");
  require(
      !options.serviceReportDirectory.empty(),
      "File-backed service-report testing requires an adapter-supplied directory");

  Session publisher(options, model, "service-report-receive-order-publisher");
  Session receiver(options, model, "service-report-receive-order-receiver");
  Session observer(options, model, "service-report-receive-order-observer");
  auto const federation =
      federationName(options, "service-report-receive-order-interaction");
  connectWithServiceReportDirectory(publisher, options);
  connectWithServiceReportDirectory(receiver, options);
  observer.connect();
  publisher.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  auto const publisherName =
      options.ownerFederateName + L"-service-report-receive-order-publisher";
  auto const receiverName =
      options.memberFederateName + L"-service-report-receive-order-receiver";
  publisher.join(
      publisherName,
      options.federateType,
      federation);
  receiver.join(
      receiverName,
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-service-report-receive-order-observer",
      options.federateType,
      federation);

  auto const reportClass = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(
      reportClass.isValid(),
      "Receive-order interaction service-report lookup returned an invalid HLAreportServiceInvocation handle");
  std::vector<rti::ParameterHandle> reportParameters;
  for (auto const& name : {
           L"HLAservice",
           L"HLAserviceType",
           L"HLAsuccessIndicator",
           L"HLAsuppliedArguments",
           L"HLAreturnedArgument",
           L"HLAexception",
           L"HLAserialNumber",
           L"HLAfederate"}) {
    auto const parameter = observer.rtiAmbassador().getParameterHandle(reportClass, name);
    require(
        parameter.isValid(),
        "Receive-order interaction service-report parameter lookup returned an invalid handle");
    reportParameters.push_back(parameter);
  }

  auto const interactionClass = publisher.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const parameter = publisher.rtiAmbassador().getParameterHandle(
      interactionClass,
      options.parameterName);
  require(
      interactionClass.isValid() && parameter.isValid(),
      "Receive-order interaction source lookup returned an invalid handle");
  auto const reliable = observer.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable");
  require(
      reliable.isValid(),
      "Receive-order interaction service-report lookup returned an invalid reliable transportation handle");

  auto const federateMomClass = observer.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const federateNameAttribute = observer.rtiAmbassador().getAttributeHandle(
      federateMomClass,
      L"HLAfederateName");
  auto const reportFileAttribute = observer.rtiAmbassador().getAttributeHandle(
      federateMomClass,
      L"HLAreportServiceFile");
  require(
      federateMomClass.isValid() && federateNameAttribute.isValid() &&
          reportFileAttribute.isValid(),
      "Standard Federate MOM did not expose HLAreportServiceFile");
  rti::AttributeHandleSet const federateMomAttributes{
      federateNameAttribute,
      reportFileAttribute};
  observer.rtiAmbassador().subscribeObjectClassAttributes(
      federateMomClass,
      federateMomAttributes,
      true,
      L"");
  std::optional<std::filesystem::path> publisherReportFile;
  std::optional<std::filesystem::path> receiverReportFile;
  waitFor(
      observer,
      receiver,
      [&] {
        publisherReportFile = advertisedServiceReportFile(
            observer.recorder(),
            federateNameAttribute,
            reportFileAttribute,
            publisherName);
        receiverReportFile = advertisedServiceReportFile(
            observer.recorder(),
            federateNameAttribute,
            reportFileAttribute,
            receiverName);
        return publisherReportFile.has_value() && receiverReportFile.has_value();
      },
      options,
      "publisher and receiver HLAreportServiceFile MOM reflections");
  require(
      std::filesystem::is_regular_file(*publisherReportFile),
      "Publisher HLAreportServiceFile did not name an existing report file");
  require(
      std::filesystem::is_regular_file(*receiverReportFile),
      "Receiver HLAreportServiceFile did not name an existing report file");
  require(
      std::filesystem::weakly_canonical(*publisherReportFile) !=
              std::filesystem::weakly_canonical(*receiverReportFile) &&
          !std::filesystem::equivalent(
              *publisherReportFile,
              *receiverReportFile),
      "Two joined federates advertised the same service-report file identity");
  auto const initialReportContents = readServiceReportFile(*publisherReportFile);
  require(
      !initialReportContents.empty(),
      "Publisher's HLAreportServiceFile did not contain its initial record");
  auto const receiverInitialReportContents =
      readServiceReportFile(*receiverReportFile);
  require(
      !receiverInitialReportContents.empty(),
      "Receiver's HLAreportServiceFile did not contain its initial record");
  auto const callbackModelValue =
      jsonStringToken(initialReportContents, "CallbackModel");
  auto const expectedCallbackModel = model == rti::HLA_EVOKED
      ? std::string{"\"HLA_EVOKED\""}
      : std::string{"\"HLA_IMMEDIATE\""};
  require(
      callbackModelValue.has_value() &&
          *callbackModelValue == expectedCallbackModel,
      "Initial service-report Configuration did not contain the selected Table 5 CallbackModel enumeration");
  auto const federationAttributes =
      federationAttributesRecord(initialReportContents);
  require(federationAttributes.has_value(),
          "Initial service-report record omitted FederationAttributes");
  std::string const expectedFederationName =
      std::string{"\""} + toNarrow(federation) + "\"";
  std::string const expectedTimeImplementation =
      std::string{"\""} +
      toNarrow(options.logicalTimeImplementationName) + "\"";
  auto const federationNameValue =
      jsonStringToken(*federationAttributes, "HLAfederationName");
  auto const rtiVersionValue =
      jsonStringToken(*federationAttributes, "HLARTIversion");
  auto const mimDesignatorValue =
      jsonStringToken(*federationAttributes, "HLAMIMDesignator");
  auto const fomModuleDesignators =
      jsonStringArrayLength(*federationAttributes, "HLAFOMmoduleDesignatorList");
  auto const timeImplementationValue =
      jsonStringToken(*federationAttributes, "HLAtimeImplementationName");
  auto const autoProvideValue =
      jsonBooleanValue(*federationAttributes, "HLAautoProvide");
  require(federationNameValue.has_value() &&
              *federationNameValue == expectedFederationName &&
              rtiVersionValue.has_value() &&
              mimDesignatorValue.has_value() &&
              fomModuleDesignators.has_value() && *fomModuleDesignators > 0U &&
              timeImplementationValue.has_value() &&
              *timeImplementationValue == expectedTimeImplementation &&
              autoProvideValue.has_value(),
          "FederationAttributes did not contain the complete six-field Table 5 record with the expected JSON types and configured identity values");

  // Keep setup calls out of the one accepted SendInteraction report. The
  // publisher reports to the public MOM subscriber; the receiver only gets
  // the ordinary application interaction.
  publisher.rtiAmbassador().setServiceReportingSwitch(false);
  publisher.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  receiver.rtiAmbassador().setServiceReportingSwitch(false);
  receiver.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().subscribeInteractionClass(reportClass);
  publisher.rtiAmbassador().publishInteractionClass(interactionClass);
  receiver.rtiAmbassador().subscribeInteractionClass(interactionClass);
  observer.recorder().clearInteraction();
  receiver.recorder().clearInteraction();

  publisher.rtiAmbassador().setServiceReportingSwitch(true);
  std::vector<std::uint8_t> const valueBytes{0x01U, 0x02U};
  std::vector<std::uint8_t> const tagBytes{'r', 'o', 'i'};
  rti::ParameterHandleValueMap values;
  values.emplace(
      parameter,
      rti::VariableLengthData(valueBytes.data(), valueBytes.size()));
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  publisher.rtiAmbassador().sendInteraction(interactionClass, values, tag);

  if (model == rti::HLA_IMMEDIATE) {
    require(
        observer.recorder().interactions().size() == 1U &&
            receiver.recorder().interactions().size() == 1U,
        "Immediate receive-order interaction did not deliver both callbacks synchronously");
  } else {
    require(
        observer.recorder().interactions().empty() &&
            receiver.recorder().interactions().empty(),
        "Evoked receive-order interaction delivered a callback before callback servicing");
    waitFor(
        observer,
        receiver,
        [&] {
          return observer.recorder().interactions().size() == 1U &&
              receiver.recorder().interactions().size() == 1U;
        },
        options,
        "receive-order interaction and service-report callbacks");
  }

  auto const report = observer.recorder().interactions().at(0U);
  auto const quoted = [](std::wstring const& value) {
    return std::wstring{L"\""} + value + L"\"";
  };
  requirePortableServiceReport(
      observer,
      report,
      reportClass,
      reportParameters,
      L"SendInteraction",
      {{27, L"Interaction class designator", quoted(interactionClass.toString())},
       {40,
        L"Constrained set of interaction parameter designator and value pairs",
        L"{\"" + parameter.toString() + L"\":\"AQI=\"}"},
       {60, L"User-supplied tag", L"\"cm9p\""},
       {34, L"Optional timestamp", L"null"}},
      {34, L"", L"null"},
      0,
      "Receive-order SendInteraction service report",
      2);

  rti::HLAfixedRecord suppliedArgumentPrototype;
  suppliedArgumentPrototype.appendElement(rti::HLAinteger32BE{})
      .appendElement(rti::HLAunicodeString{})
      .appendElement(rti::HLAunicodeString{});
  rti::HLAvariableArray decodedSuppliedArguments{suppliedArgumentPrototype};
  decodedSuppliedArguments.decode(report.parameters.at(reportParameters.at(3U)));
  std::optional<std::wstring> callbackParameterValueMap;
  for (std::size_t index = 0U; index < decodedSuppliedArguments.size(); ++index) {
    auto const& argument = dynamic_cast<rti::HLAfixedRecord const&>(
        decodedSuppliedArguments.get(index));
    auto const argumentType =
        dynamic_cast<rti::HLAinteger32BE const&>(argument.get(0U)).get();
    if (argumentType != 40) {
      continue;
    }
    require(
        !callbackParameterValueMap.has_value(),
        "SendInteraction service report returned duplicate type-40 Map arguments");
    callbackParameterValueMap =
        dynamic_cast<rti::HLAunicodeString const&>(argument.get(2U)).get();
  }
  require(
      callbackParameterValueMap.has_value() &&
          *callbackParameterValueMap ==
              L"{\"" + parameter.toString() + L"\":\"AQI=\"}",
      "Public HLAreportServiceInvocation did not encode the type-40 Map value expected from SendInteraction");

  auto const interaction = receiver.recorder().interactions().at(0U);
  require(
      interaction.interaction == interactionClass &&
          interaction.parameters.size() == 1U &&
          interaction.parameters.count(parameter) == 1U &&
          copyBytes(interaction.parameters.at(parameter)) == valueBytes &&
          interaction.tag == tagBytes && interaction.transportation == reliable &&
          interaction.producer == publisher.federateHandle() &&
          !interaction.regions.has_value(),
      "Receive-order application interaction returned non-standard callback data");

  require(
      readServiceReportFile(*publisherReportFile) == initialReportContents,
      "SendInteraction was appended while the file-reporting switch was disabled");

  // Keep the disabled-switch MOM interaction assertion above independent from
  // the enabled-switch file append assertion below.
  publisher.rtiAmbassador().setServiceReportingSwitch(false);
  publisher.rtiAmbassador().setSendServiceReportsToFileSwitch(true);
  publisher.rtiAmbassador().setServiceReportingSwitch(true);
  require(
      publisher.rtiAmbassador().getServiceReportingSwitch() &&
          publisher.rtiAmbassador().getSendServiceReportsToFileSwitch(),
      "File-backed SendInteraction coverage requires both service-reporting switches to be enabled");
  observer.recorder().clearInteraction();
  receiver.recorder().clearInteraction();
  auto const fileContentsBeforeEnabledSend =
      readServiceReportFile(*publisherReportFile);
  std::string const expectedParameterValueMap =
      std::string{"\"HLAargumentType\":40,\"HLAargumentName\":\""
                  "Constrained set of interaction parameter designator and value pairs\","
                  "\"HLAargumentValue\":"} +
      toNarrow(*callbackParameterValueMap);
  publisher.rtiAmbassador().sendInteraction(interactionClass, values, tag);
  publisher.rtiAmbassador().sendInteraction(interactionClass, values, tag);

  auto const fileAndCallbacksObserved = [&] {
    auto const currentContents = readServiceReportFile(*publisherReportFile);
    auto const appended =
        currentContents.substr(fileContentsBeforeEnabledSend.size());
    auto const allRecords = braceDelimitedObjectRecords(currentContents);
    auto const appendedRecords = braceDelimitedObjectRecords(appended);
    if (!currentContents.starts_with(fileContentsBeforeEnabledSend) ||
        !allRecords.has_value() || allRecords->size() != 3U ||
        !appendedRecords.has_value() || appendedRecords->size() != 2U) {
      return false;
    }
    std::string const expectedService =
        "\"HLAservice\":\"SendInteraction\"";
    auto const eachAppendIsExpectedService = std::all_of(
        appendedRecords->begin(), appendedRecords->end(),
        [&](std::string_view record) {
          auto const serviceMarker = record.find("\"HLAservice\":\"");
          return serviceMarker != std::string_view::npos &&
              record.compare(serviceMarker, expectedService.size(), expectedService) ==
                  0 &&
              record.find("\"HLAservice\":\"", serviceMarker +
                  std::string_view{"\"HLAservice\":\""}.size()) ==
                  std::string_view::npos &&
              record.find(expectedParameterValueMap) != std::string_view::npos;
        });
    return eachAppendIsExpectedService &&
        receiver.recorder().interactions().size() == 2U;
  };
  if (model == rti::HLA_IMMEDIATE) {
    require(
        fileAndCallbacksObserved(),
        "Enabled file reporting did not append SendInteraction before its immediate callbacks returned");
  } else {
    require(
        receiver.recorder().interactions().empty(),
        "Evoked callback model delivered the enabled-switch application interaction before callback servicing");
    waitFor(
        observer,
        receiver,
        fileAndCallbacksObserved,
        options,
        "file-backed SendInteraction service report and application callbacks");
  }
  if (model == rti::HLA_EVOKED) {
    observer.pump();
  }
  require(
      observer.recorder().interactions().empty(),
      "Enabled file reporting delivered HLAreportServiceInvocation to subscribed federates");
  for (auto const& enabledFileInteraction : receiver.recorder().interactions()) {
    require(
        enabledFileInteraction.interaction == interactionClass &&
            enabledFileInteraction.parameters.size() == 1U &&
            enabledFileInteraction.parameters.count(parameter) == 1U &&
            copyBytes(enabledFileInteraction.parameters.at(parameter)) == valueBytes &&
            enabledFileInteraction.tag == tagBytes &&
            enabledFileInteraction.transportation == reliable &&
            enabledFileInteraction.producer == publisher.federateHandle() &&
            !enabledFileInteraction.regions.has_value(),
        "File-reporting SendInteraction returned non-standard application callback data");
  }

  observer.recorder().clearInteraction();
  auto const reportFileBeforeHandleLookup =
      readServiceReportFile(*publisherReportFile);
  auto const returnedParameter = publisher.rtiAmbassador().getParameterHandle(
      interactionClass,
      options.parameterName);
  require(returnedParameter == parameter,
          "GetParameterHandle returned a different public parameter handle");
  std::string const expectedReturnedArgument =
      std::string{"\"HLAreturnedArgument\":[{\"HLAargumentType\":39,\"HLAargumentValue\":\""} +
      toNarrow(parameter.toString()) + "\"}]";
  auto const handleLookupContents =
      readServiceReportFile(*publisherReportFile);
  auto const handleLookupAppend =
      handleLookupContents.substr(reportFileBeforeHandleLookup.size());
  require(
      handleLookupContents.size() > reportFileBeforeHandleLookup.size() &&
          handleLookupAppend.find("\"HLAservice\":\"GetParameterHandle\"") !=
              std::string::npos &&
          handleLookupAppend.find(expectedReturnedArgument) !=
              std::string::npos,
      "GetParameterHandle file reporting omitted its two-field HLAreturnedArgument record");

  // A second joined federate must keep its own advertised file identity and
  // append its service records there, without cross-writing the publisher's
  // report file.
  auto const receiverInteractionClass =
      receiver.rtiAmbassador().getInteractionClassHandle(
          options.interactionClassName);
  auto const receiverFileBeforeHandleLookup =
      readServiceReportFile(*receiverReportFile);
  auto const publisherFileBeforeReceiverLookup =
      readServiceReportFile(*publisherReportFile);
  receiver.rtiAmbassador().setSendServiceReportsToFileSwitch(true);
  receiver.rtiAmbassador().setServiceReportingSwitch(true);
  auto const receiverReturnedParameter =
      receiver.rtiAmbassador().getParameterHandle(
          receiverInteractionClass,
          options.parameterName);
  require(
      receiverReturnedParameter == parameter,
      "Receiver GetParameterHandle returned a different public parameter handle");
  auto const receiverFileAfterHandleLookup =
      readServiceReportFile(*receiverReportFile);
  auto const publisherFileAfterReceiverLookup =
      readServiceReportFile(*publisherReportFile);
  auto const receiverHandleLookupAppend = receiverFileAfterHandleLookup.substr(
      receiverFileBeforeHandleLookup.size());
  require(
      receiverFileAfterHandleLookup.size() >
              receiverFileBeforeHandleLookup.size() &&
          receiverHandleLookupAppend.find(
              "\"HLAservice\":\"GetParameterHandle\"") !=
              std::string::npos &&
          receiverHandleLookupAppend.find(expectedReturnedArgument) !=
              std::string::npos,
      "Receiver's service report was not appended to its own HLAreportServiceFile");
  require(
      publisherFileAfterReceiverLookup == publisherFileBeforeReceiverLookup,
      "Receiver's GetParameterHandle service report was written to the publisher's HLAreportServiceFile");
  if (model == rti::HLA_EVOKED) {
    observer.pump();
  }
  require(
      observer.recorder().interactions().empty(),
      "Receiver file reporting delivered HLAreportServiceInvocation to a subscribed federate");

  observer.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  observer.rtiAmbassador().unsubscribeObjectClassAttributes(
      federateMomClass,
      federateMomAttributes);
  receiver.rtiAmbassador().unsubscribeInteractionClass(interactionClass);
  publisher.rtiAmbassador().setServiceReportingSwitch(false);
  publisher.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  receiver.resign(rti::NO_ACTION);
  observer.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  receiver.disconnect();
  publisher.disconnect();
}

namespace {

void requireInitialServiceReportingSwitch(
    Options const& options,
    rti::CallbackModel model,
    std::filesystem::path const& fomModule,
    bool const expectedValue,
    std::string_view const initialValueCase) {
  Session federate(options, model, "owner");
  auto const federation = federationName(
      options,
      std::string("service-reporting-fdd-initial-") +
          std::string(initialValueCase));
  federate.connect();
  federate.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{fomModule.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  federate.join(
      options.ownerFederateName + L"-service-reporting-fdd-initial",
      options.federateType,
      federation);

  require(
      federate.rtiAmbassador().getServiceReportingSwitch() == expectedValue,
      std::string("Initial HLAserviceReporting value did not follow the ") +
          std::string(initialValueCase) + " FDD");

  federate.resign(rti::NO_ACTION);
  federate.rtiAmbassador().destroyFederationExecution(federation);
  federate.disconnect();
}

}  // namespace

void scenarioFederateServiceReportingFddInitial(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.mimFom.empty() && !options.logicalTimeImplementationName.empty(),
      "FDD-initial Service Reporting testing requires standard MIM and logical-time configuration");

  auto const omittedFom =
      options.scenarioFoms.find("service-reporting-initial-omitted");
  auto const enabledFom =
      options.scenarioFoms.find("service-reporting-initial-enabled");
  require(
      omittedFom != options.scenarioFoms.end() &&
          enabledFom != options.scenarioFoms.end(),
      "FDD-initial Service Reporting testing requires omitted and enabled named FDD fixtures");

  requireInitialServiceReportingSwitch(
      options, model, omittedFom->second, false, "omitted");
  requireInitialServiceReportingSwitch(
      options, model, enabledFom->second, true, "explicitly enabled");
}

void scenarioServiceReportSaveFailureReasonFile(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.mimFom.empty() && !options.logicalTimeImplementationName.empty(),
      "SaveFailureReason service-report testing requires a standard MIM and logical-time configuration");
  require(
      !options.serviceReportDirectory.empty(),
      "SaveFailureReason service-report testing requires a service-report directory");

  Session subject(options, model, "service-report-save-failure-reason-subject");
  Session observer(options, model, "service-report-save-failure-reason-observer");
  auto const federation = federationName(options, "service-report-save-failure-reason");
  connectWithServiceReportDirectory(subject, options);
  observer.connect();
  subject.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  auto const subjectName =
      options.ownerFederateName + L"-service-report-save-failure-reason-subject";
  subject.join(subjectName, options.federateType, federation);
  require(
      !subject.rtiAmbassador().getSendServiceReportsToFileSwitch(),
      "SaveFailureReason initial-record coverage requires file reporting to be disabled at Join");
  observer.join(
      options.memberFederateName + L"-service-report-save-failure-reason-observer",
      options.federateType,
      federation);

  auto const federateMomClass = observer.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const federateNameAttribute = observer.rtiAmbassador().getAttributeHandle(
      federateMomClass,
      L"HLAfederateName");
  auto const reportFileAttribute = observer.rtiAmbassador().getAttributeHandle(
      federateMomClass,
      L"HLAreportServiceFile");
  require(
      federateMomClass.isValid() && federateNameAttribute.isValid() &&
          reportFileAttribute.isValid(),
      "Standard Federate MOM did not expose HLAreportServiceFile");
  rti::AttributeHandleSet const federateMomAttributes{
      federateNameAttribute,
      reportFileAttribute};
  observer.rtiAmbassador().subscribeObjectClassAttributes(
      federateMomClass,
      federateMomAttributes,
      true,
      L"");
  std::optional<std::filesystem::path> subjectReportFile;
  waitFor(
      observer,
      subject,
      [&] {
        subjectReportFile = advertisedServiceReportFile(
            observer.recorder(),
            federateNameAttribute,
            reportFileAttribute,
            subjectName);
        return subjectReportFile.has_value();
      },
      options,
      "subject HLAreportServiceFile MOM reflection");
  require(
      std::filesystem::is_regular_file(*subjectReportFile),
      "Subject HLAreportServiceFile did not name an existing report file");
  auto const initialContents = readServiceReportFile(*subjectReportFile);
  require(
      !initialContents.empty(),
      "Subject HLAreportServiceFile did not contain its initial record");
  auto const firstInitialRecord = firstJsonObjectRecord(initialContents);
  require(
      firstInitialRecord.has_value(),
      "HLAreportServiceFile did not begin with a complete JSON-like initial record");
  auto const configuration = configurationRecord(*firstInitialRecord);
  auto const federationAttributes =
      federationAttributesRecord(*firstInitialRecord);
  auto const federateAttributes = federateAttributesRecord(*firstInitialRecord);
  require(
      configuration.has_value() && federationAttributes.has_value() &&
          federateAttributes.has_value(),
      "Initial service-report record omitted one of the three Table 5 ServiceReportInitialRecord components");
  auto const expectedCallbackModel = model == rti::HLA_EVOKED
      ? std::string{"\"HLA_EVOKED\""}
      : std::string{"\"HLA_IMMEDIATE\""};
  auto const callbackModelValue =
      jsonStringToken(*configuration, "CallbackModel");
  auto const configurationNameValue =
      jsonStringToken(*configuration, "ConfigurationName");
  auto const rtiAddressValue = jsonStringToken(*configuration, "RTIaddress");
  auto const additionalSettingsValue =
      jsonStringToken(*configuration, "AdditionalSettings");
  auto const optionalInternalData =
      jsonObjectRecord(*configuration, "\"OptionalInternalData\":{");
  auto const configuredName = options.memberConfigurationName.empty()
      ? toWide(options.configurationName)
      : toWide(options.memberConfigurationName);
  auto const configuredAddress = toWide(options.rtiAddress);
  auto const configuredSettings =
      additionalSettingsWithServiceReportDirectory(options);
  require(
      callbackModelValue.has_value() &&
          *callbackModelValue == expectedCallbackModel &&
          configurationNameValue.has_value() &&
          *configurationNameValue == jsonStringLiteral(toNarrow(configuredName)) &&
          rtiAddressValue.has_value() &&
          *rtiAddressValue == jsonStringLiteral(toNarrow(configuredAddress)) &&
          additionalSettingsValue.has_value() &&
          *additionalSettingsValue ==
              jsonStringLiteral(toNarrow(configuredSettings)) &&
          optionalInternalData.has_value() &&
          *optionalInternalData == "{}",
      "First ServiceReportInitialRecord did not preserve the public Connect configuration values and Table 5 OptionalInternalData PairList object");
  std::string const expectedFederationName =
      std::string{"\""} + toNarrow(federation) + "\"";
  std::string const expectedTimeImplementation =
      std::string{"\""} + toNarrow(options.logicalTimeImplementationName) + "\"";
  auto const federationNameValue =
      jsonStringToken(*federationAttributes, "HLAfederationName");
  auto const rtiVersionValue =
      jsonStringToken(*federationAttributes, "HLARTIversion");
  auto const mimDesignatorValue =
      jsonStringToken(*federationAttributes, "HLAMIMDesignator");
  auto const fomModuleCount =
      jsonStringArrayLength(*federationAttributes, "HLAFOMmoduleDesignatorList");
  auto const timeImplementationValue =
      jsonStringToken(*federationAttributes, "HLAtimeImplementationName");
  auto const autoProvideValue =
      jsonBooleanValue(*federationAttributes, "HLAautoProvide");
  require(
      federationNameValue.has_value() &&
          *federationNameValue == expectedFederationName &&
          rtiVersionValue.has_value() && mimDesignatorValue.has_value() &&
          fomModuleCount.has_value() && *fomModuleCount > 0U &&
          timeImplementationValue.has_value() &&
          *timeImplementationValue == expectedTimeImplementation &&
          autoProvideValue.has_value(),
      "ServiceReportInitialRecord FederationAttributes did not encode the six Table 5 fields with their declared JSON types and configured values");
  auto const expectedFederateHandle = subject.rtiAmbassador().getFederateHandle(
      subjectName);
  std::string const quotedExpectedFederateHandle =
      std::string{"\""} + toNarrow(expectedFederateHandle.toString()) + "\"";
  std::string const quotedExpectedFederateName =
      std::string{"\""} + toNarrow(subjectName) + "\"";
  std::string const quotedExpectedFederateType =
      std::string{"\""} + toNarrow(options.federateType) + "\"";
  auto const federateHandleValue =
      jsonStringToken(*federateAttributes, "HLAfederateHandle");
  auto const federateNameValue =
      jsonStringToken(*federateAttributes, "HLAfederateName");
  auto const federateTypeValue =
      jsonStringToken(*federateAttributes, "HLAfederateType");
  auto const federateHostValue =
      jsonStringToken(*federateAttributes, "HLAfederateHost");
  auto const fomModuleDesignators =
      jsonStringArrayLength(*federateAttributes, "HLAFOMmoduleDesignatorList");
  require(
      federateHandleValue.has_value() &&
          *federateHandleValue == quotedExpectedFederateHandle &&
          federateNameValue.has_value() &&
          *federateNameValue == quotedExpectedFederateName &&
          federateTypeValue.has_value() &&
          *federateTypeValue == quotedExpectedFederateType &&
          federateHostValue.has_value() && fomModuleDesignators.has_value(),
      "Initial service-report record did not encode the five Table 5 FederateAttributes fields with their declared JSON types and joined-federate values");

  subject.rtiAmbassador().setServiceReportingSwitch(false);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(true);
  subject.rtiAmbassador().setServiceReportingSwitch(true);
  std::wstring const saveLabel = L"service-report-save-failure-reason";
  subject.rtiAmbassador().requestFederationSave(saveLabel);
  waitForSessions(
      {&subject, &observer},
      [&] {
        return subject.federateSaveInitiations().size() == 1U &&
            observer.federateSaveInitiations().size() == 1U;
      },
      options,
      "SaveFailureReason save-instruction callbacks");

  auto const beforeAbort = readServiceReportFile(*subjectReportFile);
  subject.rtiAmbassador().abortFederationSave();
  auto const afterAbort = readServiceReportFile(*subjectReportFile);
  auto const savedRecord = afterAbort.find(
      "\"HLAservice\":\"FederationSaved\"",
      beforeAbort.size());
  auto const failedSaveIndicator = afterAbort.find(
      "\"HLAargumentType\":6,\"HLAargumentName\":\"Federation save-success indicator\","
      "\"HLAargumentValue\":false",
      savedRecord);
  auto const failureArgument = afterAbort.find(
      "\"HLAargumentType\":48,\"HLAargumentName\":\"Optional failure reason\","
      "\"HLAargumentValue\":\"SAVE_ABORTED\"",
      savedRecord);
  require(
      savedRecord != std::string::npos &&
          failedSaveIndicator != std::string::npos &&
          failureArgument != std::string::npos,
      "Public HLAreportServiceFile did not append a false type-6 save-success indicator and the type-48 SAVE_ABORTED Table 5 value");
  if (model == rti::HLA_EVOKED) {
    require(
        subject.federationNotSavedReasons().empty(),
        "Evoked federationNotSaved callback ran before service-report file inspection");
  }
  waitForSessions(
      {&subject, &observer},
      [&] {
        return subject.federationNotSavedReasons().size() == 1U &&
            observer.federationNotSavedReasons().size() == 1U;
      },
      options,
      "SAVE_ABORTED Federation Not Saved callbacks");
  require(
      subject.federationNotSavedReasons() ==
              std::vector<rti::SaveFailureReason>{rti::SAVE_ABORTED} &&
          observer.federationNotSavedReasons() ==
              std::vector<rti::SaveFailureReason>{rti::SAVE_ABORTED},
      "Federation Not Saved callback did not report SAVE_ABORTED");

  std::wstring const federateReportedFailureLabel =
      L"service-report-federate-reported-save-failure";
  auto const subjectSaveInitiationsBeforeReportedFailure =
      subject.federateSaveInitiations().size();
  auto const observerSaveInitiationsBeforeReportedFailure =
      observer.federateSaveInitiations().size();
  subject.rtiAmbassador().requestFederationSave(federateReportedFailureLabel);
  waitForSessions(
      {&subject, &observer},
      [&] {
        return subject.federateSaveInitiations().size() >
                subjectSaveInitiationsBeforeReportedFailure &&
            observer.federateSaveInitiations().size() >
                observerSaveInitiationsBeforeReportedFailure;
      },
      options,
      "Federate-reported SaveFailureReason save initiation callbacks");
  subject.rtiAmbassador().federateSaveBegun();
  observer.rtiAmbassador().federateSaveBegun();
  auto const beforeFederateReportedFailure =
      readServiceReportFile(*subjectReportFile);
  observer.rtiAmbassador().federateSaveNotComplete();
  auto const afterFederateReportedFailure =
      readServiceReportFile(*subjectReportFile);
  auto const federateReportedFailureRecord =
      afterFederateReportedFailure.find(
          "\"HLAservice\":\"FederationSaved\"",
          beforeFederateReportedFailure.size());
  auto const federateReportedFailureArgument =
      afterFederateReportedFailure.find(
          "\"HLAargumentType\":48,\"HLAargumentName\":\"Optional failure reason\","
          "\"HLAargumentValue\":\"FEDERATE_REPORTED_FAILURE_DURING_SAVE\"",
          federateReportedFailureRecord);
  require(
      federateReportedFailureRecord != std::string::npos &&
          federateReportedFailureArgument != std::string::npos,
      "Public HLAreportServiceFile did not append the type-48 FEDERATE_REPORTED_FAILURE_DURING_SAVE Table 5 value");
  if (model == rti::HLA_EVOKED) {
    require(
        subject.federationNotSavedReasons().size() == 1U &&
            observer.federationNotSavedReasons().size() == 1U,
        "Evoked federationNotSaved callback ran before service-report file inspection");
  }
  waitForSessions(
      {&subject, &observer},
      [&] {
        return subject.federationNotSavedReasons().size() == 2U &&
            observer.federationNotSavedReasons().size() == 2U;
      },
      options,
      "FEDERATE_REPORTED_FAILURE_DURING_SAVE callbacks");
  require(
      subject.federationNotSavedReasons().back() ==
              rti::FEDERATE_REPORTED_FAILURE_DURING_SAVE &&
          observer.federationNotSavedReasons().back() ==
              rti::FEDERATE_REPORTED_FAILURE_DURING_SAVE,
      "Federation Not Saved callback did not report FEDERATE_REPORTED_FAILURE_DURING_SAVE");

  std::wstring const restoreSaveLabel =
      L"service-report-restore-failure-reason-save";
  auto const subjectSaveInitiationsBefore =
      subject.federateSaveInitiations().size();
  auto const observerSaveInitiationsBefore =
      observer.federateSaveInitiations().size();
  subject.rtiAmbassador().requestFederationSave(restoreSaveLabel);
  waitForSessions(
      {&subject, &observer},
      [&] {
        return subject.federateSaveInitiations().size() >
                subjectSaveInitiationsBefore &&
            observer.federateSaveInitiations().size() >
                observerSaveInitiationsBefore;
      },
      options,
      "RestoreFailureReason setup save initiation callbacks");
  subject.rtiAmbassador().federateSaveBegun();
  observer.rtiAmbassador().federateSaveBegun();
  auto const subjectSavedBefore = subject.federationSavedCount();
  auto const observerSavedBefore = observer.federationSavedCount();
  subject.rtiAmbassador().federateSaveComplete();
  observer.rtiAmbassador().federateSaveComplete();
  waitForSessions(
      {&subject, &observer},
      [&] {
        return subject.federationSavedCount() > subjectSavedBefore &&
            observer.federationSavedCount() > observerSavedBefore;
      },
      options,
      "RestoreFailureReason setup save completion callbacks");

  auto const subjectRestoreBefore =
      subject.federateRestoreInitiations().size();
  auto const observerRestoreBefore =
      observer.federateRestoreInitiations().size();
  auto const subjectRestoreBegunBefore =
      subject.federationRestoreBegunCount();
  auto const observerRestoreBegunBefore =
      observer.federationRestoreBegunCount();
  subject.rtiAmbassador().requestFederationRestore(restoreSaveLabel);
  waitForSessions(
      {&subject, &observer},
      [&] {
        return subject.federationRestoreRequestsSucceeded().size() > 0U &&
            subject.federationRestoreBegunCount() > subjectRestoreBegunBefore &&
            observer.federationRestoreBegunCount() > observerRestoreBegunBefore &&
            subject.federateRestoreInitiations().size() > subjectRestoreBefore &&
            observer.federateRestoreInitiations().size() > observerRestoreBefore;
      },
      options,
      "RestoreFailureReason restore initiation callbacks");

  auto const beforeRestoreAbort = readServiceReportFile(*subjectReportFile);
  subject.rtiAmbassador().abortFederationRestore();
  auto const afterRestoreAbort = readServiceReportFile(*subjectReportFile);
  auto const restoredRecord = afterRestoreAbort.find(
      "\"HLAservice\":\"FederationRestored\"",
      beforeRestoreAbort.size());
  auto const restoreFailureArgument = afterRestoreAbort.find(
      "\"HLAargumentType\":45,\"HLAargumentName\":\"Optional failure reason\","
      "\"HLAargumentValue\":\"RESTORE_ABORTED\"",
      restoredRecord);
  auto const restoreSuccessArgument = afterRestoreAbort.find(
      "\"HLAargumentName\":\"Federation restore-success indicator\","
      "\"HLAargumentValue\":false",
      restoredRecord);
  require(
      restoredRecord != std::string::npos &&
          restoreFailureArgument != std::string::npos &&
          restoreSuccessArgument != std::string::npos,
      "Public HLAreportServiceFile did not append the failed FederationRestored result and type-45 RESTORE_ABORTED Table 5 value");
  if (model == rti::HLA_EVOKED) {
    require(
        subject.federationNotRestoredReasons().empty(),
        "Evoked federationNotRestored callback ran before service-report file inspection");
  }
  waitForSessions(
      {&subject, &observer},
      [&] {
        return subject.federationNotRestoredReasons().size() == 1U &&
            observer.federationNotRestoredReasons().size() == 1U;
      },
      options,
      "RESTORE_ABORTED federationNotRestored callbacks");
  require(
      subject.federationNotRestoredReasons() ==
              std::vector<rti::RestoreFailureReason>{rti::RESTORE_ABORTED} &&
          observer.federationNotRestoredReasons() ==
              std::vector<rti::RestoreFailureReason>{rti::RESTORE_ABORTED},
      "Federation Not Restored callback did not report RESTORE_ABORTED");

  std::wstring const federateResignedSaveLabel =
      L"service-report-federate-resigned-during-save";
  auto const subjectSaveInitiationsBeforeResignation =
      subject.federateSaveInitiations().size();
  auto const observerSaveInitiationsBeforeResignation =
      observer.federateSaveInitiations().size();
  subject.rtiAmbassador().requestFederationSave(federateResignedSaveLabel);
  waitForSessions(
      {&subject, &observer},
      [&] {
        return subject.federateSaveInitiations().size() >
                subjectSaveInitiationsBeforeResignation &&
            observer.federateSaveInitiations().size() >
                observerSaveInitiationsBeforeResignation;
      },
      options,
      "Federate-resignation SaveFailureReason initiation callbacks");
  subject.rtiAmbassador().federateSaveBegun();
  observer.rtiAmbassador().federateSaveBegun();
  auto const subjectFailuresBeforeResignation =
      subject.federationNotSavedReasons().size();
  auto const beforeFederateResignation = readServiceReportFile(*subjectReportFile);
  observer.resign(rti::NO_ACTION);
  auto const afterFederateResignation = readServiceReportFile(*subjectReportFile);
  auto const federateResignedRecord = afterFederateResignation.find(
      "\"HLAservice\":\"FederationSaved\"",
      beforeFederateResignation.size());
  auto const federateResignedArgument = afterFederateResignation.find(
      "\"HLAargumentType\":48,\"HLAargumentName\":\"Optional failure reason\","
      "\"HLAargumentValue\":\"FEDERATE_RESIGNED_DURING_SAVE\"",
      federateResignedRecord);
  require(
      federateResignedRecord != std::string::npos &&
          federateResignedArgument != std::string::npos,
      "Remaining federate's HLAreportServiceFile did not append the type-48 FEDERATE_RESIGNED_DURING_SAVE Table 5 value");
  if (model == rti::HLA_EVOKED) {
    require(
        subject.federationNotSavedReasons().size() ==
            subjectFailuresBeforeResignation,
        "Evoked federationNotSaved callback ran before service-report file inspection");
  }
  waitForSessions(
      {&subject},
      [&] {
        return subject.federationNotSavedReasons().size() >
            subjectFailuresBeforeResignation;
      },
      options,
      "FEDERATE_RESIGNED_DURING_SAVE survivor callback");
  require(
      subject.federationNotSavedReasons().back() ==
          rti::FEDERATE_RESIGNED_DURING_SAVE,
      "Remaining federate did not receive FEDERATE_RESIGNED_DURING_SAVE");

  subject.rtiAmbassador().setServiceReportingSwitch(false);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  subject.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  subject.disconnect();
}

void scenarioServiceReportSaveFailureReasonFileContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportSaveFailureReasonFile(options, model);
}

void scenarioServiceReportSaveStatusFile(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.mimFom.empty() && !options.logicalTimeImplementationName.empty(),
      "SaveStatus service-report testing requires a standard MIM and logical-time configuration");
  require(
      !options.serviceReportDirectory.empty(),
      "SaveStatus service-report testing requires a service-report directory");

  Session subject(options, model, "service-report-save-status-subject");
  Session observer(options, model, "service-report-save-status-observer");
  auto const federation = federationName(options, "service-report-save-status");
  connectWithServiceReportDirectory(subject, options);
  observer.connect();
  subject.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  auto const subjectName = options.ownerFederateName + L"-service-report-save-status-subject";
  auto const observerName =
      options.memberFederateName + L"-service-report-save-status-observer";
  subject.join(subjectName, options.federateType, federation);
  observer.join(observerName, options.federateType, federation);

  auto const federateMomClass = observer.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const federateNameAttribute = observer.rtiAmbassador().getAttributeHandle(
      federateMomClass,
      L"HLAfederateName");
  auto const reportFileAttribute = observer.rtiAmbassador().getAttributeHandle(
      federateMomClass,
      L"HLAreportServiceFile");
  require(
      federateMomClass.isValid() && federateNameAttribute.isValid() &&
          reportFileAttribute.isValid(),
      "Standard Federate MOM did not expose HLAreportServiceFile");
  rti::AttributeHandleSet const federateMomAttributes{
      federateNameAttribute,
      reportFileAttribute};
  observer.rtiAmbassador().subscribeObjectClassAttributes(
      federateMomClass,
      federateMomAttributes,
      true,
      L"");
  std::optional<std::filesystem::path> subjectReportFile;
  waitFor(
      observer,
      subject,
      [&] {
        subjectReportFile = advertisedServiceReportFile(
            observer.recorder(),
            federateNameAttribute,
            reportFileAttribute,
            subjectName);
        return subjectReportFile.has_value();
      },
      options,
      "subject HLAreportServiceFile MOM reflection");
  require(
      std::filesystem::is_regular_file(*subjectReportFile),
      "Subject HLAreportServiceFile did not name an existing report file");
  auto const initialContents = readServiceReportFile(*subjectReportFile);
  require(
      !initialContents.empty(),
      "Subject HLAreportServiceFile did not contain its initial record");

  subject.rtiAmbassador().setServiceReportingSwitch(false);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  auto const subjectHandle = subject.rtiAmbassador().getFederateHandle(subjectName);
  auto const observerHandle = subject.rtiAmbassador().getFederateHandle(observerName);
  require(
      subjectHandle.isValid() && observerHandle.isValid(),
      "Joined federates did not have valid handles for SaveStatus response verification");
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(true);
  subject.rtiAmbassador().setServiceReportingSwitch(true);
  auto const beforeQuery = readServiceReportFile(*subjectReportFile);
  auto const subjectStatusCount = subject.federationSaveStatusResponses().size();
  subject.rtiAmbassador().queryFederationSaveStatus();
  auto const afterQuery = readServiceReportFile(*subjectReportFile);
  auto const responseServicePosition = afterQuery.find(
      "\"HLAservice\":\"FederationSaveStatusResponse\"",
      beforeQuery.size());
  require(
      responseServicePosition != std::string::npos,
      "Public HLAreportServiceFile did not record FederationSaveStatusResponse");
  auto const responseRecordStart = afterQuery.rfind('{', responseServicePosition);
  auto const responseRecordEnd = afterQuery.find('\n', responseServicePosition);
  require(
      responseRecordStart != std::string::npos,
      "FederationSaveStatusResponse was not contained in a JSON service-report record");
  auto const responseRecord = std::string_view(afterQuery).substr(
      responseRecordStart,
      (responseRecordEnd == std::string::npos ? afterQuery.size() : responseRecordEnd) -
          responseRecordStart);
  constexpr std::string_view saveStatusPairsMarker =
      "\"HLAargumentType\":17,\"HLAargumentName\":\"List of joined federates and save status for each\",\"HLAargumentValue\":[";
  auto const saveStatusPairsStart = responseRecord.find(saveStatusPairsMarker);
  require(
      saveStatusPairsStart != std::string_view::npos,
      "FederationSaveStatusResponse did not report the Table 5 FederateHandleSaveStatusPairSet argument");
  auto const pairsBegin = saveStatusPairsStart + saveStatusPairsMarker.size() - 1U;
  auto const pairsEnd = responseRecord.find(']', pairsBegin);
  require(
      pairsEnd != std::string_view::npos,
      "FederateHandleSaveStatusPairSet report value was not an array");
  auto const pairs = responseRecord.substr(pairsBegin, pairsEnd - pairsBegin + 1U);
  auto const expectedPair = [](rti::FederateHandle const& handle) {
    return std::string{"{\"handle\":\""} + toNarrow(handle.toString()) +
        "\",\"status\":\"NO_SAVE_IN_PROGRESS\"}";
  };
  auto const subjectPair = expectedPair(subjectHandle);
  auto const observerPair = expectedPair(observerHandle);
  auto const pairMarker = std::string_view{"{\"handle\":"};
  auto const firstPair = pairs.find(pairMarker);
  auto const secondPair = firstPair == std::string_view::npos
      ? std::string_view::npos
      : pairs.find(pairMarker, firstPair + pairMarker.size());
  auto const thirdPair = secondPair == std::string_view::npos
      ? std::string_view::npos
      : pairs.find(pairMarker, secondPair + pairMarker.size());
  require(
      firstPair != std::string_view::npos && secondPair != std::string_view::npos &&
          thirdPair == std::string_view::npos && pairs.find(subjectPair) != std::string_view::npos &&
          pairs.find(observerPair) != std::string_view::npos,
      "FederationSaveStatusResponse did not encode the two joined handle/status pairs with a string SaveStatus field");
  if (model == rti::HLA_EVOKED) {
    require(
        subject.federationSaveStatusResponses().size() == subjectStatusCount,
        "Evoked Federation Save Status callback ran before service-report file inspection");
  }
  waitForSessions(
      {&subject, &observer},
      [&] {
        return subject.federationSaveStatusResponses().size() == subjectStatusCount + 1U;
      },
      options,
      "NO_SAVE_IN_PROGRESS callbacks");

  subject.rtiAmbassador().setServiceReportingSwitch(false);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().unsubscribeObjectClassAttributes(
      federateMomClass,
      federateMomAttributes);
  observer.resign(rti::NO_ACTION);
  subject.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  subject.disconnect();
}

void scenarioServiceReportSaveStatusFileContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportSaveStatusFile(options, model);
}

void scenarioServiceReportServiceGroupFile(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.mimFom.empty() && !options.logicalTimeImplementationName.empty(),
      "ServiceGroup service-report testing requires a standard MIM and logical-time configuration");
  require(
      !options.serviceReportDirectory.empty(),
      "ServiceGroup service-report testing requires a service-report directory");

  Session subject(options, model, "service-report-service-group-subject");
  Session observer(options, model, "service-report-service-group-observer");
  auto const federation = federationName(options, "service-report-service-group");
  connectWithServiceReportDirectory(subject, options);
  observer.connect();
  subject.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  auto const subjectName = options.ownerFederateName + L"-service-report-service-group-subject";
  subject.join(subjectName, options.federateType, federation);
  observer.join(
      options.memberFederateName + L"-service-report-service-group-observer",
      options.federateType,
      federation);

  auto const federateMomClass = observer.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const federateNameAttribute = observer.rtiAmbassador().getAttributeHandle(
      federateMomClass,
      L"HLAfederateName");
  auto const reportFileAttribute = observer.rtiAmbassador().getAttributeHandle(
      federateMomClass,
      L"HLAreportServiceFile");
  require(
      federateMomClass.isValid() && federateNameAttribute.isValid() &&
          reportFileAttribute.isValid(),
      "Standard Federate MOM did not expose HLAreportServiceFile");
  rti::AttributeHandleSet const federateMomAttributes{
      federateNameAttribute,
      reportFileAttribute};
  observer.rtiAmbassador().subscribeObjectClassAttributes(
      federateMomClass,
      federateMomAttributes,
      true,
      L"");
  std::optional<std::filesystem::path> subjectReportFile;
  waitFor(
      observer,
      subject,
      [&] {
        subjectReportFile = advertisedServiceReportFile(
            observer.recorder(),
            federateNameAttribute,
            reportFileAttribute,
            subjectName);
        return subjectReportFile.has_value();
      },
      options,
      "subject HLAreportServiceFile MOM reflection");
  require(
      std::filesystem::is_regular_file(*subjectReportFile),
      "Subject HLAreportServiceFile did not name an existing report file");
  auto const initialContents = readServiceReportFile(*subjectReportFile);
  require(
      !initialContents.empty(),
      "Subject HLAreportServiceFile did not contain its initial record");

  subject.rtiAmbassador().setServiceReportingSwitch(false);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(true);
  subject.rtiAmbassador().setServiceReportingSwitch(true);
  struct ServiceGroupCase final {
    rti::ServiceGroup value;
    std::string_view designator;
  };
  constexpr std::array<ServiceGroupCase, 7U> serviceGroups{{
      {rti::FEDERATION_MANAGEMENT, "FEDERATION_MANAGEMENT"},
      {rti::DECLARATION_MANAGEMENT, "DECLARATION_MANAGEMENT"},
      {rti::OBJECT_MANAGEMENT, "OBJECT_MANAGEMENT"},
      {rti::OWNERSHIP_MANAGEMENT, "OWNERSHIP_MANAGEMENT"},
      {rti::TIME_MANAGEMENT, "TIME_MANAGEMENT"},
      {rti::DATA_DISTRIBUTION_MANAGEMENT, "DATA_DISTRIBUTION_MANAGEMENT"},
      {rti::SUPPORT_SERVICES, "SUPPORT_SERVICES"},
  }};
  auto previousContents = readServiceReportFile(*subjectReportFile);
  for (auto const& serviceGroup : serviceGroups) {
    auto const normalized =
        subject.rtiAmbassador().normalizeServiceGroup(serviceGroup.value);
    require(
        normalized == static_cast<unsigned long>(serviceGroup.value),
        "NormalizeServiceGroup returned the wrong numeric value for " +
            std::string{serviceGroup.designator});

    auto const afterNormalization = readServiceReportFile(*subjectReportFile);
    auto const normalizationRecord = afterNormalization.find(
        "\"HLAservice\":\"NormalizeServiceGroup\"",
        previousContents.size());
    auto const expectedServiceGroupArgument =
        std::string{
            "\"HLAargumentType\":50,\"HLAargumentName\":\"Service group indicator\","
            "\"HLAargumentValue\":\""} +
        std::string{serviceGroup.designator} + "\"";
    auto const serviceGroupArgument = afterNormalization.find(
        expectedServiceGroupArgument,
        normalizationRecord);
    require(
        normalizationRecord != std::string::npos &&
            serviceGroupArgument != std::string::npos,
        "Public HLAreportServiceFile did not record the Table 5 ServiceGroup value " +
            std::string{serviceGroup.designator});
    previousContents = afterNormalization;
  }

  subject.rtiAmbassador().setServiceReportingSwitch(false);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().unsubscribeObjectClassAttributes(
      federateMomClass,
      federateMomAttributes);
  observer.resign(rti::NO_ACTION);
  subject.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  subject.disconnect();
}

void scenarioServiceReportServiceGroupFileContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportServiceGroupFile(options, model);
}

void scenarioServiceReportFederationExecutionInformationFile(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.mimFom.empty() && !options.logicalTimeImplementationName.empty(),
      "Federation execution information reporting requires a standard MIM and logical-time configuration");
  require(
      !options.serviceReportDirectory.empty(),
      "Federation execution information reporting requires a service-report directory");

  Session subject(options, model, "service-report-federation-list-subject");
  Session observer(options, model, "service-report-federation-list-observer");
  auto const firstFederation = federationName(options, "service-report-federation-list-primary");
  auto const secondFederation = options.federationOverride.empty()
      ? federationName(options, "service-report-federation-list-secondary")
      : firstFederation + L"-secondary";
  auto const subjectName =
      options.ownerFederateName + L"-service-report-federation-list-subject";
  auto const observerName =
      options.memberFederateName + L"-service-report-federation-list-observer";

  connectWithServiceReportDirectory(subject, options);
  observer.connect();
  subject.rtiAmbassador().createFederationExecutionWithMIM(
      firstFederation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  subject.rtiAmbassador().createFederationExecutionWithMIM(
      secondFederation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(subjectName, options.federateType, firstFederation);
  observer.join(observerName, options.federateType, firstFederation);

  auto const federateMomClass = observer.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const federateNameAttribute = observer.rtiAmbassador().getAttributeHandle(
      federateMomClass,
      L"HLAfederateName");
  auto const reportFileAttribute = observer.rtiAmbassador().getAttributeHandle(
      federateMomClass,
      L"HLAreportServiceFile");
  require(
      federateMomClass.isValid() && federateNameAttribute.isValid() &&
          reportFileAttribute.isValid(),
      "Standard Federate MOM did not expose HLAreportServiceFile");
  rti::AttributeHandleSet const federateMomAttributes{
      federateNameAttribute,
      reportFileAttribute};
  observer.rtiAmbassador().subscribeObjectClassAttributes(
      federateMomClass,
      federateMomAttributes,
      true,
      L"");
  std::optional<std::filesystem::path> subjectReportFile;
  waitFor(
      observer,
      subject,
      [&] {
        subjectReportFile = advertisedServiceReportFile(
            observer.recorder(),
            federateNameAttribute,
            reportFileAttribute,
            subjectName);
        return subjectReportFile.has_value();
      },
      options,
      "subject HLAreportServiceFile MOM reflection");
  require(
      std::filesystem::is_regular_file(*subjectReportFile),
      "Subject HLAreportServiceFile did not name an existing report file");
  auto const initialContents = readServiceReportFile(*subjectReportFile);
  require(
      !initialContents.empty(),
      "Subject HLAreportServiceFile did not contain its initial record");

  subject.rtiAmbassador().setServiceReportingSwitch(false);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(true);
  subject.rtiAmbassador().setServiceReportingSwitch(true);
  auto const beforeList = readServiceReportFile(*subjectReportFile);
  subject.rtiAmbassador().listFederationExecutions();
  auto const afterList = readServiceReportFile(*subjectReportFile);
  auto const servicePosition = afterList.find(
      "\"HLAservice\":\"ListFederationExecutions\"",
      beforeList.size());
  require(
      servicePosition != std::string::npos,
      "Public HLAreportServiceFile did not record ListFederationExecutions");
  auto const recordLineBreak = afterList.rfind('\n', servicePosition);
  auto const recordStart = recordLineBreak == std::string::npos
      ? 0U
      : recordLineBreak + 1U;
  auto const recordEnd = afterList.find('\n', servicePosition);
  auto const record = std::string_view(afterList).substr(
      recordStart,
      (recordEnd == std::string::npos ? afterList.size() : recordEnd) - recordStart);
  constexpr std::string_view returnedValueMarker =
      "\"HLAargumentType\":22,\"HLAargumentValue\":[";
  auto const returnedValuePosition = record.find(returnedValueMarker);
  require(
      returnedValuePosition != std::string_view::npos,
      "ListFederationExecutions did not use the MIM FederationExecutionInformationSet type");
  auto const returnedValueStart = returnedValuePosition + returnedValueMarker.size() - 1U;
  auto const returnedValueEnd = record.find(']', returnedValueStart);
  require(
      returnedValueEnd != std::string_view::npos,
      "FederationExecutionInformationSet was not encoded as an array");
  auto const returnedValue = record.substr(
      returnedValueStart,
      returnedValueEnd - returnedValueStart + 1U);
  auto const informationRecord = [&](std::wstring const& name) {
    return std::string{"{\"federationExecutionName\":\""} + toNarrow(name) +
        "\",\"logicalTimeImplementationName\":\"" +
        toNarrow(options.logicalTimeImplementationName) + "\"}";
  };
  auto const expectedFirst = informationRecord(firstFederation);
  auto const expectedSecond = informationRecord(secondFederation);
  auto const expectedArray = std::string{"["} + expectedFirst + "," + expectedSecond + "]";
  auto const expectedReversedArray =
      std::string{"["} + expectedSecond + "," + expectedFirst + "]";
  require(
      returnedValue == expectedArray || returnedValue == expectedReversedArray,
      "FederationExecutionInformationSet did not contain exactly the two federation-name/logical-time records");

  std::map<std::wstring, std::wstring> const expectedFederations{
      {firstFederation, options.logicalTimeImplementationName},
      {secondFederation, options.logicalTimeImplementationName}};
  waitFor(
      subject,
      [&] { return subject.recorder().listsFederationsInSingleReport(expectedFederations); },
      options,
      "the complete federation execution report callback");

  subject.rtiAmbassador().setServiceReportingSwitch(false);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().unsubscribeObjectClassAttributes(
      federateMomClass,
      federateMomAttributes);
  observer.resign(rti::NO_ACTION);
  subject.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(secondFederation);
  subject.rtiAmbassador().destroyFederationExecution(firstFederation);
  observer.disconnect();
  subject.disconnect();
}

void scenarioServiceReportFederationExecutionInformationFileContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportFederationExecutionInformationFile(options, model);
}

void scenarioServiceReportFederationExecutionMembersFile(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.mimFom.empty() && !options.logicalTimeImplementationName.empty(),
      "Federation execution member reporting requires a standard MIM and logical-time configuration");
  require(
      !options.serviceReportDirectory.empty(),
      "Federation execution member reporting requires a service-report directory");

  Session subject(options, model, "service-report-federation-members-subject");
  Session observer(options, model, "service-report-federation-members-observer");
  auto const federation = federationName(options, "service-report-federation-members");
  auto const subjectName =
      options.ownerFederateName + L"-service-report-federation-members-subject";
  auto const observerName =
      options.memberFederateName + L"-service-report-federation-members-observer";
  auto const expectedMembers = std::map<std::wstring, std::wstring>{
      {subjectName, options.federateType},
      {observerName, options.federateType}};

  connectWithServiceReportDirectory(subject, options);
  observer.connect();
  subject.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(subjectName, options.federateType, federation);
  observer.join(observerName, options.federateType, federation);

  auto const federateMomClass = observer.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const federateNameAttribute = observer.rtiAmbassador().getAttributeHandle(
      federateMomClass,
      L"HLAfederateName");
  auto const reportFileAttribute = observer.rtiAmbassador().getAttributeHandle(
      federateMomClass,
      L"HLAreportServiceFile");
  require(
      federateMomClass.isValid() && federateNameAttribute.isValid() &&
          reportFileAttribute.isValid(),
      "Standard Federate MOM did not expose HLAreportServiceFile");
  rti::AttributeHandleSet const federateMomAttributes{
      federateNameAttribute,
      reportFileAttribute};
  observer.rtiAmbassador().subscribeObjectClassAttributes(
      federateMomClass,
      federateMomAttributes,
      true,
      L"");
  std::optional<std::filesystem::path> subjectReportFile;
  waitFor(
      observer,
      subject,
      [&] {
        subjectReportFile = advertisedServiceReportFile(
            observer.recorder(),
            federateNameAttribute,
            reportFileAttribute,
            subjectName);
        return subjectReportFile.has_value();
      },
      options,
      "subject HLAreportServiceFile MOM reflection");
  require(
      std::filesystem::is_regular_file(*subjectReportFile),
      "Subject HLAreportServiceFile did not name an existing report file");
  auto const initialContents = readServiceReportFile(*subjectReportFile);
  require(
      !initialContents.empty(),
      "Subject HLAreportServiceFile did not contain its initial record");

  subject.rtiAmbassador().setServiceReportingSwitch(false);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(true);
  subject.rtiAmbassador().setServiceReportingSwitch(true);
  auto const beforeList = readServiceReportFile(*subjectReportFile);
  subject.rtiAmbassador().listFederationExecutionMembers(federation);
  auto const afterList = readServiceReportFile(*subjectReportFile);
  auto const servicePosition = afterList.find(
      "\"HLAservice\":\"ListFederationExecutionMembers\"",
      beforeList.size());
  require(
      servicePosition != std::string::npos,
      "Public HLAreportServiceFile did not record ListFederationExecutionMembers");
  auto const recordLineBreak = afterList.rfind('\n', servicePosition);
  auto const recordStart = recordLineBreak == std::string::npos
      ? 0U
      : recordLineBreak + 1U;
  auto const recordEnd = afterList.find('\n', servicePosition);
  auto const record = std::string_view(afterList).substr(
      recordStart,
      (recordEnd == std::string::npos ? afterList.size() : recordEnd) - recordStart);
  constexpr std::string_view returnedValueMarker =
      "\"HLAargumentType\":24,\"HLAargumentValue\":[";
  auto const returnedValuePosition = record.find(returnedValueMarker);
  require(
      returnedValuePosition != std::string_view::npos,
      "ListFederationExecutionMembers did not use the MIM FederationExecutionMemberSet type");
  auto const returnedValueStart = returnedValuePosition + returnedValueMarker.size() - 1U;
  auto const returnedValueEnd = record.find(']', returnedValueStart);
  require(
      returnedValueEnd != std::string_view::npos,
      "FederationExecutionMemberSet was not encoded as an array");
  auto const returnedValue = record.substr(
      returnedValueStart,
      returnedValueEnd - returnedValueStart + 1U);
  auto const memberRecord = [&](std::wstring const& name) {
    return std::string{"{\"federateName\":\""} + toNarrow(name) +
        "\",\"federateType\":\"" + toNarrow(options.federateType) + "\"}";
  };
  auto const expectedFirst = memberRecord(subjectName);
  auto const expectedSecond = memberRecord(observerName);
  auto const expectedArray = std::string{"["} + expectedFirst + "," + expectedSecond + "]";
  auto const expectedReversedArray =
      std::string{"["} + expectedSecond + "," + expectedFirst + "]";
  require(
      returnedValue == expectedArray || returnedValue == expectedReversedArray,
      "FederationExecutionMemberSet did not contain exactly both joined federates and their name/type fields");

  waitFor(
      subject,
      [&] {
        return subject.recorder().listsMembersInSingleReport(
            federation, expectedMembers);
      },
      options,
      "the complete federation execution member report callback");

  subject.rtiAmbassador().setServiceReportingSwitch(false);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().unsubscribeObjectClassAttributes(
      federateMomClass,
      federateMomAttributes);
  observer.resign(rti::NO_ACTION);
  subject.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  subject.disconnect();
}

void scenarioServiceReportFederationExecutionMembersFileContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportFederationExecutionMembersFile(options, model);
}

} // namespace hla_rti_cpp_tck::detail
