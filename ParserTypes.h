#include <stack>
#include <iostream>
#include "DEBUG_CONTROL.h"

// should be greater than the number of operators
#define LAYER_INCR 10
#define LAYER_DECR LAYER_INCR

#define MAX_EXPTECTE_AST_NODES 200

// to avoid confusion;
// we can reuse tINT, etc.
// but we need to make a distinction between
// 'types' in different levels (meta-level vs object-level)
#define ValueTypes TokenTypes

// preliminary, some will go away
enum class AstNodeTypes : unsigned int
{
    aCLASS = 0,
    aCONSTRUCTOR,
    aMETHOD,
    aFUNCTION,
    aVAR,
    aSTATIC,
    aFIELD,
    aLET,
    aDO,
    aIF,
    aELSE,
    aWHILE,
    aRETURN,
    aTRUE,
    aFALSE,
    aNULL,
    aTHIS,

    aLPR,
    aRPR,
    aLBR,
    aRBR,
    aLCURL,
    aRCURL,
    aACCESS,
    aEQUAL,
    aPLUS,
    aMINUS,
    aMULT,
    aDIV,
    aAND,
    aOR,
    aNOT,
    aLT,
    aGT,

    aIDENTIFIER,
    aNUMBER,

    // only AST node types (don't overlap with TokenTypes)
    aROOT,
    aARRAY, // array node
    aWHILE_START,
    aWHILE_END,
    aELSE_START,

    // only plays a role in code generation
    // so maybe find a way to carry over
    // the distinction to the generator
    aWHILE_JUMP,
    aIF_JUMP,
    aELSE_JUMP,

    aSTATEMENTS,

    // error-type, should never happen
    aUNKNOWN
};

#ifdef DEBUG
std::map<AstNodeTypes, std::string> aTypes_to_strings
{
    {AstNodeTypes::aCLASS, "CLASS"},
    {AstNodeTypes::aCONSTRUCTOR, "CONSTRUCTOR"},
    {AstNodeTypes::aMETHOD, "METHOD"},
    {AstNodeTypes::aFUNCTION, "FUNCTION"},
    {AstNodeTypes::aVAR, "VAR"},
    {AstNodeTypes::aSTATIC, "STATIC"},
    {AstNodeTypes::aFIELD, "FIELD"},
    {AstNodeTypes::aLET, "LET"},
    {AstNodeTypes::aDO, "DO"},
    {AstNodeTypes::aIF, "IF"},
    {AstNodeTypes::aELSE, "ELSE"},
    {AstNodeTypes::aWHILE, "WHILE"},
    {AstNodeTypes::aRETURN, "RETURN"},
    {AstNodeTypes::aTRUE, "TRUE"},
    {AstNodeTypes::aFALSE, "FALSE"},
    {AstNodeTypes::aNULL, "NULL"},
    {AstNodeTypes::aTHIS, "THIS"},
    {AstNodeTypes::aLPR, "LPR"},
    {AstNodeTypes::aRPR, "RPR"},
    {AstNodeTypes::aLBR, "LBR"},
    {AstNodeTypes::aRBR, "RBR"},
    {AstNodeTypes::aLCURL, "LCURL"},
    {AstNodeTypes::aRCURL, "RCURL"},
    {AstNodeTypes::aACCESS, "ACCESS"},
    {AstNodeTypes::aEQUAL, "EQUAL"},
    {AstNodeTypes::aPLUS, "PLUS"},
    {AstNodeTypes::aMINUS, "MINUS"},
    {AstNodeTypes::aMULT, "MULT"},
    {AstNodeTypes::aDIV, "DIV"},
    {AstNodeTypes::aAND, "AND"},
    {AstNodeTypes::aOR, "OR"},
    {AstNodeTypes::aNOT, "NOT"},
    {AstNodeTypes::aLT, "LT"},
    {AstNodeTypes::aGT, "GT"},
    {AstNodeTypes::aIDENTIFIER, "IDENTIFIER"},
    {AstNodeTypes::aNUMBER, "NUMBER"},
    {AstNodeTypes::aROOT, "ROOT"},
    {AstNodeTypes::aARRAY, "ARRAY"},
    {AstNodeTypes::aWHILE_START, "WHILE_START"},
    {AstNodeTypes::aWHILE_END, "WHILE_END"},
    {AstNodeTypes::aELSE_START, "ELSE_START"},
    {AstNodeTypes::aWHILE_JUMP, "WHILE_JUMP"},
    {AstNodeTypes::aIF_JUMP, "IF_JUMP"},
    {AstNodeTypes::aELSE_JUMP, "ELSE_JUMP"},
    {AstNodeTypes::aSTATEMENTS, "STATEMENTS"},
    {AstNodeTypes::aUNKNOWN, "UNKNOWN"}
};
const std::string &aType_to_string(AstNodeTypes aType)
{
    auto iter = aTypes_to_strings.find(aType);
    if (iter != aTypes_to_strings.end())
        return iter->second;
    
    auto unkIter = aTypes_to_strings.find(AstNodeTypes::aUNKNOWN);

    assert (unkIter != aTypes_to_strings.end());
    return unkIter->second;
}
#endif

