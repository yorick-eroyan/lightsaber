# SPDX-FileCopyrightText: 2022 Tony Merlock for EROYAN.tech
#
# SPDX-License-Identifier: MIT

import alarm
import time
import board
import digitalio
import neopixel
from lib.et_blade import Blade, colors
from lib.et_statemanager import StateManager

btn_ctrl = digitalio.DigitalInOut(board.GP13)  # Button input
btn_ctrl.direction = digitalio.Direction.INPUT
btn_ctrl.pull = digitalio.Pull.UP

pin_alarm = None
btn_led = digitalio.DigitalInOut(board.GP11)  # LED on  button
btn_led.direction = digitalio.Direction.OUTPUT

blade = Blade(8, board.GP12, btn_led, 0.3, 0.2)
blade.set_color("RED")
blade.set_clash_color("WHITE")

state = StateManager(blade)

can_toggle = True
while True:
    # Turn all red
    print(state.cur_state)
    if can_toggle:
        can_toggle = False
        if not btn_ctrl.value:
            print("Button TRUE")
            if state.cur_state == "Off":
                state.set_state("Idle")
            else:
                state.set_state("PowerUp")
        else:
            print("Button FALSE")
            if state.cur_state == "On":
                state.set_state("PowerOff")
                state.set_state("Off")
                print("Going to deep sleep")
                btn_ctrl = None
                pin_alarm = alarm.pin.PinAlarm(board.GP13, value=False, pull=True)
                # Exit the program, and then deep sleep until the alarm wakes us.
                alarm.exit_and_deep_sleep_until_alarms(pin_alarm)

    time.sleep(1)
    can_toggle = True
