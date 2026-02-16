/*
 * OptaLinker
 *
 * Author: Jean-Christian Paul Denis
 * Source: https://github.com/JcDenis/OptaLinker
 *
 * @see OptaLinker.h
 * @see README.md file
 */

#ifndef OPTALINKER_IO_H
#define OPTALINKER_IO_H

#include <OptaBlue.h>

#include "OptaLinkerBase.h"

namespace optalinker {

class OptaLinkerState;
class OptaLinkerMonitor;
class OptaLinkerBoard;
class OptaLinkerConfig;

class OptaLinkerIo : public OptaLinkerBase {

private:
  OptaLinkerState &state;
  OptaLinkerMonitor &monitor;
  OptaLinkerBoard &board;
  OptaLinkerStore &store;
  OptaLinkerConfig &config;

  /*const*/ uint32_t BoardInputs[8] = { A0, A1, A2, A3, A4, A5, A6, A7 };      // I1, I2, I3, I4, I5, I6, I7, I8
  /*const*/ uint32_t BoardOutputs[4] = { D0, D1, D2, D3 };                     // O1, O2, O3, O4
  /*const*/ uint32_t BoardOutputsLeds[4] = { LED_D0, LED_D1, LED_D2, LED_D3 }; // O1, O2, O3, O4

  /**
   * Memorized if it is time to poll io.
   */
  uint8_t _isPoll = 0;

  /**
   * io poll delay.
   */
  uint32_t _pollDelay = 50;

  /**
   * Last poll time.
   */
  uint32_t _pollLast = 0;

  /**
   * Last store time.
   */
  uint32_t _storeLast = 0;

  /**
   * Number of expansion (including main board).
   */
  uint8_t _expansionsNum = 0;

  /**
   * Expansions stack with their IO (including main baord).
   */
  ExpansionStruct _expansion[OPTA_CONTROLLER_MAX_EXPANSION_NUM + 1];

public:
  OptaLinkerIo(OptaLinkerState &_state, OptaLinkerMonitor &_monitor, OptaLinkerBoard &_board, OptaLinkerStore &_store, OptaLinkerConfig &_config) : state(_state), monitor(_monitor), board(_board), store(_store), config(_config) {}

  uint8_t setup() {

    monitor.setAction(LabelIoSetup);

    analogReadResolution(12);

    // Display expansion setup message
    monitor.setInfo(LabelIoExpansionSetup);

    // Start Opta controler (expansions)
    board.freezeTimeout();
    OptaController.begin();

    // add main board to expansions count
    _expansionsNum = OptaController.getExpansionNum() + 1;

    // Display number of expansions
    monitor.setInfo(LabelIoExpansionNum + String(_expansionsNum -1));
    board.unfreezeTimeout();

    // init then read values from storage
    initializeIo();
    readFromFile();
    writeToFile();

    return 1;
  }

