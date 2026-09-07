#!/usr/bin/env python3
"""Create isolated self-signed test inputs for compilation, never cloud enrollment."""
import json
import os
from pathlib import Path
import subprocess
root = Path(__file__).resolve().parents[1]
os.umask(0o077)
out = root/'local/test-only'
out.mkdir(parents=True,exist_ok=True)
if any(out.iterdir()):
    raise SystemExit('test-only directory is not empty; existing inputs were preserved')
def run(args): subprocess.run([str(x) for x in args],check=True)
run(['openssl','ecparam','-name','prime256v1','-genkey','-noout','-out',out/'device.key'])
run(['openssl','req','-new','-x509','-key',out/'device.key','-subj','/CN=test-device',
    '-days','2','-addext','keyUsage=critical,digitalSignature',
    '-addext','extendedKeyUsage=clientAuth','-out',out/'device.pem'])
c = json.loads((root/'config/device.example.json').read_text())
c.update(device_id='test-device',mqtt_command_topic='devices/test-device/down/commands',
         mqtt_presence_topic='devices/test-device/up/messages')
for k in ['device_certificate_chain_pem','https_server_ca_pem','mqtt_server_ca_pem']:
    c[k]=str(out/'device.pem')
c['device_private_key_pem']=str(out/'device.key')
(out/'device.local.json').write_text(json.dumps(c,indent=2)+'\n')
print('Test-only inputs created in local/test-only. Endpoints are reserved .test domains.')
