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
#include <MBRBlockDevice.h>
#include "LittleFileSystem.h"
#include <FATFileSystem.h>
#include "wiced_resource.h"

#include "OptaLinkerModule.h"
#include "fwWifiCertificates.h"

namespace optalinker {

using namespace mbed;

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


public:
  OptaLinkerFlash(OptaLinkerMonitor &_monitor, OptaLinkerBoard &_board) : monitor(_monitor), board(_board) {}

  uint8_t setup() {

    // Display flash memory setup message
    monitor.setMessage(LabelFlashSetup, MonitorAction);

    // Check flash memory
    if (!format()) {
      monitor.setMessage(LabelFlashFail, MonitorWarning);

      return 0;
    }

    return 1;
  }

  uint8_t format(uint8_t force = 0) {

    uint8_t wifi_exists = 0, ota_exists = 0, kvstore_exists = 0, user_exists = 0;

    // Init partitions
    BlockDevice* root = BlockDevice::get_default_instance();
    MBRBlockDevice wifi_data(root, 1);
    MBRBlockDevice ota_data(root, 2);
    MBRBlockDevice kvstore_data(root, 3);
    MBRBlockDevice user_data(root, 4);

    // Init filesystems
    FATFileSystem wifi_data_fs("wlan");
    FATFileSystem ota_data_fs("fs");
    FileSystem *user_data_fs;

    // init instance
    if (root->init() != BD_ERROR_OK) {
      monitor.setMessage(LabelFlashInitFail, MonitorWarning);

      return 0;
    }

    // Erase all
    if (force) {
      monitor.setMessage("Erasing partitions", MonitorInfo);
      root->erase(0x0, root->size());
    } else {
      monitor.setMessage("Erasing MBR", MonitorInfo);
      // Erase only the first sector containing the MBR
      root->erase(0x0, root->get_erase_size());
    }

    // Define partions sizes
    MBRBlockDevice::partition(root, 1, 0x0B, 0, 1 * 1024 * 1024);
    MBRBlockDevice::partition(root, 2, 0x0B, 1 * 1024 * 1024,  6 * 1024 * 1024);
    MBRBlockDevice::partition(root, 3, 0x0B, 6 * 1024 * 1024,  7 * 1024 * 1024);
    MBRBlockDevice::partition(root, 4, 0x0B, 7 * 1024 * 1024, 14 * 1024 * 1024);
    // use space from 15.5MB to 16 MB for another fw, memory mapped

    // Check if Wifi partition exists
    if (!wifi_data_fs.mount(&wifi_data)) {
      monitor.setMessage("Partition 1 already contains a filesystem", MonitorInfo);
      wifi_data_fs.unmount();
      wifi_exists = 1;
    }

    // Manage Wifi partition
    if (force || !wifi_exists) {

      // Format wifi partition
      if (wifi_data_fs.reformat(&wifi_data)) {
        monitor.setMessage("Error formatting WiFi partition", MonitorWarning);

        return 0;
      }

      uint32_t chunk_size = 1024;
      uint32_t byte_count = 0;

      // Flash Wifi firwmware
      FILE* fp = fopen("/wlan/4343WA1.BIN", "wb");

      monitor.setMessage("Flashing WiFi firmware", MonitorInfo);
      monitor.setProgress(byte_count, wifi_firmware_file_size, 10, true);
      while (byte_count < wifi_firmware_file_size) {
        if(byte_count + chunk_size > wifi_firmware_file_size)
          chunk_size = wifi_firmware_file_size - byte_count;
        int ret = fwrite(&wifi_firmware_image_data[byte_count], chunk_size, 1, fp);
        if (ret != 1) {
          monitor.setMessage("Error writing firmware data", MonitorWarning);
          break;
        }
        byte_count += chunk_size;
        monitor.setProgress(byte_count, wifi_firmware_file_size, 10, false);
      }
      fclose(fp);

      // Flash Wifi certificates
      fp = fopen("/wlan/cacert.pem", "wb");

      monitor.setMessage("Flashing certificates", MonitorInfo);
      chunk_size = 128;
      byte_count = 0;
      monitor.setProgress(byte_count, wifi_firmware_cacert_pem_len, 10, true);
      while (byte_count < wifi_firmware_cacert_pem_len) {
        if(byte_count + chunk_size > wifi_firmware_cacert_pem_len)
          chunk_size = wifi_firmware_cacert_pem_len - byte_count;
        int ret = fwrite(&wifi_firmware_cacert_pem[byte_count], chunk_size, 1 ,fp);
        if (ret != 1) {
          monitor.setMessage("Error writing certificates", MonitorWarning);
          break;
        }
        byte_count += chunk_size;
        monitor.setProgress(byte_count, wifi_firmware_cacert_pem_len, 10, false);
      }
      fclose(fp);

      // Flash memory mapped Wfifi firmware
      chunk_size = 1024;
      byte_count = 0;
      const uint32_t offset = 15 * 1024 * 1024 + 1024 * 512;

      monitor.setMessage("Flashing memory mapped WiFi firmware", MonitorInfo);
      monitor.setProgress(byte_count, wifi_firmware_file_size, 10, true);
      while (byte_count < wifi_firmware_file_size) {
        if (byte_count + chunk_size > wifi_firmware_file_size)
          chunk_size = wifi_firmware_file_size - byte_count;
        int ret = root->program(&wifi_firmware_image_data[byte_count], offset + byte_count, chunk_size);
        if (ret != 0) {
          monitor.setMessage("Error writing memory mapped firmware", MonitorWarning);
          //break;

          return 0;
        }
        byte_count += chunk_size;
        monitor.setProgress(byte_count, wifi_firmware_file_size, 10, false);
      }
    }

    // Check if OTA partition exists
    if (!ota_data_fs.mount(&ota_data)) {
      monitor.setMessage("Partition 2 already contains a filesystem", MonitorInfo);
      ota_data_fs.unmount();
    }

    // Manage OTA partition
    if (!ota_exists || force) {
      // Format OTA partition
      if (ota_data_fs.reformat(&ota_data)) {
        monitor.setMessage("Error formatting OTA partition", MonitorWarning);

        return 0;
      }
    }

    // Set User parttion as little FS
    user_data_fs = new LittleFileSystem("user");

    // Check if User partition exists
    if (!user_data_fs->mount(&user_data)) {
      monitor.setMessage("Partition 4 already contains a filesystem", MonitorInfo);
      user_data_fs->unmount();
    }

    // Manage User partition
    if (!user_exists || force) {

      // Format User partition
      if (user_data_fs->reformat(&user_data)) {
        monitor.setMessage("Error formatting user partition"), MonitorWarning;

        return 0;
      }
    }

    return 1;
  }

}; // class OptaLinkerFlash

} // namespace optalinker

#endif // #ifndef OPTALINKER_FLASH_H