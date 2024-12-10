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
    // NOTE: IMPORTANT: 500 node limit so far
    parserState pState;

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
    inline AstNode *createStackTopNode(parserState &pState, AstNodeTypes aType)
    {
        AstNode *astNode = ALLOC_AST_NODE(aType);
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

    void loadSysLibSymbols(unsigned int arrayLib_className_id, unsigned int newName_id)
    {
        pState.arrayLib_classID = pState.addClass(arrayLib_className_id, false);
        // see ctorDefStateBeh for details
        const bool isMethod = false;
        const bool isCtor = true;
        pState.addFuncToClass(pState.arrayLib_classID, newName_id, 
            classID_to_ldType(pState.arrayLib_classID), 
            isMethod, isCtor);
    }

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
            assert(pState.getClassByID(idx).getIsDefined() == false);
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
            case TokenTypes::tMETHOD:
                pState.fsmCurState = ParseFsmStates::sMETHOD_DEF;
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

        // legowelt TODO: checkCreateUserDefinedDataType in case we declare
        // a var of user def type (class) that hasn't been defined yet
        // NOTE: SAME IN varDeclStateBeh

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

        // NOTE: this is needed to not do obj.constructor
        // see twin-note in ParserTypes::findVariable
        const bool isMethod = false;
        const bool isCtor = true;
        if (!pState.addCurParseClassFunc(token->tVal.value(), ctorRetType, isMethod, isCtor))
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

        std::string fullCtorName = craftFullFuncName(pState, *(pState.getCurParseClass()), ctorFunc);
        ctorNode->addChild(ALLOC_AST_NODE(AstNodeTypes::aFUNC_DEF, fullCtorName));
        ctorNode->addChild(ALLOC_AST_NODE(AstNodeTypes::aFUNC_LOCNUM, 0));

        // memory commands
        ctorNode->addChild(ALLOC_AST_NODE(AstNodeTypes::aCTOR_ALLOC,
            pState.getCurParseClass()->getFieldVars().size()));

        ctorNode->addChild(ALLOC_AST_NODE(AstNodeTypes::aSTATEMENTS));
        pState.addStackTop(ctorNode->nChildNodes.back());

        ctorNode->addChild(ALLOC_AST_NODE(AstNodeTypes::aFUNC_RET_VAL));

        pState.fsmCurState = ParseFsmStates::sSTATEMENT_DECIDE;
        return true;
    }

    bool funcDefStateBeh(parserState &pState, bool isMethod)
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

        pState.addCurParseClassFunc(token->tVal.value(), ldType_ret, isMethod);
        // advancing to (
        pState.advance();
        parseFuncPars(pState);

        // skipping the {
        if (!pState.advance(2))
            return pState.fsmTerminate(false);

        const auto &curParseFunc = *(pState.getCurParseFunc());
        auto *funcNode = createStackTopNode(pState, AstNodeTypes::aFUNCTION, curParseFunc.getID());

        std::string fullFuncName = craftFullFuncName(pState, *(pState.getCurParseClass()), curParseFunc);
        funcNode->addChild(ALLOC_AST_NODE(AstNodeTypes::aFUNC_DEF, fullFuncName));
        funcNode->addChild(ALLOC_AST_NODE(AstNodeTypes::aFUNC_LOCNUM, 0));

        auto *stmtsNode = ALLOC_AST_NODE(AstNodeTypes::aSTATEMENTS);
        funcNode->addChild(stmtsNode);
        pState.addStackTop(funcNode->nChildNodes.back());

        // method specific things, adding this
        if (isMethod)
        {
            stmtsNode->addChild(ALLOC_AST_NODE(AstNodeTypes::aARG_VAR_READ, 0));
            stmtsNode->addChild(ALLOC_AST_NODE(AstNodeTypes::aTHIS_WRITE, 0));
        }

        if (curParseFunc.ldType_ret == LangDataTypes::ldVOID)
        {
            funcNode->addChild(ALLOC_AST_NODE(AstNodeTypes::aNUMBER, 0));
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
        
        assert (funcToken.tType == TokenTypes::tIDENTIFIER);
        assert (funcToken.tVal.has_value());

        // legowelt TODO: -1 until we implement finding calling on objs/classes in here
        int TEMP_classID = -1;
        auto [contains, funcID] = pState.findFunction(funcToken.tVal.value(), TEMP_classID);

        if (contains)
        {
            if (pState.getFuncByIDFromClass(funcID, TEMP_classID).isMethod)
            {
                pState.addStackTopChild(ALLOC_AST_NODE(AstNodeTypes::aTHIS_READ));
            }
        }
        else
        {
            // TODO: error: UNKNOWN IDENTIFIER
        #ifdef ERR_DEBUG
            std::cerr << "ERR: UNKNOWN IDENTIFIER: " << pState.getIdent()->at(funcToken.tVal.value()) << '\n';
        #endif
        }

        // legowelt TODO: checking that function exists in curParseClass or class of obj it's called on
        auto [res, funcCallRoot] = parseFuncCallArgs(pState, TEMP_classID);
        pState.popStackTop();

        if (pState.getCurToken().tType != TokenTypes::tRPR)
        {
            return pState.fsmTerminate(false);
        }
        auto &curToken = pState.advanceAndGet();
        if (pState.getTokensFinished())
            return pState.fsmTerminate(false);

        if (curToken.tType != TokenTypes::tSEMICOLON)
        {
            return pState.fsmTerminate(false);
        }

        auto *stackTop = pState.getStackTop();
        stackTop->addChild(ALLOC_AST_NODE(AstNodeTypes::aTEMP_VAR_WRITE, 0));
        pState.advance();
        pState.fsmCurState = ParseFsmStates::sSTATEMENT_DECIDE;
        return res;
    }

    std::tuple<bool, AstNode*> parseFuncCallArgs(parserState &pState, int classID = -1)
    {
        auto &funcToken = pState.getCurToken();
        // legowelt TODO: called on object or class behavior
        assert (funcToken.tType == TokenTypes::tIDENTIFIER);
        assert (funcToken.tVal.has_value());
        const unsigned int nameID = funcToken.tVal.value();

        if (!pState.advance())
            return {pState.fsmTerminate(false), NULL};

        if (pState.getCurToken().tType != TokenTypes::tLPR)
        {
            // TODO: error: syntax
            return {false, NULL};
        }

        while (true)
        {
            if (!pState.advance())
                return {pState.fsmTerminate(false), NULL};

            parseExpr(pState);
            auto token = pState.getCurToken();
            if (token.tType != TokenTypes::tCOMMA)
            {
                break;
            }
        }

        // checking that we designed the processing correctly
        // hence assert
        assert(pState.getCurToken().tType == TokenTypes::tRPR || 
            pState.getCurToken().tType == TokenTypes::tSEMICOLON);

        auto *stackTop = pState.getStackTop();
        assert(stackTop->aType == AstNodeTypes::aDO);
        const unsigned int numArgs = stackTop->getNumOfChildren();

        std::string fullFuncName = craftFullFuncName(pState, pState.getClassByID(classID), nameID);
        pState.addStackTopChild(ALLOC_AST_NODE(AstNodeTypes::aFUNC_CALL, fullFuncName));
        pState.addStackTopChild(ALLOC_AST_NODE(AstNodeTypes::aFUNC_ARGNUM, numArgs));

        return {true, stackTop};
    }

    AstNode *parseExpr(parserState &pState)
    {   
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

        AstNode *curTermNode = NULL;
        auto handleFuncNodes = [&](TokenData &token, FuncMethodData &funcMethodData, int classID = -1)
        {
            // legowelt TODO: makes sense to have checkcreateFunction instead of findFunction
            auto [contains, funcID] = pState.findFunction(token.tVal.value(), classID);
            if (contains)
            {
                pState.addStackTop(ALLOC_AST_NODE(AstNodeTypes::aDO));

                if (pState.getFuncByIDFromClass(funcID, classID).isMethod)
                {
                    // pusing the address of the object on which the method is called
                    /*
                    e.g.
                    var NiceHello nh;
                    let nh = NiceHello.new(25,100);
                    return nh.nhelloFunc(700);
                    In this case, we do push local 0 
                    for pushing nh as the first argument of nhelloFunc
                    */
                    pState.addStackTopChild(ALLOC_AST_NODE(funcMethodData.varAccessType, 
                        funcMethodData.varIdx));
                }

                const int curLayer = pState.getLayer();
                pState.resetLayer();
                auto [res, funcCallRoot] = parseFuncCallArgs(pState, classID);
                curTermNode = funcCallRoot;
                pState.restoreLayer(curLayer);
                pState.popStackTop();
            }
            return contains;
        };

        auto parseFuncCall = [&](int classID, FuncMethodData &funcMethodData)
        {
            auto *token = &(pState.advanceAndGet());
            bool continueParsing = true;
            if (pState.getTokensFinished())
            {
                pState.fsmTerminate(false);
                continueParsing = false;
            }
            if (token->tType != TokenTypes::tACCESS)
            {
                pState.fsmTerminate(false);
                continueParsing = false;
            }
            token = &(pState.advanceAndGet());
            if (pState.getTokensFinished())
            {
                pState.fsmTerminate(false);
                continueParsing = false;
            }
            if (token->tType == TokenTypes::tIDENTIFIER)
            {
                if (!handleFuncNodes(*token, funcMethodData, classID))
                {
                    // TODO: error: UNKNOWN IDENTIFIER
                #ifdef ERR_DEBUG
                    std::cerr << "ERR: UNKNOWN IDENTIFIER: " << pState.getIdent()->at(token->tVal.value()) << '\n';
                #endif
                }

                continueParsing = true;
            }
            else
            {
            #ifdef ERR_DEBUG
                std::cerr << "ERR: EXPECTED IDENTIFIER, BUT FOUND: " << tType_to_string(token->tType) << '\n';
            #endif
                continueParsing = true;
            }
            return continueParsing;
        };

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
            else if (token.tType == TokenTypes::tARRAY)
            { 
                // expect function but not method,
                // we are calling on class (Array)
                FuncMethodData funcMethodData(true, false);
                if (!parseFuncCall(pState.arrayLib_classID, funcMethodData))
                    return NULL;
            }
            else if (token.tType == TokenTypes::tIDENTIFIER)
            {
                assert(token.tVal.has_value());

                auto [varScope, varIdx] = pState.findVariable(token.tVal.value());
                if (varScope != VarScopes::scUNKNOWN)
                {
                    const VariableData &varData = pState.getVariableInScope(varScope, varIdx);

                    // primitive type, can be used in the expression as is;
                    // need lookahead if we want to do something like obj1 + obj2
                    if (isprimitivevartype(ldType_to_tType(varData.valueType)))
                    {
                        curTermNode = ALLOC_AST_NODE(varScopeToAccessType(varScope), varIdx);
                    }
                    else
                    {
                        // do not expect function, but expect method,
                        // we are calling on object of class
                        FuncMethodData funcMethodData(false, true, varScopeToAccessType(varScope), varIdx);
                        if (!parseFuncCall(ldType_to_classID(varData.valueType), funcMethodData))
                            return NULL;
                    }
                }
                else
                {
                    // internal if, because we need to record containsClass res
                    // legowelt TODO: use checkCreateUserDefinedDataType over containsClass
                    auto [classExists, classID] = pState.containsClass(token.tVal.value());
                    if (classExists)
                    {   
                        // expect function but not method,
                        // we are calling on class
                        FuncMethodData funcMethodData(true, false, varScopeToAccessType(varScope), varIdx);
                        if (!parseFuncCall(classID, funcMethodData))
                            return NULL;
                    }
                    else 
                    {   
                        // can be both a method and a function (called in current class, so
                        // can't derive this info beforehand like in cases above)
                        FuncMethodData funcMethodData(true, true, AstNodeTypes::aTHIS_READ, 0);
                        if (!handleFuncNodes(token, funcMethodData))
                        {
                    #ifdef ERR_DEBUG
                        std::cerr << "ERR: UNKNOWN IDENTIFIER: " << pState.getIdent()->at(token.tVal.value()) << '\n';
                    #endif
                        }
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
            else
            {
                // some TokenTypes entry hasn't been covered by parser
                assert(false);
#ifdef ERR_DEBUG   
                std::cerr << "ERR: TOKEN TYPE NOT COVERED IN PARSER: " << tType_to_string(token.tType) << '\n';
#endif
            }

            pState.advance();
        }

        auto *stackTop = pState.getStackTop();
        assert(stackTop != NULL);
        
        if (curTermNode != NULL)
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
        return stackTop;
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
        // NOTE: allows the same file have different classes
        else if (pState.getStackTop()->aType == AstNodeTypes::aROOT)
        {
            pState.fsmCurState = ParseFsmStates::sINIT;
        }
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

    AstNode *buildAST(tokensVect &tokens, identifierVect &identifiers, unsigned int tokenOffset)
    {
        pState.setTokens(&tokens);
        pState.setIdentifiers(&identifiers);
        pState.setCurTokenID(tokenOffset);

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
                funcDefStateBeh(pState, false);
                break;
            case ParseFsmStates::sMETHOD_DEF:
                debug_strm << "sMETHOD_DEF hits\n";
                funcDefStateBeh(pState, true);
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