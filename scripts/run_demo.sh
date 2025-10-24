#
# run_demo.sh file to load nxp_simtemp.ko kernel module, and execute a 
# rutine to test its performance.
# Author: Enrique Mireles Rodriguez
# Version: 1.0
#

#!/usr/bin/env bash
# run_demo.sh - load nxp_simtemp.ko, show dmesg tail, then remove module
# Usage: ./run_demo.sh

# =====================================================================
# CONFIGURATION 
# =====================================================================
set -u
MODULE="nxp_simtemp"
KO_FILE="${MODULE}.ko"
DMESG_LINES=25

PROJECT_DIR="$(pwd)"
MODULE_DIR="$PROJECT_DIR/kernel"

# =====================================================================
# SCRIPT LOGIC TO INSERT KERNEL MODULE
# =====================================================================
# Ensure to attempt cleanup if interrupted.
cleanup() {
    # Only try to remove if module is currently loaded.
    if lsmod | awk '{print $1}' | grep -qx "${MODULE}"; then
        echo "Removing module ${MODULE}..."
        sudo rmmod "${MODULE}" || echo "Warning: rmmod failed"
    fi
}
trap cleanup EXIT INT TERM

# Check KO file existence.
if [ ! -f "$MODULE_DIR/${KO_FILE}" ]; then
    echo "Error: ${KO_FILE} not found in $(pwd)"
    exit 2
fi

echo "Inserting module: ${KO_FILE}"
if ! sudo insmod "$MODULE_DIR/${KO_FILE}"; then
    echo "insmod failed"
    exit 3
fi

# =====================================================================
# SCRIPT LOGIC TO TEST KERNEL MODULE
# =====================================================================
# Give kernel a short moment to log messages.
sleep 2

# 1) Execute "dmesg | tail" to see logged messages.
echo
echo "----- dmesg | tail -n ${DMESG_LINES} -----"
dmesg | tail -n "${DMESG_LINES}"
echo "----- end dmesg -----"
echo
sleep 2

# 2) Configure path "echo "
echo 1000,70000 | sudo tee /sys/module/nxp_simtemp/parameters/input_param
sleep 2

# last instruction to test kernel module performance.
echo
echo "----- dmesg | tail -n ${DMESG_LINES} -----"
dmesg | tail -n "${DMESG_LINES}"
echo "----- end dmesg -----"
echo

# =====================================================================
# SCRIPT LOGIC TO REMOVE KERNEL MODULE
# =====================================================================
# Remove kernel module with rmmod command, and check it..
echo "Removing module ${MODULE}..."
if ! sudo rmmod "${MODULE}"; then
    echo "rmmod failed (will still exit). Check 'lsmod' or kernel logs."
    exit 4
fi

echo "Done."
# trap cleanup EXIT will run but module already removed; safe to exit 0
exit 0
