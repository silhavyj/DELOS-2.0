#!/bin/bash

for file in programs/*.cpp; do
    name=$(basename -- "$file")
    name="${name%.*}"
    
    cp programs/$name.cpp ./ && \
    make $name.bin.h &&         \
    mv $name.bin.h programs/ && \
    rm *.bin *.o $name.cpp   && \
    sed -i "s/crt0_bin/$name\_bin/" programs/$name.bin.h
done