  uint8_t loop() {
    _isPoll = 0;
    if ((_pollDelay > 0) && ((_pollLast == 0) || ((state.getTime() - _pollLast) > _pollDelay))) {
      _pollLast = state.getTime();
      _isPoll = 1;
    }

    // update inputs
    if (_isPoll) {

      // read board inputs
      for (uint8_t i = 0; i < 8; i++) {
        if (_expansion[0].input[i].exists) {

          // analog, convert input state into mV
          uint16_t ar = (int)(analogRead(BoardInputs[i]) * 10.0 / 4095.0 * 1000);
          _expansion[0].input[i].voltage = ar;

          // state, take into account module voltage and previous state, main board is 0~10V
          uint8_t dr = (!_expansion[0].input[i].state && ar > 6500) || (_expansion[0].input[i].state && ar > 3500);

          // high
          if (dr && _expansion[0].input[i].state) {
            _expansion[0].input[i].high += _pollDelay;
            _expansion[0].input[i].partialHigh += _pollDelay;
          }

          // pulse
          if (dr && !_expansion[0].input[i].state) {
            _expansion[0].input[i].pulse++;
            _expansion[0].input[i].partialPulse++;
          }

          // state
          if (dr != _expansion[0].input[i].state) {
            monitor.setInfo(String("[I0.") + i + "] => " + dr + ", " + _expansion[0].input[i].voltage + "mV, Pulse: " + _expansion[0].input[i].pulse);

            _expansion[0].input[i].state = dr;
            _expansion[0].input[i].update = state.getTime();
          }
        }
      }

      // read board ouput
      for (uint8_t i = 0; i < 4; i++) {
        // allways increment "high" on output
        if (_expansion[0].output[i].exists && _expansion[0].output[i].state) {
          _expansion[0].output[i].high += _pollDelay;
          _expansion[0].output[i].partialHigh += _pollDelay;
        }
      }

      // read expansion
      for (uint8_t n = 0; n < OptaController.getExpansionNum(); n++) {
        uint8_t e = n + 1;
        if (_expansion[e].exists && _expansion[e].type != ExpansionType::ExpansionAnalog) { // Analog expansion not yet implemented

          DigitalMechExpansion expDmec = OptaController.getExpansion(n); 
          DigitalStSolidExpansion expDsts = OptaController.getExpansion(n);

          // Update all inputs in one step
          if (expDmec) {
            expDmec.updateAnalogInputs();
          } else if (expDsts) {
            expDsts.updateAnalogInputs();
          }

          // read input
          for (uint8_t i = 0; i < OPTA_DIGITAL_IN_NUM; i++) {
            if (_expansion[e].input[i].exists) {

              // analog (pinVoltage() arg to false as we update all inputs previously)
              uint16_t ar = 0;
              if (expDmec) {
                ar = (uint16_t)(expDmec.pinVoltage(i, false) * 1000);
              } else if (expDsts) {
                ar = (uint16_t)(expDsts.pinVoltage(i, false) * 1000);
              }
              _expansion[e].input[i].voltage = ar;

              // state, take into account module voltage and previous state, expansions are 0~24V
              uint8_t dr = (!_expansion[e].input[i].state && ar > 16000) || (_expansion[e].input[i].state && ar > 8000);

              // high
              if (dr && _expansion[e].input[i].state) {
                _expansion[e].input[i].high += _pollDelay;
                _expansion[e].input[i].partialHigh += _pollDelay;
              }

              // pulse
              if (dr && !_expansion[e].input[i].state) {
                _expansion[e].input[i].pulse++;
                _expansion[e].input[i].partialPulse++;
              }

              // state
              if (dr != _expansion[e].input[i].state) {
                monitor.setInfo(String("[I") + e + "." + i + "] => " + dr + ", " + _expansion[e].input[i].voltage + "mV, " + _expansion[e].input[i].pulse + " pulses, " + _expansion[e].input[i].high + "ms High");

                _expansion[e].input[i].state = dr;
                _expansion[e].input[i].update = state.getTime();
              }
            }
          }

          // read ouput
          for (uint8_t i = 0; i < OPTA_DIGITAL_OUT_NUM; i++) {
            // allways increment "high" on output
            if (_expansion[e].output[i].exists && _expansion[e].output[i].state) {
              _expansion[e].output[i].high += _pollDelay;
              _expansion[e].output[i].partialHigh += _pollDelay;
            }
          }
        }
      }
    }

    // Do not write expansion values to file on startup
    if (_storeLast == 0) {
      _storeLast = state.getTime();
    }

    // write expansion values into file every minute
    if (state.getTime() - _storeLast > 60000) {
      monitor.setInfo(LabelIoStore);
      _storeLast = state.getTime();
      writeToFile();
    }

    return 1;
  }

  /**
   * Convert IO pin to uniq ID.
   *
   * Ex: if expansion = 1 and input = 2: uid = 102
   * Ex: if expansion = 0 and input = 3: uid = 3
   *
   * @param   expansion The expansion id
   * @param   pin       The expansion io id
   *
   * @return The io uid.
   */
  uint16_t toPinId(uint8_t expansion, uint8_t pin) {
    if (expansion < _expansionsNum && pin < getMaxInputNum()) {
      char buffer[4];
      sprintf(buffer, "%01d%02d", expansion, pin);

      return atoi(buffer);
    }

    return 0;
  }

