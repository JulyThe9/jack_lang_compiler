#include <stack>
#include <iostream>

// should be greater than the number of operators
#define LAYER_INCR 10
#define LAYER_DECR LAYER_INCR

enum class ParseFsmStates : unsigned int
{
    sINIT = 0,
    sSTATEMENT,
    sWHILE,
    sWHILE_CLOSE,
    sEXPR
};

struct AstNode : TokenData
{
public:
    // any extra fields?
    AstNode(TokenTypes tType, int tVal) : TokenData(tType, tVal) {}
    AstNode(TokenTypes tType) : TokenData(tType) {}
    AstNode() : TokenData(TokenTypes::tUNDEFINED) {}
    ~AstNode() { std::cout << "ast node DTOR called\n"; }

    std::vector<AstNode*> nChildNodes;
    std::optional<int> nPrecCoeff;      // relevant for operators only

    void addChild(AstNode *child)
    {
        nChildNodes.push_back(child);
    }
};

struct parserState
{
private:
    tokensVect *tokens;
    identifierVect *identifiers;
    
public:
    bool fsmFinished = false;
    unsigned int curTokenId = 0;
    ParseFsmStates fsmCurState = ParseFsmStates::sINIT;

    AstNode* astRoot;
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

private:
    int layerCoeff = 0;
};