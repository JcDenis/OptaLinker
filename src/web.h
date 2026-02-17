/*
 * OptaLinker
 *
 * Author: Jean-Christian Paul Denis
 * Source: https://github.com/JcDenis/OptaLinker
 *
 * @see OptaLinker.h
 * @see README.md file
 */

#ifndef OPTALINKER_WEB_H
#define OPTALINKER_WEB_H

#include <Ethernet.h>
#include <mbedtls/base64.h>
#include <WiFi.h>

#include "OptaLinkerModule.h"
#include "html.h"

namespace optalinker {

class OptaLinkerVersion;
class OptaLinkerState;
class OptaLinkerMonitor;
class OptaLinkerBoard;
class OptaLinkerConfig;
class OptaLinkerIo;
class OptaLinkerNetwork;
class OptaLinkerClock;
class OptaLinkerMqtt;

class OptaLinkerWeb : public OptaLinkerModule {

private:
  OptaLinkerVersion &version;
  OptaLinkerState &state;
  OptaLinkerMonitor &monitor;
  OptaLinkerBoard &board;
  OptaLinkerConfig &config;
  OptaLinkerIo &io;
  OptaLinkerNetwork &network;
  OptaLinkerClock &clock;
  OptaLinkerMqtt &mqtt;

  /**
   * Web ethernet server.
   */
  EthernetServer _ethernetServer;

  /**
   * Web Wifi server.
   */
  WiFiServer _wifiServer;

  /**
   * Handle client.
   *
   * @param   client  The web server client
   */
  void handleClient(Client *&client) {
    client->setTimeout(5000);
    String webConnectRequest = "";
    uint32_t webConnectLength = 0;
    uint8_t webConnectAuth = 0;
    uint8_t webConnectBlank = 1;
    char webConnectBuffer[100];
    //memset(webConnectBuffer, 0, sizeof(webConnectBuffer));

    while (client->connected()) {
      if (!client->available()) { // Try to kill Wifi waiting client
        for (uint8_t i = 0; i < 250; i++) {
          delay(1);
          if (client->available()) {
            break;
          }
        }
        if (!client->available()) {
          client->stop();
          break;
        }
      }

      if (client->available()) {  // Wifi server client "available" sometimes freeze too long for the loop.
        char webChar = client->read(); // Read client request
        webConnectBuffer[webConnectLength] = webChar;
        if (webConnectLength < (int)sizeof(webConnectBuffer) - 1) {
          webConnectLength++;
        }

        if (webChar == '\n' && webConnectBlank) {
          if (webConnectAuth) {
            if (!webConnectRequest) {
              webConnectRequest = client->readStringUntil('\r'); // Grab end of request
            }

            client->flush();  // Nothing more to read

            if (webConnectRequest.startsWith("POST /form")) {
              receiveConfig(client);
            } else if (webConnectRequest.startsWith("GET /publish ")) {
              receivePublish(client);
            } else if (webConnectRequest.startsWith("GET /config ")) {
              sendConfig(client);
            } else if (webConnectRequest.startsWith("GET /data ")) {
              sendData(client);
            } else if (webConnectRequest.startsWith("GET /io ")) {
              sendIo(client);
            } else if (webConnectRequest.startsWith("GET /device ")) {
              sendDevice(client);
            } else if (webConnectRequest.startsWith("GET / ")) {
              sendHome(client);
            } else if (webConnectRequest.startsWith("GET /favicon.ico")) {
              sendFavicon(client);
            } else {
              sendError(client);
            }
          } else {
            sendAuth(client);
          }
          client->stop();
          break;
        }

        if (webChar == '\n') {
          webConnectBlank = 1;

          // Check if basic auth is present and ok
          String inputString = config.getDeviceUser() + ":" + config.getDevicePassword();
          if (strstr(webConnectBuffer, "Authorization: Basic ") && strstr(webConnectBuffer, base64EncodeString(inputString).c_str())) {
            webConnectAuth = 1;
          }

          // if web line buffer is the request
          if (strstr(webConnectBuffer, "GET /") || strstr(webConnectBuffer, "POST /")) {
            webConnectRequest = webConnectBuffer;
          }

          //memset(webConnectBuffer, 0, sizeof(webConnectBuffer));
          webConnectLength = 0;
        } else if (webChar != '\r') {
          webConnectBlank = 0;
        }
      }
    }
    client->stop();

    //yield();
  }

