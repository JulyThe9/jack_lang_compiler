#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <cassert>
#include <algorithm>

#include "Utils.h"
#include "JackCompilerTypes.h"
#include "ArenaAllocator.h"

// HELPER MACROS
#define ALLOC_AST_NODE new (aralloc.allocate()) AstNode

class Parser
{
private:
    // IMPORTANT: 200 node limit so far
    ArenaAllocator<AstNode> aralloc{ArenaAllocator<AstNode>(200)};
public:

    void initStateBeh(parserState &pState)
    {
        pState.astRoot = ALLOC_AST_NODE();
        pState.pendParentNodes.push(pState.astRoot);

        // TEMPORARY:
        pState.fsmCurState = ParseFsmStates::sSTATEMENT;
    }

    void statementStateBeh(parserState &pState)
    {
        // START FROM HERE
        auto token = tokens.at(pState.curTokenId);
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
    
        // checking if expr is finished
        while (true)
        {   
            auto &token = tokens.at(pState.curTokenId);

            // we hit the right bracket of while/if or the statement has ended with ;
            if ((pState.getLayer() == 0 && token.tType != TokenTypes::tRBR)
                || token.tType  == TokenTypes::tSEMICOLON)
            {
                break;
            }

            if (token.tType == TokenTypes::tNUMBER)
            {
                
            }
            else if (isopertator(token.tType))
            {

            }
            else if (token.tType == TokenTypes::tLBR)
            {
                pState.incLayer();
            }
            else if (token.tType == TokenTypes::tRBR)
            {
                pState.decLayer();
            }
            // identifiers for later
            //else if ()
        }
    }

    void whileStateBeh(parserState &pState)
    {
        TokenData *token = &(tokens[pState.curTokenId]);

        auto *astNodeWhile = ALLOC_AST_NODE(token->tType);
        pState.addStackTopChild(astNodeWhile);
        pState.addStackTop(astNodeWhile);

        pState.curTokenId++;
        token = &(tokens.at(pState.curTokenId));

        if (token->tType != TokenTypes::tLBR)
        {
            // TODO: error
            return;
        }

        parseExpr(pState);
    }

    void whileCloseStateBeh(parserState &pState)
    {
        
    }

    void exprStateBeh(parserState &pState)
    {
        
    }

    void buildAST(tokensVect &tokens, identifierVect &identifiers)
    {
        parserState pState(tokens, identifiers);

        while (!pState.fsmFinished)
        {
            switch (pState.fsmCurState)
            {
            case ParseFsmStates::sINIT:
                std::cout << "sINIT hits\n";
                initStateBeh(pState);
                break;

            case ParseFsmStates::sSTATEMENT:
                std::cout << "sSTATEMENT hits\n";
                statementStateBeh(pState);
                break;

            case ParseFsmStates::sWHILE:
                std::cout << "sWHILE hits\n";
                whileStateBeh(pState);
                break;

            case ParseFsmStates::sWHILE_CLOSE:
                std::cout << "sWHILE_CLOSE hits\n";
                whileCloseStateBeh(pState);
                break;
            
            case ParseFsmStates::sEXPR:
                std::cout << "sEXPR hits\n";
                exprStateBeh(pState);
                break;
            }
        }
    }
};