std::map<TokenTypes, AstNodeTypes> tTypes_to_aTypes
{
    {TokenTypes::tCLASS, AstNodeTypes::aCLASS},
    {TokenTypes::tCONSTRUCTOR, AstNodeTypes::aCONSTRUCTOR},
    {TokenTypes::tMETHOD, AstNodeTypes::aMETHOD},
    {TokenTypes::tFUNCTION, AstNodeTypes::aFUNCTION},
    {TokenTypes::tVAR, AstNodeTypes::aVAR},
    {TokenTypes::tSTATIC, AstNodeTypes::aSTATIC},
    {TokenTypes::tFIELD, AstNodeTypes::aFIELD},
    {TokenTypes::tLET, AstNodeTypes::aLET},
    {TokenTypes::tDO, AstNodeTypes::aDO},
    {TokenTypes::tIF, AstNodeTypes::aIF},
    {TokenTypes::tELSE, AstNodeTypes::aELSE},
    {TokenTypes::tWHILE, AstNodeTypes::aWHILE},
    {TokenTypes::tRETURN, AstNodeTypes::aRETURN},
    {TokenTypes::tTRUE, AstNodeTypes::aTRUE},
    {TokenTypes::tFALSE, AstNodeTypes::aFALSE},
    {TokenTypes::tNULL, AstNodeTypes::aNULL},
    {TokenTypes::tTHIS, AstNodeTypes::aTHIS},

    {TokenTypes::tLPR, AstNodeTypes::aLPR},
    {TokenTypes::tRPR, AstNodeTypes::aRPR},
    {TokenTypes::tLBR, AstNodeTypes::aLBR},
    {TokenTypes::tRBR, AstNodeTypes::aRBR},
    {TokenTypes::tLCURL, AstNodeTypes::aLCURL},
    {TokenTypes::tRCURL, AstNodeTypes::aRCURL},
    {TokenTypes::tACCESS, AstNodeTypes::aACCESS},
    {TokenTypes::tEQUAL, AstNodeTypes::aEQUAL},
    {TokenTypes::tPLUS, AstNodeTypes::aPLUS},
    {TokenTypes::tMINUS, AstNodeTypes::aMINUS},
    {TokenTypes::tMULT, AstNodeTypes::aMULT},
    {TokenTypes::tDIV, AstNodeTypes::aDIV},
    {TokenTypes::tAND, AstNodeTypes::aAND},
    {TokenTypes::tOR, AstNodeTypes::aOR},
    {TokenTypes::tNOT, AstNodeTypes::aNOT},
    {TokenTypes::tLT, AstNodeTypes::aLT},
    {TokenTypes::tGT, AstNodeTypes::aGT},

    {TokenTypes::tIDENTIFIER, AstNodeTypes::aIDENTIFIER},
    {TokenTypes::tNUMBER, AstNodeTypes::aNUMBER}
};

AstNodeTypes tType_to_aType(TokenTypes tType)
{
    std::map<TokenTypes, AstNodeTypes>::iterator iter;
    iter = tTypes_to_aTypes.find(tType);

    if (iter != tTypes_to_aTypes.end())
        return iter->second;
    
    return AstNodeTypes::aUNKNOWN;
}
TokenTypes aType_to_tType(AstNodeTypes aType)
{
    for (auto it = tTypes_to_aTypes.begin(); it != tTypes_to_aTypes.end(); ++it)
    if (it->second == aType)
        return it->first;

    return TokenTypes::tUNKNOWN_SYMBOL;
}

enum class ScopeTypes : unsigned int
{
    ST_LOCAL = 0,
    ST_ARG
    // static, global
};


// TODO: TYPE-CHECKING
struct VariableData
{
    unsigned int nameID;
    ValueTypes valueType;
    VariableData(unsigned int nameID, ValueTypes valueType) :
        nameID(nameID), valueType(valueType)
    {}
};

