#ifndef INC_OTA_UPDATE_H_
#define INC_OTA_UPDATE_H_

#include <stdbool.h>
#include <stdint.h>

typedef enum {
  OTA_RESULT_OK = 0,
  OTA_RESULT_NO_UPDATE,
  OTA_RESULT_BUSY,
  OTA_RESULT_ERROR
} ota_result_t;

typedef struct {
  char version[16];
  char bin_url[256];
  uint32_t app_addr;
  uint32_t size;
  uint32_t crc32;
} ota_manifest_t;

void ota_init(void);
ota_result_t ota_check_and_download(void);
bool ota_metadata_is_pending(void);

/* Called by the USART1 receive-to-idle callback. */
/* Returns true while OTA owns the modem response stream. */
bool ota_uart_rx_capture(const uint8_t *data, uint16_t length);

#endif /* INC_OTA_UPDATE_H_ */
