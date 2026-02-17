/*
 * OptaLinker
 *
 * Author: Jean-Christian Paul Denis
 * Source: https://github.com/JcDenis/OptaLinker
 *
 * @see OptaLinker.h
 * @see README.md file
 */

#ifndef OPTALINKER_VERSION_H
#define OPTALINKER_VERSION_H

#include "OptaLinkerModule.h"

namespace optalinker {

class OptaLinkerVersion : public OptaLinkerModule {

private:

  /**
   * The library major version.
   */
  static const uint8_t _major = 1;

  /**
   * The library minor version.
   */
  static const uint8_t _minor = 0;

  /**
   * The library revision.
   */
  static const uint8_t _revision = 0;

public:
	OptaLinkerVersion() {}

  static uint8_t getMajor() {
    return _major;
  }

  static uint8_t getMinor() {
    return _minor;
  }

  static uint8_t getRevision() {
    return _revision;
  }

  /**
   * Get human readable version.
   *
   * @return The string version on form x.y.z
   */
  static String toString() {

    return String(_major) + "." + _minor + "." + _revision;
  }

  /**
   * Get numeric version.
   *
   * @return The numeric version on form xxyyzz
   */
  static uint32_t toInt() {
    char buffer[7];
    sprintf(buffer, "%02d%02d%02d", _major, _minor, _revision);

    return atoi(buffer);
  }

}; // class OptaLinkerVersion

} // namespace optalinker

#endif // #ifndef OPTALINKER_VERSION_H