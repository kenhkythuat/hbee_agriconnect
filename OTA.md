# Hbee OTA qua SIMCOM A7680

## Luồng cập nhật

Thiết bị nhận chuỗi lệnh `ota_check` hoặc `ota_update` trong payload MQTT. Khi
modem đang ở trạng thái `Subscribed`, ứng dụng tải `manifest.json` qua HTTPS,
so sánh phiên bản, tải firmware theo từng khối 256 byte vào vùng staging, rồi
kiểm tra CRC32. Nếu hợp lệ, ứng dụng ghi metadata và reset. Bootloader kiểm tra
lại CRC32, chép image sang vùng application, xóa metadata và khởi động app mới.

Với cấu hình mặc định hiện tại, gửi payload ví dụ
`{"command":"ota_update"}` tới topic `demox/snac/<serial>/` (ví dụ
`demox/snac/hb000999/`).

## Phân vùng Flash (STM32F103RE, 512 KB)

| Vùng | Địa chỉ | Kích thước |
|---|---:|---:|
| Bootloader cố định | `0x08000000` | 32 KB |
| OTA application | `0x08008000` | 224 KB |
| Download staging | `0x08040000` | 252 KB |
| OTA metadata | `0x0807F000` | 2 KB |
| Cấu hình thiết bị | `0x0807F800` | 2 KB |

Vùng cấu hình hiện tại trong `cfg_store.c` đã ở `0x0807F800`, không bị
bootloader hoặc vùng staging ghi đè.

## Build và nạp lần đầu

Build bootloader (truyền đường dẫn toolchain nếu GCC ARM chưa có trong PATH):

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\build_bootloader.ps1 -ToolchainBin "C:\path\to\gnu-tools-for-stm32\bin"
```

Nạp `Bootloader/build/wbee_bootloader.hex` và application được build bằng cấu
hình Debug/Release hiện tại. Application bắt buộc link tại `0x08008000`; file
`.cproject` đã dùng `STM32F103RETX_OTA_APP.ld` và define `OTA_APP_BUILD`.
Không mass-erase bootloader ở các lần nạp application sau.

## Tạo gói phát hành

Tăng `VERSION_WBEE`, clean/rebuild rồi chạy:

```powershell
python tools/prepare_ota_manifest.py --firmware Debug/wbee_stm32f103ret6.hex --version 2.2.0 --branch ota_hbee_v1
```

Commit/push `ota/manifest.json` và file `.bin` được tạo. URL mặc định trỏ tới
GitLab `agriconnect/embedded/wbee`; repository hoặc endpoint raw phải truy cập
được từ SIMCOM mà không cần đăng nhập. Có thể đổi URL bằng macro
`OTA_MANIFEST_URL` lúc build và tùy chọn `--base-url` lúc đóng gói.

CRC32 chỉ chống lỗi truyền dữ liệu, không xác thực nguồn firmware. Trước khi
triển khai trên thiết bị không tin cậy, nên bổ sung chữ ký số cho manifest hoặc
firmware.
