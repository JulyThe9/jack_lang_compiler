#include "ParserTypes.h"

AstNode::AstNode(TokenData &token) : DebugData(token.debug_lineNum), 
    aType(tType_to_aType(token.tType)), nID(assignId()), 
    generatesCode(checkGeneratesCode(aType))
{
    if (token.tVal.has_value())
        aVal = token.tVal.value();
}

AstNode::AstNode(TokenData &token, int precCoeff) : DebugData(token.debug_lineNum), aType(tType_to_aType(token.tType)),
    nPrecCoeff(precCoeff), nID(assignId()), generatesCode(checkGeneratesCode(aType))
{
    if (token.tVal.has_value())
        aVal = token.tVal.value();
}

AstNode::AstNode(AstNodeTypes aType) : DebugData(0), aType(aType), nID(assignId()),
    generatesCode(checkGeneratesCode(aType))
{}

AstNode::AstNode(AstNodeTypes aType, int aVal) : DebugData(0), aType(aType), aVal(aVal), nID(assignId()),
    generatesCode(checkGeneratesCode(aType))
{}

AstNode::AstNode(AstNodeTypes aType, const std::string &aVal) : DebugData(0), aType(aType), aVal(aVal), 
    nID(assignId()), generatesCode(checkGeneratesCode(aType))
{}

AstNode::AstNode() : DebugData(0), aType(AstNodeTypes::aROOT), nID(assignId()), 
    generatesCode(checkGeneratesCode(AstNodeTypes::aROOT))
{}

AstNode::~AstNode() 
{
#ifdef MISC_DEBUG        
    std::cout << "ast node DTOR called\n"; 
#endif
}

void AstNode::setNodeValue(int value)
{
    // making sure no overwriting occurs
    // (programmer's responsibility, hence assert)
    assert(!aVal.index() != 0);
    aVal = value;
}

void AstNode::overwriteNodeValue(int value)
{
    aVal = value;
}

int AstNode::getNodeValue() const
{
    assert(std::holds_alternative<int>(aVal));
    return std::get<int>(aVal);
}

std::string AstNode::getNodeValueAsString() const
{
    if (std::holds_alternative<int>(aVal))
        return std::to_string(std::get<int>(aVal));
    else if (std::holds_alternative<std::string>(aVal))
        return std::get<std::string>(aVal);
    
    return "";
}

void AstNode::addChild(AstNode *child)
{
#if defined(PARSER_DEBUG)
    print();
    std::cerr << "AST NODE ID: " << nID << 
                    ", line number: " << debug_lineNum << '\n';       
#endif
    assert(child != NULL);
    nChildNodes.push_back(child);
    nChildNodes.back()->setParent(this);
}

void AstNode::addChildConditional(AstNode *child)
{
    if (!containsChildNode(child->nID))
    {
        addChild(child);
    }
}

bool AstNode::containsChildNode(int nID)
{
    auto iter = std::find_if(nChildNodes.begin(), nChildNodes.end(), 
        [nID](const AstNode *arg){ return arg->nID == nID; });

    return (iter != nChildNodes.end());
}

#if defined(DEBUG) || defined(PARSER_DEBUG)
void AstNode::print()
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


ClassData &ParserState::getClassByID(int classID)
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

const FunctionData &ParserState::getFuncByIDFromClass(unsigned int funcID, int classID)
{
    if (classID >= 0)
    {
        return getClassByID(classID).getFuncByID(funcID);
    }
    return getCurParseClass()->getFuncByID(funcID);
}

std::tuple<bool, unsigned int> ParserState::containsClass(unsigned int nameID)
{
    auto iter = std::find_if(classes.begin(), classes.end(), 
        [&nameID](const ClassData  &arg){ return arg.nameID == nameID; });

    return {iter != classes.end(), 
            std::distance(classes.begin(), iter)};
}

IDable::idx_in_cont ParserState::addClass(unsigned int nameID, bool isDefined)
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

    return curClass.getID();
}
// TODO: CHECKER: after parsing the whole prog (all files):
// run though classes and see that none have isDefined == false
// classID as idx in classes container
void ParserState::setCurParseClass(unsigned int classID)
{
    assert(classID < classes.size());
    curParseClass = &(classes[classID]);
    // this is the class we are currently defining
    curParseClass->setIsDefined(true);
}

void ParserState::addCurParseClassFieldVar(unsigned int nameID, LangDataTypes valueType)
{
    getCurParseClass()->addFieldVar(nameID, valueType);
}
void ParserState::addCurParseClassStaticVar(unsigned int nameID, LangDataTypes valueType)
{
    getCurParseClass()->addStaticVar(nameID, valueType);
}

bool ParserState::addCurParseClassFunc(unsigned int nameID, LangDataTypes ldType_ret,
    bool isMethod, bool isCtor)
{
    return getCurParseClass()->addFunc(nameID, ldType_ret, isMethod, isCtor);
}

