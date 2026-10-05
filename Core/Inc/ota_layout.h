#ifndef INC_OTA_LAYOUT_H_
#define INC_OTA_LAYOUT_H_

#include <stdint.h>

#define OTA_FLASH_BASE            0x08000000UL
#define OTA_FLASH_SIZE            (512UL * 1024UL)
#define OTA_FLASH_PAGE_SIZE       0x800UL

#define OTA_BOOTLOADER_SIZE       (32UL * 1024UL)
#define OTA_APP_START_ADDR        (OTA_FLASH_BASE + OTA_BOOTLOADER_SIZE)
#define OTA_APP_MAX_SIZE          (224UL * 1024UL)

#define OTA_STAGING_START_ADDR    0x08040000UL
#define OTA_STAGING_MAX_SIZE      0x0003F000UL
#define OTA_METADATA_ADDR         0x0807F000UL
#define OTA_CONFIG_ADDR           0x0807F800UL

#define OTA_METADATA_MAGIC        0x4F544131UL /* OTA1 */
#define OTA_METADATA_VERSION      0x00020000UL
#define OTA_STATE_PENDING         1UL

typedef struct __attribute__((packed)) {
  uint32_t magic;
  uint32_t metadata_version;
  uint32_t state;
  uint32_t app_addr;
  uint32_t staging_addr;
  uint32_t image_size;
  uint32_t image_crc32;
  char firmware_version[16];
  uint32_t header_crc32;
} ota_metadata_t;

#endif /* INC_OTA_LAYOUT_H_ */
