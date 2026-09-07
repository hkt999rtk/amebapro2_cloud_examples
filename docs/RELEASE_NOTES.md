# Initial Developer Website evaluation release

- Adds standalone MQTT, looping H.264 and MMFv2 camera examples, full flash images and source/offline guides.
- Fixes provisioned Wi-Fi retries: DHCP starts once per observed association; application cleanup preserves externally managed Wi-Fi.
- Adds isolated release builds against SDK 9.6e and GCC 10.3.1/newlib 4.1.0, leaving vendor sources unchanged.
- Website binaries use test-only settings. User Cloud access requires a local rebuild with user-provided credentials.
- No audio, runtime credential provisioning or protected-zone integration is implemented. Production private keys must use PRO2 protected zone.
- Physical board, camera, actual Wi-Fi recovery and user Cloud acceptance remain pending. A 31-second Token QEMU stress case failed; default Token request is 300 seconds. See `validation.md` and `hardware-checklist.md`.
