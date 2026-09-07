#!/usr/bin/env python3
"""Build independent examples against unmodified external AmebaPRO2/RTK SDKs."""
import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
def run(args, **kwargs):
    return subprocess.run([str(x) for x in args], check=True, **kwargs)
def snapshot(root):
    return {str(p.relative_to(root)): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in root.rglob('*') if p.is_file() and '.git' not in p.relative_to(root).parts}
def validate_certificate_time(cert, now=None):
    dates = run(['openssl','x509','-in',cert,'-noout','-startdate','-enddate'],capture_output=True,text=True).stdout.splitlines()
    now = now or datetime.now(timezone.utc)
    start, end = [datetime.strptime(x.split('=',1)[1], '%b %d %H:%M:%S %Y %Z').replace(tzinfo=timezone.utc) for x in dates]
    if not start <= now < end: raise ValueError('device certificate is not currently valid')

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('example', choices=['mqtt','webrtc_test_video','webrtc_camera'])
    ap.add_argument('--config', type=Path, required=True)
    ap.add_argument('--sdk', type=Path, default=os.getenv('RTK_AMEBA_SDK_ROOT'))
    ap.add_argument('--webrtc', type=Path, default=os.getenv('RTK_AMEBA_WEBRTC_ROOT', ROOT.parent/'rtk_ameba_webrtc'))
    ap.add_argument('--toolchain', type=Path, default=os.getenv('RTK_ARM_TOOLCHAIN_BIN'))
    ap.add_argument('--sensor', default='SENSOR_GC2053')
    ap.add_argument('--jobs', type=int, default=4)
    ap.add_argument('--configure-only', action='store_true')
    ap.add_argument('--build-root', type=Path, default=ROOT/'build')
    ap.add_argument('--output-root', type=Path, default=ROOT/'output')
    a = ap.parse_args()
    if not a.sdk or not a.toolchain: ap.error('--sdk and --toolchain (or environment variables) are required')
    sdk, webrtc, tc, config = a.sdk.resolve(), a.webrtc.resolve(), a.toolchain.resolve(), a.config.resolve()
    if sdk == ROOT or ROOT.is_relative_to(sdk): ap.error('examples repository must be outside the vendor SDK')
    os.umask(0o077)
    env = dict(os.environ, PATH=str(tc)+os.pathsep+os.environ['PATH'])
    version = run([tc/'arm-none-eabi-gcc','-dumpfullversion','-dumpversion'], capture_output=True,text=True).stdout.strip()
    newlib = run([tc/'arm-none-eabi-gcc','-E','-P','-x','c','-'], input='#include <_newlib_version.h>\n_NEWLIB_VERSION\n',capture_output=True,text=True).stdout.strip().strip('"')
    if version != '10.3.1' or newlib != '4.1.0': raise ValueError('requires GCC 10.3.1 / newlib 4.1.0')
    run([sys.executable,webrtc/'tools/verify_ameba_sdk_baseline.py','--sdk-root',sdk])
    build = a.build_root.resolve()/a.example
    generated = build/'generated'
    generated.mkdir(parents=True,exist_ok=True)
    # DEVELOPMENT ONLY: generate plaintext credential arrays for simple testing.
    # Production private keys must be provisioned in the PRO2 protected zone;
    # neither plaintext filesystem PEMs nor these embedded arrays are a production path.
    run([sys.executable,webrtc/'tools/generate_amebapro2_firmware_config.py','--config',config,'--output-dir',generated])
    # The upstream generator checks expiry; also reject a not-yet-valid certificate.
    cfg = json.loads(config.read_text())
    cert = Path(cfg['device_certificate_chain_pem'])
    if not cert.is_absolute(): cert = config.parent/cert
    validate_certificate_time(cert)
    sensor_header = sdk/'project/realtek_amebapro2_v0_example/inc/sensor.h'
    if not re.fullmatch(r'SENSOR_[A-Z0-9_]+',a.sensor) or not re.search(r'^#define\s+'+re.escape(a.sensor)+r'\s+',sensor_header.read_text(),re.M):
        raise ValueError('unknown sensor; select a named sensor from SDK sensor.h')
    # Force include the original declarations once, then select the sensor externally.
    override = generated/'sensor_selection.h'
    selection = f'#include "{sensor_header}"\n#undef USE_SENSOR\n#define USE_SENSOR {a.sensor}\n'
    if not override.exists() or override.read_text() != selection:
        override.write_text(selection)
    before = snapshot(sdk)
    try:
        vendor = sdk/'project/realtek_amebapro2_v0_example/GCC-RELEASE'
        run(['cmake','-S',vendor,'-B',build,'-G','Ninja',
            '-DCMAKE_POLICY_VERSION_MINIMUM=3.5',
            f'-DCMAKE_TOOLCHAIN_FILE={vendor}/toolchain.cmake',
            f'-DCMAKE_PROJECT_INCLUDE={ROOT}/cmake/vendor_hook.cmake',
            f'-DCLOUD_EXAMPLES_ROOT={ROOT}',f'-DCLOUD_EXAMPLE={a.example}',
            f'-DCLOUD_CONFIG_DIR={generated}',f'-DRTK_AMEBA_WEBRTC_ROOT={webrtc}',
            '-DBUILD_TZ=OFF','-DBUILD_FPGA=OFF','-DBUILD_PXP=OFF'],env=env,cwd=build)
        if not a.configure_only:
            run(['cmake','--build',build,'--target','flash','--parallel',a.jobs],env=env,cwd=build)
            elf = build/'application/application.ntz'
            for rel in ['application/application.ntz','application/application.ntz.map','application/firmware_ntz.bin','flash_ntz.bin']:
                if not (build/rel).is_file() or not (build/rel).stat().st_size: raise ValueError(f'missing artifact: {rel}')
            symbols = run([tc/'arm-none-eabi-nm','-C',elf],capture_output=True,text=True).stdout
            for required in ['app_example','rtk_ameba_mqtt_transport_start','rtk_ameba_p256_token_provider_create']:
                if not re.search(r' [Tt] '+required+r'$',symbols,re.M): raise ValueError(f'missing linked symbol: {required}')
            if a.example == 'mqtt' and any(x in symbols for x in ['rtc::PeerConnection','rtk_ameba_service_create','juice_create']):
                raise ValueError('MQTT firmware unexpectedly links a WebRTC engine')
            if a.example != 'mqtt' and 'rtc::PeerConnection' not in symbols: raise ValueError('missing WebRTC engine')
            if any(x in symbols for x in ['usrsctp_init', 'kvs_webrtc']):
                raise ValueError('unexpected KVS/SCTP implementation')
            link_map = (build/'application/application.ntz.map').read_text()
            if '/ARM_CM33_NTZ/non_secure/port.c' not in link_map or '/ARM_CM4F/' in link_map:
                raise ValueError('incorrect FreeRTOS ARM port')
            run([tc/'arm-none-eabi-size',elf])
            output = a.output_root.resolve()
            output.mkdir(mode=0o700,exist_ok=True)
            image = output/f'amebapro2_{a.example}_flash_ntz.bin'
            shutil.copy2(build/'flash_ntz.bin',image)
            image.with_suffix('.sha256').write_text(hashlib.sha256(image.read_bytes()).hexdigest()+'  '+image.name+'\n')
            print(f'BUILD_OK example={a.example} sensor={a.sensor} gcc={version} newlib={newlib}')
    finally:
        after = snapshot(sdk)
        changed = sorted(k for k in before.keys()|after.keys() if before.get(k)!=after.get(k))
        if changed: raise ValueError('vendor SDK changed: '+', '.join(changed))
        print('VENDOR_SDK_UNCHANGED')
if __name__ == '__main__':
    try: main()
    except (ValueError,OSError,subprocess.CalledProcessError) as e:
        print(f'BUILD_ERROR: {e}',file=sys.stderr)
        sys.exit(1)
