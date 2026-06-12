@page tmc_driver_page TMC5160 Driver Architecture

One of our software components developed for this project was a custom
TMC5160 driver library. The library abstracts all low-level SPI
communication and register management behind a set of reusable functions for
initialization, homing, positioning, and status monitoring. Rather than
directly manipulating motor-driver registers throughout the application code,
the firmware interacts with the motor drivers through dedicated functions.
This improved our code readability and maintainability by separating the
hardware-specific implementation details from the higher-level robot
behavior. The driver library also provides communication verification,
timeout protection, fault clearing, and position tracking.

See the @ref tmc5160 "TMC5160 driver" module documentation for the full API.
