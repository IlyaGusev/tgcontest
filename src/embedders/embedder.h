#pragma once

#include "config.pb.h"
#include "enum.pb.h"

class TEmbedder {
public:
    explicit TEmbedder(tg::EEmbedderField field = tg::EF_ALL) : Field(field) {}

    virtual ~TEmbedder() = default;

    virtual std::vector<float> CalcEmbedding(const std::string& input) const = 0;

    std::vector<float> CalcEmbedding(const std::string& title, const std::string& text) const {
        std::string input;
        if (Field == tg::EF_ALL) {
            input = title + " " + text;
        } else if (Field == tg::EF_TITLE) {
            input = title;
        } else if (Field == tg::EF_TEXT) {
            input = text;
        }
        return CalcEmbedding(input);
    }

protected:
    tg::EEmbedderField Field;
};
