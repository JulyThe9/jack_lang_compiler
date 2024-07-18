#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <cassert>
#include <algorithm>

#include "Utils.h"
#include "JackCompilerTypes.h"
//#include "ArenaAllocator.h"

class Parser
{
private:
public:

    void initStateBeh(const tokensVect &tokens, 
        const identifierVect &identifiers, parserState &pState)
    {
        //pState.astRoot = arAlloc.allocate();

        // TEMPORARY:
        pState.fsmCurState = ParseFsmStates::sSTATEMENT;
    }

    void statementStateBeh(const tokensVect &tokens, 
        const identifierVect &identifiers, parserState &pState)
    {
        auto token = tokens.at(pState.curTokenId);
        switch (token.tType)
        {
            case TokenTypes::tWHILE:
                pState.fsmCurState = ParseFsmStates::sWHILE;
                break;
        }
    }

    void whileStateBeh(const tokensVect &tokens, 
        const identifierVect &identifiers, parserState &pState)
    {
        
    }

    void whileCloseStateBeh(const tokensVect &tokens, 
        const identifierVect &identifiers, parserState &pState)
    {
        
    }

    void exprStateBeh(const tokensVect &tokens, 
        const identifierVect &identifiers, parserState &pState)
    {
        
    }

    void buildAST(tokensVect &tokens, identifierVect &identifiers)
    {
        parserState pState;

        while (!pState.fsmFinished)
        {
            switch (pState.fsmCurState)
            {
            case ParseFsmStates::sINIT:
                std::cout << "sINIT hits\n";
                initStateBeh(tokens, identifiers, pState);
                break;

            case ParseFsmStates::sSTATEMENT:
                std::cout << "sSTATEMENT hits\n";
                statementStateBeh(tokens, identifiers, pState);
                break;

            case ParseFsmStates::sWHILE:
                std::cout << "sWHILE hits\n";
                whileStateBeh(tokens, identifiers, pState);
                break;

            case ParseFsmStates::sWHILE_CLOSE:
                std::cout << "sWHILE_CLOSE hits\n";
                whileCloseStateBeh(tokens, identifiers, pState);
                break;
            
            case ParseFsmStates::sEXPR:
                std::cout << "sEXPR hits\n";
                exprStateBeh(tokens, identifiers, pState);
                break;
            }
        }
    }
};