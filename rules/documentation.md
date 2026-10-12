# Framework Documentation Rules

Sources: host AGENTS sections 6 and 11, `.codex/rules/documentation.md` and
`docs/README.md`, recorded at the host baseline in `scripts/docs-sources.json`.

- Use `docs/logs`, `docs/plans`, `docs/specs`, `docs/completions` for history,
  intent, constraints/current state and verifiable evidence respectively.
- Read the generated index before locating documents. Preserve draft authority.
  Do not infer implementation or success from titles, checked boxes or commits.
- Record the framework revision, dirty state, host revision/profile, exact
  commands, results, limitations and the actual scope of every verification.
- Distinguish static checks, host tests, firmware builds, programming and runtime
  observations. `Verified OK` proves programming verification only.
- Keep credentials, probe identifiers, serial port numbers and machine paths
  out of tracked docs. Redact these in imports and record the transformation.
- Retain historical paths in labeled excerpts. Current docs use framework-relative
  paths and links within the library; a source pointer is not a local link.
- `scripts/docs-sources.json` records pinned Git objects and hashes. Register
  new authored docs with their source records before updating the index.
  Never fabricate source, execution output or validation status.
- Imported fragments are immutable evidence. Correct misunderstandings in a
  separate current-state note. Do not silently edit an excerpt's source/hash.
- After adding, changing or removing a document run `docs-library.py update`
  and `check`. The index covers docs, rules and export tools; AGENTS is checked
  through its generated index, full-file hash and archive checksum inventory.
- Re-importing history requires both original local Git repositories and the
  pinned commits. It rebuilds initial imports and provenance; it is not a normal
  update command. Preserve later provenance records before a deliberate re-import.
- Reproduce original source hashes, fragments, API snapshot and commit metadata
  without writing using `import-kg200z-history.py --host-root <host> --check`.
  This requires pinned Git objects; ordinary library checks work without Git.
- Update shared framework records in this repository when working from another
  host. Host-only board tests stay in the host, with a concise versioned evidence
  pointer here. Publish framework documentation together with its code revision,
  then update the host gitlink through the maintainer's normal Git workflow.
