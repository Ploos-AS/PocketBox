import os
import socket
import subprocess
import tempfile
import time
from pathlib import Path
from urllib.request import urlopen
from urllib.error import HTTPError

def available_port():
    with socket.socket() as s:
        s.bind(("127.0.0.1", 0))
        return s.getsockname()[1]

with tempfile.TemporaryDirectory() as d:
    Path(d, "welcome.txt").write_text("offline-ok\n")
    hp, tp = available_port(), available_port()
    while hp == tp:
        tp = available_port()
    p = subprocess.Popen(["./pocketbox", "--root", d, "--http-port", str(hp), "--telnet-port", str(tp)])
    try:
        for _ in range(50):
            try:
                with socket.create_connection(("127.0.0.1", tp), timeout=0.2) as s:
                    assert b"POCKETBOX" in s.recv(2048)
                    s.sendall(b"Q")
                break
            except (ConnectionRefusedError, TimeoutError):
                time.sleep(0.05)
        else:
            raise AssertionError("server did not start")
        assert b"PocketBox" in urlopen(f"http://127.0.0.1:{hp}/").read()
        assert urlopen(f"http://127.0.0.1:{hp}/files/welcome.txt").read() == b"offline-ok\n"
        for bad in ["../etc/passwd", "%2e%2e", "missing"]:
            try:
                urlopen(f"http://127.0.0.1:{hp}/files/{bad}")
                raise AssertionError("unexpected success")
            except HTTPError as err:
                assert err.code in (400, 404)
        # A connected but idle terminal must not indefinitely prevent HTTP.
        # This is a regression test for the upcoming nonblocking M1 client loop.
        with socket.create_connection(("127.0.0.1", tp), timeout=1) as idle:
            assert b"POCKETBOX" in idle.recv(2048)
            started = time.monotonic()
            assert b"PocketBox" in urlopen(f"http://127.0.0.1:{hp}/", timeout=1).read()
            assert time.monotonic() - started < 1.0
        print("PASS: HTTP, Telnet, file download, invalid paths, idle-client concurrency")
    finally:
        p.terminate()
        p.wait(timeout=5)
