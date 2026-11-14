#!/usr/bin/env python3
"""Run public message-history replay TCK cases with process fault adapters."""

from __future__ import annotations

import argparse
import json
import socket
import struct
import subprocess
import sys
import tempfile
import threading
import time
import xml.etree.ElementTree as ET
from pathlib import Path

from tck_evidence import (
    TckEvidencePayload,
    combine_tck_evidence,
    empty_tck_evidence,
    load_tck_evidence,
)

DEFAULT_FOM = (
    Path(__file__).resolve().parents[4]
    / "third_party"
    / "ieee1516.2-2025"
    / "resources"
    / "examples"
    / "RestaurantFOMmodule-2025.xml"
)
DEFAULT_MIM = (
    Path(__file__).resolve().parents[4]
    / "third_party"
    / "ieee1516.2-2025"
    / "resources"
    / "mim"
    / "HLAstandardMIM-2025.xml"
)
DEFAULT_PORTABLE_TCK_FOM = (
    Path(__file__).resolve().parents[2] / "fom" / "p0-tck.xml"
)
SCENARIO = "cpp-tck.connection-fault-join-response-replay"
FEDERATE_REQUEST_SCENARIO = "cpp-tck.connection-fault-federate-request-replay"
RTI_CLOSED_RESUME_FAILURE_SCENARIO = (
    "cpp-tck.connection-fault-rti-closed-resume-fails"
)
REGISTRATION_RESPONSE_SCENARIO = (
    "cpp-tck.connection-fault-registration-response-replay"
)
CREATE_FEDERATION_RESPONSE_SCENARIO = (
    "cpp-tck.connection-fault-create-federation-response-replay"
)
RESERVATION_RESPONSE_SCENARIO = (
    "cpp-tck.connection-fault-reservation-response-replay"
)
DELETE_OBJECT_RESPONSE_SCENARIO = (
    "cpp-tck.connection-fault-delete-object-response-replay"
)
ATTRIBUTE_UPDATE_RESPONSE_SCENARIO = (
    "cpp-tck.connection-fault-attribute-update-response-replay"
)
SEND_INTERACTION_RESPONSE_SCENARIO = (
    "cpp-tck.connection-fault-send-interaction-response-replay"
)
TIME_REGULATION_RESPONSE_SCENARIO = (
    "cpp-tck.connection-fault-time-regulation-response-replay"
)
TIME_CONSTRAINED_RESPONSE_SCENARIO = (
    "cpp-tck.connection-fault-time-constrained-response-replay"
)
TIME_ADVANCE_REQUEST_RESPONSE_SCENARIO = (
    "cpp-tck.connection-fault-time-advance-request-response-replay"
)
TIME_ADVANCE_REQUEST_AVAILABLE_RESPONSE_SCENARIO = (
    "cpp-tck.connection-fault-time-advance-request-available-response-replay"
)
NEXT_MESSAGE_REQUEST_RESPONSE_SCENARIO = (
    "cpp-tck.connection-fault-next-message-request-response-replay"
)
NEXT_MESSAGE_REQUEST_AVAILABLE_RESPONSE_SCENARIO = (
    "cpp-tck.connection-fault-next-message-request-available-response-replay"
)
LRM_HEADER_CHECK_SCENARIOS = {
    NEXT_MESSAGE_REQUEST_RESPONSE_SCENARIO,
    NEXT_MESSAGE_REQUEST_AVAILABLE_RESPONSE_SCENARIO,
}
LRM_ACKNOWLEDGEMENT_SCENARIOS = {NEXT_MESSAGE_REQUEST_RESPONSE_SCENARIO}
RESUME_DIRECTION_SCENARIOS = LRM_ACKNOWLEDGEMENT_SCENARIOS | {
    FEDERATE_REQUEST_SCENARIO,
    RTI_CLOSED_RESUME_FAILURE_SCENARIO,
}
SESSION_PURGE_SCENARIO = "cpp-tck.connection-fault-inactive-session-purge"


