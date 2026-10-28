"""Provider-neutral, bounded JSON edits. No model, web or RTI dependency.

There are deliberately no copy/move/reference operations: a small input cannot
recursively fan out a shared subtree. Every changed literal is present in the
file. Reconstructed states are still validated at the optional web boundary.
"""

# pyright: strict
from __future__ import annotations

import copy
import json
from dataclasses import dataclass
from typing import Literal, TypeAlias

from .models import JsonObject, JsonValue

COMPACT_RECORDING_VERSION = "umbra-observer-recording-v2"
COMPACT_CODEC = "observer-json-edits-v1"
MAX_STATE_BYTES = 8 * 1024 * 1024
MAX_DECODED_STATE_BYTES = 1024 * 1024 * 1024
MAX_FRAME_EDITS = 20000
MAX_TOTAL_EDITS = 100000
MAX_PATH_DEPTH = 96
MAX_STATE_NODES = 750000
Path: TypeAlias = tuple[str | int, ...]


class EditError(ValueError):
    """An edit does not address the stated container exactly."""


class StateLimit(ValueError):
    """A reconstructed state exceeds the documented replay work envelope."""


@dataclass(frozen=True, slots=True)
class SetValue:
    path: Path
    value: JsonValue

    def to_json(self) -> JsonObject:
        return {"op": "set", "path": list(self.path), "value": self.value}


@dataclass(frozen=True, slots=True)
class DeleteKey:
    path: Path

    def to_json(self) -> JsonObject:
        return {"op": "delete", "path": list(self.path)}


@dataclass(frozen=True, slots=True)
class SpliceList:
    path: Path
    index: int
    delete_count: int
    items: list[JsonValue]

    def to_json(self) -> JsonObject:
        return {
            "op": "splice",
            "path": list(self.path),
            "index": self.index,
            "delete_count": self.delete_count,
            "items": self.items,
        }


Edit: TypeAlias = SetValue | DeleteKey | SpliceList
StorageFormat: TypeAlias = Literal["full-v1", "compact-v2"]


def encode(value: JsonValue) -> str:
    return json.dumps(value, ensure_ascii=False, allow_nan=False, separators=(",", ":"))


def state_size(value: JsonObject) -> int:
    """Bound depth/nodes before serialization; count UTF-8 bytes, not process RSS."""
    pending: list[tuple[JsonValue, int]] = [(value, 0)]
    nodes = 0
    while pending:
        current, depth = pending.pop()
        nodes += 1
        if depth > MAX_PATH_DEPTH or nodes > MAX_STATE_NODES:
            raise StateLimit("Reconstructed state exceeds depth/node limits.")
        if isinstance(current, dict):
            pending.extend((item, depth + 1) for item in current.values())
        elif isinstance(current, list):
            pending.extend((item, depth + 1) for item in current)
    size = len(encode(value).encode("utf-8"))
    if size > MAX_STATE_BYTES:
        raise StateLimit("Reconstructed state exceeds 8 MiB.")
    return size


def _same(before: JsonValue, after: JsonValue) -> bool:
    # Python considers True == 1. JSON booleans and numbers are not interchangeable.
    if type(before) is not type(after):
        return False
    if isinstance(before, (dict, list)):
        return encode(before) == encode(after)
    return before == after


def _overlap(before: list[JsonValue], after: list[JsonValue]) -> int:
    """Linear token KMP: retained suffix / new prefix, never quadratic searching."""
    old = [encode(item) for item in before]
    new = [encode(item) for item in after]
    tokens: list[str | None] = [*new, None, *old]
    matched = [0] * len(tokens)
    for index in range(1, len(tokens)):
        length = matched[index - 1]
        while length and tokens[index] != tokens[length]:
            length = matched[length - 1]
        if tokens[index] == tokens[length]:
            length += 1
        matched[index] = length
    return matched[-1] if matched else 0


