/*
 * OptaLinker
 *
 * Author: Jean-Christian Paul Denis
 * Source: https://github.com/JcDenis/OptaLinker
 *
 * @see OptaLinker.h
 * @see README.md file
 */

#ifndef OPTALINKER_FLASH_H
#define OPTALINKER_FLASH_H

#include <BlockDevice.h>
#include <FATFileSystem.h>
#include <MBRBlockDevice.h>

#include "OptaLinkerModule.h"
#include "fwWifiCertificates.h"

namespace optalinker {

class OptaLinkerMonitor;
class OptaLinkerBoard;

/**
 * Formatting flash memory.
 *
 * This class formats flash memory in four partitions 
 * and install Wifi firmware and cetificate.
 */
class OptaLinkerFlash : public OptaLinkerModule {

private:
  OptaLinkerMonitor &monitor;
  OptaLinkerBoard &board;

  mbed::BlockDevice *_root;

  /**
   * Flash Wifi firmare and cetificate.
   *
   * @return  1 on success, else 0
   */
  uint8_t flashWiFiFirmwareAndCertificates() {
    FILE *fp = fopen("/wlan/4343WA1.BIN", "wb");
    uint32_t chunk_size = 1024;
    uint32_t byte_count = 0;

    monitor.setMessage(LabelFlashFirmware, MonitorInfo);
    monitor.setProgress(byte_count, wifi_firmware_file_size, 10, true);
    while (byte_count < wifi_firmware_file_size) {
      if (byte_count + chunk_size > wifi_firmware_file_size)
        chunk_size = wifi_firmware_file_size - byte_count;
      int ret = fwrite(&wifi_firmware_image_data[byte_count], chunk_size, 1, fp);
      if (ret != 1) {
        monitor.setMessage(LabelFlashFirmwareFail, MonitorWarning);

        return 0;
      }
      byte_count += chunk_size;
      monitor.setProgress(byte_count, wifi_firmware_file_size, 10, false);
      board.pingTimeout();
    }
    fclose(fp);

    fp = fopen("/wlan/cacert.pem", "wb");
    chunk_size = 128;
    byte_count = 0;

    monitor.setMessage(LabelFlashCertificate, MonitorInfo);
    monitor.setProgress(byte_count, wifi_firmware_cacert_pem_len, 10, true);

    while (byte_count < wifi_firmware_cacert_pem_len) {
      if (byte_count + chunk_size > wifi_firmware_cacert_pem_len)
        chunk_size = wifi_firmware_cacert_pem_len - byte_count;
      int ret = fwrite(&wifi_firmware_cacert_pem[byte_count], chunk_size, 1, fp);
      if (ret != 1) {
        monitor.setMessage(LabelFlashCertificateFail, MonitorWarning);

        return 0;
      }
      byte_count += chunk_size;
      monitor.setProgress(byte_count, wifi_firmware_cacert_pem_len, 10, false);
      board.pingTimeout();
    }

    return 1;
  }

  /**
   * Wifi firmware mapped.
   *
   * @return  1 on succes, else 0
   */
  uint8_t flashWiFiFirmwareMapped() {
    uint32_t chunk_size = 1024;
    uint32_t byte_count = 0;
    const uint32_t offset = 15 * 1024 * 1024 + 1024 * 512;

    monitor.setMessage(LabelFlashMapped, MonitorInfo);
    monitor.setProgress(byte_count, wifi_firmware_file_size, 10, true);

    while (byte_count < wifi_firmware_file_size) {
      if (byte_count + chunk_size > wifi_firmware_file_size)
        chunk_size = wifi_firmware_file_size - byte_count;
      int ret = _root->program(wifi_firmware_image_data, offset + byte_count, chunk_size);
      if (ret != 0) {
        monitor.setMessage(LabelFlashMappedFail, MonitorWarning);

        return 0;
      }
      byte_count += chunk_size;
      monitor.setProgress(byte_count, wifi_firmware_file_size, 10, false);
      board.pingTimeout();
    }

    return 1;
  }

public:
  OptaLinkerFlash(OptaLinkerMonitor &_monitor, OptaLinkerBoard &_board) : monitor(_monitor), board(_board) {}

