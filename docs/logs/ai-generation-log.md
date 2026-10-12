# Framework AI Provenance

## 2026-10-10 - Portable documentation library

- Agent: Codex in the shared checkout; approved plan executed without commit/push.
- Initial framework: `d29419be990aa562bcbacabb17ac9c78b31a0908`, clean.
- Initial host: `8ab7a95033b30b5848def22971a35bacab766645`, clean.
- Sources: local Git objects, current source, existing AGENTS/rules and indexed
  historical documentation. Source IDs, file/blob hashes and selections are
  registered in `scripts/docs-sources.json` and `scripts/kg200z-import.json`.
- Output: AI entry, current architecture/API, scoped rules/drafts, selected
  history, evidence mapping and document checks/export tools.
- Verification: exact fresh results are in the completion record. Negative
  document-tool tests were written and failed before the tool existed.
- Limitation: no firmware build, flash, runtime observation or independent
  reviewer subagent was available/performed in this document task. Self-review
  and executable document-tool acceptance are reported separately.
