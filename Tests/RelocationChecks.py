import pathlib, subprocess, sys, tempfile
source, host = sys.argv[1:]
with tempfile.TemporaryDirectory(prefix="hendrix relocation with spaces ") as temporary:
    bundle = pathlib.Path(temporary) / "Moved Guitar.component"
    subprocess.run(["/usr/bin/ditto", "--norsrc", source, str(bundle)], check=True)
    subprocess.run(["/usr/bin/codesign", "--verify", "--deep", "--strict", str(bundle)], check=True)
    subprocess.run([host, str(bundle / "Contents/MacOS/Hendrix Guitar")], check=True)
