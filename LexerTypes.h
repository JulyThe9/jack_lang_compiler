#include "DEBUG_CONTROL.h"

enum class LexFsmStates : unsigned int
{
    sINIT = 0,
    sLETTERS,
    sDIGITS_AFTER_ALPHA,
    sDIGITS,
    sSYMBOL,
    sCOMMENT,
    sMLINE_COMMENT
};

struct lexerState
{
    bool fsmFinished = false;
    LexFsmStates fsmCurState = LexFsmStates::sINIT;

    bool commentOpen = false;
    bool mlineComment = false;
    char buffer[20];
    uint8_t bIdx = 0;

    int lexedLineIdx = 0;
    tokensVect tokens;
    identifierVect identifiers;
    // Whether the last oper or term token
    // is operator, false -> term.
    // Starting at true for when the expr
    // starts with -. E.g. x = -8
    bool lastOperTermIsOper = true;
    bool onRhs = false;

    void flush()
    {
        buffer[0] = '\0';
        bIdx = 0;
    }
    bool addBuff(char c)
    {
        if (bIdx >= 20)
            return false;
        buffer[bIdx++] = c;
        return true;
    }

    bool buffEmpty()
    {
        return buffer[0] == '\0' || bIdx == 0;
    }

    void reset()
    {
        flush();
        fsmFinished = false;
        if (commentOpen)
        {
            if (mlineComment)
            {
                fsmCurState = LexFsmStates::sMLINE_COMMENT;
                return;
            }
            else
               commentOpen = false;
        }
        fsmCurState = LexFsmStates::sINIT;
        lastOperTermIsOper = true;
        onRhs = false;
    }
};

struct DebugData
{  
    int debug_lineNum;
    DebugData(int debug_lineNum) : debug_lineNum(debug_lineNum)
    {}
    virtual ~DebugData() = 0;
};
DebugData::~DebugData() {}

struct TokenData : DebugData
{
    TokenTypes tType;
    std::optional<int> tVal;

    TokenData(int debug_lineNum, TokenTypes tType) : DebugData(debug_lineNum), tType(tType)
    {}
    TokenData(int debug_lineNum, TokenTypes tType, int tVal) : DebugData(debug_lineNum),
        tType(tType), tVal(std::make_optional(tVal))
    {}
};

enum class TokenTypes : unsigned int
{
    // alphabetic keywords
    tCLASS = 0,
    tCONSTRUCTOR,
    tMETHOD,
    tFUNCTION,
    tVAR,
    tSTATIC,
    tFIELD,
    tLET,
    tDO,
    tIF,
    tELSE,
    tWHILE,
    tRETURN,
    tTRUE,
    tFALSE,
    tNULL,
    tTHIS,
    tTHAT,

    // alphabetic data types
    tINT,
    tBOOLEAN,
    tCHAR,
    tVOID,
    tARRAY,

    // symbolic
    tLPR,
    tRPR,
    tLBR,
    tRBR,
    tLCURL,
    tRCURL,
    tCOMMA,
    tSEMICOLON,
    tACCESS,
    tEQUAL,
    tPLUS,
    tMINUS,
    tMULT,
    tDIV,
    tAND,
    tOR,
    tNOT,
    tLT,
    tGT,

    tNEG_MINUS,

    tIDENTIFIER,
    tNUMBER,

    tUNKNOWN_SYMBOL    
};

#if defined(LEXER_DEBUG) || defined(ERR_DEBUG)
std::map<TokenTypes, std::string> tTypes_to_strings 
{
    {TokenTypes::tCLASS, "CLASS"},
    {TokenTypes::tCONSTRUCTOR, "CONSTRUCTOR"},
    {TokenTypes::tMETHOD, "METHOD"},
    {TokenTypes::tFUNCTION, "FUNCTION"},
    {TokenTypes::tVAR, "VAR"},
    {TokenTypes::tSTATIC, "STATIC"},
    {TokenTypes::tFIELD, "FIELD"},
    {TokenTypes::tLET, "LET"},
    {TokenTypes::tDO, "DO"},
    {TokenTypes::tIF, "IF"},
    {TokenTypes::tELSE, "ELSE"},
    {TokenTypes::tWHILE, "WHILE"},
    {TokenTypes::tRETURN, "RETURN"},
    {TokenTypes::tTRUE, "TRUE"},
    {TokenTypes::tFALSE, "FALSE"},
    {TokenTypes::tNULL, "NULL"},
    {TokenTypes::tTHIS, "THIS"},
    {TokenTypes::tTHAT, "THAT"},
    {TokenTypes::tINT, "INT"},
    {TokenTypes::tBOOLEAN, "BOOLEAN"},
    {TokenTypes::tCHAR, "CHAR"},
    {TokenTypes::tVOID, "VOID"},
    {TokenTypes::tARRAY, "ARRAY"},
    {TokenTypes::tLPR, "LPR"},
    {TokenTypes::tRPR, "RPR"},
    {TokenTypes::tLBR, "LBR"},
    {TokenTypes::tRBR, "RBR"},
    {TokenTypes::tLCURL, "LCURL"},
    {TokenTypes::tRCURL, "RCURL"},
    {TokenTypes::tCOMMA, "COMMA"},
    {TokenTypes::tSEMICOLON, "SEMICOLON"},
    {TokenTypes::tACCESS, "ACCESS"},
    {TokenTypes::tEQUAL, "EQUAL"},
    {TokenTypes::tPLUS, "PLUS"},
    {TokenTypes::tMINUS, "MINUS"},
    {TokenTypes::tMULT, "MULT"},
    {TokenTypes::tDIV, "DIV"},
    {TokenTypes::tAND, "AND"},
    {TokenTypes::tOR, "OR"},
    {TokenTypes::tNOT, "NOT"},
    {TokenTypes::tLT, "LT"},
    {TokenTypes::tGT, "GT"},
    {TokenTypes::tNEG_MINUS, "NEG_MINUS"},
    {TokenTypes::tIDENTIFIER, "IDENTIFIER"},
    {TokenTypes::tNUMBER, "NUMBER"},
    {TokenTypes::tUNKNOWN_SYMBOL, "UNKNOWN_SYMBOL"}
};
const std::string &tType_to_string(TokenTypes tType)
{
    auto iter = tTypes_to_strings.find(tType);
    if (iter != tTypes_to_strings.end())
        return iter->second;
    
    auto unkIter = tTypes_to_strings.find(TokenTypes::tUNKNOWN_SYMBOL);

    assert (unkIter != tTypes_to_strings.end());
    return unkIter->second;
}
#endif

