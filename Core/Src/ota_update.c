#include "ota_update.h"

#include "cJSON.h"
#include "config.h"
#include "main.h"
#include "ota_layout.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if OTA_ENABLE

#define OTA_UART_CAPTURE_SIZE 700U
#define OTA_HTTP_READ_CHUNK   256U
#define OTA_MANIFEST_MAX_SIZE 600U

extern UART_HandleTypeDef huart1;
extern char array_at_command[400];
extern void send_to_simcom_a76xx(char *cmd);

static uint8_t s_uart_capture[OTA_UART_CAPTURE_SIZE];
static volatile uint16_t s_uart_capture_len;
static volatile bool s_uart_capture_active;
static bool s_ota_busy;
static char s_manifest_json[OTA_MANIFEST_MAX_SIZE];

static uint32_t crc32_update(uint32_t crc, const uint8_t *data, uint32_t len) {
  crc = ~crc;
  for (uint32_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (uint8_t bit = 0; bit < 8U; bit++) {
      uint32_t mask = (uint32_t)(-(int32_t)(crc & 1U));
      crc = (crc >> 1U) ^ (0xEDB88320UL & mask);
    }
  }
  return ~crc;
}

static void watchdog_feed(void) { IWDG->KR = 0xAAAAU; }

static void capture_begin(void) {
  uint32_t primask = __get_PRIMASK();
  __disable_irq();
  s_uart_capture_active = false;
  s_uart_capture_len = 0U;
  memset(s_uart_capture, 0, sizeof(s_uart_capture));
  s_uart_capture_active = true;
  if (primask == 0U) {
    __enable_irq();
  }
}

static void capture_end(void) {
  s_uart_capture_active = false;
  __DMB();
}

bool ota_uart_rx_capture(const uint8_t *data, uint16_t length) {
  uint16_t write_at;
  uint16_t available;

  if (!s_uart_capture_active || data == NULL || length == 0U) {
    return false;
  }

  write_at = s_uart_capture_len;
  if (write_at >= (OTA_UART_CAPTURE_SIZE - 1U)) {
    return true;
  }
  available = (uint16_t)(OTA_UART_CAPTURE_SIZE - 1U - write_at);
  if (length > available) {
    length = available;
  }
  memcpy(&s_uart_capture[write_at], data, length);
  write_at = (uint16_t)(write_at + length);
  s_uart_capture[write_at] = '\0';
  __DMB();
  s_uart_capture_len = write_at;
  return true;
}

static int32_t capture_find(const char *needle) {
  uint16_t used = s_uart_capture_len;
  size_t needle_len = strlen(needle);

  if (needle_len == 0U || used < needle_len) {
    return -1;
  }
  for (uint16_t i = 0; i <= (uint16_t)(used - needle_len); i++) {
    if (memcmp(&s_uart_capture[i], needle, needle_len) == 0) {
      return (int32_t)i;
    }
  }
  return -1;
}

static bool wait_for_text(const char *text, uint32_t timeout_ms) {
  uint32_t started = HAL_GetTick();

  while ((HAL_GetTick() - started) < timeout_ms) {
    if (capture_find(text) >= 0) {
      return true;
    }
    if (capture_find("\r\nERROR\r\n") >= 0) {
      return false;
    }
    watchdog_feed();
    HAL_Delay(10);
  }
  return false;
}

static bool send_command_wait(const char *command, const char *expected,
                              uint32_t timeout_ms) {
  bool ok;
  uint16_t response_len;

  capture_begin();
  send_to_simcom_a76xx((char *)command);
  ok = wait_for_text(expected, timeout_ms);
  if (!ok) {
    response_len = s_uart_capture_len;
    printf("[OTA] command failed: %s", command);
    if (response_len > 0U) {
      printf("[OTA] SIMCOM response (%u bytes):\r\n%.*s\r\n", response_len,
             response_len, (const char *)s_uart_capture);
    } else {
      printf("[OTA] SIMCOM response: <timeout>\r\n");
    }
  }
  capture_end();
  return ok;
}

