#!/bin/bash

LD_LIBRARY_PATH="$LD_LIBRARY_PATH:../libtorch" build/tgnews "$@"
