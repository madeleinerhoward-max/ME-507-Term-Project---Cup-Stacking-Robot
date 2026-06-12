@page software_page Software

Our software was written in C using STM32CubeIDE and the STM32 HAL
libraries. The software was organized into hardware abstraction layers and
reusable driver modules. One of the biggest software components is the
TMC5160 driver library, which encapsulates all SPI communication, register
configuration, homing behavior, and motion commands for the stepper motor
drivers.

The startup sequence is implemented as a structured state-based
initialization routine. Upon power up, the software first verifies
communication with all connected TMC5160 motor drivers by reading diagnostic
registers over SPI. Successful communication confirms that each motion axis
is available before motion commands are issued. Following the communication
verification, the robot executes a homing sequence for the X and Y axes
using the installed limit switches. Once homing is complete, the machine
establishes its coordinate system and records the current Z-axis position as
a reference location.

@subpage tmc_driver_page

@subpage motion_strategy_page

@subpage homing_page

@subpage vacuum_page

@subpage state_arch_page

@subpage ui_page

@subpage math_modeling_page
