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

    tokensVect tokens;
    identifierVect identifiers;

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
        return buffer[0] == '\0' && bIdx == 0;
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
    }
};

struct TokenData
{
    TokenTypes tType;
    std::optional<int> tVal;

    TokenData(TokenTypes tType) : tType(tType)
    {}
    TokenData(TokenTypes tType, int tVal) : tType(tType), tVal(std::make_optional(tVal))
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

    // alphabetic data types
    tINT,
    tBOOLEAN,
    tCHAR,
    tVOID,

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

    tIDENTIFIER,
    tNUMBER,

    tUNKNOWN_SYMBOL
};

std::map<std::string, TokenTypes> tokenLookup
{
    {"class", TokenTypes::tCLASS},
    {"constructor", TokenTypes::tCONSTRUCTOR},
    {"method", TokenTypes::tMETHOD},
    {"function", TokenTypes::tFUNCTION},
    {"var", TokenTypes::tVAR},
    {"static", TokenTypes::tSTATIC},
    {"field", TokenTypes::tFIELD},
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

    {"int", TokenTypes::tINT},
    {"boolean", TokenTypes::tBOOLEAN},
    {"char", TokenTypes::tCHAR},
    {"void", TokenTypes::tVOID},

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
    {">", TokenTypes::tGT}

};

bool isopertator(TokenTypes tType)
{
    return tType >= TokenTypes::tPLUS && tType <= TokenTypes::tGT;
}