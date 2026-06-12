@page state_arch_page State-Based Software Architecture

Our firmware was organized using a state-based architecture to ensure
reliable startup and operation. Rather than immediately entering normal
operation after power-up, the controller progresses through a sequence of
initializations that verify hardware functionality before enabling user
interaction. Our startup sequence begins with driver communication
verification, followed by motor-driver initialization and homing of the X
and Y axes. Once the machine reference frame has been established, the
controller transitions to a menu state where the operator can select either
manual control mode or the automatic cup-stacking routine. The software then
remains within the selected operating mode until another state transition is
requested.

This state-based structure simplified debugging and system integration
because each stage could be independently verified before proceeding to the
next. It also improved reliability by ensuring that the robot could not enter
an operational mode without first establishing communication with all
motion-control hardware and completing the homing sequence.

See @ref gantry_test_run() for the full boot and dispatch sequence.
