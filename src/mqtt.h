/*
 * OptaLinker
 *
 * Author: Jean-Christian Paul Denis
 * Source: https://github.com/JcDenis/OptaLinker
 *
 * @see OptaLinker.h
 * @see README.md file
 */

#ifndef OPTALINKER_MQTT_H
#define OPTALINKER_MQTT_H

#include <ArduinoMqttClient.h>
#include <Ethernet.h>
#include <WiFi.h>

#include "OptaLinkerModule.h"

namespace optalinker {

class OptaLinkerVersion;
class OptaLinkerState;
class OptaLinkerMonitor;
class OptaLinkerBoard;
class OptaLinkerConfig;
class OptaLinkerIo;
class OptaLinkerNetwork;
class OptaLinkerRs485;

class OptaLinkerMqtt : public OptaLinkerModule {

private:
  OptaLinkerVersion &version;
  OptaLinkerState &state;
  OptaLinkerMonitor &monitor;
  OptaLinkerBoard &board;
  OptaLinkerConfig &config;
  OptaLinkerIo &io;
  OptaLinkerNetwork &network;
  OptaLinkerRs485 &rs485;

  /**
   * The MQTT base topic.
   */
  String _baseTopic = "undefined";

  /**
   * Last connection retry time.
   */
  uint32_t _lastRetry = 0;

  /**
   * Last periodical update time.
   */
  uint32_t _lastUpdate = 0;

  /**
   * Last publish time.
   */
  uint32_t _lastPublish = 0;

  /**
   * last statictics topics update time.
   */
  uint32_t _lastStatistic = 0;

  /**
   * MQTT connection status.
   */
  uint8_t _isConnected = 0;
  
  /**
   * Ethernet client for MQTT.
   */
  EthernetClient _ethernetClient;

  /**
   * Wifi client for MQTT.
   */
  WiFiClient _wifiClient;

  /**
   * Generic client for MQTT.
   */
  MqttClient _genericClient = nullptr;

  /**
   * Last LED change time.
   */
  uint32_t _ledLast = 0;

  /**
   * Last LED state.
   */
  uint8_t _ledState = 0;

  /**
   * Connect to MQTT server.
   */
  void connect() {
    // Do not try to connect if nonetwork or under Wifi Access point
    if (!network.isConnected() || network.isAccessPoint()) {

      return;
    }

    if (_genericClient.connected()) {
      _isConnected = 1;
      _lastRetry = 0;

      return;
    }

    if (!network.isPoll(_lastRetry)) {  // retry every x seconds

      return;
    }

    _isConnected = 0;
    _lastRetry = millis();
    monitor.setMessage(LabelMqttBroker, MonitorAction);

    board.setFreeze();
    _genericClient.setId(String("opta" + config.getDeviceId()).c_str());
    _genericClient.setUsernamePassword(config.getMqttUser(), config.getMqttPassword());
    _genericClient.setConnectionTimeout(network.getTimeout()); // This directive has no effect !
    if (!_genericClient.connect(config.getMqttIp(), config.getMqttPort())) {
      monitor.setMessage(LabelMqttBrokerFail, MonitorWarning);
      board.unsetFreeze();

      return;
    }
    board.unsetFreeze();

    monitor.setMessage(LabelMqttBrokerSuccess, MonitorInfo);
    _isConnected = 1;

    // subscribe to command for device information
    _genericClient.subscribe(_baseTopic + "device/get");
    monitor.setMessage(LabelMqttSubscribe + _baseTopic + "device/get", MonitorInfo);

    // subscribe to commands for outputs
    _genericClient.subscribe(_baseTopic + "output/set/#");
    monitor.setMessage(LabelMqttSubscribe + _baseTopic + "output/set/#", MonitorInfo);

    // subscribe to commands for outputs reset high
    _genericClient.subscribe(_baseTopic + "output/reset/#");
    monitor.setMessage(LabelMqttSubscribe + _baseTopic + "output/reset/#", MonitorInfo);

    // subscribe to commands for inputs reset high
    _genericClient.subscribe(_baseTopic + "input/reset/#");
    monitor.setMessage(LabelMqttSubscribe + _baseTopic + "input/reset/#", MonitorInfo);

    publishDevice();
  }

