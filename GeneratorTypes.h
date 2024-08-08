#include "DEBUG_CONTROL.h"

typedef std::string sourceFileNameType;
typedef std::map<AstNodeTypes, std::string>::iterator genMapIter;

std::map<AstNodeTypes, std::string> generationLookup
{
    {AstNodeTypes::aWHILE_START,  "label while_start_lbl_$"},
    {AstNodeTypes::aWHILE_JUMP,   "if-goto while_end_lbl_$"},
    {AstNodeTypes::aWHILE_END,    "goto while_start_lbl_$"},
    {AstNodeTypes::aWHILE,        "label while_end_lbl_$"},

    // INVERSION FOR IF
    {AstNodeTypes::aIF_JUMP,      "if-goto if_end_lbl_$"},
    {AstNodeTypes::aIF,           "label if_end_lbl_$"},

    // work because ifNode doesn't generate code upon else block discovery
    {AstNodeTypes::aELSE_START,   "label if_end_lbl_$"},
    {AstNodeTypes::aELSE_JUMP,    "goto else_end_lbl_$"},
    {AstNodeTypes::aELSE,         "label else_end_lbl_$"},

    {AstNodeTypes::aNUMBER,       "push constant $"},

    // TODO: support =
    {AstNodeTypes::aPLUS,     "add"},
    // TODO: implement neg
    {AstNodeTypes::aMINUS,    "sub"},
    {AstNodeTypes::aMULT,     "call Math.multiply 2"},
    {AstNodeTypes::aDIV,      "call Math.divide 2"},
    {AstNodeTypes::aAND,      "and"},
    {AstNodeTypes::aOR,       "or"},
    {AstNodeTypes::aNOT,      "not"},
    {AstNodeTypes::aLT,       "lt"},
    {AstNodeTypes::aGT,       "gt"}

    // identifier
};

std::string outFileExt = "vm";