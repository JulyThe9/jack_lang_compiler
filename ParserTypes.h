#include <stack>
#include <iostream>
#include <tuple>
#include <variant>

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
    aTHIS_READ,
    aTHIS_WRITE,
    aTHAT_READ,
    aTHAT_WRITE,

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

#ifdef DEBUG
std::map<AstNodeTypes, std::string> aTypes_to_strings
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
    {AstNodeTypes::aTHIS_READ, "THIS_READ"},
    {AstNodeTypes::aTHIS_WRITE, "THIS_WRITE"},
    {AstNodeTypes::aTHAT_READ, "THAT_READ"},
    {AstNodeTypes::aTHAT_WRITE, "THAT_WRITE"},
    {AstNodeTypes::aFUNC_DEF, "FUNC_DEF"},
    {AstNodeTypes::aFUNC_LOCNUM, "FUNC_LOCNUM"},
    {AstNodeTypes::aFUNC_RET_VAL, "FUNC_RET_VAL"},
    {AstNodeTypes::aFUNC_ARGNUM, "aFUNC_ARGNUM"},
    {AstNodeTypes::aFUNC_CALL, "FUNC_CALL"},
    {AstNodeTypes::aCTOR_ALLOC, "CTOR_ALLOC"},
    {AstNodeTypes::aFIELD_VAR_READ,  "FIELD_VAR_READ"},
    {AstNodeTypes::aFIELD_VAR_WRITE, "FIELD_VAR_WRITE"},
    {AstNodeTypes::aSTATIC_VAR_READ,  "STATIC_VAR_READ"},
    {AstNodeTypes::aSTATIC_VAR_WRITE, "STATIC_VAR_WRITE"},
    {AstNodeTypes::aUNKNOWN, "UNKNOWN"}
};
const std::string &aType_to_string(AstNodeTypes aType)
{
    auto iter = aTypes_to_strings.find(aType);
    if (iter != aTypes_to_strings.end())
        return iter->second;
    
    auto unkIter = aTypes_to_strings.find(AstNodeTypes::aUNKNOWN);

    assert (unkIter != aTypes_to_strings.end());
    return unkIter->second;
}
#endif

std::map<TokenTypes, AstNodeTypes> tTypes_to_aTypes
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

AstNodeTypes tType_to_aType(TokenTypes tType)
{
    std::map<TokenTypes, AstNodeTypes>::iterator iter;
    iter = tTypes_to_aTypes.find(tType);

    if (iter != tTypes_to_aTypes.end())
        return iter->second;
    
    return AstNodeTypes::aUNKNOWN;
}
TokenTypes aType_to_tType(AstNodeTypes aType)
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

std::map<TokenTypes, int> precedLookup
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

AstNodeTypes varScopeToAccessType(VarScopes varScope, bool isWriting = false)
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
struct AstNode
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
    // any extra fields?
    explicit AstNode(TokenData &token) : aType(tType_to_aType(token.tType)),
        nID(assignId()), generatesCode(checkGeneratesCode(aType))
    {
        if (token.tVal.has_value())
            aVal = token.tVal.value();
    }

    AstNode(TokenData &token, int precCoeff) : aType(tType_to_aType(token.tType)),
        nPrecCoeff(precCoeff), nID(assignId()), generatesCode(checkGeneratesCode(aType))
    {
        if (token.tVal.has_value())
            aVal = token.tVal.value();
    }

    AstNode(AstNodeTypes aType) : aType(aType), nID(assignId()),
        generatesCode(checkGeneratesCode(aType))
    {}

    AstNode(AstNodeTypes aType, int aVal) : aType(aType), aVal(aVal), nID(assignId()),
        generatesCode(checkGeneratesCode(aType))
    {}

    AstNode(AstNodeTypes aType, const std::string &aVal) : aType(aType), aVal(aVal), nID(assignId()),
        generatesCode(checkGeneratesCode(aType))
    {}

    AstNode() : aType(AstNodeTypes::aROOT), nID(assignId()), 
        generatesCode(checkGeneratesCode(AstNodeTypes::aROOT))
    {}

    ~AstNode() 
    {
#ifdef MISC_DEBUG        
        std::cout << "ast node DTOR called\n"; 
#endif
    }

    AstNodeTypes aType;
    std::variant<std::monostate, int, std::string> aVal;

    std::vector<AstNode*> nChildNodes;
    int nPrecCoeff = 0;      // relevant for operators only
    int nID = 0;
    bool generatesCode = false;

    void setNodeValue(int value)
    {
        // making sure no overwriting occurs
        // (programmer's responsibility, hence assert)
        assert(!aVal.index() != 0);
        aVal = value;
    }
    void overwriteNodeValue(int value)
    {
        aVal = value;
    }

    int getNodeValue() const
    {
        assert(std::holds_alternative<int>(aVal));
        return std::get<int>(aVal);
    }

    std::string getNodeValueAsString() const
    {
        if (std::holds_alternative<int>(aVal))
            return std::to_string(std::get<int>(aVal));
        else if (std::holds_alternative<std::string>(aVal))
            return std::get<std::string>(aVal);
        
        return "";
    }
    
    void setParent(AstNode *parent)
    {
        parentNode = parent;
    }
    AstNode *getParent()
    {
        return parentNode;
    }
    void addChild(AstNode *child)
    {
        assert(child != NULL);
        nChildNodes.push_back(child);
        nChildNodes.back()->setParent(this);
    }
    unsigned int getNumOfChildren() const
    {
        return nChildNodes.size();
    }