struct ScopeFrame
{
private:
    std::vector<VariableData> localVars;
    std::vector<VariableData> argVars;
public:
    void addVarLocal(unsigned int nameID, ValueTypes valueType)
    {
        assert(isvartype(valueType));
        localVars.emplace_back(nameID, valueType);
    }
    void addVarArg(unsigned int nameID, ValueTypes valueType)
    {
        assert(isvartype(valueType));
        argVars.emplace_back(nameID, valueType);
    }
    std::pair<bool, unsigned int> containsLocal(unsigned int nameID)
    {
        auto iter = std::find_if(localVars.begin(), localVars.end(), 
            [&nameID](const VariableData &arg) { return arg.nameID == nameID; });

        return {iter != localVars.end(), 
                std::distance(localVars.begin(), iter)};
    }
    std::pair<bool, unsigned int> containsArg(unsigned int nameID)
    {
        auto iter = std::find_if(argVars.begin(), argVars.end(), 
            [&nameID](const VariableData &arg) { return arg.nameID == nameID; });

        return {iter != argVars.end(),
                std::distance(argVars.begin(), iter)};
    }
};

enum class ParseFsmStates : unsigned int
{
    sINIT = 0,
    sSTATEMENT_DECIDE,
    sWHILE,
    sBLOCK_CLOSE,
    sIF,
    sELSE,
    sEXPR,
    sVAR_DECL
};

std::map<TokenTypes, int> precedLookup
{
    {TokenTypes::tEQUAL, 3},
    {TokenTypes::tACCESS, 6},
    {TokenTypes::tPLUS, 4},
    {TokenTypes::tMINUS, 4},
    {TokenTypes::tMULT, 5},
    {TokenTypes::tDIV, 5},
    {TokenTypes::tAND, 1},
    {TokenTypes::tOR, 1},
    //{TokenTypes::tNOT},
    {TokenTypes::tLT, 2},
    {TokenTypes::tGT, 2}

};

/*
WHILE -> "lbl{" , "EXPR" , "JUMP" , STATEMENTS
*/
struct AstNode
{
private:
    // tbh trivial method, could return true for all and
    // generate whatever is in the "generation map file"
    // which would have empty values for these keys
    static bool checkGeneratesCode(AstNodeTypes aType)
    {
        return aType != AstNodeTypes::aSTATEMENTS &&
            aType != AstNodeTypes::aROOT;
    }
    static int assignId()
    {
        static int idPool = 0;
        return idPool++;
    }
    
    // moved to private to not mess with pointer
    AstNode *parentNode = NULL;
public:
    // any extra fields?
    explicit AstNode(TokenData &token) : aType(tType_to_aType(token.tType)), aVal(token.tVal),
        nID(assignId()), generatesCode(checkGeneratesCode(aType))
    {}

    AstNode(TokenData &token, int precCoeff) : aType(tType_to_aType(token.tType)), aVal(token.tVal),
        nPrecCoeff(precCoeff), nID(assignId()), generatesCode(checkGeneratesCode(aType))
    {}

    AstNode(AstNodeTypes aType) : aType(aType), nID(assignId()),
        generatesCode(checkGeneratesCode(aType))
    {}

    AstNode(AstNodeTypes aType, int aVal) : aType(aType), aVal(std::make_optional(aVal)), nID(assignId()),
        generatesCode(checkGeneratesCode(aType))
    {}

    AstNode() : aType(AstNodeTypes::aROOT), nID(assignId()), 
        generatesCode(checkGeneratesCode(AstNodeTypes::aROOT))
    {}

    ~AstNode() 
    {
#ifdef MISC_DEBUG        
        std::cout << "ast node DTOR called\n"; 
#endif
    }

    AstNodeTypes aType;
    std::optional<int> aVal;

    std::vector<AstNode*> nChildNodes;
    int nPrecCoeff = 0;      // relevant for operators only
    int nID = 0;
    bool generatesCode = false;

    int nValue;
    void setNValue(int value)
    {
        nValue = value;
    }

    int getNValue() const
    {
        return aVal.value();
    }
    
    void setParent(AstNode *parent)
    {
        parentNode = parent;
    }
    AstNode *getParent()
    {
        return parentNode;
    }
    void addChild(AstNode *child)
    {
        nChildNodes.push_back(child);
        nChildNodes.back()->setParent(this);
    }

#ifdef DEBUG
    void print()
    {
        std::cout << "AstNode #" << nID << '\n';
        std::cout << "Type: " << aType_to_string(aType) << '\n';
        std::cout << "Val: " << (aVal.has_value() ? std::to_string(aVal.value()) : "none")  << '\n';
        std::cout << "Children size: " << nChildNodes.size()  << '\n';
        std::cout << "Children:";
        for (auto *elem : nChildNodes)
        {
            std::cout << " #" << elem->nID;
        }
        std::cout << '\n';
        std::cout << '\n';
    }
#endif
};

