"""Import only the pinned CC0 FreePats direct guitar bank; no runtime SFZ parser."""
import argparse, hashlib, json, math, os, pathlib, re, shutil, subprocess, tempfile
SHA256="7ab2a4551bb8847342d6fd10cc56e0642fcaf43f3be8d665269c4dc9bfbbd599"
URL="https://github.com/freepats/electric-guitar-FSBS-direct/releases/download/2022-09-11/EGuitarFSBS-direct-SFZ%2BFLAC-20220911.7z"
OPCODES={"sample","key","lokey","hikey","lovel","hivel","pitch_keycenter","lorand","hirand","ampeg_release"}
def safe_path(value):
    p=pathlib.PurePosixPath(value)
    if p.is_absolute() or ".." in p.parts or "\\" in value or not p.parts:
        raise ValueError("unsafe sample/archive path")
    return p
def parse_sfz(text):
    text=re.sub(r"//[^\n]*","",text)
    global_values={}; group={}; current=None; rows=[]
    def finish():
        if current is None: return
        values={**global_values,**group,**current}
        sample=values.get("sample",""); safe_path(sample)
        key=values.get("key")
        low=int(values.get("lokey",key if key is not None else 0))
        high=int(values.get("hikey",key if key is not None else 127))
        root=int(values.get("pitch_keycenter",key if key is not None else 60))
        lv=int(values.get("lovel",1)); hv=int(values.get("hivel",127))
        release=float(values.get("ampeg_release",.2))
        if not (0<=low<=high<=127 and 0<=root<=127 and 1<=lv<=hv<=127 and math.isfinite(release) and 0<release<=10):
            raise ValueError("invalid sample region")
        lo=float(values.get("lorand",0)); hi=float(values.get("hirand",1))
        if not (0<=lo<hi<=1): raise ValueError("invalid random region")
        rows.append(dict(sample=sample,low=low,high=high,root=root,lowVelocity=lv,highVelocity=hv,release=release))
    active=None
    for token in re.findall(r"<[^>]+>|[^\s<>]+",text):
        if token.startswith("<"):
            finish(); current=None; active=token[1:-1]
            if active=="global": global_values={}; group={}
            elif active=="group": group={}
            elif active=="region": current={}
            else: raise ValueError("unsupported SFZ tag "+active)
        else:
            if "=" not in token or active is None: raise ValueError("invalid SFZ token")
            name,value=token.split("=",1)
            if name not in OPCODES: raise ValueError("unsupported opcode "+name)
            target=global_values if active=="global" else group if active=="group" else current
            target[name]=value
    finish()
    if not rows: raise ValueError("no sample regions")
    return rows
def build_mapping(directory):
    files=list(directory.rglob("*.sfz"))
    if len(files)!=1: raise ValueError("expected one SFZ mapping")
    root=files[0].parent
    rows=parse_sfz(files[0].read_text())
    for row in rows:
        path=root/row["sample"]
        if not path.is_file() or path.is_symlink() or not path.resolve().is_relative_to(root.resolve()):
            raise ValueError("missing/unsafe sample "+row["sample"])
        if path.stat().st_size>=100*1024*1024: raise ValueError("sample exceeds Git file limit")
        row["bytes"]=path.stat().st_size
        with path.open("rb") as stream: row["sha256"]=hashlib.file_digest(stream,"sha256").hexdigest()
    return rows
def import_bank(archive, destination):
    archive=pathlib.Path(archive); destination=pathlib.Path(destination)
    with archive.open("rb") as stream:
        digest=hashlib.file_digest(stream,"sha256").hexdigest()
    if digest!=SHA256:
        raise ValueError("archive checksum mismatch")
    names=subprocess.check_output(["/usr/bin/tar","-tf",str(archive)],text=True).splitlines()
    for name in names: safe_path(name)
    listing=subprocess.check_output(["/usr/bin/tar","-tvf",str(archive)],text=True).splitlines()
    if any(line and line[0] not in "-d" for line in listing): raise ValueError("archive contains links or special files")
    if destination.exists(): raise ValueError("destination already exists")
    destination.parent.mkdir(parents=True,exist_ok=True)
    with tempfile.TemporaryDirectory(dir=destination.parent,prefix=".import-") as tmp:
        extract=pathlib.Path(tmp)/"extract"; extract.mkdir()
        subprocess.run(["/usr/bin/tar","-xf",str(archive),"-C",str(extract)],check=True)
        rows=build_mapping(extract)
        source=next(extract.rglob("*.sfz")).parent
        stage=pathlib.Path(tmp)/"bank"; shutil.copytree(source,stage)
        (stage/"mapping.json").write_text(json.dumps(dict(regions=rows),indent=2)+"\n")
        metadata=dict(url=URL,sha256=SHA256,version="2022-09-11",license="CC0-1.0",regions=len(rows))
        (stage/"source.json").write_text(json.dumps(metadata,indent=2)+"\n")
        os.replace(stage,destination)
    return metadata
if __name__=="__main__":
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("archive",type=pathlib.Path); parser.add_argument("destination",type=pathlib.Path)
    args=parser.parse_args(); print(json.dumps(import_bank(args.archive,args.destination),indent=2))
