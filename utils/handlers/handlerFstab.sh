#!/bin/sh

# =============================================================================
# Variables
# =============================================================================

LOCAL_OPERATION=$1

# =============================================================================
# Functions
# =============================================================================

# None

# =============================================================================
# Main
# =============================================================================


if [ "$LOCAL_OPERATION" = "save" ]; then

	cat /etc/fstab > $HFST_FSTAB_ORIGINAL
	cat $HFST_FSTAB_ORIGINAL

elif [ "$LOCAL_OPERATION" = "restore" ]; then

	cat $HFST_FSTAB_ORIGINAL > /etc/fstab
	cat /etc/fstab

fi

# End of file