  /**
   * Initialize expansion stack.
   */
  void initializeIo() {
    // initialize io by filling in full array with default values
    for(uint8_t e = 0; e < OPTA_CONTROLLER_MAX_EXPANSION_NUM + 1; e++) {
      _expansion[e].exists = 0;
      for (uint8_t i = 0; i < getMaxInputNum(); i++) {
        _expansion[e].input[i] = { 0, toPinId(e, i), i, IoType::IoNone, 0, 0, state.getTime(), 0, 0, 0, 0, 0 };
      }
      for (uint8_t i = 0; i < getMaxOutputNum(); i++) {
        _expansion[e].output[i] = { 0, toPinId(e, i), i, IoType::IoNone, 0, 0, state.getTime(), 0, 0, 0, 0, 0 };
      }
    }

    // initialize main board io
    _expansion[0].exists = 1;
    _expansion[0].id = 0;
    _expansion[0].name = board.getName();
    _expansion[0].type = ExpansionType::ExpansionNone;
    for (uint8_t i = 0; i < 8; i++) {
      _expansion[0].input[i].exists = 1;
      _expansion[0].input[i].type = IoType::IoDigital;
    }
    for (uint8_t i = 0; i < 4; i++) {
      _expansion[0].output[i].exists = 1;
      _expansion[0].input[i].type = IoType::IoRelay;
    }

    // initialize expansions io
    for (uint8_t n = 0; n < OptaController.getExpansionNum(); n++) {
      uint8_t e = n + 1;
      _expansion[e].exists = 1;
      _expansion[e].id = e;

      uint8_t expType = OptaController.getExpansionType(n);
      switch (expType) {
        case EXPANSION_OPTA_DIGITAL_MEC:
          _expansion[e].type = ExpansionType::ExpansionDmec;
          for (uint8_t i = 0; i < OPTA_DIGITAL_IN_NUM; i++) {
            _expansion[e].input[i].exists = 1;
            _expansion[e].input[i].type = IoType::IoDigital;
          }
          for (uint8_t i = 0; i < OPTA_DIGITAL_OUT_NUM; i++) {
            _expansion[e].output[i].exists = 1;
            _expansion[e].input[i].type = IoType::IoRelay;
          }
          break;

        case EXPANSION_OPTA_DIGITAL_STS:
          _expansion[e].type = ExpansionType::ExpansionDsts;
          for (uint8_t i = 0; i < OPTA_DIGITAL_IN_NUM; i++) {
            _expansion[e].input[i].exists = 1;
            _expansion[e].input[i].type = IoType::IoDigital;
          }
          for (uint8_t i = 0; i < OPTA_DIGITAL_OUT_NUM; i++) {
            _expansion[e].output[i].exists = 1;
            _expansion[e].input[i].type = IoType::IoRelay;
          }
          break;

        case EXPANSION_OPTA_ANALOG:
          _expansion[e].type = ExpansionType::ExpansionAnalog;
          for (uint8_t i = 0; i < OA_AN_CHANNELS_NUM; i++) {
            _expansion[e].input[i].exists = 1;
            _expansion[e].input[i].type = IoType::IoAnalog;
          }
          // output not implemented
          break;

        default:
          _expansion[e].exists = 0;
          _expansion[e].type = ExpansionType::ExpansionNone;
      }
      _expansion[e].name = getName(e);

      // Display expansion name
      monitor.setInfo(LabelIoExpansionName + String(e) + ": " + _expansion[e].name);
    }
  }

  /**
   * Write an expansion stack io to JSON document instance.
   *
   * @param   ios   An io instance
   *
   * @return  A JSon document of this io
   */
  JsonDocument writeToJsonIo(IoStruct ios) {
    // note: can not pass by reference JsonDocument
    JsonDocument doc;
    if (ios.exists) {
      doc["exists"]       = 1;
      doc["uid"]          = ios.uid;
      doc["id"]           = ios.id;
      doc["type"]         = ios.type;
      doc["state"]        = ios.state;
      doc["voltage"]      = ios.voltage;
      doc["update"]       = ios.update;
      doc["reset"]        = ios.reset;
      doc["pulse"]        = ios.pulse;
      doc["partialPulse"] = ios.partialPulse;
      doc["high"]         = ios.high;
      doc["partialHigh"]  = ios.partialHigh;
    } else {
      doc["exists"] = 0;
    }

    return doc;
  }

  /**
   * Write expansion stack to JSON document string.
   *
   * @return  The JSON String
   */
  String writeToJson() {
    JsonDocument doc;
    for(uint8_t e = 0; e < _expansionsNum; e++) {
      if (_expansion[e].exists) {
        String de = "e" + String(e);
        doc[de]["type"] = _expansion[e].type;
        doc[de]["id"] = _expansion[e].id;
        doc[de]["name"] = _expansion[e].name;
        for (uint8_t i = 0; i < getMaxInputNum(); i++) {
          doc[de]["input"][String("i" + String(i))] = writeToJsonIo(_expansion[e].input[i]);
        }
        for (uint8_t i = 0; i < getMaxOutputNum(); i++) {
          doc[de]["output"][String("o" + String(i))] = writeToJsonIo(_expansion[e].output[i]);
        }
      }
    }

    String jsonString;
    serializeJson(doc, jsonString);

    return jsonString;
  }