static void http_terminate_if_active(void) {
  uint32_t started;

  capture_begin();
  send_to_simcom_a76xx("AT+HTTPTERM\r\n");
  started = HAL_GetTick();
  while ((HAL_GetTick() - started) < 1500U) {
    if (capture_find("\r\nOK\r\n") >= 0 ||
        capture_find("\r\nERROR\r\n") >= 0) {
      break;
    }
    watchdog_feed();
    HAL_Delay(10);
  }
  capture_end();
}

static bool flash_erase_pages(uint32_t start_addr, uint32_t size) {
  FLASH_EraseInitTypeDef erase = {0};
  uint32_t page_error = 0U;

  if (size == 0U || (start_addr % OTA_FLASH_PAGE_SIZE) != 0U) {
    return false;
  }

  erase.TypeErase = FLASH_TYPEERASE_PAGES;
  erase.PageAddress = start_addr;
  erase.NbPages = (size + OTA_FLASH_PAGE_SIZE - 1U) / OTA_FLASH_PAGE_SIZE;
  HAL_FLASH_Unlock();
  if (HAL_FLASHEx_Erase(&erase, &page_error) != HAL_OK) {
    HAL_FLASH_Lock();
    printf("[OTA] erase failed at 0x%08lX\r\n", (unsigned long)page_error);
    return false;
  }
  HAL_FLASH_Lock();
  return true;
}

static bool flash_program_bytes(uint32_t address, const uint8_t *data,
                                uint32_t length) {
  if ((address & 1U) != 0U || data == NULL) {
    return false;
  }

  HAL_FLASH_Unlock();
  for (uint32_t i = 0U; i < length; i += 2U) {
    uint16_t value = data[i];
    if ((i + 1U) < length) {
      value |= (uint16_t)data[i + 1U] << 8U;
    } else {
      value |= 0xFF00U;
    }
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, address + i, value) !=
        HAL_OK) {
      HAL_FLASH_Lock();
      printf("[OTA] write failed at 0x%08lX\r\n",
             (unsigned long)(address + i));
      return false;
    }
  }
  HAL_FLASH_Lock();
  return true;
}

static bool metadata_valid(const ota_metadata_t *metadata) {
  uint32_t header_crc;

  if (metadata->magic != OTA_METADATA_MAGIC ||
      metadata->metadata_version != OTA_METADATA_VERSION ||
      metadata->state != OTA_STATE_PENDING ||
      metadata->app_addr != OTA_APP_START_ADDR ||
      metadata->staging_addr != OTA_STAGING_START_ADDR ||
      metadata->image_size == 0U ||
      metadata->image_size > OTA_APP_MAX_SIZE) {
    return false;
  }
  header_crc = crc32_update(0U, (const uint8_t *)metadata,
                            sizeof(*metadata) - sizeof(metadata->header_crc32));
  return header_crc == metadata->header_crc32;
}

bool ota_metadata_is_pending(void) {
  return metadata_valid((const ota_metadata_t *)OTA_METADATA_ADDR);
}

static bool save_pending_metadata(const ota_manifest_t *manifest) {
  ota_metadata_t metadata;

  memset(&metadata, 0, sizeof(metadata));
  metadata.magic = OTA_METADATA_MAGIC;
  metadata.metadata_version = OTA_METADATA_VERSION;
  metadata.state = OTA_STATE_PENDING;
  metadata.app_addr = OTA_APP_START_ADDR;
  metadata.staging_addr = OTA_STAGING_START_ADDR;
  metadata.image_size = manifest->size;
  metadata.image_crc32 = manifest->crc32;
  snprintf(metadata.firmware_version, sizeof(metadata.firmware_version), "%s",
           manifest->version);
  metadata.header_crc32 =
      crc32_update(0U, (const uint8_t *)&metadata,
                   sizeof(metadata) - sizeof(metadata.header_crc32));

  if (!flash_erase_pages(OTA_METADATA_ADDR, OTA_FLASH_PAGE_SIZE)) {
    return false;
  }
  return flash_program_bytes(OTA_METADATA_ADDR, (const uint8_t *)&metadata,
                             sizeof(metadata));
}

static int version_component(const char **cursor) {
  int value = 0;
  while (**cursor >= '0' && **cursor <= '9') {
    value = value * 10 + (**cursor - '0');
    (*cursor)++;
  }
  if (**cursor == '.') {
    (*cursor)++;
  }
  return value;
}

