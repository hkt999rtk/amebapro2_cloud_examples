# Validation report — 2026-09-07

Validation used macOS, AmebaPro2 SDK 9.6e, GCC 10.3.1/newlib 4.1.0 and the
RTK revision recorded in `dependencies.json`. No real device certificate was used.
All firmware images were built with isolated self-signed test inputs and reserved
`.test` endpoints. These images cannot authenticate to the production/staging cloud.

## Firmware build and source isolation

| Example | ELF + map + application firmware + flash image | Complete flash bytes |
|---|---|---:|
| MQTT | PASS | 4,956,160 |
| H.264 test video | PASS | 4,956,160 |
| MMFv2 camera | PASS | 4,956,160 |

All builds checked linked application/MQTT/credential symbols and the Cortex-M33
NTZ FreeRTOS port. MQTT contains no RTK WebRTC service or libdatachannel peer engine.
Both video images retain libdatachannel. KVS/SCTP symbol checks passed.
The fixed flash image size reflects vendor image layout; it is not RAM usage.
Default sensor GC2053 was compile-validated. Alternate sensors require board testing.
Sensor selection is applied to application and boot/FCS C targets together.

Final builds passed complete vendor-file before/after SHA-256 comparison.
During initial integration the vendor CMake regenerated its `inc/build_info.h` in
place. The external hook now redirects that output. The metadata header was restored
from a matching local SDK 9.6e distribution copy: 6,969 shared files were compared,
with only that header differing; two pre-existing GPIO binaries were preserved.
Neither existing RTK SDK repository has source modifications from this work.

Local raw logs and current image hashes are retained under ignored `build/validation/`.
Credentials and firmware binaries are not committed or uploaded.

## Local and host tests

- PASS: fixture index and 90 kHz timestamp continuity over five loops, including 32-bit wrap.
- PASS: actual H.264 fixture decoded by FFmpeg over three loops, 90 frames total.
- PASS: MQTT-only identity callback refreshes the provider on two successive CONNECTs,
  maps JWT identity correctly, and clears credential buffers on refresh failure.
- PASS: eight external SDK configuration tests, including invalid PEM, missing fields,
  mismatched keys, mismatched certificate identity/topics, and tracked-secret rejection.
- PASS: certificate validity test at current time, before activation and after expiry.
- PASS: 21 external SDK host tests (session orchestration, credential mapping, frame pool,
  direct/forced TURN/UDP interop, five connect/close cycles and protocol checks).
- PASS: two additional tests send this repository's real synthetic fixture to Go/Pion
  through direct and forced TURN/UDP paths.
- PASS: the real fixture also reaches Go/Pion through Coturn forced TURN/TCP.

The host receiver checks RTP delivery; the separate FFmpeg test checks fixture decoding.
These results are not a claim of browser playback on a physical board.

## QEMU protocol validation

- PASS: FreeRTOS/lwIP/mbedTLS direct signaling and H.264 RTP reception.
- PASS: forced TURN/UDP and forced TURN/TCP with relay candidate checks.
- PASS: local device mTLS Token, MQTT subscription/QoS1 PUBACK, PLI callback,
  Cloud Close and resource cleanup. Coturn recorded four relay allocations across
  the host and QEMU tests, and its temporary container was removed.
- PASS: two same-boot sessions with 60-second test Tokens. No credential-refresh marker
  appeared in this run, so it is not counted as automatic Token-expiry recovery evidence.
- PASS: deliberate broker disconnect between two sessions using default 300-second
  Tokens. QEMU refreshed broker credentials, resubscribed, acknowledged a fresh offer
  and completed the second H.264 session (`CREDENTIAL_RECONNECT=PASS`).

The QEMU harness executes the shared SDK protocol stack on MPS2, not the newly built
Ameba firmware binaries. Wi-Fi hardware, sensor DMA/MMF and board flash/boot are not emulated.

### Known short-Token stress failure

A two-session run with **31-second Tokens failed** while resubscribing after a
credential refresh (`MQTT connection closed repeatedly before subscribe`). Logs show
`MQTT_CREDENTIAL_REFRESH` and repeated reconnect attempts. The test does not establish
whether this is entirely a harness timing issue or a transport limitation. Do not claim
31-second Token support based on this work. Examples retain the existing **300-second**
Token request, and the unit test independently checks refresh-before-CONNECT behavior.

Reproduce with `RTK_QEMU_ITERATIONS=2 RTK_QEMU_TOKEN_TTL_SECONDS=31 python3 tools/test_qemu.py direct`.

## Not yet verified

- Real Cloud authentication, signaling and broker access using user-supplied credentials.
- Physical flash/boot, camera picture, sensor compatibility and Wi-Fi recovery.
- Physical direct/relay sessions, memory measurements, reconnect cycles and long soak.

Use `docs/hardware-checklist.md` for board acceptance. No audio is implemented or tested.