  /**
   * Read an io from an io JSON document.
   *
   * @param   ios   An io instance
   * @param   doc   An io JSON document
   */
  void readFromJsonIo(IoStruct &ios, JsonDocument doc) {
    if (ios.exists && !doc.isNull()) {
      //ios.update       = doc["update"].isNull() ? state.getTime() : doc["update"].as<int>();
      ios.reset        = doc["reset"].isNull() ? 0 : doc["reset"].as<int>();
      ios.pulse        = doc["pulse"].isNull() ? 0 : doc["pulse"].as<int>();
      ios.partialPulse = doc["partialPulse"].isNull() ? 0 : doc["partialPulse"].as<int>();
      ios.high         = doc["high"].isNull() ? 0 : doc["high"].as<int>();
      ios.partialHigh  = doc["partialHigh"].isNull() ? 0 : doc["partialHigh"].as<int>();
    }
  }

  /**
   * Read expansion stack from a JSON document string.
   *
   * @param   buffer  The JSON document buffer
   * @param   length  The JSON document length
   *
   * @return  1 on succes, else 0
   */
  uint8_t readFromJson(const char *buffer, size_t length) {

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, buffer, length);

    if (error) {
      monitor.setWarning(LabelConfigJsonReadFail);

      return 0;
    }

    for(uint8_t e = 0; e < _expansionsNum; e++) {
      if (_expansion[e].exists) {
        String de = "e" + String(e);
        // Check if expansion has been changed physicaly
        if (!doc[de]["type"].isNull() && _expansion[e].type == doc[de]["type"].as<int>()) {
          for (uint8_t i = 0; i < getMaxInputNum(); i++) {
            readFromJsonIo(_expansion[e].input[i], doc[de]["input"][String("i" + String(i))]);
          }
          for (uint8_t i = 0; i < getMaxOutputNum(); i++) {
            readFromJsonIo(_expansion[e].output[i], doc[de]["output"][String("o" + String(i))]);
          }
        }
      }
    }

    return 1;
  }

  /**
   * Write expansion stack to flash memory.
   *
   * @return  1 on success, else 0
   */
  uint8_t writeToFile() {
    String stored = writeToJson();

    return store.write("io", stored.c_str());
  }

  /**
   * Read expansion stack from flash memory.
   *
   * @return 1 on success, else 0
   */
  uint8_t readFromFile() {
    String stored = store.read("io");

    return readFromJson(stored.c_str(), stored.length());
  }

  /**
   * Get number of expansions (including main board).
   *
   * @return The number of expansions
   */
  uint8_t getExpansionsNum() {

    return _expansionsNum;
  }

  /**
   * Get maximum number of expansions (including main board).
   *
   * @return The maximum number of expansions
   */
  uint8_t getMaxExpansionsNum() {

    return OPTA_CONTROLLER_MAX_EXPANSION_NUM + 1;
  }

  /**
   * Get expansions stack instance.
   *
   * @return The expansion stack
   */
  ExpansionStruct *getExpansions() {

    return _expansion;
  }

  /**
   * Get an io field value.
   *
   * @param   ios   An io instance
   * @param   query The field to get
   *
   * @return  The field value, else -1 on error
   */
  int getIo(IoStruct ios, uint8_t query) {
    int rsp = -1;
    switch (query) {
      case IoField::IoFieldExists:
        rsp = ios.exists;
        break;

      case IoField::IoFieldType:
        rsp = ios.type;
        break;

      case IoField::IoFieldState:
        rsp = ios.state;
        break;

      case IoField::IoFieldVoltage:
        rsp = ios.voltage;
        break;

      case IoField::IoFieldUpdate:
        rsp = ios.update;
        break;

      case IoField::IoFieldReset:
        rsp = ios.reset;
        break;

      case IoField::IoFieldPulse:
        rsp = ios.pulse;
        break;

      case IoField::IoFieldPartialPulse:
        rsp = ios.partialPulse;
        break;

      case IoField::IoFieldHigh:
        rsp = ios.high;
        break;

      case IoField::IoFieldPartialHigh:
        rsp = ios.partialHigh;
        break;
    }

    return rsp;
  }

  /**
   * Get maximum number of input.
   *
   * @return the maximum number of input
   */
  uint8_t getMaxInputNum() {
    return 16;
  }

  /**
   * Get maximum number of output.
   *
   * @return the maximum number of output
   */
  uint8_t getMaxOutputNum() {
    return 8;
  }

  /**
   * Get an input value.
   *
   * @param   expansion The expension um
   * @param   input     The input num
   * @param   query     The input field
   *
   * @return  The value, else -1
   */
  int getInput(uint8_t expansion, uint8_t input, uint8_t query) {
    if (expansion < _expansionsNum && input < getMaxInputNum() && _expansion[expansion].exists && _expansion[expansion].input[input].exists) {
      return getIo(_expansion[expansion].input[input], query);
    }

    return -1;
  }