  /**
   * Parse received message.
   *
   * @param   topic     The MQTT topic
   * @param   payload   The MQTT payload
   */
  void receiveMessage(String &topic, String &payload) {
    monitor.setMessage(LabelMqttReceive + topic + " = " + payload, MonitorAction);

    String match = _baseTopic + "device/get";
    if (topic == match) {
      publishDevice();
    }

    ExpansionStruct *expansion = io.getExpansions();
    for (uint8_t e = 0; e < io.getExpansionsNum(); e++) {
      if (expansion[e].exists) {
        for (uint8_t i = 0; i < 16; i++) {

          if (expansion[e].input[i].exists) {
            // reset output state
            if (topic.equals(_baseTopic + "input/reset/" + String(expansion[e].input[i].id))) {
              monitor.setMessage("Resetting from MQTT input " + String(expansion[e].input[i].id), MonitorInfo);

              io.resetInput(e, i);
            }
          }

          if (expansion[e].output[i].exists) {
            // reset output state
            if (topic.equals(_baseTopic + "output/reset/" + String(expansion[e].output[i].id))) {
              monitor.setMessage("Resetting from MQTT output " + String(expansion[e].output[i].id), MonitorInfo);

              io.resetOutput(e, i);
            }

            // set output state
            if (topic.equals(_baseTopic + "output/set/" + String(expansion[e].output[i].id))) {
              monitor.setMessage("Setting from MQTT output " + String(expansion[e].output[i].id) + " to " + payload, MonitorInfo);

              io.setOutput(e, i, payload.toInt());
            }
          }
        }
      }
    }
  }

  /**
   * Publish to MQTT io topics.
   *
   * @param   ios     An io instance
   * @param   force   Force publish even if value did not changed
   */
  void publishIo(String topic, IoStruct ios, uint8_t force = false) {
    if (ios.exists) {
      String idTopic = String(ios.id);

      if (force || ios.update > _lastPublish) {
        publishMessage(topic + "state/" + idTopic, String(ios.state));
        publishMessage(topic + "voltage/" + idTopic, String(ios.voltage));
        publishMessage(topic + "pulse/" + idTopic, String(ios.pulse));
        publishMessage(topic + "partialPulse/" + idTopic, String(ios.partialPulse));
        publishMessage(topic + "high/" + idTopic, String(ios.high));
        publishMessage(topic + "partialHigh/" + idTopic, String(ios.partialHigh));

      } else if (state.getTime() - _lastStatistic > 60000) {
        publishMessage(topic + "high/" + idTopic, String(ios.high));
        publishMessage(topic + "partialHigh/" + idTopic, String(ios.partialHigh));
      }
    }
  }


public:
  OptaLinkerMqtt(OptaLinkerVersion &_version, OptaLinkerState &_state, OptaLinkerMonitor &_monitor, OptaLinkerBoard &_board, OptaLinkerConfig &_config, OptaLinkerIo &_io, OptaLinkerNetwork &_network, OptaLinkerRs485 &_rs485) : version(_version), state(_state), monitor(_monitor), board(_board), config(_config), io(_io), network(_network), rs485(_rs485) {}

  uint8_t setup() {
    // Disable MQTT feature
    if (config.getMqttIp().toString().equals("0.0.0.0")) {
      disable();

      return 1;
    }

    monitor.setMessage(LabelMqttSetup, MonitorAction);
    monitor.setMessage(LabelMqttServer + config.getMqttIp().toString() + ":" + String(config.getMqttPort()), MonitorInfo);

    _baseTopic = config.getMqttBase() + config.getDeviceId() + "/";

    board.setFreeze();
    if (network.isEthernet()) {
      MqttClient tempMqttClient(_ethernetClient);
      _genericClient = tempMqttClient;
    } else {
      MqttClient tempMqttClient(_wifiClient);
      _genericClient = tempMqttClient;
    }
    board.unsetFreeze();

    connect();

    return 1;
  }

