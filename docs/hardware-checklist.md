# Physical board acceptance — not executed

Record board revision, sensor, SDK/RTK/example commits, firmware checksum and environment.
Do not attach certificate bundles, keys, Tokens or credential-bearing binaries to reports.

- [ ] User supplies device certificate, matching P-256 key, CAs and environment settings.
- [ ] Flash each example and confirm boot, DHCP, time synchronization and mTLS Token.
- [ ] MQTT: observe presence after SUBACK, send a command, check received byte count.
- [ ] MQTT: disconnect Wi-Fi/broker, restore it and confirm subscription/presence recovery.
- [ ] MQTT: allow Token expiry and confirm refresh; invalid CA/hostname must fail closed.
- [ ] Test video: view continuously through multiple two-second loops; no frozen boundary.
- [ ] Test video: direct, forced TURN/UDP and forced TURN/TCP; record selected candidates.
- [ ] Camera: confirm selected physical sensor, 1080p/15 fps image, keyframe response.
- [ ] Close/reopen viewer ten times; check camera/media cleanup and no steadily growing heap use.
- [ ] Repeat under Wi-Fi interruption and viewer disappearance.
- [ ] Record resource measurements and perform the product's long-duration soak before release.

Passing ELF/image generation or the QEMU harness is not physical board acceptance.
No audio playback, microphone or talkback is part of this project.

## 量產前另需完成

- [ ] Private key 已佈署於 PRO2 protected zone，並完成相應金鑰操作驗證。
- [ ] 已移除一般檔案系統中的明碼 private key／certificate 配置與明碼私鑰陣列。
- [ ] 已接上正式 certificate provisioning 流程，未夾帶測試憑證或開發韌體。

以上 protected zone 整合不包含在目前的測試範例實作中。
