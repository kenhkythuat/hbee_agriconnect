#include "ota_layout.h"
#include "stm32f103xe.h"
#include <stdbool.h>
#include <stdint.h>

#define SRAM_START_ADDR 0x20000000UL
#define SRAM_END_ADDR   0x20010000UL

__attribute__((naked, noreturn)) static void branch_to_application(
    uint32_t stack_pointer, uint32_t reset_vector) {
  (void)stack_pointer;
  (void)reset_vector;
  __asm volatile("msr msp, r0\n"
                 "cpsie i\n"
                 "bx r1\n");
}

static void watchdog_feed(void) { IWDG->KR = 0xAAAAU; }

static uint32_t crc32_update(uint32_t crc, const uint8_t *data, uint32_t len) {
  crc = ~crc;
  for (uint32_t i = 0U; i < len; i++) {
    crc ^= data[i];
    for (uint8_t bit = 0U; bit < 8U; bit++) {
      uint32_t mask = (uint32_t)(-(int32_t)(crc & 1U));
      crc = (crc >> 1U) ^ (0xEDB88320UL & mask);
    }
    if ((i & 0x3FFU) == 0U) {
      watchdog_feed();
    }
  }
  return ~crc;
}

static bool metadata_valid(const ota_metadata_t *metadata) {
  if (metadata->magic != OTA_METADATA_MAGIC ||
      metadata->metadata_version != OTA_METADATA_VERSION ||
      metadata->state != OTA_STATE_PENDING ||
      metadata->app_addr != OTA_APP_START_ADDR ||
      metadata->staging_addr != OTA_STAGING_START_ADDR ||
      metadata->image_size < 8U || metadata->image_size > OTA_APP_MAX_SIZE ||
      metadata->image_size > OTA_STAGING_MAX_SIZE) {
    return false;
  }
  return crc32_update(0U, (const uint8_t *)metadata,
                      sizeof(*metadata) - sizeof(metadata->header_crc32)) ==
         metadata->header_crc32;
}

static bool application_valid(void) {
  uint32_t stack_pointer = *(const uint32_t *)OTA_APP_START_ADDR;
  uint32_t reset_vector = *(const uint32_t *)(OTA_APP_START_ADDR + 4U);
  uint32_t reset_address = reset_vector & ~1UL;

  return stack_pointer >= SRAM_START_ADDR && stack_pointer <= SRAM_END_ADDR &&
         (stack_pointer & 0x3U) == 0U && (reset_vector & 1U) != 0U &&
         reset_address >= OTA_APP_START_ADDR &&
         reset_address < (OTA_APP_START_ADDR + OTA_APP_MAX_SIZE);
}

static bool flash_wait(void) {
  uint32_t timeout = 0x02000000UL;
  while ((FLASH->SR & FLASH_SR_BSY) != 0U && timeout > 0U) {
    timeout--;
    watchdog_feed();
  }
  return timeout > 0U;
}

static bool flash_ready(void) {
  if (!flash_wait()) {
    return false;
  }
  if ((FLASH->SR & (FLASH_SR_PGERR | FLASH_SR_WRPRTERR)) != 0U) {
    FLASH->SR = FLASH_SR_PGERR | FLASH_SR_WRPRTERR;
    return false;
  }
  return true;
}

static void flash_unlock(void) {
  if ((FLASH->CR & FLASH_CR_LOCK) != 0U) {
    FLASH->KEYR = 0x45670123UL;
    FLASH->KEYR = 0xCDEF89ABUL;
  }
}

static void flash_lock(void) { FLASH->CR |= FLASH_CR_LOCK; }

static bool flash_erase_page(uint32_t address) {
  bool ok;

  if (!flash_ready()) {
    return false;
  }
  FLASH->CR |= FLASH_CR_PER;
  FLASH->AR = address;
  FLASH->CR |= FLASH_CR_STRT;
  ok = flash_ready();
  FLASH->CR &= ~FLASH_CR_PER;
  return ok && *(const uint32_t *)address == 0xFFFFFFFFUL;
}

static bool flash_program_halfword(uint32_t address, uint16_t value) {
  bool ok;

  if (!flash_ready()) {
    return false;
  }
  FLASH->CR |= FLASH_CR_PG;
  *(volatile uint16_t *)address = value;
  ok = flash_ready();
  FLASH->CR &= ~FLASH_CR_PG;
  return ok && *(const uint16_t *)address == value;
}

static bool install_pending_image(const ota_metadata_t *metadata) {
  uint32_t page_count;

  if (crc32_update(0U, (const uint8_t *)OTA_STAGING_START_ADDR,
                   metadata->image_size) != metadata->image_crc32) {
    return false;
  }

  flash_unlock();
  page_count =
      (metadata->image_size + OTA_FLASH_PAGE_SIZE - 1U) / OTA_FLASH_PAGE_SIZE;
  for (uint32_t page = 0U; page < page_count; page++) {
    if (!flash_erase_page(OTA_APP_START_ADDR + page * OTA_FLASH_PAGE_SIZE)) {
      flash_lock();
      return false;
    }
    watchdog_feed();
  }

  for (uint32_t offset = 0U; offset < metadata->image_size; offset += 2U) {
    uint16_t value = *(const uint8_t *)(OTA_STAGING_START_ADDR + offset);
    if ((offset + 1U) < metadata->image_size) {
      value |= (uint16_t)(*(const uint8_t *)(OTA_STAGING_START_ADDR + offset + 1U))
               << 8U;
    } else {
      value |= 0xFF00U;
    }
    if (!flash_program_halfword(OTA_APP_START_ADDR + offset, value)) {
      flash_lock();
      return false;
    }
    if ((offset & 0x3FFU) == 0U) {
      watchdog_feed();
    }
  }
  flash_lock();

  if (crc32_update(0U, (const uint8_t *)OTA_APP_START_ADDR,
                   metadata->image_size) != metadata->image_crc32 ||
      !application_valid()) {
    return false;
  }

  flash_unlock();
  if (!flash_erase_page(OTA_METADATA_ADDR)) {
    flash_lock();
    return false;
  }
  flash_lock();
  return true;
}

static void jump_to_application(void) {
  uint32_t stack_pointer = *(const uint32_t *)OTA_APP_START_ADDR;
  uint32_t reset_vector = *(const uint32_t *)(OTA_APP_START_ADDR + 4U);

  __disable_irq();
  SysTick->CTRL = 0U;
  SysTick->LOAD = 0U;
  SysTick->VAL = 0U;
  for (uint8_t i = 0U; i < 8U; i++) {
    NVIC->ICER[i] = 0xFFFFFFFFUL;
    NVIC->ICPR[i] = 0xFFFFFFFFUL;
  }
  SCB->VTOR = OTA_APP_START_ADDR;
  __DSB();
  __ISB();
  branch_to_application(stack_pointer, reset_vector);
}

int main(void) {
  const ota_metadata_t *metadata =
      (const ota_metadata_t *)OTA_METADATA_ADDR;

  if (metadata_valid(metadata)) {
    (void)install_pending_image(metadata);
  }
  if (application_valid()) {
    jump_to_application();
  }

  while (1) {
    watchdog_feed();
  }
}
