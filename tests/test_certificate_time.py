import importlib.util
from pathlib import Path
from datetime import datetime, timezone
import subprocess
import tempfile
import unittest
spec = importlib.util.spec_from_file_location('build_examples', Path(__file__).resolve().parents[1]/'tools/build.py')
build = importlib.util.module_from_spec(spec)
spec.loader.exec_module(build)
class CertificateTimeTest(unittest.TestCase):
    def test_validity_window(self):
        with tempfile.TemporaryDirectory() as tmp:
            key, cert = Path(tmp)/'key.pem', Path(tmp)/'cert.pem'
            subprocess.run(['openssl','req','-x509','-newkey','ec','-pkeyopt','ec_paramgen_curve:prime256v1',
                '-nodes','-days','1','-subj','/CN=test-only','-keyout',str(key),'-out',str(cert)],
                check=True,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
            build.validate_certificate_time(cert)
            for now in [datetime(2000,1,1,tzinfo=timezone.utc), datetime(2999,1,1,tzinfo=timezone.utc)]:
                with self.subTest(now=now), self.assertRaisesRegex(ValueError,'not currently valid'):
                    build.validate_certificate_time(cert,now)
if __name__ == '__main__': unittest.main()
