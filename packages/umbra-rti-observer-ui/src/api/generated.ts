/**
 * This file is generated from the FastAPI /openapi.json document.
 * Do not edit it directly; run pnpm generate:api.
 */

export interface paths {
    "/": {
        parameters: {
            query?: never;
            header?: never;
            path?: never;
            cookie?: never;
        };
        /** Dashboard */
        get: operations["dashboard__get"];
        put?: never;
        post?: never;
        delete?: never;
        options?: never;
        head?: never;
        patch?: never;
        trace?: never;
    };
    "/api/analytics": {
        parameters: {
            query?: never;
            header?: never;
            path?: never;
            cookie?: never;
        };
        /** Analytics */
        get: operations["analytics_api_analytics_get"];
        put?: never;
        post?: never;
        delete?: never;
        options?: never;
        head?: never;
        patch?: never;
        trace?: never;
    };
    "/api/catalog": {
        parameters: {
            query?: never;
            header?: never;
            path?: never;
            cookie?: never;
        };
        /** Catalog */
        get: operations["catalog_api_catalog_get"];
        put?: never;
        post?: never;
        delete?: never;
        options?: never;
        head?: never;
        patch?: never;
        trace?: never;
    };
    "/api/commands": {
        parameters: {
            query?: never;
            header?: never;
            path?: never;
            cookie?: never;
        };
        /** Command History */
        get: operations["command_history_api_commands_get"];
        put?: never;
        /** Submit Command */
        post: operations["submit_command_api_commands_post"];
        delete?: never;
        options?: never;
        head?: never;
        patch?: never;
        trace?: never;
    };
    "/api/commands/{command_id}": {
        parameters: {
            query?: never;
            header?: never;
            path?: never;
            cookie?: never;
        };
        /** Command Result */
        get: operations["command_result_api_commands__command_id__get"];
        put?: never;
        post?: never;
        delete?: never;
        options?: never;
        head?: never;
        patch?: never;
        trace?: never;
    };
    "/api/commands/catalog": {
        parameters: {
            query?: never;
            header?: never;
            path?: never;
            cookie?: never;
        };
        /** Command Catalog */
        get: operations["command_catalog_api_commands_catalog_get"];
        put?: never;
        post?: never;
        delete?: never;
        options?: never;
        head?: never;
        patch?: never;
        trace?: never;
    };
    "/api/context": {
        parameters: {
            query?: never;
            header?: never;
            path?: never;
            cookie?: never;
        };
        get?: never;
        /** Context */
        put: operations["context_api_context_put"];
        post?: never;
        delete?: never;
        options?: never;
        head?: never;
        patch?: never;
        trace?: never;
    };
    "/api/contract": {
        parameters: {
            query?: never;
            header?: never;
            path?: never;
            cookie?: never;
        };
        /** Contract */
        get: operations["contract_api_contract_get"];
        put?: never;
        post?: never;
        delete?: never;
        options?: never;
        head?: never;
        patch?: never;
        trace?: never;
    };
    "/api/control/status": {
        parameters: {
            query?: never;
            header?: never;
            path?: never;
            cookie?: never;
        };
        get?: never;
        put?: never;
        /** Status */
        post: operations["status_api_control_status_post"];
        delete?: never;
        options?: never;
        head?: never;
        patch?: never;
        trace?: never;
    };
    "/api/events": {
        parameters: {
            query?: never;
            header?: never;
            path?: never;
            cookie?: never;
        };
        /** Events */
        get: operations["events_api_events_get"];
        put?: never;
        /** Ingest */
        post: operations["ingest_api_events_post"];
        /** Clear Events */
        delete: operations["clear_events_api_events_delete"];
        options?: never;
        head?: never;
        patch?: never;
        trace?: never;
    };
    "/api/events/stream": {
        parameters: {
            query?: never;
            header?: never;
            path?: never;
            cookie?: never;
        };
        /** Event Stream */
        get: operations["event_stream_api_events_stream_get"];
        put?: never;
        post?: never;
        delete?: never;
        options?: never;
        head?: never;
        patch?: never;
        trace?: never;
    };
    "/api/health": {
        parameters: {
            query?: never;
            header?: never;
            path?: never;
            cookie?: never;
        };
        /** Health */
        get: operations["health_api_health_get"];
        put?: never;
        post?: never;
        delete?: never;
        options?: never;
        head?: never;
        patch?: never;
        trace?: never;
    };
    "/api/inspectors/{kind}": {
        parameters: {
            query?: never;
            header?: never;
            path?: never;
            cookie?: never;
        };
        /** Inspectors */
        get: operations["inspectors_api_inspectors__kind__get"];
        put?: never;
        post?: never;
        delete?: never;
        options?: never;
        head?: never;
        patch?: never;
        trace?: never;
    };
    "/api/metrics": {
        parameters: {
            query?: never;
            header?: never;
            path?: never;
            cookie?: never;
        };
        /** Metrics */
        get: operations["metrics_api_metrics_get"];
        put?: never;
        post?: never;
        delete?: never;
        options?: never;
        head?: never;
        patch?: never;
        trace?: never;
    };
    "/api/schema": {
        parameters: {
            query?: never;
            header?: never;
            path?: never;
            cookie?: never;
        };
        /** Schema */
        get: operations["schema_api_schema_get"];
        put?: never;
        post?: never;
        delete?: never;
        options?: never;
        head?: never;
        patch?: never;
        trace?: never;
    };
    "/api/state": {
        parameters: {
            query?: never;
            header?: never;
            path?: never;
            cookie?: never;
        };
        /** State */
        get: operations["state_api_state_get"];
        put?: never;
        post?: never;
        delete?: never;
        options?: never;
        head?: never;
        patch?: never;
        trace?: never;
    };
}
export type webhooks = Record<string, never>;
export interface components {
    schemas: {
        /** ActivityBucket */
        ActivityBucket: {
            /** Callbacks */
            callbacks: number;
            /** Errors */
            errors: number;
            /** Events */
            events: number;
            /** Interactions */
            interactions: number;
            /** Object Updates */
            object_updates: number;
            /** Started At */
            started_at: number;
        };
        /** AdapterResponse */
        AdapterResponse: {
            /** Capabilities */
            capabilities?: string[];
            /** Name */
            name: string;
            /** Status */
            status: string;
        };
        /** AnalyticsResponse */
        AnalyticsResponse: {
            /** Activity */
            activity: components["schemas"]["ActivityBucket"][];
            /**
             * Basis
             * @constant
             */
            basis: "retained-observations";
            /** Bucket Seconds */
            bucket_seconds: number;
            /** Contract Version */
            contract_version: string;
            /** Event Types */
            event_types: {
                [key: string]: number;
            };
            /** Events In Window */
            events_in_window: number;
            /** Families */
            families: {
                [key: string]: number;
            };
            /** Generated At */
            generated_at: number;
            /** History Truncated */
            history_truncated: boolean;
            /** Logical Times */
            logical_times: components["schemas"]["TimeObservation"][];
            /** Participants */
            participants: components["schemas"]["FederateActivity"][];
            retention: components["schemas"]["RetentionResponse"];
            /** Scoped Federate */
            scoped_federate: string | null;
            /** Sources */
            sources: {
                [key: string]: number;
            };
            /** Unattributed Events */
            unattributed_events: number;
            /** Window Seconds */
            window_seconds: number;
            /** Window Start */
            window_start: number;
        };
        /** CapabilityResponse */
        CapabilityResponse: {
            /** Description */
            description: string;
            /** Event Types */
            event_types?: string[];
            /** Label */
            label: string;
            /** Observations */
            observations?: {
                [key: string]: components["schemas"]["JsonValue-Output"];
            }[];
            /** Owner */
            owner: string;
            /** Owners */
            owners?: string[];
            /** Reason */
            reason?: string | null;
            /** Reasons */
            reasons?: string[];
            /** Source */
            source?: string | null;
            /** Sources */
            sources?: string[];
            /**
             * State
             * @enum {string}
             */
            state: "available" | "partial" | "disabled" | "unsupported" | "unknown";
        } & {
            [key: string]: unknown;
        };
        /** CapabilitySchemaResponse */
        CapabilitySchemaResponse: {
            /** Fields */
            fields: string[];
            /** Known */
            known: string[];
            /** State Meanings */
            state_meanings: {
                [key: string]: string;
            };
            /** States */
            states: ("available" | "partial" | "disabled" | "unsupported" | "unknown")[];
        };
        /** CatalogResponse */
        CatalogResponse: {
            /** Capabilities */
            capabilities?: {
                [key: string]: components["schemas"]["CapabilityResponse"];
            };
            contract: components["schemas"]["ContractResponse"];
            /** Contract Version */
            contract_version: string;
            /** Event Schema */
            event_schema: {
                [key: string]: components["schemas"]["JsonValue-Output"];
            };
            /** Features */
            features?: string[];
            /** Schema Version */
            schema_version: string;
        };
        /** CommandCatalog */
        CommandCatalog: {
            /** Contract Version */
            contract_version: string;
            /** Enabled */
            enabled: boolean;
            /** History Limit */
            history_limit: number;
            /** Targets */
            targets: components["schemas"]["CommandTarget"][];
            /** Timeout Seconds */
            timeout_seconds: number;
        };
        /** CommandDefinition */
        CommandDefinition: {
            /**
             * Action
             * @enum {string}
             */
            action: "session.connect" | "federate.join" | "federate.resign" | "session.disconnect";
            /**
             * Completion
             * @default service-return
             * @constant
             */
            completion: "service-return";
            /** Description */
            description: string;
            /**
             * Fields
             * @default []
             */
            fields: components["schemas"]["CommandField"][];
            /** Parameter Model */
            parameter_model: string;
            /** Parameter Schema */
            parameter_schema: {
                [key: string]: components["schemas"]["JsonValue-Output"];
            };
            /** Reason */
            reason?: string | null;
            /**
             * Requires Confirmation
             * @default false
             */
            requires_confirmation: boolean;
            /**
             * Scope
             * @default controlled-session
             * @constant
             */
            scope: "controlled-session";
            /**
             * State
             * @enum {string}
             */
            state: "available" | "partial" | "disabled" | "unsupported" | "unknown";
            /** Title */
            title: string;
        };
        /** CommandError */
        CommandError: {
            /** Code */
            code: string;
            /**
             * Effect
             * @enum {string}
             */
            effect: "none" | "unknown";
            /** Message */
            message: string;
        };
        /** CommandField */
        CommandField: {
            /**
             * Choices
             * @default []
             */
            choices: string[];
            /**
             * Default Value
             * @default
             */
            default_value: string;
            /**
             * Input
             * @enum {string}
             */
            input: "text" | "select";
            /** Label */
            label: string;
            /** Name */
            name: string;
            /** Required */
            required: boolean;
        };
        /** CommandHistory */
        CommandHistory: {
            /** Contract Version */
            contract_version: string;
            /** Evicted Records */
            evicted_records: number;
            /** Limit */
            limit: number;
            /**
             * Persistence
             * @default server-process
             * @constant
             */
            persistence: "server-process";
            /** Records */
            records: components["schemas"]["CommandRecord"][];
        };
        /** CommandOutcome */
        CommandOutcome: {
            /** Federate Handle */
            federate_handle?: string | null;
            /** Summary */
            summary: string;
        };
        /** CommandRecord */
        CommandRecord: {
            /** Accepted At */
            accepted_at: number;
            /** Command Id */
            command_id: string;
            error?: components["schemas"]["CommandError"] | null;
            /** Finished At */
            finished_at?: number | null;
            late_error?: components["schemas"]["CommandError"] | null;
            /** Late Finished At */
            late_finished_at?: number | null;
            late_outcome?: components["schemas"]["CommandOutcome"] | null;
            outcome?: components["schemas"]["CommandOutcome"] | null;
            /** Request */
            request: components["schemas"]["ConnectCommand"] | components["schemas"]["JoinCommand"] | components["schemas"]["ResignCommand"] | components["schemas"]["DisconnectCommand"];
            /** Started At */
            started_at?: number | null;
            /**
             * Status
             * @enum {string}
             */
            status: "accepted" | "running" | "succeeded" | "failed" | "unavailable";
        };
        /** CommandTarget */
        CommandTarget: {
            /** Adapter */
            adapter: string;
            /**
             * Busy
             * @default false
             */
            busy: boolean;
            /** Commands */
            commands: components["schemas"]["CommandDefinition"][];
            /** Fault */
            fault?: string | null;
            /** Federate */
            federate?: string | null;
            /** Federation */
            federation?: string | null;
            /** Label */
            label: string;
            /**
             * State
             * @enum {string}
             */
            state: "disconnected" | "connected" | "joined" | "unknown";
            /**
             * Synthetic
             * @default false
             */
            synthetic: boolean;
            /** Target Id */
            target_id: string;
        };
        /** ConnectCommand */
        ConnectCommand: {
            /**
             * @description discriminator enum property added by openapi-typescript
             * @enum {string}
             */
            action: "session.connect";
            /**
             * Confirmed
             * @default false
             */
            confirmed: boolean;
            parameters: components["schemas"]["EmptyParameters"];
            /** Request Id */
            request_id: string;
            /** Target Id */
            target_id: string;
        };
        /**
         * ContextRequest
         * @description JSON object merged into the observer context.
         */
        ContextRequest: {
            [key: string]: components["schemas"]["JsonValue-Input"];
        };
        /** ContextResponse */
        ContextResponse: {
            /** Context */
            context?: {
                [key: string]: components["schemas"]["JsonValue-Output"];
            };
        };
        /** ContractResponse */
        ContractResponse: {
            /** Api Paths */
            api_paths: {
                [key: string]: string;
            };
            /** Backend Rule */
            backend_rule: string;
            /** Capabilities */
            capabilities?: {
                [key: string]: components["schemas"]["CapabilityResponse"];
            };
            capability_schema: components["schemas"]["CapabilitySchemaResponse"];
            /** Contract Version */
            contract_version: string;
            event_envelope: components["schemas"]["EventEnvelopeDescription"];
            /** Event Schema Version */
            event_schema_version: string;
            /** Event Types */
            event_types: string[];
            events: components["schemas"]["ResourceDescription"];
            /** Media Type */
            media_type: string;
            snapshot: components["schemas"]["ResourceDescription"];
            stream: components["schemas"]["StreamDescription"];
        };
        /** DeclarationScopeResponse */
        DeclarationScopeResponse: {
            /** Federate */
            federate: string | null;
            /** Federation */
            federation: string | null;
            /** Session Id */
            session_id: string | null;
            /** Source */
            source: string;
        };
        /** DeclarationSnapshotResponse */
        DeclarationSnapshotResponse: {
            /**
             * Attribute Scope
             * @enum {string}
             */
            attribute_scope: "selected" | "all" | "unknown" | "not-applicable";
            /** Attributes */
            attributes?: string[] | null;
            /** Class Key */
            class_key: string;
            /** Class Name */
            class_name?: string | null;
            /**
             * Direction
             * @enum {string}
             */
            direction: "publication" | "subscription";
            /**
             * Kind
             * @enum {string}
             */
            kind: "object" | "interaction";
            /** Observed At */
            observed_at: number;
            /** Reason */
            reason?: string | null;
            scope: components["schemas"]["DeclarationScopeResponse"];
            /** Sequence */
            sequence: number;
            /**
             * Status
             * @enum {string}
             */
            status: "declared" | "withdrawn" | "unknown";
        };
        /** DeclarationsResponse */
        DeclarationsResponse: {
            /**
             * Basis
             * @constant
             */
            basis: "last-reported-declarations";
            /** Capacity */
            capacity: number;
            /** Dropped Declarations */
            dropped_declarations: number;
            /** Snapshots */
            snapshots: components["schemas"]["DeclarationSnapshotResponse"][];
        };
        /** DisconnectCommand */
        DisconnectCommand: {
            /**
             * @description discriminator enum property added by openapi-typescript
             * @enum {string}
             */
            action: "session.disconnect";
            /**
             * Confirmed
             * @default false
             */
            confirmed: boolean;
            parameters: components["schemas"]["EmptyParameters"];
            /** Request Id */
            request_id: string;
            /** Target Id */
            target_id: string;
        };
        /**
         * EmptyParameters
         * @description Connection configuration and credentials belong to the server.
         */
        EmptyParameters: Record<string, never>;
        /**
         * EventBatchRequest
         * @description Atomic event batch request.
         *
         *     Extra keys are forbidden so an event object containing an ``events`` field
         *     cannot be silently interpreted as a batch.  Single events remain
         *     extensible through :class:`EventInput`.
         */
        EventBatchRequest: {
            /** Events */
            events: components["schemas"]["EventInput"][];
        };
        /** EventEnvelope */
        EventEnvelope: {
            /** Declaration */
            declaration?: (components["schemas"]["ObjectDeclarationObservation"] | components["schemas"]["InteractionDeclarationObservation"]) | null;
            /** Details */
            details?: {
                [key: string]: components["schemas"]["JsonValue-Output"];
            } | null;
            /** Event Type */
            event_type: string;
            /** Family */
            family: string;
            /** Federate */
            federate?: string | {
                [key: string]: components["schemas"]["JsonValue-Output"];
            } | null;
            /** Federate Name */
            federate_name?: string | null;
            /** Logical Time */
            logical_time?: number | null;
            /** Object Key */
            object_key?: string | null;
            /** Observed At */
            observed_at: number;
            /** Operation */
            operation?: string | null;
            /** Participant */
            participant?: string | {
                [key: string]: components["schemas"]["JsonValue-Output"];
            } | null;
            /** Participant Name */
            participant_name?: string | null;
            /** Sequence */
            sequence: number;
            /** Source */
            source: string;
            /** Time Observation */
            time_observation?: (components["schemas"]["TimeRequestObservation"] | components["schemas"]["TimeGrantObservation"] | components["schemas"]["TimeStateObservation"] | components["schemas"]["TimeOutcomeObservation"]) | null;
        } & {
            [key: string]: unknown;
        };
        /** EventEnvelopeDescription */
        EventEnvelopeDescription: {
            /** Extension Rule */
            extension_rule: string;
            /** Required Fields */
            required_fields: string[];
        };
        /**
         * EventInput
         * @description Normalized event input plus JSON-compatible provider extensions.
         */
        EventInput: {
            /** Attributes */
            attributes?: {
                [key: string]: components["schemas"]["JsonValue-Input"];
            } | null;
            /** Class Handle */
            class_handle?: string | null;
            /** Class Name */
            class_name?: string | null;
            /** Connection */
            connection?: string | null;
            /** Declaration */
            declaration?: (components["schemas"]["ObjectDeclarationObservation"] | components["schemas"]["InteractionDeclarationObservation"]) | null;
            /** Details */
            details?: {
                [key: string]: components["schemas"]["JsonValue-Input"];
            } | null;
            /** Event Type */
            event_type?: string | null;
            /** Extensions */
            extensions?: {
                [key: string]: components["schemas"]["JsonValue-Input"];
            };
            /** Family */
            family?: string | null;
            /** Federate */
            federate?: string | {
                [key: string]: components["schemas"]["JsonValue-Input"];
            } | null;
            /** Federate Handle */
            federate_handle?: string | null;
            /** Federate Name */
            federate_name?: string | null;
            /** Federation */
            federation?: string | null;
            /** Interaction Class */
            interaction_class?: string | null;
            /** Interaction Handle */
            interaction_handle?: string | null;
            /** Interaction Key */
            interaction_key?: string | null;
            /** Kind */
            kind?: string | null;
            /** Logical Time */
            logical_time?: number | null;
            /** Object Handle */
            object_handle?: string | null;
            /** Object Key */
            object_key?: string | null;
            /** Object Name */
            object_name?: string | null;
            /** Observed At */
            observed_at?: number | null;
            /** Operation */
            operation?: string | null;
            /** Parameters */
            parameters?: {
                [key: string]: components["schemas"]["JsonValue-Input"];
            } | null;
            /** Participant */
            participant?: string | {
                [key: string]: components["schemas"]["JsonValue-Input"];
            } | null;
            /** Participant Handle */
            participant_handle?: string | null;
            /** Participant Name */
            participant_name?: string | null;
            /** Role */
            role?: string | null;
            /** Sequence */
            sequence?: number | null;
            /** Source */
            source?: string | null;
            tag?: components["schemas"]["JsonValue-Input"];
            /** Time Observation */
            time_observation?: (components["schemas"]["TimeRequestObservation"] | components["schemas"]["TimeGrantObservation"] | components["schemas"]["TimeStateObservation"] | components["schemas"]["TimeOutcomeObservation"]) | null;
        };
        /** EventPageResponse */
        EventPageResponse: {
            /** Cleared Events */
            cleared_events: number;
            /** Contract Version */
            contract_version: string;
            /** Dropped Events */
            dropped_events: number;
            /** Events */
            events?: components["schemas"]["EventEnvelope"][];
            /** First Retained Sequence */
            first_retained_sequence: number;
            /** Gap */
            gap: boolean;
            /** Has More */
            has_more: boolean;
            /** Last Sequence */
            last_sequence: number;
            /** Next After */
            next_after: number;
            /** Retained Events */
            retained_events: number;
        };
        /** FederateActivity */
        FederateActivity: {
            /** Error Count */
            error_count: number;
            /** Event Count */
            event_count: number;
            /** Federate */
            federate: string;
            /** Handle */
            handle: string | null;
            /** Interaction Count */
            interaction_count: number;
            /** Last Seen */
            last_seen: number | null;
            /** Latest Logical Time */
            latest_logical_time: number | null;
            /** Object Keys */
            object_keys: string[];
            /** Object Updates */
            object_updates: number;
            /** Role */
            role: string | null;
            /** Source */
            source: string;
            /** Status */
            status: string;
        };
        /** FederateInspectionResponse */
        FederateInspectionResponse: {
            /** Federate */
            federate: string;
            /** Handle */
            handle?: string | null;
            /** Last Seen */
            last_seen?: number | null;
            /** Role */
            role?: string | null;
            /** Source */
            source: string;
            /** Status */
            status: string;
        };
        /** HealthResponse */
        HealthResponse: {
            /** Capabilities */
            capabilities?: {
                [key: string]: components["schemas"]["CapabilityResponse"];
            };
            /** Contract Version */
            contract_version: string;
            /** Event Count */
            event_count: number;
            retention: components["schemas"]["RetentionResponse"];
            /** Schema Version */
            schema_version: string;
            /** Service */
            service: string;
            /** Status */
            status: string;
        };
        /** HTTPValidationError */
        HTTPValidationError: {
            /** Detail */
            detail?: components["schemas"]["ValidationError"][];
        };
        /** IngestResponse */
        IngestResponse: {
            /** Accepted */
            accepted: number;
            state: components["schemas"]["StateResponse"];
        };
        /** InspectorResponse */
        InspectorResponse: {
            /** Contract Version */
            contract_version: string;
            /** Items */
            items?: (components["schemas"]["ObjectInspectionResponse"] | components["schemas"]["InteractionInspectionResponse"] | components["schemas"]["FederateInspectionResponse"])[];
            /**
             * Kind
             * @enum {string}
             */
            kind: "objects" | "interactions" | "federates";
        };
        /** InspectorsResponse */
        InspectorsResponse: {
            declarations?: components["schemas"]["DeclarationsResponse"] | null;
            /** Interactions */
            interactions?: components["schemas"]["InteractionInspectionResponse"][];
            object_observations?: components["schemas"]["ObjectObservationsResponse"] | null;
            /** Objects */
            objects?: components["schemas"]["ObjectInspectionResponse"][];
            time?: components["schemas"]["TimeInspectionResponse"] | null;
        };
        /** InteractionDeclarationObservation */
        InteractionDeclarationObservation: {
            /** Class Key */
            class_key: string;
            /** Class Name */
            class_name?: string | null;
            /**
             * Direction
             * @enum {string}
             */
            direction: "publication" | "subscription";
            /**
             * @description discriminator enum property added by openapi-typescript
             * @enum {string}
             */
            kind: "interaction";
            /** Reason */
            reason?: string | null;
            /** Session Id */
            session_id?: string | null;
            /**
             * Status
             * @enum {string}
             */
            status: "declared" | "withdrawn" | "unknown";
        };
        /** InteractionInspectionResponse */
        InteractionInspectionResponse: {
            /** Class Handle */
            class_handle?: string | null;
            /** Family */
            family: string;
            /** Federate */
            federate?: string | null;
            /** Interaction Class */
            interaction_class: string;
            /** Interaction Key */
            interaction_key: string;
            /** Observed At */
            observed_at: number;
            /** Parameters */
            parameters?: {
                [key: string]: components["schemas"]["JsonValue-Output"];
            };
            /** Sequence */
            sequence: number;
            /** Source */
            source: string;
            tag?: components["schemas"]["JsonValue-Output"];
        };
        /** JoinCommand */
        JoinCommand: {
            /**
             * @description discriminator enum property added by openapi-typescript
             * @enum {string}
             */
            action: "federate.join";
            /**
             * Confirmed
             * @default false
             */
            confirmed: boolean;
            parameters: components["schemas"]["JoinParameters"];
            /** Request Id */
            request_id: string;
            /** Target Id */
            target_id: string;
        };
        /** JoinParameters */
        JoinParameters: {
            /** Federate Name */
            federate_name: string;
            /**
             * Federate Type
             * @default Observer
             */
            federate_type: string;
            /** Federation */
            federation: string;
        };
        /**
         * JsonObjectResponse
         * @description Validated object response for the JSON schema endpoint.
         */
        JsonObjectResponse: {
            [key: string]: components["schemas"]["JsonValue-Output"];
        };
        "JsonValue-Input": unknown;
        "JsonValue-Output": unknown;
        /** LiveMetricsResponse */
        LiveMetricsResponse: {
            /** Error Count */
            error_count: number;
            /** Event Count */
            event_count: number;
            /** Event Rate Per Second */
            event_rate_per_second: number;
            /** Event Types */
            event_types?: {
                [key: string]: number;
            };
            /** Families */
            families?: {
                [key: string]: number;
            };
            /**
             * Federate Count
             * @default 0
             */
            federate_count: number;
            /** First Sequence */
            first_sequence: number;
            /**
             * Interaction Count
             * @default 0
             */
            interaction_count: number;
            /** Last Sequence */
            last_sequence: number;
            /**
             * Object Count
             * @default 0
             */
            object_count: number;
            /** Sources */
            sources?: {
                [key: string]: number;
            };
        };
        /** MetricsResponse */
        MetricsResponse: {
            /** Adapters */
            adapters?: components["schemas"]["AdapterResponse"][];
            /** Contract Version */
            contract_version: string;
            live_metrics: components["schemas"]["LiveMetricsResponse"];
            retention: components["schemas"]["RetentionResponse"];
            /** Schema Version */
            schema_version: string;
            /** Status */
            status: string;
        };
        /** ObjectAttributeChangeResponse */
        ObjectAttributeChangeResponse: {
            /** Attribute */
            attribute: string;
            /** Federate */
            federate: string | null;
            /** Generation */
            generation: number;
            /** Object Key */
            object_key: string;
            /** Observed At */
            observed_at: number;
            /** Previous Known */
            previous_known: boolean;
            previous_value: components["schemas"]["JsonValue-Output"];
            /** Sequence */
            sequence: number;
            /** Source */
            source: string;
            value: components["schemas"]["JsonValue-Output"];
        };
        /** ObjectDeclarationObservation */
        ObjectDeclarationObservation: {
            /**
             * Attribute Scope
             * @default unknown
             * @enum {string}
             */
            attribute_scope: "selected" | "all" | "unknown";
            /** Attributes */
            attributes?: string[] | null;
            /** Class Key */
            class_key: string;
            /** Class Name */
            class_name?: string | null;
            /**
             * Direction
             * @enum {string}
             */
            direction: "publication" | "subscription";
            /**
             * @description discriminator enum property added by openapi-typescript
             * @enum {string}
             */
            kind: "object";
            /** Reason */
            reason?: string | null;
            /** Session Id */
            session_id?: string | null;
            /**
             * Status
             * @enum {string}
             */
            status: "declared" | "withdrawn" | "unknown";
        };
        /** ObjectInspectionResponse */
        ObjectInspectionResponse: {
            /** Attributes */
            attributes?: {
                [key: string]: components["schemas"]["JsonValue-Output"];
            };
            /** Class Handle */
            class_handle?: string | null;
            /** Class Name */
            class_name?: string | null;
            /** Discovery Count */
            discovery_count: number;
            /** Family */
            family: string;
            /** Last Seen */
            last_seen?: number | null;
            /** Object Handle */
            object_handle?: string | null;
            /** Object Key */
            object_key: string;
            /** Object Name */
            object_name: string;
            /** Sources */
            sources?: string[];
            /** Status */
            status: string;
            /** Update Count */
            update_count: number;
        };
        /** ObjectObservationsResponse */
        ObjectObservationsResponse: {
            /** Changes */
            changes: components["schemas"]["ObjectAttributeChangeResponse"][];
            /** History First Sequence */
            history_first_sequence: number;
            /** History Last Sequence */
            history_last_sequence: number;
            /** History Truncated */
            history_truncated: boolean;
            /** Snapshots */
            snapshots: components["schemas"]["ObjectPerspectiveResponse"][];
        };
        /**
         * ObjectPerspectiveResponse
         * @description Null federate is global; a name is explicit adapter attribution only.
         */
        ObjectPerspectiveResponse: {
            /** Federate */
            federate: string | null;
            /** Generation */
            generation: number;
            /** Last Sequence */
            last_sequence: number;
            object: components["schemas"]["ObjectInspectionResponse"];
        };
        /** ReportedTimeStateResponse */
        ReportedTimeStateResponse: {
            /** Constrained */
            constrained?: boolean | null;
            /** Galt */
            galt?: number | null;
            /** Lbts */
            lbts?: number | null;
            /** Logical Time */
            logical_time?: number | null;
            /** Lookahead */
            lookahead?: number | null;
            /** Regulating */
            regulating?: boolean | null;
        };
        /** ResignCommand */
        ResignCommand: {
            /**
             * @description discriminator enum property added by openapi-typescript
             * @enum {string}
             */
            action: "federate.resign";
            /**
             * Confirmed
             * @default false
             */
            confirmed: boolean;
            parameters: components["schemas"]["ResignParameters"];
            /** Request Id */
            request_id: string;
            /** Target Id */
            target_id: string;
        };
        /** ResignParameters */
        ResignParameters: {
            /**
             * Policy
             * @default no_action
             * @enum {string}
             */
            policy: "no_action" | "unconditionally_divest_attributes" | "delete_objects" | "cancel_pending_ownership_acquisitions" | "delete_objects_then_divest" | "cancel_then_delete_then_divest";
        };
        /** ResourceDescription */
        ResourceDescription: {
            /**
             * Method
             * @enum {string}
             */
            method: "GET" | "POST" | "PUT" | "DELETE";
            /** Path */
            path: string;
            /** Query */
            query?: {
                [key: string]: string;
            };
            /** Required Fields */
            required_fields?: string[];
        };
        /** RetentionResponse */
        RetentionResponse: {
            /** Cleared Events */
            cleared_events: number;
            /** Dropped Events */
            dropped_events: number;
            /** First Retained Sequence */
            first_retained_sequence: number;
            /** Last Sequence */
            last_sequence: number;
            /** Max Events */
            max_events: number;
            /** Next Sequence */
            next_sequence: number;
            /** Retained Events */
            retained_events: number;
        };
        /** StateResponse */
        StateResponse: {
            /** Adapters */
            adapters?: components["schemas"]["AdapterResponse"][];
            /** Capabilities */
            capabilities?: {
                [key: string]: components["schemas"]["CapabilityResponse"];
            };
            /** Context */
            context?: {
                [key: string]: components["schemas"]["JsonValue-Output"];
            };
            /** Contract Version */
            contract_version: string;
            /** Federate Roster */
            federate_roster?: components["schemas"]["FederateInspectionResponse"][];
            inspectors: components["schemas"]["InspectorsResponse"];
            live_metrics: components["schemas"]["LiveMetricsResponse"];
            /** Normalized Events */
            normalized_events?: components["schemas"]["EventEnvelope"][] | null;
            retention: components["schemas"]["RetentionResponse"];
            /** Schema Version */
            schema_version: string;
            /** Status */
            status: string;
        };
        /** StatusRequest */
        StatusRequest: {
            /** Context */
            context?: {
                [key: string]: components["schemas"]["JsonValue-Input"];
            };
            /** Extensions */
            extensions?: {
                [key: string]: components["schemas"]["JsonValue-Input"];
            };
            /** Status */
            status: string;
        };
        /** StreamDescription */
        StreamDescription: {
            /** Message Types */
            message_types: ("snapshot" | "events" | "heartbeat" | "gap" | "error")[];
            /** Messages */
            messages: {
                [key: string]: string;
            };
            /** Sse Path */
            sse_path: string;
            /** Websocket Path */
            websocket_path: string;
        };
        /** TimeGrantObservation */
        TimeGrantObservation: {
            /**
             * @description discriminator enum property added by openapi-typescript
             * @enum {string}
             */
            kind: "grant";
            /** Logical Time */
            logical_time?: number | null;
            /** Request Id */
            request_id?: string | null;
            /** Session Id */
            session_id?: string | null;
        };
        /** TimeInspectionResponse */
        TimeInspectionResponse: {
            /** Dropped Requests */
            dropped_requests: number;
            /**
             * Duration Basis
             * @constant
             */
            duration_basis: "observer-ingress-monotonic";
            /** Request Capacity */
            request_capacity: number;
            /** Requests */
            requests: components["schemas"]["TimeRequestResponse"][];
            /**
             * Scope Basis
             * @constant
             */
            scope_basis: "explicit-observations";
            /** Snapshots */
            snapshots: components["schemas"]["TimeSnapshotResponse"][];
        };
        /** TimeObservation */
        TimeObservation: {
            /** Event Type */
            event_type: string;
            /** Federate */
            federate: string | null;
            /** Logical Time */
            logical_time: number;
            /**
             * Observation Kind
             * @default timestamp
             * @enum {string}
             */
            observation_kind: "timestamp" | "request" | "grant" | "state";
            /** Observed At */
            observed_at: number;
            /** Operation */
            operation: string | null;
            /** Sequence */
            sequence: number;
        };
        /** TimeOutcomeObservation */
        TimeOutcomeObservation: {
            /**
             * @description discriminator enum property added by openapi-typescript
             * @enum {string}
             */
            kind: "cancelled" | "failed";
            /** Reason */
            reason?: string | null;
            /** Request Id */
            request_id?: string | null;
            /** Session Id */
            session_id?: string | null;
        };
        /** TimeRequestObservation */
        TimeRequestObservation: {
            /**
             * @description discriminator enum property added by openapi-typescript
             * @enum {string}
             */
            kind: "request";
            /** Logical Time */
            logical_time?: number | null;
            /** Request Id */
            request_id?: string | null;
            /** Session Id */
            session_id?: string | null;
        };
        /** TimeRequestResponse */
        TimeRequestResponse: {
            /** Granted Time */
            granted_time?: number | null;
            /** Latency Seconds */
            latency_seconds?: number | null;
            /** Outcome At */
            outcome_at?: number | null;
            /** Outcome Sequence */
            outcome_sequence?: number | null;
            /** Pending Seconds */
            pending_seconds?: number | null;
            /** Reason */
            reason?: string | null;
            /** Request Id */
            request_id: string;
            /** Request Sequence */
            request_sequence: number;
            /** Requested At */
            requested_at: number;
            /** Requested Time */
            requested_time?: number | null;
            scope: components["schemas"]["TimeScopeResponse"];
            /**
             * Status
             * @enum {string}
             */
            status: "pending" | "granted" | "failed" | "cancelled" | "ambiguous" | "session-ended";
        };
        /** TimeSampleResponse */
        TimeSampleResponse: {
            /** Logical Time */
            logical_time?: number | null;
            /** Observed At */
            observed_at: number;
            /** Request Id */
            request_id?: string | null;
            /** Sequence */
            sequence: number;
        };
        /** TimeScopeResponse */
        TimeScopeResponse: {
            /** Federate */
            federate: string | null;
            /** Federation */
            federation: string | null;
            /** Session Id */
            session_id: string | null;
            /** Source */
            source: string;
        };
        /** TimeSnapshotResponse */
        TimeSnapshotResponse: {
            last_grant?: components["schemas"]["TimeSampleResponse"] | null;
            last_request?: components["schemas"]["TimeSampleResponse"] | null;
            /** Last Sequence */
            last_sequence: number;
            /** Observed At */
            observed_at: number;
            reported_state?: components["schemas"]["ReportedTimeStateResponse"] | null;
            scope: components["schemas"]["TimeScopeResponse"];
            /** State Sequence */
            state_sequence?: number | null;
            /**
             * Status
             * @enum {string}
             */
            status: "observed" | "ended";
            /** Uncorrelated Grants */
            uncorrelated_grants: number;
            /** Uncorrelated Requests */
            uncorrelated_requests: number;
        };
        /** TimeStateObservation */
        TimeStateObservation: {
            /** Constrained */
            constrained?: boolean | null;
            /** Galt */
            galt?: number | null;
            /**
             * @description discriminator enum property added by openapi-typescript
             * @enum {string}
             */
            kind: "state";
            /** Lbts */
            lbts?: number | null;
            /** Logical Time */
            logical_time?: number | null;
            /** Lookahead */
            lookahead?: number | null;
            /** Regulating */
            regulating?: boolean | null;
            /** Session Id */
            session_id?: string | null;
        };
        /** ValidationError */
        ValidationError: {
            /** Context */
            ctx?: Record<string, never>;
            /** Input */
            input?: unknown;
            /** Location */
            loc: (string | number)[];
            /** Message */
            msg: string;
            /** Error Type */
            type: string;
        };
    };
    responses: never;
    parameters: never;
    requestBodies: never;
    headers: never;
    pathItems: never;
}
export type $defs = Record<string, never>;
export interface operations {
    dashboard__get: {
        parameters: {
            query?: never;
            header?: never;
            path?: never;
            cookie?: never;
        };
        requestBody?: never;
        responses: {
            /** @description Successful Response */
            200: {
                headers: {
                    [name: string]: unknown;
                };
                content: {
                    "text/html": string;
                };
            };
        };
    };
    analytics_api_analytics_get: {
        parameters: {
            query?: {
                bucket_count?: number;
                federate?: string | null;
                window_seconds?: number;
            };
            header?: never;
            path?: never;
            cookie?: never;
        };
        requestBody?: never;
        responses: {
            /** @description Successful Response */
            200: {
                headers: {
                    [name: string]: unknown;
                };
                content: {
                    "application/json": components["schemas"]["AnalyticsResponse"];
                };
            };
            /** @description Validation Error */
            422: {
                headers: {
                    [name: string]: unknown;
                };
                content: {
                    "application/json": components["schemas"]["HTTPValidationError"];
                };
            };
        };
    };
    catalog_api_catalog_get: {
        parameters: {
            query?: never;
            header?: never;
            path?: never;
            cookie?: never;
        };
        requestBody?: never;
        responses: {
            /** @description Successful Response */
            200: {
                headers: {
                    [name: string]: unknown;
                };
                content: {
                    "application/json": components["schemas"]["CatalogResponse"];
                };
            };
        };
    };
    command_history_api_commands_get: {
        parameters: {
            query?: never;
            header?: never;
            path?: never;
            cookie?: never;
        };
        requestBody?: never;
        responses: {
            /** @description Successful Response */
            200: {
                headers: {
                    [name: string]: unknown;
                };
                content: {
                    "application/json": components["schemas"]["CommandHistory"];
                };
            };
        };
    };
    submit_command_api_commands_post: {
        parameters: {
            query?: never;
            header?: never;
            path?: never;
            cookie?: never;
        };
        requestBody: {
            content: {
                "application/json": components["schemas"]["ConnectCommand"] | components["schemas"]["JoinCommand"] | components["schemas"]["ResignCommand"] | components["schemas"]["DisconnectCommand"];
            };
        };
        responses: {
            /** @description Successful Response */
            202: {
                headers: {
                    [name: string]: unknown;
                };
                content: {
                    "application/json": components["schemas"]["CommandRecord"];
                };
            };
            /** @description Validation Error */
            422: {
                headers: {
                    [name: string]: unknown;
                };
                content: {
                    "application/json": components["schemas"]["HTTPValidationError"];
                };
            };
        };
    };
    command_result_api_commands__command_id__get: {
        parameters: {
            query?: never;
            header?: never;
            path: {
                command_id: string;
            };
            cookie?: never;
        };
        requestBody?: never;
        responses: {
            /** @description Successful Response */
            200: {
                headers: {
                    [name: string]: unknown;
                };
                content: {
                    "application/json": components["schemas"]["CommandRecord"];
                };
            };
            /** @description Validation Error */
            422: {
                headers: {
                    [name: string]: unknown;
                };
                content: {
                    "application/json": components["schemas"]["HTTPValidationError"];
                };
            };
        };
    };
    command_catalog_api_commands_catalog_get: {
        parameters: {
            query?: never;
            header?: never;
            path?: never;
            cookie?: never;
        };
        requestBody?: never;
        responses: {
            /** @description Successful Response */
            200: {
                headers: {
                    [name: string]: unknown;
                };
                content: {
                    "application/json": components["schemas"]["CommandCatalog"];
                };
            };
        };
    };
    context_api_context_put: {
        parameters: {
            query?: never;
            header?: never;
            path?: never;
            cookie?: never;
        };
        requestBody: {
            content: {
                "application/json": components["schemas"]["ContextRequest"];
            };
        };
        responses: {
            /** @description Successful Response */
            200: {
                headers: {
                    [name: string]: unknown;
                };
                content: {
                    "application/json": components["schemas"]["ContextResponse"];
                };
            };
            /** @description Validation Error */
            422: {
                headers: {
                    [name: string]: unknown;
                };
                content: {
                    "application/json": components["schemas"]["HTTPValidationError"];
                };
            };
        };
    };
    contract_api_contract_get: {
        parameters: {
            query?: never;
            header?: never;
            path?: never;
            cookie?: never;
        };
        requestBody?: never;
        responses: {
            /** @description Successful Response */
            200: {
                headers: {
                    [name: string]: unknown;
                };
                content: {
                    "application/json": components["schemas"]["ContractResponse"];
                };
            };
        };
    };
    status_api_control_status_post: {
        parameters: {
            query?: never;
            header?: never;
            path?: never;
            cookie?: never;
        };
        requestBody: {
            content: {
                "application/json": components["schemas"]["StatusRequest"];
            };
        };
        responses: {
            /** @description Successful Response */
            200: {
                headers: {
                    [name: string]: unknown;
                };
                content: {
                    "application/json": components["schemas"]["StateResponse"];
                };
            };
            /** @description Validation Error */
            422: {
                headers: {
                    [name: string]: unknown;
                };
                content: {
                    "application/json": components["schemas"]["HTTPValidationError"];
                };
            };
        };
    };
    events_api_events_get: {
        parameters: {
            query?: {
                after?: number;
                event_type?: string | null;
                family?: string | null;
                limit?: number;
                source?: string | null;
            };
            header?: never;
            path?: never;
            cookie?: never;
        };
        requestBody?: never;
        responses: {
            /** @description Successful Response */
            200: {
                headers: {
                    [name: string]: unknown;
                };
                content: {
                    "application/json": components["schemas"]["EventPageResponse"];
                };
            };
            /** @description Validation Error */
            422: {
                headers: {
                    [name: string]: unknown;
                };
                content: {
                    "application/json": components["schemas"]["HTTPValidationError"];
                };
            };
        };
    };
    ingest_api_events_post: {
        parameters: {
            query?: never;
            header?: never;
            path?: never;
            cookie?: never;
        };
        requestBody: {
            content: {
                "application/json": components["schemas"]["EventBatchRequest"] | components["schemas"]["EventInput"];
            };
        };
        responses: {
            /** @description Successful Response */
            200: {
                headers: {
                    [name: string]: unknown;
                };
                content: {
                    "application/json": components["schemas"]["IngestResponse"];
                };
            };
            /** @description Validation Error */
            422: {
                headers: {
                    [name: string]: unknown;
                };
                content: {
                    "application/json": components["schemas"]["HTTPValidationError"];
                };
            };
        };
    };
    clear_events_api_events_delete: {
        parameters: {
            query?: never;
            header?: never;
            path?: never;
            cookie?: never;
        };
        requestBody?: never;
        responses: {
            /** @description Successful Response */
            200: {
                headers: {
                    [name: string]: unknown;
                };
                content: {
                    "application/json": components["schemas"]["StateResponse"];
                };
            };
        };
    };
    event_stream_api_events_stream_get: {
        parameters: {
            query?: {
                after?: number;
            };
            header?: never;
            path?: never;
            cookie?: never;
        };
        requestBody?: never;
        responses: {
            /** @description Successful Response */
            200: {
                headers: {
                    [name: string]: unknown;
                };
                content: {
                    "application/json": unknown;
                };
            };
            /** @description Validation Error */
            422: {
                headers: {
                    [name: string]: unknown;
                };
                content: {
                    "application/json": components["schemas"]["HTTPValidationError"];
                };
            };
        };
    };
    health_api_health_get: {
        parameters: {
            query?: never;
            header?: never;
            path?: never;
            cookie?: never;
        };
        requestBody?: never;
        responses: {
            /** @description Successful Response */
            200: {
                headers: {
                    [name: string]: unknown;
                };
                content: {
                    "application/json": components["schemas"]["HealthResponse"];
                };
            };
        };
    };
    inspectors_api_inspectors__kind__get: {
        parameters: {
            query?: never;
            header?: never;
            path: {
                kind: string;
            };
            cookie?: never;
        };
        requestBody?: never;
        responses: {
            /** @description Successful Response */
            200: {
                headers: {
                    [name: string]: unknown;
                };
                content: {
                    "application/json": components["schemas"]["InspectorResponse"];
                };
            };
            /** @description Validation Error */
            422: {
                headers: {
                    [name: string]: unknown;
                };
                content: {
                    "application/json": components["schemas"]["HTTPValidationError"];
                };
            };
        };
    };
    metrics_api_metrics_get: {
        parameters: {
            query?: never;
            header?: never;
            path?: never;
            cookie?: never;
        };
        requestBody?: never;
        responses: {
            /** @description Successful Response */
            200: {
                headers: {
                    [name: string]: unknown;
                };
                content: {
                    "application/json": components["schemas"]["MetricsResponse"];
                };
            };
        };
    };
    schema_api_schema_get: {
        parameters: {
            query?: never;
            header?: never;
            path?: never;
            cookie?: never;
        };
        requestBody?: never;
        responses: {
            /** @description Successful Response */
            200: {
                headers: {
                    [name: string]: unknown;
                };
                content: {
                    "application/json": components["schemas"]["JsonObjectResponse"];
                };
            };
        };
    };
    state_api_state_get: {
        parameters: {
            query?: {
                include_events?: boolean;
            };
            header?: never;
            path?: never;
            cookie?: never;
        };
        requestBody?: never;
        responses: {
            /** @description Successful Response */
            200: {
                headers: {
                    [name: string]: unknown;
                };
                content: {
                    "application/json": components["schemas"]["StateResponse"];
                };
            };
            /** @description Validation Error */
            422: {
                headers: {
                    [name: string]: unknown;
                };
                content: {
                    "application/json": components["schemas"]["HTTPValidationError"];
                };
            };
        };
    };
}
