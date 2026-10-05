# WBEE STM32F103RET6 Firmware

Firmware WBEE cho STM32F103RETx, dùng modem SIMCOM để kết nối MQTT, đọc cảm biến, publish dữ liệu lên server Agriconnect và điều khiển bơm/relay qua lệnh MQTT nhận từ UART1.

## 1. Thông tin project

- MCU: `STM32F103RETx`
- IDE khuyến nghị: STM32CubeIDE
- Toolchain: GNU Tools for STM32 `13.3.rel1`
- Debug/build profile: `Debug`
- Linker script: `STM32F103RETX_FLASH.ld`
- File cấu hình chính: `Core/Inc/config.h`
- File bật/tắt lịch bơm tự động: `Core/Inc/main.h`

## 2. Cấu trúc thư mục

```text
Core/
  Inc/              Header và file cấu hình firmware
  Src/              Source code ứng dụng
  Startup/          Startup file STM32
Drivers/           HAL driver và CMSIS
Debug/             Makefile/build output của STM32CubeIDE
STM32F103RETX_FLASH.ld
wbee_stm32f103ret6.ioc
```

Các file quan trọng:

- `Core/Inc/config.h`: cấu hình serial, model SIMCOM, MQTT, sensor, thời gian publish.
- `Core/Inc/main.h`: khai báo global, define mode, bật/tắt lịch bơm tự động.
- `Core/Src/convert_data_uart.c`: xử lý UART1 từ SIMCOM/MQTT và điều khiển motor.
- `Core/Src/common_simcom.c`: kết nối SIMCOM, MQTT, publish data/status.
- `Core/Src/main.c`: khởi tạo peripheral, vòng lặp chính, timer callback.
- `Core/Src/ph_pump_isr.c`: logic lịch bơm pH tự động, hiện đang được ẩn bằng define.

## 3. Chuẩn bị môi trường

Cài đặt:

1. STM32CubeIDE.
2. ST-LINK driver.
3. Board STM32F103RETx và ST-LINK/V2 hoặc debugger tương thích.
4. Modem SIMCOM đúng model cấu hình trong `config.h`.

Sau khi cài STM32CubeIDE, import project bằng:

```text
File -> Import -> Existing Projects into Workspace -> chọn thư mục wbee
```

## 4. Cấu hình firmware

Mở `Core/Inc/config.h` và kiểm tra các define sau trước khi build.

### 4.1. Phiên bản firmware

```c
#define VERSION_WBEE "2.2.0"
```

### 4.2. Model SIMCOM

Chọn đúng modem đang dùng:

```c
#define a7672s 1
#define a7670c 2
#define a7670sa 3
#define a7080 4
#define a7680 5

#define SIMCOM_MODEL a7680
```

Ví dụ dùng A7670C:

```c
#define SIMCOM_MODEL a7670c
```

### 4.3. Serial number thiết bị

Serial phải viết thường:

```c
#define SERIAL_NUMBER "hb000999"
```

Serial này được dùng để tạo MQTT client ID và topic.

### 4.4. Chu kỳ publish dữ liệu

```c
#define INTERVAL_PUPLISH_DATA 10
```

Đơn vị: giây.

Lưu ý: nếu cấu hình từ `60` giây trở lên, firmware có nhánh đưa thiết bị vào sleep sau khi publish.

### 4.5. Chọn loại cảm biến

```c
#define ph_fuvitech false
#define ec_fuvitech false
#define do_fuvitech false

#define ph_rika500_12 true
#define ec_rika500_13 true
#define do_rika500_04 true
```

Đặt `true` cho loại cảm biến đang dùng, `false` cho loại không dùng.

### 4.6. Duty cycle motor mặc định

```c
#define duty_cycles_ph 50
#define duty_cycles_ec 50
#define duty_cycles_x 50
```

Giá trị là phần trăm PWM, thường trong khoảng `0..100`.

### 4.7. MQTT broker và topic

```c
#define FARM "gateway-agriconnect"
#define MQTT_USER "<mqtt_user>"
#define MQTT_PASS "<mqtt_password>"
#define MQTT_HOST "tcp://mqtt.agriconnect.vn"
#define MQTT_PORT 1883
```

Điền user/password theo tài khoản MQTT được cấp cho hệ thống triển khai.

Topic được tạo tự động từ `FARM` và `SERIAL_NUMBER`:

```c
#define MQTT_TOPIC_ACTUATOR_STATUS FARM "/sn/" SERIAL_NUMBER
#define MQTT_TOPIC_MOTOR_STATUS FARM "/sn/" SERIAL_NUMBER "/as/"
#define MQTT_TOPIC_ACTUATOR_CONTROL FARM "/snac/" SERIAL_NUMBER "/"
```

Ví dụ với `SERIAL_NUMBER = "hb000999"`:

- Publish dữ liệu: `gateway-agriconnect/sn/hb000999`
- Publish trạng thái motor: `gateway-agriconnect/sn/hb000999/as/`
- Subscribe lệnh điều khiển: `gateway-agriconnect/snac/hb000999/#`

## 5. Chế độ điều khiển bơm

Firmware hiện được cấu hình để bơm chỉ chạy theo lệnh MQTT nhận từ UART1.

Trong `Core/Inc/main.h`:

```c
#define PH_PUMP_SCHEDULE_ENABLE 0
```

Ý nghĩa:

