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
    sSTATEMENT,
    sWHILE,
    sWHILE_CLOSE,
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

struct AstNode : TokenData
{
private:
    static int assignId()
    {
        static int idPool = 0;
        return idPool++;
    }
public:
    // any extra fields?
    explicit AstNode(TokenData &token) : TokenData(token) { nID = assignId(); }
    AstNode(TokenData &token, int precCoeff) : TokenData(token), nPrecCoeff(precCoeff) { nID = assignId(); }
    AstNode(TokenTypes tType) : TokenData(tType) { nID = assignId(); }
    AstNode() : TokenData(TokenTypes::tUNDEFINED) { nID = assignId(); }
    ~AstNode() 
    {
#ifdef MISC_DEBUG        
        std::cout << "ast node DTOR called\n"; 
#endif
    }

    std::vector<AstNode*> nChildNodes;
    int nPrecCoeff = 0;      // relevant for operators only
    int nID = 0;

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

bool isconnectornode(TokenTypes tType)
{
    // also statements
    return tType == TokenTypes::tARRAY;
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
        fsmCurState = ParseFsmStates::sINIT;
        curTokenId  = 0;
        layerCoeff = 0;
    }

    int getLayer() {return layerCoeff;}
    void incLayer() {layerCoeff+=LAYER_INCR;}
    void decLayer() {layerCoeff-=LAYER_DECR;}

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
        if (curTokenId < tokens->size())
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


private:
    int layerCoeff = 0;
};