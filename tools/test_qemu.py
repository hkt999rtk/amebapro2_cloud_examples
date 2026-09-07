#!/usr/bin/env python3
"""Run the external SDK's QEMU protocol suite with outputs inside this repo.
This validates shared SDK transport, not execution of Ameba camera hardware.
"""
import argparse
import os
from pathlib import Path
import shlex
import subprocess
r = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('mode', choices=['direct','udp','tcp'])
p.add_argument('--disconnect',action='store_true',help='close test broker socket between two sessions')
a = p.parse_args()
sdk = Path(os.getenv('RTK_AMEBA_WEBRTC_ROOT',r.parent/'rtk_ameba_webrtc')).resolve()
out = r/'build/qemu'
out.mkdir(parents=True,exist_ok=True)
script = (sdk/'tools/test_qemu_freertos.sh').read_text()
original = 'repository_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)'
assert original in script
script = script.replace(original, 'repository_root='+shlex.quote(str(sdk)))
script = script.replace('$repository_root/build/', str(out)+'/')
script = script.replace('cmake --build "$build_root" --parallel', 'cmake --build "$build_root" --parallel 4')
if a.disconnect:
    handler = (sdk/'tools/qemu_cloud_handler.py').read_text()
    marker = '                    if index > 0:\n                        viewer = start_viewer(index)'
    assert handler.count(marker) == 1, 'external broker fixture changed'
    handler = handler.replace(marker,
        '                    if index > 0:\n'
        '                        mqtt_connection.shutdown(socket.SHUT_RDWR)  # deliberate broker failure\n'
        '                        mqtt_connection.close()\n'
        '                        print("TEST_BROKER_DISCONNECT", flush=True)\n'
        '                        mqtt_connection, topic = mqtt_accept_subscribed(mqtt_listener, mqtt_tls, state, None)\n'
        '                        print("RTK_QEMU_MQTT CREDENTIAL_RECONNECT=PASS", flush=True)\n'
        '                        viewer = start_viewer(index)')
    fault_handler = out/'cloud_disconnect.py'
    fault_handler.write_text(handler)
    script = script.replace('$repository_root/tools/qemu_cloud_handler.py',str(fault_handler))
runner = out/'run-sdk-qemu.sh'
runner.write_text(script)
env = dict(os.environ, RTK_QEMU_FORCE_RELAY='0' if a.mode=='direct' else '1',
           RTK_QEMU_TURN_TRANSPORT='tcp' if a.mode=='tcp' else 'udp')
if a.disconnect: env['RTK_QEMU_ITERATIONS'] = '2'
subprocess.run(['sh',str(runner)],env=env,check=True,cwd=out)
