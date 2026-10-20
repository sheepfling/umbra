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
    "/api/alerts": {
        parameters: {
            query?: never;
            header?: never;
            path?: never;
            cookie?: never;
        };
        /** Alerts */
        get: operations["alerts_api_alerts_get"];
        put?: never;
        post?: never;
        delete?: never;
        options?: never;
        head?: never;
        patch?: never;
        trace?: never;
    };
    "/api/alerts/acknowledge": {
        parameters: {
            query?: never;
            header?: never;
            path?: never;
            cookie?: never;
        };
        get?: never;
        put?: never;
        /** Acknowledge Alert */
        post: operations["acknowledge_alert_api_alerts_acknowledge_post"];
        delete?: never;
        options?: never;
        head?: never;
        patch?: never;
        trace?: never;
    };
    "/api/alerts/configuration": {
        parameters: {
            query?: never;
            header?: never;
            path?: never;
            cookie?: never;
        };
        get?: never;
        /** Configure Alerts */
        put: operations["configure_alerts_api_alerts_configuration_put"];
        post?: never;
        delete?: never;
        options?: never;
        head?: never;
        patch?: never;
        trace?: never;
    };
    "/api/alerts/reset": {
        parameters: {
            query?: never;
            header?: never;
            path?: never;
            cookie?: never;
        };
        get?: never;
        put?: never;
        /** Reset Alert */
        post: operations["reset_alert_api_alerts_reset_post"];
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
    "/api/workspaces/defaults": {
        parameters: {
            query?: never;
            header?: never;
            path?: never;
            cookie?: never;
        };
        /** Defaults */
        get: operations["defaults_api_workspaces_defaults_get"];
        put?: never;
        post?: never;
        delete?: never;
        options?: never;
        head?: never;
        patch?: never;
        trace?: never;
    };
    "/api/workspaces/validate": {
        parameters: {
            query?: never;
            header?: never;
            path?: never;
            cookie?: never;
        };
        get?: never;
        put?: never;
        /** Validate Workspace */
        post: operations["validate_workspace_api_workspaces_validate_post"];
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
        /** AlertAcknowledgeRequest */
        AlertAcknowledgeRequest: {
            /** Evidence Id */
            evidence_id: number;
            /** Expected Instance Id */
            expected_instance_id: string;
        };
        /** AlertCommandProof */
        AlertCommandProof: {
            /** Age Seconds */
            age_seconds: number;
            /** Command Id */
            command_id: string;
            /** Effect Unknown */
            effect_unknown: boolean;
            /** Target Id */
            target_id: string;
        };
        /** AlertConfiguration */
        AlertConfiguration: {
            /**
             * Configuration Version
             * @constant
             */
            configuration_version: "umbra-observer-alerts-v1";
            /** Rules */
            rules: (components["schemas"]["StaleObservationRule"] | components["schemas"]["ErrorCountRule"] | components["schemas"]["DroppedHistoryRule"] | components["schemas"]["PendingCommandRule"])[];
        };
        /** AlertConfigurationUpdate */
        AlertConfigurationUpdate: {
            configuration: components["schemas"]["AlertConfiguration"];
            /** Expected Instance Id */
            expected_instance_id: string;
            /** Expected Revision */
            expected_revision: number;
        };
        /** AlertEvidenceModel */
        AlertEvidenceModel: {
            /** Acknowledged */
            acknowledged: boolean;
            /** Evidence Id */
            evidence_id: number;
            result: components["schemas"]["AlertResultModel"];
            /** Rule */
            rule: components["schemas"]["StaleObservationRule"] | components["schemas"]["ErrorCountRule"] | components["schemas"]["DroppedHistoryRule"] | components["schemas"]["PendingCommandRule"];
        };
        /** AlertReport */
        AlertReport: {
            /**
             * Configuration Version
             * @constant
             */
            configuration_version: "umbra-observer-alerts-v1";
            /**
             * Contract Version
             * @constant
             */
            contract_version: "umbra-observer-contract-v1";
            /**
             * Error Basis
             * @constant
             */
            error_basis: "since-rule-activation-window";
            /**
             * Error Marks Per Rule
             * @constant
             */
            error_marks_per_rule: 512;
            /**
             * Evaluation Basis
             * @constant
             */
            evaluation_basis: "observer-ingress-monotonic";
            /** Evicted Evidence */
            evicted_evidence: number;
            /**
             * Evidence Capacity
             * @constant
             */
            evidence_capacity: 500;
            /** History */
            history: components["schemas"]["AlertEvidenceModel"][];
            /** Instance Id */
            instance_id: string;
            /**
             * Max Configuration Bytes
             * @constant
             */
            max_configuration_bytes: 65536;
            /**
             * Max Rules
             * @constant
             */
            max_rules: 64;
            /**
             * Persistence
             * @constant
             */
            persistence: "server-process";
            /** Results */
            results: components["schemas"]["AlertResultModel"][];
            /** Revision */
            revision: number;
            /** Rules */
            rules: (components["schemas"]["StaleObservationRule"] | components["schemas"]["ErrorCountRule"] | components["schemas"]["DroppedHistoryRule"] | components["schemas"]["PendingCommandRule"])[];
        };
        /** AlertResetRequest */
        AlertResetRequest: {
            /** Expected Instance Id */
            expected_instance_id: string;
            /** Expected Revision */
            expected_revision: number;
            /** Rule Id */
            rule_id: string;
        };
        /** AlertResultModel */
        AlertResultModel: {
            /** Captured At */
            captured_at: number;
            command: components["schemas"]["AlertCommandProof"] | null;
            /**
             * Coverage
             * @enum {string}
             */
            coverage: "complete" | "partial" | "unavailable" | "not-observed";
            history_loss: components["schemas"]["HistoryLossProof"] | null;
            /** Last Sequence */
            last_sequence: number | null;
            /** Metric */
            metric: number | null;
            /** Metric Upper */
            metric_upper: number | null;
            /** Pending Count */
            pending_count: number | null;
            /**
             * Reason
             * @enum {string}
             */
            reason: "disabled" | "not-observed" | "observations-current" | "observations-idle" | "explicit-disconnect" | "explicit-resignation" | "errors-below-threshold" | "errors-threshold" | "error-window-incomplete" | "history-below-threshold" | "history-loss-threshold" | "pending-below-threshold" | "pending-threshold" | "commands-unavailable" | "target-unavailable" | "clock-unusable";
            /** Rule Generation */
            rule_generation: number;
            /** Rule Id */
            rule_id: string;
            /**
             * State
             * @enum {string}
             */
            state: "disabled" | "unknown" | "clear" | "firing" | "ended";
            /** Threshold */
            threshold: number;
            /**
             * Unit
             * @enum {string}
             */
            unit: "seconds" | "observations";
        };
        /** AlertScopeModel */
        AlertScopeModel: {
            /** Federate */
            federate?: string | null;
            /** Federation */
            federation?: string | null;
            perspective?: components["schemas"]["ParticipantPerspectiveResponse"] | null;
            /** Session Id */
            session_id?: string | null;
            /** Source */
            source?: string | null;
        };
        /** AlertViewSettings */
        "AlertViewSettings-Input": {
            /**
             * Search
             * @default
             */
            search: string;
            /**
             * Status
             * @default all
             */
            status: string;
        };
        /** AlertViewSettings */
        "AlertViewSettings-Output": {
            /**
             * Search
             * @default
             */
            search: string;
            /**
             * Status
             * @default all
             */
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
            /** Scoped Perspective */
            scoped_perspective?: string | null;
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
            /**
             * Participant Provenance
             * @default false
             */
            participant_provenance: boolean;
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
            action: "session.connect" | "federate.join" | "federate.resign" | "session.disconnect" | "session.inspect" | "session.recover";
            /**
             * Completion
             * @default service-return
             * @enum {string}
             */
            completion: "service-return" | "inspection-result";
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
            inspection?: components["schemas"]["SessionInspectionResult"] | null;
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
            request: components["schemas"]["ConnectCommand"] | components["schemas"]["JoinCommand"] | components["schemas"]["ResignCommand"] | components["schemas"]["DisconnectCommand"] | components["schemas"]["InspectSessionCommand"] | components["schemas"]["RecoverSessionCommand"];
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
            inspection?: components["schemas"]["SessionInspectionResult"] | null;
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
            version?: components["schemas"]["CommandTargetVersion"] | null;
        };
        /**
         * CommandTargetVersion
         * @description Owned dispatcher registration and revision, never observation identity.
         */
        CommandTargetVersion: {
            /** Instance Id */
            instance_id: string;
            /** Revision */
            revision: number;
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
            expected_version?: components["schemas"]["CommandTargetVersion"] | null;
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
        /** CoordinationViewSettings */
        "CoordinationViewSettings-Input": {
            /**
             * Kind
             * @default all
             */
            kind: string;
            /**
             * Phase
             * @default all
             */
            phase: string;
            /**
             * Scope
             * @default all
             */
            scope: string;
            /**
             * Search
             * @default
             */
            search: string;
            /**
             * Selected
             * @default all
             */
            selected: string;
        };
        /** CoordinationViewSettings */
        "CoordinationViewSettings-Output": {
            /**
             * Kind
             * @default all
             */
            kind: string;
            /**
             * Phase
             * @default all
             */
            phase: string;
            /**
             * Scope
             * @default all
             */
            scope: string;
            /**
             * Search
             * @default
             */
            search: string;
            /**
             * Selected
             * @default all
             */
            selected: string;
        };
        /** DeclarationScopeResponse */
        DeclarationScopeResponse: {
            /** Federate */
            federate: string | null;
            /** Federation */
            federation: string | null;
            perspective?: components["schemas"]["ParticipantPerspectiveResponse"] | null;
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
        /** DeclarationViewSettings */
        "DeclarationViewSettings-Input": {
            /**
             * Direction
             * @default all
             */
            direction: string;
            /**
             * Kind
             * @default all
             */
            kind: string;
            /**
             * Perspective
             * @default all
             */
            perspective: string;
            /**
             * Search
             * @default
             */
            search: string;
            /**
             * Status
             * @default all
             */
            status: string;
        };
        /** DeclarationViewSettings */
        "DeclarationViewSettings-Output": {
            /**
             * Direction
             * @default all
             */
            direction: string;
            /**
             * Kind
             * @default all
             */
            kind: string;
            /**
             * Perspective
             * @default all
             */
            perspective: string;
            /**
             * Search
             * @default
             */
            search: string;
            /**
             * Status
             * @default all
             */
            status: string;
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
            expected_version?: components["schemas"]["CommandTargetVersion"] | null;
            parameters: components["schemas"]["EmptyParameters"];
            /** Request Id */
            request_id: string;
            /** Target Id */
            target_id: string;
        };
        /** DroppedHistoryRule */
        DroppedHistoryRule: {
            /**
             * Enabled
             * @default true
             */
            enabled: boolean;
            /**
             * @description discriminator enum property added by openapi-typescript
             * @enum {string}
             */
            kind: "dropped-history";
            /** Name */
            name: string;
            /** Rule Id */
            rule_id: string;
            /**
             * Severity
             * @default warning
             * @enum {string}
             */
            severity: "info" | "warning" | "critical";
            /** Threshold Count */
            threshold_count: number;
        };
        /**
         * EmptyParameters
         * @description Connection configuration and credentials belong to the server.
         */
        EmptyParameters: Record<string, never>;
        /** ErrorCountRule */
        ErrorCountRule: {
            /**
             * Enabled
             * @default true
             */
            enabled: boolean;
            /**
             * @description discriminator enum property added by openapi-typescript
             * @enum {string}
             */
            kind: "errors";
            /** Name */
            name: string;
            /** Rule Id */
            rule_id: string;
            scope?: components["schemas"]["AlertScopeModel"];
            /**
             * Severity
             * @default warning
             * @enum {string}
             */
            severity: "info" | "warning" | "critical";
            /** Threshold Count */
            threshold_count: number;
            /**
             * Window Seconds
             * @default 300
             */
            window_seconds: number;
        };
        /** ErrorMessage */
        ErrorMessage: {
            /** Message */
            message: string;
            /**
             * Type
             * @constant
             */
            type: "error";
        };
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
            /** Federation */
            federation?: string | null;
            /** Fom Observation */
            fom_observation?: (components["schemas"]["FomObjectObservation"] | components["schemas"]["FomInteractionObservation"] | components["schemas"]["FomDatatypeObservation"] | components["schemas"]["FomDimensionObservation"]) | null;
            interaction_flow?: components["schemas"]["InteractionFlowObservationInput"] | null;
            /** Logical Time */
            logical_time?: number | null;
            /** Mom Observation */
            mom_observation?: (components["schemas"]["MomMembersObservation"] | components["schemas"]["MomServiceObservation"] | components["schemas"]["MomCounterObservation"]) | null;
            object_context?: components["schemas"]["ObjectContextInput"] | null;
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
            participant_attribution?: components["schemas"]["ParticipantAttributionInput"] | null;
            /** Participant Name */
            participant_name?: string | null;
            /** Region Observation */
            region_observation?: (components["schemas"]["RegionStateObservation"] | components["schemas"]["RegionAssociationObservation"] | components["schemas"]["RegionRoutingObservation"]) | null;
            relationship?: components["schemas"]["RelationshipObservationInput"] | null;
            /** Sequence */
            sequence: number;
            /** Session Id */
            session_id?: string | null;
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
            /** Federation */
            federation?: string | null;
            /** Fom Observation */
            fom_observation?: (components["schemas"]["FomObjectObservation"] | components["schemas"]["FomInteractionObservation"] | components["schemas"]["FomDatatypeObservation"] | components["schemas"]["FomDimensionObservation"]) | null;
            interaction_flow?: components["schemas"]["InteractionFlowObservationInput"] | null;
            /** Logical Time */
            logical_time?: number | null;
            /** Mom Observation */
            mom_observation?: (components["schemas"]["MomMembersObservation"] | components["schemas"]["MomServiceObservation"] | components["schemas"]["MomCounterObservation"]) | null;
            object_context?: components["schemas"]["ObjectContextInput"] | null;
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
            participant_attribution?: components["schemas"]["ParticipantAttributionInput"] | null;
            /** Participant Name */
            participant_name?: string | null;
            /** Region Observation */
            region_observation?: (components["schemas"]["RegionStateObservation"] | components["schemas"]["RegionAssociationObservation"] | components["schemas"]["RegionRoutingObservation"]) | null;
            relationship?: components["schemas"]["RelationshipObservationInput"] | null;
            /** Sequence */
            sequence: number;
            /** Session Id */
            session_id?: string | null;
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
            object_context?: components["schemas"]["ObjectContextInput"] | null;
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
            participant_attribution?: components["schemas"]["ParticipantAttributionInput"] | null;
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
            /** Session Id */
            session_id?: string | null;
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
            observer_lifetime?: components["schemas"]["ObserverLifetimeResponse"] | null;
            /**
             * Reset
             * @default false
             */
            reset: boolean;
            /** Retained Events */
            retained_events: number;
        };
        /** EventsMessage */
        EventsMessage: {
            /** @default null */
            cursor: components["schemas"]["StreamCursorResponse"] | null;
            /** Events */
            events?: components["schemas"]["EventEnvelope-Output"][];
            state: components["schemas"]["StateResponse-Output"];
            /**
             * Type
             * @constant
             */
            type: "events";
        };
        /** EventViewSettings */
        "EventViewSettings-Input": {
            /**
             * Family
             * @default
             */
            family: string;
            /**
             * Federate
             * @default
             */
            federate: string;
            /**
             * Search
             * @default
             */
            search: string;
            /**
             * Source
             * @default
             */
            source: string;
            /**
             * Type
             * @default
             */
            type: string;
        };
        /** EventViewSettings */
        "EventViewSettings-Output": {
            /**
             * Family
             * @default
             */
            family: string;
            /**
             * Federate
             * @default
             */
            federate: string;
            /**
             * Search
             * @default
             */
            search: string;
            /**
             * Source
             * @default
             */
            source: string;
            /**
             * Type
             * @default
             */
            type: string;
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
            perspective?: components["schemas"]["ParticipantPerspectiveResponse"] | null;
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
            perspective?: components["schemas"]["ParticipantPerspectiveResponse"] | null;
            /** Role */
            role?: string | null;
            /** Source */
            source: string;
            /** Status */
            status: string;
        };
        /** FederateListSettings */
        "FederateListSettings-Input": {
            /**
             * Basis
             * @default all
             * @enum {string}
             */
            basis: "all" | "explicit-join" | "legacy-provenance" | "legacy-name";
            /**
             * Search
             * @default
             */
            search: string;
        };
        /** FederateListSettings */
        "FederateListSettings-Output": {
            /**
             * Basis
             * @default all
             * @enum {string}
             */
            basis: "all" | "explicit-join" | "legacy-provenance" | "legacy-name";
            /**
             * Search
             * @default
             */
            search: string;
        };
        /** FederateWorkspaceSettings */
        "FederateWorkspaceSettings-Input": {
            declarations?: components["schemas"]["DeclarationViewSettings-Input"];
            events?: components["schemas"]["EventViewSettings-Input"];
            /** Participant */
            participant: string;
            time?: components["schemas"]["TimeViewSettings-Input"];
        };
        /** FederateWorkspaceSettings */
        "FederateWorkspaceSettings-Output": {
            declarations: components["schemas"]["DeclarationViewSettings-Output"];
            events: components["schemas"]["EventViewSettings-Output"];
            /** Participant */
            participant: string;
            time: components["schemas"]["TimeViewSettings-Output"];
        };
        /** FederationViewSettings */
        "FederationViewSettings-Input": {
            /**
             * Distribution
             * @default event_types
             * @enum {string}
             */
            distribution: "event_types" | "sources" | "families";
        };
        /** FederationViewSettings */
        "FederationViewSettings-Output": {
            /**
             * Distribution
             * @default event_types
             * @enum {string}
             */
            distribution: "event_types" | "sources" | "families";
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
        /** FomViewSettings */
        "FomViewSettings-Input": {
            /**
             * Kind
             * @default all
             */
            kind: string;
            /**
             * Model
             * @default all
             */
            model: string;
            /**
             * Search
             * @default
             */
            search: string;
            /** Selected */
            selected?: string | null;
            /**
             * Status
             * @default all
             */
            status: string;
        };
        /** FomViewSettings */
        "FomViewSettings-Output": {
            /**
             * Kind
             * @default all
             */
            kind: string;
            /**
             * Model
             * @default all
             */
            model: string;
            /**
             * Search
             * @default
             */
            search: string;
            /** Selected */
            selected: string | null;
            /**
             * Status
             * @default all
             */
            status: string;
        };
        /** GapMessage */
        GapMessage: {
            /** @default null */
            observer_lifetime: components["schemas"]["ObserverLifetimeResponse"] | null;
            retention: components["schemas"]["RetentionGapResponse"];
            /**
             * Type
             * @constant
             */
            type: "gap";
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
        /** HeartbeatMessage */
        HeartbeatMessage: {
            /** @default null */
            cursor: components["schemas"]["StreamCursorResponse"] | null;
            /**
             * Type
             * @constant
             */
            type: "heartbeat";
        };
        /** HistoryLossProof */
        HistoryLossProof: {
            /** Baseline */
            baseline: number;
            /** Current */
            current: number;
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
        /** InspectSessionCommand */
        InspectSessionCommand: {
            /**
             * @description discriminator enum property added by openapi-typescript
             * @enum {string}
             */
            action: "session.inspect";
            /**
             * Confirmed
             * @default false
             */
            confirmed: boolean;
            expected_version: components["schemas"]["CommandTargetVersion"];
            parameters: components["schemas"]["EmptyParameters"];
            /** Request Id */
            request_id: string;
            /** Target Id */
            target_id: string;
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
            perspective?: components["schemas"]["ParticipantPerspectiveResponse"] | null;
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
            perspective?: components["schemas"]["ParticipantPerspectiveResponse"] | null;
            /** Sequence */
            sequence: number;
            /** Source */
            source: string;
            tag?: components["schemas"]["JsonValue-Output"];
        };
        /** InteractionViewSettings */
        "InteractionViewSettings-Input": {
            /**
             * Order
             * @default all
             */
            order: string;
            /**
             * Search
             * @default
             */
            search: string;
            /**
             * Status
             * @default all
             */
            status: string;
        };
        /** InteractionViewSettings */
        "InteractionViewSettings-Output": {
            /**
             * Order
             * @default all
             */
            order: string;
            /**
             * Search
             * @default
             */
            search: string;
            /**
             * Status
             * @default all
             */
            status: string;
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
            expected_version?: components["schemas"]["CommandTargetVersion"] | null;
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
        /** MomViewSettings */
        "MomViewSettings-Input": {
            /** Counter */
            counter?: string | null;
            /**
             * Scope
             * @default all
             */
            scope: string;
            /**
             * Search
             * @default
             */
            search: string;
            /**
             * Service Status
             * @default all
             */
            service_status: string;
        };
        /** MomViewSettings */
        "MomViewSettings-Output": {
            /** Counter */
            counter: string | null;
            /**
             * Scope
             * @default all
             */
            scope: string;
            /**
             * Search
             * @default
             */
            search: string;
            /**
             * Service Status
             * @default all
             */
            service_status: string;
        };
        /** ObjectAttributeChangeResponse */
        "ObjectAttributeChangeResponse-Input": {
            /** Attribute */
            attribute: string;
            /** Federate */
            federate: string | null;
            /** Generation */
            generation: number;
            object_context?: components["schemas"]["ObjectContextInput"] | null;
            /** Object Key */
            object_key: string;
            /** Observation Key */
            observation_key?: string | null;
            /** Observed At */
            observed_at: number;
            perspective?: components["schemas"]["ParticipantPerspectiveResponse"] | null;
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
            object_context?: components["schemas"]["ObjectContextInput"] | null;
            /** Object Key */
            object_key: string;
            /** Observation Key */
            observation_key?: string | null;
            /** Observed At */
            observed_at: number;
            perspective?: components["schemas"]["ParticipantPerspectiveResponse"] | null;
            /** Previous Known */
            previous_known: boolean;
            previous_value: components["schemas"]["JsonValue-Output"];
            /** Sequence */
            sequence: number;
            /** Source */
            source: string;
            value: components["schemas"]["JsonValue-Output"];
        };
        /** ObjectChartChoice */
        "ObjectChartChoice-Input": {
            /** Attribute */
            attribute: string;
            /** Object Key */
            object_key: string;
            /** Scope */
            scope?: string | null;
        };
        /** ObjectChartChoice */
        "ObjectChartChoice-Output": {
            /** Attribute */
            attribute: string;
            /** Object Key */
            object_key: string;
            /** Scope */
            scope: string | null;
        };
        /** ObjectContextInput */
        ObjectContextInput: {
            /** Federation Id */
            federation_id: string;
            /** Identity Namespace */
            identity_namespace: string;
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
            object_context?: components["schemas"]["ObjectContextInput"] | null;
            /** Object Handle */
            object_handle?: string | null;
            /** Object Key */
            object_key: string;
            /** Object Name */
            object_name: string;
            /** Observation Key */
            observation_key?: string | null;
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
            object_context?: components["schemas"]["ObjectContextInput"] | null;
            /** Object Handle */
            object_handle?: string | null;
            /** Object Key */
            object_key: string;
            /** Object Name */
            object_name: string;
            /** Observation Key */
            observation_key?: string | null;
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
            perspective?: components["schemas"]["ParticipantPerspectiveResponse"] | null;
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
            perspective?: components["schemas"]["ParticipantPerspectiveResponse"] | null;
        };
        /** ObjectViewSettings */
        "ObjectViewSettings-Input": {
            /** Scope */
            scope?: string | null;
            /**
             * Search
             * @default
             */
            search: string;
            /**
             * Selected
             * @default
             */
            selected: string;
            /**
             * Status
             * @default all
             */
            status: string;
        };
        /** ObjectViewSettings */
        "ObjectViewSettings-Output": {
            /** Scope */
            scope: string | null;
            /**
             * Search
             * @default
             */
            search: string;
            /**
             * Selected
             * @default
             */
            selected: string;
            /**
             * Status
             * @default all
             */
            status: string;
        };
        /**
         * ObserverLifetimeResponse
         * @description Storage incarnation and atomic mutation cut; not provider authority.
         */
        ObserverLifetimeResponse: {
            /** Instance Id */
            instance_id: string;
            /** Revision */
            revision: number;
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
        /** OwnershipViewSettings */
        "OwnershipViewSettings-Input": {
            /**
             * Owner State
             * @default all
             */
            owner_state: string;
            /**
             * Request Status
             * @default all
             */
            request_status: string;
            /**
             * Scope
             * @default all
             */
            scope: string;
            /**
             * Search
             * @default
             */
            search: string;
        };
        /** OwnershipViewSettings */
        "OwnershipViewSettings-Output": {
            /**
             * Owner State
             * @default all
             */
            owner_state: string;
            /**
             * Request Status
             * @default all
             */
            request_status: string;
            /**
             * Scope
             * @default all
             */
            scope: string;
            /**
             * Search
             * @default
             */
            search: string;
        };
        /** ParticipantAttributionInput */
        ParticipantAttributionInput: {
            /** Federation Id */
            federation_id: string;
            /** Identity Namespace */
            identity_namespace: string;
            participant: components["schemas"]["ParticipantIdentityResponse"];
            /** Session Id */
            session_id: string;
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
        /** ParticipantPerspectiveResponse */
        ParticipantPerspectiveResponse: {
            attribution?: components["schemas"]["ParticipantAttributionInput"] | null;
            /**
             * Basis
             * @enum {string}
             */
            basis: "explicit-join" | "legacy-provenance";
            /** Federation */
            federation?: string | null;
            /** Key */
            key: string;
            /** Reported Name */
            reported_name?: string | null;
            /** Session Id */
            session_id?: string | null;
            /** Source */
            source: string;
        };
        /** ParticipantReferenceResponse */
        ParticipantReferenceResponse: {
            /** Display Name */
            display_name?: string | null;
            /** Handle */
            handle?: string | null;
            identity?: components["schemas"]["ParticipantIdentityResponse"] | null;
        };
        /** PendingCommandRule */
        PendingCommandRule: {
            /**
             * Enabled
             * @default true
             */
            enabled: boolean;
            /**
             * @description discriminator enum property added by openapi-typescript
             * @enum {string}
             */
            kind: "pending-commands";
            /** Name */
            name: string;
            /** Rule Id */
            rule_id: string;
            /**
             * Severity
             * @default warning
             * @enum {string}
             */
            severity: "info" | "warning" | "critical";
            /** Target Id */
            target_id?: string | null;
            /** Threshold Seconds */
            threshold_seconds: number;
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
        /** RecoverSessionCommand */
        RecoverSessionCommand: {
            /**
             * @description discriminator enum property added by openapi-typescript
             * @enum {string}
             */
            action: "session.recover";
            /**
             * Confirmed
             * @default false
             */
            confirmed: boolean;
            expected_version: components["schemas"]["CommandTargetVersion"];
            parameters: components["schemas"]["EmptyParameters"];
            /** Request Id */
            request_id: string;
            /** Target Id */
            target_id: string;
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
        /** RegionViewSettings */
        "RegionViewSettings-Input": {
            /**
             * Evidence
             * @default all
             */
            evidence: string;
            /**
             * Kind
             * @default all
             */
            kind: string;
            /**
             * Search
             * @default
             */
            search: string;
            /** Selected */
            selected?: string | null;
            /**
             * Status
             * @default all
             */
            status: string;
        };
        /** RegionViewSettings */
        "RegionViewSettings-Output": {
            /**
             * Evidence
             * @default all
             */
            evidence: string;
            /**
             * Kind
             * @default all
             */
            kind: string;
            /**
             * Search
             * @default
             */
            search: string;
            /** Selected */
            selected: string | null;
            /**
             * Status
             * @default all
             */
            status: string;
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
            expected_version?: components["schemas"]["CommandTargetVersion"] | null;
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
        /** RetentionGapResponse */
        RetentionGapResponse: {
            /** Cleared Events */
            cleared_events: number;
            /** Dropped Events */
            dropped_events: number;
            /** First Retained Sequence */
            first_retained_sequence: number;
            /** Last Sequence */
            last_sequence: number;
            /**
             * Next Sequence
             * @default null
             */
            next_sequence: number | null;
            /** Retained Events */
            retained_events: number;
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
        /** SessionInspectionResult */
        SessionInspectionResult: {
            /**
             * Basis
             * @default owned-driver-query
             * @constant
             */
            basis: "owned-driver-query";
            /** Checked At */
            checked_at: number;
            observation: components["schemas"]["SessionStateObservation"];
            /** Reconciled */
            reconciled: boolean;
            /** Resolution */
            resolution: string;
        };
        /**
         * SessionStateObservation
         * @description Explicit provider facts about this driver's own connection/membership.
         */
        SessionStateObservation: {
            /**
             * Connection
             * @enum {string}
             */
            connection: "connected" | "disconnected" | "unknown";
            /** Federate */
            federate?: string | null;
            /** Federate Handle */
            federate_handle?: string | null;
            /** Federation */
            federation?: string | null;
            /**
             * Membership
             * @enum {string}
             */
            membership: "joined" | "not-joined" | "unknown";
            /** Reason */
            reason?: string | null;
        };
        /** SnapshotMessage */
        SnapshotMessage: {
            /** @default null */
            cursor: components["schemas"]["StreamCursorResponse"] | null;
            /**
             * Resume
             * @default null
             */
            resume: ("fresh" | "resumed" | "reset" | "gap") | null;
            state: components["schemas"]["StateResponse-Output"];
            /**
             * Type
             * @constant
             */
            type: "snapshot";
        };
        /** StaleObservationRule */
        StaleObservationRule: {
            /**
             * Enabled
             * @default true
             */
            enabled: boolean;
            /**
             * @description discriminator enum property added by openapi-typescript
             * @enum {string}
             */
            kind: "stale-observations";
            /** Name */
            name: string;
            /** Rule Id */
            rule_id: string;
            scope?: components["schemas"]["AlertScopeModel"];
            /**
             * Severity
             * @default warning
             * @enum {string}
             */
            severity: "info" | "warning" | "critical";
            /** Threshold Seconds */
            threshold_seconds: number;
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
            observer_lifetime?: components["schemas"]["ObserverLifetimeResponse"] | null;
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
            observer_lifetime?: components["schemas"]["ObserverLifetimeResponse"] | null;
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
        /** StreamCursorResponse */
        StreamCursorResponse: {
            /** After */
            after: number;
            /** Instance Id */
            instance_id: string;
            /** Revision */
            revision: number;
        };
        /** StreamDescription */
        StreamDescription: {
            /** Message Types */
            message_types: ("snapshot" | "events" | "heartbeat" | "gap" | "error")[];
            /** Messages */
            messages: {
                [key: string]: string;
            };
            /**
             * Resumption
             * @default false
             */
            resumption: boolean;
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
            perspective?: components["schemas"]["ParticipantPerspectiveResponse"] | null;
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
            perspective?: components["schemas"]["ParticipantPerspectiveResponse"] | null;
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
        /** TimeViewSettings */
        "TimeViewSettings-Input": {
            /**
             * Grants Only
             * @default false
             */
            grants_only: boolean;
            /** Selected */
            selected?: string | null;
        };
        /** TimeViewSettings */
        "TimeViewSettings-Output": {
            /**
             * Grants Only
             * @default false
             */
            grants_only: boolean;
            /** Selected */
            selected: string | null;
        };
        /** TopologyViewSettings */
        "TopologyViewSettings-Input": {
            /**
             * Evidence
             * @default all
             */
            evidence: string;
            /**
             * Focused
             * @default all
             */
            focused: string;
            /**
             * Kind
             * @default all
             */
            kind: string;
            /**
             * Search
             * @default
             */
            search: string;
            /**
             * Source
             * @default all
             */
            source: string;
            /**
             * Status
             * @default all
             */
            status: string;
        };
        /** TopologyViewSettings */
        "TopologyViewSettings-Output": {
            /**
             * Evidence
             * @default all
             */
            evidence: string;
            /**
             * Focused
             * @default all
             */
            focused: string;
            /**
             * Kind
             * @default all
             */
            kind: string;
            /**
             * Search
             * @default
             */
            search: string;
            /**
             * Source
             * @default all
             */
            source: string;
            /**
             * Status
             * @default all
             */
            status: string;
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
        /** WorkspaceDefaultsResponse */
        WorkspaceDefaultsResponse: {
            /**
             * Contract Version
             * @default umbra-observer-contract-v1
             * @constant
             */
            contract_version: "umbra-observer-contract-v1";
            library: components["schemas"]["WorkspaceLibrary"];
        };
        /** WorkspaceDocument */
        "WorkspaceDocument-Input": {
            /** Name */
            name: string;
            settings?: components["schemas"]["WorkspaceSettings-Input"];
            /**
             * Workspace Version
             * @constant
             */
            workspace_version: "umbra-observer-workspace-v1";
        };
        /** WorkspaceDocument */
        "WorkspaceDocument-Output": {
            /** Name */
            name: string;
            settings: components["schemas"]["WorkspaceSettings-Output"];
            /**
             * Workspace Version
             * @constant
             */
            workspace_version: "umbra-observer-workspace-v1";
        };
        /** WorkspaceLibrary */
        WorkspaceLibrary: {
            current: components["schemas"]["WorkspaceSettings-Output"];
            /**
             * Library Version
             * @constant
             */
            library_version: "umbra-observer-workspace-library-v1";
            /**
             * Saved
             * @default []
             */
            saved: components["schemas"]["WorkspaceDocument-Output"][];
        };
        /** WorkspaceSettings */
        "WorkspaceSettings-Input": {
            alerts?: components["schemas"]["AlertViewSettings-Input"];
            declarations?: components["schemas"]["DeclarationViewSettings-Input"];
            events?: components["schemas"]["EventViewSettings-Input"];
            federate_declarations?: components["schemas"]["DeclarationViewSettings-Input"];
            federate_events?: components["schemas"]["EventViewSettings-Input"];
            federate_time?: components["schemas"]["TimeViewSettings-Input"];
            /**
             * Federate Views
             * @default []
             */
            federate_views: components["schemas"]["FederateWorkspaceSettings-Input"][];
            federates?: components["schemas"]["FederateListSettings-Input"];
            federation?: components["schemas"]["FederationViewSettings-Input"];
            fom?: components["schemas"]["FomViewSettings-Input"];
            interactions?: components["schemas"]["InteractionViewSettings-Input"];
            mom?: components["schemas"]["MomViewSettings-Input"];
            /**
             * Object Charts
             * @default []
             */
            object_charts: components["schemas"]["ObjectChartChoice-Input"][];
            objects?: components["schemas"]["ObjectViewSettings-Input"];
            ownership?: components["schemas"]["OwnershipViewSettings-Input"];
            persistence?: components["schemas"]["CoordinationViewSettings-Input"];
            regions?: components["schemas"]["RegionViewSettings-Input"];
            /**
             * Selected Federate
             * @default
             */
            selected_federate: string;
            synchronization?: components["schemas"]["CoordinationViewSettings-Input"];
            time?: components["schemas"]["TimeViewSettings-Input"];
            topology?: components["schemas"]["TopologyViewSettings-Input"];
            /**
             * View
             * @default federation
             * @enum {string}
             */
            view: "federation" | "federates" | "objects" | "declarations" | "topology" | "interactions" | "ownership" | "regions" | "mom" | "fom" | "time" | "synchronization" | "persistence" | "events" | "inspectors" | "recordings" | "alerts" | "actions" | "diagnostics";
            /**
             * Window Seconds
             * @default 300
             * @enum {integer}
             */
            window_seconds: 30 | 300 | 900 | 3600;
        };
        /** WorkspaceSettings */
        "WorkspaceSettings-Output": {
            alerts: components["schemas"]["AlertViewSettings-Output"];
            declarations: components["schemas"]["DeclarationViewSettings-Output"];
            events: components["schemas"]["EventViewSettings-Output"];
            federate_declarations: components["schemas"]["DeclarationViewSettings-Output"];
            federate_events: components["schemas"]["EventViewSettings-Output"];
            federate_time: components["schemas"]["TimeViewSettings-Output"];
            /**
             * Federate Views
             * @default []
             */
            federate_views: components["schemas"]["FederateWorkspaceSettings-Output"][];
            federates: components["schemas"]["FederateListSettings-Output"];
            federation: components["schemas"]["FederationViewSettings-Output"];
            fom: components["schemas"]["FomViewSettings-Output"];
            interactions: components["schemas"]["InteractionViewSettings-Output"];
            mom: components["schemas"]["MomViewSettings-Output"];
            /**
             * Object Charts
             * @default []
             */
            object_charts: components["schemas"]["ObjectChartChoice-Output"][];
            objects: components["schemas"]["ObjectViewSettings-Output"];
            ownership: components["schemas"]["OwnershipViewSettings-Output"];
            persistence: components["schemas"]["CoordinationViewSettings-Output"];
            regions: components["schemas"]["RegionViewSettings-Output"];
            /**
             * Selected Federate
             * @default
             */
            selected_federate: string;
            synchronization: components["schemas"]["CoordinationViewSettings-Output"];
            time: components["schemas"]["TimeViewSettings-Output"];
            topology: components["schemas"]["TopologyViewSettings-Output"];
            /**
             * View
             * @default federation
             * @enum {string}
             */
            view: "federation" | "federates" | "objects" | "declarations" | "topology" | "interactions" | "ownership" | "regions" | "mom" | "fom" | "time" | "synchronization" | "persistence" | "events" | "inspectors" | "recordings" | "alerts" | "actions" | "diagnostics";
            /**
             * Window Seconds
             * @default 300
             * @enum {integer}
             */
            window_seconds: 30 | 300 | 900 | 3600;
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
    alerts_api_alerts_get: {
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
                    "application/json": components["schemas"]["AlertReport"];
                };
            };
        };
    };
    acknowledge_alert_api_alerts_acknowledge_post: {
        parameters: {
            query?: never;
            header?: never;
            path?: never;
            cookie?: never;
        };
        requestBody: {
            content: {
                "application/json": components["schemas"]["AlertAcknowledgeRequest"];
            };
        };
        responses: {
            /** @description Successful Response */
            200: {
                headers: {
                    [name: string]: unknown;
                };
                content: {
                    "application/json": components["schemas"]["AlertReport"];
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
    configure_alerts_api_alerts_configuration_put: {
        parameters: {
            query?: never;
            header?: never;
            path?: never;
            cookie?: never;
        };
        requestBody: {
            content: {
                "application/json": components["schemas"]["AlertConfigurationUpdate"];
            };
        };
        responses: {
            /** @description Successful Response */
            200: {
                headers: {
                    [name: string]: unknown;
                };
                content: {
                    "application/json": components["schemas"]["AlertReport"];
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
    reset_alert_api_alerts_reset_post: {
        parameters: {
            query?: never;
            header?: never;
            path?: never;
            cookie?: never;
        };
        requestBody: {
            content: {
                "application/json": components["schemas"]["AlertResetRequest"];
            };
        };
        responses: {
            /** @description Successful Response */
            200: {
                headers: {
                    [name: string]: unknown;
                };
                content: {
                    "application/json": components["schemas"]["AlertReport"];
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
    analytics_api_analytics_get: {
        parameters: {
            query?: {
                bucket_count?: number;
                federate?: string | null;
                perspective?: string | null;
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
                "application/json": components["schemas"]["ConnectCommand"] | components["schemas"]["JoinCommand"] | components["schemas"]["ResignCommand"] | components["schemas"]["DisconnectCommand"] | components["schemas"]["InspectSessionCommand"] | components["schemas"]["RecoverSessionCommand"];
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
                instance_id?: string | null;
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
                instance_id?: string | null;
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
    defaults_api_workspaces_defaults_get: {
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
                    "application/json": components["schemas"]["WorkspaceDefaultsResponse"];
                };
            };
        };
    };
    validate_workspace_api_workspaces_validate_post: {
        parameters: {
            query?: never;
            header?: never;
            path?: never;
            cookie?: never;
        };
        requestBody: {
            content: {
                "application/json": components["schemas"]["WorkspaceDocument-Input"];
            };
        };
        responses: {
            /** @description Successful Response */
            200: {
                headers: {
                    [name: string]: unknown;
                };
                content: {
                    "application/json": components["schemas"]["WorkspaceDocument-Output"];
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
