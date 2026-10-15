#include "tck_portable_mom_declaration_queries.hpp"
#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioFederateMomPublicationQueryState(
    Options const& options,
    rti::CallbackModel model,
    bool hasMultipleDirectedPublications,
    bool partiallyUnpublishDirectedPublicationAfterInitialQuery = false,
    bool unpublishAllDirectedPublicationsAfterPartialQuery = false,
    bool publishCompleteObjectClassAttributeSet = false,
    bool unpublishOneObjectClassAttributeAfterInitialQuery = false,
    bool unpublishAllObjectClassApplicationAttributesAfterInitialQuery = false,
    bool unpublishWholeObjectClassAfterInitialQuery = false,
    bool unpublishWholeInteractionClassAfterInitialQuery = false,
    bool unpublishOneOrdinaryInteractionClassAfterInitialQuery = false,
    bool unpublishAllOrdinaryInteractionClassesAfterInitialQuery = false) {
  require(
      !options.fom.empty() && !options.mimFom.empty() &&
          !options.logicalTimeImplementationName.empty() &&
          !options.objectClassName.empty() && !options.attributeName.empty() &&
          !options.interactionClassName.empty() &&
          (!partiallyUnpublishDirectedPublicationAfterInitialQuery ||
           hasMultipleDirectedPublications) &&
          (!unpublishAllDirectedPublicationsAfterPartialQuery ||
           partiallyUnpublishDirectedPublicationAfterInitialQuery) &&
          (!unpublishOneObjectClassAttributeAfterInitialQuery ||
           (publishCompleteObjectClassAttributeSet &&
            !hasMultipleDirectedPublications &&
            !partiallyUnpublishDirectedPublicationAfterInitialQuery &&
            !unpublishAllDirectedPublicationsAfterPartialQuery)) &&
          (!unpublishAllObjectClassApplicationAttributesAfterInitialQuery ||
           (publishCompleteObjectClassAttributeSet &&
            !unpublishOneObjectClassAttributeAfterInitialQuery &&
            !hasMultipleDirectedPublications &&
            !partiallyUnpublishDirectedPublicationAfterInitialQuery &&
            !unpublishAllDirectedPublicationsAfterPartialQuery)) &&
          (!unpublishWholeObjectClassAfterInitialQuery ||
           (!unpublishOneObjectClassAttributeAfterInitialQuery &&
            !unpublishAllObjectClassApplicationAttributesAfterInitialQuery &&
            !hasMultipleDirectedPublications &&
            !partiallyUnpublishDirectedPublicationAfterInitialQuery &&
            !unpublishAllDirectedPublicationsAfterPartialQuery)) &&
          (!unpublishWholeInteractionClassAfterInitialQuery ||
           (!unpublishOneObjectClassAttributeAfterInitialQuery &&
            !unpublishAllObjectClassApplicationAttributesAfterInitialQuery &&
            !unpublishWholeObjectClassAfterInitialQuery &&
            !hasMultipleDirectedPublications &&
            !partiallyUnpublishDirectedPublicationAfterInitialQuery &&
            !unpublishAllDirectedPublicationsAfterPartialQuery &&
            !publishCompleteObjectClassAttributeSet)) &&
          (!unpublishOneOrdinaryInteractionClassAfterInitialQuery ||
           (!unpublishOneObjectClassAttributeAfterInitialQuery &&
            !unpublishAllObjectClassApplicationAttributesAfterInitialQuery &&
            !unpublishWholeObjectClassAfterInitialQuery &&
            !unpublishWholeInteractionClassAfterInitialQuery &&
            !hasMultipleDirectedPublications &&
            !partiallyUnpublishDirectedPublicationAfterInitialQuery &&
            !unpublishAllDirectedPublicationsAfterPartialQuery &&
            !publishCompleteObjectClassAttributeSet &&
            !options.secondaryDirectedInteractionClassName.empty() &&
            options.secondaryDirectedInteractionClassName !=
                options.interactionClassName)) &&
           (!unpublishAllOrdinaryInteractionClassesAfterInitialQuery ||
            (!unpublishOneOrdinaryInteractionClassAfterInitialQuery &&
             !unpublishWholeInteractionClassAfterInitialQuery &&
             !unpublishOneObjectClassAttributeAfterInitialQuery &&
             !unpublishAllObjectClassApplicationAttributesAfterInitialQuery &&
             !unpublishWholeObjectClassAfterInitialQuery &&
             !hasMultipleDirectedPublications &&
             !partiallyUnpublishDirectedPublicationAfterInitialQuery &&
             !unpublishAllDirectedPublicationsAfterPartialQuery &&
             !publishCompleteObjectClassAttributeSet &&
             !options.secondaryDirectedInteractionClassName.empty() &&
             options.secondaryDirectedInteractionClassName !=
                 options.interactionClassName)) &&
          (!publishCompleteObjectClassAttributeSet ||
           (!hasMultipleDirectedPublications &&
            !partiallyUnpublishDirectedPublicationAfterInitialQuery &&
            !unpublishAllDirectedPublicationsAfterPartialQuery &&
            options.attributeName != options.secondaryAttributeName &&
            options.attributeName != options.objectClassThirdAttributeName &&
            options.secondaryAttributeName !=
                options.objectClassThirdAttributeName)) &&
          (!hasMultipleDirectedPublications ||
           (!options.secondaryDirectedObjectClassName.empty() &&
            !options.secondaryDirectedInteractionClassName.empty())),
      "Federate MOM publication-query reporting requires adapter-supplied FOM, standard MIM, application object class, attribute, interaction class, and logical-time configuration");

  Session subject(options, model, "owner");
  Session requester(options, model, "member");
  auto const scenarioSuffix =
      unpublishAllOrdinaryInteractionClassesAfterInitialQuery
      ? "federate-mom-multiple-interaction-class-full-unpublication-query"
      : unpublishOneOrdinaryInteractionClassAfterInitialQuery
      ? "federate-mom-interaction-class-partial-unpublication-query"
      : unpublishWholeInteractionClassAfterInitialQuery
      ? "federate-mom-interaction-class-whole-unpublication-query"
      :
      unpublishWholeObjectClassAfterInitialQuery
      ? "federate-mom-object-class-whole-unpublication-query"
      :
      unpublishAllObjectClassApplicationAttributesAfterInitialQuery
      ? "federate-mom-object-class-full-attribute-unpublication-query"
      :
      unpublishOneObjectClassAttributeAfterInitialQuery
      ? "federate-mom-object-class-attribute-unpublication-query"
      :
      publishCompleteObjectClassAttributeSet
      ? "federate-mom-complete-object-class-publication-query"
      : unpublishAllDirectedPublicationsAfterPartialQuery
      ? "federate-mom-multiple-directed-publication-full-unpublish-query"
      : partiallyUnpublishDirectedPublicationAfterInitialQuery
      ? "federate-mom-multiple-directed-publication-partial-unpublish-query"
      : hasMultipleDirectedPublications
          ? "federate-mom-multiple-directed-publication-query"
          : "federate-mom-publication-query";
  auto const federateNameSuffix =
      unpublishAllOrdinaryInteractionClassesAfterInitialQuery
      ? L"-mom-multiple-interaction-class-full-unpublication-query"
      : unpublishOneOrdinaryInteractionClassAfterInitialQuery
      ? L"-mom-interaction-class-partial-unpublication-query"
      : unpublishWholeInteractionClassAfterInitialQuery
      ? L"-mom-interaction-class-whole-unpublication-query"
      :
      unpublishWholeObjectClassAfterInitialQuery
      ? L"-mom-object-class-whole-unpublication-query"
      :
      unpublishAllObjectClassApplicationAttributesAfterInitialQuery
      ? L"-mom-object-class-full-attribute-unpublication-query"
      :
      unpublishOneObjectClassAttributeAfterInitialQuery
      ? L"-mom-object-class-attribute-unpublication-query"
      :
      publishCompleteObjectClassAttributeSet
      ? L"-mom-complete-object-class-publication-query"
      : unpublishAllDirectedPublicationsAfterPartialQuery
      ? L"-mom-multiple-directed-publication-full-unpublish-query"
      : partiallyUnpublishDirectedPublicationAfterInitialQuery
      ? L"-mom-multiple-directed-publication-partial-unpublish-query"
      : hasMultipleDirectedPublications
          ? L"-mom-multiple-directed-publication-query"
          : L"-mom-publication-query";
  auto const federation = federationName(options, scenarioSuffix);
  subject.connect();
  requester.connect();
  subject.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  auto const subjectName =
      options.ownerFederateName + federateNameSuffix + L"-subject";
  auto const requesterName =
      options.memberFederateName + federateNameSuffix + L"-requester";
  subject.join(subjectName, options.federateType, federation);
  requester.join(requesterName, options.federateType, federation);

  auto& subjectAmbassador = subject.rtiAmbassador();
  auto& requesterAmbassador = requester.rtiAmbassador();
  auto const subjectObjectClass = subjectAmbassador.getObjectClassHandle(
      hasMultipleDirectedPublications
          ? options.secondaryDirectedObjectClassName
          : options.objectClassName);
  auto const subjectAttribute = subjectAmbassador.getAttributeHandle(
      subjectObjectClass,
      options.attributeName);
  auto const subjectSecondaryAttribute =
      publishCompleteObjectClassAttributeSet
          ? subjectAmbassador.getAttributeHandle(
                subjectObjectClass,
                options.secondaryAttributeName)
          : rti::AttributeHandle{};
  auto const subjectThirdAttribute =
      publishCompleteObjectClassAttributeSet
          ? subjectAmbassador.getAttributeHandle(
                subjectObjectClass,
                options.objectClassThirdAttributeName)
          : rti::AttributeHandle{};
  auto const subjectInteractionClass =
      subjectAmbassador.getInteractionClassHandle(options.interactionClassName);
  auto const subjectSecondaryOrdinaryInteractionClass =
      (unpublishOneOrdinaryInteractionClassAfterInitialQuery ||
       unpublishAllOrdinaryInteractionClassesAfterInitialQuery)
          ? subjectAmbassador.getInteractionClassHandle(
                options.secondaryDirectedInteractionClassName)
          : rti::InteractionClassHandle{};
  auto const requesterObjectClass = requesterAmbassador.getObjectClassHandle(
      hasMultipleDirectedPublications
          ? options.secondaryDirectedObjectClassName
          : options.objectClassName);
  auto const subjectSecondaryDirectedObjectClass =
      hasMultipleDirectedPublications
          ? subjectAmbassador.getObjectClassHandle(options.objectClassName)
          : rti::ObjectClassHandle{};
  auto const requesterSecondaryDirectedObjectClass =
      hasMultipleDirectedPublications
          ? requesterAmbassador.getObjectClassHandle(options.objectClassName)
          : rti::ObjectClassHandle{};
  auto const requesterAttribute = requesterAmbassador.getAttributeHandle(
      requesterObjectClass,
      options.attributeName);
  auto const requesterSecondaryAttribute =
      publishCompleteObjectClassAttributeSet
          ? requesterAmbassador.getAttributeHandle(
                requesterObjectClass,
                options.secondaryAttributeName)
          : rti::AttributeHandle{};
  auto const requesterThirdAttribute =
      publishCompleteObjectClassAttributeSet
          ? requesterAmbassador.getAttributeHandle(
                requesterObjectClass,
                options.objectClassThirdAttributeName)
          : rti::AttributeHandle{};
  auto const requesterInteractionClass =
      requesterAmbassador.getInteractionClassHandle(options.interactionClassName);
  auto const requesterSecondaryOrdinaryInteractionClass =
      (unpublishOneOrdinaryInteractionClassAfterInitialQuery ||
       unpublishAllOrdinaryInteractionClassesAfterInitialQuery)
          ? requesterAmbassador.getInteractionClassHandle(
                options.secondaryDirectedInteractionClassName)
          : rti::InteractionClassHandle{};
  auto const subjectSecondaryDirectedInteractionClass =
      hasMultipleDirectedPublications
          ? subjectAmbassador.getInteractionClassHandle(
                options.secondaryDirectedInteractionClassName)
          : rti::InteractionClassHandle{};
  auto const requesterSecondaryDirectedInteractionClass =
      hasMultipleDirectedPublications
          ? requesterAmbassador.getInteractionClassHandle(
                options.secondaryDirectedInteractionClassName)
          : rti::InteractionClassHandle{};
  auto const targetFederate = requesterAmbassador.getFederateHandle(subjectName);
  auto const reliable =
      requesterAmbassador.getTransportationTypeHandle(L"HLAreliable");
  require(
      subjectObjectClass.isValid() && subjectAttribute.isValid() &&
          subjectInteractionClass.isValid() && requesterObjectClass.isValid() &&
          requesterAttribute.isValid() &&
          requesterInteractionClass.isValid() && targetFederate.isValid() &&
          reliable.isValid(),
      "Adapter-supplied publication handles or standard reliable transportation were invalid");
  require(
      !(unpublishOneOrdinaryInteractionClassAfterInitialQuery ||
        unpublishAllOrdinaryInteractionClassesAfterInitialQuery) ||
          (subjectSecondaryOrdinaryInteractionClass.isValid() &&
           requesterSecondaryOrdinaryInteractionClass.isValid() &&
           subjectSecondaryOrdinaryInteractionClass != subjectInteractionClass &&
           requesterSecondaryOrdinaryInteractionClass !=
               requesterInteractionClass),
      "The adapter-supplied second ordinary interaction class was invalid or duplicated the primary class");
  require(
      !hasMultipleDirectedPublications ||
          (subjectSecondaryDirectedObjectClass.isValid() &&
           requesterSecondaryDirectedObjectClass.isValid() &&
           subjectSecondaryDirectedObjectClass != subjectObjectClass &&
           requesterSecondaryDirectedObjectClass != requesterObjectClass &&
           subjectSecondaryDirectedInteractionClass.isValid() &&
           requesterSecondaryDirectedInteractionClass.isValid() &&
           subjectSecondaryDirectedInteractionClass != subjectInteractionClass &&
           requesterSecondaryDirectedInteractionClass !=
               requesterInteractionClass),
      "The adapter-supplied second directed-publication object or interaction class was invalid or duplicated the first class");
  require(
      !publishCompleteObjectClassAttributeSet ||
          (subjectSecondaryAttribute.isValid() &&
           subjectThirdAttribute.isValid() &&
           requesterSecondaryAttribute.isValid() &&
           requesterThirdAttribute.isValid() &&
           subjectSecondaryAttribute != subjectAttribute &&
           subjectThirdAttribute != subjectAttribute &&
           subjectThirdAttribute != subjectSecondaryAttribute &&
           requesterSecondaryAttribute != requesterAttribute &&
           requesterThirdAttribute != requesterAttribute &&
           requesterThirdAttribute != requesterSecondaryAttribute &&
           subjectAttribute == requesterAttribute &&
           subjectSecondaryAttribute == requesterSecondaryAttribute &&
           subjectThirdAttribute == requesterThirdAttribute),
      "The adapter-configured complete object-class publication attributes were invalid, duplicated, or inconsistent across federates");

  rti::AttributeHandleSet subjectPublishedAttributes{subjectAttribute};
  if (publishCompleteObjectClassAttributeSet) {
    subjectPublishedAttributes.insert(subjectSecondaryAttribute);
    subjectPublishedAttributes.insert(subjectThirdAttribute);
  }
  subjectAmbassador.publishObjectClassAttributes(
      subjectObjectClass,
      subjectPublishedAttributes);
  subjectAmbassador.publishInteractionClass(subjectInteractionClass);
  if (unpublishOneOrdinaryInteractionClassAfterInitialQuery ||
      unpublishAllOrdinaryInteractionClassesAfterInitialQuery) {
    subjectAmbassador.publishInteractionClass(
        subjectSecondaryOrdinaryInteractionClass);
  }
  if (hasMultipleDirectedPublications) {
    subjectAmbassador.publishObjectClassDirectedInteractions(
        subjectObjectClass,
        rti::InteractionClassHandleSet{
            subjectInteractionClass,
            subjectSecondaryDirectedInteractionClass});
    subjectAmbassador.publishObjectClassDirectedInteractions(
        subjectSecondaryDirectedObjectClass,
        rti::InteractionClassHandleSet{
            subjectInteractionClass});
  }

  auto const requestClass = requesterAmbassador.getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestPublications");
  auto const requestFederateParameter =
      requesterAmbassador.getParameterHandle(requestClass, L"HLAfederate");
  auto const objectReportClass = requesterAmbassador.getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportObjectClassPublication");
  auto const objectReportCountParameter =
      requesterAmbassador.getParameterHandle(objectReportClass, L"HLAnumberOfClasses");
  auto const objectReportClassParameter =
      requesterAmbassador.getParameterHandle(objectReportClass, L"HLAobjectClass");
  auto const objectReportAttributesParameter =
      requesterAmbassador.getParameterHandle(objectReportClass, L"HLAattributeList");
  auto const interactionReportClass = requesterAmbassador.getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportInteractionPublication");
  auto const interactionReportListParameter = requesterAmbassador.getParameterHandle(
      interactionReportClass,
      L"HLAinteractionClassList");
  auto const directedReportClass = requesterAmbassador.getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportDirectedInteractionPublication");
  auto const directedReportCountParameter =
      requesterAmbassador.getParameterHandle(directedReportClass, L"HLAnumberOfClasses");
  auto const directedReportObjectClassParameter =
      requesterAmbassador.getParameterHandle(directedReportClass, L"HLAobjectClass");
  auto const directedReportListParameter = requesterAmbassador.getParameterHandle(
      directedReportClass,
      L"HLAinteractionClassList");
  require(
      requestClass.isValid() && requestFederateParameter.isValid() &&
          objectReportClass.isValid() && objectReportCountParameter.isValid() &&
          objectReportClassParameter.isValid() &&
          objectReportAttributesParameter.isValid() &&
          interactionReportClass.isValid() &&
          interactionReportListParameter.isValid() &&
          directedReportClass.isValid() && directedReportCountParameter.isValid() &&
          directedReportObjectClassParameter.isValid() &&
          directedReportListParameter.isValid(),
      "Standard MIM publication request/report handle lookup returned an invalid handle");
  requesterAmbassador.subscribeInteractionClass(objectReportClass);
  requesterAmbassador.subscribeInteractionClass(interactionReportClass);
  requesterAmbassador.subscribeInteractionClass(directedReportClass);

  auto decodeAttributeHandleList = [&](rti::VariableLengthData const& encoded) {
    rti::HLAvariableArray const handlePrototype{rti::HLAoctet{}};
    rti::HLAvariableArray handles{
        static_cast<rti::DataElement const&>(handlePrototype)};
    handles.decode(encoded);
    std::vector<rti::AttributeHandle> decoded;
    decoded.reserve(handles.size());
    for (std::size_t index = 0; index != handles.size(); ++index) {
      auto const& encodedHandle =
          dynamic_cast<rti::HLAvariableArray const&>(handles.get(index));
      decoded.push_back(
          requesterAmbassador.decodeAttributeHandle(encodedHandle.encode()));
    }
    return decoded;
  };
  auto decodeInteractionClassHandleList =
      [&](rti::VariableLengthData const& encoded) {
        rti::HLAvariableArray const handlePrototype{rti::HLAoctet{}};
        rti::HLAvariableArray handles{
            static_cast<rti::DataElement const&>(handlePrototype)};
        handles.decode(encoded);
        std::vector<rti::InteractionClassHandle> decoded;
        decoded.reserve(handles.size());
        for (std::size_t index = 0; index != handles.size(); ++index) {
          auto const& encodedHandle =
              dynamic_cast<rti::HLAvariableArray const&>(handles.get(index));
          decoded.push_back(requesterAmbassador.decodeInteractionClassHandle(
              encodedHandle.encode()));
        }
        return decoded;
      };

  rti::ParameterHandleValueMap const requestParameters{
      {requestFederateParameter, targetFederate.encode()}};
  requesterAmbassador.sendInteraction(
      requestClass,
      requestParameters,
      rti::VariableLengthData{});
  waitFor(
      requester,
      [&] {
        return requester.recorder().interactions().size() >=
            (hasMultipleDirectedPublications ? 4U : 3U);
      },
      options,
      "the populated federate MOM publication reports");
  auto const reports = requester.recorder().interactions();
  require(
      reports.size() == (hasMultipleDirectedPublications ? 4U : 3U),
      "A publication request did not deliver exactly the standard MIM reports for its directed-publication object classes");

  InteractionRecord const* objectReport = nullptr;
  InteractionRecord const* interactionReport = nullptr;
  std::vector<InteractionRecord const*> directedReports;
  for (auto const& report : reports) {
    require(
        report.present && report.transportation == reliable,
        "A standard MIM publication report was absent or did not use reliable transportation");
    if (report.interaction == objectReportClass) {
      require(objectReport == nullptr, "Duplicate object-class publication report");
      objectReport = &report;
    } else if (report.interaction == interactionReportClass) {
      require(interactionReport == nullptr, "Duplicate interaction publication report");
      interactionReport = &report;
    } else if (report.interaction == directedReportClass) {
      directedReports.push_back(&report);
    } else {
      require(false, "Publication query delivered an unrequested interaction class");
    }
  }
  require(
      objectReport != nullptr && interactionReport != nullptr &&
          directedReports.size() ==
              (hasMultipleDirectedPublications ? 2U : 1U),
      "Publication query omitted or duplicated a standard MIM report for the directed-publication object classes");

  require(
      objectReport->parameters.size() == 3U &&
          objectReport->parameters.count(objectReportCountParameter) == 1U &&
          objectReport->parameters.count(objectReportClassParameter) == 1U &&
          objectReport->parameters.count(objectReportAttributesParameter) == 1U,
      "Object-class publication report did not contain its three MIM-defined parameters");
  rti::HLAinteger32BE objectClassCount;
  objectClassCount.decode(
      objectReport->parameters.at(objectReportCountParameter));
  require(
      objectClassCount.get() == 1 &&
          requesterAmbassador.decodeObjectClassHandle(
              objectReport->parameters.at(objectReportClassParameter)) ==
              requesterObjectClass,
      "Object-class publication report identified a different publication set");
  auto const publishedAttributes = decodeAttributeHandleList(
      objectReport->parameters.at(objectReportAttributesParameter));
  if (publishCompleteObjectClassAttributeSet) {
    auto const requesterPrivilegeToDeleteAttribute =
        requesterAmbassador.getAttributeHandle(
            requesterObjectClass,
            L"HLAprivilegeToDeleteObject");
    require(
        requesterPrivilegeToDeleteAttribute.isValid() &&
            requesterPrivilegeToDeleteAttribute != requesterAttribute &&
            requesterPrivilegeToDeleteAttribute !=
                requesterSecondaryAttribute &&
            requesterPrivilegeToDeleteAttribute != requesterThirdAttribute &&
            publishedAttributes.size() == 4U &&
            std::find(
                publishedAttributes.begin(),
                publishedAttributes.end(),
                requesterPrivilegeToDeleteAttribute) !=
                publishedAttributes.end() &&
            std::find(
                publishedAttributes.begin(),
                publishedAttributes.end(),
                requesterAttribute) != publishedAttributes.end() &&
            std::find(
                publishedAttributes.begin(),
                publishedAttributes.end(),
                requesterSecondaryAttribute) != publishedAttributes.end() &&
            std::find(
                publishedAttributes.begin(),
                publishedAttributes.end(),
                requesterThirdAttribute) != publishedAttributes.end(),
        "Object-class publication report did not contain the exact adapter-configured application attributes and inherited HLAprivilegeToDeleteObject (reported=" +
            std::to_string(publishedAttributes.size()) +
            ", primary=" +
            std::to_string(static_cast<unsigned>(std::find(
                publishedAttributes.begin(),
                publishedAttributes.end(),
                requesterAttribute) != publishedAttributes.end())) +
            ", secondary=" +
            std::to_string(static_cast<unsigned>(std::find(
                publishedAttributes.begin(),
                publishedAttributes.end(),
                requesterSecondaryAttribute) != publishedAttributes.end())) +
            ", third=" +
            std::to_string(static_cast<unsigned>(std::find(
                publishedAttributes.begin(),
                publishedAttributes.end(),
                requesterThirdAttribute) != publishedAttributes.end())) + ")");
  } else {
    require(
        std::find(
            publishedAttributes.begin(),
            publishedAttributes.end(),
            requesterAttribute) != publishedAttributes.end(),
        "Object-class publication report omitted the adapter-selected published attribute");
  }

  require(
      interactionReport->parameters.size() == 1U &&
          interactionReport->parameters.count(interactionReportListParameter) == 1U,
      "Interaction publication report did not contain its MIM-defined class list");
  auto const publishedInteractions = decodeInteractionClassHandleList(
      interactionReport->parameters.at(interactionReportListParameter));
  if (unpublishOneOrdinaryInteractionClassAfterInitialQuery ||
      unpublishAllOrdinaryInteractionClassesAfterInitialQuery) {
    require(
        publishedInteractions.size() == 2U &&
            std::find(
                publishedInteractions.begin(),
                publishedInteractions.end(),
                requesterInteractionClass) != publishedInteractions.end() &&
            std::find(
                publishedInteractions.begin(),
                publishedInteractions.end(),
                requesterSecondaryOrdinaryInteractionClass) !=
                publishedInteractions.end(),
        "Initial interaction publication report did not contain exactly the two adapter-selected ordinary interaction classes");
  } else {
    require(
        publishedInteractions.size() == 1U &&
            publishedInteractions.front() == requesterInteractionClass,
        "Interaction publication report did not contain exactly the adapter-selected published interaction class");
  }

  if (hasMultipleDirectedPublications) {
    bool reportedPrimaryObjectClass = false;
    bool reportedSecondaryObjectClass = false;
    for (auto const* directedReport : directedReports) {
      rti::HLAinteger32BE directedClassCount;
      require(
          directedReport->parameters.size() == 3U &&
              directedReport->parameters.count(
                  directedReportCountParameter) == 1U &&
              directedReport->parameters.count(
                  directedReportObjectClassParameter) == 1U &&
              directedReport->parameters.count(directedReportListParameter) ==
                  1U,
          "Populated directed-interaction publication report did not contain its three MIM-defined parameters");
      directedClassCount.decode(
          directedReport->parameters.at(directedReportCountParameter));
      auto const reportObjectClass =
          requesterAmbassador.decodeObjectClassHandle(
              directedReport->parameters.at(
                  directedReportObjectClassParameter));
      auto const directedInteractions = decodeInteractionClassHandleList(
          directedReport->parameters.at(directedReportListParameter));
      require(
          directedClassCount.get() == 2 &&
              directedInteractions.size() ==
                  (reportObjectClass == requesterObjectClass ? 2U : 1U),
          "Multiple directed-publication report did not report both published object classes and each class's published interactions");
      if (reportObjectClass == requesterObjectClass) {
        require(
            !reportedPrimaryObjectClass &&
                std::find(
                    directedInteractions.begin(),
                    directedInteractions.end(),
                    requesterInteractionClass) != directedInteractions.end() &&
                std::find(
                    directedInteractions.begin(),
                    directedInteractions.end(),
                    requesterSecondaryDirectedInteractionClass) !=
                    directedInteractions.end(),
            "Primary directed-publication report duplicated its object class or omitted one of its inherited and class-specific interactions");
        reportedPrimaryObjectClass = true;
      } else if (reportObjectClass == requesterSecondaryDirectedObjectClass) {
        require(
            !reportedSecondaryObjectClass &&
                directedInteractions.size() == 1U &&
                directedInteractions.front() == requesterInteractionClass,
            "Secondary directed-publication report duplicated its object class or identified the wrong interaction");
        reportedSecondaryObjectClass = true;
      } else {
        require(
            false,
            "Directed-publication report identified an unselected object class");
      }
    }
    require(
        reportedPrimaryObjectClass && reportedSecondaryObjectClass,
        "Multiple directed-publication reports did not cover both adapter-selected object classes");
  } else {
    auto const* directedReport = directedReports.front();
    require(
        directedReport->parameters.size() == 2U &&
            directedReport->parameters.count(directedReportCountParameter) == 1U &&
            directedReport->parameters.count(directedReportObjectClassParameter) == 0U &&
            directedReport->parameters.count(directedReportListParameter) == 1U,
        "Empty directed-interaction publication report did not use the standard NULL response parameter shape");
    rti::HLAinteger32BE directedClassCount;
    directedClassCount.decode(
        directedReport->parameters.at(directedReportCountParameter));
    require(
        directedClassCount.get() == 0 &&
            decodeInteractionClassHandleList(
                directedReport->parameters.at(directedReportListParameter)).empty(),
        "Empty directed-interaction publication report did not report zero classes and an empty list");
  }

  if (partiallyUnpublishDirectedPublicationAfterInitialQuery) {
    auto const reportOffset = requester.recorder().interactions().size();
    subjectAmbassador.unpublishObjectClassDirectedInteractions(
        subjectObjectClass,
        rti::InteractionClassHandleSet{subjectInteractionClass});
    requesterAmbassador.sendInteraction(
        requestClass,
        requestParameters,
        rti::VariableLengthData{});
    waitFor(
        requester,
        [&] {
          return requester.recorder().interactions().size() >= reportOffset + 4U;
        },
        options,
        "the federate MOM publication reports after partial unpublication");
    auto const reportsAfterUnpublication = requester.recorder().interactions();
    require(
        reportsAfterUnpublication.size() == reportOffset + 4U,
        "A publication request after partial unpublication did not deliver exactly the ordinary object and interaction reports plus both remaining directed reports");

    InteractionRecord const* remainingObjectReport = nullptr;
    InteractionRecord const* remainingInteractionReport = nullptr;
    std::vector<InteractionRecord const*> remainingDirectedReports;
    for (std::size_t index = reportOffset;
         index != reportsAfterUnpublication.size();
         ++index) {
      auto const& report = reportsAfterUnpublication[index];
      require(
          report.present && report.transportation == reliable,
          "A post-unpublication standard MIM publication report was absent or did not use reliable transportation");
      if (report.interaction == objectReportClass) {
        require(
            remainingObjectReport == nullptr,
            "Post-unpublication query delivered duplicate object-class publication reports");
        remainingObjectReport = &report;
      } else if (report.interaction == interactionReportClass) {
        require(
            remainingInteractionReport == nullptr,
            "Post-unpublication query delivered duplicate interaction publication reports");
        remainingInteractionReport = &report;
      } else if (report.interaction == directedReportClass) {
        remainingDirectedReports.push_back(&report);
      } else {
        require(
            false,
            "Post-unpublication query delivered an unrequested publication report");
      }
    }
    require(
        remainingObjectReport != nullptr &&
            remainingInteractionReport != nullptr &&
            remainingDirectedReports.size() == 2U,
        "Post-unpublication query omitted the object, interaction, or one of the two remaining directed publication reports");

    require(
        remainingObjectReport->parameters.size() == 3U &&
            remainingObjectReport->parameters.count(objectReportCountParameter) ==
                1U &&
            remainingObjectReport->parameters.count(objectReportClassParameter) ==
                1U &&
            remainingObjectReport->parameters.count(
                objectReportAttributesParameter) == 1U,
        "Post-unpublication object-class publication report did not contain its three MIM-defined parameters");
    rti::HLAinteger32BE remainingObjectClassCount;
    remainingObjectClassCount.decode(
        remainingObjectReport->parameters.at(objectReportCountParameter));
    auto const remainingPublishedAttributes = decodeAttributeHandleList(
        remainingObjectReport->parameters.at(objectReportAttributesParameter));
    require(
        remainingObjectClassCount.get() == 1 &&
            requesterAmbassador.decodeObjectClassHandle(
                remainingObjectReport->parameters.at(objectReportClassParameter)) ==
                requesterObjectClass &&
            remainingPublishedAttributes.size() == publishedAttributes.size() &&
            std::is_permutation(
                publishedAttributes.begin(),
                publishedAttributes.end(),
                remainingPublishedAttributes.begin()),
        "Post-unpublication object-class report changed the ordinary attribute publication");

    require(
        remainingInteractionReport->parameters.size() == 1U &&
            remainingInteractionReport->parameters.count(
                interactionReportListParameter) == 1U,
        "Post-unpublication interaction publication report did not contain its MIM-defined class list");
    auto const remainingPublishedInteractions = decodeInteractionClassHandleList(
        remainingInteractionReport->parameters.at(interactionReportListParameter));
    require(
        remainingPublishedInteractions.size() == 1U &&
            remainingPublishedInteractions.front() == requesterInteractionClass,
        "Post-unpublication query changed the ordinary interaction publication");

    bool remainingPrimaryObjectClass = false;
    bool remainingSecondaryObjectClass = false;
    for (auto const* directedReport : remainingDirectedReports) {
      require(
          directedReport->parameters.size() == 3U &&
              directedReport->parameters.count(directedReportCountParameter) ==
                  1U &&
              directedReport->parameters.count(
                  directedReportObjectClassParameter) == 1U &&
              directedReport->parameters.count(directedReportListParameter) ==
                  1U,
          "Remaining directed-interaction publication report did not contain its three MIM-defined parameters");
      rti::HLAinteger32BE remainingDirectedClassCount;
      remainingDirectedClassCount.decode(
          directedReport->parameters.at(directedReportCountParameter));
      auto const reportObjectClass = requesterAmbassador.decodeObjectClassHandle(
          directedReport->parameters.at(directedReportObjectClassParameter));
      auto const remainingDirectedInteractions = decodeInteractionClassHandleList(
          directedReport->parameters.at(directedReportListParameter));
      require(
          remainingDirectedClassCount.get() == 2 &&
              remainingDirectedInteractions.size() == 1U,
          "Post-unpublication directed report did not report both published object classes and one retained interaction");
      if (reportObjectClass == requesterObjectClass) {
        require(
            !remainingPrimaryObjectClass &&
                remainingDirectedInteractions.front() ==
                    requesterSecondaryDirectedInteractionClass,
            "Post-unpublication query did not retain exactly the primary class-specific directed interaction");
        remainingPrimaryObjectClass = true;
      } else if (reportObjectClass == requesterSecondaryDirectedObjectClass) {
        require(
            !remainingSecondaryObjectClass &&
                remainingDirectedInteractions.front() == requesterInteractionClass,
            "Post-unpublication query did not retain the secondary object's inherited directed interaction");
        remainingSecondaryObjectClass = true;
      } else {
        require(
            false,
            "Post-unpublication directed report identified an unselected object class");
      }
    }
    require(
        remainingPrimaryObjectClass && remainingSecondaryObjectClass,
        "Set-based unpublication removed the primary object class's remaining interaction or the independent secondary object-class publication");
  }

  if (unpublishAllDirectedPublicationsAfterPartialQuery) {
    auto const reportOffset = requester.recorder().interactions().size();
    subjectAmbassador.unpublishObjectClassDirectedInteractions(
        subjectObjectClass);
    subjectAmbassador.unpublishObjectClassDirectedInteractions(
        subjectSecondaryDirectedObjectClass);
    requesterAmbassador.sendInteraction(
        requestClass,
        requestParameters,
        rti::VariableLengthData{});
    waitFor(
        requester,
        [&] {
          return requester.recorder().interactions().size() >= reportOffset + 3U;
        },
        options,
        "the standard directed-publication NULL response after complete unpublishing");
    auto const reportsAfterCompleteUnpublication =
        requester.recorder().interactions();
    require(
        reportsAfterCompleteUnpublication.size() == reportOffset + 3U,
        "A publication request after complete directed unpublishing did not deliver exactly the ordinary reports and one directed-publication NULL response");

    InteractionRecord const* remainingObjectReport = nullptr;
    InteractionRecord const* remainingInteractionReport = nullptr;
    InteractionRecord const* directedNullReport = nullptr;
    for (std::size_t index = reportOffset;
         index != reportsAfterCompleteUnpublication.size();
         ++index) {
      auto const& report = reportsAfterCompleteUnpublication[index];
      require(
          report.present && report.transportation == reliable,
          "A complete-unpublication MIM report was absent or did not use reliable transportation");
      if (report.interaction == objectReportClass) {
        require(
            remainingObjectReport == nullptr,
            "Complete-unpublication query delivered duplicate object-class reports");
        remainingObjectReport = &report;
      } else if (report.interaction == interactionReportClass) {
        require(
            remainingInteractionReport == nullptr,
            "Complete-unpublication query delivered duplicate interaction reports");
        remainingInteractionReport = &report;
      } else if (report.interaction == directedReportClass) {
        require(
            directedNullReport == nullptr,
            "Complete-unpublication query delivered more than one directed-publication NULL response");
        directedNullReport = &report;
      } else {
        require(
            false,
            "Complete-unpublication query delivered an unrequested publication report");
      }
    }
    require(
        remainingObjectReport != nullptr &&
            remainingInteractionReport != nullptr && directedNullReport != nullptr,
        "Complete-unpublication query omitted an ordinary report or the directed-publication NULL response");

    require(
        remainingObjectReport->parameters.size() == 3U &&
            remainingObjectReport->parameters.count(objectReportCountParameter) ==
                1U &&
            remainingObjectReport->parameters.count(objectReportClassParameter) ==
                1U &&
            remainingObjectReport->parameters.count(
                objectReportAttributesParameter) == 1U,
        "Complete-unpublication object-class report did not contain its three MIM-defined parameters");
    rti::HLAinteger32BE remainingObjectClassCount;
    remainingObjectClassCount.decode(
        remainingObjectReport->parameters.at(objectReportCountParameter));
    auto const remainingPublishedAttributes = decodeAttributeHandleList(
        remainingObjectReport->parameters.at(objectReportAttributesParameter));
    require(
        remainingObjectClassCount.get() == 1 &&
            requesterAmbassador.decodeObjectClassHandle(
                remainingObjectReport->parameters.at(objectReportClassParameter)) ==
                requesterObjectClass &&
            remainingPublishedAttributes.size() == publishedAttributes.size() &&
            std::is_permutation(
                publishedAttributes.begin(),
                publishedAttributes.end(),
                remainingPublishedAttributes.begin()),
        "Complete directed unpublishing changed the ordinary object-attribute publication");

    require(
        remainingInteractionReport->parameters.size() == 1U &&
            remainingInteractionReport->parameters.count(
                interactionReportListParameter) == 1U,
        "Complete-unpublication interaction report did not contain its MIM-defined class list");
    auto const remainingPublishedInteractions = decodeInteractionClassHandleList(
        remainingInteractionReport->parameters.at(interactionReportListParameter));
    require(
        remainingPublishedInteractions.size() == 1U &&
            remainingPublishedInteractions.front() == requesterInteractionClass,
        "Complete directed unpublishing changed the ordinary interaction publication");

    require(
        directedNullReport->parameters.size() == 2U &&
            directedNullReport->parameters.count(directedReportCountParameter) ==
                1U &&
            directedNullReport->parameters.count(
                directedReportObjectClassParameter) == 0U &&
            directedNullReport->parameters.count(directedReportListParameter) ==
                1U,
        "Complete-unpublication directed report did not use the exact IEEE 1516.2-2025 NULL response parameter shape");
    rti::HLAinteger32BE directedClassCount;
    directedClassCount.decode(
        directedNullReport->parameters.at(directedReportCountParameter));
    auto const remainingDirectedInteractions = decodeInteractionClassHandleList(
        directedNullReport->parameters.at(directedReportListParameter));
    require(
        directedClassCount.get() == 0 && remainingDirectedInteractions.empty(),
        "Complete-unpublication directed report did not encode zero classes and an empty interaction list");
  }

  if (unpublishOneObjectClassAttributeAfterInitialQuery) {
    auto const reportOffset = requester.recorder().interactions().size();
    subjectAmbassador.unpublishObjectClassAttributes(
        subjectObjectClass,
        rti::AttributeHandleSet{subjectSecondaryAttribute});
    subjectPublishedAttributes.erase(subjectSecondaryAttribute);
    requesterAmbassador.sendInteraction(
        requestClass,
        requestParameters,
        rti::VariableLengthData{});
    waitFor(
        requester,
        [&] {
          return requester.recorder().interactions().size() >=
              reportOffset + 3U;
        },
        options,
        "the federate MOM publication reports after ordinary attribute unpublication");

    auto const reportsAfterAttributeUnpublication =
        requester.recorder().interactions();
    require(
        reportsAfterAttributeUnpublication.size() == reportOffset + 3U,
        "A publication request after ordinary attribute unpublication did not deliver exactly the standard object-class, interaction-class, and directed-interaction reports");
    InteractionRecord const* objectReportAfterAttributeUnpublication = nullptr;
    unsigned interactionReportCountAfterAttributeUnpublication = 0U;
    unsigned directedReportCountAfterAttributeUnpublication = 0U;
    for (std::size_t index = reportOffset;
         index != reportsAfterAttributeUnpublication.size();
         ++index) {
      auto const& report = reportsAfterAttributeUnpublication[index];
      require(
          report.present && report.transportation == reliable,
          "A post-unpublication standard MIM publication report was absent or did not use reliable transportation");
      if (report.interaction == objectReportClass) {
        require(
            objectReportAfterAttributeUnpublication == nullptr,
            "Post-unpublication query delivered duplicate object-class publication reports");
        objectReportAfterAttributeUnpublication = &report;
      } else if (report.interaction == interactionReportClass) {
        ++interactionReportCountAfterAttributeUnpublication;
      } else if (report.interaction == directedReportClass) {
        ++directedReportCountAfterAttributeUnpublication;
      } else {
        require(
            false,
            "Post-unpublication query delivered an unrequested publication report");
      }
    }
    require(
        objectReportAfterAttributeUnpublication != nullptr &&
            interactionReportCountAfterAttributeUnpublication == 1U &&
            directedReportCountAfterAttributeUnpublication == 1U,
        "Post-unpublication query omitted or duplicated a standard publication report");
    require(
        objectReportAfterAttributeUnpublication->parameters.size() == 3U &&
            objectReportAfterAttributeUnpublication->parameters.count(
                objectReportCountParameter) == 1U &&
            objectReportAfterAttributeUnpublication->parameters.count(
                objectReportClassParameter) == 1U &&
            objectReportAfterAttributeUnpublication->parameters.count(
                objectReportAttributesParameter) == 1U,
        "Post-unpublication object-class report did not contain exactly its three MIM-defined parameters");
    rti::HLAinteger32BE objectClassCountAfterAttributeUnpublication;
    objectClassCountAfterAttributeUnpublication.decode(
        objectReportAfterAttributeUnpublication->parameters.at(
            objectReportCountParameter));
    auto const attributesAfterAttributeUnpublication =
        decodeAttributeHandleList(
            objectReportAfterAttributeUnpublication->parameters.at(
                objectReportAttributesParameter));
    auto const requesterPrivilegeToDeleteAttribute =
        requesterAmbassador.getAttributeHandle(
            requesterObjectClass,
            L"HLAprivilegeToDeleteObject");
    require(
        objectClassCountAfterAttributeUnpublication.get() == 1 &&
            requesterAmbassador.decodeObjectClassHandle(
                objectReportAfterAttributeUnpublication->parameters.at(
                    objectReportClassParameter)) == requesterObjectClass &&
            requesterPrivilegeToDeleteAttribute.isValid() &&
            attributesAfterAttributeUnpublication.size() == 3U &&
            std::find(
                attributesAfterAttributeUnpublication.begin(),
                attributesAfterAttributeUnpublication.end(),
                requesterAttribute) !=
                attributesAfterAttributeUnpublication.end() &&
            std::find(
                attributesAfterAttributeUnpublication.begin(),
                attributesAfterAttributeUnpublication.end(),
                requesterThirdAttribute) !=
                attributesAfterAttributeUnpublication.end() &&
            std::find(
                attributesAfterAttributeUnpublication.begin(),
                attributesAfterAttributeUnpublication.end(),
                requesterPrivilegeToDeleteAttribute) !=
                attributesAfterAttributeUnpublication.end() &&
            std::find(
                attributesAfterAttributeUnpublication.begin(),
                attributesAfterAttributeUnpublication.end(),
                requesterSecondaryAttribute) ==
                attributesAfterAttributeUnpublication.end(),
        "Post-unpublication MIM report did not contain exactly the remaining adapter-selected attributes and inherited HLAprivilegeToDeleteObject");
  }

  if (unpublishAllObjectClassApplicationAttributesAfterInitialQuery) {
    auto const reportOffset = requester.recorder().interactions().size();
    subjectAmbassador.unpublishObjectClassAttributes(
        subjectObjectClass,
        subjectPublishedAttributes);
    subjectPublishedAttributes.clear();
    requesterAmbassador.sendInteraction(
        requestClass,
        requestParameters,
        rti::VariableLengthData{});
    waitFor(
        requester,
        [&] {
          return requester.recorder().interactions().size() >=
              reportOffset + 3U;
        },
        options,
        "the federate MOM publication reports after all application attributes are unpublished");

    auto const reportsAfterFullAttributeUnpublication =
        requester.recorder().interactions();
    require(
        reportsAfterFullAttributeUnpublication.size() == reportOffset + 3U,
        "A publication request after full application-attribute unpublication did not deliver exactly the three standard publication report interactions");
    InteractionRecord const* objectReportAfterFullAttributeUnpublication =
        nullptr;
    unsigned interactionReportCountAfterFullAttributeUnpublication = 0U;
    unsigned directedReportCountAfterFullAttributeUnpublication = 0U;
    for (std::size_t index = reportOffset;
         index != reportsAfterFullAttributeUnpublication.size();
         ++index) {
      auto const& report = reportsAfterFullAttributeUnpublication[index];
      require(
          report.present && report.transportation == reliable,
          "A post-full-unpublication standard MIM report was absent or did not use reliable transportation");
      if (report.interaction == objectReportClass) {
        require(
            objectReportAfterFullAttributeUnpublication == nullptr,
            "Full-unpublication query delivered duplicate object-class publication reports");
        objectReportAfterFullAttributeUnpublication = &report;
      } else if (report.interaction == interactionReportClass) {
        ++interactionReportCountAfterFullAttributeUnpublication;
      } else if (report.interaction == directedReportClass) {
        ++directedReportCountAfterFullAttributeUnpublication;
      } else {
        require(
            false,
            "Full-unpublication query delivered an unrequested publication report");
      }
    }
    require(
        objectReportAfterFullAttributeUnpublication != nullptr &&
            interactionReportCountAfterFullAttributeUnpublication == 1U &&
            directedReportCountAfterFullAttributeUnpublication == 1U,
        "Full-unpublication query omitted or duplicated a standard publication report");
    require(
        objectReportAfterFullAttributeUnpublication->parameters.size() == 1U &&
            objectReportAfterFullAttributeUnpublication->parameters.count(
                objectReportCountParameter) == 1U &&
            objectReportAfterFullAttributeUnpublication->parameters.count(
                objectReportClassParameter) == 0U &&
            objectReportAfterFullAttributeUnpublication->parameters.count(
                objectReportAttributesParameter) == 0U,
        "Full-unpublication object-class report did not use the exact 2025 MIM NULL response parameter shape");
    rti::HLAinteger32BE objectClassCountAfterFullAttributeUnpublication;
    objectClassCountAfterFullAttributeUnpublication.decode(
        objectReportAfterFullAttributeUnpublication->parameters.at(
            objectReportCountParameter));
    require(
        objectClassCountAfterFullAttributeUnpublication.get() == 0,
        "Full-unpublication MIM NULL response did not report zero published object classes");
  }

  if (unpublishWholeObjectClassAfterInitialQuery) {
    auto const reportOffset = requester.recorder().interactions().size();
    subjectAmbassador.unpublishObjectClass(subjectObjectClass);
    subjectPublishedAttributes.clear();
    requesterAmbassador.sendInteraction(
        requestClass,
        requestParameters,
        rti::VariableLengthData{});
    waitFor(
        requester,
        [&] {
          return requester.recorder().interactions().size() >=
              reportOffset + 3U;
        },
        options,
        "the federate MOM publication reports after the object class is unpublished");

    auto const reportsAfterWholeClassUnpublication =
        requester.recorder().interactions();
    require(
        reportsAfterWholeClassUnpublication.size() == reportOffset + 3U,
        "A publication request after whole-class unpublication did not deliver exactly the three standard publication report interactions");
    InteractionRecord const* objectReportAfterWholeClassUnpublication =
        nullptr;
    unsigned interactionReportCountAfterWholeClassUnpublication = 0U;
    unsigned directedReportCountAfterWholeClassUnpublication = 0U;
    for (std::size_t index = reportOffset;
         index != reportsAfterWholeClassUnpublication.size();
         ++index) {
      auto const& report = reportsAfterWholeClassUnpublication[index];
      require(
          report.present && report.transportation == reliable,
          "A post-whole-class-unpublication standard MIM report was absent or did not use reliable transportation");
      if (report.interaction == objectReportClass) {
        require(
            objectReportAfterWholeClassUnpublication == nullptr,
            "Whole-class-unpublication query delivered duplicate object-class publication reports");
        objectReportAfterWholeClassUnpublication = &report;
      } else if (report.interaction == interactionReportClass) {
        ++interactionReportCountAfterWholeClassUnpublication;
      } else if (report.interaction == directedReportClass) {
        ++directedReportCountAfterWholeClassUnpublication;
      } else {
        require(
            false,
            "Whole-class-unpublication query delivered an unrequested publication report");
      }
    }
    require(
        objectReportAfterWholeClassUnpublication != nullptr &&
            interactionReportCountAfterWholeClassUnpublication == 1U &&
            directedReportCountAfterWholeClassUnpublication == 1U,
        "Whole-class-unpublication query omitted or duplicated a standard publication report");
    require(
        objectReportAfterWholeClassUnpublication->parameters.size() == 1U &&
            objectReportAfterWholeClassUnpublication->parameters.count(
                objectReportCountParameter) == 1U &&
            objectReportAfterWholeClassUnpublication->parameters.count(
                objectReportClassParameter) == 0U &&
            objectReportAfterWholeClassUnpublication->parameters.count(
                objectReportAttributesParameter) == 0U,
        "Whole-class-unpublication object report did not use the exact 2025 MIM NULL response parameter shape");
    rti::HLAinteger32BE objectClassCountAfterWholeClassUnpublication;
    objectClassCountAfterWholeClassUnpublication.decode(
        objectReportAfterWholeClassUnpublication->parameters.at(
            objectReportCountParameter));
    require(
        objectClassCountAfterWholeClassUnpublication.get() == 0,
        "Whole-class-unpublication MIM NULL response did not report zero published object classes");
  }

  if (unpublishWholeInteractionClassAfterInitialQuery ||
      unpublishAllOrdinaryInteractionClassesAfterInitialQuery) {
    auto const reportOffset = requester.recorder().interactions().size();
    if (unpublishAllOrdinaryInteractionClassesAfterInitialQuery) {
      subjectAmbassador.unpublishInteractionClass(
          subjectSecondaryOrdinaryInteractionClass);
    }
    subjectAmbassador.unpublishInteractionClass(subjectInteractionClass);
    requesterAmbassador.sendInteraction(
        requestClass,
        requestParameters,
        rti::VariableLengthData{});
    waitFor(
        requester,
        [&] {
          return requester.recorder().interactions().size() >=
              reportOffset + 3U;
        },
        options,
        "the federate MOM publication reports after all selected ordinary interaction classes are unpublished");

    auto const reportsAfterInteractionUnpublication =
        requester.recorder().interactions();
    require(
        reportsAfterInteractionUnpublication.size() == reportOffset + 3U,
        "A publication request after ordinary interaction-class unpublication did not deliver exactly the standard object, interaction, and directed publication reports");
    InteractionRecord const* objectReportAfterInteractionUnpublication =
        nullptr;
    InteractionRecord const* interactionReportAfterInteractionUnpublication =
        nullptr;
    InteractionRecord const* directedReportAfterInteractionUnpublication =
        nullptr;
    for (std::size_t index = reportOffset;
         index != reportsAfterInteractionUnpublication.size();
         ++index) {
      auto const& report = reportsAfterInteractionUnpublication[index];
      require(
          report.present && report.transportation == reliable,
          "A post-interaction-unpublication standard MIM publication report was absent or did not use reliable transportation");
      if (report.interaction == objectReportClass) {
        require(
            objectReportAfterInteractionUnpublication == nullptr,
            "Interaction-unpublication query delivered duplicate object-class publication reports");
        objectReportAfterInteractionUnpublication = &report;
      } else if (report.interaction == interactionReportClass) {
        require(
            interactionReportAfterInteractionUnpublication == nullptr,
            "Interaction-unpublication query delivered duplicate interaction-class publication reports");
        interactionReportAfterInteractionUnpublication = &report;
      } else if (report.interaction == directedReportClass) {
        require(
            directedReportAfterInteractionUnpublication == nullptr,
            "Interaction-unpublication query delivered duplicate directed-interaction publication reports");
        directedReportAfterInteractionUnpublication = &report;
      } else {
        require(
            false,
            "Interaction-unpublication query delivered an unrequested publication report");
      }
    }
    require(
        objectReportAfterInteractionUnpublication != nullptr &&
            interactionReportAfterInteractionUnpublication != nullptr &&
            directedReportAfterInteractionUnpublication != nullptr,
        "Interaction-unpublication query omitted a standard publication report");
    require(
        interactionReportAfterInteractionUnpublication->parameters.size() ==
                1U &&
            interactionReportAfterInteractionUnpublication->parameters.count(
                interactionReportListParameter) == 1U &&
            decodeInteractionClassHandleList(
                interactionReportAfterInteractionUnpublication->parameters.at(
                    interactionReportListParameter)).empty(),
        "Unpublishing the ordinary interaction class did not produce the exact 2025 MIM NULL report with an empty interaction-class list");
    require(
        objectReportAfterInteractionUnpublication->parameters.size() == 3U &&
            objectReportAfterInteractionUnpublication->parameters.count(
                objectReportCountParameter) == 1U &&
            objectReportAfterInteractionUnpublication->parameters.count(
                objectReportClassParameter) == 1U &&
            objectReportAfterInteractionUnpublication->parameters.count(
                objectReportAttributesParameter) == 1U,
        "Ordinary interaction unpublication changed the independent object-class publication report shape");
    rti::HLAinteger32BE objectClassCountAfterInteractionUnpublication;
    objectClassCountAfterInteractionUnpublication.decode(
        objectReportAfterInteractionUnpublication->parameters.at(
            objectReportCountParameter));
    require(
        objectClassCountAfterInteractionUnpublication.get() == 1 &&
            requesterAmbassador.decodeObjectClassHandle(
                objectReportAfterInteractionUnpublication->parameters.at(
                    objectReportClassParameter)) == requesterObjectClass,
        "Ordinary interaction unpublication changed the independent object-class publication");
    auto const& directedReportParameters =
        directedReportAfterInteractionUnpublication->parameters;
    if (directedReportParameters.size() == 2U) {
      require(
          directedReportParameters.count(directedReportCountParameter) == 1U &&
              directedReportParameters.count(
                  directedReportObjectClassParameter) == 0U &&
              directedReportParameters.count(directedReportListParameter) == 1U,
          "The NULL directed-interaction publication report did not use the exact 2025 MIM parameter shape");
      rti::HLAinteger32BE directedClassCountAfterInteractionUnpublication;
      directedClassCountAfterInteractionUnpublication.decode(
          directedReportParameters.at(directedReportCountParameter));
      require(
          directedClassCountAfterInteractionUnpublication.get() == 0 &&
              decodeInteractionClassHandleList(
                  directedReportParameters.at(directedReportListParameter))
                  .empty(),
          "The NULL directed-interaction publication report did not encode zero classes and an empty interaction list");
    } else {
      require(
          directedReportParameters.size() == 3U &&
              directedReportParameters.count(directedReportCountParameter) ==
                  1U &&
              directedReportParameters.count(
                  directedReportObjectClassParameter) == 1U &&
              directedReportParameters.count(directedReportListParameter) == 1U,
          "The populated directed-interaction publication report did not use the exact 2025 MIM parameter shape");
      rti::HLAinteger32BE directedClassCountAfterInteractionUnpublication;
      directedClassCountAfterInteractionUnpublication.decode(
          directedReportParameters.at(directedReportCountParameter));
      auto const directedInteractionsAfterInteractionUnpublication =
          decodeInteractionClassHandleList(
              directedReportParameters.at(directedReportListParameter));
      require(
          directedClassCountAfterInteractionUnpublication.get() == 1 &&
              requesterAmbassador.decodeObjectClassHandle(
                  directedReportParameters.at(
                      directedReportObjectClassParameter)) ==
                  requesterObjectClass &&
              directedInteractionsAfterInteractionUnpublication.size() == 1U &&
              directedInteractionsAfterInteractionUnpublication.front() ==
                  requesterInteractionClass,
          "The populated directed-interaction publication report did not describe the adapter-selected 2025 declaration");
    }
  }

  if (unpublishOneOrdinaryInteractionClassAfterInitialQuery) {
    auto const reportOffset = requester.recorder().interactions().size();
    subjectAmbassador.unpublishInteractionClass(
        subjectSecondaryOrdinaryInteractionClass);
    requesterAmbassador.sendInteraction(
        requestClass,
        requestParameters,
        rti::VariableLengthData{});
    waitFor(
        requester,
        [&] {
          return requester.recorder().interactions().size() >=
              reportOffset + 3U;
        },
        options,
        "the federate MOM publication reports after one ordinary interaction class is unpublished");

    auto const reportsAfterPartialInteractionUnpublication =
        requester.recorder().interactions();
    require(
        reportsAfterPartialInteractionUnpublication.size() == reportOffset + 3U,
        "A publication request after partial ordinary interaction unpublication did not deliver exactly the standard object, interaction, and directed reports");
    InteractionRecord const* objectReportAfterPartialInteractionUnpublication =
        nullptr;
    InteractionRecord const* interactionReportAfterPartialInteractionUnpublication =
        nullptr;
    InteractionRecord const* directedReportAfterPartialInteractionUnpublication =
        nullptr;
    for (std::size_t index = reportOffset;
         index != reportsAfterPartialInteractionUnpublication.size();
         ++index) {
      auto const& report =
          reportsAfterPartialInteractionUnpublication[index];
      require(
          report.present && report.transportation == reliable,
          "A post-partial-unpublication standard MIM report was absent or did not use reliable transportation");
      if (report.interaction == objectReportClass) {
        require(
            objectReportAfterPartialInteractionUnpublication == nullptr,
            "Partial interaction-unpublication query delivered duplicate object-class reports");
        objectReportAfterPartialInteractionUnpublication = &report;
      } else if (report.interaction == interactionReportClass) {
        require(
            interactionReportAfterPartialInteractionUnpublication == nullptr,
            "Partial interaction-unpublication query delivered duplicate ordinary interaction reports");
        interactionReportAfterPartialInteractionUnpublication = &report;
      } else if (report.interaction == directedReportClass) {
        require(
            directedReportAfterPartialInteractionUnpublication == nullptr,
            "Partial interaction-unpublication query delivered duplicate directed-interaction reports");
        directedReportAfterPartialInteractionUnpublication = &report;
      } else {
        require(
            false,
            "Partial interaction-unpublication query delivered an unrequested report");
      }
    }
    require(
        objectReportAfterPartialInteractionUnpublication != nullptr &&
            interactionReportAfterPartialInteractionUnpublication != nullptr &&
            directedReportAfterPartialInteractionUnpublication != nullptr,
        "Partial interaction-unpublication query omitted a standard MIM publication report");
    require(
        interactionReportAfterPartialInteractionUnpublication->parameters.size() ==
                1U &&
            interactionReportAfterPartialInteractionUnpublication->parameters.count(
                interactionReportListParameter) == 1U,
        "Post-partial-unpublication interaction report did not contain its MIM-defined class-list parameter");
    auto const remainingOrdinaryInteractions =
        decodeInteractionClassHandleList(
            interactionReportAfterPartialInteractionUnpublication->parameters.at(
                interactionReportListParameter));
    require(
        remainingOrdinaryInteractions.size() == 1U &&
            remainingOrdinaryInteractions.front() == requesterInteractionClass,
        "Post-unpublication MIM list did not contain exactly the remaining adapter-selected ordinary interaction class");

    rti::HLAinteger32BE objectClassCountAfterPartialInteractionUnpublication;
    require(
        objectReportAfterPartialInteractionUnpublication->parameters.size() ==
                3U &&
            objectReportAfterPartialInteractionUnpublication->parameters.count(
                objectReportCountParameter) == 1U &&
            objectReportAfterPartialInteractionUnpublication->parameters.count(
                objectReportClassParameter) == 1U &&
            objectReportAfterPartialInteractionUnpublication->parameters.count(
                objectReportAttributesParameter) == 1U,
        "Post-partial-unpublication object report did not contain its three MIM-defined parameters");
    objectClassCountAfterPartialInteractionUnpublication.decode(
        objectReportAfterPartialInteractionUnpublication->parameters.at(
            objectReportCountParameter));
    require(
        objectClassCountAfterPartialInteractionUnpublication.get() == 1 &&
            requesterAmbassador.decodeObjectClassHandle(
                objectReportAfterPartialInteractionUnpublication->parameters.at(
                    objectReportClassParameter)) == requesterObjectClass,
        "Partial ordinary interaction unpublication changed the independent object-class publication");

    auto const& directedReportParameters =
        directedReportAfterPartialInteractionUnpublication->parameters;
    if (directedReportParameters.size() == 2U) {
      require(
          directedReportParameters.count(directedReportCountParameter) == 1U &&
              directedReportParameters.count(
                  directedReportObjectClassParameter) == 0U &&
              directedReportParameters.count(directedReportListParameter) == 1U,
          "The NULL directed-interaction report did not use the exact 2025 MIM parameter shape");
      rti::HLAinteger32BE directedClassCount;
      directedClassCount.decode(
          directedReportParameters.at(directedReportCountParameter));
      require(
          directedClassCount.get() == 0 &&
              decodeInteractionClassHandleList(
                  directedReportParameters.at(directedReportListParameter))
                  .empty(),
          "The NULL directed-interaction report did not encode zero classes and an empty interaction list");
    } else {
      require(
          directedReportParameters.size() == 3U &&
              directedReportParameters.count(directedReportCountParameter) ==
                  1U &&
              directedReportParameters.count(
                  directedReportObjectClassParameter) == 1U &&
              directedReportParameters.count(directedReportListParameter) == 1U,
          "The populated directed-interaction report did not use the exact 2025 MIM parameter shape");
      rti::HLAinteger32BE directedClassCount;
      directedClassCount.decode(
          directedReportParameters.at(directedReportCountParameter));
      auto const remainingDirectedInteractions =
          decodeInteractionClassHandleList(
              directedReportParameters.at(directedReportListParameter));
      require(
          directedClassCount.get() == 1 &&
              requesterAmbassador.decodeObjectClassHandle(
                  directedReportParameters.at(
                      directedReportObjectClassParameter)) ==
                  requesterObjectClass &&
              remainingDirectedInteractions.size() == 1U &&
              remainingDirectedInteractions.front() ==
                  requesterInteractionClass,
          "The populated directed-interaction report did not identify the adapter-selected declaration");
    }
  }

  requesterAmbassador.unsubscribeInteractionClass(objectReportClass);
  requesterAmbassador.unsubscribeInteractionClass(interactionReportClass);
  requesterAmbassador.unsubscribeInteractionClass(directedReportClass);
  if (!subjectPublishedAttributes.empty()) {
    subjectAmbassador.unpublishObjectClassAttributes(
        subjectObjectClass,
        subjectPublishedAttributes);
  }
  if (!unpublishWholeInteractionClassAfterInitialQuery &&
      !unpublishAllOrdinaryInteractionClassesAfterInitialQuery) {
    subjectAmbassador.unpublishInteractionClass(subjectInteractionClass);
  }
  requester.resign(rti::NO_ACTION);
  subject.resign(rti::NO_ACTION);
  subjectAmbassador.destroyFederationExecution(federation);
  requester.disconnect();
  subject.disconnect();
}

