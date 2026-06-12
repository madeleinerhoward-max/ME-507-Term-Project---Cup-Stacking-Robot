@page motion_strategy_page Motion Control Strategy

Our robot uses the integrated motion-planning capabilities of the TMC5160
stepper motor drivers. Instead of generating individual step pulses from the
STM32 microcontroller, the firmware sends target positions and motion
parameters to the drivers over SPI. The TMC5160 then handles acceleration,
deceleration, and trajectory generation internally. This method reduced
processor workload and allowed smoother motion while simplifying firmware
development. Position commands are generated in physical units and converted
into motor steps using axis-specific conversion factors. The X and Y axes
utilize a belt-driven conversion factor of 80 steps/mm, while the Z axis is
driven with a leadscrew and uses 400 steps/mm.
