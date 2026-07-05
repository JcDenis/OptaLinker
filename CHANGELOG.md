Important note: OptaLinker requires board manager Arduino Mbed OS Board 4.5.0, NOT 4.6.0 that break OTA.

TODO
====
* Add io values stack (when offline)
* Add full MQTT OTA update (missing binary)
* Add full HTTP PTA update (missing version)

OptaLinker 1.3.0 - 2026.07.05
=============================
* Change MQTT library from ArduinoMQTTClient to MQTT (at https://github.com/256dpi/arduino-mqtt )
* Update OT exemple

OptaLinker 1.2.3 - 2026.06.11
=============================
* Add OTA helpers
* Update OTA exemples to 1.2.3

OptaLinker 1.2.2 - 2026.06.10
=============================
* Require board manager Arduino Mbed OS Board 4.5.0 (not 4.6.0 that break OTA)
* Change unit of timers partial high and high from lilliseconds to seconds
* Fix io ID to UID in MQTT (take care of expansions)

OptaLinker 1.2.0 - 2026.06.01
=============================
* Add MQTT bidirectional watchdog
* Change OTA firmware topic (BC)
* Update OTA exemples to 1.2.0

OptaLinker 1.1.3 - 2026.05.27
=============================
* Add MQTT topic to update one io MQTT topic
* Change MQTT io update to only relevant values
* Change "reset" MQTT topic to use value for io (BC)
* Update OTA exemples to 1.1.3

OptaLinker 1.1.0 - 2026.02.25
=============================
* Add OTA update support

OptaLinker 1.0.0 - 2026.02.17
=============================
* First public release