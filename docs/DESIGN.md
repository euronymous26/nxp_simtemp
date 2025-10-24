
# NXP Systems Software Engineer Candidate Challenge (DESIGN)
#
# DESIGN.md file to describe software design for kernel module made.
# Author: Enrique Mireles Rodriguez
# Version: 1.0
#

======================================================================
**Goal:** System that simulates a temperature sensor as linux kernel 
on Raspberry PI 1 and exposes it to user space with a user app to 
configure/read it.
======================================================================

          +--------------------+
          | **User Space**     |
          |  Python/C Program  |
          |  - Reads from      |
          |    /dev/temp_sensor|
          +--------------------+
                    ↑
                    │ (configure/read)
                    ↓
   +--------------------------------+
   | **Kernel Space**               |
   |  Kernel Module                 |
   |  - Creates /dev/temp_sensor    |
   |  - Generates random temp       |
   |    every adjustable period     |
   |    (starting at 100ms)         |
   |  - Up/Down limits were proposed|
   |    fixed from -40.000°C up     |
   |    to 150.000°C)               |
   |  - Threshold, and temperature  | 
   |    can be modified from        |
   |    /sys/.../sampling_ms, and   |
   |    /sys/.../threshold_mC       |
   |  - Control via sysfs and/or    |
   |    `ioctl` via char device,    |
   |    and events via poll.        |
   +--------------------------------+