#ifdef DEBUG
    void print()
    {
        std::cout << "AstNode #" << nID << '\n';
        std::cout << "Type: " << aType_to_string(aType) << '\n';

        if (std::holds_alternative<int>(aVal))
            std::cout << "Val: " << std::to_string(std::get<int>(aVal))  << '\n';
        else if(std::holds_alternative<std::string>(aVal))
            std::cout << "Val: " << std::get<std::string>(aVal) << '\n';
        else
            std::cout << "Val: None\n";

        std::cout << "Children size: " << nChildNodes.size()  << '\n';
        std::cout << "Children:";
        for (auto *elem : nChildNodes)
        {
            std::cout << " #" << elem->nID;
        }
        std::cout << '\n';
        std::cout << '\n';
    }
#endif
};

AstNode *getIfBlockJump(AstNode *ifNodeDesc)
{
    auto *parent = ifNodeDesc->getParent();

    while (parent && parent->aType != AstNodeTypes::aIF)
    {
        parent = parent->getParent();
    }
    if (!parent)
        return NULL;

    assert(parent->nChildNodes.size() >= 3);
    auto *ifJumpNode = parent->nChildNodes[1];
    assert(ifJumpNode->aType == AstNodeTypes::aIF_JUMP);

    return ifJumpNode;
}

AstNode *getFuncLocNumNode(AstNode *funcNode)
{
    for (auto *e : funcNode->nChildNodes)
    {
        if (e->aType == AstNodeTypes::aFUNC_LOCNUM)
            return e;
    }
    return NULL;
}

// going up the tree from childNode
AstNode *getNearestFuncRootNode(AstNode *childNode)
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


bool greaterPreced(const AstNode &t1, const AstNode &t2)
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

int getLabelId()
{
    static int labelId = 0;
    return labelId++;
}

// bool isconnectornode(TokenTypes tType)
// {
//     // also statements
//     return tType == TokenTypes::tARRAY;
// }
bool isblockstart(AstNodeTypes aType)
{
    return aType == AstNodeTypes::aWHILE 
        || aType == AstNodeTypes::aIF
        || aType == AstNodeTypes::aFUNCTION
        || aType == AstNodeTypes::aCLASS;
}


struct parserState
{
private:
    tokensVect *tokens;
    identifierVect *identifiers;
    unsigned int curTokenId = 0;
    bool tokensFinished = false;
    ClassData *curParseClass = NULL;
    int layerCoeff = 0;
public:
    bool fsmFinished = false;
    bool fsmFinishedCorrectly = true;
    ParseFsmStates fsmCurState = ParseFsmStates::sINIT;

    std::stack<AstNode*> pendParentNodes;
    std::vector<ClassData> classes;

    bool declaringLocals = false;

    unsigned int getCurLineNum() const
    {
        // TODO: line nums
        return 0;
    }
    const std::vector<ClassData> &getClasses() const
    {
        return classes;
    }
    ClassData &getClassByID(int classID = -1)
    {
        if (classID < 0)
        {
            assert(getCurParseClass() != NULL);
            // we could have just said return return *(getCurParseClass()),
            // but with this we ensure that we DO return an actual class
            // from classes container
            return getClassByID(getCurParseClass()->getID());
        }
        assert(!classes.empty());
        assert(classID < classes.size());
        assert(classID == classes[classID].getID());
        return classes[classID];
    }
    const FunctionData &getFuncByIDFromClass(unsigned int funcID, int classID = -1)
    {
        if (classID >= 0)
        {
            return getClassByID(classID).getFuncByID(funcID);
        }
        return getCurParseClass()->getFuncByID(funcID);
    }