class RawFrameFaultProxy:
    """Transparent relay for bounded process message-history fault cases."""

    _FRAME_HEADER_BYTES = 24
    _MAXIMUM_FRAME_PAYLOAD = 4 * 1024 * 1024
    # Table 33 uses distinct data kinds for federate-to-RTI and RTI-to-federate frames.
    _DATA_FRAME_KIND = 20
    _SERVER_DATA_FRAME_KIND = 21
    _CALLBACK_REQUEST_FRAME_KIND = 22
    _CALLBACK_RESPONSE_FRAME_KIND = 23
    _RESUME_REQUEST_FRAME_KIND = 10
    _RESUME_STATUS_FRAME_KIND = 11
    _HEARTBEAT_FRAME_KIND = 3
    _HEARTBEAT_RESPONSE_FRAME_KIND = 4
    _INVALID_SEQUENCE_NUMBER = 0x80000000
    _SERVICE_REQUEST_KIND = 1
    _SERVICE_RESPONSE_KIND = 2
    _SERVICE_HEADER_BYTES = 24
    _GET_FEDERATE_HANDLE_OPERATION = 90
    _REGISTER_OBJECT_INSTANCE_OPERATION = 14
    _CREATE_FEDERATION_EXECUTION_OPERATION = 1
    _RESERVE_OBJECT_INSTANCE_NAME_OPERATION = 21
    _UPDATE_ATTRIBUTE_VALUES_OPERATION = 16
    _SEND_INTERACTION_OPERATION = 3
    _ENABLE_TIME_REGULATION_OPERATION = 49
    _ENABLE_TIME_CONSTRAINED_OPERATION = 51
    _TIME_ADVANCE_REQUEST_OPERATION = 52
    _TIME_ADVANCE_REQUEST_AVAILABLE_OPERATION = 56
    _NEXT_MESSAGE_REQUEST_OPERATION = 57
    _NEXT_MESSAGE_REQUEST_AVAILABLE_OPERATION = 58
    _QUERY_LOGICAL_TIME_OPERATION = 48
    _DELETE_OBJECT_INSTANCE_OPERATION = 47
    _RESPONSE_FAULTS = {
        REGISTRATION_RESPONSE_SCENARIO: (
            _REGISTER_OBJECT_INSTANCE_OPERATION,
            "RegisterObjectInstance",
        ),
        CREATE_FEDERATION_RESPONSE_SCENARIO: (
            _CREATE_FEDERATION_EXECUTION_OPERATION,
            "CreateFederationExecutionWithMIM",
        ),
        RESERVATION_RESPONSE_SCENARIO: (
            _RESERVE_OBJECT_INSTANCE_NAME_OPERATION,
            "ReserveObjectInstanceName",
        ),
        DELETE_OBJECT_RESPONSE_SCENARIO: (
            _DELETE_OBJECT_INSTANCE_OPERATION,
            "DeleteObjectInstance",
        ),
        ATTRIBUTE_UPDATE_RESPONSE_SCENARIO: (
            _UPDATE_ATTRIBUTE_VALUES_OPERATION,
            "UpdateAttributeValues",
        ),
        SEND_INTERACTION_RESPONSE_SCENARIO: (
            _SEND_INTERACTION_OPERATION,
            "SendInteraction",
        ),
        TIME_REGULATION_RESPONSE_SCENARIO: (
            _ENABLE_TIME_REGULATION_OPERATION,
            "EnableTimeRegulation",
        ),
        TIME_CONSTRAINED_RESPONSE_SCENARIO: (
            _ENABLE_TIME_CONSTRAINED_OPERATION,
            "EnableTimeConstrained",
        ),
        TIME_ADVANCE_REQUEST_RESPONSE_SCENARIO: (
            _TIME_ADVANCE_REQUEST_OPERATION,
            "TimeAdvanceRequest",
        ),
        TIME_ADVANCE_REQUEST_AVAILABLE_RESPONSE_SCENARIO: (
            _TIME_ADVANCE_REQUEST_AVAILABLE_OPERATION,
            "TimeAdvanceRequestAvailable",
        ),
        NEXT_MESSAGE_REQUEST_RESPONSE_SCENARIO: (
            _NEXT_MESSAGE_REQUEST_OPERATION,
            "NextMessageRequest",
        ),
        NEXT_MESSAGE_REQUEST_AVAILABLE_RESPONSE_SCENARIO: (
            _NEXT_MESSAGE_REQUEST_AVAILABLE_OPERATION,
            "NextMessageRequestAvailable",
        ),
    }

    def __init__(
        self,
        upstream_port: int,
        scenario: str,
        activation_marker: Path | None = None,
    ) -> None:
        self._upstream_port = upstream_port
        self._scenario = scenario
        self._activation_marker = activation_marker
        self._response_fault = self._RESPONSE_FAULTS.get(scenario)
        self._stop = threading.Event()
        self._lock = threading.Lock()
        self._connections: set[socket.socket] = set()
        self._workers: list[threading.Thread] = []
        self.dropped: tuple[int, int, int] | None = None
        self.replayed: tuple[int, int, int] | None = None
        self.withheld_request_for_control_fault: tuple[int, int, int] | None = None
        self.forwarded_fault_heartbeat: tuple[int, int] | None = None
        self.dropped_heartbeat_response: tuple[int, int, int] | None = None
        self.replayed_heartbeat_response: tuple[int, int, int] | None = None
        self.response_fault_request: tuple[int, int, int] | None = None
        self.dropped_service_response: tuple[int, int, int] | None = None
        self.replayed_service_response: tuple[int, int, int] | None = None
        self.dropped_service_response_status: int | None = None
        self.replayed_service_response_status: int | None = None
        self.dropped_service_response_lrm: int | None = None
        self.replayed_service_response_lrm: int | None = None
        self.lrm_acknowledgement_request: tuple[int, int, int] | None = None
        self.lrm_acknowledgement_request_value: int | None = None
        self.dropped_acknowledgement_response: tuple[int, int, int] | None = None
        self.replayed_acknowledgement_response: tuple[int, int, int] | None = None
        self.dropped_acknowledgement_response_status: int | None = None
        self.replayed_acknowledgement_response_status: int | None = None
        self.dropped_acknowledgement_response_lrm: int | None = None
        self.replayed_acknowledgement_response_lrm: int | None = None
        self.acknowledged_response_replayed_again = False
        self.resume_request_observations: list[dict[str, int | None]] = []
        self.resume_status_observations: list[dict[str, int | None]] = []
        self.resume_control_observation_errors: list[str] = []
        self.resume_control_observation_order = 0
        self.invalid_sequence_frame: dict[str, int] | None = None
        self.rti_closed_socket_after_invalid_sequence = False
        self.pending_federate_message_sequences: dict[int, list[int]] = {}
        self.observed_server_service_messages: list[dict[str, int]] = []
        self.observed_server_frames: list[dict[str, object]] = []
        self.listener = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.listener.bind(("127.0.0.1", 0))
        self.listener.listen()
        self.listener.settimeout(0.1)
        self.port = int(self.listener.getsockname()[1])
        self._accept_thread = threading.Thread(target=self._accept_loop, daemon=True)
        self._accept_thread.start()

    @staticmethod
    def _receive_exact(connection: socket.socket, size: int) -> bytes | None:
        data = bytearray()
        while len(data) < size:
            chunk = connection.recv(size - len(data))
            if not chunk:
                return None if not data else bytes(data)
            data.extend(chunk)
        return bytes(data)

    @classmethod
    def _receive_frame(cls, connection: socket.socket) -> bytes | None:
        size_prefix = cls._receive_exact(connection, 4)
        if size_prefix is None:
            return None
        if len(size_prefix) != 4:
            raise RuntimeError("The process fault proxy received a truncated frame size")
        packet_size = struct.unpack(">I", size_prefix)[0]
        if packet_size < cls._FRAME_HEADER_BYTES:
            raise RuntimeError("The process fault proxy received a malformed frame size")
        payload_size = packet_size - cls._FRAME_HEADER_BYTES
        if payload_size > cls._MAXIMUM_FRAME_PAYLOAD:
            raise RuntimeError("The process fault proxy received an oversized frame")
        packet_tail = cls._receive_exact(connection, packet_size - len(size_prefix))
        if packet_tail is None or len(packet_tail) != packet_size - len(size_prefix):
            return None
        return size_prefix + packet_tail

    @staticmethod
    def _close_pair(first: socket.socket, second: socket.socket) -> None:
        for connection in (first, second):
            try:
                connection.shutdown(socket.SHUT_RDWR)
            except OSError:
                pass
            try:
                connection.close()
            except OSError:
                pass

    def _service_message(
        self, frame: bytes
    ) -> tuple[int, int, int, int] | None:
        if (
            len(frame) < self._FRAME_HEADER_BYTES + self._SERVICE_HEADER_BYTES
            or struct.unpack(">I", frame[20:24])[0]
            not in {self._DATA_FRAME_KIND, self._SERVER_DATA_FRAME_KIND}
        ):
            return None
        payload = frame[self._FRAME_HEADER_BYTES :]
        if payload[:4] != b"UMSV":
            return None
        message_kind = payload[6]
        operation = struct.unpack(">H", payload[8:10])[0]
        status = struct.unpack(">H", payload[10:12])[0]
        request_id = struct.unpack(">Q", payload[12:20])[0]
        return message_kind, operation, status, request_id

    @staticmethod
    def _frame_identity(frame: bytes, request_id: int) -> tuple[int, int, int]:
        sequence = struct.unpack(">I", frame[4:8])[0]
        session = struct.unpack(">Q", frame[8:16])[0]
        return sequence, session, request_id

    @staticmethod
    def _last_received_message_number(frame: bytes) -> int:
        return struct.unpack(">I", frame[16:20])[0]

    @staticmethod
    def _frame_sequence_number(frame: bytes) -> int:
        return struct.unpack(">I", frame[4:8])[0]

    @staticmethod
    def _sequence_is_not_older(candidate: int, reference: int) -> bool:
        if (
            candidate == RawFrameFaultProxy._INVALID_SEQUENCE_NUMBER
            or reference == RawFrameFaultProxy._INVALID_SEQUENCE_NUMBER
        ):
            return False
        return ((candidate - reference) & 0x7FFFFFFF) < 0x40000000

    def _acknowledge_pending_federate_messages_locked(
        self, session_id: int, last_received: int
    ) -> None:
        if last_received == self._INVALID_SEQUENCE_NUMBER:
            return
        pending = self.pending_federate_message_sequences.get(session_id, [])
        self.pending_federate_message_sequences[session_id] = [
            sequence_number
            for sequence_number in pending
            if not self._sequence_is_not_older(last_received, sequence_number)
        ]

    def _observe_forwarded_frame(self, frame: bytes, client_to_server: bool) -> None:
        if self._scenario not in RESUME_DIRECTION_SCENARIOS:
            return

        frame_kind = struct.unpack(">I", frame[20:24])[0]
        sequence_number = self._frame_sequence_number(frame)
        if frame_kind == self._RESUME_REQUEST_FRAME_KIND:
            if not client_to_server:
                with self._lock:
                    self.resume_control_observation_errors.append(
                        "CTRL_RESUME_REQUEST traveled from the RTI to the federate"
                    )
                return
            if len(frame) < self._FRAME_HEADER_BYTES + 8:
                with self._lock:
                    self.resume_control_observation_errors.append(
                        "The process fault proxy received a truncated Resume Request"
                    )
                return
            with self._lock:
                session_id = struct.unpack(">Q", frame[8:16])[0]
                pending_sequences = self.pending_federate_message_sequences.get(
                    session_id, []
                )
                self.resume_control_observation_order += 1
                self.resume_request_observations.append(
                    {
                        "frame_sequence": sequence_number,
                        "session_id": session_id,
                        "observation_order": self.resume_control_observation_order,
                        "header_lrm": self._last_received_message_number(frame),
                        "last_received_rti_sequence": struct.unpack(">I", frame[24:28])[0],
                        "oldest_available_federate_sequence": struct.unpack(">I", frame[28:32])[0],
                        "expected_lrr_from_followup_public_query_lrm": self.lrm_acknowledgement_request_value,
                        "expected_oaf_from_pending_federate_message": (
                            pending_sequences[0]
                            if pending_sequences
                            else self._INVALID_SEQUENCE_NUMBER
                        ),
                    }
                )
            return

        if frame_kind == self._RESUME_STATUS_FRAME_KIND:
            if client_to_server:
                with self._lock:
                    self.resume_control_observation_errors.append(
                        "CTRL_RESUME_STATUS traveled from the federate to the RTI"
                    )
                return
            if len(frame) < self._FRAME_HEADER_BYTES + 8:
                with self._lock:
                    self.resume_control_observation_errors.append(
                        "The process fault proxy received a truncated Resume Status"
                    )
                return
            with self._lock:
                session_id = struct.unpack(">Q", frame[8:16])[0]
                self.resume_control_observation_order += 1
                self.resume_status_observations.append(
                    {
                        "frame_sequence": sequence_number,
                        "session_id": session_id,
                        "observation_order": self.resume_control_observation_order,
                        "status": struct.unpack(">I", frame[24:28])[0],
                        "last_received_federate_sequence": struct.unpack(">I", frame[28:32])[0],
                        "header_lrm": self._last_received_message_number(frame),
                        "expected_lrf_from_dropped_followup_response_lrm": self.dropped_acknowledgement_response_lrm,
                    }
                )
                self._acknowledge_pending_federate_messages_locked(
                    session_id, self._last_received_message_number(frame)
                )
            return

        message = self._service_message(frame)
        session_id = struct.unpack(">Q", frame[8:16])[0]
        with self._lock:
            if client_to_server:
                if frame_kind not in {
                    self._DATA_FRAME_KIND,
                    self._CALLBACK_RESPONSE_FRAME_KIND,
                }:
                    return
                pending_sequences = self.pending_federate_message_sequences.setdefault(
                    session_id, []
                )
                if sequence_number not in pending_sequences:
                    pending_sequences.append(sequence_number)
                return

            is_service_message = message is not None and frame_kind in {
                self._SERVER_DATA_FRAME_KIND,
                self._CALLBACK_REQUEST_FRAME_KIND,
            }
            is_acknowledging_control = frame_kind in {
                self._HEARTBEAT_FRAME_KIND,
                self._HEARTBEAT_RESPONSE_FRAME_KIND,
            }
            if is_service_message or is_acknowledging_control:
                self._acknowledge_pending_federate_messages_locked(
                    session_id, self._last_received_message_number(frame)
                )

    def _fault_identity(self, frame: bytes) -> tuple[int, int, int] | None:
        message = self._service_message(frame)
        if message is None:
            return None
        message_kind, operation, _, request_id = message
        if (
            message_kind != self._SERVICE_REQUEST_KIND
            or operation != self._GET_FEDERATE_HANDLE_OPERATION
        ):
            return None
        return self._frame_identity(frame, request_id)

    def _inject_invalid_sequence(self, frame: bytes) -> bytes | None:
        if (
            self._scenario != RTI_CLOSED_RESUME_FAILURE_SCENARIO
            or self._activation_marker is None
            or not self._activation_marker.is_file()
        ):
            return None
        if struct.unpack(">I", frame[20:24])[0] != self._DATA_FRAME_KIND:
            return None
        with self._lock:
            if self.invalid_sequence_frame is not None:
                return None
            original_sequence = self._frame_sequence_number(frame)
            session_id = struct.unpack(">Q", frame[8:16])[0]
            replacement_sequence = self._INVALID_SEQUENCE_NUMBER
            self.invalid_sequence_frame = {
                "frame_kind": self._DATA_FRAME_KIND,
                "original_transport_sequence": original_sequence,
                "injected_transport_sequence": replacement_sequence,
                "session_id": session_id,
            }
        invalid_frame = bytearray(frame)
        struct.pack_into(">I", invalid_frame, 4, replacement_sequence)
        return bytes(invalid_frame)

    def _response_fault_request_identity(
        self, frame: bytes
    ) -> tuple[int, int, int] | None:
        message = self._service_message(frame)
        if message is None:
            return None
        message_kind, operation, _, request_id = message
        if (
            message_kind != self._SERVICE_REQUEST_KIND
            or self._response_fault is None
            or operation != self._response_fault[0]
        ):
            return None
        return self._frame_identity(frame, request_id)

    def _response_fault_response(
        self, frame: bytes
    ) -> tuple[tuple[int, int, int], int] | None:
        message = self._service_message(frame)
        if message is None:
            return None
        message_kind, operation, status, request_id = message
        if (
            message_kind != self._SERVICE_RESPONSE_KIND
            or self._response_fault is None
            or operation != self._response_fault[0]
        ):
            return None
        return self._frame_identity(frame, request_id), status

    def _relay(
        self,
        source: socket.socket,
        destination: socket.socket,
        client_to_server: bool,
        client: socket.socket,
        upstream: socket.socket,
    ) -> None:
        try:
            while not self._stop.is_set():
                frame = self._receive_frame(source)
                if frame is None:
                    if not client_to_server:
                        with self._lock:
                            if self.invalid_sequence_frame is not None:
                                self.rti_closed_socket_after_invalid_sequence = True
                    break
                if client_to_server:
                    injected_frame = self._inject_invalid_sequence(frame)
                    if injected_frame is not None:
                        frame = injected_frame
                if not client_to_server:
                    with self._lock:
                        self.observed_server_frames.append(
                            {
                                "kind": struct.unpack(">I", frame[20:24])[0],
                                "sequence": self._frame_sequence_number(frame),
                                "session_id": struct.unpack(">Q", frame[8:16])[0],
                                "last_received": self._last_received_message_number(frame),
                                "bytes": len(frame),
                                "payload_prefix": frame[24:32].hex(),
                            }
                        )
                        del self.observed_server_frames[:-32]
                if client_to_server:
                    withhold_request = False
                    if self._response_fault is not None:
                        identity = self._response_fault_request_identity(frame)
                        if identity is not None:
                            with self._lock:
                                if self.response_fault_request is None:
                                    self.response_fault_request = identity
                        if self._scenario in LRM_ACKNOWLEDGEMENT_SCENARIOS:
                            message = self._service_message(frame)
                            if message is not None:
                                message_kind, operation, _, request_id = message
                                followup_identity = self._frame_identity(
                                    frame, request_id
                                )
                                with self._lock:
                                    initial_request = self.response_fault_request
                                    if (
                                        message_kind == self._SERVICE_REQUEST_KIND
                                        and operation == self._QUERY_LOGICAL_TIME_OPERATION
                                        and self.replayed_service_response is not None
                                        and initial_request is not None
                                        and followup_identity[1] == initial_request[1]
                                        and self.lrm_acknowledgement_request is None
                                    ):
                                        self.lrm_acknowledgement_request = (
                                            followup_identity
                                        )
                                        self.lrm_acknowledgement_request_value = (
                                            self._last_received_message_number(frame)
                                        )
                    elif self._scenario == FEDERATE_REQUEST_SCENARIO:
                        identity = self._fault_identity(frame)
                        frame_kind = struct.unpack(">I", frame[20:24])[0]
                        if identity is not None:
                            with self._lock:
                                if self.dropped is None:
                                    self.dropped = identity
                                    self.withheld_request_for_control_fault = identity
                                    withhold_request = True
                                elif identity == self.dropped:
                                    if self.dropped_heartbeat_response is None:
                                        withhold_request = True
                                    elif self.replayed is None:
                                        self.replayed = identity
                                elif (
                                    self.replayed == self.dropped
                                    and self.response_fault_request is None
                                ):
                                    self.response_fault_request = identity
                            if withhold_request:
                                continue
                        elif frame_kind == self._HEARTBEAT_FRAME_KIND:
                            session_id = struct.unpack(">Q", frame[8:16])[0]
                            heartbeat = (
                                self._frame_sequence_number(frame), session_id
                            )
                            with self._lock:
                                if (
                                    self.withheld_request_for_control_fault is not None
                                    and session_id
                                    == self.withheld_request_for_control_fault[1]
                                    and self.forwarded_fault_heartbeat is None
                                ):
                                    self.forwarded_fault_heartbeat = heartbeat
                    elif self._scenario != RTI_CLOSED_RESUME_FAILURE_SCENARIO:
                        identity = self._fault_identity(frame)
                        if identity is not None:
                            with self._lock:
                                if self.dropped is None:
                                    self.dropped = identity
                                    drop = True
                                else:
                                    self.replayed = identity
                                    drop = False
                            if drop:
                                self._close_pair(client, upstream)
                                return
                elif self._scenario == FEDERATE_REQUEST_SCENARIO:
                    frame_kind = struct.unpack(">I", frame[20:24])[0]
                    if (
                        frame_kind == self._HEARTBEAT_RESPONSE_FRAME_KIND
                        and len(frame) >= self._FRAME_HEADER_BYTES + 4
                    ):
                        response_to = struct.unpack(">I", frame[24:28])[0]
                        session_id = struct.unpack(">Q", frame[8:16])[0]
                        response_identity = (
                            self._frame_sequence_number(frame),
                            session_id,
                            response_to,
                        )
                        drop_heartbeat_response = False
                        with self._lock:
                            heartbeat = self.forwarded_fault_heartbeat
                            if (
                                heartbeat is not None
                                and session_id == heartbeat[1]
                                and response_to == heartbeat[0]
                            ):
                                if self.dropped_heartbeat_response is None:
                                    self.dropped_heartbeat_response = response_identity
                                    drop_heartbeat_response = True
                                elif response_identity == self.dropped_heartbeat_response:
                                    self.replayed_heartbeat_response = response_identity
                        if drop_heartbeat_response:
                            self._close_pair(client, upstream)
                            return
                    service_message = self._service_message(frame)
                    if (
                        service_message is not None
                        and service_message[0] == self._SERVICE_RESPONSE_KIND
                        and service_message[1] == self._GET_FEDERATE_HANDLE_OPERATION
                    ):
                        _, _, status, request_id = service_message
                        response_identity = self._frame_identity(frame, request_id)
                        drop_service_response = False
                        with self._lock:
                            request_identity = self.response_fault_request
                            matches_request = (
                                request_identity is not None
                                and response_identity[1] == request_identity[1]
                                and response_identity[2] == request_identity[2]
                            )
                            if matches_request:
                                if self.dropped_service_response is None:
                                    if status == 0:
                                        self.dropped_service_response = response_identity
                                        self.dropped_service_response_status = status
                                        self.dropped_service_response_lrm = (
                                            self._last_received_message_number(frame)
                                        )
                                        drop_service_response = True
                                elif response_identity == self.dropped_service_response:
                                    self.replayed_service_response = response_identity
                                    self.replayed_service_response_status = status
                                    self.replayed_service_response_lrm = (
                                        self._last_received_message_number(frame)
                                    )
                        if drop_service_response:
                            self._close_pair(client, upstream)
                            return
                elif self._response_fault is not None:
                    message = self._service_message(frame)
                    if message is not None:
                        with self._lock:
                            self.observed_server_service_messages.append(
                                {
                                    "kind": message[0],
                                    "operation": message[1],
                                    "status": message[2],
                                    "request_id": message[3],
                                    "sequence": self._frame_sequence_number(frame),
                                    "session_id": struct.unpack(">Q", frame[8:16])[0],
                                }
                            )
                            del self.observed_server_service_messages[:-32]
                    response = self._response_fault_response(frame)
                    if response is not None:
                        identity, status = response
                        with self._lock:
                            request_identity = self.response_fault_request
                            matches_request = (
                                request_identity is not None
                                and identity[1] == request_identity[1]
                                and identity[2] == request_identity[2]
                            )
                            if matches_request:
                                if self.dropped_service_response is None:
                                    if status == 0:
                                        self.dropped_service_response = identity
                                        self.dropped_service_response_status = status
                                        self.dropped_service_response_lrm = (
                                            self._last_received_message_number(frame)
                                        )
                                        drop = True
                                    else:
                                        drop = False
                                else:
                                    self.replayed_service_response = identity
                                    self.replayed_service_response_status = status
                                    self.replayed_service_response_lrm = (
                                        self._last_received_message_number(frame)
                                    )
                                    drop = False
                            else:
                                drop = False
                        if drop:
                            self._close_pair(client, upstream)
                            return
                if (
                    not client_to_server
                    and self._scenario in LRM_ACKNOWLEDGEMENT_SCENARIOS
                ):
                    message = self._service_message(frame)
                    if message is not None:
                        message_kind, _, status, request_id = message
                        if message_kind == self._SERVICE_RESPONSE_KIND:
                            identity = self._frame_identity(frame, request_id)
                            drop_acknowledgement_response = False
                            with self._lock:
                                acknowledgement_request = (
                                    self.lrm_acknowledgement_request
                                )
                                matches_acknowledgement_request = (
                                    acknowledgement_request is not None
                                    and identity[1] == acknowledgement_request[1]
                                    and identity[2] == acknowledgement_request[2]
                                )
                                if matches_acknowledgement_request:
                                    if self.dropped_acknowledgement_response is None:
                                        if status == 0:
                                            self.dropped_acknowledgement_response = (
                                                identity
                                            )
                                            self.dropped_acknowledgement_response_status = (
                                                status
                                            )
                                            self.dropped_acknowledgement_response_lrm = (
                                                self._last_received_message_number(frame)
                                            )
                                            drop_acknowledgement_response = True
                                    else:
                                        self.replayed_acknowledgement_response = identity
                                        self.replayed_acknowledgement_response_status = (
                                            status
                                        )
                                        self.replayed_acknowledgement_response_lrm = (
                                            self._last_received_message_number(frame)
                                        )
                                elif (
                                    self.dropped_acknowledgement_response is not None
                                    and identity == self.dropped_service_response
                                ):
                                    self.acknowledged_response_replayed_again = True
                            if drop_acknowledgement_response:
                                self._close_pair(client, upstream)
                                return
                self._observe_forwarded_frame(frame, client_to_server)
                destination.sendall(frame)
        except (OSError, RuntimeError):
            pass
        finally:
            self._close_pair(client, upstream)

    def _serve(self, client: socket.socket) -> None:
        upstream: socket.socket | None = None
        try:
            upstream = socket.create_connection(("127.0.0.1", self._upstream_port))
            with self._lock:
                self._connections.update((client, upstream))
            directions = (
                threading.Thread(
                    target=self._relay,
                    args=(client, upstream, True, client, upstream),
                    daemon=True,
                ),
                threading.Thread(
                    target=self._relay,
                    args=(upstream, client, False, client, upstream),
                    daemon=True,
                ),
            )
            for direction in directions:
                direction.start()
            for direction in directions:
                direction.join()
        except OSError:
            if upstream is not None:
                self._close_pair(client, upstream)
            else:
                client.close()
        finally:
            with self._lock:
                self._connections.discard(client)
                if upstream is not None:
                    self._connections.discard(upstream)

    def _accept_loop(self) -> None:
        while not self._stop.is_set():
            try:
                client, _ = self.listener.accept()
            except socket.timeout:
                continue
            except OSError:
                return
            worker = threading.Thread(target=self._serve, args=(client,), daemon=True)
            with self._lock:
                self._workers.append(worker)
            worker.start()

    def close(self) -> None:
        self._stop.set()
        self.listener.close()
        with self._lock:
            connections = list(self._connections)
            workers = list(self._workers)
        for connection in connections:
            try:
                connection.close()
            except OSError:
                pass
        self._accept_thread.join(timeout=2)
        for worker in workers:
            worker.join(timeout=2)

    def evidence(self) -> dict[str, object]:
        if self._scenario == RTI_CLOSED_RESUME_FAILURE_SCENARIO:
            if self.invalid_sequence_frame is None:
                raise RuntimeError(
                    "The proxy did not inject the invalid sequence into a federate "
                    "post-join data frame; "
                    f"state={self.diagnostic_state()}"
                )
            if not self.rti_closed_socket_after_invalid_sequence:
                raise RuntimeError(
                    "The RTI did not close its side of the socket after receiving "
                    "the invalid sequence; "
                    f"state={self.diagnostic_state()}"
                )
            evidence: dict[str, object] = {
                "invalid_sequence_frame": self.invalid_sequence_frame,
                "rti_closed_socket_after_invalid_sequence": True,
                "resume_request_count": len(self.resume_request_observations),
                "resume_request_session_ids": sorted(
                    {
                        int(observation["session_id"])
                        for observation in self.resume_request_observations
                        if isinstance(observation["session_id"], int)
                    }
                ),
                "resume_status_count": len(self.resume_status_observations),
            }
            if self.resume_control_observation_errors:
                raise RuntimeError(
                    "The proxy observed invalid Resume control-message traffic; "
                    f"errors={self.resume_control_observation_errors}, "
                    f"state={self.diagnostic_state()}"
                )
            if not self.resume_request_observations:
                raise RuntimeError(
                    "The federate did not attempt session resume after the RTI "
                    "closed its original socket; "
                    f"state={self.diagnostic_state()}"
                )
            original_session = self.invalid_sequence_frame["session_id"]
            if any(
                observation["session_id"] != original_session
                for observation in self.resume_request_observations
            ):
                raise RuntimeError(
                    "A Resume Request did not use the original joined-federate "
                    "session identity; "
                    f"state={self.diagnostic_state()}"
                )
            return evidence
        if self._response_fault is not None:
            if (
                self.response_fault_request is None
                or self.dropped_service_response is None
                or self.replayed_service_response is None
            ):
                raise RuntimeError(
                    "The proxy did not observe an accepted "
                    f"{self._response_fault[1]} response and its replay; "
                    f"state={self.diagnostic_state()}"
                )
            if self.dropped_service_response_status != 0:
                raise RuntimeError(
                    f"The proxy did not drop a successful {self._response_fault[1]} response"
                )
            if (
                self.dropped_service_response != self.replayed_service_response
                or self.replayed_service_response_status != 0
            ):
                raise RuntimeError(
                    f"The RTI replayed a different or unsuccessful {self._response_fault[1]} response"
                )
            evidence = {
                "service": self._response_fault[1],
                "operation": self._response_fault[0],
                "accepted_request_forwarded_to_rti": True,
                "accepted_response_status": self.dropped_service_response_status,
                "accepted_response_dropped_before_federate": True,
                "same_session_response_replayed": True,
                "response_sequence": self.dropped_service_response[0],
                "session_id": self.dropped_service_response[1],
                "request_id": self.dropped_service_response[2],
            }
            evidence.update(self._resume_control_direction_evidence())
            if self._scenario in LRM_HEADER_CHECK_SCENARIOS:
                if self.response_fault_request is None:
                    raise RuntimeError("The RTI response has no matching service request")
                expected_lrm = self.response_fault_request[0]
                if (
                    self.dropped_service_response_lrm != expected_lrm
                    or self.replayed_service_response_lrm != expected_lrm
                ):
                    raise RuntimeError(
                        "The RTI response LRM did not match the accepted request sequence; "
                        f"expected={expected_lrm}, state={self.diagnostic_state()}"
                    )
                evidence.update(
                    {
                        "accepted_response_lrm": self.dropped_service_response_lrm,
                        "replayed_response_lrm": self.replayed_service_response_lrm,
                        "expected_last_received_service_sequence": expected_lrm,
                        "lrm_matches_last_received_service_message": True,
                    }
                )
            if self._scenario in LRM_ACKNOWLEDGEMENT_SCENARIOS:
                if (
                    self.lrm_acknowledgement_request is None
                    or self.lrm_acknowledgement_request_value is None
                    or self.dropped_acknowledgement_response is None
                    or self.replayed_acknowledgement_response is None
                    or self.replayed_service_response is None
                ):
                    raise RuntimeError(
                        "The proxy did not observe the post-replay LRM acknowledgement "
                        "and second response replay; "
                        f"state={self.diagnostic_state()}"
                    )
                acknowledged_response_sequence = self.replayed_service_response[0]
                expected_followup_lrm = self.lrm_acknowledgement_request[0]
                if not self._sequence_is_not_older(
                    self.lrm_acknowledgement_request_value,
                    acknowledged_response_sequence,
                ):
                    raise RuntimeError(
                        "The follow-up service request did not acknowledge the replayed "
                        "response through LRM; "
                        f"response_sequence={acknowledged_response_sequence}, "
                        f"observed_lrm={self.lrm_acknowledgement_request_value}"
                    )
                if (
                    self.dropped_acknowledgement_response
                    != self.replayed_acknowledgement_response
                    or self.dropped_acknowledgement_response_status != 0
                    or self.replayed_acknowledgement_response_status != 0
                    or self.dropped_acknowledgement_response_lrm
                    != expected_followup_lrm
                    or self.replayed_acknowledgement_response_lrm
                    != expected_followup_lrm
                ):
                    raise RuntimeError(
                        "The follow-up query response was not replayed with its request "
                        "sequence in LRM; "
                        f"state={self.diagnostic_state()}"
                    )
                if self.acknowledged_response_replayed_again:
                    raise RuntimeError(
                        "The RTI replayed the LRM-acknowledged NextMessageRequest "
                        f"response during the second recovery; state={self.diagnostic_state()}"
                    )
                if (
                    len(self.resume_request_observations) < 2
                    or len(self.resume_status_observations) < 2
                ):
                    raise RuntimeError(
                        "The proxy did not observe both Resume Request/Status exchanges "
                        "required by the two response-loss recoveries; "
                        f"state={self.diagnostic_state()}"
                    )
                resume_request = self.resume_request_observations[-1]
                prior_resume_status = self.resume_status_observations[-2]
                resume_status = self.resume_status_observations[-1]
                # OAF describes the oldest retained federate-originated frame.
                # That can be a callback response older than the follow-up query;
                # the query's dropped response may have carried the acknowledgement
                # that would release the older frame.
                expected_oaf = resume_request[
                    "expected_oaf_from_pending_federate_message"
                ]
                if (
                    resume_request["last_received_rti_sequence"]
                    != self.lrm_acknowledgement_request_value
                    or resume_request["header_lrm"]
                    != resume_request["last_received_rti_sequence"]
                    or resume_request["oldest_available_federate_sequence"]
                    != expected_oaf
                    or resume_request["oldest_available_federate_sequence"]
                    == resume_request["frame_sequence"]
                ):
                    raise RuntimeError(
                        "The second Resume Request did not report retained-message "
                        "LRR/OAF "
                        "history values; "
                        f"request={resume_request}, state={self.diagnostic_state()}"
                    )
                if (
                    resume_status["status"] != 0
                    or resume_status["last_received_federate_sequence"]
                    != self.dropped_acknowledgement_response_lrm
                    or resume_status["header_lrm"]
                    != resume_status["last_received_federate_sequence"]
                    or resume_status["last_received_federate_sequence"]
                    == resume_request["frame_sequence"]
                    or resume_request["last_received_rti_sequence"]
                    == prior_resume_status["frame_sequence"]
                ):
                    raise RuntimeError(
                        "The second Resume Status/Request exchange counted a control "
                        "frame as an HLA service sequence; "
                        f"request={resume_request}, status={resume_status}, "
                        f"prior_status={prior_resume_status}, "
                        f"state={self.diagnostic_state()}"
                    )
                evidence.update(
                    {
                        "followup_service": "QueryLogicalTime",
                        "followup_request_sequence": self.lrm_acknowledgement_request[0],
                        "followup_request_lrm": (
                            self.lrm_acknowledgement_request_value
                        ),
                        "acknowledged_response_sequence": acknowledged_response_sequence,
                        "acknowledged_response_not_replayed_after_second_recovery": True,
                        "followup_response_sequence": (
                            self.dropped_acknowledgement_response[0]
                        ),
                        "followup_response_lrm": (
                            self.dropped_acknowledgement_response_lrm
                        ),
                        "followup_response_replayed_with_same_identity": True,
                        "resume_control_sequence_accounting": {
                            "second_resume_request_frame_sequence": resume_request["frame_sequence"],
                            "last_received_rti_service_sequence_lrr": resume_request["last_received_rti_sequence"],
                            "expected_last_received_rti_service_sequence_from_query_lrm": self.lrm_acknowledgement_request_value,
                            "prior_resume_status_control_sequence_excluded_from_lrr": prior_resume_status["frame_sequence"],
                            "oldest_available_federate_sequence_oaf": resume_request["oldest_available_federate_sequence"],
                            "expected_oldest_pending_federate_message_sequence": expected_oaf,
                            "resume_status_frame_sequence": resume_status["frame_sequence"],
                            "last_received_federate_service_sequence_lrf": resume_status["last_received_federate_sequence"],
                            "expected_last_received_federate_service_sequence_from_query_response_lrm": self.dropped_acknowledgement_response_lrm,
                            "resume_request_status_success": True,
                            "lrr_oaf_lrf_exclude_resume_controls": True,
                            "oar_numeric_value_exposed_by_resume_exchange": False,
                            "oar_evidence_boundary": "The successful Resume Status and same-identity service replay exercise the RTI history-availability outcome across control-frame gaps; this private adapter cannot read a separate numeric OAR because the process protocol does not transmit one.",
                        },
                    }
                )
            return evidence
        if self.dropped is None or self.replayed is None:
            raise RuntimeError(
                "The proxy did not observe a replayed federate request; "
                f"state={self.diagnostic_state()}"
            )
        if self.dropped != self.replayed:
            raise RuntimeError(
                "The federate resent a different request identity after reconnect"
            )
        if self._scenario == FEDERATE_REQUEST_SCENARIO:
            if (
                self.withheld_request_for_control_fault != self.dropped
                or self.forwarded_fault_heartbeat is None
                or self.dropped_heartbeat_response is None
            ):
                raise RuntimeError(
                    "The proxy did not withhold the service request and drop its "
                    "matching real heartbeat response; "
                    f"state={self.diagnostic_state()}"
                )
            if self.replayed_heartbeat_response is not None:
                raise RuntimeError(
                    "The dropped control Heartbeat Response was replayed after "
                    f"recovery; state={self.diagnostic_state()}"
                )
            if (
                self.response_fault_request is None
                or self.response_fault_request == self.dropped
                or self.dropped_service_response is None
                or self.replayed_service_response is None
                or self.dropped_service_response != self.replayed_service_response
                or self.dropped_service_response_status != 0
                or self.replayed_service_response_status != 0
            ):
                raise RuntimeError(
                    "The second public call did not receive the same successful "
                    "RTI service response after recovery; "
                    f"state={self.diagnostic_state()}"
                )
        evidence = {
            "dropped_before_rti_acceptance": True,
            "resent_same_sequence_session_request": True,
            "sequence": self.dropped[0],
            "session_id": self.dropped[1],
            "request_id": self.dropped[2],
        }
        if self._scenario == FEDERATE_REQUEST_SCENARIO:
            evidence["withheld_request_control_fault"] = {
                "withheld_service_request_identity": list(
                    self.withheld_request_for_control_fault or ()
                ),
                "forwarded_heartbeat_sequence": self.forwarded_fault_heartbeat[0],
                "heartbeat_session_id": self.forwarded_fault_heartbeat[1],
                "dropped_rti_heartbeat_response_identity": list(
                    self.dropped_heartbeat_response or ()
                ),
                "response_to_matches_forwarded_heartbeat": True,
                "dropped_control_response_not_replayed_after_resume": True,
            }
            evidence["replayed_hla_service_response"] = {
                "followup_request_identity": list(self.response_fault_request or ()),
                "dropped_response_identity": list(self.dropped_service_response or ()),
                "replayed_response_identity": list(self.replayed_service_response or ()),
                "response_identity_preserved": True,
                "successful_status_preserved": True,
                "service_response_replayed_after_second_resume": True,
            }
        evidence.update(self._resume_control_direction_evidence())
        return evidence

    def _resume_control_direction_evidence(self) -> dict[str, object]:
        if self._scenario not in RESUME_DIRECTION_SCENARIOS:
            return {}
        if self.resume_control_observation_errors:
            raise RuntimeError(
                "The proxy observed invalid Resume control-message traffic; "
                f"errors={self.resume_control_observation_errors}, "
                f"state={self.diagnostic_state()}"
            )
        if not self.resume_request_observations or not self.resume_status_observations:
            raise RuntimeError(
                "The proxy did not observe both federate-originated Resume Request "
                "and RTI-originated Resume Status frames; "
                f"state={self.diagnostic_state()}"
            )
        requests_by_session: dict[int, list[dict[str, int | None]]] = {}
        statuses_by_session: dict[int, list[dict[str, int | None]]] = {}
        for observation, observations_by_session in (
            (self.resume_request_observations, requests_by_session),
            (self.resume_status_observations, statuses_by_session),
        ):
            for item in observation:
                session_id = item["session_id"]
                if not isinstance(session_id, int):
                    raise RuntimeError(
                        "A Resume control message had no session identity"
                    )
                observations_by_session.setdefault(session_id, []).append(item)
        if requests_by_session.keys() != statuses_by_session.keys():
            raise RuntimeError(
                "Resume Request and Resume Status session sets differ; "
                f"requests={requests_by_session}, statuses={statuses_by_session}"
            )
        for session_id, requests in requests_by_session.items():
            statuses = statuses_by_session[session_id]
            if len(requests) != len(statuses):
                raise RuntimeError(
                    "The RTI did not send exactly one Resume Status per Resume "
                    "Request in a session; "
                    f"session_id={session_id}, requests={requests}, statuses={statuses}"
                )
            paired_observations = sorted(
                zip(requests, statuses),
                key=lambda pair: pair[0]["observation_order"] or 0,
            )
            if any(
                request["observation_order"] is None
                or status["observation_order"] is None
                or request["observation_order"] >= status["observation_order"]
                for request, status in paired_observations
            ):
                raise RuntimeError(
                    "A Resume Status did not follow its corresponding Resume "
                    "Request in the same session; "
                    f"session_id={session_id}, pairs={paired_observations}"
                )
        return {
            "resume_control_direction": {
                "federate_originated_resume_request_count": len(
                    self.resume_request_observations
                ),
                "rti_originated_resume_status_count": len(
                    self.resume_status_observations
                ),
                "one_later_resume_status_per_request_same_session": True,
                "wrong_direction_frames_absent": True,
            }
        }

    def diagnostic_state(self) -> dict[str, object]:
        with self._lock:
            return {
                "dropped_request": self.dropped,
                "resent_request": self.replayed,
                "withheld_request_for_control_fault": (
                    self.withheld_request_for_control_fault
                ),
                "forwarded_fault_heartbeat": self.forwarded_fault_heartbeat,
                "dropped_heartbeat_response": self.dropped_heartbeat_response,
                "replayed_heartbeat_response": self.replayed_heartbeat_response,
                "response_fault_request": self.response_fault_request,
                "dropped_service_response": self.dropped_service_response,
                "replayed_service_response": self.replayed_service_response,
                "dropped_service_response_status": self.dropped_service_response_status,
                "replayed_service_response_status": self.replayed_service_response_status,
                "dropped_service_response_lrm": self.dropped_service_response_lrm,
                "replayed_service_response_lrm": self.replayed_service_response_lrm,
                "lrm_acknowledgement_request": self.lrm_acknowledgement_request,
                "lrm_acknowledgement_request_value": (
                    self.lrm_acknowledgement_request_value
                ),
                "dropped_acknowledgement_response": (
                    self.dropped_acknowledgement_response
                ),
                "replayed_acknowledgement_response": (
                    self.replayed_acknowledgement_response
                ),
                "dropped_acknowledgement_response_status": (
                    self.dropped_acknowledgement_response_status
                ),
                "replayed_acknowledgement_response_status": (
                    self.replayed_acknowledgement_response_status
                ),
                "dropped_acknowledgement_response_lrm": (
                    self.dropped_acknowledgement_response_lrm
                ),
                "replayed_acknowledgement_response_lrm": (
                    self.replayed_acknowledgement_response_lrm
                ),
                "acknowledged_response_replayed_again": (
                    self.acknowledged_response_replayed_again
                ),
                "resume_request_observations": self.resume_request_observations,
                "resume_status_observations": self.resume_status_observations,
                "invalid_sequence_frame": self.invalid_sequence_frame,
                "rti_closed_socket_after_invalid_sequence": (
                    self.rti_closed_socket_after_invalid_sequence
                ),
                "pending_federate_message_sequences": (
                    self.pending_federate_message_sequences
                ),
                "observed_server_service_messages": (
                    self.observed_server_service_messages
                ),
                "observed_server_frames": self.observed_server_frames,
            }


