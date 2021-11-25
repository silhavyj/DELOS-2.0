#!/bin/bash

for file in programs/*.cpp; do
    name=$(basename -- "$file")
    name="${name%.*}"
    rm $name.cpp || true
done