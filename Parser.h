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
                pState.curTokenId++;
                pState.fsmCurState = ParseFsmStates::sWHILE;
                break;
        }
    }

    void parseExpr(const tokensVect &tokens, 
        const identifierVect &identifiers, parserState &pState)
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

    void whileStateBeh(const tokensVect &tokens, 
        const identifierVect &identifiers, parserState &pState)
    {
        auto token = tokens.at(pState.curTokenId);
        if (token.tType != TokenTypes::tLBR)
        {
            // TODO: error
            return;
        }

        parseExpr(tokens, identifiers, pState);
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