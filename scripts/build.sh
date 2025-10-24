#
# build.sh file to compile NXP temperature sensor module (kernel module).
# Author: Enrique Mireles Rodriguez.
#         <mireles.rodriguez.enrique@gmail.com>
# Version: 1.0.
#

#!/bin/sh

# =====================================================================
# CONFIGURATION 
# =====================================================================
# Kernel source directory.
KERNEL_SOURCE="/lib/modules/$(uname -r)/build"

# Project "simtemp" directory.
PROJECT_DIR="$(pwd)"
MODULE_DIR="$PROJECT_DIR/kernel"

# Kernel module name.
MODULE_NAME="nxp_simtemp"

# =====================================================================
# SCRIPT LOGIC
# =====================================================================
# Check if project directory exists.
if [ ! -d "$MODULE_DIR" ]; then
	echo "Project directory $MODULE_DIR does not exists."
fi

# Check if the kernel source directory exists.
if [ ! -d "$KERNEL_SOURCE" ]; then
	echo "Error: kernel source directory not found $KERNEL_SOURCE"
	echo "Ensure that kernel headers has been installed or provide correct path"
	exit 1
fi

# Print an echo to indicate kernel module compiling process ongoing.
echo "Compiling kernel module: ${MODULE_DIR}.ko"

# Compile kernel module using kbuild, where:
#             -C: Specifies kernel build system directory.
#  M=$MODULE_DIR: Tells kbuild where module source is.
sudo make -C "$KERNEL_SOURCE" M="$MODULE_DIR" modules

# Check if kernel module was compiled successfully..
if [ $? -eq 0 ]; then
	echo "Module: ${MODULE_NAME}.ko compiled successfully!"
	echo "You can now load it using 'sudo insmod ${MODULE_NAME}.ko'"
else
	echo "Compilation failed!"
fi
