# ConfigurablesRooms
HAL STM32 CubeMX, RTOS

Configurables rooms project for remote control via ethernet and wifi networks.

Used modules:
enc28j60 - ethernet module
esp8266 - wireless module
this modules connected via usart rs232 interface.

Implemented in stm32 part:
- on/off two output mosfet power port (12v) via http post/get requests.
- configuration web page (use dns / use static ip, network mask ip, ip address, dns ip address  change.


Implemented in esp8266 part:
- configuration web page for termo control with ds18b20 sensors and change state (on/off) on relay and power semistors keys.
- hysteresis function ...
- more under construction