void scenarioFederateMomPublicationQuery(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateMomPublicationQueryState(options, model, false);
}

void scenarioFederateMomCompleteObjectClassPublicationQuery(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateMomPublicationQueryState(options, model, false, false, false, true);
}

void scenarioFederateMomObjectClassAttributeUnpublicationQuery(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateMomPublicationQueryState(
      options,
      model,
      false,
      false,
      false,
      true,
      true);
}

void scenarioFederateMomObjectClassFullAttributeUnpublicationQuery(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateMomPublicationQueryState(
      options,
      model,
      false,
      false,
      false,
      true,
      false,
      true);
}

void scenarioFederateMomObjectClassWholeUnpublicationQuery(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateMomPublicationQueryState(
      options,
      model,
      false,
      false,
      false,
      false,
      false,
      false,
      true);
}

void scenarioFederateMomInteractionClassWholeUnpublicationQuery(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateMomPublicationQueryState(
      options,
      model,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      true);
}

void scenarioFederateMomInteractionClassPartialUnpublicationQuery(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateMomPublicationQueryState(
      options,
      model,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      true);
}

void scenarioFederateMomMultipleInteractionClassFullUnpublicationQuery(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateMomPublicationQueryState(
      options,
      model,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      true);
}

void scenarioFederateMomMultipleDirectedPublicationQuery(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateMomPublicationQueryState(options, model, true);
}

