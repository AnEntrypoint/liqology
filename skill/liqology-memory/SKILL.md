# liqology-memory

Wraps LLM/agent calls to build a continuously-updating ontology of what those
calls actually produced, so future calls can retrieve relevant prior
input/output pairs instead of re-deriving them, without unboundedly growing
the memory.

## What this skill does

Every time an agent call completes, this skill:

1. Embeds the call's input and output text.
2. Calls `liqology::MemoryStore::record(input_embedding, output_embedding)`.
3. `record` internally:
   - Compares the new output embedding against every existing entry's output
     embedding via FAISS (`liqology::DisambiguationIndex`).
   - Reinforces (raises `retention_weight` on) existing entries whose output
     is similar to the new one — relevance is judged output-to-output, since
     what an interaction *produced* is the signal that a later call would
     actually want retrieved, not what was merely asked.
   - Decays every entry's `retention_weight` by one tick
     (`CostBalancePolicy::decay_per_tick`).
   - Prunes any entry whose `retention_weight` has fallen below
     `prune_threshold` **and** whose estimated re-informing value
     (`reinform_cost * retention_weight`) no longer exceeds the flat
     `retain_cost` of keeping it. See
     `docs/adr/0002-reinforce-prune-cost-balance.md` for why both conditions
     are required, not either alone.
4. Optionally runs the retained entries' hidden states through
   `liqology::LiquidCore::refine` so bucket-level state (see
   `liqology::Bucket`) adapts to the new context at the same high-speed local
   optimization step the liquid net already provides — the memory layer does
   not duplicate that dynamics, it feeds `MemoryStore` output as the liquid
   core's per-tick context embedding.

## Invocation shape

An agent/skill runtime calling this wraps each LLM call as:

```
result = record(embed(call.input), embed(call.output))
```

`result.value()` (a `std::vector<InteractionId>`) lists ids evicted this
call — log these if the caller wants an audit trail of what was forgotten
and when.

## Why output-to-input comparison, not input-to-input

Two calls with near-identical inputs can produce very different outputs
(different context, different retrieved memory already in play) — matching
on input alone would reinforce entries whose *content* has nothing to do
with the new one. Matching each new output against prior outputs keeps
reinforcement tied to actual produced knowledge, which is what a later
retrieval would want back.

## Tuning `CostBalancePolicy`

- `reinform_cost` — set higher when re-deriving a pruned-then-needed entry
  is expensive (e.g. it required an expensive tool call or long generation).
- `retain_cost` — set higher when storage/retrieval noise from irrelevant
  entries measurably degrades later disambiguation quality.
- `prune_threshold` — the retention floor; entries never reinforced decay
  past it after `1 / decay_per_tick` ticks.
- `reinforce_gain` — how strongly a similar new output boosts an existing
  entry; scaled by the FAISS inner-product similarity score.
- `similarity_floor` — the minimum FAISS inner-product score before a match
  counts as "similar" and gets reinforced at all. Set higher for embeddings
  where even unrelated content scores moderately (e.g. short text with a
  narrow vocabulary); set lower for embeddings that spread near-orthogonal
  by default. Without this floor every stored entry gets reinforced on
  every call (querying with `k = index size` returns all of them), which
  defeats relevance-based reinforcement entirely.
