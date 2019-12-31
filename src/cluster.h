#pragma once

#include "document.h"

#include <functional>

class TNewsCluster {
private:
    std::vector<std::reference_wrapper<const TDocument>> Documents;

public:
    TNewsCluster() = default;

    void AddDocument(const TDocument& document);
    uint64_t GetTimestamp(float percentile = 0.9) const;
    uint64_t GetFreshestTimestamp() const;
    ENewsCategory GetCategory() const;
    std::string GetTitle() const { return Documents.at(0).get().Title; }
    size_t GetSize() const { return Documents.size(); }
    const std::vector<std::reference_wrapper<const TDocument>>& GetDocuments() const { return Documents; }
    std::string GetLanguage() const { return Documents.at(0).get().Language.get(); }
};

