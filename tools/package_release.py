#!/usr/bin/env python3
"""Build an immutable, isolated evaluation release; never package caller-supplied firmware."""
import argparse
from datetime import datetime,timezone
import hashlib,json,os,re,subprocess,tarfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
EXAMPLES={'mqtt':('MQTT','Cloud Token, MQTTS commands, presence and reconnect.'),'webrtc_test_video':('H.264 test video','Looping synthetic H.264 over WebRTC, direct or TURN.'),'webrtc_camera':('Live camera','MMFv2 camera encoding and H.264 WebRTC streaming.')}
def run(args,**kw):return subprocess.run([str(x) for x in args],check=True,**kw)
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--version',required=True);p.add_argument('--sdk',required=True,type=Path);p.add_argument('--toolchain',required=True,type=Path);p.add_argument('--webrtc',type=Path,default=ROOT.parent/'rtk_ameba_webrtc');a=p.parse_args()
 if not re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9._-]{0,40}',a.version):p.error('invalid version')
 if run(['git','status','--porcelain'],cwd=ROOT,capture_output=True,text=True).stdout.strip():p.error('commit the source before release packaging')
 commit=run(['git','rev-parse','HEAD'],cwd=ROOT,capture_output=True,text=True).stdout.strip()
 deps=json.loads((ROOT/'dependencies.json').read_text())
 if run(['git','rev-parse','HEAD'],cwd=a.webrtc,capture_output=True,text=True).stdout.strip()!=deps['rtk_ameba_webrtc']['commit']:p.error('RTK dependency revision differs')
 if run(['git','status','--porcelain'],cwd=a.webrtc,capture_output=True,text=True).stdout.strip():p.error('RTK dependency has local modifications')
 os.umask(0o077)
 work=ROOT/'build/releases'/a.version;work.mkdir(parents=True,exist_ok=False)
 private=work/'private';private.mkdir();out=work/'publish';out.mkdir()
 run(['openssl','ecparam','-name','prime256v1','-genkey','-noout','-out',private/'device.key'])
 run(['openssl','req','-new','-x509','-key',private/'device.key','-subj','/CN=test-device','-days','2','-addext','keyUsage=critical,digitalSignature','-addext','extendedKeyUsage=clientAuth','-out',private/'device.pem'])
 cfg=json.loads((ROOT/'config/device.example.json').read_text());cfg.update(device_id='test-device',mqtt_command_topic='devices/test-device/down/commands',mqtt_presence_topic='devices/test-device/up/messages')
 for k in ['device_certificate_chain_pem','https_server_ca_pem','mqtt_server_ca_pem']:cfg[k]=str(private/'device.pem')
 cfg['device_private_key_pem']=str(private/'device.key');config=private/'device.local.json';config.write_text(json.dumps(cfg))
 artifacts=[]
 def artifact(id,p,kind):artifacts.append(dict(id=id,filename=p.name,sha256=sha(p),size_bytes=p.stat().st_size,kind=kind))
 source=out/f'pro2-source-{a.version}.tar.gz'
 run(['git','archive','--format=tar.gz','--prefix=pro2-cloud-examples/','-o',source,commit],cwd=ROOT)
 with tarfile.open(source) as t:
  for n in t.getnames():
   rel=Path(n).parts[1:]
   if any(x in {'local','build','output','.secrets'} for x in rel) or n.endswith(('.pem','.key','.p12','.pfx','.local.json')):raise ValueError('sensitive path in archive')
 artifact('source',source,'source');examples=[]
 for id,(title,desc) in EXAMPLES.items():
  with (work/f'{id}.log').open('w') as log:
   run(['python3',ROOT/'tools/build.py',id,'--config',config,'--sdk',a.sdk,'--webrtc',a.webrtc,'--toolchain',a.toolchain,'--build-root',work/'firmware','--output-root',out],stdout=log,stderr=subprocess.STDOUT)
  image=out/f'amebapro2_{id}_flash_ntz.bin';side=image.with_suffix('.sha256')
  artifact(id,image,'firmware');artifact(id+'-sha256',side,'checksum')
  examples.append(dict(id=id,title=title,description=desc,board='AmebaPro2 SDK 9.6e compatible board; physical validation pending',sensor='GC2053' if id=='webrtc_camera' else 'Not required',firmware_id=id,checksum_id=id+'-sha256',flash_offset=0,image_type='full-flash-ntz',validation=dict(build='PASS',host='PASS (protocol baseline; see report)',hardware='NOT_RUN',cloud='NOT_RUN')))
 manifest=dict(schema='rtk-pro2-examples/v1',version=a.version,source_commit=commit,created_at=datetime.now(timezone.utc).isoformat().replace('+00:00','Z'),terms_version='evaluation-2026-09',terms=(ROOT/'docs/EVALUATION_TERMS.md').read_text(),test_only=True,dependencies=dict(amebapro2_sdk='9.6e',rtk_ameba_webrtc=deps['rtk_ameba_webrtc']['commit'],gcc='10.3.1',newlib='4.1.0'),examples=examples,artifacts=artifacts)
 (out/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
 (work/'latest.json').write_text(json.dumps(dict(version=a.version))+'\n')
 print(f'RELEASE_READY {out} source={commit}; publish only this directory, never private/ or firmware/')
if __name__=='__main__':main()
