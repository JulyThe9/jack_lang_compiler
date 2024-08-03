#include "DEBUG_CONTROL.h"

typedef std::string sourceFileNameType;
typedef std::map<TokenTypes, std::string>::iterator genMapIter;

std::map<TokenTypes, std::string> generationLookup
{
    {TokenTypes::tWHILE_START,  "label while_start_lbl_$"},
    {TokenTypes::tWHILE_JUMP,   "if-goto while_end_lbl_$"},
    {TokenTypes::tWHILE_END,    "goto while_start_lbl_$"},
    {TokenTypes::tWHILE,        "label while_end_lbl_$"},

    // INVERSION FOR IF
    {TokenTypes::tIF_JUMP,      "if-goto if_end_lbl_$"},
    {TokenTypes::tIF,           "label if_end_lbl_$"},

    // work because ifNode doesn't generate code upon else block discovery
    {TokenTypes::tELSE_START,   "label if_end_lbl_$"},
    {TokenTypes::tELSE_JUMP,    "goto else_end_lbl_$"},
    {TokenTypes::tELSE,         "label else_end_lbl_$"},

    {TokenTypes::tNUMBER,       "push constant $"},

    // TODO: support =
    {TokenTypes::tPLUS,     "add"},
    // TODO: implement neg
    {TokenTypes::tMINUS,    "sub"},
    {TokenTypes::tMULT,     "call Math.multiply 2"},
    {TokenTypes::tDIV,      "call Math.divide 2"},
    {TokenTypes::tAND,      "and"},
    {TokenTypes::tOR,       "or"},
    {TokenTypes::tNOT,      "not"},
    {TokenTypes::tLT,       "lt"},
    {TokenTypes::tGT,       "gt"}

    // identifier
};

std::string outFileExt = "vm";
