# 2. Reinforce/prune cost balance

## Status

Accepted

## Context

liqology's purpose (reframed from a generic disambiguation library) is to
codify LLM/agent interactions over time: each call's input/output pair is
embedded and stored, `MemoryStore` compares each new output embedding
against existing entries' output embeddings via FAISS
(`DisambiguationIndex`), reinforcing similar entries and decaying the rest,
then pruning entries that fall below a retention floor.

Two failure modes bound this design:

1. **Over-pruning** — evicting an entry that later turns out to still
   matter forces re-informing: re-deriving or re-fetching the same
   knowledge the next time it's needed, which costs at minimum another LLM
   call and at worst a correctness regression if the re-derivation drifts
   from the original.
2. **Over-retention** — never pruning grows the FAISS index unboundedly,
   which both costs storage and, more importantly, degrades every future
   `query()`: irrelevant entries dilute the top-k neighborhood and make
   genuinely relevant entries harder to surface, which is a correctness
   cost on every subsequent call, not just a storage cost.

Neither a pure LRU-by-age policy nor a pure similarity-threshold policy
balances these on its own: age ignores relevance entirely, and a
similarity-only threshold has no way to express that some entries are
expensive to reconstruct even if rarely matched.

A real-toolchain build against actual FAISS (not the hand-written stub used
in an earlier, incorrect verification pass — see `gm-config` commit
`ea56b6b`, "no scratch-path stubs") surfaced a genuine bug in the first
implementation: `record()` queried the FAISS index with `k =
relevance_index_.size()`, which returns every stored entry regardless of
actual similarity, and the original code reinforced all of them
unconditionally. This made every entry get reinforced on every call, so
nothing ever decayed relative to anything else and eviction never
triggered — a real dependency (real FAISS, actually returning low-score
matches for dissimilar embeddings) exposed a bug the stub's fake
always-empty `search()` implementation could never have surfaced.

## Decision

`MemoryStore::prune()` evicts an entry only when **both** hold:

- `retention_weight < prune_threshold` (the entry has decayed past the
  floor — it hasn't been reinforced by relevant activity recently), **and**
- `reinform_cost * retention_weight < retain_cost` (the *expected* cost of
  having to re-inform this entry later, discounted by how likely it is to
  still matter, no longer exceeds the flat cost of keeping it around).

Reinforcement itself is gated by `similarity_floor`: a stored entry only
gets reinforced by a new interaction when the FAISS inner-product score
between their output embeddings meets or exceeds this floor. Without it,
querying with `k = index size` (the only way to see every entry's score, not
just a fixed top-K) returns every entry, and reinforcing all of them
regardless of match quality collapses relevance-based reinforcement into
uniform reinforcement.

This is a direct implementation of the balance the reframed project purpose
names explicitly: "balancing the cost of re-informing against the cost of
overenforcing." `retention_weight` is the per-entry relevance signal
(raised by FAISS-similarity reinforcement, decayed every tick);
`reinform_cost`/`retain_cost` are the two cost estimates a caller tunes per
deployment (see `skill/liqology-memory/SKILL.md`'s tuning section).

`decay_per_tick` and `LiquidCore`'s per-bucket `tau` are deliberately
distinct knobs, not unified into one: `tau` governs how fast a bucket's
*liquid hidden state* (a continuous representation) responds to new
context, while `decay_per_tick` governs how fast a *discrete memory entry's
retention weight* fades absent reinforcement. Both express a half-life,
but at different layers (bucket dynamics vs. memory-entry lifecycle) with
independently tunable rates — collapsing them into one parameter would
force a deployment that wants slow bucket adaptation but fast memory
turnover (or vice versa) to choose one rate for both.

## Consequences

- An entry that is cheap to re-derive (`reinform_cost` low) prunes
  aggressively even under moderate `retention_weight` — correct, since
  over-retaining a cheap-to-reconstruct entry only pays the retain_cost
  side with no offsetting benefit.
- An entry that is expensive to re-derive survives longer at the same
  `retention_weight` — correct, since eviction risk is weighted by actual
  reconstruction cost, not relevance alone.
- This makes `CostBalancePolicy` a required, explicit input to
  `MemoryStore::Create` rather than a hardcoded constant — per-deployment
  tuning is a first-class concern, not an afterthought.
- Cross-reference: `docs/adr/0001-ldno-dual-branch-backend.md`'s re-open
  trigger ("a live streaming ontology-update mode... entities/relations
  arriving continuously as an event stream") is now the reframed project's
  actual intended workload (agent calls arriving continuously over time).
  `MemoryStore` as built here still treats each `record()` call as an
  independent tick with no long-horizon state beyond `retention_weight`
  and the FAISS index — if it later needs to track dependencies *across*
  many interactions (not just per-entry decay), ADR 0001 should be
  revisited rather than growing `MemoryStore` ad hoc.
