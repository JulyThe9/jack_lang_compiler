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
    std::vector<VariableData> localVars;
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
    idx_in_cont getID()
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

    // containing class reference?
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
    std::vector<FunctionData> funcs;
    std::vector<VariableData> staticVars;
    std::vector<VariableData> fieldVars;

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

    FunctionData *getFunc()
    {
        if (funcs.empty())
            return NULL;
        return &(funcs.back());
    }

    void addFunc(unsigned int nameID, LangDataTypes ldType_ret)
    {
        auto &func = funcs.emplace_back(nameID, ldType_ret);
        func.setID(funcs.size()-1);
    }

    void addFuncPar(unsigned int nameID, LangDataTypes ldType_par)
    {
        if (!funcs.empty())
        {
            funcs.back().addPar(nameID, ldType_par);
        }
    }
};

#endif