    std::tuple<bool, unsigned int> containsClass(unsigned int nameID)
    {
        auto iter = std::find_if(classes.begin(), classes.end(), 
            [&nameID](const ClassData  &arg){ return arg.nameID == nameID; });

        return {iter != classes.end(), 
                std::distance(classes.begin(), iter)};
    }
    void addClass(unsigned int nameID, bool isDefined = false)
    {
        // the next var needed purely due to pointer invalidation after reseizing
        unsigned int curParseClassIdx = 0;
        bool restateCurParseClass = false;
        if (curParseClass != NULL)
        {
            restateCurParseClass = true;
            curParseClassIdx = curParseClass->getID();
        }

        auto &curClass = classes.emplace_back(nameID);
        curClass.setIsDefined(isDefined);
        curClass.setID(classes.size()-1);
        // isDefined == true -> we are in the class definition,
        // so this becomes the current class being parsed
        if (isDefined)
        {
            curParseClass = &curClass;
        }
        else if (restateCurParseClass)
        {
            // resizing on .emplace_back causes pointer invalidation
           curParseClass = &(classes[curParseClassIdx]);
        }
    }
    // TODO: CHECKER: after parsing the whole prog (all files):
    // run though classes and see that none have isDefined == false
    // classID as idx in classes container
    void setCurParseClass(unsigned int classID)
    {
        assert(classID < classes.size());
        curParseClass = &(classes[classID]);
        // this is the class we are currently defining
        curParseClass->setIsDefined(true);
    }
    ClassData *getCurParseClass() const
    {
        return curParseClass;
    }
    void addCurParseClassFieldVar(unsigned int nameID, LangDataTypes valueType)
    {
        getCurParseClass()->addFieldVar(nameID, valueType);
    }
    void addCurParseClassStaticVar(unsigned int nameID, LangDataTypes valueType)
    {
        getCurParseClass()->addStaticVar(nameID, valueType);
    }

    bool addCurParseClassFunc(unsigned int nameID, LangDataTypes ldType_ret,
        bool isMethod, bool isCtor = false)
    {
        return getCurParseClass()->addFunc(nameID, ldType_ret, isMethod, isCtor);
    }
    
    FunctionData *getCurParseFunc() const
    {
        auto *curParseClass = getCurParseClass();
        if (curParseClass == NULL)
            return NULL;
        return curParseClass->getLastFunc();
    }
    void addCurParseFuncPar(unsigned int nameID, LangDataTypes ldType_par)
    {
        auto *curParseClass = getCurParseClass();
        if (curParseClass != NULL)
        {
            curParseClass->addFuncPar(nameID, ldType_par);
        }
    }
    void addLocalScopeFramesTopVar(unsigned int nameID, LangDataTypes valueType)
    {
        getCurParseFunc()->addLocalScopeFramesTopVar(nameID, valueType);
    }
    bool popLocalScopeFramesTop()
    {   
        return getCurParseFunc()->popLocalScopeFramesTop(); 
    }
    std::tuple<bool, unsigned int> containsArg(int identNameID)
    {
        return getCurParseFunc()->containsArg(identNameID);
    }
    std::tuple<bool, unsigned int> containsLocal(int identNameID)
    {
        return getCurParseFunc()->containsLocal(identNameID);
    }
    std::tuple<bool, unsigned int> containsField(int identNameID)
    {
        return getCurParseClass()->containsField(identNameID);
    }
    std::tuple<bool, unsigned int> containsStatic(int identNameID)
    {
        return getCurParseClass()->containsStatic(identNameID);
    }
    
    const VariableData &getArgVar(unsigned int idx) const
    {
        return getCurParseFunc()->getArgVar(idx);
    }
    const VariableData &getLocalVar(unsigned int idx) const
    {
        return getCurParseFunc()->getLocalVar(idx);
    }
    const VariableData &getFieldVar(unsigned int idx) const
    {
        return getCurParseClass()->getFieldVar(idx);
    }
    const VariableData &getStaticVar(unsigned int idx) const
    {
        return getCurParseClass()->getStaticVar(idx);
    }

    parserState()
    {
        identifiers = NULL;
        classes.clear();
        resetNonShared();
    }

    void setTokens(tokensVect *tokensPar)
    {
        tokens = tokensPar;
    }
    void setIdentifiers(identifierVect *identifiersPar)
    {
        identifiers = identifiersPar;
    }

    void resetNonShared()
    {
        tokens = NULL;
        identifiers = NULL;
        curTokenId = 0;
        tokensFinished = false;
        curParseClass = NULL;
        layerCoeff = 0;
        fsmFinished = false;
        fsmFinishedCorrectly = true;
        fsmCurState = ParseFsmStates::sINIT;
        declaringLocals = false;

        while (!pendParentNodes.empty())
            pendParentNodes.pop();

        // classes comment
    }

    int getLayer() {return layerCoeff;}
    void incLayer() {layerCoeff+=LAYER_INCR;}
    void decLayer() {layerCoeff-=LAYER_DECR;}
    void resetLayer() {layerCoeff = 0;}
    void restoreLayer(int storedLayer) { layerCoeff = storedLayer; }

