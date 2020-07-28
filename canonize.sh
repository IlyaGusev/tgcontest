#!/bin/bash

./build/tgnews --mode json --input test/data/canonical_input.json > test/data/canonical_annotation.json
./build/tgnews --mode top --input test/data/canonical_input.json > test/data/canonical_top.json
