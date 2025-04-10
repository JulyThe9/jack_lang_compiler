#ifndef _PARSER_TYPES_
#define _PARSER_TYPES_

#include <stack>
#include <iostream>
#include <tuple>
#include <variant>

#include "CheckerTypes.h"
#include "Hierarchy.h"
#include "DEBUG_CONTROL.h"

// should be greater than the number of operators
#define LAYER_INCR 10
#define LAYER_DECR LAYER_INCR

#define MAX_EXPTECTED_AST_NODES 500

// preliminary, some will go away
enum class AstNodeTypes : unsigned int
{
    aCLASS = 0,
    aCONSTRUCTOR,
    aMETHOD,
    aFUNCTION,
    aVAR,
    aSTATIC,
    aFIELD,
    aLET,
    aDO,
    aIF,
    aELSE,
    aWHILE,
    aRETURN,
    aTRUE,
    aFALSE,
    aNULL,

    aLPR,
    aRPR,
    aLBR,
    aRBR,
    aLCURL,
    aRCURL,
    aACCESS,
    aEQUAL,
    aPLUS,
    aMINUS,
    aMULT,
    aDIV,
    aAND,
    aOR,
    aNOT,
    aLT,
    aGT,
    aNEG_MINUS,

    aIDENTIFIER,
    aNUMBER,

    // only AST node types (don't overlap with TokenTypes)
    aROOT,
    aARRAY, // array node
    aWHILE_START,
    aWHILE_END,
    aELSE_START,

    // only plays a role in code generation
    // so maybe find a way to carry over
    // the distinction to the generator
    aWHILE_JUMP,
    aIF_JUMP,
    aELSE_JUMP,

    aSTATEMENTS,
    aLOCAL_VAR_READ,
    aARG_VAR_READ,
    aTEMP_VAR_READ,
    aLOCAL_VAR_WRITE,
    aARG_VAR_WRITE,
    aTEMP_VAR_WRITE,  

    // obj and arr reading/writing
    aPTR_0_READ,
    aPTR_0_WRITE,
    aPTR_1_READ,

    // array manipulation
    aPTR_1_WRITE,
    aTHAT_0_READ,
    aTHAT_0_WRITE,

    // function-related
    // def
    // for function name
    aFUNC_DEF,
    // for parnum in function name parnum
    aFUNC_LOCNUM,
    // for pushing the return value
    aFUNC_RET_VAL,
    // call
    // argnum is the root, 
    // because we generate code for it latest (as per tree traversal)
    aFUNC_ARGNUM,
    aFUNC_CALL,

    aCTOR_ALLOC,

    // class-related
    aFIELD_VAR_READ,
    aFIELD_VAR_WRITE,
    aSTATIC_VAR_READ,
    aSTATIC_VAR_WRITE,

    // error-type, should never happen
    aUNKNOWN
};