  /**
   * Get an output value.
   *
   * @param   expansion   The expension um
   * @param   output      The output num
   * @param   query       The output field
   *
   * @return  The value, else -1
   */
  int getOutput(uint8_t expansion, uint8_t output, uint8_t query) {
    if (expansion < _expansionsNum && output < getMaxOutputNum() && _expansion[expansion].exists && _expansion[expansion].output[output].exists) {
      return getIo(_expansion[expansion].output[output], query);
    }

    return -1;
  }

  /**
   * Reset an io.
   *
   * @param   ios   An io instance
   */
  void resetIo(IoStruct &ios) {
    if (ios.exists) {
      ios.partialPulse = 0;
      ios.partialHigh = 0;
      ios.reset = state.getTime();
      ios.update = state.getTime();
    }
  }

  /**
   * Reset an input.
   *
   * @param   expansion   The expansion um
   * @param   input       The input num
   */
  void resetInput(uint8_t expansion, uint8_t input) {
    if (_expansion[expansion].exists) {
      resetIo(_expansion[expansion].input[input]);
    }
  }

  /**
   * Reset an output.
   *
   * @param   expansion   The expansion um
   * @param   output      The output num
   */
  void resetOutput(uint8_t expansion, uint8_t output) {
    if (_expansion[expansion].exists) {
      resetIo(_expansion[expansion].output[output]);
    }
  }

  /**
   * Set an output value.
   *
   * Only digital output is implemented
   *
   * @param   expansion   The expansion num
   * @param   output      The output num
   * @param   value       The output value
   */
  void setOutput(uint8_t expansion, uint8_t output, uint8_t value) {
    value = value > 0 ? 1 : 0;
    if (expansion < _expansionsNum && output < 8 && _expansion[expansion].exists && _expansion[expansion].output[output].exists) {
      // only if state change
      if (value != _expansion[expansion].output[output].state) {
        // main board
        if (expansion == 0) {
          digitalWrite(BoardOutputs[output], value);
          digitalWrite(BoardOutputsLeds[output], value);
        // Digital mech expansion
        } else if (_expansion[expansion].type == ExpansionType::ExpansionDmec) {
          DigitalMechExpansion expDmec = OptaController.getExpansion(expansion - 1);
          expDmec.digitalWrite(expansion - 1, value ? HIGH : LOW, true);
          expDmec.updateDigitalOutputs();
          //expDmec.switchLedOn(expansion - 1, value ? HIGH : LOW, true);
          //expDmec.updateLeds();
        // Digital sts expansion
        } else if (_expansion[expansion].type == ExpansionType::ExpansionDmec) {
          DigitalStSolidExpansion expDsts = OptaController.getExpansion(expansion - 1);
          expDsts.digitalWrite(expansion - 1, value ? HIGH : LOW, true);
          expDsts.updateDigitalOutputs();
          //expDsts.switchLedOn(expansion - 1, value ? HIGH : LOW, true);
          //expDsts.updateLeds();
        }

        // pulse
        if (value && !_expansion[expansion].output[output].state) {
          _expansion[expansion].output[output].pulse++;
          _expansion[expansion].output[output].partialPulse++;
        }

        _expansion[expansion].output[output].state = value;
        _expansion[expansion].output[output].update = state.getTime();

        monitor.setInfo(String("[O") + expansion + "." + output+ "] => " + value + ", " + _expansion[expansion].output[output].pulse + " pulses, " + _expansion[expansion].output[output].high + "ms high");
      }
    }
  }

  /**
   * Get human readable expansion name.
   *
   * @param   expansion   The expansion slot
   *
   * @return  The huma readable expension name
   */
  String getName(uint8_t expansion) {
    String name;
    // get main board name
    if (expansion == 0) {
      name = board.getName();
    } else {
      switch (_expansion[expansion].type) {
        case ExpansionType::ExpansionDmec:
          name = LabelIoExpansionDmec;
          break;

        case ExpansionType::ExpansionDsts:
          name = LabelIoExpansionDsts;
          break;

        case ExpansionType::ExpansionAnalog:
          name = LabelIoExpansionAnalog;
          break;

        default:
          name = LabelIoExpansionNone;
      }
    }

    return name;
  }

  /**
   * Set io poll delay.
   *
   * @param   delay  The io poll delay in ms
   */
  void setPollDelay(uint32_t delay) {
    monitor.setInfo(LabelIoPoll + String(delay));
    _pollDelay = delay;
  }

  /**
   * Check if it is itme to poll io
   *
   * @return  1 on time, else 0
   */
  uint8_t isPoll() {

    return _isPoll;
  }

}; // class OptaLinkerIo

} // namespace optalinker

#endif // #ifndef OPTALINKER_IO_H