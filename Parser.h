#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <cassert>
#include <algorithm>

#include "Utils.h"
#include "JackCompilerTypes.h"
#include "ArenaAllocator.h"
#include "DEBUG_CONTROL.h"

// HELPER MACROS
#define ALLOC_AST_NODE new (aralloc.allocate()) AstNode

class Parser
{
private:
    // IMPORTANT: 200 node limit so far
    ArenaAllocator<AstNode> aralloc{ArenaAllocator<AstNode>(MAX_EXPTECTE_AST_NODES)};
    AstNode* astRoot;

    inline AstNode *createAstNode(parserState &pState, TokenData &token)
    {
        AstNode *astNode = ALLOC_AST_NODE(token);
        pState.addStackTopChild(astNode);
        pState.addStackTop(astNode);
        return astNode;
    }

public:

    void initStateBeh(parserState &pState)
    {
        // TEMPORARY:
        pState.fsmCurState = ParseFsmStates::sSTATEMENT;
    }

    void statementStateBeh(parserState &pState)
    {
        auto &token = pState.getCurToken();
        switch (token.tType)
        {
            case TokenTypes::tWHILE:
                pState.fsmCurState = ParseFsmStates::sWHILE;
                break;
        }
    }

    void parseExpr(parserState &pState)
    {
        auto storedState = pState.fsmCurState;
    
        AstNode *curTermNode = NULL;
        // checking if expr is finished
        while (true)
        {   
            auto &token = pState.getCurToken();
            // we hit the right bracket of while/if or the statement has ended with ;
            if ((pState.getLayer() == 0 && token.tType == TokenTypes::tRPR)
                || token.tType  == TokenTypes::tSEMICOLON)
            {
                break;
            }

            if (token.tType == TokenTypes::tNUMBER)
            {
                curTermNode = ALLOC_AST_NODE(token);
            }
            // arrays for later
            else if (token.tType == TokenTypes::tIDENTIFIER)
            {
                curTermNode = ALLOC_AST_NODE(token);
            }
            else if (isoperator(token.tType))
            {
                // stack top (potentially operator)
                auto &stackTop = pState.getStackTop();
                // current token (operator) as AST node
                // commin for the following two cases and needed for greaterPreced,\
                // so carried placed here
                auto *operNode = ALLOC_AST_NODE(token, pState.getLayer());

                // start of the expression, parent is while, for or general stms
                if (!isoperator(stackTop.tType) || greaterPreced(*operNode, stackTop))
                {       
                    operNode->addChild(curTermNode);
                    pState.addStackTop(operNode);      
                    pState.advance();
                    continue;
                }

                stackTop.addChild(curTermNode);
                operNode->addChild(&stackTop);
                pState.popStackTop();
                pState.addStackTop(operNode);
            }
            else if (token.tType == TokenTypes::tLPR)
            {
                pState.incLayer();
            }
            else if (token.tType == TokenTypes::tRPR)
            {
                pState.decLayer();
            }
            // array
            else if (token.tType == TokenTypes::tLBR)
            {
                auto *arrayNode = ALLOC_AST_NODE(TokenTypes::tARRAY);
                arrayNode->addChild(curTermNode);
                pState.addStackTop(arrayNode);
            }
            else if (token.tType == TokenTypes::tRBR)
            {

            }

            pState.advance();
        }

        auto *stackTop = &(pState.getStackTop());
        stackTop->addChild(curTermNode);    

        // questionable
        while (isoperator(stackTop->tType))
        {
            auto *lastStackTop = stackTop;
            pState.popStackTop();
            stackTop = &(pState.getStackTop()); 
            stackTop->addChild(lastStackTop);
        }
    }

    void whileStateBeh(parserState &pState)
    {
        auto &whileToken = pState.getCurToken();
        createAstNode(pState, whileToken);

        auto &token = pState.advanceAndGet();
        if (token.tType != TokenTypes::tLPR)
        {
            // TODO: error
            return;
        }
        pState.advance();
        parseExpr(pState);
#ifdef DEBUG
        pState.fsmFinished = true;
#endif
    }

    void whileCloseStateBeh(parserState &pState)
    {
        
    }

    void exprStateBeh(parserState &pState)
    {
        
    }

    AstNode *buildAST(tokensVect &tokens, identifierVect &identifiers)
    {
        parserState pState(tokens, identifiers);
        astRoot = ALLOC_AST_NODE();
        pState.addStackTop(astRoot);

        std::stringstream debug_strm;
        while (!pState.fsmFinished)
        {
            debug_strm.str(std::string());
            switch (pState.fsmCurState)
            {
            case ParseFsmStates::sINIT:
                debug_strm << "sINIT hits\n";
                initStateBeh(pState);
                break;

            case ParseFsmStates::sSTATEMENT:
                debug_strm << "sSTATEMENT hits\n";
                statementStateBeh(pState);
                break;

            case ParseFsmStates::sWHILE:
                debug_strm << "sWHILE hits\n";
                whileStateBeh(pState);
                break;

            case ParseFsmStates::sWHILE_CLOSE:
                debug_strm << "sWHILE_CLOSE hits\n";
                whileCloseStateBeh(pState);
                break;
            
            case ParseFsmStates::sEXPR:
                debug_strm << "sEXPR hits\n";
                exprStateBeh(pState);
                break;
            }
#ifdef DEBUG
            std::cout << debug_strm.str();
#endif

        }

        return astRoot;
    }

#ifdef DEBUG
    void printAST()
    {
        std::cout << '\n';
        printAST(astRoot);
    }
    void printAST(AstNode *curRoot)
    {
        for (auto childNode : curRoot->nChildNodes)
        {
            printAST(childNode);
        }
        curRoot->print();
    }
#endif
};