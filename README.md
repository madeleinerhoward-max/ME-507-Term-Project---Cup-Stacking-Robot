# Cup Stacking Gantry Robot — ME 507 Term Project

**Team:** Tyler Moyers, Dylan Walty, Maddie Howard
Cal Poly SLO — Department of Mechanical Engineering

## Overview

A gantry-style robot that grabs and stacks cups using a vacuum/suction-cup
end effector. Built on a modified Ender-series 3D printer frame with a
custom STM32F411-based 4-layer PCB driving four TMC5160 stepper drivers.

📖 **[Full Project Documentation](https://yourusername.github.io/your-repo/)**

The documentation site includes:
- Full source code documentation 
- Mechanical design (CAD renders)
- Custom PCB design and schematics
- Software architecture (driver library, motion control, homing, state
  machine, UI)
- Challenges, lessons learned, and performance results
- Demo videos (autonomous and manual teleop modes)

## Repository Structure

```
.
├── src/        # Firmware source (STM32CubeIDE-compatible)
├── pages/      # Doxygen report pages (Markdown)
├── images/     # CAD renders, schematics, photos for documentation
├── docs/       # Generated Doxygen output (HTML)
├── Doxyfile    # Doxygen configuration
└── .gitignore
```

## Building the Documentation

```bash
doxygen Doxyfile
```

Output is written to `docs/html/index.html`.
