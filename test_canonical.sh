#!/bin/bash

testAnnotate() {
    ./build/tgnews json test/data/canonical_input.json --from_json > canonical_annotation.json
    diff test/data/canonical_annotation.json canonical_annotation.json
    ret=$?
    assertEquals $ret 0
}

. shunit2-2.1.6/src/shunit2
