#ifndef _PARSER_
#define _PARSER_

#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <cassert>
#include <algorithm>
#include <sstream>

#include "LexerTypes.h"
#include "CheckerTypes.h"
#include "ParserTypes.h"
#include "GeneratorTypes.h"
#include "ArenaAllocator.h"
#include "DEBUG_CONTROL.h"

// HELPER MACROS
#define ALLOC_AST_NODE new (aralloc.allocate()) AstNode

class Parser
{
public:
    unsigned int thisNameID = 0;
private:
    // NOTE: IMPORTANT: 500 node limit so far
    ParserState pState;

    ArenaAllocator<AstNode> aralloc{ArenaAllocator<AstNode>(MAX_EXPTECTED_AST_NODES)};
    AstNode* astRoot = NULL;

    AstNode *createStackTopNode(ParserState &pState, TokenData &token);

    AstNode *createStackTopNode(ParserState &pState, AstNodeTypes aType, int aVal);

    AstNode *createStackTopNode(ParserState &pState, AstNodeTypes aType);

    void orderWhileLabels(AstNode *whileNode);

    void orderIfLables(AstNode *ifNode);

    void orderElseLabels(AstNode *elseNode);

    bool parseFuncPars(ParserState &pState);

public:
    void loadArrSysClass(unsigned int arrayLib_className_id);

    bool initStateBeh(ParserState &pState);

    void statementDecideStateBeh(ParserState &pState);

    void classDecideStateBeh(ParserState &pState);

    // !isStatic -> isField
    bool fieldAndStaticStateBeh(ParserState &pState, bool isStatic);

    bool ctorDefStateBeh(ParserState &pState);

    bool funcDefStateBeh(ParserState &pState, bool isMethod);

    bool returnStateBeh(ParserState &pState);

    bool funcDoCallStateBeh(ParserState &pState);

    std::tuple<bool, AstNode*> parseFuncCallArgs(ParserState &pState, int classID = -1);

    struct FuncMethodData
    {
        bool expectFunc;
        bool expectMethod;
        // scope of the variable which is
        // the object that we call a method on
        AstNodeTypes varAccessType;
        // its idx in the container for the
        // corresponding scope
        unsigned int varIdx;

        FuncMethodData(bool expectFunc, bool expectMethod)
            : expectFunc(expectFunc), expectMethod(expectMethod), 
                varAccessType(AstNodeTypes::aUNKNOWN), varIdx(0)
        {}
        FuncMethodData(bool expectFunc, bool expectMethod, AstNodeTypes varAccessType, unsigned int varIdx)
            : expectFunc(expectFunc), expectMethod(expectMethod), varAccessType(varAccessType), varIdx(varIdx)
        {}
    };

    std::tuple<bool, AstNode*> handleFuncNodes(TokenData &token, FuncMethodData &funcMethodData, int classID = -1);

    bool parseFuncCall(int classID, FuncMethodData &funcMethodData, AstNode *&resNode);

    // reference to pointer variable, because we need to change whatever pointer var
    // we provide as an argument (think C-style multiple returns)
    bool processIdentifier(TokenData &identToken, AstNode *&resNode, bool allowVariable = true);

    AstNode *parseExpr(ParserState &pState);

    bool whileStateBeh(ParserState &pState);

    bool ifStateBeh(ParserState &pState);

    bool elseStateBeh(ParserState &pState);

    void blockCloseStateBeh(ParserState &pState);

    bool varDeclStateBeh(ParserState &pState);

    bool varAssignStateBeh(ParserState &pState);

    AstNode *buildAST(tokensVect &tokens, identifierVect &identifiers, unsigned int tokenOffset);

    void resetState()
    {
        pState.resetNonShared();
    }

#ifdef DEBUG
void printAST()
{
    if (astRoot == NULL)
        return;
    std::cout << '\n';
    printAST(astRoot);
}
void printAST(AstNode *curRoot)
{
    // pre-order
    curRoot->print();
    for (auto childNode : curRoot->nChildNodes)
    {
        printAST(childNode);
    }
}
void printASTpost(AstNode *curRoot)
{
    for (auto childNode : curRoot->nChildNodes)
    {
        printASTpost(childNode);
    }
    curRoot->print();
}
#endif
};

#endif