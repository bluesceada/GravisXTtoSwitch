# GravisXTtoSwitch

Work in progress (w.i.p.) to adapt a Gameport Gamepad with a custom two wire vendor protocol to a Nintendo Switch Bluetooth Gamepad. This project builds on all what has previously been done to adapt a Gamecube Controller to Nintendo Switch Bluetooth.

This is the Gamepad in question:

<img align="left" src="https://raw.githubusercontent.com/bluesceada/GravisXTtoSwitch/References/Gravis_XT_Gamepad.jpg" alt="kicad schematic"/>

Original project there: https://github.com/NathanReeves/BlueCubeMod (the fork was done from a project that was adapted to a more recent ESP-IDF version). Thanks to all the effort from various people that has lead to that previous project.

## Information collection

 - Linux Kernel Driver for Gravis GrIP protocol devices - most probably including the Gamepad at hand: https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git/tree/drivers/input/joystick/grip.c
 - References/Gravis_XT_Gamepad.jpg: The Gamepad I have here in front of me. It is NOT the USB version (which had USB Data lines on two of the Gameport pins, and a simple passive adapter would be enough.)
 - References/Gravis_XT_idle_8MHz-sampled.sr: PulseView file, Logic Analyzer Measurements of the two relevant data pins of the Gamepad
 - Forum post discussing the Gamepad and Kernel Driver, but with no proper conclusion: https://www.avrfreaks.net/forum/interpreting-serial-data-gravis

## Plans & Progress

[x] Successfully compile existing project (BlueCubeModv2) - works with ESP-IDF stable, v4.4 https://docs.espressif.com/projects/esp-idf/en/v4.4/esp32/get-started/index.html

[x] Get information on Gravis Xterminator Gamepad protocol

[x] Decode Gravis Xterminator Protocol successfully on ESP32

[ ] Integrate into BlueCubeModv2

[ ] Have fun on Nintendo Switch with a decades old Gamepad

[ ] Properly integrate ESP32 board / Battery into the Xterminator Gamepad - 5V is needed for Gamepad and input protection for the 3.3V ESP32 pins are needed (not really 5V tolerant). Buck/Boost Battery circuit is needed.

[ ] If needed: design custom PCB for better integration into the Gamepad

[ ] Future (new project): Look into a Gravis GrIP joystick to USB Mod for PC? I still have a Firebird II around...

