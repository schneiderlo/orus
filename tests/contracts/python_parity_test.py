from __future__ import annotations

import json
import os
from pathlib import Path
import unittest

from python.orus_contracts.canonical import ContractError, canonical_json, sha256_hex, statistics


def runfile(relative: str) -> Path:
    return Path(os.environ["TEST_SRCDIR"]) / os.environ["TEST_WORKSPACE"] / relative


class PythonParityTest(unittest.TestCase):
    def test_fixed_canonical_bytes_and_digests(self) -> None:
        expected = {
            "perf-workload.json": "6ae68e519f47866d9aca4574dbda964e3557b9536bb0819a6314d564aed745d0",
            "perf-raw-sample.json": "be68aca7cc59414f245daaf3dca7bfa22b6463d97e33e80771655a6bd58d8b78",
            "perf-runner.json": "29751cbe816b7edad016dddef96f5dbd28a9e5290e5c4b664e5f5effae1f3e62",
            "perf-result.json": "53bf9f36b775d084120636acbc80490257e8f32d3e9e662de3fd0dae3a751f4b",
            "perf-comparison.json": "8aede2e8179c52f9d6c96618510ebe6e4311030b6d91f292a58bca4d7a4c94c6",
        }
        for name, digest in expected.items():
            data = runfile(f"tests/contracts/fixtures/{name}").read_bytes()
            self.assertEqual(canonical_json(data), data)
            self.assertEqual(sha256_hex(data), digest)

    def test_typed_input_emission_and_statistics(self) -> None:
        value = {"z": 1, "a": "é\n\u0001", "array": [-1, 0, True, None]}
        encoded = canonical_json(value)
        self.assertEqual(encoded, "{\"a\":\"é\\n\\u0001\",\"array\":[-1,0,true,null],\"z\":1}".encode())
        self.assertEqual(
            statistics([-4, -3, 2, 3], [500_000]),
            {
                "maximum": 3,
                "median": -1,
                "median_absolute_deviation": 3,
                "minimum": -4,
                "percentiles": [{"rank_ppm": 500_000, "value": -3}],
            },
        )

    def test_noncanonical_duplicate_unicode_number_and_terminal_bytes_reject(self) -> None:
        cases = [
            b"{\"a\":1,\"a\":2}",
            "{\"a\":\"é\"}".encode(),
            b"{\"a\":1.0}",
            b"{\"a\":9223372036854775808}",
            b"{\"a\":1}\n",
            b"{ \"a\":1}",
            b"\xef\xbb\xbf{}",
            b"{\"a\":\"\xff\"}",
        ]
        for case in cases:
            with self.subTest(case=case), self.assertRaises(ContractError):
                canonical_json(case)

    def test_parity_disagreement_never_normalizes_untrusted_bytes(self) -> None:
        noncanonical = b"{\"z\":1,\"a\":2}"
        with self.assertRaisesRegex(ContractError, "CANONICAL_JSON_NONCANONICAL"):
            canonical_json(noncanonical)
        parsed = json.loads(noncanonical)
        self.assertEqual(canonical_json(parsed), b"{\"a\":2,\"z\":1}")


if __name__ == "__main__":
    unittest.main()
