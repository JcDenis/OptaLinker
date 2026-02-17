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
   * The library version major.
   */
  uint8_t _major;

  /**
   * The library version minor.
   */
  uint8_t _minor;

  /**
   * The library version revision.
   */
  uint8_t _revision;

public:
	OptaLinkerVersion(uint8_t major, uint8_t minor, uint8_t revision) {
    _major    = major;
    _minor    = minor;
    _revision = revision;
  }

  /**
   * Get library version major.
   *
   * @return  The library version major
   */
  uint8_t getMajor() {
    return _major;
  }

  /**
   * Get library version minor.
   *
   * @return  The library version minor
   */
  uint8_t getMinor() {
    return _minor;
  }

  /**
   * Get library version revison.
   *
   * @return  The library version revision
   */
  uint8_t getRevision() {
    return _revision;
  }

  /**
   * Get human readable version.
   *
   * @return The string version on form x.y.z
   */
  String toString() {

    return String(_major) + "." + _minor + "." + _revision;
  }

  /**
   * Get numeric version.
   *
   * @return The numeric version on form xxyyzz
   */
  uint32_t toInt() {
    char buffer[10];
    sprintf(buffer, "%02d%02d%02d", _major, _minor, _revision);

    return atoi(buffer);
  }

}; // class OptaLinkerVersion

} // namespace optalinker

#endif // #ifndef OPTALINKER_VERSION_H