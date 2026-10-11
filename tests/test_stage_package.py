import pathlib
import tempfile
import unittest

from openwrt.stage_package import stage

REPO = pathlib.Path(__file__).resolve().parents[1]


class StagePackageTests(unittest.TestCase):
    def test_stage_and_refuse_overwrite(self):
        with tempfile.TemporaryDirectory() as directory:
            tree = pathlib.Path(directory)
            (tree / "include").mkdir()
            (tree / "rules.mk").touch()
            (tree / "include/package.mk").touch()
            stage(REPO, tree)
            package = tree / "package/pocketbox"
            for relative in ("Makefile", "files/pocketbox.init",
                             "files/pocketbox.config", "src/main.c",
                             "src/catalog.h", "src/http_async.h"):
                self.assertTrue((package / relative).is_file(), relative)
            with self.assertRaisesRegex(ValueError, "refusing overwrite"):
                stage(REPO, tree)

    def test_reject_non_openwrt_destination(self):
        with tempfile.TemporaryDirectory() as directory:
            tree = pathlib.Path(directory)
            with self.assertRaisesRegex(ValueError, "not an OpenWrt"):
                stage(REPO, tree)
            self.assertFalse((tree / "package/pocketbox").exists())


if __name__ == "__main__":
    unittest.main()
