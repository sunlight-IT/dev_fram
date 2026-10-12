# Portable Framework Documentation Library Plan

Approved scope: documentation inside the existing dev_fram submodule; bus/port,
DMA, ring buffer, common helpers and TIM/PWM only; documents plus provenance;
existing rules remain distinct from unapproved cross-project guidelines.

Sources: pinned host AGENTS/document governance and framework README/source;
the maintainers' approved planning conversation establishes this deliverable.

## Tasks And Acceptance

1. Freeze host/framework commits and dirty state. Inventory candidate docs and
   register exclusions instead of copying host protocols, facades or workflows.
2. Create an AI entry, scoped existing rules, current architecture/API snapshot,
   host integration contract, path mapping and guideline gap table.
3. Import selected historical source fragments from pinned Git objects; record
   original file/blob SHA-256, ranges, transformed fragment hashes and authority.
4. Keep framework changelog, AI provenance and reproducible completion records
   versioned with the framework. Host-only board tests remain in the host.
5. Implement document inventory, link/source checks and ZIP export with negative
   checks and relocation verification. Update the host navigation and index.

Deliver a ZIP plus checksum sidecar. Include document tools and their acceptance
tests; exclude firmware tests, code files, SDKs, binaries, `.git`, machine config
and generated exports. No firmware build or board verification is required for
this document-only task, and no old result becomes current runtime proof.

The initial source and API baseline remains immutable. Later framework changes
must update current-state/API docs with new source records. Do not rewrite old
excerpts to match new code. Publish documentation with the corresponding code
commit; the parent gitlink changes only after a maintainer commits the submodule.
