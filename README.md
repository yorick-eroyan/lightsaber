# lightsaber
This is a custom Lightsaber Project using AdaFruit Arduino chips.  After all, why spend $400 on a NeoPixel lightsaber by one of the well-known companies when you can spend twice that, plus do it all yourself?
A while back, I picked up a lightsaber hilt from one of the better known companies.  I was intending to just leave it on my desk as decoration, but I kept staring at it, and it kept staring at me, and then magically, parts started to get ordered.
Note that there will be no links to products here, and especially no affiliate links.
All code is (c) 2022 EROYAN.tech, licensed under the [MIT license](https://opensource.org/licenses/MIT).
## Build Components
* AdaFruit ItsyBitsy nrf52840 Express - Main controller board.  Chosen because it has Bluetooth(tm) - whether I use it or not, we shall see
* AdaFruit Music Maker Featherwing with amplifier - for playing the lightsaber sounds.  
* AdaFruit LSM6DSOX Accellerometer / Gyroscope 6-DOF IMU - for detecting motion
* Two 1m NeoPixel strips - I went with the AdaFruit Mini Skinny NeoPixel 60LED/m (WHITE) strip, and bought 4 meters.  Enough for two sabers!
* An 18650 Protected cell
* Misc components - a 1000uF capacitor, a 470Ohm resistor, a momentary pushbutton switch and wires.
