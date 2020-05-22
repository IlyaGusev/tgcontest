#pragma once

#include "../document.h"

#include <functional>

bool ComputeDocumentNasty(const TDocument& doc);

struct TWeightedDoc {
    std::reference_wrapper<const TDocument> Doc;
    double Weight = 0.0;
    bool Nasty = false;

    TWeightedDoc(const TDocument& doc, double weight)
        : Doc(doc)
        , Weight(weight)
        , Nasty(ComputeDocumentNasty(doc))
    {}
};