class SessionPurgeConnectionProxy:
    """Dedicated member route that the adapter can close to trigger inactivity."""

    def __init__(self, upstream_port: int) -> None:
        self._upstream_port = upstream_port
        self._stop = threading.Event()
        self._lock = threading.Lock()
        self._connections: set[socket.socket] = set()
        self._workers: list[threading.Thread] = []
        self._listener = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self._listener.bind(("127.0.0.1", 0))
        self._listener.listen()
        self._listener.settimeout(0.1)
        self.port = self._listener.getsockname()[1]
        self._accept_thread = threading.Thread(
            target=self._accept_loop,
            daemon=True,
        )
        self._accept_thread.start()

    def _close_pair(
        self,
        first: socket.socket,
        second: socket.socket,
    ) -> None:
        for connection in (first, second):
            try:
                connection.shutdown(socket.SHUT_RDWR)
            except OSError:
                pass
            try:
                connection.close()
            except OSError:
                pass

    def _relay(self, source: socket.socket, destination: socket.socket) -> None:
        try:
            while not self._stop.is_set():
                payload = source.recv(65536)
                if not payload:
                    break
                destination.sendall(payload)
        except OSError:
            pass
        finally:
            self._close_pair(source, destination)

    def _serve(self, client: socket.socket) -> None:
        upstream: socket.socket | None = None
        try:
            upstream = socket.create_connection(
                ("127.0.0.1", self._upstream_port),
                timeout=2.0,
            )
            with self._lock:
                self._connections.update((client, upstream))
            directions = (
                threading.Thread(
                    target=self._relay,
                    args=(client, upstream),
                    daemon=True,
                ),
                threading.Thread(
                    target=self._relay,
                    args=(upstream, client),
                    daemon=True,
                ),
            )
            for direction in directions:
                direction.start()
            for direction in directions:
                direction.join()
        except OSError:
            if upstream is None:
                client.close()
            else:
                self._close_pair(client, upstream)
        finally:
            with self._lock:
                self._connections.discard(client)
                if upstream is not None:
                    self._connections.discard(upstream)

    def _accept_loop(self) -> None:
        while not self._stop.is_set():
            try:
                client, _ = self._listener.accept()
            except socket.timeout:
                continue
            except OSError:
                return
            worker = threading.Thread(target=self._serve, args=(client,), daemon=True)
            with self._lock:
                self._workers.append(worker)
            worker.start()

    def close(self) -> None:
        self._stop.set()
        self._listener.close()
        with self._lock:
            connections = list(self._connections)
            workers = list(self._workers)
        for connection in connections:
            try:
                connection.close()
            except OSError:
                pass
        self._accept_thread.join(timeout=2)
        for worker in workers:
            worker.join(timeout=2)


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--tck-executable", type=Path, required=True)
    parser.add_argument("--process-fixture", type=Path, required=True)
    parser.add_argument("--fom-path", type=Path, default=DEFAULT_FOM)
    parser.add_argument("--mim-fom", type=Path, default=DEFAULT_MIM)
    parser.add_argument("--time-implementation", default="HLAinteger64Time")
    parser.add_argument("--provider-id", default="current-process")
    parser.add_argument("--object-class", default="HLAobjectRoot.Employee.Server")
    parser.add_argument("--attribute", default="Efficiency")
    parser.add_argument(
        "--scenario",
        choices=(
            SCENARIO,
            FEDERATE_REQUEST_SCENARIO,
            RTI_CLOSED_RESUME_FAILURE_SCENARIO,
            REGISTRATION_RESPONSE_SCENARIO,
            CREATE_FEDERATION_RESPONSE_SCENARIO,
            RESERVATION_RESPONSE_SCENARIO,
            DELETE_OBJECT_RESPONSE_SCENARIO,
            ATTRIBUTE_UPDATE_RESPONSE_SCENARIO,
            SEND_INTERACTION_RESPONSE_SCENARIO,
            TIME_REGULATION_RESPONSE_SCENARIO,
            TIME_CONSTRAINED_RESPONSE_SCENARIO,
            TIME_ADVANCE_REQUEST_RESPONSE_SCENARIO,
            TIME_ADVANCE_REQUEST_AVAILABLE_RESPONSE_SCENARIO,
            NEXT_MESSAGE_REQUEST_RESPONSE_SCENARIO,
            NEXT_MESSAGE_REQUEST_AVAILABLE_RESPONSE_SCENARIO,
            SESSION_PURGE_SCENARIO,
        ),
        default=SCENARIO,
    )
    parser.add_argument("--timeout-s", type=float, default=45.0)
    parser.add_argument("--artifact-directory", type=Path)
    parser.add_argument("--results", type=Path)
    parser.add_argument("--junit", type=Path)
    parser.add_argument(
        "--callback-model", choices=("both", "evoked", "immediate"), default="both"
    )
    arguments = parser.parse_args()
    if arguments.scenario == RTI_CLOSED_RESUME_FAILURE_SCENARIO:
        # This case does not exercise model-defined services. Use the TCK's
        # deliberately small 2025 FOM so the process fixture is independent
        # of example-FOM capabilities unrelated to connection recovery.
        arguments.fom_path = DEFAULT_PORTABLE_TCK_FOM
    return arguments