  /**
   * Encode to base64.
   *
   * Required to encode basic auth user:password
   *
   * @param   input   The string to encode
   */
  String base64EncodeString(const String& input) {
    const size_t inputLen = input.length();
    const size_t outSize = 4 * ((inputLen + 2) / 3) + 1;

    char out[outSize];
    size_t outLen;

    mbedtls_base64_encode(
      (unsigned char*)out,
      outSize,
      &outLen,
      (const unsigned char*)input.c_str(),
      inputLen
    );

    out[outLen] = '\0';
    return String(out);
  }

  /**
   * Send favicon.
   *
   * @param   client  The client
   */
  void sendFavicon(Client *&client) {
    client->println("HTTP/1.1 200 OK");
    client->println("Content-Type: image/x-icon");
    client->println("Connnection: close");
    client->println();

    const byte bufferSize = 48;
    uint8_t buffer[bufferSize];
    const size_t n = sizeof web_favicon_hex / bufferSize;
    const size_t r = sizeof web_favicon_hex % bufferSize;
    for (size_t i = 0; i < sizeof web_favicon_hex; i += bufferSize) {
      memcpy_P(buffer, web_favicon_hex + i, bufferSize);
      client->write(buffer, bufferSize);
    }
    if (r != 0) {
      memcpy_P(buffer, web_favicon_hex + n * bufferSize, r);
      client->write(buffer, r);
    }
  }


  /**
   * Send basic authentication request.
   *
   * @param   client  The client
   */
  void sendAuth(Client *&client) {
    client->println("HTTP/1.1 401 Authorization Required");
    client->println("WWW-Authenticate: Basic realm=\"Secure Area\"");
    client->println("Content-Type: text/html");
    client->println("Connnection: close");
    client->println();
    client->println(web_begin_html);
    client->println(web_auth_html);
    client->println(web_end_html);
  }


  /**
   * Send error page.
   *
   * @param   client  The client
   */
  void sendError(Client *&client) {
    client->println("HTTP/1.1 404 Not Found");
    client->println("Content-Type: text/html");
    client->println("Connnection: close");
    client->println();
    client->println(web_begin_html);
    client->println(web_error_html);
    client->println(web_end_html);
  }


  /**
   * Send home page (io list).
   *
   * @param   client  The client
   */
  void sendHome(Client *&client) {
    client->println("HTTP/1.1 200 OK");
    client->println("Content-Type: text/html");
    client->println("Connection: close");
    client->println();
    client->println(web_begin_html);
    client->println(web_home_html);
    client->println(web_end_html);
  }


  /**
   * Send device page (configuration).
   *
   * @param   client  The client
   */
  void sendDevice(Client *&client) {
    client->println("HTTP/1.1 200 OK");
    client->println("Content-Type: text/html");
    client->println("Connection: close");
    client->println();
    client->println(web_begin_html);
    client->println(web_device_html);
    client->println(web_end_html);
  }


  /**
   * Send config JSON contents.
   *
   * @param   client  The client
   */
  void sendConfig(Client *&client) {
    client->println("HTTP/1.1 200 OK");
    client->println("Content-Type: application/json");
    client->println("Connection: close");
    client->println();
    client->println(config.writeToJson(true));
  }


  /**
   * Send io JSON contents.
   *
   * @param   client  The client
   */
  void sendIo(Client *&client) {
    client->println("HTTP/1.1 200 OK");
    client->println("Content-Type: application/json");
    client->println("Connection: close");
    client->println();
    client->println(io.writeToJson());
  }


  /**
   * Send device JSON contents.
   *
   * @param   client  The client
   */
  void sendData(Client *&client) {
    JsonDocument doc;
    doc["deviceId"] = config.getDeviceId();
    doc["version"] = version.toString();
    doc["name"] = board.getName();
    doc["mqttConnected"] = mqtt.isConnected();
    doc["time"] = clock.toString();
    doc["gmt"] = config.getTimeOffset();

    // io
    JsonDocument exp;
    String stringExp = io.writeToJson();
    DeserializationError error = deserializeJson(exp, stringExp.c_str(), stringExp.length());
    if (!error) {
      doc["expansion"] = exp;
    }

    String jsonString;
    serializeJson(doc, jsonString);

    client->println("HTTP/1.1 200 OK");
    client->println("Content-Type: application/json");
    client->println("Connection: close");
    client->println();
    client->println(jsonString);
  }


