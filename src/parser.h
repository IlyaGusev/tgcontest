#pragma once

#include "document.h"

Document ParseFile(
    const char* fileName,
    bool parseLinks=false,
    bool shrinkText=false,
    size_t maxWords=200);
