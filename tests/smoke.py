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
    Path(d, "manual.pdf").write_bytes(b"PDF-test")
    Path(d, ".private").write_text("hidden")
    Path(d, "bad&name.txt").write_text("excluded")
    Path(d, "subfolder").mkdir()
    Path(d, "subfolder", "nested.txt").write_text("nested")
    Path(d, "linked.txt").symlink_to(Path(d, "manual.pdf"))
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
        listing = urlopen(f"http://127.0.0.1:{hp}/").read()
        for expected in (b"welcome.txt", b"manual.pdf"):
            assert expected in listing, (expected, listing)
        for excluded in (b".private", b"bad&name.txt", b"nested.txt", b"linked.txt"):
            assert excluded not in listing, (excluded, listing)
        assert urlopen(f"http://127.0.0.1:{hp}/files/manual.pdf").read() == b"PDF-test"
        with socket.create_connection(("127.0.0.1", tp), timeout=1) as terminal:
            terminal.settimeout(1)
            assert b"POCKETBOX" in terminal.recv(2048)
            terminal.sendall(b"F")
            chunks = []
            deadline = time.monotonic() + 1
            while time.monotonic() < deadline:
                try:
                    part = terminal.recv(2048)
                except socket.timeout:
                    break
                if not part:
                    break
                chunks.append(part)
                if b"welcome.txt" in b"".join(chunks) and b"manual.pdf" in b"".join(chunks):
                    break
            terminal_listing = b"".join(chunks)
            for expected in (b"welcome.txt", b"manual.pdf"):
                assert expected in terminal_listing, (expected, terminal_listing)
            for excluded in (b".private", b"bad&name.txt", b"nested.txt", b"linked.txt"):
                assert excluded not in terminal_listing, (excluded, terminal_listing)
        for i in range(40):
            Path(d, f"retro-{i:03}.bin").write_bytes(b"retro")
        listing = urlopen(f"http://127.0.0.1:{hp}/").read()
        assert b"Next page" in listing
        pages = [listing]
        cursor = "retro-029.bin"
        # Verify cursor paging across the bounded 32-entry view.
        while b"Next page" in pages[-1]:
            import re
            match = re.search(rb"/\?after=([^']+)", pages[-1])
            assert match, pages[-1]
            cursor = match.group(1).decode("ascii")
            pages.append(urlopen(f"http://127.0.0.1:{hp}/?after={cursor}").read())
            assert len(pages) <= 4
        all_pages = b"".join(pages)
        for i in range(40):
            assert f"retro-{i:03}.bin".encode() in all_pages
        for bad in ["../etc/passwd", "%2e%2e", "missing", ".private", "linked.txt", "bad%26name.txt"]:
            try:
                urlopen(f"http://127.0.0.1:{hp}/files/{bad}")
                raise AssertionError("unexpected success")
            except HTTPError as err:
                assert err.code in (400, 404)
        with socket.create_connection(("127.0.0.1", tp), timeout=1) as idle:
            assert b"POCKETBOX" in idle.recv(2048)
            started = time.monotonic()
            assert b"PocketBox" in urlopen(f"http://127.0.0.1:{hp}/", timeout=1).read()
            assert time.monotonic() - started < 1.0
        # A partial HTTP request must not starve another HTTP client.
        with socket.create_connection(("127.0.0.1", hp), timeout=1) as slow:
            slow.sendall(b"GET / HTTP/1.1")
            started = time.monotonic()
            assert b"PocketBox" in urlopen(f"http://127.0.0.1:{hp}/", timeout=1).read()
            assert time.monotonic() - started < 1.0
        print("PASS: HTTP, Telnet, file download, invalid paths, idle terminal and HTTP concurrency")
    finally:
        p.terminate()
        p.wait(timeout=5)
