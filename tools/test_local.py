#!/usr/bin/env python3
"""Verify playback, actual H.264 decoding, and external credential validation."""
from pathlib import Path
import os
import subprocess
import shlex
root = Path(__file__).resolve().parents[1]
sdk = Path(os.getenv('RTK_AMEBA_WEBRTC_ROOT', root.parent/'rtk_ameba_webrtc')).resolve()
out = root/'build/tests'
out.mkdir(parents=True, exist_ok=True)
def run(args): subprocess.run([str(a) for a in args],check=True)
run(['cc','-std=c11','-Wall','-Wextra','-Werror','-I'+str(root/'common'),
     '-I'+str(root/'assets'),root/'tests/test_playback.c',root/'assets/test_video.c','-o',out/'playback'])
flags = shlex.split(subprocess.check_output(['pkg-config','--cflags','--libs','libcjson'],text=True))
run(['cc','-std=c11','-Wall','-Wextra','-Werror','-I'+str(root/'common'),
     '-I'+str(sdk/'include'),'-I'+str(sdk/'platform/amebapro2/include'),
     root/'tests/test_mqtt_identity.c',root/'common/mqtt_identity.c',
     sdk/'platform/amebapro2/src/rtk_token_jwt.c',*flags,'-o',out/'mqtt_identity'])
run([out/'mqtt_identity'])
run([out/'playback',out/'loops.h264'])
run(['ffmpeg','-v','error','-xerror','-i',out/'loops.h264','-f','null','-'])
probe = subprocess.check_output(['ffprobe','-v','error','-select_streams','v:0',
    '-count_frames','-show_entries','stream=nb_read_frames','-of','csv=p=0',str(out/'loops.h264')],text=True).strip()
assert probe == '90', probe
run(['python3',sdk/'tests/tools/test_generate_amebapro2_firmware_config.py'])
run(['python3',root/'tests/test_certificate_time.py'])
print('LOCAL_TESTS_OK: timestamp wrap, three decoded loops (90 frames), SDK credential tests')