static int version_compare(const char *left, const char *right) {
  for (uint8_t part = 0U; part < 4U; part++) {
    int left_value = version_component(&left);
    int right_value = version_component(&right);
    if (left_value != right_value) {
      return left_value > right_value ? 1 : -1;
    }
  }
  return 0;
}

static uint32_t json_u32(cJSON *json, const char *key) {
  cJSON *item = cJSON_GetObjectItem(json, key);
  if (item == NULL) {
    return 0U;
  }
  if (item->valuestring != NULL) {
    return (uint32_t)strtoul(item->valuestring, NULL, 0);
  }
  return (uint32_t)item->valuedouble;
}

static bool http_open(const char *url, uint32_t *content_length) {
  int method = -1;
  int status = 0;
  int length = 0;
  int32_t action_at;

  /* ERROR is normal here when no previous HTTP session exists. */
  http_terminate_if_active();
  if (!send_command_wait("AT+CSSLCFG=\"sslversion\",0,4\r\n", "OK",
                         2000U) ||
      !send_command_wait("AT+CSSLCFG=\"authmode\",0,0\r\n", "OK",
                         2000U) ||
      !send_command_wait("AT+CSSLCFG=\"enableSNI\",0,1\r\n", "OK",
                         2000U) ||
      !send_command_wait("AT+HTTPINIT\r\n", "OK", 3000U) ||
      !send_command_wait("AT+HTTPPARA=\"SSLCFG\",0\r\n", "OK", 2000U) ||
      !send_command_wait("AT+HTTPPARA=\"CONNECTTO\",120\r\n", "OK",
                         2000U) ||
      !send_command_wait("AT+HTTPPARA=\"RECVTO\",120\r\n", "OK", 2000U)) {
    return false;
  }

  if (snprintf(array_at_command, sizeof(array_at_command),
               "AT+HTTPPARA=\"URL\",\"%s\"\r\n", url) >=
      (int)sizeof(array_at_command)) {
    printf("[OTA] URL too long\r\n");
    return false;
  }
  if (!send_command_wait(array_at_command, "OK", 3000U)) {
    return false;
  }

  capture_begin();
  send_to_simcom_a76xx("AT+HTTPACTION=0\r\n");
  if (!wait_for_text("+HTTPACTION:", 120000U)) {
    capture_end();
    printf("[OTA] HTTPACTION timeout\r\n");
    return false;
  }
  action_at = capture_find("+HTTPACTION:");
  if (action_at < 0 ||
      sscanf((char *)&s_uart_capture[action_at], "+HTTPACTION: %d,%d,%d",
             &method, &status, &length) != 3) {
    capture_end();
    printf("[OTA] HTTPACTION parse failed\r\n");
    return false;
  }
  capture_end();

  printf("[OTA] HTTP status=%d length=%d\r\n", status, length);
  if (method != 0 || status < 200 || status >= 300 || length <= 0) {
    return false;
  }
  *content_length = (uint32_t)length;
  return true;
}

static void http_close(void) {
  http_terminate_if_active();
}

static bool parse_httpread_header(uint16_t *body_at, uint16_t *body_length) {
  int32_t header_at = capture_find("+HTTPREAD:");
  uint16_t used = s_uart_capture_len;
  uint16_t cursor;
  uint32_t value = 0U;
  bool has_digit = false;

  if (header_at < 0) {
    return false;
  }
  cursor = (uint16_t)header_at + (uint16_t)strlen("+HTTPREAD:");
  while (cursor < used &&
         (s_uart_capture[cursor] == ' ' || s_uart_capture[cursor] == 'D' ||
          s_uart_capture[cursor] == 'A' || s_uart_capture[cursor] == 'T' ||
          s_uart_capture[cursor] == ',')) {
    cursor++;
  }
  while (cursor < used && s_uart_capture[cursor] >= '0' &&
         s_uart_capture[cursor] <= '9') {
    has_digit = true;
    value = value * 10U + (uint32_t)(s_uart_capture[cursor] - '0');
    cursor++;
  }
  if (!has_digit || value > OTA_HTTP_READ_CHUNK) {
    return false;
  }
  while ((cursor + 1U) < used) {
    if (s_uart_capture[cursor] == '\r' &&
        s_uart_capture[cursor + 1U] == '\n') {
      *body_at = cursor + 2U;
      *body_length = (uint16_t)value;
      return true;
    }
    cursor++;
  }
  return false;
}

