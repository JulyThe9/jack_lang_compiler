#ifndef _HIERARCHY_
#define _HIERARCHY_
#include <stack>
#include <vector>

enum class ScopeTypes : unsigned int
{
    ST_LOCAL = 0,
    ST_ARG
    // static, global
};

// TODO: TYPE-CHECKING
struct VariableData
{
    unsigned int nameID;
    LangDataTypes valueType;
    VariableData(unsigned int nameID, LangDataTypes valueType) :
        nameID(nameID), valueType(valueType)
    {}
};

struct LocalScopeFrame
{
private:
    std::vector<VariableData> localVars;
public:
    std::tuple<bool, unsigned int> containsVar(unsigned int nameID)
    {
        auto iter = std::find_if(localVars.begin(), localVars.end(), 
            [&nameID](const VariableData  &arg){ return arg.nameID == nameID; });

        return {iter != localVars.end(), 
                std::distance(localVars.begin(), iter)};
    }
    void addVar(unsigned int nameID, LangDataTypes valueType)
    {
        assert(isvartype(ldType_to_tType(valueType)));
        localVars.emplace_back(nameID, valueType);
    }
    unsigned int getNumOfLocals() const
    {
        return localVars.size();
    }
};
struct LocalScopeManager
{
protected:
    std::stack<LocalScopeFrame> locScopeFrames;
    void addNewFrame()
    {
        // pushing empty
        locScopeFrames.push({});
    }
    // ScopeTypes scopeType, 
    void addScopeFramesTopVar(unsigned int nameID, LangDataTypes valueType)
    {
        assert(!locScopeFrames.empty());
        locScopeFrames.top().addVar(nameID, valueType);
    }
    bool popScopeFramesTop()
    {   
        if (locScopeFrames.empty())
            return false;
        locScopeFrames.pop();
        return true;
    }
    LocalScopeFrame &getScopeFramesTop()
    {
        assert(!locScopeFrames.empty());
        return locScopeFrames.top();
    }

    std::tuple<bool, unsigned int> containsLocalVar(unsigned int nameID)
    {
        assert(!locScopeFrames.empty());
        return locScopeFrames.top().containsVar(nameID);
    }

    unsigned int getNumOfLocals() const
    {
        // TODO: this approach assumes no nested local scopes withing a function
        // (although there is a stack).
        // Need to change if we want to support them.
        return locScopeFrames.top().getNumOfLocals();
    }
};

struct IDable
{
typedef unsigned int idx_in_cont;
protected:
    IDable() {}
private:
    idx_in_cont ID = 0;
public:
    void setID(idx_in_cont ID)
    {
        this->ID = ID;
    }
    idx_in_cont getID() const
    {
        return ID;
    }
};

struct FunctionData : public LocalScopeManager, public IDable
{
    unsigned int nameID;
    LangDataTypes ldType_ret;
    std::vector<VariableData> argVars;

public:
    FunctionData() 
    {
        addNewFrame();
    }
    FunctionData(unsigned int nameID, LangDataTypes ldType_ret) 
        : nameID(nameID), ldType_ret(ldType_ret)
    {
        addNewFrame();
    }

    void addPar(unsigned int nameID, LangDataTypes ldType_par)
    {
        argVars.emplace_back(nameID, ldType_par);
    }

    void addLocalScopeFramesTopVar(unsigned int nameID, LangDataTypes valueType)
    {
        addScopeFramesTopVar(nameID, valueType);
    }
    bool popLocalScopeFramesTop()
    {
        return popScopeFramesTop();
    }

    std::tuple<bool, unsigned int> containsArg(unsigned int identNameID)
    {
        auto iter = std::find_if(argVars.begin(), argVars.end(), 
            [&identNameID](const VariableData  &arg){ return arg.nameID == identNameID; });

        return {iter != argVars.end(), 
                std::distance(argVars.begin(), iter)};
    }
    std::tuple<bool, unsigned int> containsLocal(unsigned int identNameID)
    {
        return containsLocalVar(identNameID);
    }

    unsigned int getNumOfPars() const
    {
        return argVars.size();
    }

    unsigned int getNumOfLocals() const
    {
        return LocalScopeManager::getNumOfLocals();
    }
};

// TODO: public or what kind of inheritance?
struct ClassData : IDable
{
    // keeping the default constructor too
    ClassData() : isDefined(false) {}
    ClassData(unsigned int nameID) : nameID(nameID), isDefined(false) {}
    unsigned int nameID;
private:
    bool isDefined = false;
    bool ctorAdded = false;
    std::vector<FunctionData> funcs;
    std::vector<VariableData> fieldVars;
    static std::vector<VariableData> staticVars;
public:
    void setIsDefined(bool isDefined)
    {
        this->isDefined = isDefined;
    }
    bool getIsDefined()
    {
        return isDefined;
    }
    // Getter for funcs
    const std::vector<FunctionData>& getFuncs() const {
        return funcs;
    }
    // Getter for staticVars
    const std::vector<VariableData>& getStaticVars() const {
        return staticVars;
    }

    // Getter for fieldVars
    const std::vector<VariableData>& getFieldVars() const {
        return fieldVars;
    }

    LangDataTypes asLangDataType()
    {
        return (LangDataTypes)getID();
    }

    FunctionData *getFunc()
    {
        if (funcs.empty())
            return NULL;
        return &(funcs.back());
    }

    bool addFunc(unsigned int nameID, LangDataTypes ldType_ret, bool isCtor = false)
    {
        if (isCtor)
        {
            if (ctorAdded)
                return false;
            
            ctorAdded = true;
        }

        auto &func = funcs.emplace_back(nameID, ldType_ret);
        func.setID(funcs.size()-1);
        return true;
    }

    void addFuncPar(unsigned int nameID, LangDataTypes ldType_par)
    {
        if (!funcs.empty())
        {
            funcs.back().addPar(nameID, ldType_par);
        }
    }

    void addFieldVar(unsigned int nameID, LangDataTypes valueType)
    {
        fieldVars.emplace_back(nameID, valueType);
    }
    void addStaticVar(unsigned int nameID, LangDataTypes valueType) const
    {
        staticVars.emplace_back(nameID, valueType);
    }
    
    std::tuple<bool, unsigned int> containsField(unsigned int identNameID) const
    {
        auto iter = std::find_if(fieldVars.begin(), fieldVars.end(), 
            [&identNameID](const VariableData  &field){ return field.nameID == identNameID; });
        return {iter != fieldVars.end(), 
                std::distance(fieldVars.begin(), iter)};
    }
    std::tuple<bool, unsigned int> containsStatic(unsigned int identNameID) const
    {
        auto iter = std::find_if(staticVars.begin(), staticVars.end(), 
            [&identNameID](const VariableData  &field){ return field.nameID == identNameID; });
        return {iter != staticVars.end(), 
                std::distance(staticVars.begin(), iter)};
    }
    std::tuple<bool, unsigned int> containsFunc(unsigned int identNameID) const
    {
        auto iter = std::find_if(funcs.begin(), funcs.end(), 
            [&identNameID](const FunctionData &func){ return func.nameID == identNameID; });
        return {iter != funcs.end(), 
                std::distance(funcs.begin(), iter)};
    }
};
std::vector<VariableData> ClassData::staticVars;

#endif