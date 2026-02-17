/*
 * OptaLinker
 *
 * Author: Jean-Christian Paul Denis
 * Source: https://github.com/JcDenis/OptaLinker
 *
 * @see OptaLinker.h
 * @see README.md file
 */

#ifndef OPTALINKER_MONITOR_H
#define OPTALINKER_MONITOR_H

#include "OptaLinkerModule.h"

namespace optalinker {

class OptaLinkerState;

class OptaLinkerMonitor : public OptaLinkerModule {

private:
  OptaLinkerState &state;

  /**
   * Colorized some messages.
   */
  uint8_t _colorized = 0;

  /**
   * Heartbeat timer.
   */
	uint32_t _heartbeat = 0;

	/**
	 * Progress percent.
	 */
  uint8_t _progress = 0;

  /**
   * Received message.
   */
  char _incoming[50]; // max length of 50

  /**
   * A message is available.
   */
  uint8_t _hasIncoming = 0;

public:
	OptaLinkerMonitor(OptaLinkerState &_state) : state(_state) {}

  uint8_t setup() {
    // Prepare serial monitor (USB)
    Serial.begin(115200);

    // Wait for Serial a maximum of 5 seconds
    for (size_t i = 0; i < 5000; i++) {
      delay(1);
      if (Serial) {
        break;
      }
    }

    // Display "welcome" message
    setMessage(LabelMonitorSetup);

    return 1;
  }

  uint8_t loop() {
  	// reset incoming mesasge
  	_incoming[0] = '\0';
  	_hasIncoming = 0;

  	// Heartbeat
  	if (state.getTime() - _heartbeat > 10000) {
        setInfo(LabelMonitorHeartbeat);
        _heartbeat = state.getTime();
  	}

  	// receive message
	  uint8_t index = 0;
	  uint8_t ended = 0;

	  while (Serial.available() && !ended) {
	    int c = Serial.read();
	    if (c != -1) {
	      switch (c) {
	        case '\n':
	          _incoming[index] = '\0';
	          setAction(String(LabelMonitorReceive) + _incoming);
	          index = 0;
	          ended = 1;
	          break;
	        default:
	          if (index < 50) {  // max length of 50
	            _incoming[index++] = (char)c;
	            _hasIncoming = 1;
	          }
	          break;
	      }
	    }
	  }

    return 1;
  }

  /**
   * Set colorized message if serial monitor supports this.
   *
   * @param 	set 	1 to enabled colorized message, else 0
   */
  void setColorisation(uint8_t set = 1) {
  	_colorized = set == 1 ? 1 : 0;
  }

  /**
   * Print a message (whitout newline).
   *
   * @param 	str 	The message.
   */
	void setMessage(String str) {
	  setMessage(str.c_str());
	}
	void setMessage(const char *str) {
	  Serial.print(String(LabelMonitorPrint) + str);
	}

	/**
	 * Print an action message.
	 *
   * @param 	str 	The message.
   */
	void setAction(String str) {
	  setAction(str.c_str());
	}
	void setAction(const char *str) {
		if (_colorized) Serial.print("\033[32m"); // green
	  Serial.print(String(LabelMonitorAction) + str);
	  Serial.println(_colorized ? "\033[0m" : "");
	}


	/**
	 * Print an informational message.
	 *
   * @param 	str 	The message.
   */
	void setInfo(String str) {
	  setInfo(str.c_str());
	}
	void setInfo(const char *str) {
		if (_colorized) Serial.print("\033[33m"); // yellow
	  Serial.print(String(LabelMonitorInfo) + str);
	  Serial.println(_colorized ? "\033[0m" : "");
	}


	/**
	 * Print a warning message.
	 *
   * @param 	str 	The message.
   */
	void setWarning(String str) {
	  setWarning(str.c_str());
	}
	void setWarning(const char *str) {
		if (_colorized) Serial.print("\033[31m"); // red
	  Serial.print(String(LabelMonitorWarning) + str);
	  Serial.println(_colorized ? "\033[0m" : "");
	}


	/**
	 * Print a progress percent.
	 *
   * @param 	offset			The current position
   * @param 	size 				The total size
   * @param 	threshold 	The thresold
   * @param 	reset 			Reset progress
   */
	void setProgress(uint32_t offset, uint32_t size, uint32_t threshold, uint8_t reset) {
	  if (reset == 1) {
	    _progress = 0;
	    setInfo(String(_progress) + "%");
	  } else {
	    uint8_t percent_done_new = offset * 100 / size;
	    if (percent_done_new >= _progress + threshold) {
	      _progress = percent_done_new;
	      setInfo(String(_progress) + "%");
	    }
	  }
	}

	/**
	 * Check if a message arrives.
	 */
	uint8_t hasIncoming() {
	  return _hasIncoming;
	}

	/**
	 * Get the last received message.
	 *
	 * A received message is available only for the current loop and is reseted on each loop.
	 */
	String getIncoming() {
	  String ret = String(_incoming);
	  ret.toLowerCase();

	  return ret;
	}

}; // class OptaLinkerConfig

} // namespace optalinker

#endif // #ifndef OPTALINKER_MONITOR_H