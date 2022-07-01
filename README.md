# lightsaber
This is a custom Lightsaber Project using AdaFruit Arduino chips.  After all, why spend $400 on a NeoPixel lightsaber by one of the well-known companies when you can spend twice that, plus do it all yourself?
A while back, I picked up a lightsaber hilt from one of the better known companies.  I was intending to just leave it on my desk as decoration, but I kept staring at it, and it kept staring at me, and then magically, parts started to get ordered.
Note that there will be no links to products here, and especially no affiliate links.
All code is (c) 2022 EROYAN.tech, licensed under the [MIT license](https://opensource.org/licenses/MIT).
## Build Components
* Arduino Nano Every - I was going to use an AdaFruit ItsyBitsy with BlueTooth, but didn't want to get into FreeRTOS for my first project.
* AdaFruit Music Maker Featherwing with amplifier - for playing the lightsaber sounds.  
* AdaFruit LSM6DSOX Accellerometer / Gyroscope 6-DOF IMU - for detecting motion
* Two 1m NeoPixel strips - I went with the AdaFruit Mini Skinny NeoPixel 60LED/m (WHITE) strip, and bought 4 meters.  Enough for two sabers!
* An 18650 Protected cell
* Misc components - a 1000uF capacitor, a 470Ohm resistor, an inline fuse holder, a momentary pushbutton switch and wires.
## Microcontroller
I built an initial prototype using an AdaFruit Arduino Nano Every, which  worked great using sample code.  I wanted to use the AdaFruit ItsyBitsy
express but it seemed like it would be a bit too complex for my first projectd, so I decided to stick with the Every.
## Powerblock
The powerblock is fairly simple - routing the 18650 cell to a connector.
![Powerblock](https://github.com/yorick-eroyan/lightsaber/blob/main/diagrams/powerblock.png)

I added an inline fuse holder because I don't want to draw more than 2A from the cell.  Each NeoPixel takes about 33.5mA, so at 60/m, with two 1m strips, I have 120 NeoPixels for 4.02A at full draw.  So I will need to limit brightness to about 40% max to give me some breathing room.  Ideally, I will draw less for longer runtime, but we shall see how the brightness goes.
## MicroController, NeoPixel and Button
The Nano Every is wired up to the NeoPixel strip using Digital pin 8.  The button is connected to pin 7, and the LED ring is connected to pin 6.  The logic is essentially a state machine, with each button push moving through a state.  The states are
* Off - no neopixels, no LED ring
* Idle - No neopixels, but LED ring glows
* Power on - Blade extends.  Automatically moves to On once done
* On - Blade steady state.  Responds to clash and motion here.
* Power off - Blade retracts.  Automatically moves to Off once done.

![Blade Control](https://github.com/yorick-eroyan/lightsaber/blob/main/diagrams/blade.png)

## Soundsystem
The AdaFruit Music Maker Featherwing was appealing to me because it is pin-compatible with the microcontroller, and it has an amplifier.
Since I'm not using a Feather, will have to figure out the wiring.  So new Fritz incoming
## Gyro
Any lightsaber worth its plasma needs to react to movement, so I went with the AdaFruit LSM6DSOX.  It came with SparkFun qwiic connectors, making connecting easy.
It is connected to the SCL and SDA pins.  The default address of 0x6A will be used.