// only for loading system library symbols
bool ParserState::addFuncToClass(int classID, unsigned int nameID, LangDataTypes ldType_ret,
    bool isMethod, bool isCtor)
{
    getClassByID(classID).addFunc(nameID, ldType_ret, isMethod, isCtor);
}

FunctionData *ParserState::getCurParseFunc() const
{
    auto *curParseClass = getCurParseClass();
    if (curParseClass == NULL)
        return NULL;
    return curParseClass->getLastFunc();
}
void ParserState::addCurParseFuncPar(unsigned int nameID, LangDataTypes ldType_par)
{
    auto *curParseClass = getCurParseClass();
    if (curParseClass != NULL)
    {
        curParseClass->addFuncPar(nameID, ldType_par);
    }
}
void ParserState::addLocalScopeFramesTopVar(unsigned int nameID, LangDataTypes valueType)
{
    getCurParseFunc()->addLocalScopeFramesTopVar(nameID, valueType);
}
bool ParserState::popLocalScopeFramesTop()
{   
    return getCurParseFunc()->popLocalScopeFramesTop(); 
}
std::tuple<bool, unsigned int> ParserState::containsArg(int identNameID)
{
    return getCurParseFunc()->containsArg(identNameID);
}
std::tuple<bool, unsigned int> ParserState::containsLocal(int identNameID)
{
    return getCurParseFunc()->containsLocal(identNameID);
}
std::tuple<bool, unsigned int> ParserState::containsField(int identNameID)
{
    return getCurParseClass()->containsField(identNameID);
}
std::tuple<bool, unsigned int> ParserState::containsStatic(int identNameID)
{
    return getCurParseClass()->containsStatic(identNameID);
}

const VariableData &ParserState::getArgVar(unsigned int idx) const
{
    return getCurParseFunc()->getArgVar(idx);
}
const VariableData &ParserState::getLocalVar(unsigned int idx) const
{
    return getCurParseFunc()->getLocalVar(idx);
}
const VariableData &ParserState::getFieldVar(unsigned int idx) const
{
    return getCurParseClass()->getFieldVar(idx);
}
const VariableData &ParserState::getStaticVar(unsigned int idx) const
{
    return getCurParseClass()->getStaticVar(idx);
}

ParserState::ParserState()
{
    arrayLib_classID = 0;
    identifiers = NULL;
    classes.clear();
    resetNonShared();
}

void ParserState::setTokens(tokensVect *tokensPar)
{
    tokens = tokensPar;
}
void ParserState::setIdentifiers(identifierVect *identifiersPar)
{
    identifiers = identifiersPar;
}

void ParserState::resetNonShared()
{
    tokens = NULL;
    identifiers = NULL;
    curTokenId = 0;
    tokensFinished = false;
    curParseClass = NULL;
    layerCoeff = 0;
    arrayEnteryNum = 0;
    fsmFinished = false;
    fsmFinishedCorrectly = true;
    fsmCurState = ParseFsmStates::sINIT;
    declaringLocals = false;

    while (!pendParentNodes.empty())
        pendParentNodes.pop();

    // NOTE: we are not resetting classes,
    // because they are SHARED between
    // multiple translation units.
    
    // This function only resets the individual
    // state for each input source file.
}

void ParserState::addStackTopChild(AstNode *child)
{
    pendParentNodes.top()->addChild(child);
}
void ParserState::addStackTop(AstNode *newTop)
{
    pendParentNodes.push(newTop);
}
bool ParserState::popStackTop()
{   
    if (pendParentNodes.empty())
        return false;
    pendParentNodes.pop();
    return true;
}
AstNode *ParserState::getStackTop()
{
    if (pendParentNodes.empty())
        return NULL;
    return pendParentNodes.top();
}

LangDataTypes ParserState::checkCreateUserDefinedDataType(const TokenData &token, bool onlyCheck)
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

std::tuple<VarScopes, unsigned int> ParserState::findVariable(unsigned int identNameID)
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

    // might be a function or class name,
    // this scenario taken care of in the caller
    return {VarScopes::scUNKNOWN, 0};
}

std::tuple<bool, unsigned int> ParserState::findFunction(int identNameID, int classID)
{
    if (classID >= 0)
    {
        return getClassByID(classID).containsFunc(identNameID);
    }
    return getCurParseClass()->containsFunc(identNameID);
}

const VariableData &ParserState::getVariableInScope(VarScopes varScope, unsigned int idx) const
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

bool ParserState::advance(unsigned int step)
{
    if (curTokenId < tokens->size()-step)
    {
        curTokenId+=step;
        return true;
    }
    tokensFinished = true;
    return false;
}

TokenData &ParserState::advanceAndGet(unsigned int step)
{
    advance(step);
    return getCurToken();
}

std::tuple<bool, TokenData&> ParserState::lookAheadGet()
{
    if (curTokenId + 1 >= tokens->size())
        return {false, tokens->at(0)};

    return {true, tokens->at(curTokenId + 1)};
}

bool ParserState::fsmTerminate(bool finishedCorrectly)
{
    fsmFinished = true;
    fsmFinishedCorrectly = finishedCorrectly;
    return fsmFinishedCorrectly;
}