#if defined(DEBUG) || defined(PARSER_DEBUG)
inline std::map<AstNodeTypes, std::string> aTypes_to_strings
{
    {AstNodeTypes::aCLASS, "CLASS"},
    {AstNodeTypes::aCONSTRUCTOR, "CONSTRUCTOR"},
    {AstNodeTypes::aMETHOD, "METHOD"},
    {AstNodeTypes::aFUNCTION, "FUNCTION"},
    {AstNodeTypes::aVAR, "VAR"},
    {AstNodeTypes::aSTATIC, "STATIC"},
    {AstNodeTypes::aFIELD, "FIELD"},
    {AstNodeTypes::aLET, "LET"},
    {AstNodeTypes::aDO, "DO"},
    {AstNodeTypes::aIF, "IF"},
    {AstNodeTypes::aELSE, "ELSE"},
    {AstNodeTypes::aWHILE, "WHILE"},
    {AstNodeTypes::aRETURN, "RETURN"},
    {AstNodeTypes::aTRUE, "TRUE"},
    {AstNodeTypes::aFALSE, "FALSE"},
    {AstNodeTypes::aNULL, "NULL"},
    {AstNodeTypes::aLPR, "LPR"},
    {AstNodeTypes::aRPR, "RPR"},
    {AstNodeTypes::aLBR, "LBR"},
    {AstNodeTypes::aRBR, "RBR"},
    {AstNodeTypes::aLCURL, "LCURL"},
    {AstNodeTypes::aRCURL, "RCURL"},
    {AstNodeTypes::aACCESS, "ACCESS"},
    {AstNodeTypes::aEQUAL, "EQUAL"},
    {AstNodeTypes::aPLUS, "PLUS"},
    {AstNodeTypes::aMINUS, "MINUS"},
    {AstNodeTypes::aMULT, "MULT"},
    {AstNodeTypes::aDIV, "DIV"},
    {AstNodeTypes::aAND, "AND"},
    {AstNodeTypes::aOR, "OR"},
    {AstNodeTypes::aNOT, "NOT"},
    {AstNodeTypes::aLT, "LT"},
    {AstNodeTypes::aGT, "GT"},
    {AstNodeTypes::aIDENTIFIER, "IDENTIFIER"},
    {AstNodeTypes::aNUMBER, "NUMBER"},
    {AstNodeTypes::aROOT, "ROOT"},
    {AstNodeTypes::aARRAY, "ARRAY"},
    {AstNodeTypes::aWHILE_START, "WHILE_START"},
    {AstNodeTypes::aWHILE_END, "WHILE_END"},
    {AstNodeTypes::aELSE_START, "ELSE_START"},
    {AstNodeTypes::aWHILE_JUMP, "WHILE_JUMP"},
    {AstNodeTypes::aIF_JUMP, "IF_JUMP"},
    {AstNodeTypes::aELSE_JUMP, "ELSE_JUMP"},
    {AstNodeTypes::aSTATEMENTS, "STATEMENTS"},
    {AstNodeTypes::aLOCAL_VAR_READ,"LOCAL_VAR_READ"},
    {AstNodeTypes::aARG_VAR_READ, "ARG_VAR_READ"},
    {AstNodeTypes::aTEMP_VAR_READ, "TEMP_VAR_READ"},
    {AstNodeTypes::aLOCAL_VAR_WRITE, "LOCAL_VAR_WRITE"},
    {AstNodeTypes::aARG_VAR_WRITE, "ARG_VAR_WRITE"},
    {AstNodeTypes::aTEMP_VAR_WRITE, "TEMP_VAR_WRITE"},
    {AstNodeTypes::aPTR_0_READ, "PTR_0_READ"},
    {AstNodeTypes::aPTR_0_WRITE, "PTR_0_WRITE"},
    {AstNodeTypes::aPTR_1_READ, "PTR_1_READ"},
    {AstNodeTypes::aPTR_1_WRITE, "PTR_1_WRITE"},
    {AstNodeTypes::aTHAT_0_READ, "THAT_0_READ"},
    {AstNodeTypes::aTHAT_0_WRITE, "THAT_0_WRITE"},
    {AstNodeTypes::aFUNC_DEF, "FUNC_DEF"},
    {AstNodeTypes::aFUNC_LOCNUM, "FUNC_LOCNUM"},
    {AstNodeTypes::aFUNC_RET_VAL, "FUNC_RET_VAL"},
    {AstNodeTypes::aFUNC_ARGNUM, "FUNC_ARGNUM"},
    {AstNodeTypes::aFUNC_CALL, "FUNC_CALL"},
    {AstNodeTypes::aCTOR_ALLOC, "CTOR_ALLOC"},
    {AstNodeTypes::aFIELD_VAR_READ,  "FIELD_VAR_READ"},
    {AstNodeTypes::aFIELD_VAR_WRITE, "FIELD_VAR_WRITE"},
    {AstNodeTypes::aSTATIC_VAR_READ,  "STATIC_VAR_READ"},
    {AstNodeTypes::aSTATIC_VAR_WRITE, "STATIC_VAR_WRITE"},
    {AstNodeTypes::aUNKNOWN, "UNKNOWN"}
};
inline const std::string &aType_to_string(AstNodeTypes aType)
{
    auto iter = aTypes_to_strings.find(aType);
    if (iter != aTypes_to_strings.end())
        return iter->second;
    
    auto unkIter = aTypes_to_strings.find(AstNodeTypes::aUNKNOWN);

    assert (unkIter != aTypes_to_strings.end());
    return unkIter->second;
}
#endif

