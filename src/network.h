/*
 * OptaLinker
 *
 * Author: Jean-Christian Paul Denis
 * Source: https://github.com/JcDenis/OptaLinker
 *
 * @see OptaLinker.h
 * @see README.md file
 */

#ifndef OPTALINKER_NETWORK_H
#define OPTALINKER_NETWORK_H

#include <Ethernet.h>
#include <WiFi.h>

#include "OptaLinkerModule.h"

namespace optalinker {

class OptaLinkerState;
class OptaLinkerMonitor;
class OptaLinkerBoard;
class OptaLinkerConfig;

/**
 * OptaLinker Library network module.
 *
 * Manage connection network through wifi or ethernet.
 */
class OptaLinkerNetwork : public OptaLinkerModule {

private:
  OptaLinkerState &state;
  OptaLinkerMonitor &monitor;
  OptaLinkerBoard &board;
  OptaLinkerConfig &config;

  /**
   * The network type
   */
  uint8_t _networkType = NetworkNone;

  /**
   * Connection state.
   */
  uint8_t _isConnected = 0;

  /**
   * Connection timeout.
   */
  uint32_t _timeoutDelay = 10000;

  /**
   * Connection rerty delay.
   */
  uint32_t _pollDelay = 50000;

  /**
   * Last connection rerty time.
   */
  uint32_t _lastRetry = 0;

  /**
   * First Wifi Acces Point test.
   */
  uint8_t _apFirstLoop = 1;

  /**
   * Wifi Access Point status.
   */
  uint8_t _apStatus = WL_IDLE_STATUS;

  /**
   * Last LED change.
   */
  uint32_t _ledLast = 0;

  /**
   * Last LED state.
   */
  uint8_t _ledState = 0;

  /**
   * Netowrk connection using Ethernet.
   */
	void connectEthernet() {
	  monitor.setMessage(LabelNetworkEthernet, MonitorAction);

	  int ret = 0;
	  board.setFreeze();
	  if (config.getNetworkDhcp()) {
	    ret = Ethernet.begin(nullptr, getTimeout(), 4000);
	    if (ret) {
	    	monitor.setMessage(LabelNetworkMode + String("DHCP"), MonitorSuccess);
	    }
	  } else {
	    ret = Ethernet.begin(nullptr, config.getNetworkIp(), config.getNetworkDns(), config.getNetworkGateway(), config.getNetworkSubnet(), getTimeout(), 4000);
	    if (ret) {
	    	monitor.setMessage(LabelNetworkMode + String("Static IP"), MonitorSuccess);
	    }
	  }
	  board.unsetFreeze();

	  if (ret == 0) {
	    _isConnected = 0;
	    monitor.setMessage(LabelNetworkEthernetFail, MonitorFail);
	    if (Ethernet.linkStatus() == LinkOFF) {
	      monitor.setMessage(LabelNetworkEthernetDisconnect, MonitorWarning);
	    }
	  } else {
	    _isConnected = 1;
	    monitor.setMessage(LabelNetworkEthernetSuccess + getLocalIp().toString(), MonitorSuccess);
	  }
	}

	/**
	 * Network connection using Wifi standard.
	 */
	void connectStandard() {
	  monitor.setMessage(LabelNetworkSta, MonitorAction);

	  String netApSsid = config.getNetworkSsid();
	  String netApPass = config.getNetworkPassword();
	  char ssid[32];
	  char pass[32];
	  netApSsid.toCharArray(ssid, sizeof(ssid));
	  netApPass.toCharArray(pass, sizeof(pass));

	  monitor.setMessage(LabelNetworkSsid + netApSsid + " / " + netApPass, MonitorInfo);
	  if (config.getNetworkDhcp()) {
	    monitor.setMessage(LabelNetworkMode + String("DHCP"), MonitorInfo);
	  } else {
	    monitor.setMessage(LabelNetworkMode + String("Static IP"), MonitorInfo);
	    WiFi.config(config.getNetworkIp(), config.getNetworkDns(), config.getNetworkGateway(), config.getNetworkSubnet());
	  }

	  board.setFreeze();
	  WiFi.setTimeout(getTimeout());
	  int ret = WiFi.begin(ssid, pass);
	  board.unsetFreeze();

	  if (ret != WL_CONNECTED) {
	    monitor.setMessage(LabelNetworkStaFail, MonitorFail);
	    _isConnected = 0;
	  } else {
	    monitor.setMessage(LabelNetworkStaSuccess, MonitorSuccess);
	    _isConnected = 1;
	  }
	}

public:
  OptaLinkerNetwork(OptaLinkerState &_state, OptaLinkerMonitor &_monitor, OptaLinkerBoard &_board, OptaLinkerConfig &_config) : state(_state), monitor(_monitor), board(_board), config(_config) {}

