#include <stack>

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


    std::vector<AstNode*> nChildNodes;
    std::optional<int> nPrecCoeff;      // relevant for operators only
};

struct parserState
{
    bool fsmFinished = false;
    unsigned int curTokenId = 0;
    ParseFsmStates fsmCurState = ParseFsmStates::sINIT;

    AstNode* astRoot;
    std::stack<AstNode*> pendParentNodes;

    parserState()
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

private:
    int layerCoeff = 0;
};