inline std::map<TokenTypes, AstNodeTypes> tTypes_to_aTypes
{
    {TokenTypes::tCLASS, AstNodeTypes::aCLASS},
    {TokenTypes::tCONSTRUCTOR, AstNodeTypes::aCONSTRUCTOR},
    {TokenTypes::tMETHOD, AstNodeTypes::aMETHOD},
    {TokenTypes::tFUNCTION, AstNodeTypes::aFUNCTION},
    {TokenTypes::tVAR, AstNodeTypes::aVAR},
    {TokenTypes::tSTATIC, AstNodeTypes::aSTATIC},
    {TokenTypes::tFIELD, AstNodeTypes::aFIELD},
    {TokenTypes::tLET, AstNodeTypes::aLET},
    {TokenTypes::tDO, AstNodeTypes::aDO},
    {TokenTypes::tIF, AstNodeTypes::aIF},
    {TokenTypes::tELSE, AstNodeTypes::aELSE},
    {TokenTypes::tWHILE, AstNodeTypes::aWHILE},
    {TokenTypes::tRETURN, AstNodeTypes::aRETURN},
    {TokenTypes::tTRUE, AstNodeTypes::aTRUE},
    {TokenTypes::tFALSE, AstNodeTypes::aFALSE},
    {TokenTypes::tNULL, AstNodeTypes::aNULL},

    {TokenTypes::tLPR, AstNodeTypes::aLPR},
    {TokenTypes::tRPR, AstNodeTypes::aRPR},
    {TokenTypes::tLBR, AstNodeTypes::aLBR},
    {TokenTypes::tRBR, AstNodeTypes::aRBR},
    {TokenTypes::tLCURL, AstNodeTypes::aLCURL},
    {TokenTypes::tRCURL, AstNodeTypes::aRCURL},
    {TokenTypes::tACCESS, AstNodeTypes::aACCESS},
    {TokenTypes::tEQUAL, AstNodeTypes::aEQUAL},
    {TokenTypes::tPLUS, AstNodeTypes::aPLUS},
    {TokenTypes::tMINUS, AstNodeTypes::aMINUS},
    {TokenTypes::tMULT, AstNodeTypes::aMULT},
    {TokenTypes::tDIV, AstNodeTypes::aDIV},
    {TokenTypes::tAND, AstNodeTypes::aAND},
    {TokenTypes::tOR, AstNodeTypes::aOR},
    {TokenTypes::tNOT, AstNodeTypes::aNOT},
    {TokenTypes::tLT, AstNodeTypes::aLT},
    {TokenTypes::tGT, AstNodeTypes::aGT},

    {TokenTypes::tNEG_MINUS, AstNodeTypes::aNEG_MINUS},

    {TokenTypes::tIDENTIFIER, AstNodeTypes::aIDENTIFIER},
    {TokenTypes::tNUMBER, AstNodeTypes::aNUMBER}
};

inline AstNodeTypes tType_to_aType(TokenTypes tType)
{
    std::map<TokenTypes, AstNodeTypes>::iterator iter;
    iter = tTypes_to_aTypes.find(tType);

    if (iter != tTypes_to_aTypes.end())
        return iter->second;
    
    return AstNodeTypes::aUNKNOWN;
}
inline TokenTypes aType_to_tType(AstNodeTypes aType)
{
    for (auto it = tTypes_to_aTypes.begin(); it != tTypes_to_aTypes.end(); ++it)
    if (it->second == aType)
        return it->first;

    return TokenTypes::tUNKNOWN_SYMBOL;
}