void scenarioFederateMomMultipleDirectedPublicationPartialUnpublishQuery(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateMomPublicationQueryState(options, model, true, true);
}

void scenarioFederateMomMultipleDirectedPublicationFullUnpublishQuery(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateMomPublicationQueryState(options, model, true, true, true);
}

void scenarioFederateMomPublicationQueryContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateMomPublicationQuery(options, model);
}

void scenarioFederateMomCompleteObjectClassPublicationQueryContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateMomCompleteObjectClassPublicationQuery(options, model);
}

void scenarioFederateMomObjectClassAttributeUnpublicationQueryContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateMomObjectClassAttributeUnpublicationQuery(options, model);
}

void scenarioFederateMomObjectClassFullAttributeUnpublicationQueryContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateMomObjectClassFullAttributeUnpublicationQuery(options, model);
}

void scenarioFederateMomObjectClassWholeUnpublicationQueryContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateMomObjectClassWholeUnpublicationQuery(options, model);
}

void scenarioFederateMomInteractionClassWholeUnpublicationQueryContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateMomInteractionClassWholeUnpublicationQuery(options, model);
}

void scenarioFederateMomInteractionClassPartialUnpublicationQueryContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateMomInteractionClassPartialUnpublicationQuery(options, model);
}