static bool http_read_binary(uint32_t offset, uint16_t requested,
                             uint8_t *output, uint16_t *received) {
  uint32_t started;
  uint16_t body_at = 0U;
  uint16_t body_length = 0U;

  snprintf(array_at_command, sizeof(array_at_command),
           "AT+HTTPREAD=%lu,%u\r\n", (unsigned long)offset, requested);
  capture_begin();
  send_to_simcom_a76xx(array_at_command);
  started = HAL_GetTick();

  while ((HAL_GetTick() - started) < 15000U) {
    if (parse_httpread_header(&body_at, &body_length) &&
        s_uart_capture_len >= (uint16_t)(body_at + body_length)) {
      memcpy(output, &s_uart_capture[body_at], body_length);
      *received = body_length;
      capture_end();
      return body_length > 0U;
    }
    if (capture_find("\r\nERROR\r\n") >= 0) {
      break;
    }
    watchdog_feed();
    HAL_Delay(10);
  }

  capture_end();
  printf("[OTA] HTTPREAD failed offset=%lu\r\n", (unsigned long)offset);
  return false;
}

static bool http_get_text(const char *url, char *output, uint32_t capacity) {
  uint32_t content_length;
  uint16_t received;

  if (!http_open(url, &content_length)) {
    return false;
  }
  if (content_length == 0U || content_length >= capacity ||
      content_length > OTA_HTTP_READ_CHUNK) {
    printf("[OTA] manifest length invalid=%lu\r\n",
           (unsigned long)content_length);
    http_close();
    return false;
  }
  if (!http_read_binary(0U, (uint16_t)content_length, (uint8_t *)output,
                        &received) ||
      received != content_length) {
    http_close();
    return false;
  }
  output[received] = '\0';
  http_close();
  return true;
}

static bool load_manifest(ota_manifest_t *manifest) {
  const char *urls[] = {
      OTA_MANIFEST_URL,
#ifdef OTA_MANIFEST_URL_FALLBACK_1
      OTA_MANIFEST_URL_FALLBACK_1,
#endif
#ifdef OTA_MANIFEST_URL_FALLBACK_2
      OTA_MANIFEST_URL_FALLBACK_2,
#endif
  };
  cJSON *json;
  cJSON *version;
  cJSON *bin_url;
  cJSON *device;
  bool downloaded = false;

  memset(manifest, 0, sizeof(*manifest));
  for (uint8_t i = 0U; i < (uint8_t)(sizeof(urls) / sizeof(urls[0])); i++) {
    printf("[OTA] manifest URL %u\r\n", i);
    if (http_get_text(urls[i], s_manifest_json, sizeof(s_manifest_json))) {
      downloaded = true;
      break;
    }
  }
  if (!downloaded) {
    printf("[OTA] manifest download failed\r\n");
    return false;
  }

  json = cJSON_Parse(s_manifest_json);
  if (json == NULL) {
    printf("[OTA] manifest JSON invalid\r\n");
    return false;
  }
  version = cJSON_GetObjectItem(json, "version");
  bin_url = cJSON_GetObjectItem(json, "bin_url");
  device = cJSON_GetObjectItem(json, "device");
  if (version == NULL || version->valuestring == NULL || bin_url == NULL ||
      bin_url->valuestring == NULL || device == NULL ||
      device->valuestring == NULL ||
      strcmp(device->valuestring, OTA_DEVICE_ID) != 0) {
    printf("[OTA] manifest fields/device invalid\r\n");
    cJSON_Delete(json);
    return false;
  }

  snprintf(manifest->version, sizeof(manifest->version), "%s",
           version->valuestring);
  snprintf(manifest->bin_url, sizeof(manifest->bin_url), "%s",
           bin_url->valuestring);
  manifest->app_addr = json_u32(json, "app_addr");
  manifest->size = json_u32(json, "size");
  manifest->crc32 = json_u32(json, "crc32");
  cJSON_Delete(json);

  if (manifest->app_addr != OTA_APP_START_ADDR || manifest->size < 8U ||
      manifest->size > OTA_APP_MAX_SIZE ||
      manifest->size > OTA_STAGING_MAX_SIZE || manifest->crc32 == 0U) {
    printf("[OTA] manifest address/size/CRC invalid\r\n");
    return false;
  }
  return true;
}

