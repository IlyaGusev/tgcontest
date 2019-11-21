#include <exception>
#include <iostream>

#include "parser.h"

#include "../thirdparty/tinyxml2/tinyxml2.h"

std::string GetFullText(const tinyxml2::XMLElement* element) {
    if (element->ToText()) {
        return element->ToText()->Value();
    }
    std::string text;
    const tinyxml2::XMLNode* node = element->FirstChild();
    while (node) {
        if (node->ToText()) {
            text += node->ToText()->Value();
        } else if (node->ToElement()) {
            text += GetFullText(node->ToElement());
        }
        node = node->NextSibling();
    }
    return text;
}

Document ParseFile(const char* fileName) {
    tinyxml2::XMLDocument originalDoc;
    originalDoc.LoadFile(fileName);
    const tinyxml2::XMLElement* htmlElement = originalDoc.FirstChildElement("html");
    if (!htmlElement) {
        throw std::runtime_error("Parser error: no html");
    }
    const tinyxml2::XMLElement* headElement = htmlElement->FirstChildElement("head");
    if (!headElement) {
        throw std::runtime_error("Parser error: no head");
    }
    const tinyxml2::XMLElement* metaElement = headElement->FirstChildElement("meta");
    if (!metaElement) {
        throw std::runtime_error("Parser error: no meta");
    }
    Document doc;
    doc.FileName = fileName;
    while (metaElement != 0) {
        const char* property = metaElement->Attribute("property");
        const char* content = metaElement->Attribute("content");
        if (content == 0 || property == 0) {
            metaElement = metaElement->NextSiblingElement("meta");
            continue;
        }
        if (std::string(property) == "og:title") {
            doc.Title = content;
        }
        if (std::string(property) == "og:url") {
            doc.Url = content;
        }
        if (std::string(property) == "og:site_name") {
            doc.SiteName = content;
        }
        if (std::string(property) == "og:description") {
            doc.Description = content;
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
    while (pElement) {
        doc.Text += GetFullText(pElement) + "\n";
        pElement = pElement->NextSiblingElement("p");
    }
    const tinyxml2::XMLElement* addressElement = articleElement->FirstChildElement("address");
    if (addressElement) {
        const tinyxml2::XMLElement* timeElement = addressElement->FirstChildElement("time");
        if (timeElement && timeElement->Attribute("datetime")) {
            doc.DateTime = timeElement->Attribute("datetime");
        }
        const tinyxml2::XMLElement* aElement = addressElement->FirstChildElement("a");
        if (aElement && aElement->Attribute("rel") && std::string(aElement->Attribute("rel")) == "author") {
            doc.Author = aElement->GetText();
        }
    }
    return doc;
}