def wait_for_marker(
    marker: Path,
    fixture: subprocess.Popen[bytes],
    deadline: float,
    tck: subprocess.Popen[bytes] | None = None,
) -> None:
    while time.monotonic() < deadline:
        if marker.is_file():
            return
        status = fixture.poll()
        if status is not None:
            raise RuntimeError(
                f"The process fixture exited with code {status} before {marker.name}"
            )
        if tck is not None and tck.poll() is not None:
            raise RuntimeError(
                f"The TCK exited with code {tck.returncode} before {marker.name}"
            )
        time.sleep(0.01)
    raise RuntimeError(f"Timed out waiting for process fixture marker {marker.name}")


def run_model(
    arguments: argparse.Namespace,
    model: str,
    directory: Path,
) -> tuple[int, TckEvidencePayload, ET.Element]:
    directory.mkdir(parents=True, exist_ok=True)
    session_purge = arguments.scenario == SESSION_PURGE_SCENARIO
    federate_request_replay = arguments.scenario == FEDERATE_REQUEST_SCENARIO
    rti_closed_resume_failure = (
        arguments.scenario == RTI_CLOSED_RESUME_FAILURE_SCENARIO
    )
    response_replay = arguments.scenario in RawFrameFaultProxy._RESPONSE_FAULTS
    raw_frame_fault = (
        federate_request_replay or rti_closed_resume_failure or response_replay
    )
    stale_markers = [
        marker
        for marker in (
            directory / "port.txt",
            directory / "message-history-recovery-complete.ok",
            directory / "session-purge-trigger.ok",
            directory / "resume-failure-trigger.ok",
            *(
                ()
                if raw_frame_fault
                else (
                    directory / "join-response-ready.ok",
                    directory / "drop-join-response.ok",
                    directory / "join-response-socket-closed.ok",
                )
            ),
        )
        if marker.exists()
    ]
    if stale_markers:
        names = ", ".join(marker.name for marker in stale_markers)
        raise RuntimeError(
            "Refusing to reuse stale process-fixture control markers "
            f"({names}); choose a fresh per-run artifact directory"
        )
    result_path = directory / "results.json"
    junit_path = directory / "results.xml"
    tck_complete = directory / "message-history-recovery-complete.ok"
    fixture = subprocess.Popen(
        [
            str(arguments.process_fixture),
            (
                "public-server-session-purge"
                if session_purge
                else "public-server-message-history"
                if raw_frame_fault
                else "public-server-join-response-replay"
            ),
            str(directory),
        ]
    )
    tck: subprocess.Popen[bytes] | None = None
    proxy: RawFrameFaultProxy | None = None
    purge_proxy: SessionPurgeConnectionProxy | None = None
    try:
        deadline = time.monotonic() + arguments.timeout_s
        port_path = directory / "port.txt"
        wait_for_marker(port_path, fixture, deadline)
        port = int(port_path.read_text(encoding="utf-8").strip())
        if not 0 < port <= 65535:
            raise RuntimeError(f"The process fixture published invalid port {port}")
        upstream_port = port
        if raw_frame_fault:
            proxy = RawFrameFaultProxy(
                port,
                arguments.scenario,
                (
                    directory / "resume-failure-trigger.ok"
                    if rti_closed_resume_failure
                    else None
                ),
            )
            port = proxy.port
        elif session_purge:
            purge_proxy = SessionPurgeConnectionProxy(port)

        tck_command = [
                str(arguments.tck_executable),
                "--fom",
                str(arguments.fom_path),
                "--mim-fom",
                str(arguments.mim_fom),
                "--time-implementation",
                arguments.time_implementation,
                "--object-class",
                arguments.object_class,
                "--attribute",
                arguments.attribute,
                "--scenario",
                arguments.scenario,
                "--callback-model",
                model,
                "--provider-id",
                arguments.provider_id,
                "--rti-address",
                f"tcp://127.0.0.1:{port}",
                "--owner-configuration-name",
                "message-history-owner",
                "--member-configuration-name",
                "message-history-member",
                "--message-history-recovery-server-managed",
                "--message-history-recovery-marker",
                str(tck_complete),
                "--results",
                str(result_path),
                "--junit",
                str(junit_path),
            ]
        if session_purge:
            if purge_proxy is None:
                raise RuntimeError("The inactive-session purge proxy was not started")
            tck_command.extend(
                [
                    "--owner-rti-address",
                    f"tcp://127.0.0.1:{port}",
                    "--member-rti-address",
                    f"tcp://127.0.0.1:{purge_proxy.port}",
                    "--message-history-session-purge-trigger-marker",
                    str(directory / "session-purge-trigger.ok"),
                ]
            )
        elif rti_closed_resume_failure:
            if proxy is None:
                raise RuntimeError("The RTI-close resume-failure proxy was not started")
            tck_command.extend(
                [
                    "--owner-rti-address",
                    f"tcp://127.0.0.1:{upstream_port}",
                    "--member-rti-address",
                    f"tcp://127.0.0.1:{proxy.port}",
                    "--additional-settings",
                    "FED_INT_HEART=0.02;FED_TIMEOUT_RECONNECT=0.25",
                    "--message-history-resume-failure-trigger-marker",
                    str(directory / "resume-failure-trigger.ok"),
                ]
            )
        elif federate_request_replay:
            tck_command.extend(
                [
                    "--additional-settings",
                    "FED_INT_HEART=0.02;FED_TIMEOUT_RECONNECT=0.25",
                ]
            )
        tck = subprocess.Popen(
            tck_command,
            stdout=None,
            stderr=None,
        )

        if session_purge:
            wait_for_marker(
                directory / "session-purge-trigger.ok",
                fixture,
                deadline,
                tck,
            )
            if purge_proxy is None:
                raise RuntimeError("The inactive-session purge proxy was not started")
            purge_proxy.close()
        elif rti_closed_resume_failure:
            wait_for_marker(
                directory / "resume-failure-trigger.ok",
                fixture,
                deadline,
                tck,
            )
        elif not raw_frame_fault:
            ready_path = directory / "join-response-ready.ok"
            wait_for_marker(ready_path, fixture, deadline, tck)
            (directory / "drop-join-response.ok").write_text(
                "drop\n", encoding="utf-8"
            )

        try:
            return_code = tck.wait(timeout=max(0.0, deadline - time.monotonic()))
        except subprocess.TimeoutExpired as error:
            if session_purge:
                progress = (
                    "session-purge-trigger="
                    f"{'present' if (directory / 'session-purge-trigger.ok').is_file() else 'missing'}, "
                    "tck-completion="
                    f"{'present' if tck_complete.is_file() else 'missing'}"
                )
            elif rti_closed_resume_failure:
                progress = (
                    "resume-failure-trigger="
                    f"{'present' if (directory / 'resume-failure-trigger.ok').is_file() else 'missing'}, "
                    "fault-proxy="
                    f"{proxy.diagnostic_state() if proxy is not None else 'missing'}, "
                    "tck-completion="
                    f"{'present' if tck_complete.is_file() else 'missing'}"
                )
            elif raw_frame_fault:
                progress = (
                    "message-history-completion="
                    f"{'present' if tck_complete.is_file() else 'missing'}"
                )
            else:
                progress = (
                    "join-response-ready="
                    f"{'present' if (directory / 'join-response-ready.ok').is_file() else 'missing'}, "
                    "join-response-socket-closed="
                    f"{'present' if (directory / 'join-response-socket-closed.ok').is_file() else 'missing'}, "
                    "tck-completion="
                    f"{'present' if tck_complete.is_file() else 'missing'}"
                )
            fixture_state = (
                "running" if fixture.poll() is None else f"exited:{fixture.returncode}"
            )
            result_state = "present" if result_path.is_file() else "missing"
            raise RuntimeError(
                f"The TCK timed out in {model} callback mode "
                f"({progress}; fixture={fixture_state}; result={result_state})"
            ) from error
        if return_code != 0:
            proxy_state = (
                f"; fault-proxy={proxy.diagnostic_state()}"
                if proxy is not None
                else ""
            )
            raise RuntimeError(
                f"The TCK failed in {model} callback mode "
                f"(exit {return_code}){proxy_state}"
            )
        fault_evidence: dict[str, object]
        if session_purge:
            fault_evidence = {
                "member_transport_closed_after_public_join": True,
                "member_reconnects_blocked_until_purge": True,
                "rti_timeout_heart_ms": 40,
                "rti_timeout_purge_ms": 160,
            }
        elif raw_frame_fault:
            if proxy is None:
                raise RuntimeError("The raw-frame message-history proxy was not started")
            fault_evidence = proxy.evidence()
        else:
            if not (directory / "join-response-socket-closed.ok").is_file():
                raise RuntimeError(
                    "The fixture did not confirm the Join-response socket fault"
                )
            fault_evidence = {"dropped_join_response_before_send": True}
        (directory / "fault-evidence.json").write_text(
            json.dumps(fault_evidence, indent=2) + "\n", encoding="utf-8"
        )
        if not tck_complete.is_file():
            raise RuntimeError("The TCK did not confirm public API recovery")
        if rti_closed_resume_failure and proxy is not None:
            # The failed-session path can leave the process fixture servicing
            # reconnect attempts after the public TCK has recorded
            # ConnectionLost. Close the test relay, then stop this fixture-only
            # server process rather than waiting on its unrelated retry loop.
            proxy.close()
            fixture_status = fixture.poll()
            if fixture_status is None:
                fixture.terminate()
                try:
                    fixture.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    fixture.kill()
                    fixture.wait()
            elif fixture_status != 0:
                raise RuntimeError(
                    "The process fixture failed after the failed-resume case "
                    f"(exit {fixture_status})"
                )
        elif fixture.wait(timeout=max(0.0, deadline - time.monotonic())) != 0:
            raise RuntimeError("The process fixture failed after the TCK recovery case")

        payload = load_tck_evidence(result_path) if result_path.is_file() else empty_tck_evidence()
        junit_root = ET.parse(junit_path).getroot() if junit_path.is_file() else ET.Element("testsuite")
        return return_code, payload, junit_root
    finally:
        if tck is not None and tck.poll() is None:
            tck.terminate()
            try:
                tck.wait(timeout=5)
            except subprocess.TimeoutExpired:
                tck.kill()
                tck.wait()
        if proxy is not None:
            proxy.close()
        if purge_proxy is not None:
            purge_proxy.close()
        if fixture.poll() is None:
            fixture.terminate()
            try:
                fixture.wait(timeout=5)
            except subprocess.TimeoutExpired:
                fixture.kill()
                fixture.wait()