void scenarioFederateMomMultipleInteractionClassFullUnpublicationQueryContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateMomMultipleInteractionClassFullUnpublicationQuery(
      options,
      model);
}

void scenarioFederateMomMultipleDirectedPublicationQueryContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateMomMultipleDirectedPublicationQuery(options, model);
}

void scenarioFederateMomMultipleDirectedPublicationPartialUnpublishQueryContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateMomMultipleDirectedPublicationPartialUnpublishQuery(
      options,
      model);
}

void scenarioFederateMomMultipleDirectedPublicationFullUnpublishQueryContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateMomMultipleDirectedPublicationFullUnpublishQuery(
      options,
      model);
}

void scenarioFederateMomSubscriptionQueryState(
    Options const& options,
    rti::CallbackModel model,
    bool hasObjectSubscription,
    bool activeObjectSubscription,
    bool hasInteractionSubscription,
    bool activeInteractionSubscription,
    bool hasDirectedSubscription,
    bool hasMultipleDirectedSubscriptions,
    bool unsubscribeAllDirectedInteractionsAfterPartialQuery = false,
    bool activatePassiveInteractionAfterInitialQuery = false,
    bool hasSecondaryInteractionSubscription = false,
    bool activeSecondaryInteractionSubscription = false,
    bool unsubscribeSecondaryInteractionAfterInitialQuery = false,
    bool unsubscribePrimaryInteractionAfterSecondaryUnsubscribe = false,
    bool partiallyUnsubscribeObjectAttributeAfterInitialQuery = false,
    bool fullyUnsubscribeObjectAttributesAfterInitialQuery = false,
    bool unsubscribeWholeObjectClassAfterInitialQuery = false,
    bool subscribeAllObjectClassAttributes = false,
    bool activatePassiveCompleteObjectClassSubscriptionAfterInitialQuery = false) {
  auto const usesTwoObjectAttributes =
      partiallyUnsubscribeObjectAttributeAfterInitialQuery ||
      fullyUnsubscribeObjectAttributesAfterInitialQuery ||
      unsubscribeWholeObjectClassAfterInitialQuery;
  auto const usesThreeObjectAttributes = subscribeAllObjectClassAttributes;
  auto const usesMultipleObjectAttributes =
      usesTwoObjectAttributes || usesThreeObjectAttributes;
  auto const usesFullObjectUnsubscription =
      fullyUnsubscribeObjectAttributesAfterInitialQuery ||
      unsubscribeWholeObjectClassAfterInitialQuery;
  require(
      !options.fom.empty() && !options.mimFom.empty() &&
          !options.rateFom.empty() &&
          !options.logicalTimeImplementationName.empty() &&
          !options.objectClassName.empty() && !options.attributeName.empty() &&
          (!usesMultipleObjectAttributes ||
           !options.secondaryAttributeName.empty()) &&
          (!usesThreeObjectAttributes ||
           !options.objectClassThirdAttributeName.empty()) &&
          !options.interactionClassName.empty() &&
          (!hasMultipleDirectedSubscriptions ||
           (!options.secondaryDirectedObjectClassName.empty() &&
            !options.secondaryDirectedInteractionClassName.empty())) &&
          !options.fomUpdateRateName.empty(),
      "Federate MOM subscription-query reporting requires adapter-supplied application FOM, rate FOM, standard MIM, object class, attribute, interaction class, update-rate name, and logical-time configuration");
  require(
      !unsubscribeAllDirectedInteractionsAfterPartialQuery ||
          hasMultipleDirectedSubscriptions,
      "The fully unsubscribed directed-interaction NULL-response scenario requires multiple directed subscriptions");
  require(
      !hasMultipleDirectedSubscriptions || hasDirectedSubscription,
      "Multiple directed subscription reporting requires a populated directed subscription");
  require(
      !activatePassiveInteractionAfterInitialQuery ||
          (hasInteractionSubscription && !activeInteractionSubscription),
      "Passive-interaction activation reporting requires an initially passive interaction subscription");
  require(
      !hasSecondaryInteractionSubscription || hasInteractionSubscription,
      "Multiple interaction-subscription reporting requires the primary subscription");
  require(
      !hasSecondaryInteractionSubscription ||
          !activatePassiveInteractionAfterInitialQuery,
      "The multiple interaction-subscription scenario does not combine with the separate activation lifecycle");
  require(
      !unsubscribeSecondaryInteractionAfterInitialQuery ||
          hasSecondaryInteractionSubscription,
      "Secondary interaction unsubscription reporting requires multiple interaction subscriptions");
  require(
      !unsubscribePrimaryInteractionAfterSecondaryUnsubscribe ||
          unsubscribeSecondaryInteractionAfterInitialQuery,
      "Full interaction unsubscription reporting requires the secondary interaction to be removed first");
  require(
      !partiallyUnsubscribeObjectAttributeAfterInitialQuery ||
          options.attributeName != options.secondaryAttributeName,
      "Partial object-attribute unsubscription requires two distinct adapter-selected attributes");
  require(
      !fullyUnsubscribeObjectAttributesAfterInitialQuery ||
          options.attributeName != options.secondaryAttributeName,
      "Full object-attribute unsubscription requires two distinct adapter-selected attributes");
  require(
      !unsubscribeWholeObjectClassAfterInitialQuery ||
          options.attributeName != options.secondaryAttributeName,
      "Whole-class unsubscription requires two distinct adapter-selected attributes");
  require(
      !subscribeAllObjectClassAttributes ||
          hasObjectSubscription,
      "Complete object-class attribute subscription requires an object-class subscription");
  require(
      !activatePassiveCompleteObjectClassSubscriptionAfterInitialQuery ||
          (subscribeAllObjectClassAttributes && hasObjectSubscription &&
           !activeObjectSubscription),
      "Complete object-class subscription activation requires an initially passive full attribute-set subscription");
  require(
      !subscribeAllObjectClassAttributes ||
          (options.attributeName != options.secondaryAttributeName &&
           options.attributeName != options.objectClassThirdAttributeName &&
           options.secondaryAttributeName != options.objectClassThirdAttributeName),
      "Complete object-class attribute subscription requires three distinct adapter-selected attribute names");
  require(
      static_cast<unsigned>(partiallyUnsubscribeObjectAttributeAfterInitialQuery) +
              static_cast<unsigned>(fullyUnsubscribeObjectAttributesAfterInitialQuery) +
              static_cast<unsigned>(unsubscribeWholeObjectClassAfterInitialQuery) +
              static_cast<unsigned>(subscribeAllObjectClassAttributes &&
                                    !activatePassiveCompleteObjectClassSubscriptionAfterInitialQuery) +
              static_cast<unsigned>(activatePassiveCompleteObjectClassSubscriptionAfterInitialQuery) <=
          1U,
      "Partial unsubscribe, full attribute-set unsubscribe, whole-class unsubscribe, and complete attribute-set subscription are separate lifecycle scenarios");

  Session subject(options, model, "owner");
  Session requester(options, model, "member");
  auto const scenarioSuffix =
      activatePassiveCompleteObjectClassSubscriptionAfterInitialQuery
      ? "federate-mom-object-class-complete-attribute-subscription-activation-query"
      : subscribeAllObjectClassAttributes
      ? (activeObjectSubscription
             ? "federate-mom-object-class-complete-attribute-subscription-query"
             : "federate-mom-object-class-passive-complete-attribute-subscription-query")
      : unsubscribeWholeObjectClassAfterInitialQuery
      ? "federate-mom-object-class-full-unsubscribe-query"
      : fullyUnsubscribeObjectAttributesAfterInitialQuery
      ? "federate-mom-object-attribute-full-unsubscribe-query"
      : partiallyUnsubscribeObjectAttributeAfterInitialQuery
      ? "federate-mom-object-attribute-partial-unsubscribe-query"
      : unsubscribePrimaryInteractionAfterSecondaryUnsubscribe
      ? "federate-mom-multiple-interaction-subscription-full-unsubscribe-query"
      : unsubscribeSecondaryInteractionAfterInitialQuery
      ? "federate-mom-multiple-interaction-subscription-unsubscribe-query"
      : hasSecondaryInteractionSubscription
      ? "federate-mom-multiple-interaction-subscription-query"
      : activatePassiveInteractionAfterInitialQuery
      ? "federate-mom-passive-interaction-subscription-activation-query"
      : unsubscribeAllDirectedInteractionsAfterPartialQuery
      ? "federate-mom-multiple-directed-subscription-full-unsubscribe-query"
      : hasMultipleDirectedSubscriptions
      ? "federate-mom-multiple-directed-subscription-query"
      : !hasObjectSubscription &&
          !hasInteractionSubscription
      ? (hasDirectedSubscription
             ? "federate-mom-directed-subscription-query"
             : "federate-mom-empty-subscription-query")
      : activeObjectSubscription
          ? (activeInteractionSubscription
                 ? "federate-mom-subscription-query"
                 : "federate-mom-passive-interaction-subscription-query")
          : (activeInteractionSubscription
                 ? "federate-mom-passive-subscription-query"
                 : "federate-mom-passive-object-and-interaction-subscription-query");
  auto const federateNameSuffix =
      activatePassiveCompleteObjectClassSubscriptionAfterInitialQuery
      ? L"-mom-object-class-complete-attribute-subscription-activation-query"
      : subscribeAllObjectClassAttributes
      ? (activeObjectSubscription
             ? L"-mom-object-class-complete-attribute-subscription-query"
             : L"-mom-object-class-passive-complete-attribute-subscription-query")
      : unsubscribeWholeObjectClassAfterInitialQuery
      ? L"-mom-object-class-full-unsubscribe-query"
      : fullyUnsubscribeObjectAttributesAfterInitialQuery
      ? L"-mom-object-attribute-full-unsubscribe-query"
      : partiallyUnsubscribeObjectAttributeAfterInitialQuery
      ? L"-mom-object-attribute-partial-unsubscribe-query"
      : unsubscribePrimaryInteractionAfterSecondaryUnsubscribe
      ? L"-mom-multiple-interaction-subscription-full-unsubscribe-query"
      : unsubscribeSecondaryInteractionAfterInitialQuery
      ? L"-mom-multiple-interaction-subscription-unsubscribe-query"
      : hasSecondaryInteractionSubscription
      ? L"-mom-multiple-interaction-subscription-query"
      : activatePassiveInteractionAfterInitialQuery
      ? L"-mom-passive-interaction-subscription-activation-query"
      : unsubscribeAllDirectedInteractionsAfterPartialQuery
      ? L"-mom-multiple-directed-subscription-full-unsubscribe-query"
      : hasMultipleDirectedSubscriptions
      ? L"-mom-multiple-directed-subscription-query"
      : !hasObjectSubscription &&
          !hasInteractionSubscription
      ? (hasDirectedSubscription
             ? L"-mom-directed-subscription-query"
             : L"-mom-empty-subscription-query")
      : activeObjectSubscription
          ? (activeInteractionSubscription
                 ? L"-mom-subscription-query"
                 : L"-mom-passive-interaction-subscription-query")
          : (activeInteractionSubscription
                 ? L"-mom-passive-subscription-query"
                 : L"-mom-passive-object-and-interaction-subscription-query");
  auto const federation = federationName(options, scenarioSuffix);
  subject.connect();
  requester.connect();
  subject.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{
          options.fom.wstring(),
          options.rateFom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  auto const subjectName =
      options.ownerFederateName + federateNameSuffix + L"-subject";
  auto const requesterName =
      options.memberFederateName + federateNameSuffix + L"-requester";
  subject.join(subjectName, options.federateType, federation);
  requester.join(requesterName, options.federateType, federation);

  auto& subjectAmbassador = subject.rtiAmbassador();
  auto& requesterAmbassador = requester.rtiAmbassador();
  auto const subjectObjectClass =
      subjectAmbassador.getObjectClassHandle(options.objectClassName);
  auto const subjectAttribute = subjectAmbassador.getAttributeHandle(
      subjectObjectClass,
      options.attributeName);
  auto const subjectSecondaryAttribute =
      usesMultipleObjectAttributes
      ? subjectAmbassador.getAttributeHandle(
            subjectObjectClass,
            options.secondaryAttributeName)
      : rti::AttributeHandle{};
  auto const subjectThirdAttribute =
      usesThreeObjectAttributes
      ? subjectAmbassador.getAttributeHandle(
            subjectObjectClass,
            options.objectClassThirdAttributeName)
      : rti::AttributeHandle{};
  auto const subjectInteractionClass =
      subjectAmbassador.getInteractionClassHandle(options.interactionClassName);
  auto const requesterObjectClass =
      requesterAmbassador.getObjectClassHandle(options.objectClassName);
  auto const subjectSecondaryDirectedObjectClass =
      hasMultipleDirectedSubscriptions
          ? subjectAmbassador.getObjectClassHandle(
                options.secondaryDirectedObjectClassName)
          : rti::ObjectClassHandle{};
  auto const requesterSecondaryDirectedObjectClass =
      hasMultipleDirectedSubscriptions
          ? requesterAmbassador.getObjectClassHandle(
                options.secondaryDirectedObjectClassName)
          : rti::ObjectClassHandle{};
  auto const subjectSecondaryInteractionClass =
      hasSecondaryInteractionSubscription
          ? subjectAmbassador.getInteractionClassHandle(
                L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation")
          : rti::InteractionClassHandle{};
  auto const requesterSecondaryInteractionClass =
      hasSecondaryInteractionSubscription
          ? requesterAmbassador.getInteractionClassHandle(
                L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation")
          : rti::InteractionClassHandle{};
  auto const requesterAttribute = requesterAmbassador.getAttributeHandle(
      requesterObjectClass,
      options.attributeName);
  auto const requesterSecondaryAttribute =
      usesMultipleObjectAttributes
      ? requesterAmbassador.getAttributeHandle(
            requesterObjectClass,
            options.secondaryAttributeName)
      : rti::AttributeHandle{};
  auto const requesterThirdAttribute =
      usesThreeObjectAttributes
      ? requesterAmbassador.getAttributeHandle(
            requesterObjectClass,
            options.objectClassThirdAttributeName)
      : rti::AttributeHandle{};
  auto const requesterInteractionClass =
      requesterAmbassador.getInteractionClassHandle(options.interactionClassName);
  auto const subjectSecondaryDirectedInteractionClass =
      hasMultipleDirectedSubscriptions
          ? subjectAmbassador.getInteractionClassHandle(
                options.secondaryDirectedInteractionClassName)
          : rti::InteractionClassHandle{};
  auto const requesterSecondaryDirectedInteractionClass =
      hasMultipleDirectedSubscriptions
          ? requesterAmbassador.getInteractionClassHandle(
                options.secondaryDirectedInteractionClassName)
          : rti::InteractionClassHandle{};
  auto const targetFederate = requesterAmbassador.getFederateHandle(subjectName);
  auto const reliable =
      requesterAmbassador.getTransportationTypeHandle(L"HLAreliable");
  auto const requestClass = requesterAmbassador.getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestSubscriptions");
  auto const requestFederateParameter =
      requesterAmbassador.getParameterHandle(requestClass, L"HLAfederate");
  auto const objectReportClass = requesterAmbassador.getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportObjectClassSubscription");
  auto const objectReportCountParameter =
      requesterAmbassador.getParameterHandle(objectReportClass, L"HLAnumberOfClasses");
  auto const objectReportClassParameter =
      requesterAmbassador.getParameterHandle(objectReportClass, L"HLAobjectClass");
  auto const objectReportActiveParameter =
      requesterAmbassador.getParameterHandle(objectReportClass, L"HLAactive");
  auto const objectReportUpdateRateParameter =
      requesterAmbassador.getParameterHandle(objectReportClass, L"HLAmaxUpdateRate");
  auto const objectReportAttributesParameter =
      requesterAmbassador.getParameterHandle(objectReportClass, L"HLAattributeList");
  auto const interactionReportClass = requesterAmbassador.getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportInteractionSubscription");
  auto const interactionReportListParameter = requesterAmbassador.getParameterHandle(
      interactionReportClass,
      L"HLAinteractionClassList");
  auto const directedReportClass = requesterAmbassador.getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportDirectedInteractionSubscription");
  auto const directedReportCountParameter =
      requesterAmbassador.getParameterHandle(directedReportClass, L"HLAnumberOfClasses");
  auto const directedReportObjectClassParameter =
      requesterAmbassador.getParameterHandle(directedReportClass, L"HLAobjectClass");
  auto const directedReportListParameter = requesterAmbassador.getParameterHandle(
      directedReportClass,
      L"HLAinteractionClassList");
  require(
      subjectObjectClass.isValid() && subjectAttribute.isValid() &&
          subjectInteractionClass.isValid() &&
          requesterObjectClass.isValid() && requesterAttribute.isValid() &&
          requesterInteractionClass.isValid() && targetFederate.isValid() &&
          reliable.isValid() && requestClass.isValid() &&
          requestFederateParameter.isValid() && objectReportClass.isValid() &&
          objectReportCountParameter.isValid() &&
          objectReportClassParameter.isValid() &&
          objectReportActiveParameter.isValid() &&
          objectReportUpdateRateParameter.isValid() &&
          objectReportAttributesParameter.isValid() &&
          interactionReportClass.isValid() &&
          interactionReportListParameter.isValid() &&
          directedReportClass.isValid() &&
          directedReportCountParameter.isValid() &&
          directedReportObjectClassParameter.isValid() &&
          directedReportListParameter.isValid(),
      "Adapter-supplied application handles or standard MIM subscription request/report handles were invalid");
  require(
      !hasMultipleDirectedSubscriptions ||
          (subjectSecondaryDirectedObjectClass.isValid() &&
           requesterSecondaryDirectedObjectClass.isValid() &&
           subjectSecondaryDirectedObjectClass != subjectObjectClass &&
           requesterSecondaryDirectedObjectClass != requesterObjectClass &&
           subjectSecondaryDirectedInteractionClass.isValid() &&
           requesterSecondaryDirectedInteractionClass.isValid() &&
           subjectSecondaryDirectedInteractionClass != subjectInteractionClass &&
           requesterSecondaryDirectedInteractionClass != requesterInteractionClass),
      "The adapter-supplied second directed-subscription object or interaction class was invalid or duplicated the first class");
  require(
      !hasSecondaryInteractionSubscription ||
          (subjectSecondaryInteractionClass.isValid() &&
           requesterSecondaryInteractionClass.isValid() &&
           subjectSecondaryInteractionClass != subjectInteractionClass &&
           requesterSecondaryInteractionClass != requesterInteractionClass),
      "The standard MIM service-report interaction class was invalid or duplicated the adapter-selected interaction class");
  require(
      !usesMultipleObjectAttributes ||
          (subjectSecondaryAttribute.isValid() &&
           requesterSecondaryAttribute.isValid() &&
           subjectSecondaryAttribute != subjectAttribute &&
           requesterSecondaryAttribute != requesterAttribute &&
           subjectSecondaryAttribute == requesterSecondaryAttribute),
      "The adapter-selected second object attribute was invalid, duplicated, or inconsistent across federates");
  require(
      !usesThreeObjectAttributes ||
          (subjectThirdAttribute.isValid() && requesterThirdAttribute.isValid() &&
           subjectThirdAttribute != subjectAttribute &&
           subjectThirdAttribute != subjectSecondaryAttribute &&
           requesterThirdAttribute != requesterAttribute &&
           requesterThirdAttribute != requesterSecondaryAttribute &&
           subjectThirdAttribute == requesterThirdAttribute),
      "The adapter-selected third object attribute was invalid, duplicated, or inconsistent across federates");

  requesterAmbassador.subscribeInteractionClass(objectReportClass);
  requesterAmbassador.subscribeInteractionClass(interactionReportClass);
  requesterAmbassador.subscribeInteractionClass(directedReportClass);
  if (hasObjectSubscription) {
    subjectAmbassador.subscribeObjectClassAttributes(
        subjectObjectClass,
        usesThreeObjectAttributes
            ? rti::AttributeHandleSet{
                  subjectAttribute,
                  subjectSecondaryAttribute,
                  subjectThirdAttribute}
            : usesTwoObjectAttributes
            ? rti::AttributeHandleSet{subjectAttribute, subjectSecondaryAttribute}
            : rti::AttributeHandleSet{subjectAttribute},
        activeObjectSubscription,
        options.fomUpdateRateName);
  }
  if (hasInteractionSubscription) {
    subjectAmbassador.subscribeInteractionClass(
        subjectInteractionClass,
        activeInteractionSubscription);
  }
  if (hasSecondaryInteractionSubscription) {
    subjectAmbassador.subscribeInteractionClass(
        subjectSecondaryInteractionClass,
        activeSecondaryInteractionSubscription);
  }
  if (hasDirectedSubscription) {
    subjectAmbassador.subscribeObjectClassDirectedInteractions(
        subjectObjectClass,
        rti::InteractionClassHandleSet{subjectInteractionClass},
        false);
    if (hasMultipleDirectedSubscriptions) {
      subjectAmbassador.subscribeObjectClassDirectedInteractions(
          subjectSecondaryDirectedObjectClass,
          rti::InteractionClassHandleSet{
              subjectSecondaryDirectedInteractionClass},
          false);
    }
  }

  struct InteractionSubscription {
    rti::InteractionClassHandle interactionClass;
    bool active;
  };
  auto decodeAttributeHandleList = [&](rti::VariableLengthData const& encoded) {
    rti::HLAvariableArray const handlePrototype{rti::HLAoctet{}};
    rti::HLAvariableArray handles{
        static_cast<rti::DataElement const&>(handlePrototype)};
    handles.decode(encoded);
    std::vector<rti::AttributeHandle> decoded;
    decoded.reserve(handles.size());
    for (std::size_t index = 0; index != handles.size(); ++index) {
      auto const& encodedHandle =
          dynamic_cast<rti::HLAvariableArray const&>(handles.get(index));
      decoded.push_back(
          requesterAmbassador.decodeAttributeHandle(encodedHandle.encode()));
    }
    return decoded;
  };
  auto decodeInteractionClassHandleList =
      [&](rti::VariableLengthData const& encoded) {
        rti::HLAvariableArray const handlePrototype{rti::HLAoctet{}};
        rti::HLAvariableArray handles{
            static_cast<rti::DataElement const&>(handlePrototype)};
        handles.decode(encoded);
        std::vector<rti::InteractionClassHandle> decoded;
        decoded.reserve(handles.size());
        for (std::size_t index = 0; index != handles.size(); ++index) {
          auto const& encodedHandle =
              dynamic_cast<rti::HLAvariableArray const&>(handles.get(index));
          decoded.push_back(
              requesterAmbassador.decodeInteractionClassHandle(
                  encodedHandle.encode()));
        }
        return decoded;
      };
  auto decodeInteractionSubscriptions =
      [&](rti::VariableLengthData const& encoded) {
        rti::HLAvariableArray const handlePrototype{rti::HLAoctet{}};
        rti::HLAfixedRecord subscriptionPrototype;
        subscriptionPrototype.appendElement(handlePrototype);
        subscriptionPrototype.appendElement(rti::HLAboolean{});
        rti::HLAvariableArray subscriptions{
            static_cast<rti::DataElement const&>(subscriptionPrototype)};
        subscriptions.decode(encoded);
        std::vector<InteractionSubscription> decoded;
        decoded.reserve(subscriptions.size());
        for (std::size_t index = 0; index != subscriptions.size(); ++index) {
          auto const& record =
              dynamic_cast<rti::HLAfixedRecord const&>(subscriptions.get(index));
          auto const& encodedHandle =
              dynamic_cast<rti::HLAvariableArray const&>(record.get(0));
          auto const& active =
              dynamic_cast<rti::HLAboolean const&>(record.get(1));
          decoded.push_back(
              InteractionSubscription{
                  requesterAmbassador.decodeInteractionClassHandle(
                      encodedHandle.encode()),
                  active.get()});
        }
        return decoded;
      };

  rti::ParameterHandleValueMap const requestParameters{
      {requestFederateParameter, targetFederate.encode()}};
  requesterAmbassador.sendInteraction(
      requestClass,
      requestParameters,
      rti::VariableLengthData{});
  waitFor(
      requester,
      [&] {
        return requester.recorder().interactions().size() >=
            (hasMultipleDirectedSubscriptions ? 4U : 3U);
      },
      options,
      "the standard federate MOM subscription reports");
  auto const reports = requester.recorder().interactions();
  require(
      reports.size() == (hasMultipleDirectedSubscriptions ? 4U : 3U),
      "A subscription request did not deliver exactly the standard MIM reports for its directed-subscription object classes");

  InteractionRecord const* objectReport = nullptr;
  InteractionRecord const* interactionReport = nullptr;
  std::vector<InteractionRecord const*> directedReports;
  for (auto const& report : reports) {
    require(
        report.present && report.transportation == reliable,
        "A standard MIM subscription report was absent or did not use reliable transportation");
    if (report.interaction == objectReportClass) {
      require(objectReport == nullptr, "Duplicate object-class subscription report");
      objectReport = &report;
    } else if (report.interaction == interactionReportClass) {
      require(interactionReport == nullptr, "Duplicate interaction subscription report");
      interactionReport = &report;
    } else if (report.interaction == directedReportClass) {
      directedReports.push_back(&report);
    } else {
      require(false, "Subscription query delivered an unrequested interaction class");
    }
  }
  require(
      objectReport != nullptr && interactionReport != nullptr &&
          directedReports.size() ==
              (hasMultipleDirectedSubscriptions ? 2U : 1U),
      "Subscription query omitted or duplicated a standard MIM report for the directed-subscription object classes");

  rti::HLAinteger32BE objectClassCount;
  require(
      objectReport->parameters.count(objectReportCountParameter) == 1U,
      "Object-class subscription report omitted its MIM-defined class count");
  objectClassCount.decode(objectReport->parameters.at(objectReportCountParameter));
  if (hasObjectSubscription) {
    require(
        objectReport->parameters.size() == 5U &&
            objectReport->parameters.count(objectReportClassParameter) == 1U &&
            objectReport->parameters.count(objectReportActiveParameter) == 1U &&
            objectReport->parameters.count(objectReportUpdateRateParameter) ==
                1U &&
            objectReport->parameters.count(objectReportAttributesParameter) ==
                1U,
        "Populated object-class subscription report did not contain its five MIM-defined parameters");
    rti::HLAboolean objectSubscriptionActive;
    objectSubscriptionActive.decode(
        objectReport->parameters.at(objectReportActiveParameter));
    rti::HLAunicodeString objectSubscriptionUpdateRate;
    objectSubscriptionUpdateRate.decode(
        objectReport->parameters.at(objectReportUpdateRateParameter));
    require(
        objectClassCount.get() == 1 &&
            requesterAmbassador.decodeObjectClassHandle(
                    objectReport->parameters.at(objectReportClassParameter)) ==
                requesterObjectClass &&
            objectSubscriptionActive.get() == activeObjectSubscription &&
            objectSubscriptionUpdateRate.get() == options.fomUpdateRateName,
        "Object-class subscription report did not identify the adapter-selected subscription state and update-rate name");
    auto const subscribedAttributes = decodeAttributeHandleList(
        objectReport->parameters.at(objectReportAttributesParameter));
    if (usesThreeObjectAttributes) {
      require(
          subscribedAttributes.size() == 3U &&
              std::find(
                  subscribedAttributes.begin(),
                  subscribedAttributes.end(),
                  requesterAttribute) != subscribedAttributes.end() &&
              std::find(
                  subscribedAttributes.begin(),
                  subscribedAttributes.end(),
                  requesterSecondaryAttribute) != subscribedAttributes.end() &&
              std::find(
                  subscribedAttributes.begin(),
                  subscribedAttributes.end(),
                  requesterThirdAttribute) != subscribedAttributes.end(),
          "Whole-class-equivalent object subscription report did not contain all three adapter-selected attributes");
    } else if (usesTwoObjectAttributes) {
      require(
          subscribedAttributes.size() == 2U &&
              std::find(
                  subscribedAttributes.begin(),
                  subscribedAttributes.end(),
                  requesterAttribute) != subscribedAttributes.end() &&
              std::find(
                  subscribedAttributes.begin(),
                  subscribedAttributes.end(),
                  requesterSecondaryAttribute) != subscribedAttributes.end(),
          "Initial object-class subscription report did not contain both adapter-selected attributes");
    } else {
      require(
          subscribedAttributes.size() == 1U &&
              subscribedAttributes.front() == requesterAttribute,
          "Object-class subscription report did not contain exactly the adapter-selected attribute");
    }
  } else {
    require(
        objectReport->parameters.size() == 1U && objectClassCount.get() == 0 &&
            objectReport->parameters.count(objectReportClassParameter) == 0U &&
            objectReport->parameters.count(objectReportActiveParameter) == 0U &&
            objectReport->parameters.count(objectReportUpdateRateParameter) ==
                0U &&
            objectReport->parameters.count(objectReportAttributesParameter) ==
                0U,
        "Empty object-class subscription report did not use the standard zero-class NULL response shape");
  }

  if (partiallyUnsubscribeObjectAttributeAfterInitialQuery) {
    subjectAmbassador.unsubscribeObjectClassAttributes(
        subjectObjectClass,
        rti::AttributeHandleSet{subjectAttribute});
    auto const priorInteractionCount = requester.recorder().interactions().size();
    requesterAmbassador.sendInteraction(
        requestClass,
        requestParameters,
        rti::VariableLengthData{});
    waitFor(
        requester,
        [&] {
          return requester.recorder().interactions().size() >=
              priorInteractionCount + 3U;
        },
        options,
        "the standard MIM subscription reports after one object attribute is unsubscribed");
    auto const partialReports = requester.recorder().interactions();
    require(
        partialReports.size() == priorInteractionCount + 3U,
        "Partial object-attribute unsubscription did not produce exactly one fresh report of each standard subscription kind");
    InteractionRecord const* remainingObjectReport = nullptr;
    std::size_t remainingInteractionReportCount = 0U;
    std::size_t remainingDirectedReportCount = 0U;
    for (std::size_t index = priorInteractionCount;
         index < partialReports.size();
         ++index) {
      auto const& report = partialReports[index];
      require(
          report.present && report.transportation == reliable,
          "A post-unsubscribe standard MIM subscription report was absent or not reliably transported");
      if (report.interaction == objectReportClass) {
        require(
            remainingObjectReport == nullptr,
            "Duplicate object-class subscription report after partial unsubscription");
        remainingObjectReport = &report;
      } else if (report.interaction == interactionReportClass) {
        ++remainingInteractionReportCount;
      } else if (report.interaction == directedReportClass) {
        ++remainingDirectedReportCount;
      }
    }
    require(
        remainingObjectReport != nullptr &&
            remainingInteractionReportCount == 1U &&
            remainingDirectedReportCount == 1U,
        "Partial object-attribute unsubscription omitted or duplicated a standard MIM subscription report");
    rti::HLAinteger32BE remainingClassCount;
    require(
        remainingObjectReport->parameters.size() == 5U &&
            remainingObjectReport->parameters.count(objectReportCountParameter) == 1U &&
            remainingObjectReport->parameters.count(objectReportClassParameter) == 1U &&
            remainingObjectReport->parameters.count(objectReportActiveParameter) == 1U &&
            remainingObjectReport->parameters.count(objectReportUpdateRateParameter) == 1U &&
            remainingObjectReport->parameters.count(objectReportAttributesParameter) == 1U,
        "Post-unsubscribe object-class subscription report did not contain its five MIM-defined parameters");
    remainingClassCount.decode(
        remainingObjectReport->parameters.at(objectReportCountParameter));
    rti::HLAboolean remainingActive;
    remainingActive.decode(
        remainingObjectReport->parameters.at(objectReportActiveParameter));
    rti::HLAunicodeString remainingUpdateRate;
    remainingUpdateRate.decode(
        remainingObjectReport->parameters.at(objectReportUpdateRateParameter));
    auto const remainingAttributes = decodeAttributeHandleList(
        remainingObjectReport->parameters.at(objectReportAttributesParameter));
    require(
        remainingClassCount.get() == 1 &&
            requesterAmbassador.decodeObjectClassHandle(
                remainingObjectReport->parameters.at(objectReportClassParameter)) ==
                requesterObjectClass &&
            remainingActive.get() == activeObjectSubscription &&
            remainingUpdateRate.get() == options.fomUpdateRateName &&
            remainingAttributes.size() == 1U &&
            remainingAttributes.front() == requesterSecondaryAttribute,
        "Post-unsubscribe MIM report did not retain only the still-subscribed adapter-selected attribute and original subscription settings");
  }

  if (usesFullObjectUnsubscription) {
    if (unsubscribeWholeObjectClassAfterInitialQuery) {
      subjectAmbassador.unsubscribeObjectClass(subjectObjectClass);
    } else {
      subjectAmbassador.unsubscribeObjectClassAttributes(
          subjectObjectClass,
          rti::AttributeHandleSet{subjectAttribute, subjectSecondaryAttribute});
    }
    auto const priorInteractionCount = requester.recorder().interactions().size();
    requesterAmbassador.sendInteraction(
        requestClass,
        requestParameters,
        rti::VariableLengthData{});
    waitFor(
        requester,
        [&] {
          return requester.recorder().interactions().size() >=
              priorInteractionCount + 3U;
        },
        options,
        "the standard MIM subscription reports after all object attributes are unsubscribed");
    auto const emptyReports = requester.recorder().interactions();
    require(
        emptyReports.size() == priorInteractionCount + 3U,
        "Full object-attribute unsubscription did not produce exactly one fresh report of each standard subscription kind");
    InteractionRecord const* emptyObjectReport = nullptr;
    InteractionRecord const* emptyInteractionReport = nullptr;
    InteractionRecord const* emptyDirectedReport = nullptr;
    for (std::size_t index = priorInteractionCount;
         index < emptyReports.size();
         ++index) {
      auto const& report = emptyReports[index];
      require(
          report.present && report.transportation == reliable,
          "A post-unsubscribe standard MIM subscription report was absent or not reliably transported");
      if (report.interaction == objectReportClass) {
        require(
            emptyObjectReport == nullptr,
            "Duplicate object-class subscription NULL report after full unsubscription");
        emptyObjectReport = &report;
      } else if (report.interaction == interactionReportClass) {
        require(
            emptyInteractionReport == nullptr,
            "Duplicate interaction-subscription report after full object unsubscription");
        emptyInteractionReport = &report;
      } else if (report.interaction == directedReportClass) {
        require(
            emptyDirectedReport == nullptr,
            "Duplicate directed-subscription report after full object unsubscription");
        emptyDirectedReport = &report;
      }
    }
    require(
        emptyObjectReport != nullptr && emptyInteractionReport != nullptr &&
            emptyDirectedReport != nullptr,
        "Full object-attribute unsubscription omitted a standard MIM subscription report");
    rti::HLAinteger32BE emptyObjectClassCount;
    require(
        emptyObjectReport->parameters.size() == 1U &&
            emptyObjectReport->parameters.count(objectReportCountParameter) == 1U &&
            emptyObjectReport->parameters.count(objectReportClassParameter) == 0U &&
            emptyObjectReport->parameters.count(objectReportActiveParameter) == 0U &&
            emptyObjectReport->parameters.count(objectReportUpdateRateParameter) == 0U &&
            emptyObjectReport->parameters.count(objectReportAttributesParameter) == 0U,
        "Full object-attribute unsubscription did not use the exact MIM NULL response parameter shape");
    emptyObjectClassCount.decode(
        emptyObjectReport->parameters.at(objectReportCountParameter));
    require(
        emptyObjectClassCount.get() == 0,
        "Full object-attribute unsubscription NULL response did not report zero classes");
    require(
        emptyInteractionReport->parameters.size() == 1U &&
            emptyInteractionReport->parameters.count(
                interactionReportListParameter) == 1U &&
            decodeInteractionSubscriptions(
                emptyInteractionReport->parameters.at(
                    interactionReportListParameter)).empty(),
        "Full object-attribute unsubscription changed the ordinary interaction-subscription NULL report");
    rti::HLAinteger32BE emptyDirectedClassCount;
    require(
        emptyDirectedReport->parameters.size() == 2U &&
            emptyDirectedReport->parameters.count(directedReportCountParameter) == 1U &&
            emptyDirectedReport->parameters.count(
                directedReportObjectClassParameter) == 0U &&
            emptyDirectedReport->parameters.count(directedReportListParameter) == 1U,
        "Full object-attribute unsubscription changed the directed-subscription NULL parameter shape");
    emptyDirectedClassCount.decode(
        emptyDirectedReport->parameters.at(directedReportCountParameter));
    require(
        emptyDirectedClassCount.get() == 0 &&
            decodeInteractionClassHandleList(
                emptyDirectedReport->parameters.at(
                    directedReportListParameter)).empty(),
        "Full object-attribute unsubscription changed the directed-subscription NULL response");
  }

  require(
      interactionReport->parameters.size() == 1U &&
          interactionReport->parameters.count(interactionReportListParameter) ==
              1U,
      "Interaction subscription report did not contain its MIM-defined class list");
  auto const subscribedInteractions = decodeInteractionSubscriptions(
      interactionReport->parameters.at(interactionReportListParameter));
  if (hasInteractionSubscription) {
    if (hasSecondaryInteractionSubscription) {
      bool foundPrimary = false;
      bool foundSecondary = false;
      for (auto const& subscription : subscribedInteractions) {
        if (subscription.interactionClass == requesterInteractionClass) {
          require(!foundPrimary, "Duplicate primary interaction subscription report");
          foundPrimary = true;
          require(
              subscription.active == activeInteractionSubscription,
              "The MIM interaction list reported the primary interaction with the wrong active/passive state");
        } else if (subscription.interactionClass ==
                   requesterSecondaryInteractionClass) {
          require(!foundSecondary, "Duplicate secondary interaction subscription report");
          foundSecondary = true;
          require(
              subscription.active == activeSecondaryInteractionSubscription,
              "The MIM interaction list reported the standard MIM interaction with the wrong active/passive state");
        }
      }
      require(
          subscribedInteractions.size() == 2U && foundPrimary && foundSecondary,
          "Interaction subscription report did not contain exactly the two requested classes and their mixed active/passive states");
    } else {
      require(
          subscribedInteractions.size() == 1U &&
              subscribedInteractions.front().interactionClass ==
                  requesterInteractionClass &&
              subscribedInteractions.front().active == activeInteractionSubscription,
          "Interaction subscription report did not contain exactly the adapter-selected interaction subscription mode");
    }
  } else {
    require(
        subscribedInteractions.empty(),
        "Empty interaction subscription report did not use the standard empty class-list NULL response shape");
  }

  if (hasDirectedSubscription) {
    if (hasMultipleDirectedSubscriptions) {
      bool reportedPrimaryObjectClass = false;
      bool reportedSecondaryObjectClass = false;
      for (auto const* directedReport : directedReports) {
        rti::HLAinteger32BE directedClassCount;
        require(
            directedReport->parameters.size() == 3U &&
                directedReport->parameters.count(
                    directedReportCountParameter) == 1U &&
                directedReport->parameters.count(
                    directedReportObjectClassParameter) == 1U &&
                directedReport->parameters.count(
                    directedReportListParameter) == 1U,
            "Populated directed-interaction subscription report did not contain its three MIM-defined parameters");
        directedClassCount.decode(
            directedReport->parameters.at(directedReportCountParameter));
        auto const reportObjectClass =
            requesterAmbassador.decodeObjectClassHandle(
                directedReport->parameters.at(
                    directedReportObjectClassParameter));
        auto const directedInteractions = decodeInteractionClassHandleList(
            directedReport->parameters.at(directedReportListParameter));
        require(
            directedClassCount.get() == 2 &&
                directedInteractions.size() == 1U,
            "Multiple directed-subscription report did not report two subscribed object classes and one interaction for this object class");
        if (reportObjectClass == requesterObjectClass) {
          require(
              !reportedPrimaryObjectClass &&
                  directedInteractions.front() == requesterInteractionClass,
              "Primary directed-subscription report duplicated its object class or identified the wrong interaction");
          reportedPrimaryObjectClass = true;
        } else if (reportObjectClass ==
                   requesterSecondaryDirectedObjectClass) {
          require(
              !reportedSecondaryObjectClass &&
                  directedInteractions.front() ==
                      requesterSecondaryDirectedInteractionClass,
              "Secondary directed-subscription report duplicated its object class or identified the wrong interaction");
          reportedSecondaryObjectClass = true;
        } else {
          require(
              false,
              "Directed-subscription report identified an unselected object class");
        }
      }
      require(
          reportedPrimaryObjectClass && reportedSecondaryObjectClass,
          "Multiple directed-subscription reports did not cover both adapter-selected object classes");
    } else {
      auto const* directedReport = directedReports.front();
      rti::HLAinteger32BE directedClassCount;
      require(
          directedReport->parameters.count(directedReportCountParameter) ==
              1U,
          "Directed-interaction subscription report omitted its MIM-defined class count");
      directedClassCount.decode(
          directedReport->parameters.at(directedReportCountParameter));
      require(
          directedReport->parameters.size() == 3U &&
              directedReport->parameters.count(
                  directedReportObjectClassParameter) == 1U &&
              directedReport->parameters.count(directedReportListParameter) ==
                  1U,
          "Populated directed-interaction subscription report did not contain its three MIM-defined parameters");
      auto const directedInteractions = decodeInteractionClassHandleList(
          directedReport->parameters.at(directedReportListParameter));
      require(
          directedClassCount.get() == 1 &&
              requesterAmbassador.decodeObjectClassHandle(
                      directedReport->parameters.at(
                          directedReportObjectClassParameter)) ==
                  requesterObjectClass &&
              directedInteractions.size() == 1U &&
              directedInteractions.front() == requesterInteractionClass,
          "Populated directed-interaction subscription report did not identify the adapter-selected object and interaction classes");
    }
  } else {
    auto const* directedReport = directedReports.front();
    rti::HLAinteger32BE directedClassCount;
    require(
        directedReport->parameters.count(directedReportCountParameter) == 1U,
        "Empty directed-interaction subscription report omitted its MIM-defined class count");
    directedClassCount.decode(
        directedReport->parameters.at(directedReportCountParameter));
    require(
        directedReport->parameters.size() == 2U &&
            directedReport->parameters.count(
                directedReportObjectClassParameter) == 0U &&
            directedReport->parameters.count(directedReportListParameter) == 1U,
        "Empty directed-interaction subscription report did not use the standard NULL response parameter shape");
    require(
        directedClassCount.get() == 0 &&
            decodeInteractionClassHandleList(
                directedReport->parameters.at(directedReportListParameter)).empty(),
        "Empty directed-interaction subscription report did not report zero classes and an empty list");
  }

  if (activatePassiveCompleteObjectClassSubscriptionAfterInitialQuery) {
    subjectAmbassador.subscribeObjectClassAttributes(
        subjectObjectClass,
        rti::AttributeHandleSet{
            subjectAttribute,
            subjectSecondaryAttribute,
            subjectThirdAttribute},
        true,
        options.fomUpdateRateName);
    auto const priorInteractionCount =
        requester.recorder().interactions().size();
    requesterAmbassador.sendInteraction(
        requestClass,
        requestParameters,
        rti::VariableLengthData{});
    waitFor(
        requester,
        [&] {
          return requester.recorder().interactions().size() >=
              priorInteractionCount + 3U;
        },
        options,
        "the standard MIM subscription reports after activating the complete object-class attribute set");
    auto const activatedReports = requester.recorder().interactions();
    require(
        activatedReports.size() == priorInteractionCount + 3U,
        "Complete object-class subscription activation did not produce exactly one fresh report of each standard subscription kind");
    InteractionRecord const* activatedObjectReport = nullptr;
    std::size_t activatedInteractionReportCount = 0U;
    std::size_t activatedDirectedReportCount = 0U;
    for (std::size_t index = priorInteractionCount;
         index < activatedReports.size();
         ++index) {
      auto const& report = activatedReports[index];
      require(
          report.present && report.transportation == reliable,
          "A post-activation standard MIM subscription report was absent or not reliably transported");
      if (report.interaction == objectReportClass) {
        require(
            activatedObjectReport == nullptr,
            "Duplicate object-class subscription report after complete-set activation");
        activatedObjectReport = &report;
      } else if (report.interaction == interactionReportClass) {
        ++activatedInteractionReportCount;
      } else if (report.interaction == directedReportClass) {
        ++activatedDirectedReportCount;
      }
    }
    require(
        activatedObjectReport != nullptr &&
            activatedInteractionReportCount == 1U &&
            activatedDirectedReportCount == 1U,
        "Complete object-class subscription activation omitted or duplicated a standard MIM subscription report");
    require(
        activatedObjectReport->parameters.size() == 5U &&
            activatedObjectReport->parameters.count(objectReportCountParameter) ==
                1U &&
            activatedObjectReport->parameters.count(objectReportClassParameter) ==
                1U &&
            activatedObjectReport->parameters.count(objectReportActiveParameter) ==
                1U &&
            activatedObjectReport->parameters.count(
                objectReportUpdateRateParameter) == 1U &&
            activatedObjectReport->parameters.count(
                objectReportAttributesParameter) == 1U,
        "Activated object-class subscription report did not contain all five MIM-defined parameters");
    rti::HLAinteger32BE activatedClassCount;
    activatedClassCount.decode(
        activatedObjectReport->parameters.at(objectReportCountParameter));
    rti::HLAboolean activatedSubscriptionState;
    activatedSubscriptionState.decode(
        activatedObjectReport->parameters.at(objectReportActiveParameter));
    rti::HLAunicodeString activatedUpdateRate;
    activatedUpdateRate.decode(
        activatedObjectReport->parameters.at(objectReportUpdateRateParameter));
    auto const activatedAttributes = decodeAttributeHandleList(
        activatedObjectReport->parameters.at(objectReportAttributesParameter));
    require(
        activatedClassCount.get() == 1 &&
            requesterAmbassador.decodeObjectClassHandle(
                activatedObjectReport->parameters.at(objectReportClassParameter)) ==
                requesterObjectClass &&
            activatedSubscriptionState.get() &&
            activatedUpdateRate.get() == options.fomUpdateRateName &&
            activatedAttributes.size() == 3U &&
            std::find(
                activatedAttributes.begin(),
                activatedAttributes.end(),
                requesterAttribute) != activatedAttributes.end() &&
            std::find(
                activatedAttributes.begin(),
                activatedAttributes.end(),
                requesterSecondaryAttribute) != activatedAttributes.end() &&
            std::find(
                activatedAttributes.begin(),
                activatedAttributes.end(),
                requesterThirdAttribute) != activatedAttributes.end(),
        "The standard MIM did not report the complete attribute set as active after the repeated standard subscription service call");
  }

  if (activatePassiveInteractionAfterInitialQuery) {
    subjectAmbassador.subscribeInteractionClass(
        subjectInteractionClass,
        true);
    auto const priorInteractionCount =
        requester.recorder().interactions().size();
    requesterAmbassador.sendInteraction(
        requestClass,
        requestParameters,
        rti::VariableLengthData{});
    waitFor(
        requester,
        [&] {
          return requester.recorder().interactions().size() >=
              priorInteractionCount + 3U;
        },
        options,
        "the standard MIM subscription reports after passive interaction activation");
    auto const activatedReports = requester.recorder().interactions();
    require(
        activatedReports.size() == priorInteractionCount + 3U,
        "Passive interaction activation did not produce exactly one fresh report of each standard subscription kind");
    InteractionRecord const* activatedInteractionReport = nullptr;
    std::size_t activatedObjectReportCount = 0U;
    std::size_t activatedDirectedReportCount = 0U;
    for (std::size_t index = priorInteractionCount;
         index < activatedReports.size();
         ++index) {
      auto const& report = activatedReports[index];
      require(
          report.present && report.transportation == reliable,
          "A post-activation standard MIM subscription report was absent or not reliably transported");
      if (report.interaction == interactionReportClass) {
        require(
            activatedInteractionReport == nullptr,
            "Duplicate interaction-subscription report after passive activation");
        activatedInteractionReport = &report;
      } else if (report.interaction == objectReportClass) {
        ++activatedObjectReportCount;
      } else if (report.interaction == directedReportClass) {
        ++activatedDirectedReportCount;
      }
    }
    require(
        activatedInteractionReport != nullptr &&
            activatedObjectReportCount == 1U &&
            activatedDirectedReportCount == 1U,
        "Passive interaction activation omitted or duplicated a standard MIM subscription report");
    require(
        activatedInteractionReport->parameters.size() == 1U &&
            activatedInteractionReport->parameters.count(
                interactionReportListParameter) == 1U,
        "Post-activation interaction report did not contain its MIM-defined class list");
    auto const activatedSubscriptions = decodeInteractionSubscriptions(
        activatedInteractionReport->parameters.at(
            interactionReportListParameter));
    require(
        activatedSubscriptions.size() == 1U &&
            activatedSubscriptions.front().interactionClass ==
                requesterInteractionClass &&
            activatedSubscriptions.front().active,
        "The standard MIM interaction-subscription list did not report the passive-to-active transition");
  }

  if (unsubscribeSecondaryInteractionAfterInitialQuery) {
    subjectAmbassador.unsubscribeInteractionClass(
        subjectSecondaryInteractionClass);
    auto const priorInteractionCount =
        requester.recorder().interactions().size();
    requesterAmbassador.sendInteraction(
        requestClass,
        requestParameters,
        rti::VariableLengthData{});
    waitFor(
        requester,
        [&] {
          return requester.recorder().interactions().size() >=
              priorInteractionCount + 3U;
        },
        options,
        "the standard MIM subscription reports after one interaction class is unsubscribed");
    auto const remainingReports = requester.recorder().interactions();
    require(
        remainingReports.size() == priorInteractionCount + 3U,
        "Unsubscribing one interaction class did not produce exactly one fresh report of each standard subscription kind");
    InteractionRecord const* remainingInteractionReport = nullptr;
    std::size_t remainingObjectReportCount = 0U;
    std::size_t remainingDirectedReportCount = 0U;
    for (std::size_t index = priorInteractionCount;
         index < remainingReports.size();
         ++index) {
      auto const& report = remainingReports[index];
      require(
          report.present && report.transportation == reliable,
          "A post-unsubscribe standard MIM subscription report was absent or not reliably transported");
      if (report.interaction == interactionReportClass) {
        require(
            remainingInteractionReport == nullptr,
            "Duplicate interaction-subscription report after unsubscription");
        remainingInteractionReport = &report;
      } else if (report.interaction == objectReportClass) {
        ++remainingObjectReportCount;
      } else if (report.interaction == directedReportClass) {
        ++remainingDirectedReportCount;
      }
    }
    require(
        remainingInteractionReport != nullptr &&
            remainingObjectReportCount == 1U &&
            remainingDirectedReportCount == 1U,
        "Unsubscription omitted or duplicated a standard MIM subscription report");
    require(
        remainingInteractionReport->parameters.size() == 1U &&
            remainingInteractionReport->parameters.count(
                interactionReportListParameter) == 1U,
        "Post-unsubscribe interaction report did not contain its MIM-defined class list");
    auto const remainingSubscriptions = decodeInteractionSubscriptions(
        remainingInteractionReport->parameters.at(
            interactionReportListParameter));
    require(
        remainingSubscriptions.size() == 1U &&
            remainingSubscriptions.front().interactionClass ==
                requesterInteractionClass &&
            remainingSubscriptions.front().active,
        "The post-unsubscribe MIM list did not remove only the passive secondary interaction and preserve the active primary interaction");
  }

  if (unsubscribePrimaryInteractionAfterSecondaryUnsubscribe) {
    subjectAmbassador.unsubscribeInteractionClass(subjectInteractionClass);
    auto const priorInteractionCount =
        requester.recorder().interactions().size();
    requesterAmbassador.sendInteraction(
        requestClass,
        requestParameters,
        rti::VariableLengthData{});
    waitFor(
        requester,
        [&] {
          return requester.recorder().interactions().size() >=
              priorInteractionCount + 3U;
        },
        options,
        "the standard MIM subscription reports after the last interaction class is unsubscribed");
    auto const emptyReports = requester.recorder().interactions();
    require(
        emptyReports.size() == priorInteractionCount + 3U,
        "Full interaction unsubscription did not produce exactly one fresh report of each standard subscription kind");
    InteractionRecord const* emptyInteractionReport = nullptr;
    std::size_t emptyObjectReportCount = 0U;
    std::size_t emptyDirectedReportCount = 0U;
    for (std::size_t index = priorInteractionCount;
         index < emptyReports.size();
         ++index) {
      auto const& report = emptyReports[index];
      require(
          report.present && report.transportation == reliable,
          "A post-unsubscribe standard MIM subscription report was absent or not reliably transported");
      if (report.interaction == interactionReportClass) {
        require(
            emptyInteractionReport == nullptr,
            "Duplicate interaction-subscription NULL report after full unsubscription");
        emptyInteractionReport = &report;
      } else if (report.interaction == objectReportClass) {
        ++emptyObjectReportCount;
      } else if (report.interaction == directedReportClass) {
        ++emptyDirectedReportCount;
      }
    }
    require(
        emptyInteractionReport != nullptr &&
            emptyObjectReportCount == 1U &&
            emptyDirectedReportCount == 1U,
        "Full interaction unsubscription omitted or duplicated a standard MIM subscription report");
    require(
        emptyInteractionReport->parameters.size() == 1U &&
            emptyInteractionReport->parameters.count(
                interactionReportListParameter) == 1U &&
            decodeInteractionSubscriptions(
                emptyInteractionReport->parameters.at(
                    interactionReportListParameter)).empty(),
        "The final MIM interaction-subscription report did not use the standard empty-list NULL response");
  }

  if (hasMultipleDirectedSubscriptions) {
    subjectAmbassador.unsubscribeObjectClassDirectedInteractions(
        subjectObjectClass,
        rti::InteractionClassHandleSet{subjectInteractionClass});

    auto const interactionsBeforePartialUnsubscribeQuery =
        requester.recorder().interactions().size();
    requesterAmbassador.sendInteraction(
        requestClass,
        requestParameters,
        rti::VariableLengthData{});
    waitFor(
        requester,
        [&] {
          return requester.recorder().interactions().size() >=
              interactionsBeforePartialUnsubscribeQuery + 3U;
        },
        options,
        "the directed-subscription report after set-based partial unsubscribe");
    auto const partialUnsubscribeReports =
        requester.recorder().interactions();
    require(
        partialUnsubscribeReports.size() ==
            interactionsBeforePartialUnsubscribeQuery + 3U,
        "Partial directed unsubscription did not produce exactly three standard subscription reports");
    InteractionRecord const* partialDirectedReport = nullptr;
    std::size_t partialObjectReportCount = 0U;
    std::size_t partialInteractionReportCount = 0U;
    for (std::size_t index =
             interactionsBeforePartialUnsubscribeQuery;
         index != partialUnsubscribeReports.size();
         ++index) {
      auto const& report = partialUnsubscribeReports[index];
      require(
          report.present && report.transportation == reliable,
          "A partial-unsubscribe MIM report was absent or did not use reliable transportation");
      if (report.interaction == objectReportClass) {
        ++partialObjectReportCount;
      } else if (report.interaction == interactionReportClass) {
        ++partialInteractionReportCount;
      } else if (report.interaction == directedReportClass) {
        require(
            partialDirectedReport == nullptr,
            "Partial directed unsubscription produced duplicate directed-subscription reports");
        partialDirectedReport = &report;
      } else {
        require(false, "Partial directed unsubscription produced an unexpected MIM report");
      }
    }
    require(
        partialObjectReportCount == 1U &&
            partialInteractionReportCount == 1U &&
            partialDirectedReport != nullptr,
        "Partial directed unsubscription omitted a standard subscription report");
    require(
        partialDirectedReport->parameters.size() == 3U &&
            partialDirectedReport->parameters.count(
                directedReportCountParameter) == 1U &&
            partialDirectedReport->parameters.count(
                directedReportObjectClassParameter) == 1U &&
            partialDirectedReport->parameters.count(
                directedReportListParameter) == 1U,
        "Partial directed-subscription report did not contain its three MIM-defined parameters");
    rti::HLAinteger32BE partialDirectedClassCount;
    partialDirectedClassCount.decode(
        partialDirectedReport->parameters.at(directedReportCountParameter));
    auto const remainingDirectedInteractions =
        decodeInteractionClassHandleList(
            partialDirectedReport->parameters.at(directedReportListParameter));
    require(
        partialDirectedClassCount.get() == 1 &&
            requesterAmbassador.decodeObjectClassHandle(
                    partialDirectedReport->parameters.at(
                        directedReportObjectClassParameter)) ==
                requesterSecondaryDirectedObjectClass &&
            remainingDirectedInteractions.size() == 1U &&
            remainingDirectedInteractions.front() ==
                requesterSecondaryDirectedInteractionClass,
        "Partial set unsubscribe did not leave exactly the second directed interaction in the MIM report");
  }

  if (unsubscribeAllDirectedInteractionsAfterPartialQuery) {
    auto const interactionsBeforeFullUnsubscribeQuery =
        requester.recorder().interactions().size();
    subjectAmbassador.unsubscribeObjectClassDirectedInteractions(
        subjectSecondaryDirectedObjectClass);
    requesterAmbassador.sendInteraction(
        requestClass,
        requestParameters,
        rti::VariableLengthData{});
    waitFor(
        requester,
        [&] {
          return requester.recorder().interactions().size() >=
              interactionsBeforeFullUnsubscribeQuery + 3U;
        },
        options,
        "the standard directed-subscription NULL response after full unsubscribe");
    auto const reportsAfterFullUnsubscribe = requester.recorder().interactions();
    require(
        reportsAfterFullUnsubscribe.size() ==
            interactionsBeforeFullUnsubscribeQuery + 3U,
        "A subscription request after full directed unsubscribe did not deliver exactly the ordinary NULL reports and one directed-subscription NULL response");

    InteractionRecord const* emptyObjectReport = nullptr;
    InteractionRecord const* emptyInteractionReport = nullptr;
    InteractionRecord const* directedNullReport = nullptr;
    for (std::size_t index = interactionsBeforeFullUnsubscribeQuery;
         index != reportsAfterFullUnsubscribe.size();
         ++index) {
      auto const& report = reportsAfterFullUnsubscribe[index];
      require(
          report.present && report.transportation == reliable,
          "A full-unsubscribe MIM report was absent or did not use reliable transportation");
      if (report.interaction == objectReportClass) {
        require(
            emptyObjectReport == nullptr,
            "Full-unsubscribe query delivered duplicate object-subscription reports");
        emptyObjectReport = &report;
      } else if (report.interaction == interactionReportClass) {
        require(
            emptyInteractionReport == nullptr,
            "Full-unsubscribe query delivered duplicate interaction-subscription reports");
        emptyInteractionReport = &report;
      } else if (report.interaction == directedReportClass) {
        require(
            directedNullReport == nullptr,
            "Full-unsubscribe query delivered more than one directed-subscription NULL response");
        directedNullReport = &report;
      } else {
        require(
            false,
            "Full-unsubscribe query delivered an unrequested subscription report");
      }
    }
    require(
        emptyObjectReport != nullptr && emptyInteractionReport != nullptr &&
            directedNullReport != nullptr,
        "Full-unsubscribe query omitted an ordinary NULL report or the directed-subscription NULL response");

    rti::HLAinteger32BE emptyObjectClassCount;
    require(
        emptyObjectReport->parameters.size() == 1U &&
            emptyObjectReport->parameters.count(objectReportCountParameter) ==
                1U &&
            emptyObjectReport->parameters.count(objectReportClassParameter) ==
                0U &&
            emptyObjectReport->parameters.count(objectReportActiveParameter) ==
                0U &&
            emptyObjectReport->parameters.count(
                objectReportUpdateRateParameter) == 0U &&
            emptyObjectReport->parameters.count(
                objectReportAttributesParameter) == 0U,
        "Full-unsubscribe object-subscription report did not preserve the standard NULL response shape");
    emptyObjectClassCount.decode(
        emptyObjectReport->parameters.at(objectReportCountParameter));
    require(
        emptyObjectClassCount.get() == 0,
        "Full-unsubscribe object-subscription NULL response did not report zero classes");

    require(
        emptyInteractionReport->parameters.size() == 1U &&
            emptyInteractionReport->parameters.count(
                interactionReportListParameter) == 1U &&
            decodeInteractionSubscriptions(
                emptyInteractionReport->parameters.at(
                    interactionReportListParameter)).empty(),
        "Full-unsubscribe query changed the ordinary interaction-subscription NULL response");

    require(
        directedNullReport->parameters.size() == 2U &&
            directedNullReport->parameters.count(directedReportCountParameter) ==
                1U &&
            directedNullReport->parameters.count(
                directedReportObjectClassParameter) == 0U &&
            directedNullReport->parameters.count(directedReportListParameter) ==
                1U,
        "Full-unsubscribe directed report did not use the exact IEEE 1516.2-2025 NULL response parameter shape");
    rti::HLAinteger32BE directedClassCount;
    directedClassCount.decode(
        directedNullReport->parameters.at(directedReportCountParameter));
    require(
        directedClassCount.get() == 0 &&
            decodeInteractionClassHandleList(
                directedNullReport->parameters.at(
                    directedReportListParameter)).empty(),
        "Full-unsubscribe directed report did not encode zero classes and an empty interaction list");
  }

  if (partiallyUnsubscribeObjectAttributeAfterInitialQuery) {
    subjectAmbassador.unsubscribeObjectClassAttributes(
        subjectObjectClass,
        rti::AttributeHandleSet{subjectSecondaryAttribute});
  } else if (hasObjectSubscription &&
             !usesFullObjectUnsubscription) {
    subjectAmbassador.unsubscribeObjectClassAttributes(
        subjectObjectClass,
        rti::AttributeHandleSet{subjectAttribute});
  }
  if (hasInteractionSubscription &&
      !unsubscribePrimaryInteractionAfterSecondaryUnsubscribe) {
    subjectAmbassador.unsubscribeInteractionClass(subjectInteractionClass);
  }
  if (hasSecondaryInteractionSubscription &&
      !unsubscribeSecondaryInteractionAfterInitialQuery) {
    subjectAmbassador.unsubscribeInteractionClass(
        subjectSecondaryInteractionClass);
  }
  if (hasMultipleDirectedSubscriptions &&
      !unsubscribeAllDirectedInteractionsAfterPartialQuery) {
    subjectAmbassador.unsubscribeObjectClassDirectedInteractions(
        subjectSecondaryDirectedObjectClass,
        rti::InteractionClassHandleSet{
            subjectSecondaryDirectedInteractionClass});
  } else if (hasDirectedSubscription) {
    subjectAmbassador.unsubscribeObjectClassDirectedInteractions(
        subjectObjectClass,
        rti::InteractionClassHandleSet{subjectInteractionClass});
  }
  requesterAmbassador.unsubscribeInteractionClass(objectReportClass);
  requesterAmbassador.unsubscribeInteractionClass(interactionReportClass);
  requesterAmbassador.unsubscribeInteractionClass(directedReportClass);
  requester.resign(rti::NO_ACTION);
  subject.resign(rti::NO_ACTION);
  subjectAmbassador.destroyFederationExecution(federation);
  requester.disconnect();
  subject.disconnect();
}

void scenarioFederateMomSubscriptionQuery(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateMomSubscriptionQueryState(
      options, model, true, true, true, true, false, false);
}

void scenarioFederateMomPassiveSubscriptionQuery(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateMomSubscriptionQueryState(
      options, model, true, false, true, true, false, false);
}

void scenarioFederateMomPassiveInteractionSubscriptionQuery(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateMomSubscriptionQueryState(
      options, model, true, true, true, false, false, false);
}

void scenarioFederateMomPassiveInteractionSubscriptionActivationQuery(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateMomSubscriptionQueryState(
      options, model, true, true, true, false, false, false, false, true);
}

void scenarioFederateMomMultipleInteractionSubscriptionQuery(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateMomSubscriptionQueryState(
      options,
      model,
      true,
      true,
      true,
      true,
      false,
      false,
      false,
      false,
      true,
      false);
}

void scenarioFederateMomMultipleInteractionSubscriptionUnsubscribeQuery(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateMomSubscriptionQueryState(
      options,
      model,
      true,
      true,
      true,
      true,
      false,
      false,
      false,
      false,
      true,
      false,
      true);
}

void scenarioFederateMomMultipleInteractionSubscriptionFullUnsubscribeQuery(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateMomSubscriptionQueryState(
      options,
      model,
      true,
      true,
      true,
      true,
      false,
      false,
      false,
      false,
      true,
      false,
      true,
      true);
}

void scenarioFederateMomObjectAttributePartialUnsubscribeQuery(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateMomSubscriptionQueryState(
      options,
      model,
      true,
      true,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      true);
}

void scenarioFederateMomObjectAttributeFullUnsubscribeQuery(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateMomSubscriptionQueryState(
      options,
      model,
      true,
      true,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      true);
}

void scenarioFederateMomObjectClassFullUnsubscribeQuery(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateMomSubscriptionQueryState(
      options,
      model,
      true,
      true,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      true);
}

void scenarioFederateMomObjectClassCompleteAttributeSubscriptionQuery(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateMomSubscriptionQueryState(
      options,
      model,
      true,
      true,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      true);
}

void scenarioFederateMomObjectClassPassiveCompleteAttributeSubscriptionQuery(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateMomSubscriptionQueryState(
      options,
      model,
      true,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      true);
}

void scenarioFederateMomObjectClassCompleteAttributeSubscriptionActivationQuery(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateMomSubscriptionQueryState(
      options,
      model,
      true,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      false,
      true,
      true);
}

void scenarioFederateMomEmptySubscriptionQuery(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateMomSubscriptionQueryState(
      options, model, false, false, false, false, false, false);
}

void scenarioFederateMomDirectedSubscriptionQuery(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateMomSubscriptionQueryState(
      options, model, false, false, false, false, true, false);
}

void scenarioFederateMomMultipleDirectedSubscriptionQuery(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateMomSubscriptionQueryState(
      options, model, false, false, false, false, true, true);
}

void scenarioFederateMomMultipleDirectedSubscriptionFullUnsubscribeQuery(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateMomSubscriptionQueryState(
      options, model, false, false, false, false, true, true, true);
}

void scenarioFederateMomSubscriptionQueryContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateMomSubscriptionQuery(options, model);
}

void scenarioFederateMomPassiveSubscriptionQueryContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateMomPassiveSubscriptionQuery(options, model);
}

void scenarioFederateMomPassiveInteractionSubscriptionQueryContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateMomPassiveInteractionSubscriptionQuery(options, model);
}

