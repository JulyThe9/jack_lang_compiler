#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <cassert>
#include <algorithm>

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
    AstNode* astRoot = NULL;

    inline AstNode *createStackTopNode(parserState &pState, TokenData &token)
    {
        AstNode *astNode = ALLOC_AST_NODE(token);
        pState.addStackTopChild(astNode);
        pState.addStackTop(astNode);
        return astNode;
    }

    void orderWhileLabels(AstNode *whileNode)
    {
        assert(whileNode->nChildNodes.size() == 5);
        assert(whileNode->aType == AstNodeTypes::aWHILE);

        auto *whStartNode = whileNode->nChildNodes[0];
        assert(whStartNode->aType == AstNodeTypes::aWHILE_START);

        auto *whJumpNode = whileNode->nChildNodes[2];
        assert(whJumpNode->aType == AstNodeTypes::aWHILE_JUMP);

        auto *whEndNode = whileNode->nChildNodes[4];
        assert(whEndNode->aType == AstNodeTypes::aWHILE_END);

        // hooking the labels to each other
        // making sure we jump to condition expr label (whStartNode label id)
        whEndNode->setNValue(whStartNode->nValue);
        // making sure we name the while end label as the destionation of whJumpNode
        whileNode->setNValue(whJumpNode->nValue);
    }

    void orderIfLables(AstNode *ifNode)
    {
        assert(ifNode->aType == AstNodeTypes::aIF);

        auto *ifJumpNode = ifNode->nChildNodes[1];
        assert(ifJumpNode->aType == AstNodeTypes::aIF_JUMP);

        // to jump outside of if
        ifNode->setNValue(ifJumpNode->nValue);
    }
    void orderElseLabels(AstNode *elseNode)
    {
        assert(elseNode->aType == AstNodeTypes::aELSE);

        // jumping to else end
        auto *elseJumpNode = elseNode->nChildNodes[0];
        assert(elseJumpNode->aType == AstNodeTypes::aELSE_JUMP);

        auto *elseStartNode = elseNode->nChildNodes[1];
        assert(elseStartNode->aType == AstNodeTypes::aELSE_START);

        auto *ifJumpNode = getIfBlockJump(elseStartNode);
        assert(ifJumpNode != NULL);

        // making sure that in if-else block we jump to else start
        // in case of false condition
        elseStartNode->setNValue(ifJumpNode->nValue);
        // making sure the if block jump jumps to 
        // the lable after the else block
        elseNode->setNValue(elseJumpNode->nValue);
    }

