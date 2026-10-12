"""Exercise documentation checks and export using isolated real files."""

import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
import zipfile


ROOT = Path(__file__).resolve().parents[1]
TOOL = ROOT / "scripts/docs-library.py"


class DocumentationLibraryTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="dev-fram-docs-test-")
        self.root = Path(self.temp.name) / "library"
        (self.root / "scripts").mkdir(parents=True)
        (self.root / "docs/specs").mkdir(parents=True)
        (self.root / "rules").mkdir()
        self.assertTrue(TOOL.is_file(), "documentation library tool is missing")
        shutil.copyfile(TOOL, self.root / "scripts/docs-library.py")
        (self.root / "AGENTS.md").write_text("# Entry\n", encoding="utf-8")
        (self.root / "README.md").write_text(
            "# Library\n\n[State](docs/specs/state.md)\n", encoding="utf-8")
        (self.root / "docs/specs/state.md").write_text(
            "# State\n\nObserved source baseline.\n", encoding="utf-8")
        (self.root / "rules/coding.md").write_text("# Coding\n", encoding="utf-8")
        self.sources = {
            "schema_version": 1,
            "baseline": {"framework_commit": "d" * 40,
                         "host_commit": "8" * 40, "date": "2026-10-10"},
            "sources": [{"id": "S001", "repository": "framework",
                         "commit": "d" * 40, "path": "bus/dev/example.h",
                         "sha256": "a" * 64}],
            "documents": [{"path": "docs/specs/state.md", "kind": "derived",
                           "status": "active", "source_ids": ["S001"]}],
            "excluded": [],
        }
        self.save_sources()

    def tearDown(self):
        self.temp.cleanup()

    def save_sources(self):
        (self.root / "scripts/docs-sources.json").write_text(
            json.dumps(self.sources), encoding="utf-8")

    def run_tool(self, *args, expected=0):
        result = subprocess.run(
            [sys.executable, str(self.root / "scripts/docs-library.py"), *args],
            capture_output=True, text=True, encoding="utf-8")
        self.assertEqual(result.returncode, expected, result.stdout + result.stderr)
        return result

    def test_check_reports_changed_and_unregistered_document(self):
        self.run_tool("update")
        self.run_tool("check")
        (self.root / "docs/specs/state.md").write_text("# Changed\n", encoding="utf-8")
        result = self.run_tool("check", expected=1)
        self.assertIn("hash mismatch", result.stdout + result.stderr)
        self.run_tool("update")
        (self.root / "docs/specs/unregistered.md").write_text("# Added\n", encoding="utf-8")
        result = self.run_tool("check", expected=1)
        self.assertIn("inventory", result.stdout + result.stderr)

    def test_check_rejects_unresolved_source_and_broken_link(self):
        self.sources["documents"][0]["source_ids"] = ["S999"]
        self.save_sources()
        self.run_tool("update", expected=1)
        self.sources["documents"][0]["source_ids"] = ["S001"]
        self.save_sources()
        (self.root / "README.md").write_text("[Missing](docs/missing.md)\n", encoding="utf-8")
        result = self.run_tool("update", expected=1)
        self.assertIn("broken link", result.stdout + result.stderr)

    def test_export_excludes_source_and_survives_relocation_without_git(self):
        (self.root / "bus").mkdir()
        (self.root / "bus/ignored.c").write_text("source", encoding="utf-8")
        self.run_tool("update")
        output = Path(self.temp.name) / "packages"
        self.run_tool("export", "--output", str(output))
        archive = next(output.glob("*.zip"))
        sidecar = archive.with_suffix(".zip.sha256")
        self.assertEqual(sidecar.read_text().split()[0],
                         hashlib.sha256(archive.read_bytes()).hexdigest())
        relocated = Path(self.temp.name) / "other project"
        with zipfile.ZipFile(archive) as bundle:
            self.assertNotIn("bus/ignored.c", bundle.namelist())
            self.assertIn("SHA256SUMS", bundle.namelist())
            bundle.extractall(relocated)
        result = subprocess.run(
            [sys.executable, str(relocated / "scripts/docs-library.py"), "check"],
            capture_output=True, text=True, encoding="utf-8")
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.run_tool("verify-archive", str(archive))

    def test_verify_archive_rejects_modified_file(self):
        self.run_tool("update")
        output = Path(self.temp.name) / "packages"
        self.run_tool("export", "--output", str(output))
        original = next(output.glob("*.zip"))
        corrupted = Path(self.temp.name) / "corrupted.zip"
        with zipfile.ZipFile(original) as source, zipfile.ZipFile(corrupted, "w") as target:
            for name in source.namelist():
                target.writestr(name, b"changed" if name == "README.md" else source.read(name))
        result = self.run_tool("verify-archive", str(corrupted), expected=1)
        self.assertIn("hash mismatch", result.stdout + result.stderr)

    def test_check_reports_changed_agent_instructions(self):
        self.run_tool("update")
        agents = self.root / "AGENTS.md"
        agents.write_text(agents.read_text().replace("# Entry", "# Changed entry"),
                          encoding="utf-8")
        result = self.run_tool("check", expected=1)
        self.assertIn("agent instructions", result.stdout + result.stderr)

    def test_update_cannot_bless_a_changed_imported_fragment(self):
        fragment = "Original history.\n"
        (self.root / "docs/specs/state.md").write_text(
            "<!-- SOURCE_FRAGMENT S001-F1 -->\n" + fragment +
            "<!-- END_SOURCE_FRAGMENT S001-F1 -->\n", encoding="utf-8")
        self.sources["documents"][0].update({
            "kind": "excerpt", "fragments": [{"id": "S001-F1",
                "sha256": hashlib.sha256(fragment.encode()).hexdigest()}]})
        self.save_sources()
        self.run_tool("update")
        history = self.root / "docs/specs/state.md"
        history.write_text(history.read_text().replace("Original history", "Invented result"),
                           encoding="utf-8")
        result = self.run_tool("update", expected=1)
        self.assertIn("excerpt hash mismatch", result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
