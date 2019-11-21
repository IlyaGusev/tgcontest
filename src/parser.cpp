#include "parser.h"

#include "../thirdparty/tinyxml2/tinyxml2.h"

Document ParseFile(const char* fileName) {
    tinyxml2::XMLDocument originalDoc;
    originalDoc.LoadFile(fileName);
    const tinyxml2::XMLElement* headElement = originalDoc.FirstChildElement("html")->FirstChildElement("head");
    const tinyxml2::XMLElement* metaElement = headElement->FirstChildElement("meta");
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
    return doc;
}


