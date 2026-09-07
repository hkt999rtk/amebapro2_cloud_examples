# AmebaPro2 Cloud Examples

Three independent AmebaPro2 firmware examples. WebRTC supports **H.264 video only**.
Application sources, configuration and outputs live here; vendor SDK sources remain external.

| Example | Behavior |
|---|---|
| `mqtt` | Wi-Fi, P-256 mTLS Token, MQTTS subscription, presence publication and reconnect |
| `webrtc_test_video` | MQTT offers and H.264 synthetic video over WebRTC, without a camera or SD card |
| `webrtc_camera` | MMFv2 camera H.264 over WebRTC, default 1920×1080, 15 fps, 2,097,152 bps |

The MQTT example publishes presence after each successful subscription and reports received
message sizes. It does not execute cloud commands or expose arbitrary publish commands.
It excludes the WebRTC engine from the linked firmware. The WebRTC examples use the same
MQTT transport and the existing RTK service lifecycle.

## Dependencies

- Unmodified AmebaPro2 SDK **9.6e** (official baseline hashes checked before building).
- External `rtk_ameba_webrtc`, including its vendored dependencies; see `dependencies.json`.
- Arm GNU Toolchain **10.3.1**, newlib **4.1.0**.
- CMake 3.20+ (validated with CMake 4), Ninja, Python 3.9+, OpenSSL; macOS or Linux.
- Host tests additionally need a C/C++ compiler, pkg-config, cJSON, libcurl, OpenSSL,
  Go, FFmpeg and FFprobe. QEMU tests need `qemu-system-arm`; TURN/TCP tests need Docker.

The existing `rtk_cloud_client` desktop C++ MQTT example is not linked into the
FreeRTOS firmware. The embedded MQTT/P-256 transport is provided by `rtk_ameba_webrtc`.

```sh
export RTK_AMEBA_SDK_ROOT=/absolute/path/to/sdk-ameba-v9.6e
export RTK_AMEBA_WEBRTC_ROOT=/absolute/path/to/rtk_ameba_webrtc
export RTK_ARM_TOOLCHAIN_BIN=/absolute/path/to/gcc-arm-none-eabi-10.3-2021.10/bin
```

Check out the recorded RTK revision in a separate dependency checkout if your workspace
has newer ongoing work; the build does not switch dependency branches for you.

## Device certificate: supplied by the user

**Development-only plaintext credentials — not a production storage design.**
For simple testing, this example reads the user-supplied certificate/private key from
plaintext PEM files on the development host, then converts them into C arrays embedded
in the firmware. Both the local files and generated firmware therefore contain readable
credential material. File permissions and Git ignore rules do not make this production-safe.

**Production requirement:** do not retain the private key/certificate as plaintext files
in a general-purpose filesystem, including a filesystem on the device's Flash. The private
key **must be provisioned into the AmebaPro2 (PRO2) protected zone**. Replace the development
PEM/embedded-key path with the product's protected-zone credential integration, and manage
the certificate through the production provisioning process. Do not ship the plaintext
private-key arrays, PEM files or test credential bundles. **This repository does not yet
implement or validate protected-zone provisioning or key operations.**


Download the device client certificate through the **Developer UI** and supply its
matching **P-256 private key**. If the download does not include the key, use the key
created with that device certificate. This project does not request or issue certificates.

```sh
mkdir -p local
cp config/device.example.json local/device.local.json
chmod 700 local
chmod 600 local/device.local.json
```

Edit the local JSON with your Wi-Fi settings, device ID, Token/Cloud/MQTT endpoints,
command/presence topics and paths to these PEM files:

- Device certificate chain and matching private key.
- HTTPS server CA and MQTT server CA.

Relative PEM paths resolve from the JSON directory. Retain the cloud fields in the
shared template even for MQTT. The device ID must match the certificate CN and topics.
Certificate format, current validity, P-256 key matching, Digital Signature and ClientAuth
usage are checked before compilation. Runtime TLS checks CA and hostname.

Wi-Fi modes: `wpa2-aes`, `open`, or `provisioned` (previously provisioned on the board).
The worker connects Wi-Fi, gets DHCP and synchronizes time before TLS. After unrecoverable
transport or network failure it releases resources and retries the application after 5 seconds.
The MQTT transport refreshes its Token before each CONNECT and handles bounded reconnect.

Local config and PEM files must stay untracked. Development credentials and Wi-Fi settings
are embedded into the firmware: **do not publish build directories or distribute images
containing real keys**. Builds create private directories with owner-only defaults.
No real device credentials are bundled with this project.

## Build

Run from this repository root:

