@page sensors_page Sensors

The robot uses limit switches on the X and Y axes as its primary positional
sensors. These switches provide reliable reference locations that allow the
machine to establish a known coordinate system each time it powers on.
During initialization, the firmware commands each axis to move toward its
respective limit switch until activation is detected. Once triggered, the
controller backs away from the limit switch and records that location as a
machine reference point. The design uses simple, robust sensors rather than
relying solely on open-loop positioning. This approach improves repeatability
and prevents accumulated position error from causing failed stacking
operations.
