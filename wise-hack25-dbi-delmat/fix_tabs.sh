#!/bin/bash
# Convertir 4 espaces en tab
sed -i 's/^    /\t/g' Makefile
sed -i 's/^\t\t/\t\t\t/g' Makefile
echo "Tabs fixed"
