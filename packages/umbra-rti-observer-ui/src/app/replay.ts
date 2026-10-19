/** Pure replay clock/navigation. Inputs have already passed the recording model. */
export function frameAt(frames: ReadonlyArray<{ offset_seconds: number }>, seconds: number): number {
  let left = 0, right = frames.length;
  while (left < right) {
    const middle = Math.floor((left + right) / 2);
    if (frames[middle]!.offset_seconds <= seconds) left = middle + 1;
    else right = middle;
  }
  return Math.max(0, left - 1);
}

export function advancePosition(origin: number, elapsedSeconds: number, speed: number, duration: number): number {
  return Math.max(0, Math.min(duration, origin + Math.max(0, elapsedSeconds) * speed));
}
