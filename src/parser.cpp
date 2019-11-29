#include "parser.h"
#include <regex>
#include <stdexcept>
#include <ctime>
#include <tinyxml2/tinyxml2.h>

std::string GetFullText(const tinyxml2::XMLElement* element) {
    if (element->ToText()) {
        return element->ToText()->Value();
    }
    std::string text;
    const tinyxml2::XMLNode* node = element->FirstChild();
    while (node) {
        if (node->ToText()) {
            text += node->ToText()->Value();
        }
        node = node->NextSibling();
    }
    return text;
}

void ParseLinksFromText(const tinyxml2::XMLElement* element, std::vector<std::string>& links) {
    const tinyxml2::XMLNode* node = element->FirstChild();
    while (node) {
        if (const auto nodeElement = node->ToElement()) {
            if (nodeElement->Value() == std::string("a") && nodeElement->Attribute("href")) {
                links.push_back(nodeElement->Attribute("href"));
            }
            ParseLinksFromText(nodeElement, links);
        }
        node = node->NextSibling();
    }
}

uint64_t DateToTimestamp(const std::string& date) {
    //TO DO: parse timezones (std cant handle them)
    std::regex ex("(\\d\\d\\d\\d)-(\\d\\d)-(\\d\\d)T(\\d\\d):(\\d\\d):(\\d\\d)([+-])(\\d\\d):(\\d\\d)");
    std::smatch what;
    if (std::regex_match(date, what, ex) && what.size() >= 10) {
        std::tm t = {
            .tm_sec = std::stoi(what[6]),
            .tm_min = std::stoi(what[5]),
            .tm_hour = std::stoi(what[4]),
            .tm_mday = std::stoi(what[3]),
            .tm_mon = std::stoi(what[2]),
            .tm_year = std::stoi(what[1]) - 1900,
            .tm_wday = 0,
            .tm_yday = 0,
            .tm_isdst = 0
        };
        std::tm worldBeginning = {
            .tm_sec = 0,
            .tm_min = 0,
            .tm_hour = 0,
            .tm_mday = 1,
            .tm_mon = 0,
            .tm_year = 70,
            .tm_wday = 0,
            .tm_yday = 0,
            .tm_isdst = 0
        };

        const auto timestamp = std::difftime(std::mktime(&t), std::mktime(&worldBeginning));
        return timestamp > 0 ? timestamp : 0;
    }
    return 0.0;

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
    {
        std::vector<std::string> links;
        while (pElement) {
            doc.Text += GetFullText(pElement) + "\n";
            ParseLinksFromText(pElement, links);
            pElement = pElement->NextSiblingElement("p");
        }
        doc.OutLinks = std::move(links);
    }
    const tinyxml2::XMLElement* addressElement = articleElement->FirstChildElement("address");
    if (addressElement) {
        const tinyxml2::XMLElement* timeElement = addressElement->FirstChildElement("time");
        if (timeElement && timeElement->Attribute("datetime")) {
            doc.DateTime = timeElement->Attribute("datetime");
            doc.Timestamp = DateToTimestamp(doc.DateTime);
        }
        const tinyxml2::XMLElement* aElement = addressElement->FirstChildElement("a");
        if (aElement && aElement->Attribute("rel") && std::string(aElement->Attribute("rel")) == "author") {
            doc.Author = aElement->GetText();
        }
    }

    return doc;
}
