# 1. LDNO dual-branch backend

## Status

Rejected (deferred, with a concrete re-open trigger)

## Context

LDNO ("A low-power dynamic neural operator inspired by liquid state machines
for solving partial differential equations", ScienceDirect) is a dual-branch
architecture: an LSM (Liquid State Machine, event-driven spiking) branch for
dynamic temporal feature encoding, plus an LSTM branch for long-horizon
dependencies, fused via an adaptive cross-fusion module, with a multi-strategy
DownSampling module reducing the LSM branch's input dimensionality. It targets
time-dependent PDE operator learning and reports 80-99% inference energy
reduction versus FNO/WNO baselines when run on neuromorphic-inspired hardware
substrates.

liqology's `LiquidCore` (PLAN-style, arxiv 2608.03041) implements a
parallelizable Euler-discretized liquid time-constant update over
neuron-connectable buckets, for ontology disambiguation and bucket-grouping.
The open question for this ADR: should an LDNO-style dual-branch (LSM+LSTM)
core be added as a second, pluggable `LiquidCore`-shaped backend
(`refine(Span<Bucket>, Span<const float>) -> Result<void, Error>`)?

Two premises of LDNO do not hold for liqology as currently scoped:

1. LDNO's energy-reduction case is built on event-driven spiking dynamics
   executing on neuromorphic-inspired silicon. liqology runs as an ordinary
   C++ service on conventional CPUs; there is no spiking substrate to exploit
   here, so the claimed energy advantage does not transfer.
2. LSM's core structural advantage over a plain recurrent/parallel update is
   sparse, event-driven encoding of a *long temporal stream* — the LSTM
   branch exists specifically to carry long-horizon dependencies that the LSM
   branch's short-memory reservoir cannot. liqology's query model (see
   `include/liqology/query_context.hpp` and its dissolution invariant,
   `R_c(A,B) = 0` for `t > t_ack`) treats an ontology as a value constructed
   fresh per query and torn down at acknowledgment — a static function of one
   query's context, not a long-running temporal stream with cross-query state
   to encode. There is no long horizon for an LSTM branch to track, and no
   event stream for an LSM branch to sparsify.

## Decision

Defer adoption of an LDNO-style dual-branch backend. It is not added now, and
`liquid_core.hpp`'s pluggable-backend surface should not be pre-shaped around
it speculatively (YAGNI) — the PLAN-style parallel core remains the only
`LiquidCore` implementation.

This is re-opened, and an `LdnoCore` becomes worth building, if and only if
liqology grows a **live streaming ontology-update mode**: entities/relations
arriving continuously as an event stream, where refinement must track
long-horizon dependencies *across* queries rather than within one query's
transient context — i.e. the moment `DissolvingQueryContext`'s per-query dissolution
invariant stops describing the actual workload. At that point the LSM
branch's event-driven sparsity and the LSTM branch's long-horizon memory both
become genuinely applicable, and an `LdnoCore` should be scoped with the same
interface shape as `LiquidCore` (`refine(Span<Bucket>, Span<const float>) ->
Result<void, Error>`) so it plugs in without disturbing callers.

## Consequences

- No new dependency, no new backend surface, no speculative interface
  branching in `liquid_core.hpp` right now.
- The trigger condition above is the explicit, falsifiable re-open test for
  this decision — not "revisit later," a specific workload shape to watch
  for.
- If a streaming mode is added later without revisiting this ADR, that is a
  process gap: the streaming-mode PRD row should reference this ADR and
  re-open it rather than silently bolting temporal machinery onto the
  existing PLAN core.
