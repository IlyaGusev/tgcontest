#pragma once

#include "../db_document.h"

#include <functional>

struct TWeightedDoc {
    TDbDocument Doc;
    double Weight = 0.0;

    TWeightedDoc(const TDbDocument& doc, double weight)
        : Doc(doc)
        , Weight(weight)
    {}
};
