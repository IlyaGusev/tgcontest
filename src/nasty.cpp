#include "nasty.h"

bool ComputeDocumentNasty(const TDbDocument& document) {
    if ((document.Language == tg::LN_EN) && (document.Title.size() < 16)) {
        return true;
    }

    if ((document.Language == tg::LN_RU) && (document.Title.size() < 30)) {
        return true;
    }

    unsigned char lastSymb = document.Title.back();
    if (lastSymb == 0x21 || lastSymb == 0x3f || lastSymb == 0x2e || lastSymb == 0x20) { 
        return true; // !?. and space
    }

    unsigned char firstSymb = document.Title.front();
    if (firstSymb == 0x22 || firstSymb == 0xab) {
        return true; // "«
    }

    if (std::count(document.Title.begin(), document.Title.end(), 0x20) < 2) {
        return true; // 3 words min
    }

    return false;
}
