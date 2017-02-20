# ESP8266DM_SM

Firmware for Wemos D1 mini (ESP8266) board 
controlling single stepper motor module (A4988) 
without extra stepper libraries 
with minimum pins used.

Used pins to connect driver module with D1 mini: 

DIR - logic direction 

and 

STP - signal for step 

and 

GND - ground

Minimum pins: "G" and "S". "D" is not obligatory in case you want move only one direction (default).
<br>
Tested 1.5A stepper motor (17HD40005-22B model). Motor powered by 3A.max 10V source. Chip (A4988) powered by 1A.max 5V source.
