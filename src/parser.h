#pragma once

#include "document.h"

TDocument ParseFile(
    const char* fileName,
    bool parseLinks=false,
    bool shrinkText=false,
    size_t maxWords=200);
