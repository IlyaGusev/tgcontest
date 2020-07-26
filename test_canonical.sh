#!/bin/bash

testAnnotate() {
    ./build/tgnews json test/data/canonical_input.json > canonical_annotation.json
    diff test/data/canonical_annotation.json canonical_annotation.json
    ret=$?
    assertEquals $ret 0
}

testTop() {
    ./build/tgnews top test/data/canonical_input.json > canonical_top.json
    diff test/data/canonical_top.json canonical_top.json
    ret=$?
    assertEquals $ret 0
}

. shunit2-2.1.6/src/shunit2