def main() -> int:
    arguments = parse_arguments()
    models = ("evoked", "immediate") if arguments.callback_model == "both" else (arguments.callback_model,)
    payloads: list[TckEvidencePayload] = []
    junit_roots: list[ET.Element] = []
    temporary: tempfile.TemporaryDirectory[str] | None = None
    if arguments.artifact_directory:
        root = arguments.artifact_directory
        root.mkdir(parents=True, exist_ok=True)
    else:
        temporary = tempfile.TemporaryDirectory(prefix="umbra-message-history-tck-")
        root = Path(temporary.name)
    try:
        for model in models:
            _, payload, junit_root = run_model(arguments, model, root / model)
            if payload.document:
                payloads.append(payload)
            junit_roots.append(junit_root)
        if arguments.artifact_directory or arguments.results or arguments.junit:
            combined_results = combine_tck_evidence(
                payloads, arguments.callback_model
            )
            combined_results["transport_fault_evidence"] = {
                arguments.scenario: {
                    model: json.loads(
                        (root / model / "fault-evidence.json").read_text(
                            encoding="utf-8"
                        )
                    )
                    for model in models
                }
            }
            encoded_results = json.dumps(combined_results, indent=2) + "\n"
            if arguments.artifact_directory:
                (root / "results.json").write_text(
                    encoded_results, encoding="utf-8"
                )
            if arguments.results:
                arguments.results.parent.mkdir(parents=True, exist_ok=True)
                arguments.results.write_text(encoded_results, encoding="utf-8")
            suite = ET.Element("testsuite", tests=str(len(junit_roots)))
            for child in junit_roots:
                for case in list(child):
                    suite.append(case)
            if arguments.artifact_directory:
                ET.ElementTree(suite).write(
                    root / "results.xml", encoding="utf-8", xml_declaration=True
                )
            if arguments.junit:
                arguments.junit.parent.mkdir(parents=True, exist_ok=True)
                ET.ElementTree(suite).write(
                    arguments.junit, encoding="utf-8", xml_declaration=True
                )
        print(f"{arguments.scenario}: passed in {', '.join(models)} callback mode(s)")
        if arguments.artifact_directory:
            print(f"Evidence: {root}")
        return 0
    except (OSError, RuntimeError, ValueError, subprocess.SubprocessError) as error:
        print(f"message-history recovery adapter failed: {error}", file=sys.stderr)
        return 1
    finally:
        if temporary is not None:
            temporary.cleanup()


if __name__ == "__main__":
    raise SystemExit(main())
