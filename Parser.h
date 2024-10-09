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
    // IMPORTANT: 500 node limit so far
    ArenaAllocator<AstNode> aralloc{ArenaAllocator<AstNode>(MAX_EXPTECTED_AST_NODES)};
    AstNode* astRoot = NULL;

    inline AstNode *createStackTopNode(parserState &pState, TokenData &token)
    {
        AstNode *astNode = ALLOC_AST_NODE(token);
        pState.addStackTopChild(astNode);
        pState.addStackTop(astNode);
        return astNode;
    }
    inline AstNode *createStackTopNode(parserState &pState, AstNodeTypes aType, int aVal)
    {
        AstNode *astNode = ALLOC_AST_NODE(aType, aVal);
        pState.addStackTopChild(astNode);
        pState.addStackTop(astNode);
        return astNode;
    }

    void orderWhileLabels(AstNode *whileNode)
    {
        assert(whileNode->getNumOfChildren() == 5);
        assert(whileNode->aType == AstNodeTypes::aWHILE);

        auto *whStartNode = whileNode->nChildNodes[0];
        assert(whStartNode->aType == AstNodeTypes::aWHILE_START);

        auto *whJumpNode = whileNode->nChildNodes[2];
        assert(whJumpNode->aType == AstNodeTypes::aWHILE_JUMP);

        auto *whEndNode = whileNode->nChildNodes[4];
        assert(whEndNode->aType == AstNodeTypes::aWHILE_END);

        // hooking the labels to each other
        // making sure we jump to condition expr label (whStartNode label id)
        whEndNode->setNodeValue(whStartNode->getNodeValue());
        // making sure we name the while end label as the destionation of whJumpNode
        whileNode->setNodeValue(whJumpNode->getNodeValue());
    }

    void orderIfLables(AstNode *ifNode)
    {
        assert(ifNode->aType == AstNodeTypes::aIF);

        auto *ifJumpNode = ifNode->nChildNodes[1];
        assert(ifJumpNode->aType == AstNodeTypes::aIF_JUMP);

        // to jump outside of if
        ifNode->setNodeValue(ifJumpNode->getNodeValue());
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
        elseStartNode->setNodeValue(ifJumpNode->getNodeValue());
        // making sure the if block jump jumps to 
        // the lable after the else block
        elseNode->setNodeValue(elseJumpNode->getNodeValue());
    }
    bool parseFuncPars(parserState &pState)
    {
        auto *token = &(pState.getCurToken());
        assert(token->tType == TokenTypes::tLPR);
        assert(pState.getCurParseFunc() != NULL);

        bool success = false;
        while (true)
        {
            // parameter type
            token = &(pState.advanceAndGet());
            if (pState.getTokensFinished()) 
            {
                return pState.fsmTerminate(false);
            }

            // no arguments case
            if (token->tType == TokenTypes::tRPR)
            {
                success = true;
                break;
            }

            if (!isvartype(token->tType))
                return pState.fsmTerminate(false);
            LangDataTypes curParValType = tType_to_ldType(token->tType);
            if (curParValType == LangDataTypes::ldCLASS)
            {
                curParValType = pState.checkCreateUserDefinedDataType(*token);
            }

            // parameter name
            token = &(pState.advanceAndGet());
            if (pState.getTokensFinished())
            {
                return pState.fsmTerminate(false);
            }
            assert(token->tType == TokenTypes::tIDENTIFIER);
            assert(token->tVal.has_value());
            pState.addCurParseFuncPar(token->tVal.value(), curParValType);

            // comma or function decl closing bracket
            token = &(pState.advanceAndGet());
            if (pState.getTokensFinished())
            {
                return pState.fsmTerminate(false);
            }
            if (token->tType == TokenTypes::tRPR)
            {
                success = true;
                break;
            }
            else if (token->tType != TokenTypes::tCOMMA)
                return pState.fsmTerminate(false);
        }

        return 
            success ? true : pState.fsmTerminate(false);
    }

