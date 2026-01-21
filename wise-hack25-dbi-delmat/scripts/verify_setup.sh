#!/bin/bash
# scripts/verify_setup.sh
echo "Checking DynamoRIO installation..."
drrun -version || exit 1
echo "Checking libraries..."
ldd libwise_dbi.so
echo "Setup OK!"