void scenarioFederateMomPassiveInteractionSubscriptionActivationQueryContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateMomPassiveInteractionSubscriptionActivationQuery(
      options,
      model);
}

void scenarioFederateMomMultipleInteractionSubscriptionQueryContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateMomMultipleInteractionSubscriptionQuery(options, model);
}

void scenarioFederateMomMultipleInteractionSubscriptionUnsubscribeQueryContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateMomMultipleInteractionSubscriptionUnsubscribeQuery(
      options,
      model);
}

void scenarioFederateMomMultipleInteractionSubscriptionFullUnsubscribeQueryContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateMomMultipleInteractionSubscriptionFullUnsubscribeQuery(
      options,
      model);
}

void scenarioFederateMomObjectAttributePartialUnsubscribeQueryContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateMomObjectAttributePartialUnsubscribeQuery(options, model);
}

void scenarioFederateMomObjectAttributeFullUnsubscribeQueryContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateMomObjectAttributeFullUnsubscribeQuery(options, model);
}

void scenarioFederateMomObjectClassFullUnsubscribeQueryContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateMomObjectClassFullUnsubscribeQuery(options, model);
}

void scenarioFederateMomObjectClassCompleteAttributeSubscriptionQueryContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateMomObjectClassCompleteAttributeSubscriptionQuery(
      options,
      model);
}

void scenarioFederateMomObjectClassPassiveCompleteAttributeSubscriptionQueryContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateMomObjectClassPassiveCompleteAttributeSubscriptionQuery(
      options,
      model);
}

void scenarioFederateMomObjectClassCompleteAttributeSubscriptionActivationQueryContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateMomObjectClassCompleteAttributeSubscriptionActivationQuery(
      options,
      model);
}

void scenarioFederateMomEmptySubscriptionQueryContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateMomEmptySubscriptionQuery(options, model);
}

void scenarioFederateMomDirectedSubscriptionQueryContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateMomDirectedSubscriptionQuery(options, model);
}

void scenarioFederateMomMultipleDirectedSubscriptionQueryContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateMomMultipleDirectedSubscriptionQuery(options, model);
}

void scenarioFederateMomMultipleDirectedSubscriptionFullUnsubscribeQueryContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateMomMultipleDirectedSubscriptionFullUnsubscribeQuery(
      options,
      model);
}

}  // namespace hla_rti_cpp_tck::detail
