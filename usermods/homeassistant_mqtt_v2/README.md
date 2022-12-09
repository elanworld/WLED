# MQTT controllable entities
This usermod allows discovery homeassistant entities.


# define this module
add `-D USERMOD_HMEASSISTANT_DISCOVERY' to plantformio.ini

Example `plantformio.ini`:

```
[env:esp32dev]
board = esp32dev
platform = ${esp32.platform}
platform_packages = ${esp32.platform_packages}
build_unflags = ${common.build_unflags}
build_flags = ${common.build_flags_esp32} -D WLED_RELEASE_NAME=ESP32 -D USERMOD_HMEASSISTANT_DISCOVERY
lib_deps = ${esp32.lib_deps}
monitor_filters = esp32_exception_decoder
board_build.partitions = ${esp32.default_partitions}
```


### Home Assistant auto-discovery
Auto-discovery information is automatically published and you shoudn't have to do anything to register in Home Assistant.
 
