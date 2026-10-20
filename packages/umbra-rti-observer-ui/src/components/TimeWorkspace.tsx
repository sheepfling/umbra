import type { AnalyticsResponse, StateResponse } from "../api/types";
import { EmptyPanel } from "./ChartPanel";
import { PersistentTimeView } from "./PersistentTimeView";
import { TimeView } from "./TimeView";

export function TimeWorkspace({ state, analytics, federate, participantLabel }: {
  state: StateResponse; analytics?: AnalyticsResponse; federate?: string; participantLabel?: string;
}) {
  return <>
    <PersistentTimeView state={state} federate={federate} />
    {analytics ? <TimeView analytics={analytics} capability={state.capabilities.logical_time} federate={federate} participantLabel={participantLabel} />
      : <EmptyPanel title="Time analytics unavailable">Windowed timestamp analytics is unavailable. Persistent time inspection above is independent of analytics and MOM.</EmptyPanel>}
  </>;
}
