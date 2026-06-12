@page challenges_page Challenges

One of the biggest challenges that our group faced during this project was
dealing with our power distribution from 24 volts to 5 and 3.3 volts, as well
as the current draw for our stepper motors. During our testing stages, our
system was experiencing a lot of current surges right at the startup of the
vacuum motor and whenever we tried to run multiple steppers at once. The
original PCB design included an input fuse intended to protect the board from
overcurrent conditions. However, after some more research into the fuse
component we selected, we realized that it was tripping at a much lower
current than what we had originally designed for. This caused the fuse to
introduce a current limit to our system, which produced voltage drops and
unreliable system behavior during our high-load startup conditions. After we
verified that our PCB traces would hold a higher current load, we decided to
replace the fuse with a direct connection to reduce the voltage drop and
improve current delivery to our system. While this modification removed one
overcurrent protection on our PCB, it significantly improved the reliability
of our robot during operation and eliminated all of the startup issues
associated with the current surges.

Another challenge our team encountered was unreliable homing on the Y-axis.
During initial testing, the Y-axis would occasionally stop almost immediately
after starting its homing sequence, well before reaching the physical limit
switch. After investigation, we found that the REFL switch input was
experiencing brief noise glitches that the TMC5160 was registering as a valid
stop signal, causing the driver to halt prematurely and report a false "home"
position. To fix this, we implemented a debounce check in our homing routine
that requires the stop signal to remain active for several consecutive reads
before it is accepted as a real switch press. If the signal drops out before
that threshold is reached, the homing routine treats it as a glitch and
recommands the move to continue. We also added diagnostic snapshots that
record the state of the REFL switch immediately before homing begins, which
helped us confirm whether the issue was electrical noise versus a switch that
was already triggered at power-on. This fix made our homing sequence
dramatically more reliable.

@subpage lessons_page
