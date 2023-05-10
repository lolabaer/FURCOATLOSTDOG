# MCU Temp Usermod
This usermod adds the temperature readout to the Info tab and also publishes that over the topic `mcutemp` topic.

ATM only ESP32 is supported. 
A shown temp of 53,33°C might indicate that the internal temp is not supported.

ESP8266 do not have a internal temp sensor

ESP32C3, ESP32S2 and ESP32S3 seem to have a internal temp sensor, but i dont know yet how to get their values.

Buildflag: `-D USERMOD_MCUTEMP`
