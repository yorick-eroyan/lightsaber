# SPDX-FileCopyrightText: 2022 Tony Merlock for EROYAN.tech
#
# SPDX-License-Identifier: MIT
#
"""
et_statemanager
This module exposes a class to manage the state of the lightsaber blade.
States are:
Off:  As powered down as can-be.
Idle:  Blade off, sound off, button LED on
PowerUp: Blade neopixels extend; extend sound plays
On: Blade steady-state, sound hums, responds to gyro
PowerOff: Blade neopixels retract, retraction sound plays
Clash: Center area of blade flashes, sound plays clash sound
"""
from lib.et_blade import Blade, colors

states = ["Off", "Idle", "PowerUp", "On", "PowerOff", "Clash"]


class StateManager:
    def __init__(self, blade: Blade):
        self.cur_state = "Off"  # Always start in Off state
        self.prev_state = "Off"
        self.blade = blade

    def set_state(self, new_state):
        # verify that new state exists and is valid for the cur_state
        if self.is_valid(new_state):
            # Perform whatever action is needed
            print(new_state)
            self.prev_state = self.cur_state
            self.cur_state = new_state
            if new_state == "Off":
                self.blade.clear_indicator_led()
                self.blade.turn_off()
            if new_state == "Idle":
                self.blade.set_indicator_led()
            if new_state == "PowerUp":
                # Start async sound playing
                self.blade.power_up()
                self.set_state("On")
            if new_state == "On":
                # play Hum sound
                if self.prev_state == "Clash":
                    self.blade.restore()  #clear the clash light color66
                pass  # No pixel blade changes
            if new_state == "PowerOff":
                # play poweroff sound
                self.blade.power_down()
                self.set_state("Idle")
            if new_state == "Clash":
                # play clash sound, and I don't mean London Calling
                self.blade.clash()
                self.new_state("On")

        # Update self.cur_state and self.prev_state

    def is_valid(self, new_state):
        if new_state not in states:
            print("New state {} not valid".format(new_state))
            return False
        if self.cur_state == "Off" and new_state != "Idle":
            print("Invalid move from {} to {}".format(self.cur_state, new_state))
            return False  # can't move to anywhere but Off
        if self.cur_state == "Idle" and not (new_state == "Off" or new_state == "PowerUp"):
            print("Invalid move from {} to {}".format(self.cur_state, new_state))
            return False  # can't move to anywhere but Off or PowerUp
        if self.cur_state == "PowerUp" and new_state != "On":
            print("Invalid move from {} to {}".format(self.cur_state, new_state))
            return False  #
        if self.cur_state == "On" and not (new_state == "PowerOff" or new_state == "Clash"):
            print("Invalid move from {} to {}".format(self.cur_state, new_state))
            return False
        if self.cur_state == "PowerOff" and not(new_state == "Off" or new_state == "Idle"):
            print("Invalid move from {} to {}".format(self.cur_state, new_state))
            return False
        if self.cur_state == "Clash" and new_state != "On":
            print("Invalid move from {} to {}".format(self.cur_state, new_state))
            return False
        return True
