from __future__ import annotations

import os
from pathlib import Path
import unittest

from tests.fuzz.fuzz_smoke_verifier import FuzzVerificationError, verify_fuzz_smoke


def runfile(relative: str) -> Path:
    return Path(os.environ["TEST_SRCDIR"]) / os.environ["TEST_WORKSPACE"] / relative


class PublicBoundaryTest(unittest.TestCase):
    def test_public_header_contains_no_third_party_or_native_layout(self) -> None:
        header = runfile("contracts/include/orus/contracts/contracts.h").read_text(encoding="utf-8")
        for forbidden in ("glaze/", "utf8proc", "openssl/", "EVP_", "glz::", "utf8proc_"):
            self.assertNotIn(forbidden, header)
        self.assertIn("namespace orus::contracts", header)
        self.assertIn("class Sha256Stream", header)

    def test_fuzz_registration_has_checked_in_seed_and_malformed_corpus(self) -> None:
        corpus = runfile("tests/fuzz/corpus/canonical_json")
        entries = sorted(path for path in corpus.iterdir() if path.is_file())
        self.assertGreaterEqual(len(entries), 4)
        self.assertTrue(any(path.read_bytes() == b"{}" for path in entries))
        self.assertTrue(any(b"duplicate" in path.name.encode() for path in entries))

        registration = runfile("tests/fuzz/BUILD.bazel").read_text(encoding="utf-8")
        configuration = runfile(".bazelrc").read_text(encoding="utf-8")
        harness = runfile("tests/fuzz/canonical_json_parser_fuzz.cc").read_text(encoding="utf-8")
        self.assertIn('name = "canonical_json_parser_fuzz_smoke"', registration)
        self.assertIn('"-runs=256"', registration)
        self.assertIn('data = [":canonical_corpus"]', registration)
        self.assertIn("-fsanitize=fuzzer-no-link,address,undefined", configuration)
        self.assertIn("-fsanitize=fuzzer,address,undefined", configuration)
        self.assertIn("__builtin_trap()", harness)

    def test_fuzz_verifier_rejects_every_required_failure_simulation(self) -> None:
        corpus = ["valid-empty-object", "invalid-duplicate"]
        valid = {
            "target": "canonical_json_parser_fuzz_smoke",
            "executions": 256,
            "exit_code": 0,
            "timed_out": False,
            "oom": False,
            "asan_finding": False,
            "ubsan_finding": False,
            "loaded_corpus": corpus,
        }
        verify_fuzz_smoke(
            valid,
            expected_target="canonical_json_parser_fuzz_smoke",
            corpus_entries=corpus,
        )
        simulations = {
            "empty_corpus": ({}, []),
            "zero_executions": ({"executions": 0}, corpus),
            "crash": ({"exit_code": 1}, corpus),
            "timeout": ({"timed_out": True}, corpus),
            "oom": ({"oom": True}, corpus),
            "asan": ({"asan_finding": True}, corpus),
            "ubsan": ({"ubsan_finding": True}, corpus),
            "target_omission": ({"target": "other"}, corpus),
            "corpus_omission": ({"loaded_corpus": corpus[:1]}, corpus),
        }
        for name, (changes, entries) in simulations.items():
            with self.subTest(name=name), self.assertRaises(FuzzVerificationError):
                verify_fuzz_smoke(
                    valid | changes,
                    expected_target="canonical_json_parser_fuzz_smoke",
                    corpus_entries=entries,
                )


if __name__ == "__main__":
    unittest.main()
