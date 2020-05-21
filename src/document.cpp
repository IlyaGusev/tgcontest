#include "document.h"
#include "util.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/filesystem.hpp>

#include <sstream>
#include <fstream>

TDocument::TDocument(const char* fileName) {
    if (boost::algorithm::ends_with(fileName, ".html")) {
        FromHtml(fileName);
    } else if (boost::algorithm::ends_with(fileName, ".json")) {
        FromJson(fileName);
    }
}

TDocument::TDocument(const nlohmann::json& json) {
    FromJson(json);
}

TDocument::TDocument(
    const tinyxml2::XMLDocument& html,
    const std::string& fileName
) {
    FromHtml(html, fileName);
}

nlohmann::json TDocument::ToJson() const {
    nlohmann::json json({
        {"url", Url},
        {"site_name", SiteName},
        {"timestamp", FetchTime},
        {"title", Title},
        {"description", Description},
        {"file_name", CleanFileName(FileName)},
        {"text", Text},
    });
    if (!OutLinks.empty()) {
        json["out_links"] = OutLinks;
    }
    if (Language) {
        json["language"] = Language.get();
    }
    if (Category != NC_UNDEFINED) {
        json["category"] = Category;
    }
    return json;
}

void TDocument::FromJson(const char* fileName) {
    std::ifstream fileStream(fileName);
    nlohmann::json json;
    fileStream >> json;
    FromJson(json);
}

void TDocument::FromJson(const nlohmann::json& json) {
    json.at("url").get_to(Url);
    json.at("site_name").get_to(SiteName);
    json.at("timestamp").get_to(FetchTime);
    json.at("title").get_to(Title);
    json.at("description").get_to(Description);
    json.at("text").get_to(Text);
    if (json.contains("file_name")) {
        json.at("file_name").get_to(FileName);
    }
    if (json.contains("out_links")) {
        json.at("out_links").get_to(OutLinks);
    }
    if (json.contains("language")) {
        Language = json.at("language");
    }
    if (json.contains("category")) {
        json.at("category").get_to(Category);
    }
}

std::string GetFullText(const tinyxml2::XMLElement* element) {
    if (const tinyxml2::XMLText* textNode = element->ToText()) {
        return textNode->Value();
    }
    std::string text;
    const tinyxml2::XMLNode* node = element->FirstChild();
    while (node) {
        if (const tinyxml2::XMLElement* elementNode = node->ToElement()) {
            text += GetFullText(elementNode);
        } else if (const tinyxml2::XMLText* textNode = node->ToText()) {
            text += textNode->Value();
        }
        node = node->NextSibling();
    }
    return text;
}

void ParseLinksFromText(const tinyxml2::XMLElement* element, std::vector<std::string>& links) {
    const tinyxml2::XMLNode* node = element->FirstChild();
    while (node) {
        if (const tinyxml2::XMLElement* nodeElement = node->ToElement()) {
            if (std::strcmp(nodeElement->Value(), "a") == 0 && nodeElement->Attribute("href")) {
                links.push_back(nodeElement->Attribute("href"));
            }
            ParseLinksFromText(nodeElement, links);
        }
        node = node->NextSibling();
    }
}

void TDocument::FromHtml(
    const char* fileName,
    bool parseLinks,
    bool shrinkText,
    size_t maxWords)
{
    if (!boost::filesystem::exists(fileName)) {
        throw std::runtime_error("No HTML file");
    }
    tinyxml2::XMLDocument originalDoc;
    originalDoc.LoadFile(fileName);

    FromHtml(originalDoc, fileName, parseLinks, shrinkText, maxWords);
}

void TDocument::FromHtml(
    const tinyxml2::XMLDocument& originalDoc,
    const std::string& fileName,
    bool parseLinks,
    bool shrinkText,
    size_t maxWords)
{
    FileName = fileName;

    const tinyxml2::XMLElement* htmlElement = originalDoc.FirstChildElement("html");
    if (!htmlElement) {
        throw std::runtime_error("Parser error: no html tag");
    }
    const tinyxml2::XMLElement* headElement = htmlElement->FirstChildElement("head");
    if (!headElement) {
        throw std::runtime_error("Parser error: no head");
    }
    const tinyxml2::XMLElement* metaElement = headElement->FirstChildElement("meta");
    if (!metaElement) {
        throw std::runtime_error("Parser error: no meta");
    }
    while (metaElement != 0) {
        const char* property = metaElement->Attribute("property");
        const char* content = metaElement->Attribute("content");
        if (content == nullptr || property == nullptr) {
            metaElement = metaElement->NextSiblingElement("meta");
            continue;
        }
        if (std::strcmp(property, "og:title") == 0) {
            Title = content;
        }
        if (std::strcmp(property, "og:url") == 0) {
            Url = content;
        }
        if (std::strcmp(property, "og:site_name") == 0) {
            SiteName = content;
        }
        if (std::strcmp(property, "og:description") == 0) {
            Description = content;
        }
        if (std::strcmp(property, "article:published_time") == 0) {
            FetchTime = DateToTimestamp(content);
        }
        metaElement = metaElement->NextSiblingElement("meta");
    }
    const tinyxml2::XMLElement* bodyElement = htmlElement->FirstChildElement("body");
    if (!bodyElement) {
        throw std::runtime_error("Parser error: no body");
    }
    const tinyxml2::XMLElement* articleElement = bodyElement->FirstChildElement("article");
    if (!articleElement) {
        throw std::runtime_error("Parser error: no article");
    }
    const tinyxml2::XMLElement* pElement = articleElement->FirstChildElement("p");
    {
        std::vector<std::string> links;
        size_t wordCount = 0;
        while (pElement && (!shrinkText || wordCount < maxWords)) {
            std::string pText = GetFullText(pElement);
            if (shrinkText) {
                std::istringstream iss(pText);
                std::string word;
                while (iss >> word) {
                    wordCount++;
                }
            }
            Text += pText + "\n";
            if (parseLinks) {
                ParseLinksFromText(pElement, links);
            }
            pElement = pElement->NextSiblingElement("p");
        }
        OutLinks = std::move(links);
    }
    const tinyxml2::XMLElement* addressElement = articleElement->FirstChildElement("address");
    if (!addressElement) {
        return;
    }
    const tinyxml2::XMLElement* timeElement = addressElement->FirstChildElement("time");
    if (timeElement && timeElement->Attribute("datetime")) {
        PubTime = DateToTimestamp(timeElement->Attribute("datetime"));
    }
    const tinyxml2::XMLElement* aElement = addressElement->FirstChildElement("a");
    if (aElement && aElement->Attribute("rel") && std::string(aElement->Attribute("rel")) == "author") {
        Author = aElement->GetText();
    }
}

std::string Preprocess(const std::string& text, const onmt::Tokenizer& tokenizer) {
    std::vector<std::string> tokens;
    tokenizer.tokenize(text, tokens);
    return boost::join(tokens, " ");
}

void TDocument::PreprocessTextFields(const onmt::Tokenizer& tokenizer) {
    PreprocessedTitle = Preprocess(Title, tokenizer);
    PreprocessedText = Preprocess(Text, tokenizer);
}