public:

    bool initStateBeh(parserState &pState)
    {
        auto *token = &(pState.getCurToken());
        while (token->tType != TokenTypes::tCLASS)
        {
            token = &(pState.advanceAndGet());
            if (pState.getTokensFinished())
                return pState.fsmTerminate(false);
        }

        // token is class at this point
        // advancing to name
        token = &(pState.advanceAndGet());
        if (pState.getTokensFinished())
            return pState.fsmTerminate(false);

        assert(token->tType == TokenTypes::tIDENTIFIER);
        assert(token->tVal.has_value());
        auto classNameID = token->tVal.value();
        auto [classExists, idx] = pState.containsClass(classNameID);
        if (classExists)
        {
            pState.setCurParseClass(idx);
        }
        else 
        {
            const bool isDefined = true;
            pState.addClass(classNameID, isDefined);
        }

        auto *classNode = createStackTopNode(pState, AstNodeTypes::aCLASS, pState.getCurParseClass()->getID());

        // skipping the {
        pState.advanceAndGet(2);
        if (pState.getTokensFinished())
            return pState.fsmTerminate(false);

        pState.fsmCurState = ParseFsmStates::sCLASS_DECIDE;
        return true;
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
            case TokenTypes::tDO:
                pState.fsmCurState = ParseFsmStates::sFUNC_DO_CALL;
                break;
            case TokenTypes::tLET:
                pState.fsmCurState = ParseFsmStates::sVAR_ASSIGN;
                break;
            case TokenTypes::tRETURN:
                pState.fsmCurState = ParseFsmStates::sRETURN;
                break;
            default:
#ifdef ERR_DEBUG
                std::cerr << "ERR: UNKNOWN sSTATEMENT_DECIDE outgoing state: " << (unsigned int)token.tType << '\n';
#endif
                pState.fsmTerminate(false);
                break;
        }     
    }

    void classDecideStateBeh(parserState &pState)
    {
        auto &token = pState.getCurToken();
        switch (token.tType)
        {
            case TokenTypes::tFIELD:
                pState.fsmCurState = ParseFsmStates::sFIELD_DECL;
                break;
            case TokenTypes::tSTATIC:
                pState.fsmCurState = ParseFsmStates::sSTATIC_DECL;
                break;
            case TokenTypes::tCONSTRUCTOR:
                pState.fsmCurState = ParseFsmStates::sCTOR_DEF;
                break;
            case TokenTypes::tFUNCTION:
                pState.fsmCurState = ParseFsmStates::sFUNC_DEF;
                break;
            case TokenTypes::tRCURL:
                pState.fsmCurState = ParseFsmStates::sBLOCK_CLOSE;
                break;
        }
    }

    // !isStatic -> isField
    bool fieldAndStaticStateBeh(parserState &pState, bool isStatic)
    {
        auto &declToken = pState.getCurToken();
        pState.addStackTopChild(ALLOC_AST_NODE(declToken));

        // current token is tVAR
        auto &valTypeToken = pState.advanceAndGet();
        if (pState.getTokensFinished())
            return pState.fsmTerminate(false);

        if (!isvartype(valTypeToken.tType))
            return pState.fsmTerminate(false);

        bool declsFinished = false;
        while (!declsFinished)
        {
            auto &varToken = pState.advanceAndGet();
            if (pState.getTokensFinished())
                return pState.fsmTerminate(false);

            unsigned int nameID = varToken.tVal.value();
            if (std::get<0>(pState.containsField(varToken.tVal.value())) ||
                std::get<0>(pState.containsStatic(varToken.tVal.value())))
            {
#ifdef ERR_DEBUG
                assert (nameID < pState.getIdent()->size());
                std::cerr << "ERR: VARIABLE REDECLARATION: " << pState.getIdent()->at(nameID) << '\n';
#endif
                // TODO: error: variable redeclaration
            }
            else
            {
                if (isStatic)
                    pState.addCurParseClassStaticVar(nameID, tType_to_ldType(valTypeToken.tType));
                else
                    pState.addCurParseClassFieldVar(nameID, tType_to_ldType(valTypeToken.tType));
            }

            auto &token = pState.advanceAndGet();
            if (pState.getTokensFinished())
                return pState.fsmTerminate(false);

            if (token.tType == TokenTypes::tSEMICOLON)
                declsFinished = true;
        }

        pState.advance();
        pState.fsmCurState = ParseFsmStates::sCLASS_DECIDE;
        return true;
    }

    bool ctorDefStateBeh(parserState &pState)
    {
        // token is constructor at this point
        // advancing to return type
        auto *token = &(pState.advanceAndGet());
        if (pState.getTokensFinished())
            return pState.fsmTerminate(false);

        if (!isvartype(token->tType))
            return pState.fsmTerminate(false);

        if (tType_to_ldType(token->tType) != LangDataTypes::ldCLASS)
        {
            // TODO: error: ctor invalid ret type
            return pState.fsmTerminate(false);
        }
        const bool onlyCheck = true;
        const LangDataTypes ctorRetType = pState.checkCreateUserDefinedDataType(*token, onlyCheck);
        // if it's unknown then it's definitely not the class that the ctor belongs to,
        // otherwise it would have been known (class def comes before ctor def)
        if (ctorRetType == LangDataTypes::ldUNKNOWN)
        {
            // TODO: error: ctor invalid ret type
            return pState.fsmTerminate(false);
        }

        if (ctorRetType 
                != classID_to_ldType(pState.getCurParseClass()->getID()))
        {
            // TODO: error: ctor invalid ret type
            return pState.fsmTerminate(false);
        }

        // advancing to name
        token = &(pState.advanceAndGet());
        if (pState.getTokensFinished())
            return pState.fsmTerminate(false);

        assert(token->tType == TokenTypes::tIDENTIFIER);
        assert(token->tVal.has_value());

        const bool isCtor = true;
        if (!pState.addCurParseClassFunc(token->tVal.value(), ctorRetType, isCtor))
        {
            // TODO: error: ctor already defined
            return pState.fsmTerminate(false);
        }
        
        // advancing to (
        pState.advance();
        parseFuncPars(pState);

        // skipping the {
        if (!pState.advance(2))
            return pState.fsmTerminate(false);

        const auto &ctorFunc = *(pState.getCurParseFunc());
        auto *ctorNode = createStackTopNode(pState, AstNodeTypes::aFUNCTION, ctorFunc.getID());

        ctorNode->addChild(ALLOC_AST_NODE(AstNodeTypes::aFUNC_DEF, ctorFunc.nameID));
        ctorNode->addChild(ALLOC_AST_NODE(AstNodeTypes::aFUNC_LOCNUM, 0));

        // memory commands
        ctorNode->addChild(ALLOC_AST_NODE(AstNodeTypes::aCTOR_ALLOC,
            pState.getCurParseFunc()->getNumOfPars()));

        ctorNode->addChild(ALLOC_AST_NODE(AstNodeTypes::aSTATEMENTS));
        pState.addStackTop(ctorNode->nChildNodes.back());

        ctorNode->addChild(ALLOC_AST_NODE(AstNodeTypes::aFUNC_RET_VAL));

        pState.fsmCurState = ParseFsmStates::sSTATEMENT_DECIDE;
        return true;
    }

    bool funcDefStateBeh(parserState &pState)
    {
        // token is function at this point
        // advancing to return type
        auto *token = &(pState.advanceAndGet());
        if (pState.getTokensFinished())
            return pState.fsmTerminate(false);

        if (!isvartype(token->tType))
            return pState.fsmTerminate(false);
        LangDataTypes ldType_ret = tType_to_ldType(token->tType);
        if (ldType_ret == LangDataTypes::ldCLASS)
        {
            ldType_ret = pState.checkCreateUserDefinedDataType(*token);
        }

        // advancing to name
        token = &(pState.advanceAndGet());
        if (pState.getTokensFinished())
            return pState.fsmTerminate(false);

        assert(token->tType == TokenTypes::tIDENTIFIER);
        assert(token->tVal.has_value());

        pState.addCurParseClassFunc(token->tVal.value(), ldType_ret);
        // advancing to (
        pState.advance();
        parseFuncPars(pState);

        // skipping the {
        if (!pState.advance(2))
            return pState.fsmTerminate(false);

        const auto &curParseFunc = *(pState.getCurParseFunc());
        auto *funcNode = createStackTopNode(pState, AstNodeTypes::aFUNCTION, curParseFunc.getID());

        funcNode->addChild(ALLOC_AST_NODE(AstNodeTypes::aFUNC_DEF, curParseFunc.nameID));
        funcNode->addChild(ALLOC_AST_NODE(AstNodeTypes::aFUNC_LOCNUM, 0));

        funcNode->addChild(ALLOC_AST_NODE(AstNodeTypes::aSTATEMENTS));
        pState.addStackTop(funcNode->nChildNodes.back());

        if (curParseFunc.ldType_ret != LangDataTypes::ldVOID)
        {
            funcNode->addChild(ALLOC_AST_NODE(AstNodeTypes::aFUNC_RET_VAL));
        }

        pState.fsmCurState = ParseFsmStates::sSTATEMENT_DECIDE;
        return true;
    }

    bool returnStateBeh(parserState &pState)
    {
        auto *token = &(pState.advanceAndGet());
        if (pState.getTokensFinished())
            return pState.fsmTerminate(false);

        if (token->tType == TokenTypes::tSEMICOLON)
        {
            pState.addStackTopChild(ALLOC_AST_NODE(AstNodeTypes::aNUMBER, 0));
            pState.advance();
            pState.fsmCurState = ParseFsmStates::sSTATEMENT_DECIDE;
            return true;
        }

        parseExpr(pState);
        pState.advance();
        pState.fsmCurState = ParseFsmStates::sSTATEMENT_DECIDE;
        return true;
    }

    bool funcDoCallStateBeh(parserState &pState)
    {
        auto &doToken = pState.getCurToken();
        createStackTopNode(pState, doToken);
        // current token is tDO
        auto &funcToken = pState.advanceAndGet();
        if (pState.getTokensFinished())
            return pState.fsmTerminate(false);

        const bool res = parseFuncCall(pState);
        pState.popStackTop();
        pState.fsmCurState = ParseFsmStates::sSTATEMENT_DECIDE;
        
        return res;
    }

    bool parseFuncCall(parserState &pState)
    {
        auto &funcToken = pState.getCurToken();
        assert (funcToken.tType == TokenTypes::tIDENTIFIER);
        const unsigned int nameID = funcToken.tVal.value();

        if (!pState.advance())
            return pState.fsmTerminate(false);

        if (pState.getCurToken().tType != TokenTypes::tLPR)
        {
            // TODO: error: syntax
            return false;
        }

        while (true)
        {
            if (!pState.advance())
                return pState.fsmTerminate(false);

            parseExpr(pState);
            // here - stack reparenting?

            auto token = pState.getCurToken();
            if (token.tType == TokenTypes::tRPR)
            {
                break;
            }
            else if (token.tType != TokenTypes::tCOMMA)
                return pState.fsmTerminate(false);
        }


        // checking that we designed the processing correctly
        // hence assert
        assert(pState.getCurToken().tType == TokenTypes::tRPR);
        auto &curToken = pState.advanceAndGet();
        if (pState.getTokensFinished())
            return pState.fsmTerminate(false);
        
        if (curToken.tType != TokenTypes::tSEMICOLON)
        {
#ifdef ERR_DEBUG
                std::cerr << "ERR: MISSING SEMICOLON, LINE: " << pState.getCurLineNum() << '\n';
#endif
                // TODO: error: missing semicolon
            return pState.fsmTerminate(false);
        }
        pState.advance();

        auto *stackTop = pState.getStackTop();
        assert(stackTop->aType == AstNodeTypes::aDO);
        const unsigned int numArgs = stackTop->getNumOfChildren();

        pState.addStackTopChild(ALLOC_AST_NODE(AstNodeTypes::aFUNC_CALL, nameID));
        pState.addStackTopChild(ALLOC_AST_NODE(AstNodeTypes::aFUNC_ARGNUM, numArgs));

        return true;
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
                assert(token.tVal.has_value());

                auto [varScope, idx] = pState.findVariable(token.tVal.value());
                if (varScope != VarScopes::scUNKNOWN)
                {
                    curTermNode = ALLOC_AST_NODE(varScopeToAccessType(varScope), idx);
                }
                else
                {
                    auto [contains, idx] = pState.findFunction(token.tVal.value());
                    if (contains)
                    {
                        // legowelt
                        // implement funcCallStateBeh first
                    }
                    else
                    {
                        // check object
                    }
                }
            }
            else if (isexprkeyword(token.tType))
            {
                if (token.tType == TokenTypes::tTHIS)
                    curTermNode = ALLOC_AST_NODE(AstNodeTypes::aTHIS_READ);
                else if (token.tType == TokenTypes::tTHAT)
                    curTermNode = ALLOC_AST_NODE(AstNodeTypes::aTHAT_READ);
                else if (token.tType == TokenTypes::tTRUE)
                    curTermNode = ALLOC_AST_NODE(AstNodeTypes::aNUMBER, 1);
                else if (token.tType == TokenTypes::tFALSE || token.tType == TokenTypes::tNULL)
                    curTermNode = ALLOC_AST_NODE(AstNodeTypes::aNUMBER, 0);
            }
            else if (isbinaryperator(token.tType))
            {
                // stack top (potentially operator)
                auto *stackTop = pState.getStackTop();
                assert(stackTop != NULL);
                // current token (operator) as AST node
                // commin for the following two cases and needed for greaterPreced,\
                // so carried placed here
                auto *operNode = ALLOC_AST_NODE(token, pState.getLayer());

                // start of the expression, parent is while, for or general stms
                if (!isoperator(aType_to_tType(stackTop->aType)) || greaterPreced(*operNode, *stackTop))
                {       
                    operNode->addChild(curTermNode);
                    pState.addStackTop(operNode);      
                    pState.advance();
                    continue;
                }

                // We need to go up in stack until
                // we find an operator that is not of greater
                // preced than that of operNode.
                // See: 8 - 1 * (5+7) - 2
                // vs 8 - (1 * (5+7) - 2)
                do
                {
                    stackTop->addChild(curTermNode);
                    curTermNode = stackTop;
                    pState.popStackTop();
                    stackTop = pState.getStackTop();
                    if (stackTop == NULL 
                        || !isoperator(aType_to_tType(stackTop->aType)))
                    {
                        break;
                    }
                }
                while (!greaterPreced(*operNode, *stackTop));
                operNode->addChild(curTermNode);
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
                    stackTop = pState.getStackTop();
                    assert(stackTop != NULL);
                    stackTop->addChild(curTermNode);
                    curTermNode = stackTop;
                    pState.popStackTop();
                }
                while (stackTop->aType != AstNodeTypes::aARRAY);
                assert(stackTop->aType == AstNodeTypes::aARRAY);
                curTermNode = stackTop;
            }
            else if (token.tType == TokenTypes::tCOMMA)
            {
                break;
            }
            else if (token.tType == TokenTypes::tNEG_MINUS)
            {
                pState.addStackTop(ALLOC_AST_NODE(token, pState.getLayer()));
            }

            pState.advance();
        }

        auto *stackTop = pState.getStackTop();
        assert(stackTop != NULL);
        stackTop->addChild(curTermNode);    

        // either create isexprconnectornode (for array, func call, etc.)
        // or exclude tSTATEMENTS from isconnectornode
        while (isoperator(aType_to_tType(stackTop->aType)) || stackTop->aType == AstNodeTypes::aARRAY)
        {
            auto *lastStackTop = stackTop;
            pState.popStackTop();
            stackTop = pState.getStackTop();
            assert(stackTop != NULL);
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
        whileStartNode->setNodeValue(getLabelId());
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
        whileNode->nChildNodes.back()->setNodeValue(getLabelId());
        
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
        ifNode->nChildNodes.back()->setNodeValue(getLabelId());

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
        elseNode->nChildNodes.back()->setNodeValue(getLabelId());

        elseNode->addChild(ALLOC_AST_NODE(AstNodeTypes::aELSE_START));
        // getNodeValue() will be set in orderElseLabels

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
            auto *stackTop = pState.getStackTop();
            if (stackTop == NULL)
            {
                return;
            }
            while (!isblockstart(stackTop->aType))
            {
                pState.popStackTop();
                stackTop = pState.getStackTop();
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
            auto *ifNode = pState.getStackTop();

            assert(ifNode->aType == AstNodeTypes::aIF);
            // else node will do the label generation now
            ifNode->generatesCode = false;

            pState.fsmCurState = ParseFsmStates::sELSE;

            return;
        }

        popUntilBlockParent();
        // on to the next statement parsing
        if (pState.getStackTop()->aType == AstNodeTypes::aCLASS)
        {
            pState.fsmCurState = ParseFsmStates::sCLASS_DECIDE;
        }
        else if (pState.getStackTop()->aType == AstNodeTypes::aFUNCTION)
        {
            // TODO: why never reached?
            pState.fsmCurState = ParseFsmStates::sSTATEMENT_DECIDE;
        }
        else
            pState.fsmCurState = ParseFsmStates::sSTATEMENT_DECIDE;
    }

    bool varDeclStateBeh(parserState &pState)
    {
        auto &varDeclToken = pState.getCurToken();
        pState.addStackTopChild(ALLOC_AST_NODE(varDeclToken));
        pState.declaringLocals = true;

        // current token is tVAR
        auto &valTypeToken = pState.advanceAndGet();
        if (pState.getTokensFinished())
            return pState.fsmTerminate(false);

        if (!isvartype(valTypeToken.tType))
            return pState.fsmTerminate(false);

        bool declsFinished = false;
        while (!declsFinished)
        {
            auto &varToken = pState.advanceAndGet();
            if (pState.getTokensFinished())
                return pState.fsmTerminate(false);

            // id is index (actual vector index) in pState.identifiers;
            // can be used to look-up the actual string
            unsigned int nameID = varToken.tVal.value();
            if (std::get<0>(pState.containsLocal(varToken.tVal.value())) || 
                std::get<0>(pState.containsArg(varToken.tVal.value())))
            {
#ifdef ERR_DEBUG
                assert (nameID < pState.getIdent()->size());
                std::cerr << "ERR: VARIABLE REDECLARATION: " << pState.getIdent()->at(nameID) << '\n';
#endif
                // TODO: error: variable redeclaration
            }
            else
            {
                pState.addLocalScopeFramesTopVar(nameID, tType_to_ldType(valTypeToken.tType));
            }

            auto &token = pState.advanceAndGet();
            if (pState.getTokensFinished())
                return pState.fsmTerminate(false);

            if (token.tType == TokenTypes::tSEMICOLON)
                declsFinished = true;
        }

        pState.advance();
        pState.fsmCurState = ParseFsmStates::sSTATEMENT_DECIDE;
        return true;
    }

    bool varAssignStateBeh(parserState &pState)
    {
        auto &varAssignToken = pState.getCurToken();
        createStackTopNode(pState, varAssignToken);
        // current token is tLET
        auto &varToken = pState.advanceAndGet();

        // NOTE: for details see varDeclStateBeh
        if (pState.getTokensFinished())
            return pState.fsmTerminate(false);

        assert (varToken.tType == TokenTypes::tIDENTIFIER);
        const unsigned int nameID = varToken.tVal.value();

        // skipping =
        pState.advance(2);
        if (pState.getTokensFinished())
            return pState.fsmTerminate(false);

        parseExpr(pState);
        // making sure parseExpr didnt mess up stacl top:
        // we need it to be aLET (parent of expr and LOCAL_WRITE/ARG_WRITE/etc.)
        assert(pState.getStackTop()->aType == AstNodeTypes::aLET);

        auto [varScope, idx] = pState.findVariable(nameID);
        if (varScope != VarScopes::scUNKNOWN)
        {   
            const bool isWriting = true;
            pState.addStackTopChild(ALLOC_AST_NODE(varScopeToAccessType(varScope, isWriting), idx));
            
            pState.advance();
            pState.fsmCurState = ParseFsmStates::sSTATEMENT_DECIDE;
            return true;
        }

        // NOTE: cannot be a class obj name, because class members
        // are only set through setters
        std::cerr << "ERR: UNKNOWN VARIABLE NAME: " << pState.getIdent()->at(nameID) << '\n';
        // TODO: error: unknown variable name
        return pState.fsmTerminate(false);
    }

    AstNode *buildAST(tokensVect &tokens, identifierVect &identifiers)
    {
        parserState pState(tokens, identifiers);
        astRoot = ALLOC_AST_NODE();
        pState.addStackTop(astRoot);

        std::stringstream debug_strm;
        bool ignore = false;
        while (!pState.getFsmFinished())
        {
            debug_strm.str(std::string());
            
            // means we have just finished declaring local variables
            // for current parse func
            if (pState.fsmCurState != ParseFsmStates::sVAR_DECL &&
                pState.declaringLocals == true)
            {
                auto *funcRootNode = getNearestFuncRootNode(pState.getStackTop());
                assert (funcRootNode != NULL);
                auto *funcLocNumNode = getFuncLocNumNode(funcRootNode);
                assert (funcLocNumNode != NULL);
                funcLocNumNode->overwriteNodeValue(pState.getCurParseFunc()->getNumOfLocals());

                pState.declaringLocals = false;
            }

            switch (pState.fsmCurState)
            {
            case ParseFsmStates::sINIT:
                debug_strm << "sINIT hits\n";
                initStateBeh(pState);
                break;

            case ParseFsmStates::sSTATEMENT_DECIDE:
                ignore = true;
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

            case ParseFsmStates::sVAR_ASSIGN:
                debug_strm << "sVAR_ASSIGN hits\n";
                varAssignStateBeh(pState);
                break;

            case ParseFsmStates::sCLASS_DECIDE:
                debug_strm << "sCLASS_DECIDE hits\n";
                classDecideStateBeh(pState);
                break;

            case ParseFsmStates::sFIELD_DECL:
            {
                const bool isStatic = false;
                debug_strm << "sFIELD_DECL hits\n";
                fieldAndStaticStateBeh(pState, isStatic);
            }
                break;

            case ParseFsmStates::sSTATIC_DECL:
            {
                const bool isStatic = true;
                debug_strm << "sSTATIC_DECL hits\n";
                fieldAndStaticStateBeh(pState, isStatic);
            }
                break;

            case ParseFsmStates::sCTOR_DEF:
                debug_strm << "sCTOR_DEF hits\n";
                ctorDefStateBeh(pState);
                break;

            case ParseFsmStates::sFUNC_DEF:
                debug_strm << "sFUNC_DEF hits\n";
                funcDefStateBeh(pState);
                break;

            case ParseFsmStates::sRETURN:
                debug_strm << "sRETURN hits\n";
                returnStateBeh(pState);
                break;

            case ParseFsmStates::sFUNC_DO_CALL:
                debug_strm << "sFUNC_DO_CALL hits\n";
                funcDoCallStateBeh(pState);
                break;

            }
#ifdef DEBUG
            if (!ignore)
                std::cout << debug_strm.str();
            ignore = false;
#endif
        }

        return astRoot;
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