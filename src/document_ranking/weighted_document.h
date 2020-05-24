#pragma once

#include "../db_document.h"

#include <functional>

bool ComputeDocumentNasty(const TDbDocument& doc);

struct TWeightedDoc {
    TDbDocument Doc;
    double Weight = 0.0;
    bool Nasty = false;

    TWeightedDoc(const TDbDocument& doc, double weight)
        : Doc(doc)
        , Weight(weight)
        , Nasty(ComputeDocumentNasty(doc))
    {}
};