  uint8_t setup() {

	  monitor.setMessage(LabelNetworkSetup, MonitorAction);

	  // Check configuration to use Wifi Standard
	  if (board.isWifi() && config.getNetworkWifi() && config.getNetworkSsid() != "" && config.getNetworkPassword() != "") {
	    _networkType = NetworkStandard;

	    if (WiFi.status() == WL_NO_MODULE) {
				monitor.setMessage(LabelNetworkFail, MonitorFail);

				return 0;
	    }

	    monitor.setMessage(LabelNetworkMode + String("Wifi standard network"), MonitorInfo);
	    connectStandard();
	  // Check configuration to use Wifi Access Point
	  } else if (board.isWifi() && config.getNetworkWifi()) {
	    _networkType = NetworkAccessPoint;

	    if (WiFi.status() == WL_NO_MODULE) {
				monitor.setMessage(LabelNetworkFail, MonitorFail);

				return 0;
	    }

	    String netApSsid = "optalinker" + String(config.getDeviceId());
	    String netApPass = "optalinker";
	    char ssid[32];
	    char pass[32];
	    netApSsid.toCharArray(ssid, sizeof(ssid));
	    netApPass.toCharArray(pass, sizeof(pass));

	    monitor.setMessage(LabelNetworkSsid + netApSsid + " / " + netApPass, MonitorInfo);
	    monitor.setMessage(LabelNetworkStaticIp + config.getNetworkIp().toString(), MonitorInfo);

	    WiFi.config(config.getNetworkIp());

	    board.setFreeze();
	    int ret = WiFi.beginAP(ssid, pass);
	    board.unsetFreeze();

	    if (ret != WL_AP_LISTENING) {
				monitor.setMessage(LabelNetworkApFail, MonitorFail);

				return 0;
	    } else {
	    	monitor.setMessage(LabelNetworkMode + String("Wifi Access Point network"), MonitorSuccess);
	      //monitor.setMessage(LabelNetworkApSuccess, MonitorSuccess);
	      _isConnected = 1;
	    }
	  // At least, use Ethernet
	  } else {
	    _networkType = NetworkEthernet;

	    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
				monitor.setMessage(LabelNetworkFail, MonitorWarning);

				return 0;
	    }

	    monitor.setMessage(LabelNetworkMode + String("Ethernet network"), MonitorSuccess);
	    connectEthernet();
	  }

	  if (isConnected() && config.getNetworkDhcp()) {
	    monitor.setMessage(LabelNetworkDhcpIp + getLocalIp().toString(), MonitorInfo);
	  }

	  board.pingTimeout();

    return 1;
  }

  uint8_t loop() {
	  if (isEthernet()) {
	    // Mauvaise condition
	    if (!isConnected() && isPoll(_lastRetry)) {
	      _lastRetry = state.getTime();
	      if (isConnected()) {
	        Ethernet.maintain();
	      } else {
	        connectEthernet();
	      }
	    }
	    if (!isConnected() && Ethernet.linkStatus() == LinkON) {
	      monitor.setMessage(LabelNetworkEthernetConnect, MonitorInfo);
	      _isConnected = 1;
	    }
	    if (isConnected() && Ethernet.linkStatus() != LinkON) {
	      monitor.setMessage(LabelNetworkEthernetDisconnect, MonitorWarning);
	      _isConnected = 0;
	    }
	    if (isConnected() && Ethernet.linkStatus() == LinkON) {
	      _lastRetry = 0;
	      _isConnected = 1;
	    }
	  } else if (isStandard()) {
	    if (!isConnected() && isPoll(_lastRetry)) {
	      _lastRetry = state.getTime();
	      connectStandard();
	    }
	  } else if (isAccessPoint()) {
	    if (_apStatus != WiFi.status()) {
	      _apStatus = WiFi.status();

	      if (_apStatus == WL_AP_CONNECTED) {
	        monitor.setMessage(LabelNetworkApConnect, MonitorInfo);
	      } else if (_apFirstLoop) {  // do not display message on startup
	        _apFirstLoop = 0;
	      } else {
	        monitor.setMessage(LabelNetworkApDisconnect, MonitorWarning);
	      }
	    }
	  }

	  // Check LEDs
	  if (state.getTime() - _ledLast > 750) {
	    _ledLast = state.getTime();
	    _ledState = _ledState ? 0 : 1;

	    board.setRed(isConnected() ? 0 : _ledState);
	    // Blink blue LED on Wifi AP
	    if (isAccessPoint()) {
	      board.setBlue(_ledState);
	    // Fix blue LED on Wifi ST
	    } else if (isStandard()) {
	      board.setBlue(1);
	    }
	  }

    // Check button: If network not connected or as access point and button push less than 1s: switch DHCP mode in config and reboot
    if ((!isConnected() || isAccessPoint()) && board.isPushDuration(0, 1000)) {
      config.setNetworkDhcp(config.getNetworkDhcp() ? 0 : 1);
      config.writeToFile();
      board.reboot();
    }

    // Check button: If network not connected or as access point and button push bettwen 1s and 3s: switch WIFI mode in config and reboot
    if ((!isConnected() || isAccessPoint()) && board.isPushDuration(1100, 3000)) {
      config.setNetworkWifi(config.getNetworkWifi() ? 0 : 1);
      config.writeToFile();
      board.reboot();
    }

    return 1;
  }

