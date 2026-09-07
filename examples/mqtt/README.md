# MQTT 雲端連線

驗證 Wi-Fi、裝置 Token、MQTTS 訂閱、presence 發布及重連；不連結 WebRTC 媒體引擎。

## 私鑰與憑證：僅供開發測試

此範例為了簡單測試，暫時使用開發主機上的**明碼 private key／certificate PEM 檔案**，
建置時再轉成 C 陣列編入韌體。因此，PEM 檔案、生成的原始碼與 BIN 都可能包含明碼私鑰；
限制檔案權限或加入 `.gitignore` 並不代表已具備量產保護。

**量產不能沿用這個方式：**

- 私鑰與憑證不應以明碼檔案常駐於一般檔案系統，包括板上 Flash 檔案系統。
- **量產 private key 必須佈署於 PRO2 protected zone**，並改接對應的受保護金鑰操作流程。
- Certificate 由量產佈署流程管理；不要直接沿用本範例的明碼檔案配置。
- 不得將明碼私鑰 C 陣列、測試憑證或含私鑰的開發韌體當作量產交付物。

本範例**尚未實作或驗證 protected zone 的佈署與金鑰操作**。裝置憑證由使用者提供，
可從 Developer UI 下載，並須提供與憑證相符的 P-256 private key。

## 建置與產物

先依[主 README](../../README.md)設定外部 SDK、工具鏈與 `local/device.local.json`。
在 repo 根目錄執行：

```sh
python3 tools/build.py mqtt --config local/device.local.json
```

完整可燒錄映像為 `output/amebapro2_mqtt_flash_ntz.bin`，校驗值為同名 `.sha256` 檔案。
使用 AmebaPro2 SDK 9.6e Image Tool 與正確的板子／serial port 燒錄；不會自動燒錄。
實際雲端連線須使用你的裝置憑證與環境設定，測試憑證無法登入實際雲端。

參閱[驗證結果](../../docs/validation.md)與[實機檢查表](../../docs/hardware-checklist.md)。
