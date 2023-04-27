# BRPTGuide
How-to-guide, user manual, and miscellaneous files to construct and operate a BRPT (Blue Robotics Pressure and Temperature) instruments for measuring water temperature (C) and pressure (mbar). The BRPT samples at a rate of ~16 Hz and may be deployed (in its current design) for a period of 4-10 days (depending on battery capacity and environmental conditions) in up to 10 m of water depth. Original instrument designed, fabricated, and tested by Spencer Marquardt, M.S.

PLEASE READ THE DISCLAIMERS AT THE END OF THIS README.

PURPOSE: This repository contains information and instructions on how to construct and operate a novel pressure/temperature instrument to collect environmental data. BRPTs have primarily been utilized and tested in nearshore/wave-current climates to observe swash zone, wave, and flooding dynamics. The BRPT may also be appropriate for other applications and in various environments.

FILES: This repository contains the following directories.
  Arduino        — This directory contains the Arduino .ino sketch to control the Teensy microcontroller and BlueRobotics sensors. The associated libraries                    (for the pressure and temperature sensor) are also included. Source code in these libraries has been modified to work with the BRPT .ino                    sketch.
  AssemblyGuide  - This directory contains the overview document for how to construct the BRPT and its various components. Note, this assembly guide                          provides instructions for creating BRPTs using the 1ST GENERATION BlueRobotics acrylic Series 3 housing and end caps, which has been                        DISCONTINUED at the time of upload. Document needs to be updated to reflect newer designs using 2nd Generation BlueRobotics housings and                    end caps and/or custom housing designs using PVC or other material.
  CAD_3DPrinting - This directory contains CAD files to generate a 3D SolidWorks model of the BRPT with the 1ST GENERATION acrylic housing and end caps.                      Files are also included for the 3D-printed components required to assemble the battery housing. CAD drawings are provided to show                          modifications required for the BlueRobotics end caps (3 hole end cap plate is CUSTOM machined part) and the BlueRobotics 1st Gen housing                    (to add threaded inserts to the end walls for more secure fastening of end caps to the housing body, as well as boring out the inner                        diameter of the 1st Gen housing to ensure a uniform contact surface for the end cap o-rings).
  OpsManual      - This directory contains the operations manual ("Quick Start Guide") providing instructions for how to prepare the circuit, battery                          housing, external housing, and end caps for deployment of the 1ST GENERATION acrylic housing BRPT. The document also briefly explains                      the data retrieval and conversion process at the conclusion of data collection.
  PartsList      - This directory contains a spreadheet of materials required to construct the BRPTs. It has been updated to reflect materials required for                    the NEW 2ND GENERATION locking-flange acrylic housings and end caps provided by BlueRobotics at the time of publication (April 2023). T                    This list should be reviewed thoroughly by the user prior to being utilized to purchase supplies to construct a BRPT.
  PCB            - This directory contains the files required to print a custom printed circuit board.
  SLERJ_Logger   - This directory contains information about the multi-channel serial datalogger used in the BRPT, including a general setup and data                          conversion guide. A brief overview of the specific steps required for setup and data conversion with the BRPTs is also included in the                      Quick Start Guide in the OpsManual directory.


DISCLAIMERS:
1. THIS REPOSITORY IS STILL BEING EDITED, UNLESS NOTED OTHERWISE.
2. All files (guides, documents, codes, etc.) in this repository may contain unintentional errors and should be considered as DRAFT documents. Please thoroughly review them and follow instructions or suggestions at your own discretion.
3. Instructions and designs contained herein are works in progress, and as such, span a range of part generations, design iterations, etc. and are not finalized unless noted otherwise.
4. This repository is meant to serve as an educational resouce for the construction of BRPT (pressure/temperature) instruments and is not intended to provide a foolproof, commercial-quality instrument. 

Thank you in advance for your interest and understanding!
Savannah DeVoe
Coastal Processes Lab
School of Marine Science & Ocean Engineering
University of New Hampshire