  uint8_t loop() {
    connect();
    if (isConnected()) {

      // Write periodically all inputs values to MQTT
      if (config.getMqttInterval() > 0 && ((state.getTime() - _lastUpdate) > (config.getMqttInterval() * 1000))) {
        _lastUpdate = state.getTime();
        publishDevice();
        publishInputs();
      }

      // Write changed inputs values to MQTT
      if (io.isPoll()) {
        ExpansionStruct *expansion = io.getExpansions();
        for (uint8_t e = 0; e < io.getExpansionsNum(); e++) {
          if (expansion[e].exists) {
            for (uint8_t i = 0; i < 16; i++) {
              publishIo(_baseTopic + "input/", expansion[e].input[i]);
            }
            for (uint8_t i = 0; i < 8; i++) {
              publishIo(_baseTopic + "output/", expansion[e].output[i]);
            }
          }
        }
        _lastPublish = state.getTime();
        // every minute publish "high" stats
        if (state.getTime() - _lastStatistic > 60000) {
          _lastStatistic = state.getTime();
        }
      }

      // Read output command from MQTT
      int rspSize = _genericClient.parseMessage();
      if (rspSize) {
        String rspTopic = _genericClient.messageTopic();
        String rspPayload = "";
        for (int index = 0; index < rspSize; index++) {
          rspPayload += (char)_genericClient.read();
        }

        receiveMessage(rspTopic, rspPayload);
      }

      // Write rs485 incoming message to MQTT
      if (config.getRs485ToMqtt() && rs485.incoming()) {
        publishMessage(_baseTopic + "rs485", rs485.received());
      }

    }

    // Fix LED if MQTT is connected
    if (isConnected()) {
      board.setGreen(1);
    // Blink LEDs if MQTT is notconnected
    } else if (state.getTime() - _ledLast > 750) {
      _ledLast = state.getTime();
      _ledState = _ledState ? 0 : 1;

      board.setGreen(network.isConnected() ? _ledState : 0);
    }

    // Check button to publish inputs to MQTT
    if (isConnected() && board.isPushDuration(0, 1000)) {
      publishDevice();
      publishInputs();
    }

    return 1;
  }

  /**
   * Check if it is conected to MQTT server.
   */
  uint8_t isConnected() {

    return _isConnected;
  }

  /**
   * Subscribe to a topic.
   *
   * @param   topic   The topic to subscribe to
   *
   * @return  1 on success, else 0
   */
  uint8_t subscribeTopic(String topic) {
    if(isConnected()) {
      _genericClient.subscribe(topic);

      return 1;
    }

    return 0;
  }

  /**
   * Publish a message.
   *
   * @param   topic     The topic to publish to
   * @param   message   The message (payload) to publish
   *
   * @return  1 on success, else 0
   */
  uint8_t publishMessage(String topic, String message) {
    if(isConnected()) {
      _genericClient.beginMessage(topic);
      _genericClient.print(message);
      _genericClient.endMessage();

      return 1;
    }

    return 0;
  }

  /**
   * Publish all device informations.
   */
  void publishDevice() {
    if (network.isConnected() && isConnected()) {
      monitor.setMessage(LabelMqttPublishDevice, MonitorAction);

      publishMessage(_baseTopic + "device/type", board.getName());
      publishMessage(_baseTopic + "device/ip", network.getLocalIp().toString());
      publishMessage(_baseTopic + "device/revision", String(version.toInt()));
    }
  }

  /**
   * Publish all inputs values.
   */
  void publishInputs() {
    if (network.isConnected() && isConnected()) {
      monitor.setMessage(LabelMqttPublishInput, MonitorAction);

      ExpansionStruct *expansion = io.getExpansions();
      for (uint8_t e = 0; e < io.getExpansionsNum(); e++) {
        if (expansion[e].exists) {
          for (uint8_t i = 0; i < 16; i++) {
            publishIo(_baseTopic + "input/", expansion[e].input[i], true);
          }
          for (uint8_t o = 0; o < 8; o++) {
            publishIo(_baseTopic + "output/", expansion[e].output[o], true);
          }
        }
      }
    }
  }

}; // class OptaLinkerMqtt

} // namespace optalinker

#endif // #ifndef OPTALINKER_MQTT_H