enum class ParseFsmStates : unsigned int
{
    sINIT = 0,
    sSTATEMENT_DECIDE,
    sWHILE,
    sBLOCK_CLOSE,
    sIF,
    sELSE,
    sEXPR,
    sVAR_DECL,
    sVAR_ASSIGN,

    // class stuff
    sCLASS_DECIDE,
    sFIELD_DECL,
    sSTATIC_DECL,
    sCTOR_DEF,
    sFUNC_DEF,
    sMETHOD_DEF,
    sFUNC_DO_CALL,
    sRETURN
};

inline std::map<TokenTypes, int> precedLookup
{
    {TokenTypes::tEQUAL, 3},
    {TokenTypes::tACCESS, 7},
    {TokenTypes::tNEG_MINUS, 6},
    {TokenTypes::tPLUS, 4},
    {TokenTypes::tMINUS, 4},
    {TokenTypes::tMULT, 5},
    {TokenTypes::tDIV, 5},
    {TokenTypes::tAND, 1},
    {TokenTypes::tOR, 1},
    //{TokenTypes::tNOT},
    {TokenTypes::tLT, 2},
    {TokenTypes::tGT, 2}
};

enum class VarScopes : unsigned int
{
    scLOCAL = 0,
    scARG,
    scFIELD,
    scSTATIC,
    scUNKNOWN
};

inline AstNodeTypes varScopeToAccessType(VarScopes varScope, bool isWriting = false)
{
    if (varScope == VarScopes::scLOCAL)
        return isWriting ? AstNodeTypes::aLOCAL_VAR_WRITE : AstNodeTypes::aLOCAL_VAR_READ;
    if (varScope == VarScopes::scARG)
        return isWriting ? AstNodeTypes::aARG_VAR_WRITE : AstNodeTypes::aARG_VAR_READ;
    if (varScope == VarScopes::scFIELD)
        return isWriting ? AstNodeTypes::aFIELD_VAR_WRITE : AstNodeTypes::aFIELD_VAR_READ;
    if (varScope == VarScopes::scSTATIC)
        return isWriting ? AstNodeTypes::aSTATIC_VAR_WRITE : AstNodeTypes::aSTATIC_VAR_READ;

    return AstNodeTypes::aUNKNOWN;
}

/*
EXAMPLES:
WHILE -> "lbl{" , "EXPR" , "JUMP" , STATEMENTS
FUNCTION -> FUNC_DEF, FUNC_LOCNUM, STATEMENTS, FUNC_RET_VAL
*/
class AstNode : DebugData
{
private:
    // tbh trivial method, could return true for all and
    // generate whatever is in the "generation map file"
    // which would have empty values for these keys
    static bool checkGeneratesCode(AstNodeTypes aType)
    {
        return aType != AstNodeTypes::aSTATEMENTS &&
            aType != AstNodeTypes::aROOT &&
            aType != AstNodeTypes::aCLASS;
    }
    static int assignId()
    {
        static int idPool = 0;
        return idPool++;
    }
    
    // moved to private to not mess with pointer
    AstNode *parentNode = NULL;

public:
    
    AstNodeTypes aType;
    std::variant<std::monostate, int, std::string> aVal;

    std::vector<AstNode*> nChildNodes;
    int nPrecCoeff = 0;      // relevant for operators only
    int nID = 0;
    bool generatesCode = false;
    
    explicit AstNode(TokenData &token);

    AstNode(TokenData &token, int precCoeff);
    AstNode(AstNodeTypes aType);
    AstNode(AstNodeTypes aType, int aVal);
    AstNode(AstNodeTypes aType, const std::string &aVal);
    AstNode();

    ~AstNode();

    void setNodeValue(int value);

    void overwriteNodeValue(int value);

