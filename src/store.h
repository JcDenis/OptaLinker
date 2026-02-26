/*
 * OptaLinker
 *
 * Author: Jean-Christian Paul Denis
 * Source: https://github.com/JcDenis/OptaLinker
 *
 * @see OptaLinker.h
 * @see README.md file
 */

#ifndef OPTALINKER_STORE_H
#define OPTALINKER_STORE_H

#include <mbed.h>
#include <BlockDevice.h>
#include <MBRBlockDevice.h>
#include "LittleFileSystem.h"
#include <FATFileSystem.h>
#include "wiced_resource.h"
#include <KVStore.h>
#include <kvstore_global_api.h>

#include "OptaLinkerModule.h"
#include "fwWifiCertificates.h"

namespace optalinker {

using namespace mbed;

class OptaLinkerMonitor;
class OptaLinkerBoard;

/**
 * OptaLinker Library flash memory module.
 *
 * This class formats flash memory in four partitions 
 * and install Wifi firmware and cetificate.
 *
 * * 1 : Wifi : 1MB
 * * 2 : OTA : 5MB
 * * 3 : KVstore : 1MB
 * * 4 : User : 7MB
 *
 * There are methods to help KV store usage.
 */
class OptaLinkerStore : public OptaLinkerModule {

private:
  OptaLinkerMonitor &monitor;
  OptaLinkerBoard &board;

  /**
   * Boot count.
   */
  uint32_t _bootCount = 0;

public:
  OptaLinkerStore(OptaLinkerMonitor &_monitor, OptaLinkerBoard &_board) : monitor(_monitor), board(_board) {}

  uint8_t setup() {

    // Display flash memory setup message
    monitor.setMessage(LabelStoreSetup, MonitorAction);

    // Check flash memory
    if (!formatMemory()) {

      return 0;
    }

    const char key[10] = "bootcount";
    const char *bc = readKey(key);
    _bootCount = abs(atoi(bc) + 1);
    writeKey(key, String(_bootCount).c_str());

    return 1;
  }

  /**
   * Print to serial monitor store contents.
   *
   * This prints keys and contents length.
   */
  void printKeys() {
    kv_iterator_t it;
    kv_info_t info;
    char key[32] = {0};
    if (kv_iterator_open(&it, nullptr) == MBED_SUCCESS) {
        while(kv_iterator_next(it, key, 32) == MBED_SUCCESS) {
        if (kv_get_info(key, &info) == MBED_SUCCESS) {
          monitor.setMessage(String(key) + " : " + String(info.size), MonitorReceive);
        }
      }
      kv_iterator_close(it);
    }
  }

  /**
   * Read contents of a stored key.
   *
   * @return  The stored key contents (or an empty string)
   */
  const char *readKey(const char *key) {
    kv_info_t info;
    if (kv_get_info(key, &info) == MBED_SUCCESS) {
      char* buffer = (char*)malloc(info.size + 1);
      size_t actual; 
      if (kv_get(key, buffer, info.size, &actual) == MBED_SUCCESS) {
        buffer[actual] = '\0';
        
        return buffer;
      } 
    }

    monitor.setMessage(LabelStoreReadFail, MonitorWarning);

    return "";
  }

  /**
   * Write a key contents to store.
   *
   * @param   key   The key
   * @param   value   The contents
   *
   * @return  1 on success, else 0
   */
  uint8_t writeKey(const char *key, const char *value) {

    return kv_set(key, value, strlen(value), 0) == MBED_SUCCESS ? 1 : 0;
  }

  /**
   * Deleted a stored key.
   *
   * @param   key   The key to delete.
   *
   * @return  1 on success, else 0
   */
  uint8_t eraseKey(const char *key) {

    return kv_remove(key) == MBED_SUCCESS ? 1 : 0;
  }

  /**
   * Get number of times board boots.
   *
   * @return  The boot count
   */
  uint32_t getBootCount() {

    return _bootCount;
  }

  /**
   * Process flash memory partition and formatting.
   *
   * @return  1 on succes (even if nothing is done), else 0
   */
  uint8_t formatMemory(uint8_t force = 0) {

    uint8_t wifi_exists = 0, ota_exists = 0, user_exists = 0;

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
      monitor.setMessage(LabelStoreInitFail, MonitorWarning);

      return 0;
    }

