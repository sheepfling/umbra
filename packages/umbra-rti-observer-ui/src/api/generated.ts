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
    "/api/recordings": {
        parameters: {
            query?: never;
            header?: never;
            path?: never;
            cookie?: never;
        };
        /** Status */
        get: operations["status_api_recordings_get"];
        put?: never;
        post?: never;
        delete?: never;
        options?: never;
        head?: never;
        patch?: never;
        trace?: never;
    };
    "/api/recordings/analyze": {
        parameters: {
            query?: never;
            header?: never;
            path?: never;
            cookie?: never;
        };
        get?: never;
        put?: never;
        /** Analyze */
        post: operations["analyze_api_recordings_analyze_post"];
        delete?: never;
        options?: never;
        head?: never;
        patch?: never;
        trace?: never;
    };
    "/api/recordings/export": {
        parameters: {
            query?: never;
            header?: never;
            path?: never;
            cookie?: never;
        };
        /** Export */
        get: operations["export_api_recordings_export_get"];
        put?: never;
        post?: never;
        delete?: never;
        options?: never;
        head?: never;
        patch?: never;
        trace?: never;
    };
    "/api/recordings/start": {
        parameters: {
            query?: never;
            header?: never;
            path?: never;
            cookie?: never;
        };
        get?: never;
        put?: never;
        /** Start */
        post: operations["start_api_recordings_start_post"];
        delete?: never;
        options?: never;
        head?: never;
        patch?: never;
        trace?: never;
    };
    "/api/recordings/stop": {
        parameters: {
            query?: never;
            header?: never;
            path?: never;
            cookie?: never;
        };
        get?: never;
        put?: never;
        /** Stop */
        post: operations["stop_api_recordings_stop_post"];
        delete?: never;
        options?: never;
        head?: never;
        patch?: never;
        trace?: never;
    };
    "/api/recordings/validate": {
        parameters: {
            query?: never;
            header?: never;
            path?: never;
            cookie?: never;
        };
        get?: never;
        put?: never;
        /** Validated Response */
        post: operations["validated_response_api_recordings_validate_post"];
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
            roster_storage?: components["schemas"]["StorageCollectionResponse"] | null;
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
        /**
         * AnalyticsWindow
         * @description Consistent retained observations/roster, without unrelated inspectors.
         */
        AnalyticsWindow: {
            /** Contract Version */
            contract_version: string;
            /** Federate Roster */
            federate_roster: components["schemas"]["FederateInspectionResponse"][];
            /** Normalized Events */
            normalized_events: components["schemas"]["EventEnvelope-Input"][];
            retention: components["schemas"]["RetentionResponse"];
            storage?: components["schemas"]["StorageCoverageResponse"] | null;
        };
        /** CapabilityResponse */
        "CapabilityResponse-Input": {
            /** Description */
            description: string;
            /** Event Types */
            event_types?: string[];
            /** Label */
            label: string;
            /** Observations */
            observations?: {
                [key: string]: components["schemas"]["JsonValue-Input"];
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
        /** CapabilityResponse */
        "CapabilityResponse-Output": {
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
        /** CaptureStatus */
        CaptureStatus: {
            /** Active */
            active: boolean;
            /** Captured Bytes */
            captured_bytes: number;
            /** Decoded State Bytes */
            decoded_state_bytes?: number | null;
            /** Duration Seconds */
            duration_seconds: number;
            /** Edit Count */
            edit_count?: number | null;
            /** Frame Count */
            frame_count: number;
            /** Label */
            label: string;
            /** Max Bytes */
            max_bytes: number;
            /** Max Decoded State Bytes */
            max_decoded_state_bytes?: 1073741824 | null;
            /** Max Frames */
            max_frames: number;
            /** Max State Bytes */
            max_state_bytes?: 8388608 | null;
            /** Recording Id */
            recording_id: string;
            /** Started At */
            started_at: number;
            /** Stop Reason */
            stop_reason: ("user" | "frame-limit" | "byte-limit" | "invalid-state" | "backend-stopped" | "state-limit" | "decode-limit" | "edit-limit") | null;
            /** Storage Format */
            storage_format?: ("full-v1" | "compact-v2") | null;
        };
        /** CatalogResponse */
        CatalogResponse: {
            /** Capabilities */
            capabilities?: {
                [key: string]: components["schemas"]["CapabilityResponse-Output"];
            };
            contract: components["schemas"]["ContractResponse-Output"];
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
        /** CompactRecordingDocument */
        "CompactRecordingDocument-Input": {
            baseline: components["schemas"]["StateResponse-Input"];
            /** Captured Bytes */
            captured_bytes: number;
            /**
             * Codec
             * @constant
             */
            codec: "observer-json-edits-v1";
            contract: components["schemas"]["ContractResponse-Input"];
            /** Decoded State Bytes */
            decoded_state_bytes: number;
            /** Duration Seconds */
            duration_seconds: number;
            /** Edit Count */
            edit_count: number;
            /** Frames */
            frames: components["schemas"]["RecordingDeltaFrame-Input"][];
            /** Label */
            label: string;
            /** Max Bytes */
            max_bytes: number;
            /**
             * Max Decoded State Bytes
             * @constant
             */
            max_decoded_state_bytes: 1073741824;
            /** Max Frames */
            max_frames: number;
            /**
             * Max State Bytes
             * @constant
             */
            max_state_bytes: 8388608;
            /** Recording Id */
            recording_id: string;
            /**
             * @description discriminator enum property added by openapi-typescript
             * @enum {string}
             */
            recording_version: "umbra-observer-recording-v2";
            /** Started At */
            started_at: number;
            /**
             * Stop Reason
             * @enum {string}
             */
            stop_reason: "user" | "frame-limit" | "byte-limit" | "invalid-state" | "backend-stopped" | "state-limit" | "decode-limit" | "edit-limit";
            /** Stopped At */
            stopped_at: number;
        };
        /** CompactRecordingDocument */
        "CompactRecordingDocument-Output": {
            baseline: components["schemas"]["StateResponse-Output"];
            /** Captured Bytes */
            captured_bytes: number;
            /**
             * Codec
             * @constant
             */
            codec: "observer-json-edits-v1";
            contract: components["schemas"]["ContractResponse-Output"];
            /** Decoded State Bytes */
            decoded_state_bytes: number;
            /** Duration Seconds */
            duration_seconds: number;
            /** Edit Count */
            edit_count: number;
            /** Frames */
            frames: components["schemas"]["RecordingDeltaFrame-Output"][];
            /** Label */
            label: string;
            /** Max Bytes */
            max_bytes: number;
            /**
             * Max Decoded State Bytes
             * @constant
             */
            max_decoded_state_bytes: 1073741824;
            /** Max Frames */
            max_frames: number;
            /**
             * Max State Bytes
             * @constant
             */
            max_state_bytes: 8388608;
            /** Recording Id */
            recording_id: string;
            /**
             * @description discriminator enum property added by openapi-typescript
             * @enum {string}
             */
            recording_version: "umbra-observer-recording-v2";
            /** Started At */
            started_at: number;
            /**
             * Stop Reason
             * @enum {string}
             */
            stop_reason: "user" | "frame-limit" | "byte-limit" | "invalid-state" | "backend-stopped" | "state-limit" | "decode-limit" | "edit-limit";
            /** Stopped At */
            stopped_at: number;
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
        "ContractResponse-Input": {
            /** Api Paths */
            api_paths: {
                [key: string]: string;
            };
            /** Backend Rule */
            backend_rule: string;
            /** Capabilities */
            capabilities?: {
                [key: string]: components["schemas"]["CapabilityResponse-Input"];
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
        /** ContractResponse */
        "ContractResponse-Output": {
            /** Api Paths */
            api_paths: {
                [key: string]: string;
            };
            /** Backend Rule */
            backend_rule: string;
            /** Capabilities */
            capabilities?: {
                [key: string]: components["schemas"]["CapabilityResponse-Output"];
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
        /** CoordinationParticipantResponse */
        CoordinationParticipantResponse: {
            identity: components["schemas"]["ParticipantIdentityResponse"];
            /** Reason */
            reason?: string | null;
            /**
             * Status
             * @enum {string}
             */
            status: "pending" | "achieved" | "begun" | "completed" | "failed" | "unknown";
        };
        /** CoordinationResponse */
        CoordinationResponse: {
            /**
             * Basis
             * @constant
             */
            basis: "explicit-coordination-observations";
            /** Capacity */
            capacity: number;
            /** Dropped Operations */
            dropped_operations: number;
            /** Dropped Transitions */
            dropped_transitions: number;
            /** History Capacity */
            history_capacity: number;
            /** Snapshots */
            snapshots: components["schemas"]["CoordinationSnapshotResponse"][];
            /** Transitions */
            transitions: components["schemas"]["CoordinationSnapshotResponse"][];
        };
        /** CoordinationSnapshotResponse */
        CoordinationSnapshotResponse: {
            /** Federation Id */
            federation_id: string;
            /** Identity Namespace */
            identity_namespace: string;
            /**
             * Kind
             * @enum {string}
             */
            kind: "synchronization" | "save" | "restore";
            /** Label */
            label: string;
            /** Observed At */
            observed_at: number;
            /** Operation Id */
            operation_id: string;
            /** Participants */
            participants?: components["schemas"]["CoordinationParticipantResponse"][] | null;
            /**
             * Phase
             * @enum {string}
             */
            phase: "requested" | "registered" | "registration-failed" | "announced" | "achieved" | "begun" | "completed" | "failed";
            /** Reason */
            reason?: string | null;
            /**
             * Scope
             * @enum {string}
             */
            scope: "local" | "federation";
            /** Sequence */
            sequence: number;
            /** Session Id */
            session_id: string;
            /** Source */
            source: string;
            subject?: components["schemas"]["ParticipantIdentityResponse"] | null;
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
        /** DimensionRangeResponse */
        DimensionRangeResponse: {
            /** Dimension Id */
            dimension_id: string;
            /** Dimension Upper Bound */
            dimension_upper_bound?: string | null;
            /** Lower Bound */
            lower_bound: string;
            /** Upper Bound */
            upper_bound: string;
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
        "EventEnvelope-Input": {
            /** Coordination */
            coordination?: (components["schemas"]["SynchronizationObservation"] | components["schemas"]["PersistenceObservation"]) | null;
            /** Declaration */
            declaration?: (components["schemas"]["ObjectDeclarationObservation"] | components["schemas"]["InteractionDeclarationObservation"]) | null;
            /** Details */
            details?: {
                [key: string]: components["schemas"]["JsonValue-Input"];
            } | null;
            /** Event Type */
            event_type: string;
            /** Family */
            family: string;
            /** Federate */
            federate?: string | {
                [key: string]: components["schemas"]["JsonValue-Input"];
            } | null;
            /** Federate Name */
            federate_name?: string | null;
            /** Fom Observation */
            fom_observation?: (components["schemas"]["FomObjectObservation"] | components["schemas"]["FomInteractionObservation"] | components["schemas"]["FomDatatypeObservation"] | components["schemas"]["FomDimensionObservation"]) | null;
            interaction_flow?: components["schemas"]["InteractionFlowObservationInput"] | null;
            /** Logical Time */
            logical_time?: number | null;
            /** Mom Observation */
            mom_observation?: (components["schemas"]["MomMembersObservation"] | components["schemas"]["MomServiceObservation"] | components["schemas"]["MomCounterObservation"]) | null;
            /** Object Key */
            object_key?: string | null;
            /** Observed At */
            observed_at: number;
            /** Operation */
            operation?: string | null;
            /** Ownership */
            ownership?: (components["schemas"]["OwnershipStateObservation"] | components["schemas"]["OwnershipRequestObservation"] | components["schemas"]["OwnershipOutcomeObservation"]) | null;
            /** Participant */
            participant?: string | {
                [key: string]: components["schemas"]["JsonValue-Input"];
            } | null;
            /** Participant Name */
            participant_name?: string | null;
            /** Region Observation */
            region_observation?: (components["schemas"]["RegionStateObservation"] | components["schemas"]["RegionAssociationObservation"] | components["schemas"]["RegionRoutingObservation"]) | null;
            relationship?: components["schemas"]["RelationshipObservationInput"] | null;
            /** Sequence */
            sequence: number;
            /** Source */
            source: string;
            /** Time Observation */
            time_observation?: (components["schemas"]["TimeRequestObservation"] | components["schemas"]["TimeGrantObservation"] | components["schemas"]["TimeStateObservation"] | components["schemas"]["TimeOutcomeObservation"]) | null;
        } & {
            [key: string]: unknown;
        };
        /** EventEnvelope */
        "EventEnvelope-Output": {
            /** Coordination */
            coordination?: (components["schemas"]["SynchronizationObservation"] | components["schemas"]["PersistenceObservation"]) | null;
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
            /** Fom Observation */
            fom_observation?: (components["schemas"]["FomObjectObservation"] | components["schemas"]["FomInteractionObservation"] | components["schemas"]["FomDatatypeObservation"] | components["schemas"]["FomDimensionObservation"]) | null;
            interaction_flow?: components["schemas"]["InteractionFlowObservationInput"] | null;
            /** Logical Time */
            logical_time?: number | null;
            /** Mom Observation */
            mom_observation?: (components["schemas"]["MomMembersObservation"] | components["schemas"]["MomServiceObservation"] | components["schemas"]["MomCounterObservation"]) | null;
            /** Object Key */
            object_key?: string | null;
            /** Observed At */
            observed_at: number;
            /** Operation */
            operation?: string | null;
            /** Ownership */
            ownership?: (components["schemas"]["OwnershipStateObservation"] | components["schemas"]["OwnershipRequestObservation"] | components["schemas"]["OwnershipOutcomeObservation"]) | null;
            /** Participant */
            participant?: string | {
                [key: string]: components["schemas"]["JsonValue-Output"];
            } | null;
            /** Participant Name */
            participant_name?: string | null;
            /** Region Observation */
            region_observation?: (components["schemas"]["RegionStateObservation"] | components["schemas"]["RegionAssociationObservation"] | components["schemas"]["RegionRoutingObservation"]) | null;
            relationship?: components["schemas"]["RelationshipObservationInput"] | null;
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
            /** Coordination */
            coordination?: (components["schemas"]["SynchronizationObservation"] | components["schemas"]["PersistenceObservation"]) | null;
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
            /** Fom Observation */
            fom_observation?: (components["schemas"]["FomObjectObservation"] | components["schemas"]["FomInteractionObservation"] | components["schemas"]["FomDatatypeObservation"] | components["schemas"]["FomDimensionObservation"]) | null;
            /** Interaction Class */
            interaction_class?: string | null;
            interaction_flow?: components["schemas"]["InteractionFlowObservationInput"] | null;
            /** Interaction Handle */
            interaction_handle?: string | null;
            /** Interaction Key */
            interaction_key?: string | null;
            /** Kind */
            kind?: string | null;
            /** Logical Time */
            logical_time?: number | null;
            /** Mom Observation */
            mom_observation?: (components["schemas"]["MomMembersObservation"] | components["schemas"]["MomServiceObservation"] | components["schemas"]["MomCounterObservation"]) | null;
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
            /** Ownership */
            ownership?: (components["schemas"]["OwnershipStateObservation"] | components["schemas"]["OwnershipRequestObservation"] | components["schemas"]["OwnershipOutcomeObservation"]) | null;
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
            /** Region Observation */
            region_observation?: (components["schemas"]["RegionStateObservation"] | components["schemas"]["RegionAssociationObservation"] | components["schemas"]["RegionRoutingObservation"]) | null;
            relationship?: components["schemas"]["RelationshipObservationInput"] | null;
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
            events?: components["schemas"]["EventEnvelope-Output"][];
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
            /** Next Sequence */
            next_sequence?: number | null;
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
            /** Roster Retained */
            roster_retained?: boolean | null;
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
        /** FomDatatypeObservation */
        FomDatatypeObservation: {
            /** Cardinality */
            cardinality?: string | null;
            /** Description */
            description?: string | null;
            /** Element Id */
            element_id: string;
            /** Element Type Id */
            element_type_id?: string | null;
            /**
             * Encoding
             * @enum {string}
             */
            encoding: "primitive" | "enumeration" | "array" | "record" | "variant" | "opaque" | "unknown";
            /** Enumerators */
            enumerators?: components["schemas"]["FomEnumeratorResponse"][] | null;
            /** Federation Id */
            federation_id: string;
            /** Fields */
            fields?: components["schemas"]["FomFieldResponse"][] | null;
            /** Identity Namespace */
            identity_namespace: string;
            /**
             * @description discriminator enum property added by openapi-typescript
             * @enum {string}
             */
            kind: "datatype";
            /** Model Id */
            model_id: string;
            /** Name */
            name?: string | null;
            /** Representation */
            representation?: string | null;
            /** Revision */
            revision: string;
            /** Session Id */
            session_id: string;
            /**
             * Status
             * @enum {string}
             */
            status: "present" | "removed" | "unknown";
        };
        /** FomDatatypeReportResponse */
        FomDatatypeReportResponse: {
            /** Cardinality */
            cardinality?: string | null;
            /** Description */
            description?: string | null;
            /** Element Id */
            element_id: string;
            /** Element Type Id */
            element_type_id?: string | null;
            /**
             * Encoding
             * @enum {string}
             */
            encoding: "primitive" | "enumeration" | "array" | "record" | "variant" | "opaque" | "unknown";
            /** Enumerators */
            enumerators?: components["schemas"]["FomEnumeratorResponse"][] | null;
            /** Federation Id */
            federation_id: string;
            /** Fields */
            fields?: components["schemas"]["FomFieldResponse"][] | null;
            /** Identity Namespace */
            identity_namespace: string;
            /**
             * @description discriminator enum property added by openapi-typescript
             * @enum {string}
             */
            kind: "datatype";
            /** Model Id */
            model_id: string;
            /** Name */
            name?: string | null;
            /** Observed At */
            observed_at: number;
            /** Representation */
            representation?: string | null;
            /** Revision */
            revision: string;
            /** Sequence */
            sequence: number;
            /** Session Id */
            session_id: string;
            /** Source */
            source: string;
            /**
             * Status
             * @enum {string}
             */
            status: "present" | "removed" | "unknown";
        };
        /** FomDimensionObservation */
        FomDimensionObservation: {
            /** Description */
            description?: string | null;
            /** Element Id */
            element_id: string;
            /** Federation Id */
            federation_id: string;
            /** Identity Namespace */
            identity_namespace: string;
            /**
             * @description discriminator enum property added by openapi-typescript
             * @enum {string}
             */
            kind: "dimension";
            /** Model Id */
            model_id: string;
            /** Name */
            name?: string | null;
            /** Revision */
            revision: string;
            /** Session Id */
            session_id: string;
            /**
             * Status
             * @enum {string}
             */
            status: "present" | "removed" | "unknown";
            /** Unit */
            unit?: string | null;
            /** Upper Bound */
            upper_bound?: string | null;
        };
        /** FomDimensionReportResponse */
        FomDimensionReportResponse: {
            /** Description */
            description?: string | null;
            /** Element Id */
            element_id: string;
            /** Federation Id */
            federation_id: string;
            /** Identity Namespace */
            identity_namespace: string;
            /**
             * @description discriminator enum property added by openapi-typescript
             * @enum {string}
             */
            kind: "dimension";
            /** Model Id */
            model_id: string;
            /** Name */
            name?: string | null;
            /** Observed At */
            observed_at: number;
            /** Revision */
            revision: string;
            /** Sequence */
            sequence: number;
            /** Session Id */
            session_id: string;
            /** Source */
            source: string;
            /**
             * Status
             * @enum {string}
             */
            status: "present" | "removed" | "unknown";
            /** Unit */
            unit?: string | null;
            /** Upper Bound */
            upper_bound?: string | null;
        };
        /** FomEnumeratorResponse */
        FomEnumeratorResponse: {
            /** Name */
            name: string;
            /** Value */
            value: string;
        };
        /** FomFieldResponse */
        FomFieldResponse: {
            /** Datatype Id */
            datatype_id?: string | null;
            /** Description */
            description?: string | null;
            /** Field Id */
            field_id: string;
            /** Name */
            name?: string | null;
        };
        /** FomInteractionObservation */
        FomInteractionObservation: {
            /** Description */
            description?: string | null;
            /** Element Id */
            element_id: string;
            /** Federation Id */
            federation_id: string;
            /**
             * Field Scope
             * @enum {string}
             */
            field_scope: "declared" | "effective" | "unknown";
            /** Identity Namespace */
            identity_namespace: string;
            /**
             * @description discriminator enum property added by openapi-typescript
             * @enum {string}
             */
            kind: "interaction-class";
            /** Model Id */
            model_id: string;
            /** Name */
            name?: string | null;
            /** Parameters */
            parameters?: components["schemas"]["FomFieldResponse"][] | null;
            /** Parent Id */
            parent_id?: string | null;
            /**
             * Parent State
             * @enum {string}
             */
            parent_state: "root" | "parent" | "unknown";
            /** Revision */
            revision: string;
            /** Session Id */
            session_id: string;
            /**
             * Status
             * @enum {string}
             */
            status: "present" | "removed" | "unknown";
        };
        /** FomInteractionReportResponse */
        FomInteractionReportResponse: {
            /** Description */
            description?: string | null;
            /** Element Id */
            element_id: string;
            /** Federation Id */
            federation_id: string;
            /**
             * Field Scope
             * @enum {string}
             */
            field_scope: "declared" | "effective" | "unknown";
            /** Identity Namespace */
            identity_namespace: string;
            /**
             * @description discriminator enum property added by openapi-typescript
             * @enum {string}
             */
            kind: "interaction-class";
            /** Model Id */
            model_id: string;
            /** Name */
            name?: string | null;
            /** Observed At */
            observed_at: number;
            /** Parameters */
            parameters?: components["schemas"]["FomFieldResponse"][] | null;
            /** Parent Id */
            parent_id?: string | null;
            /**
             * Parent State
             * @enum {string}
             */
            parent_state: "root" | "parent" | "unknown";
            /** Revision */
            revision: string;
            /** Sequence */
            sequence: number;
            /** Session Id */
            session_id: string;
            /** Source */
            source: string;
            /**
             * Status
             * @enum {string}
             */
            status: "present" | "removed" | "unknown";
        };
        /** FomObjectObservation */
        FomObjectObservation: {
            /** Attributes */
            attributes?: components["schemas"]["FomFieldResponse"][] | null;
            /** Description */
            description?: string | null;
            /** Element Id */
            element_id: string;
            /** Federation Id */
            federation_id: string;
            /**
             * Field Scope
             * @enum {string}
             */
            field_scope: "declared" | "effective" | "unknown";
            /** Identity Namespace */
            identity_namespace: string;
            /**
             * @description discriminator enum property added by openapi-typescript
             * @enum {string}
             */
            kind: "object-class";
            /** Model Id */
            model_id: string;
            /** Name */
            name?: string | null;
            /** Parent Id */
            parent_id?: string | null;
            /**
             * Parent State
             * @enum {string}
             */
            parent_state: "root" | "parent" | "unknown";
            /** Revision */
            revision: string;
            /** Session Id */
            session_id: string;
            /**
             * Status
             * @enum {string}
             */
            status: "present" | "removed" | "unknown";
        };
        /** FomObjectReportResponse */
        FomObjectReportResponse: {
            /** Attributes */
            attributes?: components["schemas"]["FomFieldResponse"][] | null;
            /** Description */
            description?: string | null;
            /** Element Id */
            element_id: string;
            /** Federation Id */
            federation_id: string;
            /**
             * Field Scope
             * @enum {string}
             */
            field_scope: "declared" | "effective" | "unknown";
            /** Identity Namespace */
            identity_namespace: string;
            /**
             * @description discriminator enum property added by openapi-typescript
             * @enum {string}
             */
            kind: "object-class";
            /** Model Id */
            model_id: string;
            /** Name */
            name?: string | null;
            /** Observed At */
            observed_at: number;
            /** Parent Id */
            parent_id?: string | null;
            /**
             * Parent State
             * @enum {string}
             */
            parent_state: "root" | "parent" | "unknown";
            /** Revision */
            revision: string;
            /** Sequence */
            sequence: number;
            /** Session Id */
            session_id: string;
            /** Source */
            source: string;
            /**
             * Status
             * @enum {string}
             */
            status: "present" | "removed" | "unknown";
        };
        /** FomResponse */
        FomResponse: {
            /**
             * Basis
             * @constant
             */
            basis: "adapter-supplied-fom-metadata";
            /** Capacity */
            capacity: number;
            /** Dropped Snapshots */
            dropped_snapshots: number;
            /** Dropped Transitions */
            dropped_transitions: number;
            /** History Capacity */
            history_capacity: number;
            /** Snapshots */
            snapshots: (components["schemas"]["FomObjectReportResponse"] | components["schemas"]["FomInteractionReportResponse"] | components["schemas"]["FomDatatypeReportResponse"] | components["schemas"]["FomDimensionReportResponse"])[];
            /** Transitions */
            transitions: (components["schemas"]["FomObjectReportResponse"] | components["schemas"]["FomInteractionReportResponse"] | components["schemas"]["FomDatatypeReportResponse"] | components["schemas"]["FomDimensionReportResponse"])[];
        };
        /** HealthResponse */
        HealthResponse: {
            /** Capabilities */
            capabilities?: {
                [key: string]: components["schemas"]["CapabilityResponse-Output"];
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
            state: components["schemas"]["StateResponse-Output"];
        };
        /** InspectorResponse */
        InspectorResponse: {
            /** Contract Version */
            contract_version: string;
            /** Items */
            items?: (components["schemas"]["ObjectInspectionResponse-Output"] | components["schemas"]["InteractionInspectionResponse-Output"] | components["schemas"]["FederateInspectionResponse"])[];
            /**
             * Kind
             * @enum {string}
             */
            kind: "objects" | "interactions" | "federates";
        };
        /** InspectorsResponse */
        "InspectorsResponse-Input": {
            coordination?: components["schemas"]["CoordinationResponse"] | null;
            declarations?: components["schemas"]["DeclarationsResponse"] | null;
            fom?: components["schemas"]["FomResponse"] | null;
            interaction_flow?: components["schemas"]["InteractionFlowResponse"] | null;
            /** Interactions */
            interactions?: components["schemas"]["InteractionInspectionResponse-Input"][];
            mom?: components["schemas"]["MomResponse"] | null;
            object_observations?: components["schemas"]["ObjectObservationsResponse-Input"] | null;
            /** Objects */
            objects?: components["schemas"]["ObjectInspectionResponse-Input"][];
            ownership?: components["schemas"]["OwnershipResponse"] | null;
            regions?: components["schemas"]["RegionsResponse"] | null;
            relationships?: components["schemas"]["RelationshipsResponse"] | null;
            time?: components["schemas"]["TimeInspectionResponse"] | null;
        };
        /** InspectorsResponse */
        "InspectorsResponse-Output": {
            coordination?: components["schemas"]["CoordinationResponse"] | null;
            declarations?: components["schemas"]["DeclarationsResponse"] | null;
            fom?: components["schemas"]["FomResponse"] | null;
            interaction_flow?: components["schemas"]["InteractionFlowResponse"] | null;
            /** Interactions */
            interactions?: components["schemas"]["InteractionInspectionResponse-Output"][];
            mom?: components["schemas"]["MomResponse"] | null;
            object_observations?: components["schemas"]["ObjectObservationsResponse-Output"] | null;
            /** Objects */
            objects?: components["schemas"]["ObjectInspectionResponse-Output"][];
            ownership?: components["schemas"]["OwnershipResponse"] | null;
            regions?: components["schemas"]["RegionsResponse"] | null;
            relationships?: components["schemas"]["RelationshipsResponse"] | null;
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
        /** InteractionFlowObservationInput */
        InteractionFlowObservationInput: {
            /** Class Key */
            class_key?: string | null;
            /** Federation Id */
            federation_id?: string | null;
            /** Identity Namespace */
            identity_namespace?: string | null;
            /**
             * Kind
             * @enum {string}
             */
            kind: "send" | "receive";
            /** Logical Time */
            logical_time?: number | null;
            /** Message Id */
            message_id?: string | null;
            /**
             * Order
             * @enum {string}
             */
            order: "receive-order" | "timestamped" | "unknown";
            producer?: components["schemas"]["ParticipantIdentityResponse"] | null;
            recipient?: components["schemas"]["ParticipantIdentityResponse"] | null;
            /** Session Id */
            session_id?: string | null;
        };
        /** InteractionFlowRecordResponse */
        InteractionFlowRecordResponse: {
            /** Arrival Delay Seconds */
            arrival_delay_seconds?: number | null;
            /** Last Sequence */
            last_sequence: number;
            receive?: components["schemas"]["InteractionFlowReportResponse"] | null;
            /** Receive Count */
            receive_count: number;
            /** Record Id */
            record_id: number;
            send?: components["schemas"]["InteractionFlowReportResponse"] | null;
            /** Send Count */
            send_count: number;
            /**
             * Status
             * @enum {string}
             */
            status: "uncorrelated" | "send-only" | "receive-only" | "paired" | "ambiguous";
        };
        /** InteractionFlowReportResponse */
        InteractionFlowReportResponse: {
            /** Class Key */
            class_key?: string | null;
            /** Federation Id */
            federation_id?: string | null;
            /** Identity Namespace */
            identity_namespace?: string | null;
            /**
             * Kind
             * @enum {string}
             */
            kind: "send" | "receive";
            /** Logical Time */
            logical_time?: number | null;
            /** Message Id */
            message_id?: string | null;
            /** Observed At */
            observed_at: number;
            /**
             * Order
             * @enum {string}
             */
            order: "receive-order" | "timestamped" | "unknown";
            producer?: components["schemas"]["ParticipantIdentityResponse"] | null;
            recipient?: components["schemas"]["ParticipantIdentityResponse"] | null;
            /** Sequence */
            sequence: number;
            /** Session Id */
            session_id?: string | null;
            /** Source */
            source: string;
        };
        /** InteractionFlowResponse */
        InteractionFlowResponse: {
            /**
             * Basis
             * @constant
             */
            basis: "explicit-interaction-observations";
            /** Capacity */
            capacity: number;
            /** Dropped Records */
            dropped_records: number;
            /** Receive Observations */
            receive_observations: number;
            /** Records */
            records: components["schemas"]["InteractionFlowRecordResponse"][];
            /** Send Observations */
            send_observations: number;
        };
        /** InteractionInspectionResponse */
        "InteractionInspectionResponse-Input": {
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
                [key: string]: components["schemas"]["JsonValue-Input"];
            };
            /** Sequence */
            sequence: number;
            /** Source */
            source: string;
            tag?: components["schemas"]["JsonValue-Input"];
        };
        /** InteractionInspectionResponse */
        "InteractionInspectionResponse-Output": {
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
        /** MomCounterObservation */
        MomCounterObservation: {
            /** Counter Key */
            counter_key: string;
            /** Federation Id */
            federation_id: string;
            /** Identity Namespace */
            identity_namespace: string;
            /**
             * @description discriminator enum property added by openapi-typescript
             * @enum {string}
             */
            kind: "counter";
            /** Reason */
            reason?: string | null;
            /** Report Id */
            report_id: string;
            /**
             * Semantics
             * @enum {string}
             */
            semantics: "cumulative" | "gauge" | "unknown";
            /** Session Id */
            session_id: string;
            subject?: components["schemas"]["ParticipantReferenceResponse"] | null;
            /** Unit */
            unit?: string | null;
            /** Value */
            value?: number | null;
            /** Value Text */
            value_text?: string | null;
        };
        /** MomCounterReportResponse */
        MomCounterReportResponse: {
            /** Counter Key */
            counter_key: string;
            /** Federation Id */
            federation_id: string;
            /** Identity Namespace */
            identity_namespace: string;
            /**
             * @description discriminator enum property added by openapi-typescript
             * @enum {string}
             */
            kind: "counter";
            /** Observed At */
            observed_at: number;
            /** Reason */
            reason?: string | null;
            /** Report Id */
            report_id: string;
            /**
             * Semantics
             * @enum {string}
             */
            semantics: "cumulative" | "gauge" | "unknown";
            /** Sequence */
            sequence: number;
            /** Session Id */
            session_id: string;
            /** Source */
            source: string;
            subject?: components["schemas"]["ParticipantReferenceResponse"] | null;
            /** Unit */
            unit?: string | null;
            /** Value */
            value?: number | null;
            /** Value Text */
            value_text?: string | null;
        };
        /** MomMemberResponse */
        MomMemberResponse: {
            participant: components["schemas"]["ParticipantReferenceResponse"];
            /**
             * Status
             * @enum {string}
             */
            status: "joined" | "resigned" | "unknown";
        };
        /** MomMembersObservation */
        MomMembersObservation: {
            /**
             * Coverage
             * @enum {string}
             */
            coverage: "complete" | "partial" | "unknown";
            /** Federation Id */
            federation_id: string;
            /** Identity Namespace */
            identity_namespace: string;
            /**
             * @description discriminator enum property added by openapi-typescript
             * @enum {string}
             */
            kind: "members";
            /** Members */
            members?: components["schemas"]["MomMemberResponse"][] | null;
            /** Reason */
            reason?: string | null;
            /** Report Id */
            report_id: string;
            /** Session Id */
            session_id: string;
        };
        /** MomMembersReportResponse */
        MomMembersReportResponse: {
            /**
             * Coverage
             * @enum {string}
             */
            coverage: "complete" | "partial" | "unknown";
            /** Federation Id */
            federation_id: string;
            /** Identity Namespace */
            identity_namespace: string;
            /**
             * @description discriminator enum property added by openapi-typescript
             * @enum {string}
             */
            kind: "members";
            /** Members */
            members?: components["schemas"]["MomMemberResponse"][] | null;
            /** Observed At */
            observed_at: number;
            /** Reason */
            reason?: string | null;
            /** Report Id */
            report_id: string;
            /** Sequence */
            sequence: number;
            /** Session Id */
            session_id: string;
            /** Source */
            source: string;
        };
        /** MomResponse */
        MomResponse: {
            /**
             * Basis
             * @constant
             */
            basis: "explicit-management-reports";
            /** Capacity */
            capacity: number;
            /** Dropped Snapshots */
            dropped_snapshots: number;
            /** Dropped Transitions */
            dropped_transitions: number;
            /** History Capacity */
            history_capacity: number;
            /** Snapshots */
            snapshots: (components["schemas"]["MomMembersReportResponse"] | components["schemas"]["MomServiceReportResponse"] | components["schemas"]["MomCounterReportResponse"])[];
            /** Transitions */
            transitions: (components["schemas"]["MomMembersReportResponse"] | components["schemas"]["MomServiceReportResponse"] | components["schemas"]["MomCounterReportResponse"])[];
        };
        /** MomServiceObservation */
        MomServiceObservation: {
            /** Error Name */
            error_name?: string | null;
            /** Federation Id */
            federation_id: string;
            /** Identity Namespace */
            identity_namespace: string;
            /** Invocation Id */
            invocation_id?: string | null;
            /**
             * @description discriminator enum property added by openapi-typescript
             * @enum {string}
             */
            kind: "service";
            /** Reason */
            reason?: string | null;
            /** Report Id */
            report_id: string;
            /** Service Name */
            service_name: string;
            /** Session Id */
            session_id: string;
            /**
             * Status
             * @enum {string}
             */
            status: "invoked" | "completed" | "failed" | "unknown";
            subject?: components["schemas"]["ParticipantReferenceResponse"] | null;
        };
        /** MomServiceReportResponse */
        MomServiceReportResponse: {
            /** Error Name */
            error_name?: string | null;
            /** Federation Id */
            federation_id: string;
            /** Identity Namespace */
            identity_namespace: string;
            /** Invocation Id */
            invocation_id?: string | null;
            /**
             * @description discriminator enum property added by openapi-typescript
             * @enum {string}
             */
            kind: "service";
            /** Observed At */
            observed_at: number;
            /** Reason */
            reason?: string | null;
            /** Report Id */
            report_id: string;
            /** Sequence */
            sequence: number;
            /** Service Name */
            service_name: string;
            /** Session Id */
            session_id: string;
            /** Source */
            source: string;
            /**
             * Status
             * @enum {string}
             */
            status: "invoked" | "completed" | "failed" | "unknown";
            subject?: components["schemas"]["ParticipantReferenceResponse"] | null;
        };
        /** ObjectAttributeChangeResponse */
        "ObjectAttributeChangeResponse-Input": {
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
            previous_value: components["schemas"]["JsonValue-Input"];
            /** Sequence */
            sequence: number;
            /** Source */
            source: string;
            value: components["schemas"]["JsonValue-Input"];
        };
        /** ObjectAttributeChangeResponse */
        "ObjectAttributeChangeResponse-Output": {
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
        "ObjectInspectionResponse-Input": {
            /** Attributes */
            attributes?: {
                [key: string]: components["schemas"]["JsonValue-Input"];
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
        /** ObjectInspectionResponse */
        "ObjectInspectionResponse-Output": {
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
        "ObjectObservationsResponse-Input": {
            /** Changes */
            changes: components["schemas"]["ObjectAttributeChangeResponse-Input"][];
            /** Dropped History Batches */
            dropped_history_batches?: number | null;
            /** Dropped Snapshots */
            dropped_snapshots?: number | null;
            /** History First Sequence */
            history_first_sequence: number;
            /** History Last Sequence */
            history_last_sequence: number;
            /** History Truncated */
            history_truncated: boolean;
            /** Omitted Changes */
            omitted_changes?: number | null;
            /** Snapshot Capacity */
            snapshot_capacity?: number | null;
            /** Snapshots */
            snapshots: components["schemas"]["ObjectPerspectiveResponse-Input"][];
        };
        /** ObjectObservationsResponse */
        "ObjectObservationsResponse-Output": {
            /** Changes */
            changes: components["schemas"]["ObjectAttributeChangeResponse-Output"][];
            /** Dropped History Batches */
            dropped_history_batches?: number | null;
            /** Dropped Snapshots */
            dropped_snapshots?: number | null;
            /** History First Sequence */
            history_first_sequence: number;
            /** History Last Sequence */
            history_last_sequence: number;
            /** History Truncated */
            history_truncated: boolean;
            /** Omitted Changes */
            omitted_changes?: number | null;
            /** Snapshot Capacity */
            snapshot_capacity?: number | null;
            /** Snapshots */
            snapshots: components["schemas"]["ObjectPerspectiveResponse-Output"][];
        };
        /**
         * ObjectPerspectiveResponse
         * @description Null federate is global; a name is explicit adapter attribution only.
         */
        "ObjectPerspectiveResponse-Input": {
            /** Federate */
            federate: string | null;
            /** Generation */
            generation: number;
            /** Last Sequence */
            last_sequence: number;
            object: components["schemas"]["ObjectInspectionResponse-Input"];
        };
        /**
         * ObjectPerspectiveResponse
         * @description Null federate is global; a name is explicit adapter attribution only.
         */
        "ObjectPerspectiveResponse-Output": {
            /** Federate */
            federate: string | null;
            /** Generation */
            generation: number;
            /** Last Sequence */
            last_sequence: number;
            object: components["schemas"]["ObjectInspectionResponse-Output"];
        };
        /** OwnershipOutcomeObservation */
        OwnershipOutcomeObservation: {
            /**
             * Action
             * @enum {string}
             */
            action: "acquire" | "divest";
            /** Attribute Key */
            attribute_key: string;
            /** Federation Id */
            federation_id: string;
            /** Identity Namespace */
            identity_namespace: string;
            /**
             * @description discriminator enum property added by openapi-typescript
             * @enum {string}
             */
            kind: "outcome";
            /** Object Id */
            object_id: string;
            /**
             * Outcome
             * @enum {string}
             */
            outcome: "completed" | "failed" | "cancelled";
            /** Reason */
            reason?: string | null;
            /** Request Id */
            request_id: string;
            requester: components["schemas"]["ParticipantIdentityResponse"];
            /** Session Id */
            session_id: string;
        };
        /** OwnershipOutcomeReportResponse */
        OwnershipOutcomeReportResponse: {
            /**
             * Action
             * @enum {string}
             */
            action: "acquire" | "divest";
            /** Attribute Key */
            attribute_key: string;
            /** Federation Id */
            federation_id: string;
            /** Identity Namespace */
            identity_namespace: string;
            /**
             * @description discriminator enum property added by openapi-typescript
             * @enum {string}
             */
            kind: "outcome";
            /** Object Id */
            object_id: string;
            /** Observed At */
            observed_at: number;
            /**
             * Outcome
             * @enum {string}
             */
            outcome: "completed" | "failed" | "cancelled";
            /** Reason */
            reason?: string | null;
            /** Request Id */
            request_id: string;
            requester: components["schemas"]["ParticipantIdentityResponse"];
            /** Sequence */
            sequence: number;
            /** Session Id */
            session_id: string;
            /** Source */
            source: string;
        };
        /** OwnershipRequestObservation */
        OwnershipRequestObservation: {
            /**
             * Action
             * @enum {string}
             */
            action: "acquire" | "divest";
            /** Attribute Key */
            attribute_key: string;
            /** Federation Id */
            federation_id: string;
            /** Identity Namespace */
            identity_namespace: string;
            /**
             * @description discriminator enum property added by openapi-typescript
             * @enum {string}
             */
            kind: "request";
            /** Object Id */
            object_id: string;
            /** Reason */
            reason?: string | null;
            /** Request Id */
            request_id: string;
            requester: components["schemas"]["ParticipantIdentityResponse"];
            /** Session Id */
            session_id: string;
        };
        /** OwnershipRequestReportResponse */
        OwnershipRequestReportResponse: {
            /**
             * Action
             * @enum {string}
             */
            action: "acquire" | "divest";
            /** Attribute Key */
            attribute_key: string;
            /** Federation Id */
            federation_id: string;
            /** Identity Namespace */
            identity_namespace: string;
            /**
             * @description discriminator enum property added by openapi-typescript
             * @enum {string}
             */
            kind: "request";
            /** Object Id */
            object_id: string;
            /** Observed At */
            observed_at: number;
            /** Reason */
            reason?: string | null;
            /** Request Id */
            request_id: string;
            requester: components["schemas"]["ParticipantIdentityResponse"];
            /** Sequence */
            sequence: number;
            /** Session Id */
            session_id: string;
            /** Source */
            source: string;
        };
        /** OwnershipResponse */
        OwnershipResponse: {
            /**
             * Basis
             * @constant
             */
            basis: "explicit-ownership-observations";
            /** Capacity */
            capacity: number;
            /** Dropped Requests */
            dropped_requests: number;
            /** Dropped States */
            dropped_states: number;
            /** Dropped Transitions */
            dropped_transitions: number;
            /** History Capacity */
            history_capacity: number;
            /** Request Capacity */
            request_capacity: number;
            /** Requests */
            requests: components["schemas"]["OwnershipTicketResponse"][];
            /** States */
            states: components["schemas"]["OwnershipStateReportResponse"][];
            /** Transitions */
            transitions: (components["schemas"]["OwnershipStateReportResponse"] | components["schemas"]["OwnershipRequestReportResponse"] | components["schemas"]["OwnershipOutcomeReportResponse"])[];
            /** Unmatched Outcomes */
            unmatched_outcomes: number;
        };
        /** OwnershipStateObservation */
        OwnershipStateObservation: {
            /** Attribute Key */
            attribute_key: string;
            /** Federation Id */
            federation_id: string;
            /** Identity Namespace */
            identity_namespace: string;
            /**
             * @description discriminator enum property added by openapi-typescript
             * @enum {string}
             */
            kind: "state";
            /** Object Id */
            object_id: string;
            owner?: components["schemas"]["ParticipantIdentityResponse"] | null;
            /**
             * Owner State
             * @enum {string}
             */
            owner_state: "known" | "unowned" | "rti" | "unknown";
            /** Reason */
            reason?: string | null;
            /** Reported Owner Handle */
            reported_owner_handle?: string | null;
            /** Reported Owner Name */
            reported_owner_name?: string | null;
            /** Session Id */
            session_id: string;
            /**
             * Transition
             * @enum {string}
             */
            transition: "reported" | "acquired" | "divested";
        };
        /** OwnershipStateReportResponse */
        OwnershipStateReportResponse: {
            /** Attribute Key */
            attribute_key: string;
            /** Federation Id */
            federation_id: string;
            /** Identity Namespace */
            identity_namespace: string;
            /**
             * @description discriminator enum property added by openapi-typescript
             * @enum {string}
             */
            kind: "state";
            /** Object Id */
            object_id: string;
            /** Observed At */
            observed_at: number;
            owner?: components["schemas"]["ParticipantIdentityResponse"] | null;
            /**
             * Owner State
             * @enum {string}
             */
            owner_state: "known" | "unowned" | "rti" | "unknown";
            /** Reason */
            reason?: string | null;
            /** Reported Owner Handle */
            reported_owner_handle?: string | null;
            /** Reported Owner Name */
            reported_owner_name?: string | null;
            /** Sequence */
            sequence: number;
            /** Session Id */
            session_id: string;
            /** Source */
            source: string;
            /**
             * Transition
             * @enum {string}
             */
            transition: "reported" | "acquired" | "divested";
        };
        /** OwnershipTicketResponse */
        OwnershipTicketResponse: {
            /** Duplicate Requests */
            duplicate_requests: number;
            /** Last Sequence */
            last_sequence: number;
            outcome?: components["schemas"]["OwnershipOutcomeReportResponse"] | null;
            request: components["schemas"]["OwnershipRequestReportResponse"];
            /**
             * Status
             * @enum {string}
             */
            status: "pending" | "completed" | "failed" | "cancelled" | "ambiguous";
        };
        /**
         * ParticipantIdentityResponse
         * @description Opaque IDs identify a participant join; names and handles never do.
         */
        ParticipantIdentityResponse: {
            /** Display Name */
            display_name?: string | null;
            /** Handle */
            handle?: string | null;
            /** Membership Id */
            membership_id: string;
            /** Participant Id */
            participant_id: string;
        };
        /** ParticipantReferenceResponse */
        ParticipantReferenceResponse: {
            /** Display Name */
            display_name?: string | null;
            /** Handle */
            handle?: string | null;
            identity?: components["schemas"]["ParticipantIdentityResponse"] | null;
        };
        /** PersistenceObservation */
        PersistenceObservation: {
            /** Federation Id */
            federation_id: string;
            /** Identity Namespace */
            identity_namespace: string;
            /**
             * @description discriminator enum property added by openapi-typescript
             * @enum {string}
             */
            kind: "restore" | "save";
            /** Label */
            label: string;
            /** Operation Id */
            operation_id: string;
            /** Participants */
            participants?: components["schemas"]["CoordinationParticipantResponse"][] | null;
            /**
             * Phase
             * @enum {string}
             */
            phase: "requested" | "begun" | "completed" | "failed";
            /** Reason */
            reason?: string | null;
            /**
             * Scope
             * @enum {string}
             */
            scope: "local" | "federation";
            /** Session Id */
            session_id: string;
            subject?: components["schemas"]["ParticipantIdentityResponse"] | null;
        };
        /** RecordedWindowAnalysisRequest */
        RecordedWindowAnalysisRequest: {
            /**
             * Bucket Count
             * @default 30
             */
            bucket_count: number;
            /** Captured At */
            captured_at: number;
            window: components["schemas"]["AnalyticsWindow"];
            /**
             * Window Seconds
             * @default 300
             */
            window_seconds: number;
        };
        /** RecordingAnalysisRequest */
        RecordingAnalysisRequest: {
            /**
             * Bucket Count
             * @default 30
             */
            bucket_count: number;
            frame: components["schemas"]["RecordingFrame-Input"];
            /**
             * Window Seconds
             * @default 300
             */
            window_seconds: number;
        };
        /** RecordingDeleteEdit */
        RecordingDeleteEdit: {
            /**
             * @description discriminator enum property added by openapi-typescript
             * @enum {string}
             */
            op: "delete";
            /** Path */
            path: (string | number)[];
        };
        /** RecordingDeltaFrame */
        "RecordingDeltaFrame-Input": {
            /** Arrivals */
            arrivals: components["schemas"]["EventEnvelope-Input"][];
            /** Captured At */
            captured_at: number;
            /**
             * Change
             * @enum {string}
             */
            change: "start" | "events" | "metadata" | "clear" | "checkpoint" | "stop";
            /** Edits */
            edits: (components["schemas"]["RecordingSetEdit-Input"] | components["schemas"]["RecordingDeleteEdit"] | components["schemas"]["RecordingSpliceEdit-Input"])[];
            /** Index */
            index: number;
            /** Offset Seconds */
            offset_seconds: number;
            /** State Bytes */
            state_bytes: number;
        };
        /** RecordingDeltaFrame */
        "RecordingDeltaFrame-Output": {
            /** Arrivals */
            arrivals: components["schemas"]["EventEnvelope-Output"][];
            /** Captured At */
            captured_at: number;
            /**
             * Change
             * @enum {string}
             */
            change: "start" | "events" | "metadata" | "clear" | "checkpoint" | "stop";
            /** Edits */
            edits: (components["schemas"]["RecordingSetEdit-Output"] | components["schemas"]["RecordingDeleteEdit"] | components["schemas"]["RecordingSpliceEdit-Output"])[];
            /** Index */
            index: number;
            /** Offset Seconds */
            offset_seconds: number;
            /** State Bytes */
            state_bytes: number;
        };
        /** RecordingDocument */
        "RecordingDocument-Input": {
            /** Captured Bytes */
            captured_bytes: number;
            contract: components["schemas"]["ContractResponse-Input"];
            /** Duration Seconds */
            duration_seconds: number;
            /** Frames */
            frames: components["schemas"]["RecordingFrame-Input"][];
            /** Label */
            label: string;
            /** Max Bytes */
            max_bytes: number;
            /** Max Frames */
            max_frames: number;
            /** Recording Id */
            recording_id: string;
            /**
             * @description discriminator enum property added by openapi-typescript
             * @enum {string}
             */
            recording_version: "umbra-observer-recording-v1";
            /** Started At */
            started_at: number;
            /**
             * Stop Reason
             * @enum {string}
             */
            stop_reason: "user" | "frame-limit" | "byte-limit" | "invalid-state" | "backend-stopped" | "state-limit" | "decode-limit" | "edit-limit";
            /** Stopped At */
            stopped_at: number;
        };
        /** RecordingDocument */
        "RecordingDocument-Output": {
            /** Captured Bytes */
            captured_bytes: number;
            contract: components["schemas"]["ContractResponse-Output"];
            /** Duration Seconds */
            duration_seconds: number;
            /** Frames */
            frames: components["schemas"]["RecordingFrame-Output"][];
            /** Label */
            label: string;
            /** Max Bytes */
            max_bytes: number;
            /** Max Frames */
            max_frames: number;
            /** Recording Id */
            recording_id: string;
            /**
             * @description discriminator enum property added by openapi-typescript
             * @enum {string}
             */
            recording_version: "umbra-observer-recording-v1";
            /** Started At */
            started_at: number;
            /**
             * Stop Reason
             * @enum {string}
             */
            stop_reason: "user" | "frame-limit" | "byte-limit" | "invalid-state" | "backend-stopped" | "state-limit" | "decode-limit" | "edit-limit";
            /** Stopped At */
            stopped_at: number;
        };
        /** RecordingFrame */
        "RecordingFrame-Input": {
            /** Arrivals */
            arrivals: components["schemas"]["EventEnvelope-Input"][];
            /** Captured At */
            captured_at: number;
            /**
             * Change
             * @enum {string}
             */
            change: "start" | "events" | "metadata" | "clear" | "checkpoint" | "stop";
            /** Index */
            index: number;
            /** Offset Seconds */
            offset_seconds: number;
            state: components["schemas"]["StateResponse-Input"];
        };
        /** RecordingFrame */
        "RecordingFrame-Output": {
            /** Arrivals */
            arrivals: components["schemas"]["EventEnvelope-Output"][];
            /** Captured At */
            captured_at: number;
            /**
             * Change
             * @enum {string}
             */
            change: "start" | "events" | "metadata" | "clear" | "checkpoint" | "stop";
            /** Index */
            index: number;
            /** Offset Seconds */
            offset_seconds: number;
            state: components["schemas"]["StateResponse-Output"];
        };
        /** RecordingSetEdit */
        "RecordingSetEdit-Input": {
            /**
             * @description discriminator enum property added by openapi-typescript
             * @enum {string}
             */
            op: "set";
            /** Path */
            path: (string | number)[];
            value: components["schemas"]["JsonValue-Input"];
        };
        /** RecordingSetEdit */
        "RecordingSetEdit-Output": {
            /**
             * @description discriminator enum property added by openapi-typescript
             * @enum {string}
             */
            op: "set";
            /** Path */
            path: (string | number)[];
            value: components["schemas"]["JsonValue-Output"];
        };
        /** RecordingSpliceEdit */
        "RecordingSpliceEdit-Input": {
            /** Delete Count */
            delete_count: number;
            /** Index */
            index: number;
            /** Items */
            items: components["schemas"]["JsonValue-Input"][];
            /**
             * @description discriminator enum property added by openapi-typescript
             * @enum {string}
             */
            op: "splice";
            /** Path */
            path: (string | number)[];
        };
        /** RecordingSpliceEdit */
        "RecordingSpliceEdit-Output": {
            /** Delete Count */
            delete_count: number;
            /** Index */
            index: number;
            /** Items */
            items: components["schemas"]["JsonValue-Output"][];
            /**
             * @description discriminator enum property added by openapi-typescript
             * @enum {string}
             */
            op: "splice";
            /** Path */
            path: (string | number)[];
        };
        /** RecordingStartRequest */
        RecordingStartRequest: {
            /** Label */
            label: string;
            /**
             * Max Bytes
             * @default 16777216
             */
            max_bytes: number;
            /**
             * Max Frames
             * @default 600
             */
            max_frames: number;
            /** Replace Id */
            replace_id?: string | null;
            /**
             * Storage Format
             * @default full-v1
             * @enum {string}
             */
            storage_format: "full-v1" | "compact-v2";
        };
        /** RecordingStatusResponse */
        RecordingStatusResponse: {
            /**
             * Contract Version
             * @default umbra-observer-contract-v1
             * @constant
             */
            contract_version: "umbra-observer-contract-v1";
            recording: components["schemas"]["CaptureStatus"] | null;
            /**
             * Scope
             * @default backend-process
             * @constant
             */
            scope: "backend-process";
        };
        /** RecordingTargetRequest */
        RecordingTargetRequest: {
            /** Recording Id */
            recording_id: string;
        };
        /** RegionAssociationObservation */
        RegionAssociationObservation: {
            /** Association Id */
            association_id: string;
            /** Attribute Keys */
            attribute_keys?: string[] | null;
            /**
             * Direction
             * @enum {string}
             */
            direction: "publication" | "subscription" | "update" | "send" | "unknown";
            /**
             * Evidence
             * @enum {string}
             */
            evidence: "declared" | "observed" | "unknown";
            /** Federation Id */
            federation_id: string;
            /** Identity Namespace */
            identity_namespace: string;
            /**
             * @description discriminator enum property added by openapi-typescript
             * @enum {string}
             */
            kind: "association";
            /** Reason */
            reason?: string | null;
            /** Region Ids */
            region_ids?: string[] | null;
            /** Session Id */
            session_id: string;
            /**
             * Status
             * @enum {string}
             */
            status: "active" | "ended" | "unknown";
            subject?: components["schemas"]["ParticipantIdentityResponse"] | null;
            /** Target Id */
            target_id: string;
            /**
             * Target Kind
             * @enum {string}
             */
            target_kind: "object" | "interaction";
        };
        /** RegionAssociationReportResponse */
        RegionAssociationReportResponse: {
            /** Association Id */
            association_id: string;
            /** Attribute Keys */
            attribute_keys?: string[] | null;
            /**
             * Direction
             * @enum {string}
             */
            direction: "publication" | "subscription" | "update" | "send" | "unknown";
            /**
             * Evidence
             * @enum {string}
             */
            evidence: "declared" | "observed" | "unknown";
            /** Federation Id */
            federation_id: string;
            /** Identity Namespace */
            identity_namespace: string;
            /**
             * @description discriminator enum property added by openapi-typescript
             * @enum {string}
             */
            kind: "association";
            /** Observed At */
            observed_at: number;
            /** Reason */
            reason?: string | null;
            /** Region Ids */
            region_ids?: string[] | null;
            /** Sequence */
            sequence: number;
            /** Session Id */
            session_id: string;
            /** Source */
            source: string;
            /**
             * Status
             * @enum {string}
             */
            status: "active" | "ended" | "unknown";
            subject?: components["schemas"]["ParticipantIdentityResponse"] | null;
            /** Target Id */
            target_id: string;
            /**
             * Target Kind
             * @enum {string}
             */
            target_kind: "object" | "interaction";
        };
        /** RegionExtentResponse */
        RegionExtentResponse: {
            /** Dimensions */
            dimensions: components["schemas"]["DimensionRangeResponse"][];
            /** Extent Id */
            extent_id: string;
        };
        /** RegionRoutingObservation */
        RegionRoutingObservation: {
            /**
             * Evidence
             * @enum {string}
             */
            evidence: "declared" | "observed" | "unknown";
            /** Federation Id */
            federation_id: string;
            /** Identity Namespace */
            identity_namespace: string;
            /**
             * @description discriminator enum property added by openapi-typescript
             * @enum {string}
             */
            kind: "routing";
            producer?: components["schemas"]["ParticipantIdentityResponse"] | null;
            /** Reason */
            reason?: string | null;
            recipient?: components["schemas"]["ParticipantIdentityResponse"] | null;
            /** Region Ids */
            region_ids?: string[] | null;
            /** Routing Id */
            routing_id: string;
            /** Session Id */
            session_id: string;
            /**
             * Status
             * @enum {string}
             */
            status: "active" | "ended" | "unknown";
            subject?: components["schemas"]["ParticipantIdentityResponse"] | null;
            /** Target Id */
            target_id?: string | null;
            /** Target Kind */
            target_kind?: ("object" | "interaction") | null;
        };
        /** RegionRoutingReportResponse */
        RegionRoutingReportResponse: {
            /**
             * Evidence
             * @enum {string}
             */
            evidence: "declared" | "observed" | "unknown";
            /** Federation Id */
            federation_id: string;
            /** Identity Namespace */
            identity_namespace: string;
            /**
             * @description discriminator enum property added by openapi-typescript
             * @enum {string}
             */
            kind: "routing";
            /** Observed At */
            observed_at: number;
            producer?: components["schemas"]["ParticipantIdentityResponse"] | null;
            /** Reason */
            reason?: string | null;
            recipient?: components["schemas"]["ParticipantIdentityResponse"] | null;
            /** Region Ids */
            region_ids?: string[] | null;
            /** Routing Id */
            routing_id: string;
            /** Sequence */
            sequence: number;
            /** Session Id */
            session_id: string;
            /** Source */
            source: string;
            /**
             * Status
             * @enum {string}
             */
            status: "active" | "ended" | "unknown";
            subject?: components["schemas"]["ParticipantIdentityResponse"] | null;
            /** Target Id */
            target_id?: string | null;
            /** Target Kind */
            target_kind?: ("object" | "interaction") | null;
        };
        /** RegionsResponse */
        RegionsResponse: {
            /**
             * Basis
             * @constant
             */
            basis: "explicit-region-observations";
            /** Capacity */
            capacity: number;
            /** Dropped Snapshots */
            dropped_snapshots: number;
            /** Dropped Transitions */
            dropped_transitions: number;
            /** History Capacity */
            history_capacity: number;
            /** Snapshots */
            snapshots: (components["schemas"]["RegionStateReportResponse"] | components["schemas"]["RegionAssociationReportResponse"] | components["schemas"]["RegionRoutingReportResponse"])[];
            /** Transitions */
            transitions: (components["schemas"]["RegionStateReportResponse"] | components["schemas"]["RegionAssociationReportResponse"] | components["schemas"]["RegionRoutingReportResponse"])[];
        };
        /** RegionStateObservation */
        RegionStateObservation: {
            /**
             * Evidence
             * @enum {string}
             */
            evidence: "declared" | "observed" | "unknown";
            /** Extents */
            extents?: components["schemas"]["RegionExtentResponse"][] | null;
            /** Federation Id */
            federation_id: string;
            /** Identity Namespace */
            identity_namespace: string;
            /**
             * @description discriminator enum property added by openapi-typescript
             * @enum {string}
             */
            kind: "region";
            /** Reason */
            reason?: string | null;
            /** Region Id */
            region_id: string;
            /** Session Id */
            session_id: string;
            /**
             * Status
             * @enum {string}
             */
            status: "active" | "deleted" | "unknown";
            subject?: components["schemas"]["ParticipantIdentityResponse"] | null;
        };
        /** RegionStateReportResponse */
        RegionStateReportResponse: {
            /**
             * Evidence
             * @enum {string}
             */
            evidence: "declared" | "observed" | "unknown";
            /** Extents */
            extents?: components["schemas"]["RegionExtentResponse"][] | null;
            /** Federation Id */
            federation_id: string;
            /** Identity Namespace */
            identity_namespace: string;
            /**
             * @description discriminator enum property added by openapi-typescript
             * @enum {string}
             */
            kind: "region";
            /** Observed At */
            observed_at: number;
            /** Reason */
            reason?: string | null;
            /** Region Id */
            region_id: string;
            /** Sequence */
            sequence: number;
            /** Session Id */
            session_id: string;
            /** Source */
            source: string;
            /**
             * Status
             * @enum {string}
             */
            status: "active" | "deleted" | "unknown";
            subject?: components["schemas"]["ParticipantIdentityResponse"] | null;
        };
        /** RelationshipObservationInput */
        RelationshipObservationInput: {
            /** Class Key */
            class_key?: string | null;
            /** Class Name */
            class_name?: string | null;
            /**
             * Evidence
             * @enum {string}
             */
            evidence: "declared" | "observed";
            /** Federation Id */
            federation_id: string;
            /** Identity Namespace */
            identity_namespace: string;
            /**
             * Kind
             * @enum {string}
             */
            kind: "object" | "interaction";
            producer: components["schemas"]["ParticipantIdentityResponse"];
            /** Reason */
            reason?: string | null;
            recipient: components["schemas"]["ParticipantIdentityResponse"];
            /** Relationship Id */
            relationship_id: string;
            /** Session Id */
            session_id: string;
            /**
             * Status
             * @enum {string}
             */
            status: "active" | "ended" | "unknown";
            /**
             * Viewpoint
             * @enum {string}
             */
            viewpoint: "observer" | "producer" | "recipient" | "unknown";
        };
        /** RelationshipSnapshotResponse */
        RelationshipSnapshotResponse: {
            /** Class Key */
            class_key?: string | null;
            /** Class Name */
            class_name?: string | null;
            /**
             * Evidence
             * @enum {string}
             */
            evidence: "declared" | "observed";
            /** Federation Id */
            federation_id: string;
            /** Identity Namespace */
            identity_namespace: string;
            /**
             * Kind
             * @enum {string}
             */
            kind: "object" | "interaction";
            /** Observed At */
            observed_at: number;
            producer: components["schemas"]["ParticipantIdentityResponse"];
            /** Reason */
            reason?: string | null;
            recipient: components["schemas"]["ParticipantIdentityResponse"];
            /** Relationship Id */
            relationship_id: string;
            /** Sequence */
            sequence: number;
            /** Session Id */
            session_id: string;
            /** Source */
            source: string;
            /**
             * Status
             * @enum {string}
             */
            status: "active" | "ended" | "unknown";
            /**
             * Viewpoint
             * @enum {string}
             */
            viewpoint: "observer" | "producer" | "recipient" | "unknown";
        };
        /** RelationshipsResponse */
        RelationshipsResponse: {
            /**
             * Basis
             * @constant
             */
            basis: "last-reported-explicit-relationships";
            /** Capacity */
            capacity: number;
            /** Dropped Relationships */
            dropped_relationships: number;
            /** Snapshots */
            snapshots: components["schemas"]["RelationshipSnapshotResponse"][];
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
        "StateResponse-Input": {
            /** Adapters */
            adapters?: components["schemas"]["AdapterResponse"][];
            /** Capabilities */
            capabilities?: {
                [key: string]: components["schemas"]["CapabilityResponse-Input"];
            };
            /** Context */
            context?: {
                [key: string]: components["schemas"]["JsonValue-Input"];
            };
            /** Contract Version */
            contract_version: string;
            /** Federate Roster */
            federate_roster?: components["schemas"]["FederateInspectionResponse"][];
            inspectors: components["schemas"]["InspectorsResponse-Input"];
            live_metrics: components["schemas"]["LiveMetricsResponse"];
            /** Normalized Events */
            normalized_events?: components["schemas"]["EventEnvelope-Input"][] | null;
            retention: components["schemas"]["RetentionResponse"];
            /** Schema Version */
            schema_version: string;
            /** Status */
            status: string;
            storage?: components["schemas"]["StorageCoverageResponse"] | null;
        };
        /** StateResponse */
        "StateResponse-Output": {
            /** Adapters */
            adapters?: components["schemas"]["AdapterResponse"][];
            /** Capabilities */
            capabilities?: {
                [key: string]: components["schemas"]["CapabilityResponse-Output"];
            };
            /** Context */
            context?: {
                [key: string]: components["schemas"]["JsonValue-Output"];
            };
            /** Contract Version */
            contract_version: string;
            /** Federate Roster */
            federate_roster?: components["schemas"]["FederateInspectionResponse"][];
            inspectors: components["schemas"]["InspectorsResponse-Output"];
            live_metrics: components["schemas"]["LiveMetricsResponse"];
            /** Normalized Events */
            normalized_events?: components["schemas"]["EventEnvelope-Output"][] | null;
            retention: components["schemas"]["RetentionResponse"];
            /** Schema Version */
            schema_version: string;
            /** Status */
            status: string;
            storage?: components["schemas"]["StorageCoverageResponse"] | null;
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
        /** StorageBudgetResponse */
        StorageBudgetResponse: {
            /** Accounted Bytes */
            accounted_bytes: number;
            /**
             * Basis
             * @constant
             */
            basis: "encoded-json-plus-record-reserve";
            /** Collections */
            collections: components["schemas"]["StorageCollectionResponse"][];
            /** Max Bytes */
            max_bytes: number;
            /** Max Record Bytes */
            max_record_bytes: number;
            /**
             * Policy
             * @enum {string}
             */
            policy: "shared-evidence-lru" | "shared-evidence-lru-with-soft-reservations";
            /** Record Reserve Bytes */
            record_reserve_bytes: number;
            /** Unbudgeted Collections */
            unbudgeted_collections: string[];
        };
        /** StorageCollectionResponse */
        StorageCollectionResponse: {
            /** Accounted Bytes */
            accounted_bytes: number;
            /** Byte Limit Drops */
            byte_limit_drops: number;
            /** Capacity */
            capacity: number;
            /** Collection */
            collection: string;
            /** Count Limit Drops */
            count_limit_drops: number;
            /** Dropped Bytes */
            dropped_bytes: number;
            /** Dropped Records */
            dropped_records: number;
            /** Encoding Drops */
            encoding_drops: number;
            /** Oversized Drops */
            oversized_drops: number;
            /** Retained Records */
            retained_records: number;
            /** Soft Reservation Bytes */
            soft_reservation_bytes?: number | null;
            /** Unit */
            unit: string;
            /** Window Drops */
            window_drops: number;
        };
        /** StorageCoverageResponse */
        StorageCoverageResponse: {
            context: components["schemas"]["StorageBudgetResponse"];
            evidence: components["schemas"]["StorageBudgetResponse"];
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
        /** SynchronizationObservation */
        SynchronizationObservation: {
            /** Federation Id */
            federation_id: string;
            /** Identity Namespace */
            identity_namespace: string;
            /**
             * @description discriminator enum property added by openapi-typescript
             * @enum {string}
             */
            kind: "synchronization";
            /** Label */
            label: string;
            /** Operation Id */
            operation_id: string;
            /** Participants */
            participants?: components["schemas"]["CoordinationParticipantResponse"][] | null;
            /**
             * Phase
             * @enum {string}
             */
            phase: "requested" | "registered" | "registration-failed" | "announced" | "achieved" | "completed" | "failed";
            /** Reason */
            reason?: string | null;
            /**
             * Scope
             * @enum {string}
             */
            scope: "local" | "federation";
            /** Session Id */
            session_id: string;
            subject?: components["schemas"]["ParticipantIdentityResponse"] | null;
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
            /** Dropped Snapshots */
            dropped_snapshots?: number | null;
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
            /** Snapshot Capacity */
            snapshot_capacity?: number | null;
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
            /** Snapshot Retained */
            snapshot_retained?: boolean | null;
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
                    "application/json": components["schemas"]["ContractResponse-Output"];
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
                    "application/json": components["schemas"]["StateResponse-Output"];
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
                    "application/json": components["schemas"]["StateResponse-Output"];
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
    status_api_recordings_get: {
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
                    "application/json": components["schemas"]["RecordingStatusResponse"];
                };
            };
        };
    };
    analyze_api_recordings_analyze_post: {
        parameters: {
            query?: never;
            header?: never;
            path?: never;
            cookie?: never;
        };
        requestBody: {
            content: {
                "application/json": components["schemas"]["RecordingAnalysisRequest"] | components["schemas"]["RecordedWindowAnalysisRequest"];
            };
        };
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
    export_api_recordings_export_get: {
        parameters: {
            query: {
                recording_id: string;
                storage_format?: "full-v1" | "compact-v2";
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
                    "application/json": components["schemas"]["RecordingDocument-Output"] | components["schemas"]["CompactRecordingDocument-Output"];
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
    start_api_recordings_start_post: {
        parameters: {
            query?: never;
            header?: never;
            path?: never;
            cookie?: never;
        };
        requestBody: {
            content: {
                "application/json": components["schemas"]["RecordingStartRequest"];
            };
        };
        responses: {
            /** @description Successful Response */
            200: {
                headers: {
                    [name: string]: unknown;
                };
                content: {
                    "application/json": components["schemas"]["CaptureStatus"];
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
    stop_api_recordings_stop_post: {
        parameters: {
            query?: never;
            header?: never;
            path?: never;
            cookie?: never;
        };
        requestBody: {
            content: {
                "application/json": components["schemas"]["RecordingTargetRequest"];
            };
        };
        responses: {
            /** @description Successful Response */
            200: {
                headers: {
                    [name: string]: unknown;
                };
                content: {
                    "application/json": components["schemas"]["CaptureStatus"];
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
    validated_response_api_recordings_validate_post: {
        parameters: {
            query?: never;
            header?: never;
            path?: never;
            cookie?: never;
        };
        requestBody: {
            content: {
                "application/json": components["schemas"]["RecordingDocument-Input"] | components["schemas"]["CompactRecordingDocument-Input"];
            };
        };
        responses: {
            /** @description Successful Response */
            200: {
                headers: {
                    [name: string]: unknown;
                };
                content: {
                    "application/json": components["schemas"]["RecordingDocument-Output"] | components["schemas"]["CompactRecordingDocument-Output"];
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
                    "application/json": components["schemas"]["StateResponse-Output"];
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
