#include <map>
#include "DEBUG_CONTROL.h"
#define LD_CLASS_OFFSET 50

enum class LangDataTypes : unsigned int
{
    ldINT,
    ldBOOLEAN,
    ldCHAR,
    ldVOID,
    ldARRAY,

    ldUNKNOWN,
    ldCLASS = LD_CLASS_OFFSET,
};

std::map<TokenTypes, LangDataTypes> tTypes_to_ldTypes
{
    {TokenTypes::tINT, LangDataTypes::ldINT},
    {TokenTypes::tBOOLEAN, LangDataTypes::ldBOOLEAN},
    {TokenTypes::tCHAR, LangDataTypes::ldCHAR},
    {TokenTypes::tVOID, LangDataTypes::ldVOID},
    {TokenTypes::tARRAY, LangDataTypes::ldARRAY},

    // special
    {TokenTypes::tIDENTIFIER, LangDataTypes::ldCLASS}
};
LangDataTypes tType_to_ldType(TokenTypes tType)
{
    std::map<TokenTypes, LangDataTypes>::iterator iter;
    iter = tTypes_to_ldTypes.find(tType);

    if (iter != tTypes_to_ldTypes.end())
        return iter->second;
    
    return LangDataTypes::ldUNKNOWN;
}

std::map<LangDataTypes, TokenTypes> ldTypes_to_tTypes
{
    {LangDataTypes::ldINT, TokenTypes::tINT},
    {LangDataTypes::ldBOOLEAN, TokenTypes::tBOOLEAN},
    {LangDataTypes::ldCHAR, TokenTypes::tCHAR},
    {LangDataTypes::ldVOID, TokenTypes::tVOID},
    {LangDataTypes::ldARRAY, TokenTypes::tARRAY},

    // special
    {LangDataTypes::ldCLASS, TokenTypes::tIDENTIFIER}
};

TokenTypes ldType_to_tType(LangDataTypes ldType)
{
    std::map<LangDataTypes, TokenTypes>::iterator iter;
    iter = ldTypes_to_tTypes.find(ldType);

    if (iter != ldTypes_to_tTypes.end())
        return iter->second;
    
    return TokenTypes::tUNKNOWN_SYMBOL;
}