def difference(before: JsonObject, after: JsonObject) -> list[Edit]:
    result: list[Edit] = []

    class DenseDelta(Exception):
        pass

    def add(edit: Edit) -> None:
        if len(result) >= MAX_FRAME_EDITS:
            raise DenseDelta
        result.append(edit)

    def visit(old: JsonValue, new: JsonValue, path: Path) -> None:
        if _same(old, new):
            return
        if len(path) >= MAX_PATH_DEPTH:
            add(SetValue(path, new))
        elif isinstance(old, dict) and isinstance(new, dict):
            for key in old:
                if key not in new:
                    add(DeleteKey((*path, key)))
            for key, value in new.items():
                if key in old:
                    visit(old[key], value, (*path, key))
                else:
                    add(SetValue((*path, key), value))
        elif isinstance(old, list) and isinstance(new, list):
            prefix = 0
            while prefix < min(len(old), len(new)) and _same(old[prefix], new[prefix]):
                prefix += 1
            suffix = 0
            while suffix < min(len(old), len(new)) - prefix and _same(
                old[len(old) - suffix - 1], new[len(new) - suffix - 1]
            ):
                suffix += 1
            if prefix == 0 and suffix == 0 and old and new:
                overlap = _overlap(old, new)
                if overlap >= min(len(old), len(new)) // 2 and overlap:
                    add(SpliceList(path, 0, len(old) - overlap, []))
                    if len(new) > overlap:
                        add(SpliceList(path, overlap, 0, new[overlap:]))
                    return
            if len(old) == len(new):
                for index in range(prefix, len(new) - suffix):
                    visit(old[index], new[index], (*path, index))
            else:
                add(
                    SpliceList(
                        path,
                        prefix,
                        len(old) - prefix - suffix,
                        new[prefix : len(new) - suffix if suffix else len(new)],
                    )
                )
        else:
            add(SetValue(path, new))

    try:
        visit(before, after, ())
    except DenseDelta:
        # Stop constructing an excessive program, not merely after allocating it.
        return [SetValue((), after)]
    return result


def _child(container: JsonValue, key: str | int) -> JsonValue:
    if isinstance(container, dict) and isinstance(key, str) and key in container:
        return container[key]
    if isinstance(container, list) and type(key) is int and 0 <= key < len(container):
        return container[key]
    raise EditError(
        "Edit path must address an existing own key or bounded array index."
    )


def apply_edits(state: JsonObject, edits: list[Edit]) -> JsonObject:
    """Mutate only an owned working tree; callers clone before retaining a view."""
    if len(edits) > MAX_FRAME_EDITS:
        raise EditError("Too many edits in a frame.")
    root = state
    for edit in edits:
        if len(edit.path) > MAX_PATH_DEPTH or any(
            type(key) not in (int, str) for key in edit.path
        ):
            raise EditError("Invalid edit path.")
        target: JsonValue = root
        if isinstance(edit, SpliceList):
            for key in edit.path:
                target = _child(target, key)
            if (
                not isinstance(target, list)
                or type(edit.index) is not int
                or type(edit.delete_count) is not int
                or not 0 <= edit.index <= len(target)
                or not 0 <= edit.delete_count <= len(target) - edit.index
            ):
                raise EditError("Splice must stay within an existing array.")
            target[edit.index : edit.index + edit.delete_count] = copy.deepcopy(
                edit.items
            )
            continue
        if not edit.path:
            if not isinstance(edit, SetValue) or not isinstance(edit.value, dict):
                raise EditError(
                    "Only an object replacement can address the state root."
                )
            root = copy.deepcopy(edit.value)
            continue
        for key in edit.path[:-1]:
            target = _child(target, key)
        key = edit.path[-1]
        if isinstance(target, dict) and isinstance(key, str):
            if isinstance(edit, DeleteKey):
                if key not in target:
                    raise EditError("Delete requires an existing object key.")
                del target[key]
            else:
                target[key] = copy.deepcopy(edit.value)
        elif (
            isinstance(edit, SetValue)
            and isinstance(target, list)
            and type(key) is int
            and 0 <= key < len(target)
        ):
            target[key] = copy.deepcopy(edit.value)
        else:
            raise EditError(
                "Set/delete must address the stated container type exactly."
            )
    return root
