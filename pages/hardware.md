@page hardware_page Hardware

The motion system is driven by multiple stepper motors which were originally
integrated into the Ender printer platform. These motors provide precise
position control and are good for repeatable gantry motion. A vacuum motor
and suction cup assembly are our main object manipulation mechanism, which
allow plastic cups to be picked up and transported without requiring a
complex mechanical grip system. Position feedback is provided through limit
switches mounted on the X and Y axes. These limit switches establish
repeatable machine reference positions during startup homing operations. The
electronics are centered around an STM32F411RET6 microcontroller connected
to a custom four-layer PCB. The board incorporates four TMC5160 stepper
motor drivers, power regulation circuitry, programming and debugging
interfaces, and connectors for motors, sensors, and external peripherals.
Power is supplied from the AC-to-DC switcher that came with the Ender at 24
volts and is regulated down to both 5-volt and 3.3-volt rails for logic and
peripheral devices.

@image html Wiring_Diagram.png "Full system wiring diagram" width=900px

@subpage pcb_page

@subpage sensors_page
