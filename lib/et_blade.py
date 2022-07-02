# SPDX-FileCopyrightText: 2022 Tony Merlock for EROYAN.tech
#
# SPDX-License-Identifier: MIT
#
"""
et_blade.py
Contains the Blade class for the lightsaber project.
"""
import time
import board
import digitalio
import neopixel

""" Define colors we can use here """
colors = {
    "RED": (255, 0, 0),
    "YELLOW": (255, 150, 0),
    "GREEN": (0, 255, 0),
    "CYAN": (0, 255, 255),
    "BLUE": (0, 0, 255),
    "PURPLE": (180, 0, 255),
    "BLACK": (0, 0, 0),
    "WHITE": (255, 255, 255)
}


class Blade:
    def __init__(self, num_pixels, pixel_pin, led_pin, brightness, timer):
        """
        Initializes the blade.  Takes in the number of pixels, the control
        pin, the brightness and the time in seconds between pixel changes
        """
        self.color = "BLACK"
        self.clash_color = "WHITE"
        self.num_pixels = num_pixels
        self.pixel_pin = pixel_pin
        self.led_pin = led_pin # This is the LED in the button or other indicator LED.  Must be a digital out pin
        self.brightness = brightness
        self.blade = neopixel.NeoPixel(pixel_pin, num_pixels,
                                       brightness=brightness, auto_write=False)
        self.timer = timer  # seconds between pixel changes

    def set_color(self, new_color):
        """
        Sets the color of the blade
        """
        if new_color in colors:
            self.color = new_color

    def set_clash_color(self, new_color):
        """
        Sets the clash color - typically white, but could be different
        depending on desire
        """
        if new_color in colors:
            self.clash_color = new_color

    def set_brightness(self, brightness):
        """
        Sets the brightness - primarily used to control battery drain.
        Should be about 30% max to prevent over-draining battery
        """
        self.brightness = brightness

    def set_timer(self, timer):
        """
        Sets the timer between pixel changes for when the blade is extended
        and retracted.
        """
        self.timer = timer

    def turn_off(self):
        """
        Turns the blade off completely.  Rapid shutdown, no effects
        """
        self.color = "BLACK"
        self.blade.fill(colors[self.color])
        self.blade.show()

    def set_indicator_led(self):
        """
        Turns on the indicator led - usually the led in the button ring
        :return:
        """
        self.led_pin.value = True

    def clear_indicator_led(self):
        """
        Turns off the indicator LED - usually the LED in the button ring
        :return:
        """
        self.led_pin.value = False

    def power_up(self):
        """
        Goes through power-on sequence.  Note there are no links to sound
        here.  Have to figure out how to thread that.
        """
        for x in range(0, self.num_pixels):
            self.blade[x] = colors[self.color]
            self.blade.show()
            time.sleep(self.timer)

    def power_down(self):
        # Reverse power_up
        saved_color = self.color
        self.color = "BLACK"
        for x in range(self.num_pixels - 1, -1, -1):
            self.blade[x] = colors[self.color]
            self.blade.show()
            time.sleep(self.timer)
        self.color = saved_color

    def clash(self):
        # Clash will flicker the center of the blade
        # in the self.clash_color.
        # Will have to be un-done by restore
        # determine mid-third
        start = int(self.num_pixels / 3)
        end = start * 2
        for x in range(start, end):
            self.blade[x] = colors[self.clash_color]
            self.blade.show()

    def restore(self):
        self.blade.fill(colors[self.color])
        self.blade.show()

    def __repr__(self):
        return "<Color: {}>".format(self.color)
