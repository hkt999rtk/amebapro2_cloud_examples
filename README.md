# AmebaPro2 Cloud Examples

Independent firmware examples for AmebaPro2 SDK 9.6e and the RTK Cloud SDKs.
This repository is currently a project scaffold; firmware examples are not yet implemented.

## Planned examples

- `mqtt`: Wi-Fi, cloud authentication, MQTT publish/subscribe and reconnect.
- `webrtc_test_media`: H.264 test video and Opus test audio, including audio/video synchronization.
- `webrtc_camera`: live camera H.264 and microphone Opus streaming to a viewer.

Initial audio scope is device-to-viewer streaming. Two-way talk is not included.
The RTK WebRTC SDK currently requires audio-track and audio-send support, and its
test viewer requires Opus support before audio examples can run.

## Integration boundaries

- Keep example sources, shared application code, test assets and build outputs here.
- Reference the unmodified vendor SDK through `RTK_AMEBA_SDK_ROOT`.
- Reference RTK SDK repositories through configurable external paths; do not copy their source here.
- Add reusable WebRTC audio support to `rtk_ameba_webrtc`.
- Keep generated firmware and local credentials out of Git.

Current local dependencies:

- Vendor SDK: `/Users/kevinhuang/work/amebapro2_sdk/sdk-ameba-v9.6e`
- MQTT SDK: `../rtk_cloud_client`
- WebRTC SDK: `../rtk_ameba_webrtc`

These paths document the current workspace layout, not implemented build commands.
Build instructions will be added with the examples.