    void addStackTopChild(AstNode *child)
    {
        pendParentNodes.top()->addChild(child);
    }
    void addStackTop(AstNode *newTop)
    {
        pendParentNodes.push(newTop);
    }
    bool popStackTop()
    {   
        if (pendParentNodes.empty())
            return false;
        pendParentNodes.pop();
        return true;
    }
    AstNode *getStackTop()
    {
        if (pendParentNodes.empty())
            return NULL;
        return pendParentNodes.top();
    }

    LangDataTypes checkCreateUserDefinedDataType(const TokenData &token, bool onlyCheck = false)
    {
        assert(token.tVal.has_value());
        unsigned int classID = 0;
        auto classNameID = token.tVal.value();      
        auto [classExists, idx] = containsClass(classNameID);
        if (classExists)
        {
            classID = idx;               
        }
        else if (onlyCheck)
        {
            return LangDataTypes::ldUNKNOWN;
        }
        else
        {
            // "extending" LangDataTypes enum, by adding class id in classes to class offset
            // in the enum (LangDataTypes::ldCLASS)
            classID = getClasses().size();
            const bool isDefined = false;
            addClass(classNameID, isDefined);
        }

        return classID_to_ldType(classID);
    }    

    std::tuple<VarScopes, unsigned int> findVariable(unsigned int identNameID)
    {   
        // LOCAL
        auto [contains, idx] = containsLocal(identNameID);
        if (contains)                 
            return {VarScopes::scLOCAL, idx};

        // ARG
        std::tie(contains, idx) = containsArg(identNameID);
        if (contains)
            return {VarScopes::scARG, idx};
        
        // FIELD
        // NOTE: a constructor is not a method in asense that you cannot
        // call it on an object, but it must still be possible to access the
        // field variables from it
        if (getCurParseFunc()->isMethod || getCurParseFunc()->isCtor)
        {
            std::tie(contains, idx) = containsField(identNameID);
            if (contains)
                return {VarScopes::scFIELD, idx};
        }

        // STATIC
        std::tie(contains, idx) = containsStatic(identNameID);
        if (contains)
            return {VarScopes::scSTATIC, idx};

        // might be a function or Class(?) name
        return {VarScopes::scUNKNOWN, 0};
    }

    std::tuple<bool, unsigned int> findFunction(int identNameID, int classID)
    {
        if (classID >= 0)
        {
            return getClassByID(classID).containsFunc(identNameID);
        }
        return getCurParseClass()->containsFunc(identNameID);
    }

    const VariableData &getVariableInScope(VarScopes varScope, unsigned int idx) const
    {
        if (varScope == VarScopes::scLOCAL)
            return getLocalVar(idx);

        if (varScope == VarScopes::scARG)
            return getArgVar(idx);

        if (varScope == VarScopes::scFIELD)
            return getFieldVar(idx);

        if (varScope == VarScopes::scSTATIC)
            return getStaticVar(idx);

        assert(false);
        // to not have the missing return warning
        VariableData dummy(0, LangDataTypes::ldUNKNOWN);
        return dummy;
    }

    auto *getTokens()
    {
        return tokens;
    }
    auto *getIdent()
    {
        return identifiers;
    }

    void setCurTokenID(unsigned int curTokenIdPar)
    {
        curTokenId = curTokenIdPar;
    }
    
    TokenData &getCurToken()
    {
        return tokens->at(curTokenId);
    }
    bool advance(unsigned int step = 1)
    {
        if (curTokenId < tokens->size()-step)
        {
            curTokenId+=step;
            return true;
        }
        tokensFinished = true;
        return false;
    }
    TokenData &advanceAndGet(unsigned int step = 1)
    {
        advance(step);
        return getCurToken();
    }
    bool getTokensFinished() const
    {
        return tokensFinished;
    }
    bool fsmTerminate(bool finishedCorrectly)
    {
        fsmFinished = true;
        fsmFinishedCorrectly = finishedCorrectly;
        return fsmFinishedCorrectly;
    }
    bool getFsmFinished()
    {
        return fsmFinished || tokensFinished;
    }
};

std::string craftFullFuncName(parserState &pState, const ClassData &classData, const FunctionData &funcData)
{   
    auto &idents = *(pState.getIdent());
    assert(classData.nameID < idents.size());
    assert(funcData.nameID < idents.size());

    return idents[classData.nameID] + "." + idents[funcData.nameID];
}
std::string craftFullFuncName(parserState &pState, const ClassData &classData, unsigned int funcNameID)
{   
    auto &idents = *(pState.getIdent());
    assert(classData.nameID < idents.size());
    assert(funcNameID < idents.size());

    return idents[classData.nameID] + "." + idents[funcNameID];
}