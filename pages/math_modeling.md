@page math_modeling_page Motion Calculations / Mathematical Modeling

The motion of the axes is based on converting physical distance into motor
positions. The TMC5160 motor drivers operate using step counts, while the
user and application logic use physical units. We had to use conversion
factors for each axis to translate the desired linear motion into motor
steps. By using the motion-planning hardware built into the TMC5160 drivers,
the robot can execute controlled movements without requiring the STM32 to
generate individual step pulses.
