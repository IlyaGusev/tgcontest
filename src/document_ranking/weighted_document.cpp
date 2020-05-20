#include "weighted_document.h"

#include <string>
#include <locale>
#include <codecvt>
#include <uchar.h>

bool ComputeDocumentNasty(const TDocument& doc) {
    std::u16string utf16 = std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>{}.from_bytes(doc.Title.data());
 
    if (utf16.length() < 16) {
        return true;
    }    

    const char16_t lastSymb = utf16.back();
    if (lastSymb == 0x21 || lastSymb == 0x3f || lastSymb == 0x2e || lastSymb == 0x20) {
        return true; // !?. and space
    }

    const char16_t firstSymb = utf16.front();
    if (firstSymb == 0x22 || firstSymb == 0xab) {
        return true; // "«
    }

    if (std::count(utf16.begin(), utf16.end(), 0x20) < 2) {
        return true; // 3 words min
    }

    return false;
}