public:

    void initStateBeh(parserState &pState)
    {
        // TEMPORARY:
        pState.fsmCurState = ParseFsmStates::sSTATEMENT_DECIDE;
    }

    void statementDecideStateBeh(parserState &pState)
    {
        auto &token = pState.getCurToken();
        switch (token.tType)
        {
            case TokenTypes::tWHILE:
                pState.fsmCurState = ParseFsmStates::sWHILE;
                break;
            case TokenTypes::tIF:
                pState.fsmCurState = ParseFsmStates::sIF;
                break;
            case TokenTypes::tRCURL:
                pState.fsmCurState = ParseFsmStates::sBLOCK_CLOSE;
                break;
            case TokenTypes::tVAR:
                pState.fsmCurState = ParseFsmStates::sVAR_DECL;
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
                if (!isoperator(aType_to_tType(stackTop.aType)) || greaterPreced(*operNode, stackTop))
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
                auto *arrayNode = ALLOC_AST_NODE(AstNodeTypes::aARRAY);
                arrayNode->addChild(curTermNode);
                pState.addStackTop(arrayNode);
            }
            else if (token.tType == TokenTypes::tRBR)
            {
                AstNode *stackTop = NULL;
                do 
                {
                    stackTop = &(pState.getStackTop());
                    stackTop->addChild(curTermNode);
                    curTermNode = stackTop;
                    pState.popStackTop();
                }
                while (stackTop->aType != AstNodeTypes::aARRAY);
                assert(stackTop->aType == AstNodeTypes::aARRAY);
                curTermNode = stackTop;
            }

            pState.advance();
        }

        auto *stackTop = &(pState.getStackTop());
        stackTop->addChild(curTermNode);    

        // either create isexprconnectornode (for array, func call, etc.)
        // or exclude tSTATEMENTS from isconnectornode
        while (isoperator(aType_to_tType(stackTop->aType)) || stackTop->aType == AstNodeTypes::aARRAY)
        {
            auto *lastStackTop = stackTop;
            pState.popStackTop();
            stackTop = &(pState.getStackTop()); 
            stackTop->addChild(lastStackTop);
        }

        pState.resetLayer();
    }

    bool whileStateBeh(parserState &pState)
    {
        auto &whileToken = pState.getCurToken();
        auto *whileNode = createStackTopNode(pState, whileToken);

        // add WHILE_START node
        // we will jump to the lable generated by this node
        // in the resulting code
        auto *whileStartNode = ALLOC_AST_NODE(AstNodeTypes::aWHILE_START);
        whileStartNode->setNValue(getLabelId());
        whileNode->addChild(whileStartNode);

        auto &token = pState.advanceAndGet();
        if (pState.getTokensFinished())
            return pState.fsmTerminate(false);

        if (token.tType != TokenTypes::tLPR)
            return false; // TODO: error

        if (!pState.advance())
            return pState.fsmTerminate(false);

        parseExpr(pState);

        whileNode->addChild(ALLOC_AST_NODE(AstNodeTypes::aWHILE_JUMP));
        whileNode->nChildNodes.back()->setNValue(getLabelId());
        
        whileNode->addChild(ALLOC_AST_NODE(AstNodeTypes::aSTATEMENTS));

        // further stuff goes to STATEMENTS node under current while
        // (until the corresponding })
        pState.addStackTop(whileNode->nChildNodes.back());
        whileNode->addChild(ALLOC_AST_NODE(AstNodeTypes::aWHILE_END));

        orderWhileLabels(whileNode);

        auto &lcurltoken = pState.advanceAndGet();
        if (pState.getTokensFinished())
            return pState.fsmTerminate(false);

        if (lcurltoken.tType == TokenTypes::tLCURL)
        {
            if (!pState.advance())
                return pState.fsmTerminate(false);
        }
        // else error
        pState.fsmCurState = ParseFsmStates::sSTATEMENT_DECIDE;
        return true;
    }

    bool ifStateBeh(parserState &pState)
    {
        auto &ifToken = pState.getCurToken();
        auto *ifNode = createStackTopNode(pState, ifToken);

        auto &token = pState.advanceAndGet();
        if (pState.getTokensFinished())
            return pState.fsmTerminate(false);

        if (token.tType != TokenTypes::tLPR)
            return false; // TODO: error

        if (!pState.advance())
            return pState.fsmTerminate(false);

        parseExpr(pState);

        ifNode->addChild(ALLOC_AST_NODE(AstNodeTypes::aIF_JUMP));
        ifNode->nChildNodes.back()->setNValue(getLabelId());

        ifNode->addChild(ALLOC_AST_NODE(AstNodeTypes::aSTATEMENTS));
        pState.addStackTop(ifNode->nChildNodes.back());
        
        auto &lcurltoken = pState.advanceAndGet();
        if (pState.getTokensFinished())
            return pState.fsmTerminate(false);
            
        if (lcurltoken.tType == TokenTypes::tLCURL)
        {
            if (!pState.advance())
                return pState.fsmTerminate(false);
        }

        orderIfLables(ifNode);

        // else error
        pState.fsmCurState = ParseFsmStates::sSTATEMENT_DECIDE;
        return true;
    }

    bool elseStateBeh(parserState &pState)
    {
        auto &elseToken = pState.getCurToken();
        auto *elseNode = createStackTopNode(pState, elseToken);

        elseNode->addChild(ALLOC_AST_NODE(AstNodeTypes::aELSE_JUMP));
        elseNode->nChildNodes.back()->setNValue(getLabelId());

        elseNode->addChild(ALLOC_AST_NODE(AstNodeTypes::aELSE_START));
        // nValue will be set in orderElseLabels

        elseNode->addChild(ALLOC_AST_NODE(AstNodeTypes::aSTATEMENTS));

        pState.addStackTop(elseNode->nChildNodes.back());

        auto &lcurltoken = pState.advanceAndGet();
        if (pState.getTokensFinished())
            return pState.fsmTerminate(false);
            
        if (lcurltoken.tType == TokenTypes::tLCURL)
        {
            if (!pState.advance())
                return pState.fsmTerminate(false);
        }

        orderElseLabels(elseNode);

        // else error
        pState.fsmCurState = ParseFsmStates::sSTATEMENT_DECIDE;
        return true;
    }

    void blockCloseStateBeh(parserState &pState)
    {
        // we closed a block, so we need to pop the stack
        // until the block's parent is at the top
        // so that we can add [to its children]
        // another statement (same tree depth, a sibling of
        // the current block)
        auto popUntilBlockStart = [&] ()
        {
            auto *stackTop = &(pState.getStackTop());
            while (!isblockstart(stackTop->aType))
            {
                pState.popStackTop();
                stackTop = &(pState.getStackTop());
                if (!stackTop)
                    break;
            }
        };
        auto popUntilBlockParent = [&] ()
        {
            popUntilBlockStart();
            // popping the actual block start
            pState.popStackTop();
        };

        // triggered on } toke in Jack language
        // not adding any children
        // because all the "post" actions are done
        // in the corresponding while/if block
        auto &token = pState.advanceAndGet();

        // last } case in the program will be
        // covered here
        if (pState.getTokensFinished())
        {
            popUntilBlockParent();
            pState.fsmFinished = true;
            pState.fsmFinishedCorrectly = true;
            return;
        }
        if (token.tType == TokenTypes::tELSE)
        {
            popUntilBlockStart();
            auto *ifNode = &(pState.getStackTop());
            assert(ifNode->aType == AstNodeTypes::aIF);

            // else node will do the label generation now
            ifNode->generatesCode = false;

            pState.fsmCurState = ParseFsmStates::sELSE;

            return;
        }

        popUntilBlockParent();
        // on to the next statement parsing
        pState.fsmCurState = ParseFsmStates::sSTATEMENT_DECIDE;
    }

    bool varDeclStateBeh(parserState &pState)
    {
        assert(pState.scopeFrames.size() > 0);
        // current token is tVAR
        auto &valTypeToken = pState.advanceAndGet();
        if (pState.getTokensFinished())
            return pState.fsmTerminate(false);

        if (!isvartype(valTypeToken.tType))
            return pState.fsmTerminate(false);

        auto &scopeFramesTop = pState.getScopeFramesTop();

        bool declsFinished = false;
        while (!declsFinished)
        {
            auto &varToken = pState.advanceAndGet();
            if (pState.getTokensFinished())
                return pState.fsmTerminate(false);

            // id is index (actual vector index) in pState.identifiers;
            // can be used to look-up the actual string
            unsigned int nameID = varToken.tVal.value();
            if (scopeFramesTop.containsLocal(varToken.tVal.value()).first || 
                scopeFramesTop.containsArg(varToken.tVal.value()).first)
            {
                // error, variable redeclaration
            }
            else
            {
                pState.addScopeFramesTopVar(nameID, ScopeTypes::ST_LOCAL, valTypeToken.tType);
            }

            auto &token = pState.advanceAndGet();
            if (pState.getTokensFinished())
                return pState.fsmTerminate(false);

            if (token.tType == TokenTypes::tSEMICOLON)
                declsFinished = true;
        }

        return true;
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

            case ParseFsmStates::sSTATEMENT_DECIDE:
                debug_strm << "sSTATEMENT_DECIDE hits\n";
                statementDecideStateBeh(pState);
                break;

            case ParseFsmStates::sWHILE:
                debug_strm << "sWHILE hits\n";
                whileStateBeh(pState);
                break;

            case ParseFsmStates::sIF:
                debug_strm << "sIF hits\n";
                ifStateBeh(pState);
                break;

            case ParseFsmStates::sELSE:
                debug_strm << "sELSE hits\n";
                elseStateBeh(pState);
                break;

            case ParseFsmStates::sBLOCK_CLOSE:
                debug_strm << "sBLOCK_CLOSE hits\n";
                blockCloseStateBeh(pState);
                break;

            case ParseFsmStates::sVAR_DECL:
                debug_strm << "sVAR_DECL hits\n";
                varDeclStateBeh(pState);
                break;
            
            // case ParseFsmStates::sEXPR:
            //     debug_strm << "sEXPR hits\n";
            //     exprStateBeh(pState);
            //     break;
            }
#ifdef DEBUG
            std::cout << debug_strm.str();
#endif
        }

        return astRoot;
    }

    void printAST()
    {
        if (astRoot == NULL)
            return;
        std::cout << '\n';
        printAST(astRoot);
    }
    void printAST(AstNode *curRoot)
    {
#ifndef DEBUG       // RELEASE
        // post-order
        for (auto childNode : curRoot->nChildNodes)
        {
            printAST(childNode);
        }
        curRoot->print();
#else
        // pre-order
        curRoot->print();
        for (auto childNode : curRoot->nChildNodes)
        {
            printAST(childNode);
        }
#endif
    }
};