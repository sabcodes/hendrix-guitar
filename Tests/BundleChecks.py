import json, pathlib, subprocess, sys
bundle=pathlib.Path(sys.argv[1])
bank=bundle/"Contents/Resources/Guitar"
regions=json.loads((bank/"mapping.json").read_text())["regions"]
assert len(regions)==120
assert (bank/"LICENSE.txt").is_file()
assert all((bank/r["sample"]).is_file() for r in regions)
subprocess.run(["/usr/bin/codesign","--verify","--deep","--strict",str(bundle)],check=True)
print("Bundled samples, license and signature verified.")

