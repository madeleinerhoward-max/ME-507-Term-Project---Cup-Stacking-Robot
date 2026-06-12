@page homing_page Homing and Machine Referencing

In order to establish a repeatable coordinate system for our robot, our
firmware uses a dedicated homing routine for the X and Y axes. During
startup, each axis moves toward its corresponding limit switch until
activation is detected. Once the switch is triggered, the driver records
that position as zero, backs away from the switch by a fixed distance, and
re-establishes machine zero at the backed-off position. We added this
back-off step to prevent electrical noise and switch chatter from causing
false end-stop detections during running. The software requires multiple
consecutive valid switch readings before accepting the signal as a true
limit switch activation. This allowed us to achieve improved homing
reliability and repeatability during testing.

See @ref tmc5160_home() for the implementation.