AstNode *getIfBlockJump(AstNode *ifNodeDesc)
{
    auto *parent = ifNodeDesc->getParent();

    while (parent && parent->aType != AstNodeTypes::aIF)
    {
        parent = parent->getParent();
    }
    if (!parent)
        return NULL;

    assert(parent->nChildNodes.size() >= 3);
    auto *ifJumpNode = parent->nChildNodes[1];
    assert(ifJumpNode->aType == AstNodeTypes::aIF_JUMP);

    return ifJumpNode;
}

bool greaterPreced(const AstNode &t1, const AstNode &t2)
{
    auto tType1 = aType_to_tType(t1.aType);
    auto tType2 = aType_to_tType(t2.aType);
    
    assert(isoperator(tType1) && isoperator(tType2));

    auto itr1 = precedLookup.find(tType1);
    assert (itr1 != precedLookup.end());

    auto itr2 = precedLookup.find(tType2);
    assert (itr2 != precedLookup.end());

    return (itr1->second + t1.nPrecCoeff > itr2->second + t2.nPrecCoeff);
}

int getLabelId()
{
    static int labelId = 0;
    return labelId++;
}

// bool isconnectornode(TokenTypes tType)
// {
//     // also statements
//     return tType == TokenTypes::tARRAY;
// }
bool isblockstart(AstNodeTypes aType)
{
    return aType == AstNodeTypes::aWHILE 
        || aType == AstNodeTypes::aIF;
}

struct parserState
{
private:
    tokensVect *tokens;
    identifierVect *identifiers;
    unsigned int curTokenId = 0;
    bool tokensFinished = false;
    
public:
    bool fsmFinished = false;
    bool fsmFinishedCorrectly = true;
    ParseFsmStates fsmCurState = ParseFsmStates::sINIT;

    std::stack<AstNode*> pendParentNodes;
    std::stack<ScopeFrame> scopeFrames;

    parserState( tokensVect &tokens, identifierVect &identifiers) : 
        tokens(&tokens), identifiers(&identifiers)
    {
        reset();
    }

    void reset()
    {
        fsmFinished = false;
        fsmFinishedCorrectly = true;
        fsmCurState = ParseFsmStates::sINIT;
        curTokenId  = 0;
        layerCoeff = 0;
    }

    int getLayer() {return layerCoeff;}
    void incLayer() {layerCoeff+=LAYER_INCR;}
    void decLayer() {layerCoeff-=LAYER_DECR;}
    void resetLayer() {layerCoeff = 0;}

    void addStackTopChild(AstNode *child)
    {
        pendParentNodes.top()->addChild(child);
    }
    void addStackTop(AstNode *newTop)
    {
        pendParentNodes.push(newTop);
    }
    bool popStackTop()
    {   
        if (pendParentNodes.empty())
            return false;
        pendParentNodes.pop();
        return true;
    }
    AstNode &getStackTop()
    {
        return *(pendParentNodes.top());
    }

    void addScopeFramesTopVar(unsigned int nameID, ScopeTypes scopeType, ValueTypes valueType)
    {
        if (scopeType == ScopeTypes::ST_LOCAL)
            scopeFrames.top().addVarLocal(nameID, valueType);
        else if (scopeType == ScopeTypes::ST_ARG)
            scopeFrames.top().addVarArg(nameID, valueType);
    }
    void addNewScopFrame()
    {
        // pushing empty
        scopeFrames.push({});
    }
    bool popScopeFramesTop()
    {   
        if (scopeFrames.empty())
            return false;
        scopeFrames.pop();
        return true;
    }
    ScopeFrame &getScopeFramesTop()
    {
        return scopeFrames.top();
    }

    auto *getTokens()
    {
        return tokens;
    }
    auto *getIdent()
    {
        return identifiers;
    }

    TokenData &getCurToken()
    {
        return tokens->at(curTokenId);
    }
    bool advance()
    {
        if (curTokenId < tokens->size()-1)
        {
            curTokenId++;
            return true;
        }
        tokensFinished = true;
        return false;
    }
    TokenData &advanceAndGet()
    {
        advance();
        return getCurToken();
    }
    bool getTokensFinished() const
    {
        return tokensFinished;
    }
    bool fsmTerminate(bool finishedCorrectly)
    {
        fsmFinished = true;
        fsmFinishedCorrectly = finishedCorrectly;
        return fsmFinishedCorrectly;
    }


private:
    int layerCoeff = 0;
};