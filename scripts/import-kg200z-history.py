"""Reproduce the initial provenance import from pinned local Git objects."""

import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess


ROOT = Path(__file__).resolve().parents[1]
VERIFY_ONLY = False


def sha(data):
    return hashlib.sha256(data).hexdigest()


def git(root, *args):
    return subprocess.check_output(["git", "-C", str(root), *args])


def save(path, text):
    if VERIFY_ONLY:
        if not path.is_file() or path.read_bytes() != text.encode("utf-8"):
            raise ValueError("import/source reproduction mismatch: " + str(path.relative_to(ROOT)))
        return
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8", newline="\n")


def redact(text):
    text = re.sub(r"\bCOM\d+\b", "[historical serial port]", text)
    text = re.sub(r"[A-Z]:[\\/][^\s`\"<>]*", "[local tool path]", text)
    return text


def main():
    global VERIFY_ONLY
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--host-root", type=Path, required=True)
    parser.add_argument("--check", action="store_true",
                        help="reproduce sources/excerpts in memory and compare without writing")
    args = parser.parse_args()
    VERIFY_ONLY = args.check
    config = json.loads((ROOT / "scripts/kg200z-import.json").read_text(encoding="utf-8"))
    baseline = config["baseline"]
    manifest = {"schema_version": 1, "baseline": baseline,
                "repositories": config["repositories"], "sources": [],
                "documents": [], "excluded": config["excluded"]}
    lookup = {}
    source_bytes = {}

    def add_source(repository, path):
        key = repository + ":" + path
        if key in lookup:
            return lookup[key]
        root = args.host_root if repository == "host" else ROOT
        commit = baseline["host_commit" if repository == "host" else "framework_commit"]
        data = git(root, "show", commit + ":" + path)
        item = {"id": "S%03d" % (len(manifest["sources"]) + 1),
                "repository": repository, "commit": commit, "path": path,
                "git_blob": git(root, "rev-parse", commit + ":" + path).decode().strip(),
                "sha256": sha(data),
                "url": config["repositories"][repository] + "/blob/" + commit + "/" + path}
        manifest["sources"].append(item)
        lookup[key] = item["id"]
        source_bytes[key] = data
        return item["id"]

    for entry in config["imports"]:
        source_id = add_source("host", entry["source"])
        text = source_bytes["host:" + entry["source"]].decode("utf-8-sig")
        lines = text.splitlines()
        is_draft = entry["target"].endswith("guideline-draft.md")
        document = {"path": entry["target"], "kind": "excerpt",
                    "status": "unverified" if is_draft else "historical",
                    "source_ids": [source_id], "source_path": entry["source"],
                    "applicable_version": "historical; verified runtime commit undetermined",
                    "transformation": "Select line ranges; normalize LF; redact serial port and local tool paths. No semantic rewrites.",
                    "note": entry["note"], "fragments": []}
        output = ["# " + entry["title"], "",
                  "> Authority: review draft, not an active rule." if is_draft else
                  "> Authority: historical source excerpt, not current API or current verification.",
                  "> Source: " + source_id + ", host `" + baseline["host_commit"] + "`,",
                  "> original path `" + entry["source"] + "`.", "", entry["note"], "",
                  "The source manifest preserves original file/blob hashes and selected ranges.",
                  "Old paths and commands describe the historical host, not this library.", ""]
        for index, (start, end) in enumerate(entry["ranges"], 1):
            if start < 1 or end > len(lines) or end < start:
                raise ValueError("invalid source range: " + entry["source"])
            original = "\n".join(lines[start - 1:end]) + "\n"
            if sum(line.startswith("```") for line in original.splitlines()) % 2:
                raise ValueError("source range splits a code fence: " + entry["source"])
            fragment = redact(original)
            fragment_id = source_id + "-F" + str(index)
            document["fragments"].append({"id": fragment_id, "lines": [start, end],
                                          "original_sha256": sha(original.encode()),
                                          "sha256": sha(fragment.encode())})
            output.extend(["## Source lines %d-%d" % (start, end), "",
                           "<!-- SOURCE_FRAGMENT " + fragment_id + " -->\n" +
                           fragment + "<!-- END_SOURCE_FRAGMENT " + fragment_id + " -->", ""])
        save(ROOT / entry["target"], "\n".join(output))
        manifest["documents"].append(document)

    for path in config["additional_host_sources"]:
        add_source("host", path)
    files = git(ROOT, "ls-tree", "-r", "--name-only", baseline["framework_commit"]).decode().splitlines()
    for path in files:
        if path.endswith((".h", ".c")) or path == "README.md":
            add_source("framework", path)
    for item in config["derived"]:
        source_ids = []
        for key in item["sources"]:
            repository, path = key.split(":", 1)
            source_ids.append(add_source(repository, path))
        manifest["documents"].append({"path": item["path"], "kind": "derived",
                                      "status": "unverified" if "gap-review" in item["path"] else "active",
                                      "source_ids": source_ids,
                                      "transformation": "Authored synthesis from cited sources; fresh observations identified in the completion record."})

    api_headers = ["bus/dev/dri_ops.h", "bus/dev/dev_gpio.h", "bus/dev/dev_i2c.h",
                   "bus/dev/dev_spi.h", "bus/dev/dev_uart.h", "bus/dev/dev_timer.h",
                   "bus/dev/dev_pwm.h", "component/ring_buf.h"]
    output = ["# Public header contract snapshot", "",
              "Framework baseline: `" + baseline["framework_commit"] + "`.", "",
              "These are public-header excerpts, not a firmware/source distribution.",
              "They document the actual declarations including limits and legacy naming.",
              "They do not establish behavior not specified by the implementation.",
              "The archive has no source files; source links require the matching Git repository.", ""]
    api_sources = []
    for path in api_headers:
        source_id = lookup["framework:" + path]
        api_sources.append(source_id)
        text = source_bytes["framework:" + path].decode("utf-8-sig").rstrip()
        output.extend(["## " + path, "", "Source: " + source_id + ".", "", "```c", text, "```", ""])
    api_path = "docs/specs/api-inventory.md"
    save(ROOT / api_path, "\n".join(output))
    manifest["documents"].append({"path": api_path, "kind": "api-snapshot", "status": "active",
                                  "source_ids": api_sources, "transformation": "Public headers quoted from the pinned framework commit; LF normalized."})
    history = git(ROOT, "log", "--reverse", "--format=%H %ad %s", "--date=short",
                  baseline["framework_commit"]).decode("utf-8")
    history_path = "docs/logs/history/framework-git-history.md"
    save(ROOT / history_path, "# Framework commit history\n\n"
         "Source: local Git log ending at `" + baseline["framework_commit"] + "`.\n\n"
         "Commit subjects describe changes; they do not prove builds or hardware checks.\n\n"
         "```text\n" + history + "```\n")
    manifest["documents"].append({"path": history_path, "kind": "git-history", "status": "historical",
                                  "source_ids": [lookup["framework:README.md"]],
                                  "command": "git log --reverse --format='%H %ad %s' --date=short " + baseline["framework_commit"],
                                  "transformation": "Local Git commit metadata, not inferred completion evidence."})
    save(ROOT / "scripts/docs-sources.json", json.dumps(manifest, ensure_ascii=False, indent=2) + "\n")
    print("[IMPORT]", "Verified" if VERIFY_ONLY else "Wrote", len(manifest["documents"]), "document provenance records from",
          len(manifest["sources"]), "pinned sources")


if __name__ == "__main__":
    main()
