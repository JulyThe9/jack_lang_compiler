#include "DEBUG_CONTROL.h"

typedef std::string sourceFileNameType;
typedef std::map<AstNodeTypes, std::string>::iterator genMapIter;

// TODO: validation: if node generates code but its type not found in
// generationLookup, then we made a mistake somewhere
std::map<AstNodeTypes, std::string> generationLookup
{
    {AstNodeTypes::aWHILE_START,    "label while_start_lbl_$\r\n"},
    {AstNodeTypes::aWHILE_JUMP,     "if-goto while_end_lbl_$\r\n"},
    {AstNodeTypes::aWHILE_END,      "goto while_start_lbl_$\r\n"},
    {AstNodeTypes::aWHILE,          "label while_end_lbl_$\r\n"},

    // INVERSION FOR IF
    {AstNodeTypes::aIF_JUMP,        "if-goto if_end_lbl_$\r\n"},
    {AstNodeTypes::aIF,             "label if_end_lbl_$\r\n"},

    // work because ifNode doesn't generate code upon else block discovery
    {AstNodeTypes::aELSE_START,     "label if_end_lbl_$\r\n"},
    {AstNodeTypes::aELSE_JUMP,      "goto else_end_lbl_$\r\n"},
    {AstNodeTypes::aELSE,           "label else_end_lbl_$\r\n"},

    {AstNodeTypes::aNUMBER,          "push constant $\r\n"},
    {AstNodeTypes::aLOCAL_VAR_READ,  "push local $\r\n"},
    {AstNodeTypes::aARG_VAR_READ,    "push arg $\r\n"},
    {AstNodeTypes::aLOCAL_VAR_WRITE, "pop local $\r\n"},
    {AstNodeTypes::aARG_VAR_WRITE,   "pop arg $\r\n"},
    
    {AstNodeTypes::aTHIS_READ,       "push pointer 0\r\n"},
    {AstNodeTypes::aTHIS_WRITE,      "pop pointer 0\r\n"}, // ever used?
    {AstNodeTypes::aTHAT_READ,       "push pointer 1\r\n"},
    {AstNodeTypes::aTHAT_WRITE,      "pop pointer 1\r\n"}, // ever used?

    // TODO: support =
    {AstNodeTypes::aPLUS,     "add\r\n"},
    // TODO: implement neg
    {AstNodeTypes::aMINUS,    "sub\r\n"},
    {AstNodeTypes::aMULT,     "call Math.multiply 2\r\n"},
    {AstNodeTypes::aDIV,      "call Math.divide 2\r\n"},
    {AstNodeTypes::aAND,      "and\r\n"},
    {AstNodeTypes::aOR,       "or\r\n"},
    {AstNodeTypes::aNOT,      "not\r\n"},
    {AstNodeTypes::aLT,       "lt\r\n"},
    {AstNodeTypes::aGT,       "gt\r\n"},
    {AstNodeTypes::aNEG_MINUS,"neg\r\n"},

    // function-related
    {AstNodeTypes::aFUNC_DEF,       "function $ "},
    {AstNodeTypes::aFUNC_LOCNUM,    "$\r\n"},


    {AstNodeTypes::aFUNC_CALL,    "call $ "},
    {AstNodeTypes::aFUNC_ARGNUM,  "$\r\n"},

    // will need to paramtetrize on local/arg/field + idx
    //{AstNodeTypes::aFUNC_RET_VAL,     "push ...\r\n"},

    // this node is visited the last in a function subtree of AST,
    // so it's simply a return
    {AstNodeTypes::aFUNCTION,       "return\r\n"},

    // class-related
    // ctor "func" variation
    {AstNodeTypes::aCTOR_ALLOC,     "push $\r\ncall Memory.alloc 1\r\npop pointer 0\r\n"},
    
    {AstNodeTypes::aFIELD_VAR_READ,   "push this $\r\n"},
    {AstNodeTypes::aFIELD_VAR_WRITE,  "pop this $\r\n"},
    {AstNodeTypes::aSTATIC_VAR_READ,  "push static $\r\n"},
    {AstNodeTypes::aSTATIC_VAR_WRITE, "pop static $\r\n"}
};

std::string outFileExt = "vm";