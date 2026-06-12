@page ui_page User Interface and Manual Control

Our robot includes a dedicated user-control mode that allows a user to
manually position the gantry and control the vacuum pickup system through a
serial terminal interface. Commands are transmitted from a laptop through
PuTTY over USART communication and interpreted by the STM32 firmware. The
X-axis is controlled using the A and D keys, the Y-axis uses the W and S
keys, and the Z-axis uses the K and L keys. The vacuum system can be toggled
on and off using the space bar. Rather than directly commanding motor
movements from keyboard inputs, the software maintains internal position
variables for each axis and updates these commanded positions in fixed
increments. Each keypress modifies the desired position by 5 mm, after which
the updated target position is sent to the appropriate TMC5160 driver. Since
the TMC5160 performs its own motion planning and trajectory generation, the
STM32 is free to continue processing user input while the motion is being
executed. In order to prevent accidental overtravel, software travel limits
were implemented on all three axes. Any commanded motion beyond the allowable
workspace is automatically clamped to the predefined travel boundaries. This
provides an additional layer of protection beyond the physical limit
switches and reduces the risk of mechanical collisions during testing.

The user interface also utilizes an interrupt-driven UART communication
system with a circular ring buffer for command storage. Incoming keystrokes
are captured by the UART interrupt service routine and stored in memory until
processed by the user-mode task. This approach prevents command loss during
rapid user input and separates communication timing from motor-control
execution, improving overall system responsiveness and reliability.

See @ref user_mode "User mode (teleop)" module documentation for the full API.
