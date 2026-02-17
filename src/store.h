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

#include <KVStore.h>
#include <kvstore_global_api.h>
#include <mbed.h>

#include "OptaLinkerModule.h"

namespace optalinker {

class OptaLinkerMonitor;

class OptaLinkerStore : public OptaLinkerModule {

private:
  OptaLinkerMonitor &monitor;

  uint32_t _bootCount = 0;

public:
  OptaLinkerStore(OptaLinkerMonitor &_monitor) : monitor(_monitor) {}

  uint8_t setup() {
    const char key[10] = "bootcount";
    const char *bc = read(key);
    _bootCount = abs(atoi(bc) + 1);
    write(key, String(_bootCount).c_str());

    return 1;
  }

  /**
   * Print to serial monitor store contents.
   *
   * This prints keys and contents length.
   */
  void print() {
    kv_iterator_t it;
    kv_info_t info;
    char key[32] = {0};
    if (kv_iterator_open(&it, nullptr) == MBED_SUCCESS) {
        while(kv_iterator_next(it, key, 32) == MBED_SUCCESS) {
        if (kv_get_info(key, &info) == MBED_SUCCESS) {
          monitor.setInfo(String(key) + " : " + String(info.size));
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
  const char *read(const char *key) {
    kv_info_t info;
    if (kv_get_info(key, &info) == MBED_SUCCESS) {
      char* buffer = (char*)malloc(info.size + 1);
      size_t actual; 
      if (kv_get(key, buffer, info.size, &actual) == MBED_SUCCESS) {
        buffer[actual] = '\0';
        
        return buffer;
      } 
    }

    monitor.setWarning(LabelStoreReadFail);

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
  uint8_t write(const char *key, const char *value) {
    if (!String(key).equals("config")) {

      return kv_set(key, value, strlen(value), 0) == MBED_SUCCESS ? 1 : 0;
    }

    return 0;
  }

  /**
   * Deleted a stored key.
   *
   * @param   key   The key to delete.
   *
   * @return  1 on success, else 0
   */
  uint8_t erase(const char *key) {
    if (!String(key).equals("config")) {

      return kv_remove(key) == MBED_SUCCESS ? 1 : 0;
    }

    return 0;
  }

  uint32_t getBootCount() {
    return _bootCount;
  }

}; // class OptaLinkerStore

} // namespace optalinker

#endif // #ifndef OPTALINKER_STORE_H