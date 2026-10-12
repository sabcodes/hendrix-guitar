import pathlib, subprocess, sys, tempfile, unittest, zipfile
SOURCE = pathlib.Path(sys.argv.pop(1)) if len(sys.argv)>1 else None
SCRIPT = pathlib.Path(__file__).resolve().parents[1] / "Scripts/install_au.sh"
class InstallChecks(unittest.TestCase):
    def test_install_spaces_backup_and_missing_source(self):
        with tempfile.TemporaryDirectory(prefix="hendrix install ") as temp:
            destination = pathlib.Path(temp) / "Test Components"
            result = subprocess.run(["/bin/bash", str(SCRIPT), str(SOURCE), str(destination)], capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            installed = destination / "Hendrix Guitar.component"
            self.assertTrue((installed / "Contents/Resources/Guitar/mapping.json").is_file())
            subprocess.run(["codesign", "--verify", "--deep", "--strict", str(installed)], check=True)
            marker = installed / "previous-copy.txt"
            marker.write_text("preserve this prior install")
            result = subprocess.run(["/bin/bash", str(SCRIPT), str(SOURCE), str(destination)], capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            archives = list((destination.parent / "HendrixGuitar Backups").glob("*.zip"))
            self.assertEqual(len(archives), 1)
            with zipfile.ZipFile(archives[0]) as archive:
                self.assertEqual(archive.read("Hendrix Guitar.component/previous-copy.txt").decode(), "preserve this prior install")
            self.assertFalse(marker.exists())
            result = subprocess.run(["/bin/bash", str(SCRIPT), str(pathlib.Path(temp)/"missing.component"), str(destination)], capture_output=True, text=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertTrue((installed / "Contents/Resources/Guitar/mapping.json").is_file())
if __name__ == "__main__": unittest.main()