- `0`: tắt lịch bơm pH tự động. Bơm chỉ chạy theo lệnh MQTT từ UART1.
- `1`: bật lại lịch bơm tự động theo pH, `time_on`, `time_off`, `pump_power`, `control_mode`.

Khi `PH_PUMP_SCHEDULE_ENABLE = 0`, callback TIM6 vẫn dùng để đếm chu kỳ publish data, nhưng không gọi `ph_pump_isr_tick_1s(...)` để tự điều khiển bơm.

## 6. Build firmware

### Build bằng STM32CubeIDE

1. Import project vào STM32CubeIDE.
2. Chọn build configuration `Debug`.
3. Nhấn `Project -> Build Project`.
4. File output sau build:

```text
Debug/wbee_stm32f103ret6.elf
Debug/wbee_stm32f103ret6.map
Debug/wbee_stm32f103ret6.list
```

### Build bằng terminal

Nếu đã có `make` và `arm-none-eabi-gcc` trong PATH:

```bash
make -C Debug
```

Clean build:

```bash
make -C Debug clean
make -C Debug
```

## 7. Nạp code vào STM32

### Cách 1: nạp bằng STM32CubeIDE

1. Kết nối ST-LINK với board STM32.
2. Cấp nguồn cho board.
3. Mở project trong STM32CubeIDE.
4. Chọn `Run -> Debug Configurations`.
5. Chọn cấu hình debug của project.
6. Nhấn `Debug` hoặc `Run`.

STM32CubeIDE sẽ build và nạp file `.elf` xuống MCU.

### Cách 2: nạp bằng STM32CubeProgrammer

1. Build project để tạo file `Debug/wbee_stm32f103ret6.elf`.
2. Mở STM32CubeProgrammer.
3. Chọn kết nối `ST-LINK`.
4. Nhấn `Connect`.
5. Chọn file `.elf`.
6. Nhấn `Download`.
7. Reset board sau khi nạp.

## 8. UART và kết nối ngoại vi

Các UART chính trong firmware:

- `USART1`: giao tiếp SIMCOM, nhận phản hồi modem và payload MQTT.
- `USART2`: đọc dữ liệu sensor Fuvitech/Rika tùy cấu hình.
- `UART4`: sensor pH.
- `UART5`: giao tiếp màn hình/ESP32, gửi dữ liệu hiển thị và nhận cấu hình JSON nếu bật xử lý.

Luồng MQTT chính:

1. STM32 bật SIMCOM.
2. Chờ modem sẵn sàng.
3. Kiểm tra SIM/network.
4. Kết nối MQTT broker.
5. Subscribe topic điều khiển.
6. Nhận lệnh MQTT qua UART1.
7. Điều khiển PWM motor và publish trạng thái.

## 9. Lệnh điều khiển motor qua MQTT

Firmware subscribe topic:

```text
<FARM>/snac/<SERIAL_NUMBER>/#
```

Với cấu hình mặc định:

```text
gateway-agriconnect/snac/hb000999/#
```

Payload/topic được xử lý trong `Core/Src/convert_data_uart.c`. Các motor đang được map:

- Motor `1`: pH plus, PWM TIM2 CH4.
- Motor `2`: pH minus, PWM TIM2 CH3.
- Motor `3`: motor X, PWM TIM2 CH1.

Trạng thái motor được publish dạng JSON:

```json
{"1":0,"2":0,"3":0}
```

## 10. Cấu hình JSON từ UART5

Firmware có parser JSON cho cấu hình pH:

```json
{
  "ph_high": 7.5,
  "ph_low": 6.0,
  "time_off": 10,
  "time_on": 5,
  "pump_power": 50,
  "control_mode": 0
}
```

Trong code hiện tại, `process_uart_rx()` đang được comment trong `main.c`, nên cấu hình JSON UART5 chưa được xử lý ở vòng lặp chính. Nếu cần dùng lại phần này, bật lại lời gọi `process_uart_rx()` trong `while (1)` hoặc vị trí phù hợp.

## 11. Checklist trước khi triển khai

Trước khi nạp thiết bị mới:

1. Chọn đúng `SIMCOM_MODEL`.
2. Đổi đúng `SERIAL_NUMBER`.
3. Kiểm tra `FARM`, `MQTT_USER`, `MQTT_PASS`, `MQTT_HOST`, `MQTT_PORT`.
4. Chọn đúng loại sensor.
5. Kiểm tra `INTERVAL_PUPLISH_DATA`.
6. Kiểm tra `PH_PUMP_SCHEDULE_ENABLE`.
7. Build không lỗi.
8. Nạp firmware.
9. Kiểm tra log UART/SWO nếu cần debug.
10. Kiểm tra thiết bị đã subscribe đúng topic MQTT.

## 12. Ghi chú bảo trì

- Không sửa trực tiếp các file trong `Drivers/` nếu không thật sự cần.
- Khi đổi cấu hình pin/peripheral trong `.ioc`, cần generate lại code bằng STM32CubeMX/STM32CubeIDE và kiểm tra các vùng `USER CODE`.
- Các thông tin nhạy cảm như MQTT user/password nên được quản lý cẩn thận khi đưa project lên repository công khai.
- Nếu bật lại lịch bơm tự động, cần test kỹ tương tác giữa lịch tự động và lệnh MQTT để tránh hai luồng cùng điều khiển PWM.
