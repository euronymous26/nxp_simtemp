#######################################################################
#
# Makefile to create NXP temperature sensor module
# Author: Enrique Mireles Rodriguez 
#         <mireles.rodriguez.enrique@gmail.com>
# Version: 1.0
#
# Description: 
# Makefile to automate kbuild for 'nxp_tempsimp' kernel module, designed 
# to be compatible with the standard linux build system (kbuild).
#
# Usage:
# - make 		: Builds the 'nxp_simtemp.ko' kernel module.
# - make clean  : Removes all generated build files.
#
#######################################################################

# =====================================================================
# CONFIGURATION VARIABLES
# =====================================================================

# nxp_simtemp.o defect object module compilation.
obj-m := nxp_simtemp.o
# Execute a recursive "make" inside kernel headers.
all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
# Clean generated files.
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

# =====================================================================
# EXTRA RULES AND VARIABLES (Optional).
# =====================================================================
#
