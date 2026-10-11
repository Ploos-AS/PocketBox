import pathlib
import struct
import tempfile
import unittest

from openwrt.check_binary import inspect

class BinaryInspectionTests(unittest.TestCase):
    def test_mips_little_endian_elf32(self):
        with tempfile.TemporaryDirectory() as d:
            p = pathlib.Path(d) / "pocketbox"
            header = bytearray(20)
            header[:4] = b"\x7fELF"
            header[4] = 1
            header[5] = 1
            struct.pack_into("<H", header, 18, 8)
            p.write_bytes(header)
            self.assertEqual(inspect(p), {
                "bytes": 20, "elf_bits": 32, "endian": "little", "machine": 8
            })

    def test_big_endian_elf32(self):
        with tempfile.TemporaryDirectory() as d:
            p = pathlib.Path(d) / "pocketbox"
            header = bytearray(20)
            header[:4] = b"\x7fELF"
            header[4] = 1
            header[5] = 2
            struct.pack_into(">H", header, 18, 8)
            p.write_bytes(header)
            self.assertEqual(inspect(p)["endian"], "big")

    def test_reject_non_elf(self):
        with tempfile.TemporaryDirectory() as d:
            p = pathlib.Path(d) / "binary"
            p.write_bytes(b"hello")
            with self.assertRaisesRegex(ValueError, "not an ELF"):
                inspect(p)


if __name__ == "__main__":
    unittest.main()