  uint8_t setup() {

    // Display falsh memoery setup message
    monitor.setMessage(LabelFlashSetup, MonitorAction);

    // Get block device instance
    _root = mbed::BlockDevice::get_default_instance();
    if (_root->init() != mbed::BD_ERROR_OK) {
      monitor.setMessage(LabelFlashInitFail, MonitorWarning);

      return 0;
    }

    // Check flash memory
    if (!format()) {
      monitor.setMessage(LabelFlashFail, MonitorWarning);

      return 0;
    }

    return 1;
  }

  /**
   * Process flash memory formatting.
   *
   * @return 1 on success, else 0
   */
  uint8_t format(uint8_t force = 0) {
    mbed::MBRBlockDevice wifi_data(_root, 1);
    mbed::FATFileSystem wifi_data_fs("wlan");
    bool noWifi = wifi_data_fs.mount(&wifi_data) != 0;
    monitor.setMessage((noWifi ? LabelFlashMissing : LabelFlashExisting) + String("Wifi"), MonitorInfo);

    mbed::MBRBlockDevice ota_data(_root, 2);
    mbed::FATFileSystem ota_data_fs("fs");
    bool noOta = ota_data_fs.mount(&ota_data) != 0;
    monitor.setMessage((noOta ? LabelFlashMissing : LabelFlashExisting) + String("OTA"), MonitorInfo);

    mbed::MBRBlockDevice kvstore_data(_root, 3);
    // do not touch this one

    mbed::MBRBlockDevice user_data(_root, 4);
    mbed::FATFileSystem user_data_fs("fs");
    bool noUser = user_data_fs.mount(&user_data) != 0;
    monitor.setMessage((noUser ? LabelFlashMissing : LabelFlashExisting) + String("User"), MonitorInfo);

    bool perform = force || noWifi || noOta || noUser;

    if (perform) {
      monitor.setMessage(LabelFlashErase, MonitorInfo);
      _root->erase(0x0, _root->size());
      monitor.setMessage(LabelFlashEraseSuccess, MonitorInfo);
    }

    mbed::MBRBlockDevice::partition(_root, 1, 0x0B, 0, 1 * 1024 * 1024);                // WIFI
    mbed::MBRBlockDevice::partition(_root, 2, 0x0B, 1 * 1024 * 1024, 6 * 1024 * 1024);  // OTA
    mbed::MBRBlockDevice::partition(_root, 3, 0x0B, 6 * 1024 * 1024, 7 * 1024 * 1024);  // KV
    mbed::MBRBlockDevice::partition(_root, 4, 0x0B, 7 * 1024 * 1024, 14 * 1024 * 1024); // USER
    // use space from 15.5MB to 16 MB for another fw, memory mapped

    if (force || noWifi) {
      monitor.setMessage(LabelFlashFormat + String("Wifi"), MonitorInfo);

      wifi_data_fs.unmount();
      if (wifi_data_fs.reformat(&wifi_data) != 0) {  // not used yet
        monitor.setMessage(LabelFlashFormatFail, MonitorWarning);

        return 0;
      }
      board.pingTimeout();

      if (!flashWiFiFirmwareAndCertificates() || !flashWiFiFirmwareMapped()) {

        return 0;
      }
    }

    if (force || noOta) {
      monitor.setMessage(LabelFlashFormat + String("OTA"), MonitorInfo);

      ota_data_fs.unmount();
      if (ota_data_fs.reformat(&ota_data) != 0) {
        monitor.setMessage(LabelFlashFormatFail, MonitorWarning);

        return 0;
      }
      board.pingTimeout();
    }

    if (force || noUser) {
      monitor.setMessage(LabelFlashFormat + String("User"), MonitorInfo);

      user_data_fs.unmount();
      if (user_data_fs.reformat(&user_data) != 0) {
        monitor.setMessage(LabelFlashFormatFail, MonitorWarning);

        return 0;
      }
      board.pingTimeout();
    }

    return 1;
  }

}; // class OptaLinkerFlash

} // namespace optalinker

#endif // #ifndef OPTALINKER_FLASH_H