std::map<std::string, TokenTypes> tokenLookup
{
    {"class", TokenTypes::tCLASS},
    {"constructor", TokenTypes::tCONSTRUCTOR},
    {"method", TokenTypes::tMETHOD},
    {"function", TokenTypes::tFUNCTION},
    {"var", TokenTypes::tVAR},
    {"field", TokenTypes::tFIELD},
    {"static", TokenTypes::tSTATIC},
    {"let", TokenTypes::tLET},
    {"do", TokenTypes::tDO},
    {"if", TokenTypes::tIF},
    {"else", TokenTypes::tELSE},
    {"while", TokenTypes::tWHILE},
    {"return", TokenTypes::tRETURN},
    {"true", TokenTypes::tTRUE},
    {"false", TokenTypes::tFALSE},
    {"null", TokenTypes::tNULL},
    {"this", TokenTypes::tTHIS},
    {"that", TokenTypes::tTHAT},

    {"int", TokenTypes::tINT},
    {"boolean", TokenTypes::tBOOLEAN},
    {"char", TokenTypes::tCHAR},
    {"void", TokenTypes::tVOID},
    {"Array", TokenTypes::tARRAY},

    {"(", TokenTypes::tLPR},
    {")", TokenTypes::tRPR},
    {"[", TokenTypes::tLBR},
    {"]", TokenTypes::tRBR},
    {"{", TokenTypes::tLCURL},
    {"}", TokenTypes::tRCURL},
    {",", TokenTypes::tCOMMA},
    {";", TokenTypes::tSEMICOLON},
    {"=", TokenTypes::tEQUAL},
    {".", TokenTypes::tACCESS},
    {"+", TokenTypes::tPLUS},
    {"-", TokenTypes::tMINUS},
    {"*", TokenTypes::tMULT},
    {"/", TokenTypes::tDIV},
    {"&", TokenTypes::tAND},
    {"|", TokenTypes::tOR},
    {"~", TokenTypes::tNOT},
    {"<", TokenTypes::tLT},
    {">", TokenTypes::tGT},
    {"-", TokenTypes::tNEG_MINUS}
    
#ifdef DEBUG    
    // ,
    // {"_ROOT_", TokenTypes::tROOT},
    // {"NUMBER", TokenTypes::tNUMBER},
    // {"IDENTIFIER", TokenTypes::tIDENTIFIER},
    // {"ARRAY NODE", TokenTypes::tARRAY},
    // {"WHILE START", TokenTypes::tWHILE_START},
    // {"WHILE END", TokenTypes::tWHILE_END},
    // {"ELSE START", TokenTypes::tELSE_START},
    // {"WHILE JUMP", TokenTypes::tWHILE_JUMP},
    // {"IF JUMP", TokenTypes::tIF_JUMP},
    // {"ELSE JUMP", TokenTypes::tELSE_JUMP},
    // {"STATEMENTS", TokenTypes::tSTATEMENTS}
#endif
};

bool isbinaryperator(TokenTypes tType)
{
    return tType >= TokenTypes::tEQUAL && tType <= TokenTypes::tGT;
}
bool isoperator(TokenTypes tType)
{
    return isbinaryperator(tType) || tType== TokenTypes::tNEG_MINUS;
}

// tType is meta-language (C++) type VS.
// the value of it corresponds to object language (Jack) type

bool isprimitivevartype(TokenTypes tType)
{
    return tType == TokenTypes::tINT    ||
        tType == TokenTypes::tBOOLEAN   ||
        tType == TokenTypes::tCHAR      ||
        tType == TokenTypes::tVOID;
}

bool isarraytype(TokenTypes tType)
{
    return tType == TokenTypes::tARRAY;
}

bool isvartype(TokenTypes tType)
{
    return isprimitivevartype(tType) ||
        isarraytype(tType) ||
        tType == TokenTypes::tIDENTIFIER;
}

bool isexprkeyword(TokenTypes tType)
{
    return tType == TokenTypes::tTHIS ||
        tType == TokenTypes::tTHAT ||
        tType == TokenTypes::tTRUE ||
        tType == TokenTypes::tFALSE ||
        tType == TokenTypes::tNULL;
}

#ifdef DEBUG
std::string emptyStr("");
const std::string &tokenLookupFindByVal(TokenTypes tType)
{
    for (auto it = tokenLookup.begin(); it != tokenLookup.end(); ++it)
        if (it->second == tType)
            return it->first;

    return emptyStr;
}
#endif