    int getNodeValue() const;

    std::string getNodeValueAsString() const;
    
    inline void setParent(AstNode *parent)
    {
        parentNode = parent;
    }

    inline AstNode *getParent()
    {
        return parentNode;
    }
     
    inline unsigned int getNumOfChildren() const
    {
        return nChildNodes.size();
    }

    void addChild(AstNode *child);

    void addChildConditional(AstNode *child);

#if defined(DEBUG) || defined(PARSER_DEBUG)
    void print();
#endif

private:

    bool containsChildNode(int nID);
};

inline AstNode *getIfBlockJump(AstNode *ifNodeDesc)
{
    auto *parent = ifNodeDesc->getParent();

    while (parent && parent->aType != AstNodeTypes::aIF)
    {
        parent = parent->getParent();
    }
    if (!parent)
        return NULL;

    assert(parent->nChildNodes.size() >= 4);
    auto *ifJumpNode = parent->nChildNodes[2];
    assert(ifJumpNode->aType == AstNodeTypes::aIF_JUMP);

    return ifJumpNode;
}

inline AstNode *getFuncLocNumNode(AstNode *funcNode)
{
    for (auto *e : funcNode->nChildNodes)
    {
        if (e->aType == AstNodeTypes::aFUNC_LOCNUM)
            return e;
    }
    return NULL;
}

// going up the tree from childNode
inline AstNode *getNearestFuncRootNode(AstNode *childNode)
{
    AstNode *queryNode = childNode;
    while (queryNode->aType != AstNodeTypes::aFUNCTION)
    {
        queryNode = queryNode->getParent();
        if (queryNode == NULL)
            return NULL;
    }
    return queryNode;
}

inline bool greaterPreced(const AstNode &t1, const AstNode &t2)
{
    auto tType1 = aType_to_tType(t1.aType);
    auto tType2 = aType_to_tType(t2.aType);
    
    assert(isoperator(tType1) && isoperator(tType2));

    auto itr1 = precedLookup.find(tType1);
    assert (itr1 != precedLookup.end());

    auto itr2 = precedLookup.find(tType2);
    assert (itr2 != precedLookup.end());

    return (itr1->second + t1.nPrecCoeff > itr2->second + t2.nPrecCoeff);
}

inline int getLabelId()
{
    static int labelId = 0;
    return labelId++;
}

inline bool isblockstart(AstNodeTypes aType)
{
    return aType == AstNodeTypes::aWHILE 
        || aType == AstNodeTypes::aIF
        || aType == AstNodeTypes::aFUNCTION
        || aType == AstNodeTypes::aCLASS;
}

class ParserState
{
private:
    tokensVect *tokens;
    identifierVect *identifiers;
    unsigned int curTokenId = 0;
    bool tokensFinished = false;
    ClassData *curParseClass = NULL;
    int layerCoeff = 0;
    int arrayEnteryNum = 0;

public:
    bool fsmFinished = false;
    bool fsmFinishedCorrectly = true;
    ParseFsmStates fsmCurState = ParseFsmStates::sINIT;

    std::stack<AstNode*> pendParentNodes;
    std::vector<ClassData> classes;

    bool declaringLocals = false;

    unsigned int arrayLib_classID = 0;

    inline const std::vector<ClassData> &getClasses() const
    {
        return classes;
    }

    ClassData &getClassByID(int classID = -1);

    const FunctionData &getFuncByIDFromClass(unsigned int funcID, int classID = -1);

    std::tuple<bool, unsigned int> containsClass(unsigned int nameID);

    IDable::idx_in_cont addClass(unsigned int nameID, bool isDefined = false);

    // TODO: CHECKER: after parsing the whole prog (all files):
    // run though classes and see that none have isDefined == false
    // classID as idx in classes container
    void setCurParseClass(unsigned int classID);
    inline ClassData *getCurParseClass() const
    {
        return curParseClass;
    }

