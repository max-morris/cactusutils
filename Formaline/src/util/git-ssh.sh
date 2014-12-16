#! /bin/bash
if [ -z "$CACTUS_CENTRAL_SSH_ID" ]; then
    ssh "$@"
else
    ssh -i "$CACTUS_CENTRAL_SSH_ID" "$@"
fi
