"""Internal lossless codec checks, independent of an RTI or web framework."""
from __future__ import annotations

import copy
import random

import pytest

from umbra_rti_observer.recording_codec import (
    DeleteKey, EditError, SetValue, SpliceList, StateLimit, apply_edits, difference, encode, state_size,
)


@pytest.mark.parametrize("before,after", [
    ({"x": True}, {"x": 1}), ({"x": 1.0}, {"x": 1}),
    ({"x": None}, {}), ({}, {"x": None}),
    ({"x": list(range(100))}, {"x": list(range(10, 110))}),
    ({"x": [1, 2]}, {"x": [1, 3, 2]}),
    ({"x": [1, 2, 3]}, {"x": [1, 3]}),
    ({"x": [{"a": "first", "v": 1}, {"a": "second", "v": 2}]}, {"x": [{"a": "first", "v": 3}, {"a": "second", "v": 2}]}),
    ({"__proto__": {"constructor": 1}}, {"__proto__": {"constructor": 2}}),
])
def test_exact_json_round_trip_and_literals_are_not_mutated(before, after):
    edits = difference(before, after)
    saved = copy.deepcopy(edits)
    result = apply_edits(copy.deepcopy(before), edits)
    assert encode(result) == encode(after)
    apply_edits(result, [SetValue(("later",), True)])
    assert edits == saved


def test_random_list_and_nested_edits_round_trip():
    randomizer = random.Random(23)
    for _ in range(100):
        before = {"items": [{"id": index, "v": randomizer.choice([None, True, 1, 1.0, "x"])} for index in range(20)]}
        after = copy.deepcopy(before)
        after["items"][randomizer.randrange(20)]["v"] = randomizer.random()
        if randomizer.randrange(2):
            after["items"] = after["items"][randomizer.randrange(10):] + [{"id": 99, "v": None}]
        assert apply_edits(copy.deepcopy(before), difference(before, after)) == after


def test_rolling_history_is_two_splices_not_a_full_window_replacement():
    before = {"rows": [{"sequence": index, "payload": "x" * 100} for index in range(2000)]}
    after = {"rows": [{"sequence": index, "payload": "x" * 100} for index in range(100, 2100)]}
    edits = difference(before, after)
    assert len(edits) == 2 and all(isinstance(edit, SpliceList) for edit in edits)
    assert len(encode([edit.to_json() for edit in edits])) < len(encode(after)) / 10


@pytest.mark.parametrize("edit", [
    DeleteKey(()), DeleteKey(("missing",)), DeleteKey(("array", 0)),
    SetValue(("array", 2), 1), SetValue(("array", True), 1), SetValue(("array", "0"), 1),
    SetValue(("missing", "x"), 1), SetValue((), []),
    SpliceList(("array",), 2, 0, []), SpliceList(("array",), 0, 2, []),
    SpliceList(("object",), 0, 0, []),
])
def test_invalid_container_paths_indices_and_splices_are_rejected(edit):
    with pytest.raises(EditError):
        apply_edits({"array": [0], "object": {}}, [edit])


def test_state_budget_rejects_oversize_depth_nonfinite_and_wide_values():
    with pytest.raises(StateLimit): state_size({"large": "x" * (8 * 1024 * 1024)})
    nested = {}
    for _ in range(100): nested = {"x": nested}
    with pytest.raises(StateLimit): state_size(nested)
    with pytest.raises(ValueError): state_size({"x": float("nan")})


def test_dense_edits_fall_back_before_building_an_excessive_program():
    before = {str(index): 0 for index in range(25000)}
    after = {str(index): 1 for index in range(25000)}
    edits = difference(before, after)
    assert edits == [SetValue((), after)]
    assert apply_edits(copy.deepcopy(before), edits) == after
