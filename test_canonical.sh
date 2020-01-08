#!/bin/bash

testCanonical() {
    ./build/tgnews top test/data/canonical_input.json --from_json > canonical_output.json
    diff test/data/canonical_output.json canonical_output.json
    ret=$?
    assertEquals $ret 0
}

. shunit2-2.1.6/src/shunit2
