from __future__ import annotations

import errno
import socket
import unittest
from pathlib import Path


class NetworkDenialTest(unittest.TestCase):
    def test_sandbox_has_no_external_interface_or_route(self) -> None:
        interfaces = {name for _, name in socket.if_nameindex()}
        self.assertLessEqual(interfaces, {"lo"})

        routes = (Path("/proc/net/route").read_text(encoding="ascii").splitlines()[1:])
        default_routes = [line for line in routes if line.split()[1] == "00000000"]
        self.assertEqual(default_routes, [])

        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as client:
            client.settimeout(0.1)
            outcome = client.connect_ex(("192.0.2.1", 9))
        self.assertIn(outcome, {errno.ENETUNREACH, errno.EHOSTUNREACH})


if __name__ == "__main__":
    unittest.main()