```sh
python3 tools/build.py mqtt --config local/device.local.json
python3 tools/build.py webrtc_test_video --config local/device.local.json
python3 tools/build.py webrtc_camera --config local/device.local.json --sensor SENSOR_GC2053
```

`--sdk`, `--webrtc` and `--toolchain` override the corresponding environment variables.
`--jobs 4` is the default. `--configure-only` validates inputs and prepares the build.
Sensor names must exist in the vendor `sensor.h`. GC2053 is the default; select the
sensor on your board. Selection is injected into C compilation outside the vendor tree;
unsupported camera dimensions/FPS cause a `sensor_capacity` error at runtime.

Each successful build produces a clearly named full image in `output/`:

- `amebapro2_mqtt_flash_ntz.bin`
- `amebapro2_webrtc_test_video_flash_ntz.bin`
- `amebapro2_webrtc_camera_flash_ntz.bin`

Each image has a `.sha256` sidecar. Detailed artifacts also remain at:

```text
build/<example>/flash_ntz.bin
build/<example>/application/firmware_ntz.bin
build/<example>/application/application.ntz
build/<example>/application/application.ntz.map
```

CMake uses an external hook. It redirects the vendor-generated `build_info.h` into
`build/<example>/generated/`; no source is copied into the vendor SDK. Each build
hashes vendor files before/after and fails if they change. The hook references existing
RTK camera/media helpers directly. No audio tracks or audio API additions are included.

For a clean build, use a fresh ignored build directory (or remove only the chosen
example's build directory after preserving any needed local artifacts).

## Flash and view

Use the AmebaPro2 SDK 9.6e Image Tool appropriate for the board, select the board's serial
port/download mode and load `build/<example>/flash_ntz.bin` as the complete non-TrustZone
image. Follow the board's Image Tool instructions for download/reset. The build never
selects a serial port or flashes a board automatically.

Watch UART for `EXAMPLE_NETWORK_READY` and `EXAMPLE_READY kind=...`. Start the existing
RTK Cloud viewer using a separately provisioned viewer identity, matching cloud environment
and device ID. Device certificates are not viewer credentials. With `force_relay: true`,
WebRTC requires relay candidates; TURN URLs/credentials come from Cloud ICE configuration.
The SDK supports TURN/UDP and TURN/TCP, not TURN/TLS.

Test video is a 2-second, 320×240, 15 fps synthetic fixture (~100 KB). Every frame is IDR
with repeated SPS/PPS, so the next frame satisfies a new session/PLI. Playback loops
without resetting its 90 kHz RTP timestamp (normal 32-bit wrap is supported).
Regenerate with `python3 tools/generate_video.py`; byte-identical regeneration requires
the same FFmpeg/libx264 version, but the encoded format and source pattern are fixed.

Camera callbacks use the external SDK's bounded frame pool/queue and request encoder
keyframes on PLI. Cleanup stops the producer before draining the queue and releasing memory.

## Validation

```sh
python3 tools/test_local.py
cmake -S tests -B build/example-tests -G Ninja \
  -DRTK_AMEBA_WEBRTC_ROOT="$RTK_AMEBA_WEBRTC_ROOT"
cmake --build build/example-tests --parallel 4
ctest --test-dir build/example-tests --output-on-failure
python3 tools/test_qemu.py direct
python3 tools/test_qemu.py udp
python3 tools/test_qemu.py direct --disconnect
python3 tools/test_turn_tcp.py
```

`test_local.py` checks RTP wrap, three loops decoded by FFmpeg (90 frames), and the SDK's
credential rejection cases. Host interop tests also send this repository's real fixture to
Go/Pion over direct and forced TURN/UDP paths.

QEMU executes the shared SDK FreeRTOS/lwIP/mbedTLS protocol harness, not the physical Ameba
firmware or MMFv2 camera. It validates local test MQTT/mTLS, H.264, PLI and cleanup with
isolated ephemeral credentials. It does not validate your cloud account or real board.
`test_turn_tcp.py` runs a temporary Coturn container and validates host and QEMU TURN/TCP;
it removes its own container afterward. `--disconnect` deliberately closes the test broker socket between two sessions.
QEMU modes must run sequentially because the
external test harness uses fixed local ports.

To reproduce compile-only validation without cloud credentials, run
`python3 tests/create_build_config.py` once in a fresh checkout and build with
`--config local/test-only/device.local.json`. These short-lived self-signed inputs
use reserved `.test` endpoints and cannot authenticate to the real cloud.

See [validation results](docs/validation.md) and [board checklist](docs/hardware-checklist.md).

## Developer Website release

The canonical website/offline guide is `docs/developer/protwo-cloud-examples.en.md`.
Website evaluation images use isolated test credentials only and cannot reach your Cloud.
Never publish an image built with user credentials. Release packaging builds fresh isolated inputs.
