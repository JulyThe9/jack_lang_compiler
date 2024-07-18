#include <stack>

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

private:
    std::vector<AstNode*> childNodes;
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
    }
};