    // Erase all
    if (force) {
      monitor.setMessage(LabelStoreErase, MonitorInfo);
      root->erase(0x0, root->size());
    } else {
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
      monitor.setMessage(LabelStoreExisting + String("Wifi"), MonitorInfo);
      wifi_data_fs.unmount();
      wifi_exists = 1;
    }

    // Manage Wifi partition
    if (force || !wifi_exists) {

      // Format wifi partition
      if (wifi_data_fs.reformat(&wifi_data)) {
        monitor.setMessage(LabelStoreFormatFail + String("Wifi"), MonitorWarning);

        return 0;
      }

      uint32_t chunk_size = 1024;
      uint32_t byte_count = 0;

      // Flash Wifi firwmware
      FILE* fp = fopen("/wlan/4343WA1.BIN", "wb");

      monitor.setMessage(LabelStoreFirmware, MonitorInfo);
      monitor.setProgress(byte_count, wifi_firmware_file_size, 10, true);
      while (byte_count < wifi_firmware_file_size) {
        if(byte_count + chunk_size > wifi_firmware_file_size)
          chunk_size = wifi_firmware_file_size - byte_count;
        int ret = fwrite(&wifi_firmware_image_data[byte_count], chunk_size, 1, fp);
        if (ret != 1) {
          monitor.setMessage(LabelStoreFirmwareFail, MonitorWarning);
          break;
        }
        byte_count += chunk_size;
        monitor.setProgress(byte_count, wifi_firmware_file_size, 10, false);
      }
      fclose(fp);

      // Flash Wifi certificates
      fp = fopen("/wlan/cacert.pem", "wb");

      monitor.setMessage(LabelStoreCertificate, MonitorInfo);
      chunk_size = 128;
      byte_count = 0;
      monitor.setProgress(byte_count, wifi_firmware_cacert_pem_len, 10, true);
      while (byte_count < wifi_firmware_cacert_pem_len) {
        if(byte_count + chunk_size > wifi_firmware_cacert_pem_len)
          chunk_size = wifi_firmware_cacert_pem_len - byte_count;
        int ret = fwrite(&wifi_firmware_cacert_pem[byte_count], chunk_size, 1 ,fp);
        if (ret != 1) {
          monitor.setMessage(LabelStoreCertificateFail, MonitorWarning);
          break;
        }
        byte_count += chunk_size;
        monitor.setProgress(byte_count, wifi_firmware_cacert_pem_len, 10, false);
      }
      fclose(fp);

      // Flash memory mapped Wifi firmware
      chunk_size = 1024;
      byte_count = 0;
      const uint32_t offset = 15 * 1024 * 1024 + 1024 * 512;

      monitor.setMessage(LabelStorehMapped, MonitorInfo);
      monitor.setProgress(byte_count, wifi_firmware_file_size, 10, true);
      while (byte_count < wifi_firmware_file_size) {
        if (byte_count + chunk_size > wifi_firmware_file_size)
          chunk_size = wifi_firmware_file_size - byte_count;
        int ret = root->program(&wifi_firmware_image_data[byte_count], offset + byte_count, chunk_size);
        if (ret != 0) {
          monitor.setMessage(LabelStoreMappedFail, MonitorWarning);
          //break;

          return 0;
        }
        byte_count += chunk_size;
        monitor.setProgress(byte_count, wifi_firmware_file_size, 10, false);
      }
    }

    // Check if OTA partition exists
    if (!ota_data_fs.mount(&ota_data)) {
      monitor.setMessage(LabelStoreExisting + String("OTA"), MonitorInfo);
      ota_data_fs.unmount();
    }

    // Manage OTA partition
    if (!ota_exists || force) {
      // Format OTA partition
      if (ota_data_fs.reformat(&ota_data)) {
        monitor.setMessage(LabelStoreFormatFail + String("OTA"), MonitorWarning);

        return 0;
      }
    }

    // Set User parttion as little FS
    user_data_fs = new LittleFileSystem("user");

    // Check if User partition exists
    if (!user_data_fs->mount(&user_data)) {
      monitor.setMessage(LabelStoreExisting + String("User"), MonitorInfo);
      user_data_fs->unmount();
    }

    // Manage User partition
    if (!user_exists || force) {

      // Format User partition
      if (user_data_fs->reformat(&user_data)) {
        monitor.setMessage(LabelStoreFormatFail + String("User"), MonitorWarning);

        return 0;
      }
    }

    return 1;
  }

}; // class OptaLinkerStore

} // namespace optalinker

#endif // #ifndef OPTALINKER_STORE_H