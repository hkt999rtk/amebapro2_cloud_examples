#!/usr/bin/env python3
"""Isolated Coturn TURN/TCP test for the actual video fixture and SDK QEMU stack."""
import os
from pathlib import Path
import socket
import subprocess
import time
r = Path(__file__).resolve().parents[1]
sdk = Path(os.getenv('RTK_AMEBA_WEBRTC_ROOT',r.parent/'rtk_ameba_webrtc')).resolve()
name = f'amebapro2-examples-turn-{os.getpid()}'
image = 'coturn/coturn:4.6.3@sha256:71c3c990283385567f11794ee692e3a47b66fd9b0bb39e42afbe776e331dd888'
user, password = 'example-test', 'isolated-test-password'
url = 'turn:127.0.0.1:34780?transport=tcp'
def run(args, **kw): return subprocess.run([str(x) for x in args],check=True,**kw)
try:
    run(['docker','run','-d','--rm','--name',name,
         '-p','127.0.0.1:34780:3478/tcp','-p','127.0.0.1:34780:3478/udp',
         '-p','127.0.0.1:49160-49170:49160-49170/udp',image,
         '-n','--log-file=stdout','--realm=example.test',f'--user={user}:{password}',
         '--min-port=49160','--max-port=49170','--external-ip=127.0.0.1',
         '--listening-port=3478','--no-tls','--no-dtls','--no-cli'])
    deadline = time.monotonic()+30
    while True:
        try:
            with socket.create_connection(('127.0.0.1',34780),timeout=1): break
        except OSError:
            if time.monotonic() > deadline: raise
            time.sleep(.2)
    run(['python3',sdk/'tests/native/run_ameba_pion_e2e.py',
         '--device',r/'build/example-tests/example_video_endpoint',
         '--go-root',sdk/'packages/golang','--turn-url',url,
         '--turn-username',user,'--turn-password',password])
    print('FIXTURE_TURN_TCP_PASS',flush=True)
    env = dict(os.environ, RTK_QEMU_TURN_USERNAME=user, RTK_QEMU_TURN_PASSWORD=password,
        RTK_QEMU_TURN_VIEWER_URL=url,
        RTK_QEMU_TURN_DEVICE_URL='turn:10.0.2.2:34780?transport=tcp')
    run(['python3',r/'tools/test_qemu.py','tcp'],env=env)
    logs = run(['docker','logs',name],capture_output=True,text=True)
    count = (logs.stdout+logs.stderr).count('Global turn allocation count incremented')
    if count < 4: raise RuntimeError('expected host and QEMU relay allocations')
    print(f'TURN_TCP_PASS allocations={count}')
finally:
    subprocess.run(['docker','rm','-f',name],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
