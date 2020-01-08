#pragma once

#include "document.h"

#include <memory>
#include <set>
#include <unordered_map>
#include <vector>

#include <fasttext.h>

using TModelStorage = std::unordered_map<std::string, std::unique_ptr<fasttext::FastText>>;

void Annotate(
    const std::vector<std::string>& fileNames,
    const TModelStorage& models,
    const std::set<std::string>& languages,
    std::vector<TDocument>& docs,
    size_t minTextLength = 20,
    bool parseLinks = false);
