#define BOOST_TEST_DYN_LINK

#define BOOST_TEST_MODULE "ParserModule"

#define STR_EXPAND(tok) #tok
#define STR(tok) STR_EXPAND(tok)

#include "../src/parser.h"
#include "../src/document.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE( parser )
{
    const char* testFile = STR(TEST_PATH)"/example1.html";
    TDocument document = ParseFile(testFile);
    BOOST_CHECK(document.Title == "Израиль приостановил процедуру экстрадиции гражданина РФ в США");
}

