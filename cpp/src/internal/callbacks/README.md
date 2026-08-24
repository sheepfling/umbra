# Callback-delivery internals

This is the smallest runtime domain. It owns the private dispatcher and
callback-session bookkeeping that implement immediate and evoked callback
delivery. It does not decide whether an RTI service is legal or what
federation state a callback represents.

## Key files

- callback_dispatcher files queue, enable, disable, and invoke callback work.
- callback_session files bind a callback model and instrumentation state to a
  connected federate session.

## Working here

- Add callback queueing, cancellation, or invocation mechanics here.
- The producer of a callback remains responsible for validating runtime state
  before it queues work and, where required, before invocation.
- Use [observability](../observability/README.md) for timing and record
  capture; do not add service-report formatting here.
- Do not store federation or time policy in the dispatcher. Use the
  [federation](../federation/README.md) or [time](../time/README.md) domain
  instead.

## Tests and references

Start with callback_dispatcher_catch2.cpp under
[cpp/tests/](../../../tests/). Broader integration cases exercise delivery
through federation and time behavior. The callback boundary is described in
[architecture](../../../../docs/architecture/ARCHITECTURE.md).
