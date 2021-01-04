Information about the files

- BOM.txt contains the list of the materials needed to build the Bogus Finger and some link to commercial sites where they can be found.
- The images in /drawings/renders/ will guide you in building the device. If any doubts, open /drawings/bogus_finger.stl to disassemble the 3D model. 
- fritzing project is just used as connection schema for electronics


How to build the Bogus Finger device

- screw the metal frame A (250x50x50) to the base (140x140) using 3 metal braces: 2 to the sides, one on the rear
- fix the metal frame B (12x8x140) to the base right in front of the metal vetical frame A
- screw firmly the linear stroke to the vertical metal frame A: the base of the metal stroke should contact the frame B
- fix the Arduino board and the motor contoller to base or the frame A 
- 3D print the control box and attach: the 3 buttons, the switch and the LED. Fix the control box to front-right corner and wire the buttons to the board as shown in the fritzing project.
- fix the display on the frame A and connect the 4 wires (VCC, GND, SDA, SCL)
- connect the load cell to the linear stroke through a metal brace and two M4 rubber stroke absorber
- create the control circuit (see the fritzing schema in attachment)
- attach the control circuit to the Arduino board and upload the given software
- connect the load cell and the motor driver to the control circuit


