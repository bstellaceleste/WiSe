#!/bin/bash
export PIN_ROOT=$HOME/Bureau/wise_hack/pin-external-4.0-99633-g5ca9893f2-gcc-linux
export PATH=$PATH:$PIN_ROOT

echo "[1/3] Compiling target test app..."
g++ target_test.cpp -o target_app

echo "[2/3] Compiling Instrumentation Engine..."
make -C $PIN_ROOT/source/tools/MyTool obj-intel64/mon_moteur.so TARGET=intel64

echo "[3/3] Running Moteur on target_app..."
pin -t $PIN_ROOT/source/tools/MyTool/obj-intel64/mon_moteur.so -- ./target_app
