#include <stack>
#include <iostream>
#include "DEBUG_CONTROL.h"

// should be greater than the number of operators
#define LAYER_INCR 10
#define LAYER_DECR LAYER_INCR

#define MAX_EXPTECTE_AST_NODES 200

enum class ParseFsmStates : unsigned int
{
    sINIT = 0,
    sSTATEMENT_DECIDE,
    sWHILE,
    sBLOCK_CLOSE,
    sIF,
    sELSE,
    sEXPR
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
struct AstNode : TokenData
{
private:
    // tbh trivial method, could return true for all and
    // generate whatever is in the "generation map file"
    // which would have empty values for these keys
    static bool checkGeneratesCode(TokenTypes tType)
    {
        return tType != TokenTypes::tSTATEMENTS &&
            tType != TokenTypes::tROOT;
    }
    static int assignId()
    {
        static int idPool = 0;
        return idPool++;
    }
public:
    // any extra fields?
    explicit AstNode(TokenData &token) : TokenData(token), nID(assignId()), 
        generatesCode(checkGeneratesCode(token.tType))
    {}

    AstNode(TokenData &token, int precCoeff) : TokenData(token), nPrecCoeff(precCoeff), nID(assignId()),
        generatesCode(checkGeneratesCode(token.tType))
    {}

    AstNode(TokenTypes tType) : TokenData(tType), nID(assignId()),
        generatesCode(checkGeneratesCode(tType))
    {}

    AstNode() : TokenData(TokenTypes::tROOT), nID(assignId()), 
        generatesCode(checkGeneratesCode(TokenTypes::tROOT))
    {}

    ~AstNode() 
    {
#ifdef MISC_DEBUG        
        std::cout << "ast node DTOR called\n"; 
#endif
    }

    std::vector<AstNode*> nChildNodes;
    int nPrecCoeff = 0;      // relevant for operators only
    int nID = 0;
    bool generatesCode = false;

    int nValue;
    int nValueExtra;

    //"while_start_lbl_" + 
    void setNValue(int value)
    {
        nValue = value;
    }
    void setNValueExtra(int valueExtra)
    {
        nValueExtra = valueExtra;
    }

    void addChild(AstNode *child)
    {
        nChildNodes.push_back(child);
    }

#ifdef DEBUG
    void print()
    {
        std::cout << "AstNode #" << nID << '\n';
        std::cout << "Type: " << tokenLookupFindByVal(tType) << '\n';
        std::cout << "Val: " << (tVal.has_value() ? std::to_string(tVal.value()) : "none")  << '\n';
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

bool greaterPreced(const AstNode &t1, const AstNode &t2)
{
    assert(isoperator(t1.tType) && isoperator(t2.tType));

    auto itr1 = precedLookup.find(t1.tType);
    assert (itr1 != precedLookup.end());

    auto itr2 = precedLookup.find(t2.tType);
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
bool isblockstart(TokenTypes tType)
{
    return tType == TokenTypes::tWHILE 
        || tType == TokenTypes::tIF;
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