#include "DEBUG_CONTROL.h"

typedef std::string sourceFileNameType;

std::map<TokenTypes, std::string> generationLookup
{
    {TokenTypes::tWHILE_START,  "label $"},
    {TokenTypes::tJUMP,         "goto $"},
    {TokenTypes::tWHILE,        "goto $\nlabel %"},
    {TokenTypes::tNUMBER,       "push constant $"}
    // identifier
};

std::string outFileExt = ".vm";