    void addCurParseClassFieldVar(unsigned int nameID, LangDataTypes valueType);
    void addCurParseClassStaticVar(unsigned int nameID, LangDataTypes valueType);

    bool addCurParseClassFunc(unsigned int nameID, LangDataTypes ldType_ret,
        bool isMethod, bool isCtor = false);

    // only for loading system library symbols
    bool addFuncToClass(int classID, unsigned int nameID, LangDataTypes ldType_ret,
        bool isMethod, bool isCtor = false);
    
    FunctionData *getCurParseFunc() const;
    void addCurParseFuncPar(unsigned int nameID, LangDataTypes ldType_par);
    void addLocalScopeFramesTopVar(unsigned int nameID, LangDataTypes valueType);
    bool popLocalScopeFramesTop();

    std::tuple<bool, unsigned int> containsArg(int identNameID);
    std::tuple<bool, unsigned int> containsLocal(int identNameID);
    std::tuple<bool, unsigned int> containsField(int identNameID);
    std::tuple<bool, unsigned int> containsStatic(int identNameID);
    
    const VariableData &getArgVar(unsigned int idx) const;
    const VariableData &getLocalVar(unsigned int idx) const;
    const VariableData &getFieldVar(unsigned int idx) const;
    const VariableData &getStaticVar(unsigned int idx) const;

    ParserState();

    void setTokens(tokensVect *tokensPar);

    void setIdentifiers(identifierVect *identifiersPar);

    void resetNonShared();

    inline int getLayer() {return layerCoeff;}
    inline void incLayer() {layerCoeff+=LAYER_INCR;}
    inline void decLayer() {layerCoeff-=LAYER_DECR;}
    inline void resetLayer() {layerCoeff = 0;}
    inline void restoreLayer(int storedLayer) { layerCoeff = storedLayer; }

    inline int getArrayEnteryNum() {return arrayEnteryNum;}
    inline void incArrayEnteryNum() {arrayEnteryNum++;}
    inline void decArrayEnteryNum() {arrayEnteryNum--;}

    void addStackTopChild(AstNode *child);

    void addStackTop(AstNode *newTop);

    bool popStackTop();

    AstNode *getStackTop();

    LangDataTypes checkCreateUserDefinedDataType(const TokenData &token, bool onlyCheck = false);  

    std::tuple<VarScopes, unsigned int> findVariable(unsigned int identNameID);

    std::tuple<bool, unsigned int> findFunction(int identNameID, int classID);

    const VariableData &getVariableInScope(VarScopes varScope, unsigned int idx) const;

    bool advance(unsigned int step = 1);

    TokenData &advanceAndGet(unsigned int step = 1);

    std::tuple<bool, TokenData&> lookAheadGet();

    bool fsmTerminate(bool finishedCorrectly);

    inline auto *getTokens()
    {
        return tokens;
    }
    inline auto *getIdent()
    {
        return identifiers;
    }

    inline void setCurTokenID(unsigned int curTokenIdPar)
    {
        curTokenId = curTokenIdPar;
    }
    
    inline TokenData &getCurToken()
    {
        return tokens->at(curTokenId);
    }

    inline bool getTokensFinished() const
    {
        return tokensFinished;
    }

    inline bool getFsmFinished()
    {
        return fsmFinished || tokensFinished;
    }
};

inline std::string craftFullFuncName(ParserState &pState, const ClassData &classData, const FunctionData &funcData)
{   
    auto &idents = *(pState.getIdent());
    assert(classData.nameID < idents.size());
    assert(funcData.nameID < idents.size());

    return idents[classData.nameID] + "." + idents[funcData.nameID];
}
inline std::string craftFullFuncName(ParserState &pState, const ClassData &classData, unsigned int funcNameID)
{   
    auto &idents = *(pState.getIdent());
    assert(classData.nameID < idents.size());
    assert(funcNameID < idents.size());

    return idents[classData.nameID] + "." + idents[funcNameID];
}

#endif