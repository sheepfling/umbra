from __future__ import annotations

import query_rti_work
from query_rti_work_regression_context import MappingLookupInputs, MappingRowInputs
from query_rti_work_regression_mapping_mom import run_mom_mapping_rows
from query_rti_work_regression_mapping_object import run_object_mapping_rows
from query_rti_work_regression_mapping_process import run_process_mapping_rows
from query_rti_work_regression_mapping_time import run_time_mapping_rows


def run_mapping_rows(inputs: MappingRowInputs) -> query_rti_work.JsonObject:
    lookup_inputs = MappingLookupInputs(index=inputs.index, tests=inputs.tests)
    run_object_mapping_rows(inputs)
    run_time_mapping_rows(lookup_inputs)
    run_process_mapping_rows(lookup_inputs)
    return run_mom_mapping_rows(lookup_inputs)
