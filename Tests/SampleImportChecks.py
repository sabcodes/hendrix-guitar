import hashlib, importlib.util, pathlib, tempfile, unittest
spec=importlib.util.spec_from_file_location("import_samples",pathlib.Path(__file__).parents[1]/"Scripts/import_samples.py")
mod=importlib.util.module_from_spec(spec); spec.loader.exec_module(mod)
class SampleImportChecks(unittest.TestCase):
    def test_checksum_mismatch(self):
        with tempfile.TemporaryDirectory() as d:
            p=pathlib.Path(d)/"bad.7z"; p.write_bytes(b"invalid")
            with self.assertRaisesRegex(ValueError,"checksum"): mod.import_bank(p,pathlib.Path(d)/"out")
    def test_path_escape(self):
        with self.assertRaises(ValueError): mod.parse_sfz("<region> key=60 sample=../../secret.wav")
    def test_inherited_zones(self):
        text="<global> ampeg_release=0.2 <group> lokey=59 hikey=62 hivel=92 pitch_keycenter=60 "
        for i in range(4): text+=f"<region> lorand={i/4} hirand={(i+1)/4} sample=samples/a{i}.flac "
        got=mod.parse_sfz(text)
        self.assertEqual(len(got),4)
        self.assertEqual({k:got[0][k] for k in ("low","high","root","lowVelocity","highVelocity","release")},
                         dict(low=59,high=62,root=60,lowVelocity=1,highVelocity=92,release=.2))
        self.assertEqual(got[3]["sample"],"samples/a3.flac")
    def test_missing_sample(self):
        with tempfile.TemporaryDirectory() as d:
            p=pathlib.Path(d); (p/"test.sfz").write_text("<region> key=60 sample=missing.wav")
            with self.assertRaisesRegex(ValueError,"sample"): mod.build_mapping(p)
    def test_sample_integrity_recorded(self):
        with tempfile.TemporaryDirectory() as d:
            p=pathlib.Path(d); data=b"fixture sample bytes"
            (p/"sample.flac").write_bytes(data); (p/"test.sfz").write_text("<region> key=60 sample=sample.flac")
            row=mod.build_mapping(p)[0]
            self.assertEqual(row.get("bytes"),len(data))
            self.assertEqual(row.get("sha256"),hashlib.sha256(data).hexdigest())
    def test_unknown_opcode(self):
        with self.assertRaisesRegex(ValueError,"opcode"): mod.parse_sfz("<region> key=60 sample=a.wav filter_type=lpf")
if __name__=="__main__": unittest.main()
