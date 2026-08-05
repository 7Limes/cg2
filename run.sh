#!/bin/bash

g2a font.g2 assembled.g2b
./build/cg2 assembled.g2b --scale 2
# g2py assembled.g2b --scale 10 -d
