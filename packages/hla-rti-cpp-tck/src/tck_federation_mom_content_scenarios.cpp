#include "tck_federation_mom_content_scenarios.hpp"
#include "tck_main_scenarios.hpp"
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>
#include <type_traits>

namespace hla_rti_cpp_tck::detail {

namespace {

bool disconnectServiceUsed(std::wstring const& fdd) {
  // HLAcurrentFDD is a standard FDD, but XML whitespace, quote style, attribute
  // order, and namespace-prefix choice are not part of this test's contract.
  std::wregex const element(
      LR"(<(?:[A-Za-z_][A-Za-z_0-9.-]*:)?disconnect\b[^>]*>)");
  std::wregex const attribute(
      LR"((section|isCallback|isUsed)\s*=\s*(['"])([^'"]*)\2)");
  bool found = false;
  bool used = false;
  for (std::wsregex_iterator tag(fdd.begin(), fdd.end(), element), end;
       tag != end; ++tag) {
    std::wstring const xml = tag->str();
    std::map<std::wstring, std::wstring> values;
    for (std::wsregex_iterator field(xml.begin(), xml.end(), attribute);
         field != end; ++field) {
      values[(*field)[1].str()] = (*field)[3].str();
    }
    if (values[L"section"] == L"4.3" && values[L"isCallback"] == L"false") {
      require(!found, "current FDD duplicated the disconnect service row");
      require(values.count(L"isUsed") == 1U,
              "current FDD omitted disconnect service usage");
      found = true;
      used = values[L"isUsed"] == L"true";
      require(used || values[L"isUsed"] == L"false",
              "current FDD returned a non-Boolean disconnect usage value");
    }
  }
  require(found, "current FDD omitted the adapter's disconnect service row");
  return used;
}

std::wstring normalizedXmlText(std::wstring const& value) {
  std::wistringstream words(value);
  std::wstring token;
  std::wstring result;
  while (words >> token) {
    if (!result.empty()) result += L' ';
    result += token;
  }
  return result;
}

std::string normalizedXmlText(std::string const& value) {
  std::istringstream words(value);
  std::string token;
  std::string result;
  while (words >> token) {
    if (!result.empty()) result += ' ';
    result += token;
  }
  return result;
}

std::vector<std::string> xmlLeafContent(std::string const& xml) {
  std::regex const leaf(
      R"(<([A-Za-z_][A-Za-z_0-9.:-]*)\b[^>]*>([^<>]*)</\1\s*>)");
  std::vector<std::string> result;
  for (std::sregex_iterator entry(xml.begin(), xml.end(), leaf), end;
       entry != end; ++entry) {
    result.push_back((*entry)[1].str() + ":" +
                     normalizedXmlText((*entry)[2].str()));
  }
  return result;
}

std::string normalizedXmlLineEndings(std::string const& value) {
  std::string result;
  result.reserve(value.size());
  for (std::size_t index = 0; index != value.size(); ++index) {
    if (value[index] == '\r') {
      if (index + 1U != value.size() && value[index + 1U] == '\n') {
        ++index;
      }
      result += '\n';
    } else {
      result += value[index];
    }
  }
  return result;
}

std::string utf8XmlText(std::wstring const& value) {
  std::string result;
  result.reserve(value.size() * 2U);
  for (std::size_t index = 0; index != value.size(); ++index) {
    auto codePoint = static_cast<std::uint32_t>(
        static_cast<std::make_unsigned_t<wchar_t>>(value[index]));
    if constexpr (sizeof(wchar_t) == 2U) {
      if (codePoint >= 0xd800U && codePoint <= 0xdbffU) {
        require(index + 1U != value.size(),
                "Unicode XML report contained an incomplete UTF-16 surrogate pair");
        auto const low = static_cast<std::uint32_t>(
            static_cast<std::make_unsigned_t<wchar_t>>(value[++index]));
        require(low >= 0xdc00U && low <= 0xdfffU,
                "Unicode XML report contained an invalid UTF-16 surrogate pair");
        codePoint = 0x10000U + ((codePoint - 0xd800U) << 10U) +
                    (low - 0xdc00U);
      } else {
        require(codePoint < 0xdc00U || codePoint > 0xdfffU,
                "Unicode XML report contained an unmatched UTF-16 low surrogate");
      }
    } else {
      require(codePoint < 0xd800U || codePoint > 0xdfffU,
              "Unicode XML report contained a UTF-32 surrogate value");
    }
    require(codePoint <= 0x10ffffU,
            "Unicode XML report contained a code point outside the Unicode range");
    if (codePoint <= 0x7fU) {
      result += static_cast<char>(codePoint);
    } else if (codePoint <= 0x7ffU) {
      result += static_cast<char>(0xc0U | (codePoint >> 6U));
      result += static_cast<char>(0x80U | (codePoint & 0x3fU));
    } else if (codePoint <= 0xffffU) {
      result += static_cast<char>(0xe0U | (codePoint >> 12U));
      result += static_cast<char>(0x80U | ((codePoint >> 6U) & 0x3fU));
      result += static_cast<char>(0x80U | (codePoint & 0x3fU));
    } else {
      result += static_cast<char>(0xf0U | (codePoint >> 18U));
      result += static_cast<char>(0x80U | ((codePoint >> 12U) & 0x3fU));
      result += static_cast<char>(0x80U | ((codePoint >> 6U) & 0x3fU));
      result += static_cast<char>(0x80U | (codePoint & 0x3fU));
    }
  }
  return normalizedXmlLineEndings(result);
}

std::string readXmlFile(std::filesystem::path const& path,
                        std::string const& description) {
  std::ifstream input(path, std::ios::binary);
  require(static_cast<bool>(input),
          "could not open " + description + " XML test input");
  std::ostringstream content;
  content << input.rdbuf();
  require(!input.bad(), "could not read " + description + " XML test input");
  std::string const bytes = content.str();
  require(!bytes.empty(), description + " XML test input was empty");
  return normalizedXmlLineEndings(bytes);
}

void requireDistinctReferencedNotes(std::wstring const& fdd) {
  std::wregex const note(
      LR"(<(?:[A-Za-z_][A-Za-z_0-9.-]*:)?note\b[^>]*>([\s\S]*?)</(?:[A-Za-z_][A-Za-z_0-9.-]*:)?note\s*>)");
  std::wregex const label(
      LR"(<(?:[A-Za-z_][A-Za-z_0-9.-]*:)?label\b[^>]*>([\s\S]*?)</(?:[A-Za-z_][A-Za-z_0-9.-]*:)?label\s*>)");
  std::wregex const semantics(
      LR"(<(?:[A-Za-z_][A-Za-z_0-9.-]*:)?semantics\b[^>]*>([\s\S]*?)</(?:[A-Za-z_][A-Za-z_0-9.-]*:)?semantics\s*>)");
  std::map<std::wstring, std::wstring> labelsByMeaning;
  for (std::wsregex_iterator item(fdd.begin(), fdd.end(), note), end;
       item != end; ++item) {
    std::wstring const body = (*item)[1].str();
    std::wsmatch labelMatch;
    std::wsmatch semanticsMatch;
    if (!std::regex_search(body, labelMatch, label) ||
        !std::regex_search(body, semanticsMatch, semantics)) continue;
    auto const meaning = normalizedXmlText(semanticsMatch[1].str());
    if (meaning == L"TCK first colliding note semantics." ||
        meaning == L"TCK second colliding note semantics.") {
      require(labelsByMeaning.emplace(meaning, normalizedXmlText(labelMatch[1].str())).second,
              "current FDD duplicated a contributing note");
    }
  }
  auto const first = labelsByMeaning.find(L"TCK first colliding note semantics.");
  auto const second = labelsByMeaning.find(L"TCK second colliding note semantics.");
  require(first != labelsByMeaning.end() && second != labelsByMeaning.end() &&
              !first->second.empty() && !second->second.empty() &&
              first->second != second->second,
          "current FDD did not retain both notes with distinct labels");

  std::wregex const disconnect(
      LR"(<(?:[A-Za-z_][A-Za-z_0-9.-]*:)?disconnect\b[^>]*>)");
  std::wregex const attribute(
      LR"((section|isCallback|noteReferences)\s*=\s*(['"])([^'"]*)\2)");
  bool found = false;
  for (std::wsregex_iterator tag(fdd.begin(), fdd.end(), disconnect), end;
       tag != end; ++tag) {
    std::wstring const xml = tag->str();
    std::map<std::wstring, std::wstring> attributes;
    for (std::wsregex_iterator field(xml.begin(), xml.end(), attribute);
         field != end; ++field) {
      attributes[(*field)[1].str()] = (*field)[3].str();
    }
    if (attributes[L"section"] != L"4.3" ||
        attributes[L"isCallback"] != L"false") continue;
    require(!found, "current FDD duplicated the referenced disconnect row");
    found = true;
    std::wistringstream references(attributes[L"noteReferences"]);
    std::set<std::wstring> referenceLabels;
    std::wstring reference;
    while (references >> reference) referenceLabels.insert(reference);
    require(referenceLabels.count(first->second) == 1U &&
                referenceLabels.count(second->second) == 1U,
            "current FDD disconnect row did not reference both remapped notes");
  }
  require(found, "current FDD omitted the referenced disconnect row");
}

}  // namespace

std::wstring composedCurrentFdd(Options const& options, rti::CallbackModel model,
                                std::vector<std::wstring> const& modules,
                                std::string const& suffix) {
  Session creator(options, model, suffix + "-creator");
  Session observer(options, model, suffix + "-observer");
  auto const federation = federationName(options, suffix);
  creator.connect();
  observer.connect();
  creator.rtiAmbassador().createFederationExecutionWithMIM(
      federation, modules, options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  observer.join(options.memberFederateName, options.federateType, federation);
  auto const federationClass = observer.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederation");
  auto const currentFdd = observer.rtiAmbassador().getAttributeHandle(
      federationClass, L"HLAcurrentFDD");
  rti::AttributeHandleSet const attributes{currentFdd};
  observer.rtiAmbassador().subscribeObjectClassAttributes(
      federationClass, attributes, true, L"");
  waitFor(observer, [&] {
    auto const discovery = observer.recorder().discovery();
    return discovery.present && discovery.objectClass == federationClass &&
        discovery.name == L"HLAfederation";
  }, options, "the federation MOM object for composed FDD");
  auto const object = observer.recorder().discovery().object;
  observer.recorder().clearReflection();
  observer.rtiAmbassador().requestAttributeValueUpdate(
      object, attributes, rti::VariableLengthData{});
  waitFor(observer, [&] {
    auto const reflection = observer.recorder().reflection();
    return reflection.present && reflection.object == object &&
        reflection.values.count(currentFdd) == 1U;
  }, options, "the composed current FDD");
  rti::HLAunicodeString decoded;
  decoded.decode(observer.recorder().reflection().values.at(currentFdd));
  std::wstring value = decoded.get();
  observer.resign(rti::NO_ACTION);
  creator.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  creator.disconnect();
  return value;
}

void scenarioFomServiceUsageMerge(Options const& options,
                                  rti::CallbackModel model) {
  require(!options.mimFom.empty(),
          "service-usage merge requires an adapter-supplied standard MIM");
  auto const first = options.scenarioFoms.find("service-usage-first");
  auto const second = options.scenarioFoms.find("service-usage-second");
  require(first != options.scenarioFoms.end() &&
              second != options.scenarioFoms.end(),
          "service-usage merge requires two adapter-supplied FOM modules");

  auto const readUsage = [&](std::vector<std::wstring> const& modules,
                             bool expected) {
    require(disconnectServiceUsed(composedCurrentFdd(
                options, model, modules, "fom-service-usage-merge")) == expected,
            "current FDD did not logically OR the disconnect service usage");
  };
  readUsage({first->second.wstring()}, false);
  readUsage({second->second.wstring()}, true);
  readUsage({first->second.wstring(), second->second.wstring()}, true);
  readUsage({second->second.wstring(), first->second.wstring()}, true);
}

void scenarioFomServiceUsageMergeContract(Options const& options,
                                          rti::CallbackModel model) {
  scenarioFomServiceUsageMerge(options, model);
}

void scenarioFomNoteCollisionRemap(Options const& options,
                                   rti::CallbackModel model) {
  require(!options.mimFom.empty(),
          "note-remapping requires an adapter-supplied standard MIM");
  auto const first = options.scenarioFoms.find("note-collision-first");
  auto const second = options.scenarioFoms.find("note-collision-second");
  require(first != options.scenarioFoms.end() &&
              second != options.scenarioFoms.end(),
          "note-remapping requires two adapter-supplied FOM modules");
  requireDistinctReferencedNotes(composedCurrentFdd(
      options, model, {first->second.wstring(), second->second.wstring()},
      "fom-note-collision-remap"));
  requireDistinctReferencedNotes(composedCurrentFdd(
      options, model, {second->second.wstring(), first->second.wstring()},
      "fom-note-collision-remap-reversed"));
}

void scenarioFomNoteCollisionRemapContract(Options const& options,
                                           rti::CallbackModel model) {
  scenarioFomNoteCollisionRemap(options, model);
}

void scenarioFederationMomCurrentFdd(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.mimFom.empty(),
      "Federation MOM current-FDD testing requires an adapter-supplied standard MIM");
  require(
      !options.additionalFomModules.empty(),
      "Federation MOM current-FDD testing requires an adapter-supplied extension FOM");

  Session owner(options, model, "federation-mom-current-fdd-owner");
  Session observer(options, model, "federation-mom-current-fdd-observer");
  Session extension(options, model, "federation-mom-current-fdd-extension");
  auto const federation = federationName(options, "federation-mom-current-fdd");
  owner.connect();
  observer.connect();
  extension.connect();
  owner.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName, options.federateType, federation);
  observer.join(options.memberFederateName, options.federateType, federation);

  auto const federationClass = observer.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederation");
  auto const currentFddAttribute = observer.rtiAmbassador().getAttributeHandle(
      federationClass,
      L"HLAcurrentFDD");
  auto const reliable = observer.rtiAmbassador().getTransportationTypeHandle(
      L"HLAreliable");
  require(
      federationClass.isValid() && currentFddAttribute.isValid() && reliable.isValid(),
      "Federation MOM current-FDD lookup returned an invalid standard handle");

  rti::AttributeHandleSet const currentFddAttributes{currentFddAttribute};
  observer.rtiAmbassador().subscribeObjectClassAttributes(
      federationClass,
      currentFddAttributes,
      true,
      L"");
  waitFor(
      observer,
      [&] {
        auto const discovery = observer.recorder().discovery();
        return discovery.present && discovery.objectClass == federationClass &&
            discovery.name == L"HLAfederation";
      },
      options,
      "the standard federation MOM object discovery");

  auto const federationObject = observer.recorder().discovery().object;
  require(
      observer.rtiAmbassador().getKnownObjectClassHandle(federationObject) ==
          federationClass,
      "Federation MOM current-FDD discovery returned the wrong known class");
  require(
      observer.rtiAmbassador().getObjectInstanceName(federationObject) ==
          L"HLAfederation",
      "Federation MOM current-FDD discovery returned the wrong object name");
  require(
      observer.rtiAmbassador().getObjectInstanceHandle(L"HLAfederation") ==
          federationObject,
      "Federation MOM current-FDD object identity did not round-trip");

  observer.recorder().clearTransportationRecords();
  observer.rtiAmbassador().queryAttributeTransportationType(
      federationObject,
      currentFddAttribute);
  waitFor(
      observer,
      [&] { return !observer.recorder().attributeTransportationReports().empty(); },
      options,
      "the federation MOM current-FDD transportation report");
  auto const transportationReports = observer.recorder().attributeTransportationReports();
  auto const& transportation = transportationReports.back();
  require(
      transportation.object == federationObject &&
          transportation.attribute == currentFddAttribute &&
          transportation.transportation == reliable,
      "Federation MOM current-FDD reported the wrong transportation type");

  auto requestCurrentFdd = [&](std::string const& description) {
    observer.recorder().clearReflection();
    observer.rtiAmbassador().requestAttributeValueUpdate(
        federationObject,
        currentFddAttributes,
        rti::VariableLengthData{});
    waitFor(
        observer,
        [&] {
          auto const reflection = observer.recorder().reflection();
          return reflection.present && reflection.object == federationObject &&
              reflection.values.count(currentFddAttribute) == 1U;
        },
        options,
        description);
    auto const reflection = observer.recorder().reflection();
    require(
        reflection.transportation == reliable && reflection.tag.empty() &&
            !reflection.producer.isValid() && !reflection.regions.has_value(),
        description + " returned non-standard MOM callback metadata");
    rti::HLAunicodeString currentFdd;
    currentFdd.decode(reflection.values.at(currentFddAttribute));
    require(!currentFdd.get().empty(), description + " returned an empty HLAcurrentFDD");
    return currentFdd.get();
  };

  auto const baseFdd = requestCurrentFdd("the base current-FDD request");
  require(
      baseFdd.find(L"TckObject") != std::wstring::npos,
      "the base current-FDD value omitted the adapter FOM declaration");
  require(
      baseFdd.find(L"<name>HLAserviceReporting</name>") != std::wstring::npos &&
          baseFdd.find(L"<name>HLAsetSwitches</name>") != std::wstring::npos &&
          baseFdd.find(L"<name>HLAreportServiceInvocation</name>") != std::wstring::npos,
      "the current FDD omitted selected predefined MOM elements while "
      "only the federation-current-FDD attribute is used");
  require(
      baseFdd.find(L"TckExtensionObject") == std::wstring::npos,
      "the base current-FDD value already contained the extension declaration");

  observer.recorder().clearReflection();
  extension.join(
      options.memberFederateName + L"-current-fdd-extension",
      options.federateType,
      federation,
      fomModuleNames(options.additionalFomModules));
  waitFor(
      observer,
      [&] {
        auto const reflection = observer.recorder().reflection();
        if (!reflection.present || reflection.object != federationObject) {
          return false;
        }
        auto const value = reflection.values.find(currentFddAttribute);
        if (value == reflection.values.end()) {
          return false;
        }
        rti::HLAunicodeString currentFdd;
        currentFdd.decode(value->second);
        return currentFdd.get().find(L"TckExtensionObject") != std::wstring::npos;
      },
      options,
      "the refreshed current-FDD reflection after an additional-FOM join");
  auto const refreshedReflection = observer.recorder().reflection();
  rti::HLAunicodeString refreshedFdd;
  refreshedFdd.decode(refreshedReflection.values.at(currentFddAttribute));
  auto const refreshedValue = refreshedFdd.get();
  require(
      refreshedValue.find(L"TckObject") != std::wstring::npos &&
          refreshedValue.find(L"TckExtensionObject") != std::wstring::npos &&
          refreshedValue.size() > baseFdd.size(),
      "the refreshed current-FDD value omitted the composed extension declaration");

  auto const directlyRequestedFdd = requestCurrentFdd(
      "the post-composition current-FDD request");
  require(
      directlyRequestedFdd == refreshedValue,
      "the direct current-FDD request disagreed with the join-triggered refresh");

  extension.resign(rti::NO_ACTION);
  auto const retainedFdd = requestCurrentFdd(
      "the current-FDD request after the supplying federate resigned");
  require(
      retainedFdd.find(L"TckExtensionObject") != std::wstring::npos &&
          retainedFdd.find(L"ExtensionValue") != std::wstring::npos &&
          retainedFdd.find(L"TckExtensionInteger") != std::wstring::npos,
      "the federation current FDD dropped declarations from a joined module after its supplying federate resigned");
  auto const retainedClass = observer.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.TckExtensionObject");
  auto const retainedAttribute = observer.rtiAmbassador().getAttributeHandle(
      retainedClass,
      L"ExtensionValue");
  require(
      retainedClass.isValid() && retainedAttribute.isValid() &&
          observer.rtiAmbassador().getObjectClassName(retainedClass) ==
              L"HLAobjectRoot.TckExtensionObject" &&
          observer.rtiAmbassador().getAttributeName(
              retainedClass, retainedAttribute) == L"ExtensionValue",
      "the public FDD lookup surface lost a joined module declaration after its supplying federate resigned");
  observer.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  extension.disconnect();
  observer.disconnect();
  owner.disconnect();
}

namespace {

std::wstring localFileUrl(std::filesystem::path const& file) {
  auto const utf8 = std::filesystem::absolute(file).generic_u8string();
  std::string path;
  path.reserve(utf8.size());
  for (auto const byte : utf8) {
    path.push_back(static_cast<char>(byte));
  }
  require(!path.empty(), "file URL test requires an adapter-supplied local FOM");
  auto const lastSlash = path.find_last_of('/');
  auto const filenameStart = lastSlash == std::string::npos ? 0U : lastSlash + 1U;
  auto const alphanumeric = [](unsigned char byte) {
    return (byte >= 'A' && byte <= 'Z') || (byte >= 'a' && byte <= 'z') ||
        (byte >= '0' && byte <= '9');
  };
  std::string url = path.starts_with("//") ? "file:" : "file://";
  if (path.front() != '/') {
    url.push_back('/');
  }
  constexpr char hex[] = "0123456789ABCDEF";
  for (std::size_t index = 0; index < path.size(); ++index) {
    auto const byte = static_cast<unsigned char>(path[index]);
    auto const allowed = alphanumeric(byte) || byte == '/' || byte == ':' ||
        byte == '-' || byte == '.' || byte == '_' || byte == '~';
    if (!allowed || (index == filenameStart && alphanumeric(byte))) {
      url.push_back('%');
      url.push_back(hex[byte >> 4U]);
      url.push_back(hex[byte & 0x0FU]);
    } else {
      url.push_back(static_cast<char>(byte));
    }
  }
  return {url.begin(), url.end()};
}

void scenarioFederationMomFomModuleDesignatorListWithBase(
    Options const& options,
    rti::CallbackModel model,
    std::wstring const& baseDesignator,
    bool withExplicitMim) {
  require(
      !options.fom.empty() && !options.additionalFomModules.empty() &&
          (withExplicitMim ? !options.mimFom.empty()
                           : !options.logicalTimeImplementationName.empty()),
      "Federation MOM FOM-module designator testing requires its selected FOM, MIM, and time inputs");
  if (withExplicitMim) {
    require(
        !options.dataTypeMergeProviderFom.empty(),
        "Federation MOM designator filtering requires the adapter-supplied data-type-only FOM module");
  }
  auto const additionalModules = fomModuleNames(options.additionalFomModules);
  require(
      std::find(additionalModules.begin(), additionalModules.end(),
                baseDesignator) == additionalModules.end(),
      "Federation MOM FOM-module designator testing requires a distinct additional module");

  Session owner(options, model, "federation-mom-fom-designators-owner");
  Session observer(options, model, "federation-mom-fom-designators-observer");
  Session extension(options, model, "federation-mom-fom-designators-extension");
  auto const federation = federationName(options, "federation-mom-fom-designators");
  owner.connect();
  observer.connect();
  extension.connect();
  std::vector<std::wstring> createModuleDesignators{baseDesignator};
  if (withExplicitMim) {
    auto const duplicateContentDesignator = localFileUrl(options.fom);
    require(
        duplicateContentDesignator != baseDesignator,
        "identical FOM content test requires distinct module designators");
    createModuleDesignators.push_back(duplicateContentDesignator);
  }
  if (!withExplicitMim) {
    require(!options.logicalTimeImplementationName.empty(),
            "file URL FOM designator requires adapter-supplied logical time");
    owner.rtiAmbassador().createFederationExecution(
        federation,
        createModuleDesignators,
        options.logicalTimeImplementationName);
  } else if (options.logicalTimeImplementationName.empty()) {
    owner.rtiAmbassador().createFederationExecutionWithMIM(
        federation,
        createModuleDesignators,
        options.mimFom.wstring());
  } else {
    owner.rtiAmbassador().createFederationExecutionWithMIM(
        federation,
        createModuleDesignators,
        options.mimFom.wstring(),
        options.logicalTimeImplementationName);
  }
  auto const ownerName = options.ownerFederateName + L"-fom-designators-owner";
  auto const observerName = options.memberFederateName + L"-fom-designators-observer";
  auto const extensionName = options.memberFederateName + L"-fom-designators-extension";
  owner.join(ownerName, options.federateType, federation);
  observer.join(observerName, options.federateType, federation);

  auto const federationClass = observer.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederation");
  auto const designatorAttribute = observer.rtiAmbassador().getAttributeHandle(
      federationClass,
      L"HLAFOMmoduleDesignatorList");
  auto const reliable = observer.rtiAmbassador().getTransportationTypeHandle(
      L"HLAreliable");
  require(
      federationClass.isValid() && designatorAttribute.isValid() && reliable.isValid() &&
          observer.rtiAmbassador().getObjectClassName(federationClass) ==
              L"HLAobjectRoot.HLAmanager.HLAfederation" &&
          observer.rtiAmbassador().getAttributeName(federationClass, designatorAttribute) ==
              L"HLAFOMmoduleDesignatorList" &&
          observer.rtiAmbassador().getTransportationTypeName(reliable) == L"HLAreliable",
      "Federation MOM FOM-module designator lookup did not resolve standard MIM handles");
  rti::AttributeHandleSet const designatorAttributes{designatorAttribute};
  observer.rtiAmbassador().subscribeObjectClassAttributes(
      federationClass,
      designatorAttributes,
      true,
      L"");
  waitFor(
      observer,
      [&] {
        auto const discovery = observer.recorder().discovery();
        return discovery.present && discovery.objectClass == federationClass &&
            discovery.name == L"HLAfederation";
      },
      options,
      "standard federation MOM discovery for the FOM-module designator list");
  auto const federationObject = observer.recorder().discovery().object;
  require(
      observer.rtiAmbassador().getKnownObjectClassHandle(federationObject) ==
              federationClass &&
          observer.rtiAmbassador().getObjectInstanceName(federationObject) ==
              L"HLAfederation" &&
          observer.rtiAmbassador().getObjectInstanceHandle(L"HLAfederation") ==
              federationObject,
      "Federation MOM FOM-module designator discovery did not preserve standard object identity");

  auto decodeDesignators = [](rti::VariableLengthData const& encoded) {
    rti::HLAvariableArrayT<rti::HLAunicodeString> values;
    values.decode(encoded);
    std::vector<std::wstring> result;
    result.reserve(values.size());
    for (std::size_t index = 0; index < values.size(); ++index) {
      result.push_back(
          dynamic_cast<rti::HLAunicodeString const&>(values.get(index)).get());
    }
    return result;
  };
  auto canonicalizeDesignators = [](std::vector<std::wstring> values) {
    std::sort(values.begin(), values.end());
    require(
        std::adjacent_find(values.begin(), values.end()) == values.end(),
        "HLAFOMmoduleDesignatorList contained a duplicate module designator");
    return values;
  };
  auto const initialDesignators = canonicalizeDesignators(
      std::vector<std::wstring>{baseDesignator});
  auto const allDesignators = [&] {
    auto result = initialDesignators;
    result.insert(result.end(), additionalModules.begin(), additionalModules.end());
    return canonicalizeDesignators(std::move(result));
  }();

  auto verifyDesignatorReflection = [&](
      std::vector<std::wstring> const& expected,
      std::string const& description) {
    observer.recorder().clearReflection();
    observer.rtiAmbassador().requestAttributeValueUpdate(
        federationObject,
        designatorAttributes,
        rti::VariableLengthData{});
    waitFor(
        observer,
        [&] {
          auto const reflection = observer.recorder().reflection();
          return reflection.present && reflection.object == federationObject &&
              reflection.values.count(designatorAttribute) == 1U;
        },
        options,
        description);
    auto const reflection = observer.recorder().reflection();
    require(
        reflection.values.size() == 1U && reflection.transportation == reliable &&
            reflection.tag.empty() && !reflection.producer.isValid() &&
            !reflection.regions.has_value(),
        description + " returned incomplete data or non-standard MOM callback metadata");
    auto const actual = canonicalizeDesignators(
        decodeDesignators(reflection.values.at(designatorAttribute)));
    require(
        actual == expected,
        description + " returned " + std::to_string(actual.size()) +
            " FOM-module designators; expected " + std::to_string(expected.size()));
    return actual;
  };

  auto const initialValue = verifyDesignatorReflection(
      initialDesignators,
      "the requested initial federation FOM-module designator list");
  require(
      initialValue.size() == 1U,
      "the created federation FOM-module designator list had an unexpected size");

  if (!withExplicitMim) {
    auto const mimDesignatorAttribute = observer.rtiAmbassador().getAttributeHandle(
        federationClass,
        L"HLAMIMdesignator");
    require(
        mimDesignatorAttribute.isValid() &&
            observer.rtiAmbassador().getAttributeName(
                federationClass,
                mimDesignatorAttribute) == L"HLAMIMdesignator",
        "Federation MOM automatic MIM-designator lookup returned an invalid handle");
    rti::AttributeHandleSet const mimDesignatorAttributes{mimDesignatorAttribute};
    observer.rtiAmbassador().subscribeObjectClassAttributes(
        federationClass,
        mimDesignatorAttributes,
        true,
        L"");
    observer.recorder().clearReflection();
    observer.rtiAmbassador().requestAttributeValueUpdate(
        federationObject,
        mimDesignatorAttributes,
        rti::VariableLengthData{});
    waitFor(
        observer,
        [&] {
          auto const reflection = observer.recorder().reflection();
          return reflection.present && reflection.object == federationObject &&
              reflection.values.count(mimDesignatorAttribute) == 1U;
        },
        options,
        "the federation MOM designator for the automatically supplied standard MIM");
    auto const mimReflection = observer.recorder().reflection();
    require(
        mimReflection.values.size() == 1U &&
            mimReflection.transportation == reliable && mimReflection.tag.empty() &&
            !mimReflection.producer.isValid() && !mimReflection.regions.has_value(),
        "the automatic standard-MIM designator reflection returned non-standard metadata");
    rti::HLAunicodeString mimDesignator;
    mimDesignator.decode(mimReflection.values.at(mimDesignatorAttribute));
    require(
        mimDesignator.get() == L"HLAstandardMIM",
        "the automatically supplied standard MIM did not use its required HLAstandardMIM designator");
    observer.rtiAmbassador().unsubscribeObjectClassAttributes(
        federationClass,
        mimDesignatorAttributes);
  }

  observer.recorder().clearReflection();
  extension.join(
      extensionName,
      options.federateType,
      federation,
      additionalModules);
  waitFor(
      observer,
      [&] {
        auto const reflection = observer.recorder().reflection();
        if (!reflection.present || reflection.object != federationObject ||
            reflection.values.count(designatorAttribute) != 1U) {
          return false;
        }
        return canonicalizeDesignators(
                   decodeDesignators(reflection.values.at(designatorAttribute))) ==
            allDesignators;
      },
      options,
      "the conditional federation FOM-module designator reflection after Join Federation Execution");
  auto const joinedReflection = observer.recorder().reflection();
  require(
      joinedReflection.values.size() == 1U &&
          joinedReflection.transportation == reliable && joinedReflection.tag.empty() &&
          !joinedReflection.producer.isValid() && !joinedReflection.regions.has_value(),
      "the join-triggered federation FOM-module designator reflection returned non-standard metadata");
  auto const joinedValue = canonicalizeDesignators(
      decodeDesignators(joinedReflection.values.at(designatorAttribute)));
  require(
      joinedValue == allDesignators && joinedValue.size() == 1U + additionalModules.size(),
      "the join-triggered federation FOM-module designator list omitted or duplicated modules");

  auto const requestedAfterJoin = verifyDesignatorReflection(
      allDesignators,
      "the requested federation FOM-module designator list after composition");
  require(
      requestedAfterJoin == joinedValue,
      "the requested federation designator list disagreed with the join-triggered update");

  if (withExplicitMim) {
    auto const currentFddAttribute = observer.rtiAmbassador().getAttributeHandle(
        federationClass,
        L"HLAcurrentFDD");
    require(
        currentFddAttribute.isValid(),
        "Federation MOM current-FDD lookup returned an invalid handle during designator filtering");
    rti::AttributeHandleSet const currentFddAttributes{currentFddAttribute};
    observer.rtiAmbassador().subscribeObjectClassAttributes(
        federationClass,
        currentFddAttributes,
        true,
        L"");
    auto requestCurrentFdd = [&](std::string const& description) {
      observer.recorder().clearReflection();
      observer.rtiAmbassador().requestAttributeValueUpdate(
          federationObject,
          currentFddAttributes,
          rti::VariableLengthData{});
      waitFor(
          observer,
          [&] {
            auto const reflection = observer.recorder().reflection();
            return reflection.present && reflection.object == federationObject &&
                reflection.values.count(currentFddAttribute) == 1U;
          },
          options,
          description);
      rti::HLAunicodeString currentFdd;
      currentFdd.decode(
          observer.recorder().reflection().values.at(currentFddAttribute));
      return currentFdd.get();
    };

    auto const currentFddBeforeDataTypeJoin = requestCurrentFdd(
        "the current FDD before joining the data-type-only module");
    require(
        currentFddBeforeDataTypeJoin.find(L"TckMergeValue") == std::wstring::npos,
        "the current FDD unexpectedly contained the data-type-only module's declaration before Join");

    Session dataTypeOnly(options, model, "federation-mom-data-type-only");
    dataTypeOnly.connect();
    dataTypeOnly.join(
        options.memberFederateName + L"-designator-data-type-only",
        options.federateType,
        federation,
        std::vector<std::wstring>{options.dataTypeMergeProviderFom.wstring()});
    require(
        dataTypeOnly.federateHandle().isValid(),
        "the data-type-only FOM module did not join the existing federation");

    auto const currentFddAfterDataTypeJoin = requestCurrentFdd(
        "the current FDD after joining the data-type-only module");
    require(
        currentFddAfterDataTypeJoin.find(L"TckMergeValue") != std::wstring::npos,
        "the current FDD omitted the data-type-only module's merged declaration");
    auto const designatorsAfterDataTypeJoin = verifyDesignatorReflection(
        allDesignators,
        "the federation FOM-module designator list after the data-type-only Join");
    require(
        designatorsAfterDataTypeJoin == allDesignators,
        "a data-type-only module was incorrectly included in HLAFOMmoduleDesignatorList");

    observer.rtiAmbassador().unsubscribeObjectClassAttributes(
        federationClass,
        currentFddAttributes);
    dataTypeOnly.resign(rti::NO_ACTION);
    dataTypeOnly.disconnect();
  }

  observer.rtiAmbassador().unsubscribeObjectClassAttributes(
      federationClass,
      designatorAttributes);
  extension.resign(rti::NO_ACTION);
  observer.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  extension.disconnect();
  observer.disconnect();
  owner.disconnect();
}

}  // namespace

void scenarioFederationMomFomModuleDesignatorList(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederationMomFomModuleDesignatorListWithBase(
      options, model, options.fom.wstring(), true);
}

void scenarioFomFileUrlDesignator(Options const& options,
                                  rti::CallbackModel model) {
  require(std::filesystem::is_regular_file(options.fom),
          "file URL FOM designator requires an adapter-supplied local FOM file");
  scenarioFederationMomFomModuleDesignatorListWithBase(
      options, model, localFileUrl(options.fom), false);

  auto const restaurantFom =
      options.scenarioFoms.find("official-restaurant-example");
  require(
      restaurantFom != options.scenarioFoms.end() &&
          std::filesystem::is_regular_file(restaurantFom->second),
      "file URL FOM designator requires the adapter-supplied official 2025 Restaurant FOM");
  scenarioFederationMomFomModuleDesignatorListWithBase(
      options, model, localFileUrl(restaurantFom->second), false);
}

void scenarioFomFileUrlDesignatorContract(Options const& options,
                                          rti::CallbackModel model) {
  scenarioFomFileUrlDesignator(options, model);
}

void scenarioFederationMomContentReports(
    Options const& options,
    rti::CallbackModel model) {
  Session subject(options, model, "federation-mom-content-subject");
  Session observer(options, model, "federation-mom-content-observer");
  auto const federation = federationName(options, "federation-mom-content-reports");
  auto const secondFom = options.scenarioFoms.find("service-usage-second");
  require(!options.fom.empty() && !options.mimFom.empty() &&
              secondFom != options.scenarioFoms.end(),
          "federation MOM content reports require the adapter FOM, standard MIM, "
          "and second service-usage FOM module");
  std::vector<std::wstring> const fomModules{
      options.fom.wstring(), secondFom->second.wstring()};
  subject.connect();
  observer.connect();
  subject.rtiAmbassador().createFederationExecutionWithMIM(
      federation, fomModules, options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(options.ownerFederateName, options.federateType, federation);
  observer.join(options.memberFederateName, options.federateType, federation);

  auto const fomRequestClass = subject.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederation.HLArequest.HLArequestFOMmoduleData");
  auto const fomRequestIndicator = subject.rtiAmbassador().getParameterHandle(
      fomRequestClass,
      L"HLAFOMmoduleIndicator");
  auto const fomReportClass = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederation.HLAreport.HLAreportFOMmoduleData");
  auto const fomReportIndicator = observer.rtiAmbassador().getParameterHandle(
      fomReportClass,
      L"HLAFOMmoduleIndicator");
  auto const fomReportData = observer.rtiAmbassador().getParameterHandle(
      fomReportClass,
      L"HLAFOMmoduleData");
  std::wstring const mimRequestName =
      L"HLAinteractionRoot.HLAmanager.HLAfederation.HLArequest.HLArequestMIMdata";
  std::wstring const mimReportName =
      L"HLAinteractionRoot.HLAmanager.HLAfederation.HLAreport.HLAreportMIMdata";
  auto const mimRequestClass =
      subject.rtiAmbassador().getInteractionClassHandle(mimRequestName);
  auto const mimReportClass =
      observer.rtiAmbassador().getInteractionClassHandle(mimReportName);
  auto const mimReportData = observer.rtiAmbassador().getParameterHandle(
      mimReportClass,
      L"HLAMIMdata");
  requireException(
      [&] {
        static_cast<void>(observer.rtiAmbassador().getParameterHandle(
            mimReportClass, L"HLAMIMData"));
      },
      L"NameNotFound",
      "Clause 11 HLAMIMData spelling when Annex G defines HLAMIMdata");
  auto const reliable = observer.rtiAmbassador().getTransportationTypeHandle(
      L"HLAreliable");
  auto const federationMomClass = observer.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederation");
  auto const currentFddAttribute =
      observer.rtiAmbassador().getAttributeHandle(
          federationMomClass,
          L"HLAcurrentFDD");
  require(
      fomRequestClass.isValid() && fomRequestIndicator.isValid() &&
          fomReportClass.isValid() && fomReportIndicator.isValid() &&
          fomReportData.isValid() && mimRequestClass.isValid() &&
          mimReportClass.isValid() && mimReportData.isValid() && reliable.isValid() &&
          federationMomClass.isValid() && currentFddAttribute.isValid(),
      "Federation MOM content-report lookup returned an invalid standard handle");
  require(
      subject.rtiAmbassador().getInteractionClassName(mimRequestClass) ==
              mimRequestName &&
          observer.rtiAmbassador().getInteractionClassName(mimReportClass) ==
              mimReportName,
      "the RTI did not preserve the exact Annex G MIM interaction names");
  require(
      observer.rtiAmbassador().getParameterName(mimReportClass, mimReportData) ==
          L"HLAMIMdata",
      "the Annex G MIM report parameter handle did not resolve to its exact lowercase name");
  auto const resolvedMimReportParameterName =
      observer.rtiAmbassador().getParameterName(mimReportClass, mimReportData);
  require(
      observer.rtiAmbassador().getParameterHandle(
          mimReportClass, resolvedMimReportParameterName) == mimReportData,
      "the exact Annex G MIM parameter name did not resolve back to its original handle");
  requireException(
      [&] {
        static_cast<void>(subject.rtiAmbassador().getInteractionClassHandle(
            L"HLAinteractionRoot.HLAmanager.HLAfederation.HLArequest.HLArequestMIMData"));
      },
      L"NameNotFound",
      "Clause 11 HLArequestMIMData spelling when Annex G defines HLArequestMIMdata");
  requireException(
      [&] {
        static_cast<void>(observer.rtiAmbassador().getInteractionClassHandle(
            L"HLAinteractionRoot.HLAmanager.HLAfederation.HLAreport.HLAreportMIMData"));
      },
      L"NameNotFound",
      "Clause 11 HLAreportMIMData spelling when Annex G defines HLAreportMIMdata");

  rti::AttributeHandleSet const currentFddAttributes{currentFddAttribute};
  observer.recorder().clearDiscovery();
  observer.rtiAmbassador().subscribeObjectClassAttributes(
      federationMomClass,
      currentFddAttributes,
      true,
      L"");
  waitFor(
      observer,
      [&] {
        auto const discovery = observer.recorder().discovery();
        return discovery.present && discovery.objectClass == federationMomClass &&
            discovery.name == L"HLAfederation";
      },
      options,
      "the Federation MOM object for composed-FDD identification");
  auto const federationMomObject = observer.recorder().discovery().object;
  require(
      federationMomObject.isValid() &&
          observer.rtiAmbassador().getKnownObjectClassHandle(federationMomObject) ==
              federationMomClass &&
          observer.rtiAmbassador().getObjectInstanceName(federationMomObject) ==
              L"HLAfederation",
      "the Federation MOM object did not expose its standard identity");
  observer.recorder().clearReflection();
  observer.rtiAmbassador().requestAttributeValueUpdate(
      federationMomObject,
      currentFddAttributes,
      rti::VariableLengthData{});
  waitFor(
      observer,
      [&] {
        auto const reflection = observer.recorder().reflection();
        return reflection.present && reflection.object == federationMomObject &&
            reflection.values.count(currentFddAttribute) == 1U;
      },
      options,
      "the composed Current FDD Object Model Identification table");
  auto const currentFddReflection = observer.recorder().reflection();
  require(
      currentFddReflection.values.size() == 1U &&
          currentFddReflection.transportation == reliable &&
          currentFddReflection.tag.empty() &&
          !currentFddReflection.producer.isValid() &&
          !currentFddReflection.regions.has_value(),
      "Current FDD reflection returned non-standard MOM metadata");
  rti::HLAunicodeString decodedCurrentFdd;
  decodedCurrentFdd.decode(currentFddReflection.values.at(currentFddAttribute));
  std::string const currentFddXml = utf8XmlText(decodedCurrentFdd.get());
  std::regex const modelIdentification(
      R"(<(?:[A-Za-z_][A-Za-z_0-9.-]*:)?modelIdentification\b[^>]*>([\s\S]*?)</(?:[A-Za-z_][A-Za-z_0-9.-]*:)?modelIdentification\s*>)");
  std::smatch modelIdentificationMatch;
  require(
      std::regex_search(currentFddXml, modelIdentificationMatch, modelIdentification),
      "the composed Current FDD omitted its modelIdentification table");
  std::string const modelIdentificationXml = modelIdentificationMatch[1].str();
  std::regex const referenceElement(
      R"(<(?:[A-Za-z_][A-Za-z_0-9.-]*:)?reference\b[^>]*>([\s\S]*?)</(?:[A-Za-z_][A-Za-z_0-9.-]*:)?reference\s*>)");
  std::regex const referenceIdentification(
      R"(<(?:[A-Za-z_][A-Za-z_0-9.-]*:)?identification\b[^>]*>([^<>]*)</(?:[A-Za-z_][A-Za-z_0-9.-]*:)?identification\s*>)");
  std::vector<std::string> modelReferences;
  for (std::sregex_iterator reference(
           modelIdentificationXml.begin(), modelIdentificationXml.end(),
           referenceElement),
       end;
       reference != end;
       ++reference) {
    std::string const referenceXml = (*reference)[1].str();
    std::smatch identificationMatch;
    if (std::regex_search(
            referenceXml,
            identificationMatch,
            referenceIdentification)) {
      modelReferences.push_back(
          normalizedXmlText(identificationMatch[1].str()));
    }
  }
  auto const sourceModelName = [](std::filesystem::path const& path,
                                 std::string const& description) {
    auto const sourceXml = readXmlFile(path, description);
    std::smatch identificationMatch;
    std::regex const identificationElement(
        R"(<(?:[A-Za-z_][A-Za-z_0-9.-]*:)?modelIdentification\b[^>]*>([\s\S]*?)</(?:[A-Za-z_][A-Za-z_0-9.-]*:)?modelIdentification\s*>)");
    require(
        std::regex_search(sourceXml, identificationMatch, identificationElement),
        description + " omitted its modelIdentification table");
    std::string const identificationXml = identificationMatch[1].str();
    std::smatch nameMatch;
    std::regex const nameElement(
        R"(<(?:[A-Za-z_][A-Za-z_0-9.-]*:)?name\b[^>]*>([^<>]*)</(?:[A-Za-z_][A-Za-z_0-9.-]*:)?name\s*>)");
    require(
        std::regex_search(identificationXml, nameMatch, nameElement),
        description + " omitted its model name");
    auto const name = normalizedXmlText(nameMatch[1].str());
    require(!name.empty(), description + " supplied an empty model name");
    return name;
  };
  auto const firstModelName = sourceModelName(options.fom, "primary FOM module");
  auto const secondModelName = sourceModelName(
      secondFom->second,
      "second FOM module");
  auto const formatNames = [](std::vector<std::string> const& values) {
    std::string result{"{"};
    for (std::size_t index = 0U; index < values.size(); ++index) {
      if (index != 0U) result += ", ";
      result += values[index];
    }
    result += "}";
    return result;
  };
  std::vector<std::string> const expectedModelNames{
      firstModelName,
      secondModelName};
  require(
      firstModelName != secondModelName &&
          std::count(modelReferences.begin(), modelReferences.end(), firstModelName) ==
              1 &&
          std::count(modelReferences.begin(), modelReferences.end(), secondModelName) ==
              1,
      "the composed Object Model Identification table omitted or duplicated a "
      "contributing FOM module reference; actual=" + formatNames(modelReferences) +
          " expected-at-least=" + formatNames(expectedModelNames) +
          " modelIdentification=" + modelIdentificationXml);

  observer.rtiAmbassador().subscribeInteractionClass(fomReportClass);
  observer.rtiAmbassador().subscribeInteractionClass(mimReportClass);

  auto const hasInteraction = [&](rti::InteractionClassHandle const& expected) {
    auto const record = observer.recorder().interaction();
    return record.present && record.interaction == expected;
  };
  auto const sendFomRequest = [&](std::int32_t moduleIndex) {
    rti::ParameterHandleValueMap parameters;
    parameters.emplace(
        fomRequestIndicator, rti::HLAinteger32BE{moduleIndex}.encode());
    subject.rtiAmbassador().sendInteraction(
        fomRequestClass,
        parameters,
        rti::VariableLengthData{});
  };

  // In evoked mode this first request exercises subscription revalidation at
  // callback time. In immediate mode the callback boundary has already been
  // crossed, so it establishes the corresponding immediate-delivery contract.
  observer.recorder().clearInteraction();
  sendFomRequest(0);
  if (model == rti::HLA_EVOKED) {
    observer.rtiAmbassador().unsubscribeInteractionClass(fomReportClass);
    for (int pass = 0; pass != 8; ++pass) {
      observer.pump();
    }
    require(
        observer.recorder().interactions().empty(),
        "evoked MOM FOM-module report survived callback-time unsubscription");
    observer.rtiAmbassador().subscribeInteractionClass(fomReportClass);
  } else {
    waitFor(
        observer,
        [&] { return hasInteraction(fomReportClass); },
        options,
        "the immediate federation MOM FOM-module content report");
    require(
        observer.recorder().interactions().size() == 1U,
        "immediate MOM FOM-module request delivered more than one report");
    observer.rtiAmbassador().unsubscribeInteractionClass(fomReportClass);
    observer.rtiAmbassador().subscribeInteractionClass(fomReportClass);
  }

  auto const checkFomModuleReport = [&](
      std::int32_t moduleIndex,
      std::filesystem::path const& modulePath,
      std::wstring const& expectedMarker) {
    observer.recorder().clearInteraction();
    sendFomRequest(moduleIndex);
    waitFor(
        observer,
        [&] { return hasInteraction(fomReportClass); },
        options,
        "the federation MOM FOM-module report for index " +
            std::to_string(moduleIndex));
    auto const report = observer.recorder().interaction();
    require(
        report.parameters.size() == 2U &&
            report.parameters.count(fomReportIndicator) == 1U &&
            report.parameters.count(fomReportData) == 1U,
        "federation MOM FOM-module report returned the wrong parameter set");
    require(
        report.tag.empty() && report.transportation == reliable &&
            !report.producer.isValid() && !report.regions.has_value(),
        "federation MOM FOM-module report returned non-standard callback metadata");
    rti::HLAinteger32BE decodedIndicator;
    decodedIndicator.decode(report.parameters.at(fomReportIndicator));
    require(
        decodedIndicator.get() == moduleIndex,
        "federation MOM FOM-module report returned the wrong module indicator");
    rti::HLAunicodeString decodedFom;
    decodedFom.decode(report.parameters.at(fomReportData));
    auto const actualFomLeafContent = xmlLeafContent(utf8XmlText(decodedFom.get()));
    auto const expectedFomLeafContent = xmlLeafContent(readXmlFile(
        modulePath, "adapter FOM module " + std::to_string(moduleIndex)));
    require(!expectedFomLeafContent.empty(),
            "adapter FOM module did not contain inspectable XML leaf content");
    std::size_t firstDifferentFomLeaf = 0;
    while (firstDifferentFomLeaf != actualFomLeafContent.size() &&
           firstDifferentFomLeaf != expectedFomLeafContent.size() &&
           actualFomLeafContent[firstDifferentFomLeaf] ==
               expectedFomLeafContent[firstDifferentFomLeaf]) {
      ++firstDifferentFomLeaf;
    }
    require(actualFomLeafContent == expectedFomLeafContent,
            "federation MOM FOM-module report differed from source module " +
                std::to_string(moduleIndex) + " ordered XML leaf content at element " +
                std::to_string(firstDifferentFomLeaf) + " (report elements " +
                std::to_string(actualFomLeafContent.size()) + ", source elements " +
                std::to_string(expectedFomLeafContent.size()) + ")");
    require(!expectedMarker.empty() &&
                decodedFom.get().find(expectedMarker) != std::wstring::npos,
            "federation MOM FOM-module report omitted its expected source-module marker");
  };
  checkFomModuleReport(0, options.fom, L"TckObject");
  checkFomModuleReport(1, secondFom->second, L"TCK Service Usage Second");

  observer.recorder().clearInteraction();
  subject.rtiAmbassador().sendInteraction(
      mimRequestClass,
      rti::ParameterHandleValueMap{},
      rti::VariableLengthData{});
  waitFor(
      observer,
      [&] { return hasInteraction(mimReportClass); },
      options,
      "the federation MOM MIM content report");
  auto const mimReport = observer.recorder().interaction();
  require(
      mimReport.parameters.size() == 1U &&
          mimReport.parameters.count(mimReportData) == 1U,
      "federation MOM MIM report returned the wrong parameter set");
  require(
      mimReport.tag.empty() && mimReport.transportation == reliable &&
          !mimReport.producer.isValid() && !mimReport.regions.has_value(),
      "federation MOM MIM report returned non-standard callback metadata");
  rti::HLAunicodeString decodedMim;
  decodedMim.decode(mimReport.parameters.at(mimReportData));
  auto const actualMim = utf8XmlText(decodedMim.get());
  auto const expectedMim = readXmlFile(options.mimFom, "pinned standard MIM");
  auto const actualMimLeafContent = xmlLeafContent(actualMim);
  auto const expectedMimLeafContent = xmlLeafContent(expectedMim);
  require(expectedMimLeafContent.size() > 1000U,
          "standard MIM comparison did not inspect its expected leaf content");
  std::size_t firstDifferentLeaf = 0;
  while (firstDifferentLeaf != actualMimLeafContent.size() &&
         firstDifferentLeaf != expectedMimLeafContent.size() &&
         actualMimLeafContent[firstDifferentLeaf] ==
             expectedMimLeafContent[firstDifferentLeaf]) {
    ++firstDifferentLeaf;
  }
  require(actualMimLeafContent == expectedMimLeafContent,
          "federation MOM MIM report differed from the pinned IEEE 1516-2025 XML "
          "leaf content at element " + std::to_string(firstDifferentLeaf) +
          " (report elements " + std::to_string(actualMimLeafContent.size()) +
          ", expected elements " +
          std::to_string(expectedMimLeafContent.size()) + ")");
  require(
      !decodedMim.get().empty() &&
          decodedMim.get().find(L"HLArequestMIMdata") != std::wstring::npos &&
          decodedMim.get().find(L"HLAreportMIMdata") != std::wstring::npos,
      "federation MOM MIM report did not carry standard MIM XML");
  require(
      decodedMim.get().find(
          L"Standard MOM and Initialization Module (MIM) for HLA IEEE 1516-2025") !=
          std::wstring::npos,
      "federation MOM MIM report omitted the Annex G standard-MIM model identification");
  auto const arrayDataTypesStart = decodedMim.get().find(L"<arrayDataTypes>");
  auto const arrayDataTypesEnd = decodedMim.get().find(
      L"</arrayDataTypes>", arrayDataTypesStart);
  require(
      arrayDataTypesStart != std::wstring::npos &&
          arrayDataTypesEnd != std::wstring::npos &&
          arrayDataTypesStart < arrayDataTypesEnd,
      "standard federation MOM MIM report omitted the array-data-type table");
  auto arrayEntryCursor = arrayDataTypesStart;
  auto requireNextPredefinedArray = [&](
      std::wstring const& expectedName,
      std::wstring const& expectedSemantics) {
    auto const entryStart = decodedMim.get().find(L"<arrayData>", arrayEntryCursor);
    auto const nameStart = decodedMim.get().find(L"<name>", entryStart);
    auto const nameEnd = decodedMim.get().find(L"</name>", nameStart);
    auto const entryEnd = decodedMim.get().find(L"</arrayData>", entryStart);
    auto const semanticsStart = decodedMim.get().find(L"<semantics>", nameEnd);
    auto const semanticsEnd = decodedMim.get().find(L"</semantics>", semanticsStart);
    require(
        entryStart != std::wstring::npos && entryStart < arrayDataTypesEnd &&
            entryEnd != std::wstring::npos && entryEnd < arrayDataTypesEnd &&
            nameStart != std::wstring::npos && nameStart < entryEnd &&
            nameEnd != std::wstring::npos && nameEnd < entryEnd &&
            semanticsStart != std::wstring::npos && semanticsStart < entryEnd &&
            semanticsEnd != std::wstring::npos && semanticsEnd < entryEnd,
        "standard federation MOM MIM report had a malformed leading array entry");
    require(
        decodedMim.get().substr(nameStart + 6U, nameEnd - (nameStart + 6U)) ==
            expectedName,
        "standard federation MOM MIM report omitted or reordered a predefined array type");
    auto const semanticsValueStart =
        semanticsStart + std::wstring(L"<semantics>").size();
    require(
        normalizedXmlText(decodedMim.get().substr(
            semanticsValueStart, semanticsEnd - semanticsValueStart)) ==
            expectedSemantics,
        "standard federation MOM MIM report omitted a predefined array type's standardized semantics");
    arrayEntryCursor = entryEnd + std::wstring(L"</arrayData>").size();
  };
  requireNextPredefinedArray(L"HLAASCIIstring", L"ASCII string representation");
  requireNextPredefinedArray(L"HLAunicodeString", L"Unicode string representation");
  requireNextPredefinedArray(L"HLAopaqueData", L"Uninterpreted sequence of bytes");
  requireNextPredefinedArray(L"HLAtoken", L"Token representation");

  // The request class has no parameters. Supplying a parameter from the
  // FOM-module request must therefore fail with the standard public exception
  // and must not synthesize an application-level report.
  observer.recorder().clearInteraction();
  rti::ParameterHandleValueMap invalidMimParameters;
  invalidMimParameters.emplace(fomRequestIndicator, rti::HLAinteger32BE{0}.encode());
  requireException(
      [&] {
        subject.rtiAmbassador().sendInteraction(
            mimRequestClass,
            invalidMimParameters,
            rti::VariableLengthData{});
      },
      L"InteractionParameterNotDefined",
      "MOM MIM request with an undefined parameter");
  for (int pass = 0; pass != 8; ++pass) {
    observer.pump();
  }
  require(
      observer.recorder().interactions().empty(),
      "malformed MOM MIM request produced an application-level report");

  observer.rtiAmbassador().unsubscribeInteractionClass(fomReportClass);
  observer.rtiAmbassador().unsubscribeInteractionClass(mimReportClass);
  observer.rtiAmbassador().unsubscribeObjectClassAttributes(
      federationMomClass,
      currentFddAttributes);
  observer.resign(rti::NO_ACTION);
  subject.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  subject.disconnect();
}

void scenarioFederationMomCurrentFddContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederationMomCurrentFdd(options, model);
}

void scenarioFederationMomFomModuleDesignatorListContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederationMomFomModuleDesignatorList(options, model);
}

void scenarioFederationMomContentReportsContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederationMomContentReports(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
