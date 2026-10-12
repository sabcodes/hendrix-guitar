import hashlib, json, pathlib, subprocess, sys
bundle=pathlib.Path(sys.argv[1])
bank=bundle/"Contents/Resources/Guitar"
regions=json.loads((bank/"mapping.json").read_text())["regions"]
assert len(regions)==120
assert (bank/"LICENSE.txt").is_file()
for region in regions:
    sample=bank/region["sample"]
    assert sample.is_file() and sample.stat().st_size==region["bytes"]
    with sample.open("rb") as stream:
        assert hashlib.file_digest(stream,"sha256").hexdigest()==region["sha256"]
subprocess.run(["/usr/bin/codesign","--verify","--deep","--strict",str(bundle)],check=True)
print("Bundled samples, license and signature verified.")