  /**
   * Set network connection retry delay.
   *
   * @param 	delay 	Retry delay, 0~120000 ms
   */
  void setPollDelay(uint32_t delay) {
	  if (delay > 0 && delay < 120000) {
	    monitor.setMessage(LabelNetworkPoll, MonitorInfo);
	    _pollDelay = delay;
	  }
  }

  /**
   * Check if it is time to retry connection.
   *
   * @param 	last 	Last attempt time.
   *
   * @return 	1 if it is time to retry, else 0
   */
	uint8_t isPoll(uint32_t last) {

	  return (_pollDelay > 0) && ((last == 0) || ((state.getTime() - last) > _pollDelay)) ? 1 : 0;
	}

	/**
	 * Set connection timetout.
	 *
	 * @param 	timeout 	The conenction timeout, 0~120000 ms
	 */
	void setTimeout(uint32_t timeout) {
	  if (timeout > 0 && timeout < 120000) {
	    monitor.setMessage(LabelNetworkTimeout + String(timeout), MonitorSuccess);
	    _timeoutDelay = timeout;
	  }
	}

	/**
	 * Get connection timeout.
	 *
	 * @return 	The network connection timeout setting
	 */
	uint32_t getTimeout() {

	  return _timeoutDelay;
	}

	/**
	 * Check is device is connected to network.
	 *
	 * @return 	1 if connected, else 0
	 */
	uint8_t isConnected() {

	  return _isConnected;
	}

	/**
	 * Check if netowrk is set as Wifi Access Point.
	 *
	 * return 	A if it is Wifi Access Point, else 0
	 */
	uint8_t isAccessPoint() {

	  return _networkType == NetworkAccessPoint ? 1 : 0;
	}

	/**
	 * Check if netowrk is set as Wifi standard.
	 *
	 * return 	A if it is Wifi standard, else 0
	 */
	uint8_t isStandard() {

	  return _networkType == NetworkStandard ? 1 : 0;
	}

	/**
	 * Check if netowrk is set as Ethernet.
	 *
	 * return 	A if it is Ethernet, else 0
	 */
	uint8_t isEthernet() {

	  return _networkType == NetworkEthernet ? 1 : 0;
	}


	/**
	 * Get local IP V4 address.
	 *
	 * return 	device IPv4 IPAddress insatnce
	 */
	IPAddress getLocalIp() {

	  return isEthernet() ? Ethernet.localIP() : WiFi.localIP();
	}

	/**
	 * Convert IPv4.
	 *
	 * @param 	ip The IPv4 string
	 *
	 * @return The IPv4 IPAddress instance
	 */
	IPAddress stringToIp(String ip) {
	  unsigned int res[4];
	  sscanf(ip.c_str(), "%u.%u.%u.%u", &res[0], &res[1], &res[2], &res[3]);
	  IPAddress ret(res[0], res[1], res[2], res[3]);

	  return ret;
	}

}; // class OptaLinkerNetwork

} // namespace optalinker

#endif // #ifndef OPTALINKER_NETWORK_H