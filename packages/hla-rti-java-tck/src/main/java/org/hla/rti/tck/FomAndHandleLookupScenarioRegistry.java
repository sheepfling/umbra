package org.hla.rti.tck;

/** Owns the FOM, handle-lookup, and FOM-dependent lifecycle scenario registrations. */
final class FomAndHandleLookupScenarioRegistry {
   private FomAndHandleLookupScenarioRegistry() { }

   static void register() {
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.handle-parameter-value-round-trip",
            "preserve and reconstitute an object-instance handle through interaction and attribute value maps",
            "handle-encoding-decoding",
            "RTIambassador.connect;RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getAttributeHandle;RTIambassador.getInteractionClassHandle;RTIambassador.getParameterHandle;RTIambassador.getAttributeHandleSetFactory;AttributeHandleSetFactory.create;AttributeHandleSet.add;RTIambassador.publishObjectClassAttributes;RTIambassador.subscribeObjectClassAttributes;RTIambassador.publishInteractionClass;RTIambassador.subscribeInteractionClass;RTIambassador.registerObjectInstance;RTIambassador.getObjectInstanceName;ObjectInstanceHandle.encodedLength;ObjectInstanceHandle.encode;ObjectInstanceHandle.equals;ObjectClassHandle.equals;InteractionClassHandle.equals;FederateHandle.equals;RTIambassador.getParameterHandleValueMapFactory;ParameterHandleValueMapFactory.create;ParameterHandleValueMap.put;RTIambassador.getAttributeHandleValueMapFactory;AttributeHandleValueMapFactory.create;AttributeHandleValueMap.put;RTIambassador.disableCallbacks;RTIambassador.sendInteraction;RTIambassador.updateAttributeValues;RTIambassador.enableCallbacks;RTIambassador.getObjectInstanceHandleFactory;ObjectInstanceHandleFactory.decode;RTIambassador.getObjectInstanceHandle;RTIambassador.getObjectInstanceName;RTIambassador.getFederateHandleFactory;FederateHandleFactory.decode;RTIambassador.getObjectClassHandleFactory;ObjectClassHandleFactory.decode;RTIambassador.getInteractionClassHandleFactory;InteractionClassHandleFactory.decode;RTIambassador.getAttributeHandleFactory;AttributeHandleFactory.decode;RTIambassador.getParameterHandleFactory;ParameterHandleFactory.decode;RTIambassador.getDimensionHandleFactory;DimensionHandleFactory.decode;RTIambassador.getMessageRetractionHandleFactory;MessageRetractionHandleFactory.decode;RTIambassador.getRegionHandleFactory;RegionHandleFactory.decode;RTIambassador.unsubscribeObjectClassAttributes;RTIambassador.unsubscribeInteractionClass;RTIambassador.unpublishObjectClassAttributes;RTIambassador.unpublishInteractionClass;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateAmbassador.discoverObjectInstance;FederateAmbassador.receiveInteraction;FederateAmbassador.reflectAttributeValues",
            new String[] {
               "requirement-candidate-content-clauses-11-management-object-model-page-353-l120-39",
               "requirement-candidate-content-clauses-11-management-object-model-page-353-l123-40",
               "requirement-candidate-content-clauses-11-management-object-model-page-353-l132-43",
               "requirement-candidate-content-clauses-11-management-object-model-page-377-l210-68",
               "requirement-candidate-content-clauses-12-programming-language-mappings-page-396-l186-61"
            }, true), RtiTckMain::handleParameterValueRoundTrip);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.support-lookups-valid-inputs-after-resignation",
            "check class, attribute, and parameter lookups reject valid FOM inputs after resignation",
            "handle-lookup",
            "RTIambassador.connect;RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getObjectClassName;RTIambassador.getAttributeHandle;RTIambassador.getAttributeName;RTIambassador.getInteractionClassHandle;RTIambassador.getInteractionClassName;RTIambassador.getParameterHandle;RTIambassador.getParameterName;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {
               "cpp-1516.1-object-class-handle-membership-boundary-after-resignation",
               "cpp-1516.1-object-class-name-membership-boundary-after-resignation",
               "cpp-1516.1-attribute-handle-membership-boundary-after-resignation",
               "cpp-1516.1-attribute-name-membership-boundary-after-resignation",
               "cpp-1516.1-interaction-class-handle-membership-boundary-after-resignation",
               "cpp-1516.1-interaction-class-name-membership-boundary-after-resignation",
               "cpp-1516.1-parameter-handle-membership-boundary-after-resignation",
               "cpp-1516.1-parameter-name-membership-boundary-after-resignation"
            }, true), RtiTckMain::supportLookupsAfterResignation);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.dimension-lookup-valid-handles-after-resignation",
            "check dimension lookup membership boundaries after resignation with valid FOM handles",
            "handle-lookup",
            "RTIambassador.connect;RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getObjectClassName;RTIambassador.getInteractionClassHandle;RTIambassador.getInteractionClassName;RTIambassador.getDimensionHandle;RTIambassador.getDimensionName;RTIambassador.getDimensionUpperBound;RTIambassador.getAvailableDimensionsForObjectClass;RTIambassador.getAvailableDimensionsForInteractionClass;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {
               "cpp-1516.1-object-class-dimension-lookup-membership-boundary-after-resignation",
               "cpp-1516.1-interaction-class-dimension-lookup-membership-boundary-after-resignation",
               "cpp-1516.1-dimension-name-membership-boundary-after-resignation",
               "cpp-1516.1-dimension-upper-bound-membership-boundary-after-resignation"
            }, true), RtiTckMain::dimensionLookupAfterResignation);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-model",
            "Load a rich adapter FOM and verify hierarchy, metadata, and typed delivery",
            "fom-model",
            "RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getObjectClassName;RTIambassador.getAttributeHandle;RTIambassador.getAttributeName;RTIambassador.getInteractionClassHandle;RTIambassador.getInteractionClassName;RTIambassador.getParameterHandle;RTIambassador.getParameterName;RTIambassador.getDimensionHandle;RTIambassador.getDimensionName;RTIambassador.getDimensionUpperBound;RTIambassador.getAvailableDimensionsForObjectClass;RTIambassador.getAvailableDimensionsForInteractionClass;RTIambassador.getUpdateRateValue;RTIambassador.getTransportationTypeHandle;RTIambassador.getTransportationTypeName;RTIambassador.getAutoProvideSwitch;RTIambassador.getObjectClassRelevanceAdvisorySwitch;RTIambassador.getAttributeRelevanceAdvisorySwitch;RTIambassador.getInteractionRelevanceAdvisorySwitch;RTIambassador.publishObjectClassAttributes;RTIambassador.subscribeObjectClassAttributes;RTIambassador.registerObjectInstance;RTIambassador.updateAttributeValues;RTIambassador.publishInteractionClass;RTIambassador.subscribeInteractionClass;RTIambassador.sendInteraction;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateAmbassador.discoverObjectInstance;FederateAmbassador.reflectAttributeValues;FederateAmbassador.receiveInteraction;EncoderFactory.createHLAinteger32BE;EncoderFactory.createHLAASCIIstring;EncoderFactory.createHLAfloat64BE;EncoderFactory.createHLAunicodeString;EncoderFactory.createHLAfixedArray;EncoderFactory.createHLAvariableArray;EncoderFactory.createHLAfixedRecord;EncoderFactory.createHLAvariantRecord;EncoderFactory.createHLAopaqueData",
            new String[] {"cpp-1516.2-fom-root-hierarchy",
               "cpp-1516.2-fom-data-representations",
               "cpp-1516.2-fom-dimension-and-metadata-lookups"}, false),
         RtiTckMain::fomModel);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-qualified-class-name-identity",
            "Resolve repeated local object-class names by fully qualified superclass paths",
            "fom-model",
            "RTIambassador.connect;RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getObjectClassName;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"requirement-candidate-sections-semantic-clause-4a-page-040-l22-4"}, false),
         RtiTckMain::fomQualifiedClassNameIdentity);
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.fom-interaction-qualified-class-name-identity",
            "Resolve repeated local interaction-class names by fully qualified superclass paths",
            "fom-model",
            "RTIambassador.connect;RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getInteractionClassHandle;RTIambassador.getInteractionClassName;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {
               "requirement-candidate-sections-semantic-clause-4a-page-044-l74-5",
               "requirement-candidate-sections-semantic-clauses-5-6-page-088-l70-8",
               "requirement-candidate-sections-semantic-clauses-5-6-page-092-l14-2"
            }, false), RtiTckMain::fomInteractionQualifiedClassNameIdentity);
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.fom-duplicate-object-class-attribute-name",
            "Reject a FOM that repeats an object-class attribute name",
            "fom-model",
            "RTIambassador.connect;RTIambassador.createFederationExecution(single invalid FOM module);RTIambassador.disconnect",
            new String[] {"requirement-candidate-sections-semantic-clause-4b-page-048-l101-3"},
            false), RtiTckMain::fomDuplicateObjectClassAttributeName);
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.fom-basic-data-representation-endian-values",
            "Accept Big and Little Endian values for basic data representations",
            "fom-lifecycle",
            "RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {
               "requirement-candidate-sections-semantic-clause-4c-page-070-l86-1",
               "requirement-candidate-sections-semantic-clauses-5-6-page-094-l38-7"
            }, false), FomAndHandleLookupScenarioRegistry::fomBasicDataRepresentationEndianValues);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-transportation-handle-stability",
            "Preserve transportation handle identity across an additional-FOM join",
            "fom-composition",
            "RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getTransportationTypeHandle;RTIambassador.getTransportationTypeName;RTIambassador.joinFederationExecution(additionalFomModules);RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.1-fom-module-composition",
               "cpp-1516.1-transportation-type-control",
               "cpp-1516.1-federation-create-join-resign-destroy",
               "cpp-1516.2-fom-composition-and-reference-resolution"}, false),
         RtiTckMain::fomTransportationHandleStability);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-module-composition",
            "Compose adapter FOM modules at Create and Join and deliver a directed extension interaction",
            "fom-composition",
            "RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution(fomModules as file URLs);RTIambassador.joinFederationExecution;RTIambassador.joinFederationExecution(additionalFomModules as file URLs);RTIambassador.getObjectClassHandle;RTIambassador.getObjectClassName;RTIambassador.getAttributeHandle;RTIambassador.getAttributeName;RTIambassador.getInteractionClassHandle;RTIambassador.getInteractionClassName;RTIambassador.getParameterHandle;RTIambassador.getParameterName;RTIambassador.getParameterHandleValueMapFactory;RTIambassador.getInteractionClassHandleSetFactory;RTIambassador.publishObjectClassAttributes;RTIambassador.subscribeObjectClassAttributes;RTIambassador.publishObjectClassDirectedInteractions;RTIambassador.subscribeObjectClassDirectedInteractions;RTIambassador.publishInteractionClass;RTIambassador.subscribeInteractionClass;RTIambassador.registerObjectInstance;RTIambassador.updateAttributeValues;RTIambassador.sendInteraction;RTIambassador.sendDirectedInteraction;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateAmbassador.discoverObjectInstance;FederateAmbassador.reflectAttributeValues;FederateAmbassador.receiveInteraction;FederateAmbassador.receiveDirectedInteraction",
            new String[] {"cpp-1516.1-fom-module-composition",
               "cpp-1516.2-fom-composition-and-reference-resolution",
               "requirement-candidate-content-clauses-04-federation-management-page-047-l17-4",
               "requirement-candidate-content-clauses-04-federation-management-page-047-l17-4-interaction-structure",
               "requirement-candidate-content-clauses-04-federation-management-page-047-l17-4-directed-interaction",
               "requirement-candidate-content-clauses-04-federation-management-page-047-l17-4-attributes",
               "requirement-candidate-content-clauses-04-federation-management-page-047-l17-4-parameters",
               "requirement-candidate-content-clauses-04-federation-management-page-048-l21-6",
               "requirement-candidate-content-clauses-04-federation-management-page-048-l33-10",
               "requirement-candidate-content-clauses-04-federation-management-page-048-l48-15",
               "requirement-candidate-content-clauses-04-federation-management-page-048-l150-49",
               "requirement-candidate-content-clauses-12-programming-language-mappings-page-396-l117-38"}, false),
         RtiTckMain::fomModuleComposition);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-incomplete-class-definition",
            "Reject unfinished object and interaction class scaffolds at Create and Join",
            "fom-composition",
            "RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution(fomModules);RTIambassador.joinFederationExecution(additionalFomModules);RTIambassador.getObjectClassHandle;RTIambassador.getObjectClassName;RTIambassador.getInteractionClassHandle;RTIambassador.getInteractionClassName;RTIambassador.getFederateHandle;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.1-fom-module-composition",
               "cpp-1516.2-fom-composition-and-reference-resolution"}, true),
         RtiTckMain::fomIncompleteClassDefinition);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-additional-module-join-atomicity",
            "Enforce additional FOM module consistency and atomicity during Join Federation Execution",
            "fom-composition",
            "RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.joinFederationExecution(additionalFomModules);RTIambassador.getObjectClassHandle;RTIambassador.getObjectClassName;RTIambassador.getAttributeHandle;RTIambassador.getDimensionHandle;RTIambassador.getDimensionUpperBound;RTIambassador.getFederateHandle;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {
               "requirement-candidate-content-clauses-04-federation-management-page-048-l24-7",
               "requirement-candidate-content-clauses-04-federation-management-page-048-l54-17",
               "requirement-candidate-content-clauses-04-federation-management-page-057-l44-6"
            }, false),
         RtiTckMain::fomAdditionalModuleJoinAtomicity);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-invalid-composite-join-atomicity",
            "Reject mixed valid and invalid additional FOM modules without partial composition",
            "fom-composition",
            "RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.joinFederationExecution(additionalFomModules);RTIambassador.getObjectClassHandle;RTIambassador.getFederateHandle;RTIambassador.getObjectClassName;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.1-malformed-fom-input",
               "cpp-1516.1-fom-module-composition",
               "cpp-1516.1-federation-create-join-resign-destroy",
               "cpp-1516.2-fom-composition-and-reference-resolution"}, false),
         RtiTckMain::fomInvalidCompositeJoinAtomicity);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-update-rate-value-stability",
            "Preserve existing update-rate values and compose additional rates across a module join",
            "fom-composition",
            "RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getUpdateRateValue;RTIambassador.joinFederationExecution(additionalFomModules);RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"requirement-candidate-content-clauses-04-federation-management-page-047-l17-4-update-rates"},
            false),
         RtiTckMain::fomUpdateRateValueStability);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-invalid-create-atomicity",
            "Reject invalid FOM create inputs without reserving the federation name",
            "fom-lifecycle",
            "RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getFederateHandle;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.1-malformed-fom-input",
               "cpp-1516.1-federation-create-join-resign-destroy"}, false),
         RtiTckMain::fomInvalidCreateAtomicity);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-noncompliant-module",
            "Reject a schema-valid FOM module that violates the zero-update-rate constraint",
            "fom-lifecycle",
            "RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution(fomModules);RTIambassador.joinFederationExecution;RTIambassador.joinFederationExecution(additionalFomModules);RTIambassador.getObjectClassHandle;RTIambassador.getFederateHandle;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.1-fom-module-composition",
               "cpp-1516.1-federation-create-join-resign-destroy"}, true),
         RtiTckMain::fomNoncompliantModule);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-invalid-mim-create-atomicity",
            "Reject invalid MIM create inputs without reserving the federation name",
            "fom-lifecycle",
            "RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecutionWithMIM;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getFederateHandle;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.1-malformed-fom-input",
               "cpp-1516.1-explicit-mim-composition",
               "cpp-1516.1-federation-create-join-resign-destroy"}, false),
         RtiTckMain::fomInvalidMimCreateAtomicity);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-invalid-composite-mim-create-atomicity",
            "Reject a valid-plus-invalid FOM module set during MIM-backed federation creation",
            "fom-composition",
            "RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecutionWithMIM;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getFederateHandle;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.1-malformed-fom-input",
               "cpp-1516.1-explicit-mim-composition",
               "cpp-1516.1-fom-module-composition",
               "cpp-1516.1-federation-create-join-resign-destroy"}, false),
         RtiTckMain::fomInvalidCompositeMimCreateAtomicity);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-empty-mim-create-atomicity",
            "Reject an empty FOM module array without reserving the federation name during MIM-backed creation",
            "fom-lifecycle",
            "RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecutionWithMIM(empty FOM module array);RTIambassador.createFederationExecutionWithMIM(valid FOM module);RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getFederateHandle;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.1-malformed-fom-input",
               "cpp-1516.1-explicit-mim-composition",
               "cpp-1516.1-federation-create-join-resign-destroy"}, false),
         RtiTckMain::fomEmptyMimCreateAtomicity);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-empty-module-validation",
            "Reject an empty FOM module list without reserving a federation execution",
            "fom-lifecycle",
            "RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution(empty FOM module array);RTIambassador.createFederationExecution(valid FOM module);RTIambassador.joinFederationExecution;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.1-malformed-fom-input",
               "cpp-1516.1-federation-create-join-resign-destroy"}, false),
         RtiTckMain::fomEmptyModuleValidation);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-empty-domain-tables-accepted",
            "Create and join a FOM with only root classes and empty domain tables",
            "fom-model-validity",
            "RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution(valid FOM module);RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getInteractionClassHandle;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {
               "requirement-candidate-sections-semantic-clause-4a-page-034-l6-1",
               "requirement-candidate-sections-semantic-clause-4a-page-034-l14-3"
            }, false), RtiTckMain::fomEmptyDomainTablesAccepted);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-data-type-forward-reference",
            "Resolve an array element type supplied by a later FOM module",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution(fomModules);RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.2-fom-composition-and-reference-resolution"}, true),
         RtiTckMain::fomDataTypeForwardReference);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-data-type-conflicting-definition",
            "Reject incompatible definitions of one named data type across FOM modules",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution(fomModules);RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.2-fom-composition-and-reference-resolution"}, true),
         RtiTckMain::fomDataTypeConflictingDefinition);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-variant-alternative-overlap",
            "Accept disjoint and reject overlapping named extendable-variant alternatives",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution(fomModules);RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.2-fom-composition-and-reference-resolution"}, true),
         RtiTckMain::fomVariantAlternativeOverlap);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-interaction-parameter-merge-conflict",
            "Merge disjoint interaction parameters and reject a conflicting data type",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution(fomModules);RTIambassador.joinFederationExecution;RTIambassador.getInteractionClassHandle;RTIambassador.getParameterHandle;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.2-fom-composition-and-reference-resolution"}, true),
         RtiTckMain::fomInteractionParameterMergeConflict);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-object-attribute-merge-conflict",
            "Merge disjoint object attributes and reject conflicting semantics",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution(fomModules);RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getAttributeHandle;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.2-fom-composition-and-reference-resolution"}, true),
         RtiTckMain::fomObjectAttributeMergeConflict);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-object-class-semantics-conflict",
            "Reject conflicting semantics for one named object class and allow a valid retry",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution(fomModules);RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.2-fom-composition-and-reference-resolution"}, true),
         RtiTckMain::fomObjectClassSemanticsConflict);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-matching-data-types",
            "Accept matching named declarations in eight data-type categories across FOM modules",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution(fomModules);RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.2-fom-composition-and-reference-resolution"}, true),
         RtiTckMain::fomMatchingDataTypes);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-object-class-dimension-merge",
            "Merge disjoint available dimensions on one named object class",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution(fomModules);RTIambassador.joinFederationExecution;RTIambassador.getDimensionHandle;RTIambassador.getObjectClassHandle;RTIambassador.getAvailableDimensionsForObjectClass;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.2-fom-composition-and-reference-resolution"}, true),
         RtiTckMain::fomObjectClassDimensionMerge);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-interaction-class-dimension-merge",
            "Merge disjoint available dimensions on one named interaction class",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution(fomModules);RTIambassador.joinFederationExecution;RTIambassador.getDimensionHandle;RTIambassador.getInteractionClassHandle;RTIambassador.getAvailableDimensionsForInteractionClass;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.2-fom-composition-and-reference-resolution"}, true),
         RtiTckMain::fomInteractionClassDimensionMerge);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-named-variant-alternative-merge",
            "Accept disjoint discriminant enumerators on one named extendable-variant alternative",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution(fomModules);RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.2-fom-composition-and-reference-resolution"}, true),
         RtiTckMain::fomNamedVariantAlternativeMerge);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-extendable-variant-hlaother",
            "Reject an HLAother alternative in an HLAextendableVariantRecord FOM",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution(fomModules);RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.2-fom-composition-and-reference-resolution"}, true),
         RtiTckMain::fomExtendableVariantHlaOther);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-enumerated-value-collision",
            "Reject one enumerated value assigned to different names across FOM modules",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution(fomModules);RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.2-fom-composition-and-reference-resolution"}, true),
         RtiTckMain::fomEnumeratedValueCollision);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-nonextendable-variant-alternative-conflict",
            "Reject different alternative sets on one named HLAvariantRecord",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution(fomModules);RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.2-fom-composition-and-reference-resolution"}, true),
         RtiTckMain::fomNonextendableVariantAlternativeConflict);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-array-cardinality-forms",
            "Accept standard array cardinality forms with compatible predefined encodings",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.2-fom-composition-and-reference-resolution"}, true),
         RtiTckMain::fomArrayCardinalityForms);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-array-cardinality-rejections",
            "Reject reversed array ranges and incompatible predefined encodings",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.2-fom-composition-and-reference-resolution"}, true),
         RtiTckMain::fomArrayCardinalityRejections);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-time-representation-valid",
            "Accept a permitted logical-time representation data type",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.2-fom-composition-and-reference-resolution"}, true),
         RtiTckMain::fomTimeRepresentationValid);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-default-logical-time-selection",
            "Select HLAfloat64Time by default and reject conflicting FDD time documentation",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getTimeFactory;LogicalTimeFactory.getName;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.1-federation-create-join-resign-destroy"}, true),
         RtiTckMain::fomDefaultLogicalTimeSelection);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-unavailable-logical-time-factory",
            "Reject an adapter-declared unavailable logical-time factory and release the federation name",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.1-federation-create-join-resign-destroy"}, true),
         RtiTckMain::fomUnavailableLogicalTimeFactory);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-time-representation-rejections",
            "Reject basic and reference data types as logical-time representations",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.2-fom-composition-and-reference-resolution"}, true),
         RtiTckMain::fomTimeRepresentationRejections);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-member-data-type-categories",
            "Reject raw basic data types on object attributes and interaction parameters",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.2-fom-composition-and-reference-resolution"}, true),
         RtiTckMain::fomMemberDataTypeCategories);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-attribute-na-companion-completion",
            "Accept explicit and cross-module completed NA attribute companions",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.2-fom-composition-and-reference-resolution"}, true),
         RtiTckMain::fomAttributeNaCompanionCompletion);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-attribute-na-companion-rejections",
            "Reject incompatible update and transportation companions for NA attributes",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.2-fom-composition-and-reference-resolution"}, true),
         RtiTckMain::fomAttributeNaCompanionRejections);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-dynamic-update-condition-valid",
            "Accept supplied dynamic update conditions and an omitted partial condition",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.2-fom-composition-and-reference-resolution"}, true),
         RtiTckMain::fomDynamicUpdateConditionValid);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-dynamic-update-condition-rejections",
            "Reject NA or empty supplied conditions for dynamic attributes",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.2-fom-composition-and-reference-resolution"}, true),
         RtiTckMain::fomDynamicUpdateConditionRejections);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-attribute-value-required-sharing",
            "Reject value-required attributes when sharing is Neither",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.2-fom-composition-and-reference-resolution"}, true),
         RtiTckMain::fomAttributeValueRequiredSharing);
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.fom-attribute-table-nonblank-semantics",
            "Reject an attribute table row with blank Semantics",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution(fomModules);RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {
               "requirement-candidate-sections-semantic-clauses-5-6-page-092-l90-19"
            }, true), FomAndHandleLookupScenarioRegistry::fomAttributeTableNonblankSemantics);
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.fom-reference-data-type-name-convention",
            "Reject a reference data type name that violates HLA name conventions",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution(valid control FOM);RTIambassador.destroyFederationExecution;RTIambassador.createFederationExecution(invalid FOM);RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {
               "requirement-candidate-sections-semantic-clauses-5-6-page-094-l86-16"
            }, true), FomAndHandleLookupScenarioRegistry::fomReferenceDataTypeNameConvention);
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.fom-simple-data-type-name-convention",
            "Reject simple data type names that violate HLA name conventions",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution(valid control FOM);RTIambassador.destroyFederationExecution;RTIambassador.createFederationExecution(each invalid FOM);RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {
               "requirement-candidate-sections-semantic-clause-4c-page-071-l61-2",
               "requirement-candidate-sections-semantic-clauses-5-6-page-094-l50-9"
            }, true), FomAndHandleLookupScenarioRegistry::fomSimpleDataTypeNameConvention);
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.fom-model-identification-name-valid",
            "Accept a conforming Model Identification Name through public Create",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution(adapter FOM);RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {
               "requirement-candidate-sections-semantic-clauses-5-6-page-091-l54-7"
            }, false), FomAndHandleLookupScenarioRegistry::fomModelIdentificationNameValid);
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.fom-enumerated-data-type-name-convention",
            "Reject an enumerated data type name that violates HLA name conventions",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution(valid control FOM);RTIambassador.destroyFederationExecution;RTIambassador.createFederationExecution(invalid FOM);RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {
               "requirement-candidate-sections-semantic-clauses-5-6-page-094-l66-12",
               "requirement-candidate-sections-semantic-clause-4c-page-072-l124-2"
            }, true), FomAndHandleLookupScenarioRegistry::fomEnumeratedDataTypeNameConvention);
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.fom-enumerator-name-convention",
            "Reject an enumerator name that violates HLA name conventions",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution(valid control FOM);RTIambassador.destroyFederationExecution;RTIambassador.createFederationExecution(invalid FOM);RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {
               "requirement-candidate-sections-semantic-clauses-5-6-page-094-l74-14",
               "requirement-candidate-sections-semantic-clause-4c-page-073-l91-1"
            }, true), FomAndHandleLookupScenarioRegistry::fomEnumeratorNameConvention);
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.fom-array-data-type-name-convention",
            "Reject an array data type name that violates HLA name conventions",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution(valid control FOM);RTIambassador.destroyFederationExecution;RTIambassador.createFederationExecution(invalid FOM);RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {
               "requirement-candidate-sections-semantic-clauses-5-6-page-094-l114-22"
            }, false), FomAndHandleLookupScenarioRegistry::fomArrayDataTypeNameConvention);
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.fom-array-element-type-reference",
            "Reject an array data type with an unresolved Element Type",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution(valid HLAbyte control FOM);RTIambassador.destroyFederationExecution;RTIambassador.createFederationExecution(unresolved Element Type FOM);RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {
               "requirement-candidate-sections-semantic-clauses-5-6-page-094-l118-23"
            }, false), FomAndHandleLookupScenarioRegistry::fomArrayElementTypeReference);
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.fom-variant-record-name-convention",
            "Reject a Variant Record Name that violates HLA name conventions",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution(valid control FOM);RTIambassador.destroyFederationExecution;RTIambassador.createFederationExecution(invalid FOM);RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {
               "requirement-candidate-sections-semantic-clauses-5-6-page-095-l46-9",
               "requirement-candidate-sections-semantic-clause-4c-page-077-l58-2"
            }, false), FomAndHandleLookupScenarioRegistry::fomVariantRecordNameConvention);
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.fom-variant-discriminant-name-convention",
            "Reject a Variant Record Discriminant Name that violates HLA name conventions",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution(valid control FOM);RTIambassador.destroyFederationExecution;RTIambassador.createFederationExecution(invalid FOM);RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {
               "requirement-candidate-sections-semantic-clauses-5-6-page-095-l50-10",
               "requirement-candidate-sections-semantic-clause-4c-page-077-l62-3"
            }, false), FomAndHandleLookupScenarioRegistry::fomVariantDiscriminantNameConvention);
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.fom-variant-alternative-name-convention",
            "Reject a Variant Record Alternative Name that violates HLA name conventions",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution(valid control FOM);RTIambassador.destroyFederationExecution;RTIambassador.createFederationExecution(invalid FOM);RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {
               "requirement-candidate-sections-semantic-clauses-5-6-page-095-l70-15",
               "requirement-candidate-sections-semantic-clause-4c-page-077-l74-6"
            }, false), FomAndHandleLookupScenarioRegistry::fomVariantAlternativeNameConvention);
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.fom-variant-alternative-type-category",
            "Reject a basic data type used for a Variant Record Alternative Type",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution(enumerated HLAboolean control FOM);RTIambassador.destroyFederationExecution;RTIambassador.createFederationExecution(basic HLAinteger32BE alternative FOM);RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {
               "requirement-candidate-sections-semantic-clauses-5-6-page-095-l74-16"
            }, false), FomAndHandleLookupScenarioRegistry::fomVariantAlternativeTypeCategory);
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.fom-variant-record-encoding",
            "Accept predefined HLAvariantRecord and HLAextendableVariantRecord Encoding markers",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution(valid marker FOM);RTIambassador.destroyFederationExecution;RTIambassador.createFederationExecution(mismatched HLAfixedRecord FOM);RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {
               "requirement-candidate-sections-semantic-clauses-5-6-page-095-l78-17",
               "requirement-candidate-sections-semantic-clause-4c-page-078-l56-2"
            }, false), FomAndHandleLookupScenarioRegistry::fomVariantRecordEncoding);
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.fom-synchronization-table-validation",
            "Validate FOM synchronization table label and capability fields",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution(valid FOM);RTIambassador.destroyFederationExecution;RTIambassador.createFederationExecution(each invalid FOM);RTIambassador.createFederationExecution(valid retry);RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {
               "requirement-candidate-sections-semantic-clause-4b-page-061-l90-5",
               "requirement-candidate-sections-semantic-clauses-5-6-page-093-l66-12",
               "requirement-candidate-sections-semantic-clauses-5-6-page-093-l70-13",
               "requirement-candidate-sections-semantic-clauses-5-6-page-093-l78-15"
            }, false), FomAndHandleLookupScenarioRegistry::fomSynchronizationTableValidation);
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.fom-transportation-description-ignored",
            "Ignore transportation descriptions when combining FOM modules",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution(modules with same transportation name/reliability and different Semantics);RTIambassador.joinFederationExecution;RTIambassador.getTransportationTypeHandle;RTIambassador.getTransportationTypeName;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.joinFederationExecution(additionalFomModules);RTIambassador.getTransportationTypeHandle;RTIambassador.getTransportationTypeName",
            new String[] {
               "requirement-candidate-content-clauses-04-federation-management-page-048-l42-13"
            }, false), FomAndHandleLookupScenarioRegistry::fomTransportationDescriptionIgnored);
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.fom-automatic-resign-action-setting",
            "Accept the six defined Automatic Resign Action FOM settings and reject an unsupported value",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecutionWithMIM;RTIambassador.joinFederationExecution;RTIambassador.getAutomaticResignDirective;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {
               "requirement-candidate-sections-semantic-clauses-5-6-page-094-l10-1",
               "requirement-candidate-sections-semantic-clauses-5-6-page-094-l14-2",
               "requirement-candidate-sections-semantic-clause-4b-page-067-l30-2"
            }, true), FomAndHandleLookupScenarioRegistry::fomAutomaticResignActionSetting);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-root-hierarchy",
            "Require standard object and interaction roots",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getInteractionClassHandle;RTIambassador.getObjectClassName;RTIambassador.getInteractionClassName;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.2-fom-root-hierarchy"}, true),
         RtiTckMain::fomRootHierarchy);
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.fom-interaction-class-hierarchy",
            "Resolve an interaction-class superclass hierarchy through public handles",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getInteractionClassHandle;RTIambassador.getInteractionClassName;RTIambassador.getParameterHandle;RTIambassador.getParameterName;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {
               "requirement-candidate-sections-semantic-clause-4a-page-043-l125-2",
               "requirement-candidate-sections-semantic-clause-4b-page-052-l54-10"
            }, false), RtiTckMain::fomInteractionClassHierarchy);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-tag-data-type-categories",
            "Validate tag data-type categories and FOM synchronization capability",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.2-fom-composition-and-reference-resolution"}, false),
         RtiTckMain::fomTagDataTypeCategories);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-inherited-member-names",
            "Preserve inherited member identity and reject inherited attribute or parameter name reuse",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getAttributeHandle;RTIambassador.getAttributeName;RTIambassador.getInteractionClassHandle;RTIambassador.getParameterHandle;RTIambassador.getParameterName;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.2-fom-composition-and-reference-resolution"}, true),
         RtiTckMain::fomInheritedMemberNames);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-equivalent-class-conflict",
            "Accept equivalent duplicate modules and reject conflicting class sharing",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getObjectClassName;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.2-fom-composition-and-reference-resolution"}, true),
         RtiTckMain::fomEquivalentClassConflict);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-reference-class-resolution",
            "Resolve reference-data classes from a later module and reject unresolved classes",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getObjectClassName;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.2-fom-composition-and-reference-resolution"}, true),
         RtiTckMain::fomReferenceClassResolution);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-enumerated-representation",
            "Accept omitted and reject non-basic enumerated representations",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.2-fom-composition-and-reference-resolution"}, true),
         RtiTckMain::fomEnumeratedRepresentation);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-array-element-category",
            "Accept named and omitted array Element Types and reject raw basic-data elements",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.2-fom-composition-and-reference-resolution"}, true),
         RtiTckMain::fomArrayElementCategory);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-record-member-category",
            "Accept named and omitted record member types and reject raw basic-data members",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.2-fom-composition-and-reference-resolution"}, true),
         RtiTckMain::fomRecordMemberCategory);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-variant-discriminant-category",
            "Accept enumerated and omitted variant discriminants and reject other categories",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.2-fom-composition-and-reference-resolution"}, true),
         RtiTckMain::fomVariantDiscriminantCategory);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-variant-enumerator-syntax",
            "Accept range, standalone HLAother, and omitted variant enumerators; reject malformed forms",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.2-fom-composition-and-reference-resolution"}, true),
         RtiTckMain::fomVariantEnumeratorSyntax);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-variant-enumerator-membership",
            "Require supplied variant discriminant members to belong to the resolved enumeration",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.2-fom-composition-and-reference-resolution"}, true),
         RtiTckMain::fomVariantEnumeratorMembership);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-variant-range-semantics",
            "Resolve merged variant ranges and reject an overlapping named alternative",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.2-fom-composition-and-reference-resolution"}, true),
         RtiTckMain::fomVariantRangeSemantics);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-reference-attribute-resolution",
            "Resolve referenced attributes and representations across FOM modules",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getAttributeHandle;RTIambassador.getAttributeName;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.2-fom-composition-and-reference-resolution"}, true),
         RtiTckMain::fomReferenceAttributeResolution);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-data-representation-resolution",
            "Accept special instance identifiers and reject invalid data representations",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.2-fom-composition-and-reference-resolution"}, true),
         RtiTckMain::fomDataRepresentationResolution);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-transportation-reference-resolution",
            "Resolve object and interaction transportation names from a later FOM module",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getTransportationTypeHandle;RTIambassador.getTransportationTypeName;RTIambassador.getObjectClassHandle;RTIambassador.getAttributeHandle;RTIambassador.getAttributeName;RTIambassador.getInteractionClassHandle;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.2-fom-composition-and-reference-resolution"}, true),
         RtiTckMain::fomTransportationReferenceResolution);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-directed-reference-resolution",
            "Resolve a later-module directed interaction and deliver it to a subscribed target",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getInteractionClassHandle;RTIambassador.getAttributeHandle;RTIambassador.publishObjectClassAttributes;RTIambassador.subscribeObjectClassAttributes;RTIambassador.publishObjectClassDirectedInteractions;RTIambassador.subscribeObjectClassDirectedInteractionsUniversally;RTIambassador.registerObjectInstance;RTIambassador.sendDirectedInteraction;FederateAmbassador.receiveDirectedInteraction;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.2-fom-composition-and-reference-resolution"}, true),
         RtiTckMain::fomDirectedReferenceResolution);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-dimension-reference-resolution",
            "Resolve object and interaction available dimensions from a later FOM module",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getDimensionHandle;RTIambassador.getDimensionUpperBound;RTIambassador.getObjectClassHandle;RTIambassador.getInteractionClassHandle;RTIambassador.getAvailableDimensionsForObjectClass;RTIambassador.getAvailableDimensionsForInteractionClass;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.2-fom-composition-and-reference-resolution"}, true),
         RtiTckMain::fomDimensionReferenceResolution);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-dimension-duplicate-union",
            "Accept an identical dimension duplicate and retain a new dimension",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getDimensionHandle;RTIambassador.getDimensionName;RTIambassador.getDimensionUpperBound;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.2-fom-composition-and-reference-resolution"}, true),
         RtiTckMain::fomDimensionDuplicateUnion);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-dimension-duplicate-conflict",
            "Reject a conflicting upper bound on a same-named dimension",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.2-fom-composition-and-reference-resolution"}, true),
         RtiTckMain::fomDimensionDuplicateConflict);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-dimension-input-type",
            "Reject raw basic dimension inputs while accepting NA and table-defined types",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecutionWithMIM;RTIambassador.joinFederationExecution;RTIambassador.getDimensionHandle;RTIambassador.getDimensionName;RTIambassador.getDimensionUpperBound;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.2-fom-composition-and-reference-resolution"}, true),
         RtiTckMain::fomDimensionInputType);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-dimension-input-description",
            "Require a meaningful description when a dimension has no named input type",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecutionWithMIM;RTIambassador.joinFederationExecution;RTIambassador.getDimensionHandle;RTIambassador.getDimensionName;RTIambassador.getDimensionUpperBound;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.2-fom-composition-and-reference-resolution"}, true),
         RtiTckMain::fomDimensionInputDescription);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-dimension-input-na-exclusivity",
            "Keep the NA dimension input marker exclusive of named types",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecutionWithMIM;RTIambassador.joinFederationExecution;RTIambassador.getDimensionHandle;RTIambassador.getDimensionName;RTIambassador.getDimensionUpperBound;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.2-fom-composition-and-reference-resolution"}, true),
         RtiTckMain::fomDimensionInputNaExclusivity);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-dimension-name-uniqueness",
            "Reject repeated dimension names within one module, preserving legal cross-module duplicates",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecutionWithMIM;RTIambassador.joinFederationExecution;RTIambassador.getDimensionHandle;RTIambassador.getDimensionName;RTIambassador.getDimensionUpperBound;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.2-fom-composition-and-reference-resolution"}, true),
         RtiTckMain::fomDimensionNameUniqueness);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-object-class-name-conventions",
            "Reject invalid object-class local names and accept permitted punctuation",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecutionWithMIM;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getObjectClassName;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.2-fom-composition-and-reference-resolution"}, true),
         RtiTckMain::fomObjectClassNameConventions);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-model-modification-date",
            "Reject a timezone-suffixed modification date and accept YYYY-MM-DD",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecutionWithMIM;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getObjectClassName;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.2-fom-composition-and-reference-resolution"}, true),
         RtiTckMain::fomModelModificationDate);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-positive-update-rate",
            "Reject zero update rates and expose a matched positive rate",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecutionWithMIM;RTIambassador.joinFederationExecution;RTIambassador.getUpdateRateValue;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.2-fom-composition-and-reference-resolution"}, true),
         RtiTckMain::fomPositiveUpdateRate);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-dimension-default-range",
            "Reject an out-of-bounds dimension default range and accept its exact boundary",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecutionWithMIM;RTIambassador.joinFederationExecution;RTIambassador.getDimensionHandle;RTIambassador.getDimensionName;RTIambassador.getDimensionUpperBound;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.2-fom-composition-and-reference-resolution"}, true),
         RtiTckMain::fomDimensionDefaultRange);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-synchronization-duplicate-equal",
            "Accept identical same-label synchronization declarations",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution(fomModules);RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.2-fom-composition-and-reference-resolution"}, true),
         RtiTckMain::fomSynchronizationDuplicateEqual);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-synchronization-duplicate-note-conflict",
            "Reject a changed note reference on one synchronization declaration",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution(fomModules);RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.2-fom-composition-and-reference-resolution"}, true),
         RtiTckMain::fomSynchronizationDuplicateNoteConflict);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-transportation-duplicate-equal",
            "Accept identical same-name transportation declarations",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution(fomModules);RTIambassador.joinFederationExecution;RTIambassador.getTransportationTypeHandle;RTIambassador.getTransportationTypeName;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.2-fom-composition-and-reference-resolution"}, true),
         RtiTckMain::fomTransportationDuplicateEqual);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-transportation-duplicate-note-conflict",
            "Reject a changed note reference on one transportation declaration",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution(fomModules);RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.2-fom-composition-and-reference-resolution"}, true),
         RtiTckMain::fomTransportationDuplicateNoteConflict);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-update-rate-duplicate-equal",
            "Accept identical same-name update-rate declarations",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution(fomModules);RTIambassador.joinFederationExecution;RTIambassador.getUpdateRateValue;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {
               "requirement-candidate-sections-semantic-clause-7-annexes-a-c-page-112-l70-9",
               "requirement-candidate-sections-semantic-clause-7-annexes-a-c-page-112-l74-10"
            }, true),
         RtiTckMain::fomUpdateRateDuplicateEqual);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-update-rate-duplicate-note-conflict",
            "Reject conflicting same-name update-rate declarations",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution(fomModules);RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {
               "requirement-candidate-sections-semantic-clause-7-annexes-a-c-page-112-l70-9",
               "requirement-candidate-sections-semantic-clause-7-annexes-a-c-page-112-l78-11",
               "req-fom-module-reject-update-rate-conflict"
            }, true),
         RtiTckMain::fomUpdateRateDuplicateNoteConflict);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-switch-duplicate-equal",
            "Accept equivalent duplicate support-switch declarations",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution(fomModules);RTIambassador.joinFederationExecution;RTIambassador.getNonRegulatedGrantSwitch;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.2-fom-composition-and-reference-resolution"}, true),
         RtiTckMain::fomSwitchDuplicateEqual);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-switch-duplicate-order",
            "Reject conflicting support-switch declarations during Create and Join",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution(fomModules);RTIambassador.joinFederationExecution;RTIambassador.joinFederationExecution(additionalFomModules);RTIambassador.getNonRegulatedGrantSwitch;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {
               "requirement-candidate-sections-semantic-clause-7-annexes-a-c-page-113-l34-5",
               "req-fom-module-reject-switch-conflict"
            }, true),
         RtiTckMain::fomSwitchDuplicateOrder);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-switch-duplicate-omitted",
            "Treat omitted switch isEnabled as Disabled in either module order",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution(fomModules);RTIambassador.joinFederationExecution;RTIambassador.getNonRegulatedGrantSwitch;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.2-fom-composition-and-reference-resolution"}, true),
         RtiTckMain::fomSwitchDuplicateOmitted);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-advisory-switch-duplicate-equal",
            "Accept equivalent duplicate known-class advisory declarations",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution(fomModules);RTIambassador.joinFederationExecution;RTIambassador.getAdvisoriesUseKnownClassSwitch;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.2-fom-composition-and-reference-resolution"}, true),
         RtiTckMain::fomAdvisorySwitchDuplicateEqual);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-advisory-switch-duplicate-order",
            "Reject conflicting known-class advisory declarations in either module order",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution(fomModules);RTIambassador.joinFederationExecution;RTIambassador.getAdvisoriesUseKnownClassSwitch;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {
               "requirement-candidate-sections-semantic-clause-7-annexes-a-c-page-113-l34-5",
               "req-fom-module-reject-switch-conflict"
            }, true),
         RtiTckMain::fomAdvisorySwitchDuplicateOrder);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-advisory-switch-duplicate-omitted",
            "Treat omitted known-class advisory isEnabled as Disabled in either module order",
            "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecution(fomModules);RTIambassador.joinFederationExecution;RTIambassador.getAdvisoriesUseKnownClassSwitch;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.2-fom-composition-and-reference-resolution"}, true),
         RtiTckMain::fomAdvisorySwitchDuplicateOmitted);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-service-usage-merge",
            "Logically OR service usage in the public composed current FDD",
            "fom-composition",
            "RTIambassador.createFederationExecutionWithMIM;RTIambassador.joinFederationExecution;RTIambassador.subscribeObjectClassAttributes;RTIambassador.requestAttributeValueUpdate;FederateAmbassador.reflectAttributeValues",
            new String[] {"cpp-1516.2-fom-composition-and-reference-resolution"}, true),
         RtiTckMain::fomServiceUsageMerge);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-note-collision-remap",
            "Preserve distinct referenced notes after FOM label collision",
            "fom-composition",
            "RTIambassador.createFederationExecutionWithMIM;RTIambassador.joinFederationExecution;RTIambassador.subscribeObjectClassAttributes;RTIambassador.requestAttributeValueUpdate;FederateAmbassador.reflectAttributeValues",
            new String[] {"cpp-1516.2-fom-composition-and-reference-resolution"}, true),
         RtiTckMain::fomNoteCollisionRemap);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-standard-invalid-input-corpus",
            "Reject adapter-supplied standard-invalid FOM files and prove each federation name remains reusable",
            "fom-lifecycle",
            "RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution(invalid FOM);RTIambassador.createFederationExecution(valid FOM);RTIambassador.joinFederationExecution;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.1-federation-create-join-resign-destroy",
               "cpp-1516.1-fom-module-composition"}, false),
         RtiTckMain::fomStandardInvalidInputCorpus);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.fom-dimension-handle-stability",
            "Preserve dimension handle identity and class associations across an additional-FOM join",
            "fom-composition",
            "RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getDimensionHandle;RTIambassador.getDimensionName;RTIambassador.getDimensionUpperBound;RTIambassador.getAvailableDimensionsForObjectClass;RTIambassador.getAvailableDimensionsForInteractionClass;RTIambassador.getObjectClassHandle;RTIambassador.getInteractionClassHandle;RTIambassador.getAttributeHandle;RTIambassador.joinFederationExecution(additionalFomModules);RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.1-fom-module-composition",
               "cpp-1516.1-fom-dimension-and-metadata-lookups",
               "cpp-1516.1-federation-create-join-resign-destroy",
               "cpp-1516.2-fom-composition-and-reference-resolution"}, false),
         RtiTckMain::fomDimensionHandleStability);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.interaction-class-lookup-lifecycle",
            "interaction-class lookup boundaries and additional-FOM composition",
            "handle-lookup",
            "RTIambassador.connect;RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.joinFederationExecution(additionalFomModules);RTIambassador.getInteractionClassHandle;RTIambassador.getInteractionClassName;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {
               "cpp-1516.1-interaction-lookup",
               "cpp-1516.1-support-lookup-negative-boundaries",
               "cpp-1516.1-fom-module-composition",
               "cpp-1516.1-federation-create-join-resign-destroy"
            }, false), RtiTckMain::interactionClassLookupLifecycle);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.object-class-lookup-lifecycle",
            "object-class lookup boundaries and additional-FOM composition",
            "handle-lookup",
            "RTIambassador.connect;RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.joinFederationExecution(additionalFomModules);RTIambassador.getObjectClassHandle;RTIambassador.getObjectClassName;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {
               "cpp-1516.1-object-class-lookup",
               "cpp-1516.1-support-lookup-negative-boundaries",
               "cpp-1516.1-fom-module-composition",
               "cpp-1516.1-federation-create-join-resign-destroy"
            }, false), RtiTckMain::objectClassLookupLifecycle);
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.known-object-class-and-instance-handle-lookup",
            "resolve class and instance handles for a discovered named object",
            "handle-lookup",
            "RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution(time implementation supplied by adapter);RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getAttributeHandle;RTIambassador.getAttributeHandleSetFactory;AttributeHandleSetFactory.create;AttributeHandleSet.add;RTIambassador.publishObjectClassAttributes;RTIambassador.subscribeObjectClassAttributes;RTIambassador.reserveObjectInstanceName;FederateAmbassador.objectInstanceNameReservationSucceeded;RTIambassador.registerObjectInstance(named);FederateAmbassador.discoverObjectInstance;RTIambassador.getKnownObjectClassHandle;RTIambassador.getObjectInstanceHandle;RTIambassador.getObjectInstanceName;RTIambassador.unsubscribeObjectClassAttributes;RTIambassador.unpublishObjectClassAttributes;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;ObjectClassHandle.equals;AttributeHandle.equals;ObjectInstanceHandle.equals",
            new String[] {
               "cpp-1516.1-object-registration-discovery",
               "cpp-1516.1-object-instance-name-lifecycle"
            }, true), () -> KnownObjectClassAndInstanceHandleLookupTck.run(
               RtiTckMain.factory(), RtiTckMain.requireFom(), RtiTckMain.timeImplementation(),
               RtiTckMain.objectClassName(), RtiTckMain.attributeName(),
               RtiTckMain.callbackModel()));
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.attribute-lookup-lifecycle",
            "attribute lookup boundaries and inherited object-class identity",
            "handle-lookup",
            "RTIambassador.connect;RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getAttributeHandle;RTIambassador.getAttributeName;ObjectClassHandle.equals;AttributeHandle.equals;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {
               "cpp-1516.1-attribute-lookup",
               "cpp-1516.1-object-class-lookup",
               "cpp-1516.1-support-lookup-negative-boundaries",
               "cpp-1516.2-fom-root-hierarchy",
               "cpp-1516.1-federation-create-join-resign-destroy",
               "requirement-candidate-content-clauses-10-support-services-page-249-l146-36",
               "requirement-candidate-content-clauses-10-support-services-page-250-l109-27"
            }, true), RtiTckMain::attributeLookupLifecycle);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.parameter-lookup-lifecycle",
            "parameter lookup boundaries and inherited interaction-class identity",
            "handle-lookup",
            "RTIambassador.connect;RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getInteractionClassHandle;RTIambassador.getParameterHandle;RTIambassador.getParameterName;InteractionClassHandle.equals;ParameterHandle.equals;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {
               "cpp-1516.1-parameter-lookup",
               "cpp-1516.1-interaction-lookup",
               "cpp-1516.1-support-lookup-negative-boundaries",
               "cpp-1516.2-fom-root-hierarchy",
               "cpp-1516.1-federation-create-join-resign-destroy",
               "requirement-candidate-content-clauses-10-support-services-page-254-l26-3",
               "requirement-candidate-content-clauses-10-support-services-page-254-l143-35"
            }, true), RtiTckMain::parameterLookupLifecycle);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.dimension-lookup-lifecycle",
            "dimension metadata lookups across connection and federation lifecycle",
            "handle-lookup",
            "RTIambassador.connect;RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getInteractionClassHandle;RTIambassador.getAvailableDimensionsForObjectClass;RTIambassador.getAvailableDimensionsForInteractionClass;RTIambassador.getDimensionHandle;RTIambassador.getDimensionName;RTIambassador.getDimensionUpperBound;DimensionHandle.equals;DimensionHandleSet.contains;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {
               "cpp-1516.1-ddm-dimension-metadata",
               "cpp-1516.1-support-lookup-negative-boundaries",
               "cpp-1516.2-fom-dimension-and-metadata-lookups",
               "cpp-1516.1-federation-create-join-resign-destroy",
               "requirement-candidate-content-clauses-10-support-services-page-261-l30-6"
            }, true), RtiTckMain::dimensionLookupLifecycle);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.handle-normalization-membership-boundaries",
            "standard handle and service-group normalization call-state boundaries",
            "handle-lookup",
            "RTIambassador.connect;RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getAttributeHandle;RTIambassador.getInteractionClassHandle;RTIambassador.publishObjectClassAttributes;RTIambassador.registerObjectInstance;RTIambassador.normalizeServiceGroup;RTIambassador.normalizeFederateHandle;RTIambassador.normalizeObjectClassHandle;RTIambassador.normalizeInteractionClassHandle;RTIambassador.normalizeObjectInstanceHandle;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {
               "cpp-1516.1-normalize-service-group-membership-boundaries",
               "cpp-1516.1-normalize-federate-handle-membership-boundaries",
               "cpp-1516.1-normalize-object-class-handle-membership-boundaries",
               "cpp-1516.1-normalize-interaction-class-handle-membership-boundaries",
               "cpp-1516.1-normalize-object-instance-handle-membership-boundaries"
            }, true), RtiTckMain::handleNormalizationMembershipBoundaries);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.update-rate-lookups-valid-inputs-after-resignation",
            "check update-rate lookups reject valid FOM inputs after resignation",
            "handle-lookup",
            "RTIambassador.connect;RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getObjectClassName;RTIambassador.getAttributeHandle;RTIambassador.getAttributeName;RTIambassador.publishObjectClassAttributes;RTIambassador.subscribeObjectClassAttributes(update-rate designator);RTIambassador.registerObjectInstance;RTIambassador.getUpdateRateValue;RTIambassador.getUpdateRateValueForAttribute;RTIambassador.resignFederationExecution(NO_ACTION);RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateAmbassador.discoverObjectInstance",
            new String[] {
               "cpp-1516.1-update-rate-value-membership-boundary-after-resignation",
               "cpp-1516.1-update-rate-value-for-attribute-membership-boundary-after-resignation"
            }, true), RtiTckMain::updateRateLookupsAfterResignation);

   }

   static void fomAttributeTableNonblankSemantics() throws Exception {
      FomAttributeTableNonblankSemanticsTck.run(RtiTckMain.factory(),
         RtiTckMain.requireAdapterFile("hla.rti.tck.attributeTableEmptySemanticsFom"),
         RtiTckMain.timeImplementation(), RtiTckMain.callbackModel());
   }

   static void fomReferenceDataTypeNameConvention() throws Exception {
      FomReferenceDataTypeNameConventionTck.run(RtiTckMain.factory(),
         RtiTckMain.requireAdapterFile("hla.rti.tck.referenceDataTypeValidNameFom"),
         RtiTckMain.requireAdapterFile("hla.rti.tck.referenceDataTypeInvalidNameFom"),
         RtiTckMain.timeImplementation(), RtiTckMain.callbackModel());
   }

   static void fomSimpleDataTypeNameConvention() throws Exception {
      FomSimpleDataTypeNameConventionTck.run(RtiTckMain.factory(),
         RtiTckMain.requireAdapterFile("hla.rti.tck.simpleDataTypeValidNameFom"),
         new String[] {
            RtiTckMain.requireAdapterFile("hla.rti.tck.simpleDataTypeInvalidNameFom"),
            RtiTckMain.requireAdapterFile("hla.rti.tck.simpleDataTypeReservedPrefixNameFom"),
            RtiTckMain.requireAdapterFile("hla.rti.tck.simpleDataTypeReservedNaNameFom")
         }, RtiTckMain.timeImplementation(), RtiTckMain.callbackModel());
   }

   static void fomModelIdentificationNameValid() throws Exception {
      FomModelIdentificationNameValidTck.run(RtiTckMain.factory(),
         RtiTckMain.requireAdapterFile("hla.rti.tck.modelIdentificationNameValidFom"),
         RtiTckMain.timeImplementation(), RtiTckMain.callbackModel());
   }

   static void fomEnumeratedDataTypeNameConvention() throws Exception {
      FomEnumeratedDataTypeNameConventionTck.run(RtiTckMain.factory(),
         RtiTckMain.requireAdapterFile("hla.rti.tck.enumeratedDataTypeValidNameFom"),
         RtiTckMain.requireAdapterFile("hla.rti.tck.enumeratedDataTypeInvalidNameFom"),
         RtiTckMain.timeImplementation(), RtiTckMain.callbackModel());
   }

   static void fomEnumeratorNameConvention() throws Exception {
      FomEnumeratorNameConventionTck.run(RtiTckMain.factory(),
         RtiTckMain.requireAdapterFile("hla.rti.tck.enumeratorValidNameFom"),
         RtiTckMain.requireAdapterFile("hla.rti.tck.enumeratorInvalidNameFom"),
         RtiTckMain.timeImplementation(), RtiTckMain.callbackModel());
   }

   static void fomArrayDataTypeNameConvention() throws Exception {
      FomArrayDataTypeNameConventionTck.run(RtiTckMain.factory(),
         RtiTckMain.requireAdapterFile("hla.rti.tck.arrayDataTypeValidNameFom"),
         RtiTckMain.requireAdapterFile("hla.rti.tck.arrayDataTypeInvalidNameFom"),
         RtiTckMain.timeImplementation(), RtiTckMain.callbackModel());
   }

   static void fomArrayElementTypeReference() throws Exception {
      FomArrayElementTypeReferenceTck.run(RtiTckMain.factory(),
         RtiTckMain.requireAdapterFile("hla.rti.tck.arrayElementTypeValidFom"),
         RtiTckMain.requireAdapterFile("hla.rti.tck.arrayElementTypeInvalidFom"),
         RtiTckMain.timeImplementation(), RtiTckMain.callbackModel());
   }

   static void fomVariantRecordNameConvention() throws Exception {
      FomVariantRecordNameConventionTck.run(RtiTckMain.factory(),
         RtiTckMain.requireAdapterFile("hla.rti.tck.variantRecordValidNameFom"),
         RtiTckMain.requireAdapterFile("hla.rti.tck.variantRecordInvalidNameFom"),
         RtiTckMain.timeImplementation(), RtiTckMain.callbackModel());
   }

   static void fomVariantDiscriminantNameConvention() throws Exception {
      FomVariantDiscriminantNameConventionTck.run(RtiTckMain.factory(),
         RtiTckMain.requireAdapterFile("hla.rti.tck.variantDiscriminantValidNameFom"),
         RtiTckMain.requireAdapterFile("hla.rti.tck.variantDiscriminantInvalidNameFom"),
         RtiTckMain.timeImplementation(), RtiTckMain.callbackModel());
   }

   static void fomVariantAlternativeNameConvention() throws Exception {
      FomVariantAlternativeNameConventionTck.run(RtiTckMain.factory(),
         RtiTckMain.requireAdapterFile("hla.rti.tck.variantAlternativeValidNameFom"),
         RtiTckMain.requireAdapterFile("hla.rti.tck.variantAlternativeInvalidNameFom"),
         RtiTckMain.timeImplementation(), RtiTckMain.callbackModel());
   }

   static void fomVariantAlternativeTypeCategory() throws Exception {
      FomVariantAlternativeTypeCategoryTck.run(RtiTckMain.factory(),
         RtiTckMain.requireAdapterFile("hla.rti.tck.variantAlternativeTypeValidFom"),
         RtiTckMain.requireAdapterFile("hla.rti.tck.variantAlternativeTypeInvalidFom"),
         RtiTckMain.timeImplementation(), RtiTckMain.callbackModel());
   }

   static void fomVariantRecordEncoding() throws Exception {
      FomVariantRecordEncodingTck.run(RtiTckMain.factory(),
         RtiTckMain.requireAdapterFile("hla.rti.tck.variantRecordEncodingValidFom"),
         RtiTckMain.requireAdapterFile("hla.rti.tck.variantRecordEncodingInvalidFom"),
         RtiTckMain.timeImplementation(), RtiTckMain.callbackModel());
   }

   static void fomSynchronizationTableValidation() throws Exception {
      FomSynchronizationTableValidationTck.run(RtiTckMain.factory(),
         RtiTckMain.requireAdapterFile("hla.rti.tck.synchronizationCapabilityNaFom"),
         RtiTckMain.requireAdapterFile("hla.rti.tck.synchronizationCapabilityActionFom"),
         RtiTckMain.requireAdapterFile("hla.rti.tck.synchronizationLabelInvalidFom"),
         RtiTckMain.timeImplementation(), RtiTckMain.callbackModel());
   }

   static void fomTransportationDescriptionIgnored() throws Exception {
      FomTransportationDescriptionIgnoredTck.run(RtiTckMain.factory(),
         RtiTckMain.requireAdapterFile("hla.rti.tck.transportationDuplicateBaseFom"),
         RtiTckMain.requireAdapterFile("hla.rti.tck.transportationDescriptionDifferentFom"),
         RtiTckMain.timeImplementation(), RtiTckMain.callbackModel());
   }

   static void fomBasicDataRepresentationEndianValues() throws Exception {
      FomBasicDataRepresentationEndianValuesTck.run(RtiTckMain.factory(),
         RtiTckMain.requireAdapterFile("hla.rti.tck.basicDataRepresentationEndianBigFom"),
         RtiTckMain.requireAdapterFile("hla.rti.tck.basicDataRepresentationEndianLittleFom"),
         RtiTckMain.timeImplementation(), RtiTckMain.callbackModel());
   }

   static void fomAutomaticResignActionSetting() throws Exception {
      FomAutomaticResignActionSettingTck.run(RtiTckMain.factory(),
         RtiTckMain.requireAdapterFile("hla.rti.tck.resignActionInvalidFom"),
         RtiTckMain.requireAdapterFile("hla.rti.tck.resignActionUnconditionallyDivestFom"),
         RtiTckMain.requireAdapterFile("hla.rti.tck.resignActionDeleteObjectsFom"),
         RtiTckMain.requireAdapterFile("hla.rti.tck.resignActionCancelPendingFom"),
         RtiTckMain.requireAdapterFile("hla.rti.tck.resignActionDeleteThenDivestFom"),
         RtiTckMain.requireAdapterFile("hla.rti.tck.resignActionCancelThenDeleteThenDivestFom"),
         RtiTckMain.requireAdapterFile("hla.rti.tck.resignActionNoActionFom"),
         RtiTckMain.requireMim(), RtiTckMain.timeImplementation(),
         RtiTckMain.callbackModel(),
         System.getProperty("hla.rti.tck.federateType", "tck-type"));
   }

}