static bool download_firmware(const ota_manifest_t *manifest) {
  uint8_t chunk[OTA_HTTP_READ_CHUNK];
  uint32_t content_length;
  uint32_t offset = 0U;
  uint32_t crc = 0U;

  if (!http_open(manifest->bin_url, &content_length)) {
    return false;
  }
  if (content_length != manifest->size) {
    printf("[OTA] binary length mismatch HTTP=%lu manifest=%lu\r\n",
           (unsigned long)content_length, (unsigned long)manifest->size);
    http_close();
    return false;
  }
  if (!flash_erase_pages(OTA_STAGING_START_ADDR, manifest->size)) {
    http_close();
    return false;
  }

  while (offset < manifest->size) {
    uint32_t remaining = manifest->size - offset;
    uint16_t requested = remaining > sizeof(chunk) ? sizeof(chunk)
                                                   : (uint16_t)remaining;
    uint16_t received = 0U;
    if (!http_read_binary(offset, requested, chunk, &received) ||
        received != requested) {
      http_close();
      return false;
    }
    if (!flash_program_bytes(OTA_STAGING_START_ADDR + offset, chunk, received)) {
      http_close();
      return false;
    }
    crc = crc32_update(crc, chunk, received);
    offset += received;
    watchdog_feed();
    if ((offset % 4096U) == 0U || offset == manifest->size) {
      printf("[OTA] downloaded %lu/%lu\r\n", (unsigned long)offset,
             (unsigned long)manifest->size);
    }
  }
  http_close();

  if (crc != manifest->crc32) {
    printf("[OTA] stream CRC mismatch got=0x%08lX expected=0x%08lX\r\n",
           (unsigned long)crc, (unsigned long)manifest->crc32);
    return false;
  }
  crc = crc32_update(0U, (const uint8_t *)OTA_STAGING_START_ADDR,
                     manifest->size);
  if (crc != manifest->crc32) {
    printf("[OTA] flash CRC mismatch got=0x%08lX expected=0x%08lX\r\n",
           (unsigned long)crc, (unsigned long)manifest->crc32);
    return false;
  }
  return true;
}

void ota_init(void) {
  s_uart_capture_active = false;
  s_uart_capture_len = 0U;
  s_ota_busy = false;
  if (ota_metadata_is_pending()) {
    printf("[OTA] pending image found; bootloader is required\r\n");
  }
}

ota_result_t ota_check_and_download(void) {
  ota_manifest_t manifest;
  ota_result_t result = OTA_RESULT_ERROR;

  if (s_ota_busy) {
    return OTA_RESULT_BUSY;
  }
  s_ota_busy = true;
  printf("[OTA] checking update manifest\r\n");

  if (!load_manifest(&manifest)) {
    goto done;
  }
  printf("[OTA] remote=%s current=%s\r\n", manifest.version, VERSION_WBEE);
  if (version_compare(manifest.version, VERSION_WBEE) <= 0) {
    result = OTA_RESULT_NO_UPDATE;
    goto done;
  }
  if (!download_firmware(&manifest)) {
    printf("[OTA] firmware download/verify failed\r\n");
    goto done;
  }
  if (!save_pending_metadata(&manifest)) {
    printf("[OTA] cannot save pending metadata\r\n");
    goto done;
  }

  printf("[OTA] firmware ready for bootloader\r\n");
  result = OTA_RESULT_OK;

done:
  capture_end();
  s_ota_busy = false;
  return result;
}

#else

void ota_init(void) {}
ota_result_t ota_check_and_download(void) { return OTA_RESULT_NO_UPDATE; }
bool ota_metadata_is_pending(void) { return false; }
bool ota_uart_rx_capture(const uint8_t *data, uint16_t length) {
  (void)data;
  (void)length;
  return false;
}

#endif