  /**
   * Handle received configuration.
   *
   * @param   client  The client
   */
  void receiveConfig(Client *&client) {
    monitor.setAction(LabelWebConfig);

    bool isValid = true;
    String jsonString = "";

    String oldDevicePassword = config.getDevicePassword();
    String oldNetPassword = config.getNetworkPassword();
    String oldMqttPassword = config.getMqttPassword();

    board.setFreeze();
    while (client->available()) {
      String line = client->readStringUntil('\n');  // Read line-by-line

      if (line == "\r") {  // Detect the end of headers (an empty line)
        isValid = false;
        break;
      }

      jsonString += line;
    }
    board.pingTimeout();

    if (!isValid || config.readFromJson(jsonString.c_str(), jsonString.length()) < 1) {
      monitor.setWarning(LabelWebConfigFail);
      isValid = false;
    } else {
      if (config.getDeviceUser() == "") {  // device user must be set
        monitor.setWarning(LabelWebConfigFailUser);
        isValid = false;
      }
      if (config.getDevicePassword() == "") {  // get old device password if none set
        monitor.setInfo(LabelWebConfigKeepDevice);
        config.setDevicePassword(oldDevicePassword);
      }
      if (config.getNetworkPassword() == "" && config.getNetworkSsid() != "") {  // get old wifi password if none set
        monitor.setInfo(LabelWebConfigKeepWifi);
        config.setNetworkPassword(oldNetPassword);
      }

      if (config.getMqttPassword() == "" && config.getMqttUser() != "") {  // get old mqtt password if none set
        monitor.setInfo(LabelWebConfigKeepMqtt);
        config.setMqttPassword(oldMqttPassword);
      }

    }
    board.pingTimeout();

    if (isValid) {
      config.writeToFile();

      client->println("HTTP/1.1 200 OK");
      client->println("Content-Type: application/json");
      client->println("Connection: close");
      client->println();
      client->println("{\"status\":\"success\",\"message\":\"Configuration updated\"}");
      client->stop();

      board.reboot();
    } else {
      client->println("HTTP/1.1 403 FORBIDDEN");
      client->println("Content-Type: application/json");
      client->println("Connection: close");
      client->println();
      client->println("{\"status\":\"error\",\"message\":\"Configuration not updated\"}");
    }
    board.unsetFreeze();
  }


  /**
   * Handle receive MQTT publish commnand.
   *
   * @param   client  The client
   */
  void receivePublish(Client *&client) {
    mqtt.publishDevice();
    mqtt.publishInputs();

    client->println("HTTP/1.1 200 OK");
    client->println("Content-Type: application/json");
    client->println("Connection: close");
    client->println();
    client->println("{\"status\":\"success\",\"message\":\"Informations published\"}");
  }

public:
  OptaLinkerWeb(OptaLinkerVersion &_version, OptaLinkerState &_state, OptaLinkerMonitor &_monitor, OptaLinkerBoard &_board, OptaLinkerConfig &_config, OptaLinkerIo &_io, OptaLinkerNetwork &_network, OptaLinkerClock &_clock, OptaLinkerMqtt &_mqtt) : version(_version), state(_state), monitor(_monitor), board(_board), config(_config), io(_io), network(_network), clock(_clock), mqtt(_mqtt) {}

  uint8_t setup() {
    monitor.setAction(LabelWebSetup);

    _ethernetServer = EthernetServer(80);
    _wifiServer = WiFiServer(80);
    if (network.isEthernet()) {
      monitor.setInfo(LabelWebEthernet);
      _ethernetServer.begin();
    } else {
      monitor.setInfo(LabelWebWifi);
      _wifiServer.begin();
    }

    return 1;
  }

  uint8_t loop() {
    if (network.isConnected() && state.isOdd()) {  // state.isOdd() leave place for other things
      // generic client
      Client *webClient = nullptr;
      if (network.isEthernet()) {
        EthernetClient webEthernetClient = _ethernetServer.accept();
        webClient = &webEthernetClient;
        if (webClient) {
          handleClient(webClient);
        }
      } else {
        WiFiClient webWifiClient = _wifiServer.accept();
        webClient = &webWifiClient;
        if (webClient) {
          handleClient(webClient);
        }
      }
    }

    return 1;
  }


}; // class OptaLinkerWeb

} // namespace optalinker

#endif // #ifndef OPTALINKER_WEB_H