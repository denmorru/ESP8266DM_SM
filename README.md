# ESP8266DM_SM

Firmware for Wemos D1 mini (ESP8266) board <br>
controlling single stepper motor module (A4988) <br>
without extra stepper libraries <br>
with minimum pins used.
<br>
Used pins to connect driver module with D1 mini: 
<br>
DIR - logic direction 
<br>
and 
<br>
STP - signal for step 
<br>
and 
<br>
GND - ground
<br>
Minimum pins: "G" and "S". "D" is not obligatory in case you want move only one direction (default).
<br>
Tested 1.5A stepper motor (17HD40005-22B model). Motor powered by 3A.max 10V source. Chip (A4988) powered by 1A.max 5V source.
