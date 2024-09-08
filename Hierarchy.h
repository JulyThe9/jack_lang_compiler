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

struct ScopeFrame
{
private:
    std::vector<VariableData> localVars;
    std::vector<VariableData> argVars;
public:
    void addVarLocal(unsigned int nameID, LangDataTypes valueType)
    {
        assert(isvartype(ldType_to_tType(valueType)));
        localVars.emplace_back(nameID, valueType);
    }
    void addVarArg(unsigned int nameID, LangDataTypes valueType)
    {
        assert(isvartype(ldType_to_tType(valueType)));
        argVars.emplace_back(nameID, valueType);
    }
    std::tuple<bool, unsigned int> containsLocal(unsigned int nameID)
    {
        auto iter = std::find_if(localVars.begin(), localVars.end(), 
            [&nameID](const VariableData &arg) { return arg.nameID == nameID; });

        return {iter != localVars.end(), 
                std::distance(localVars.begin(), iter)};
    }
    std::tuple<bool, unsigned int> containsArg(unsigned int nameID)
    {
        auto iter = std::find_if(argVars.begin(), argVars.end(), 
            [&nameID](const VariableData &arg) { return arg.nameID == nameID; });

        return {iter != argVars.end(),
                std::distance(argVars.begin(), iter)};
    }
};

void addScopeFramesTopVar(unsigned int nameID, ScopeTypes scopeType, LangDataTypes valueType)
{
    if (scopeType == ScopeTypes::ST_LOCAL)
        scopeFrames.top().addVarLocal(nameID, valueType);
    else if (scopeType == ScopeTypes::ST_ARG)
        scopeFrames.top().addVarArg(nameID, valueType);
}
void addNewScopFrame()
{
    // pushing empty
    scopeFrames.push({});
}
bool popScopeFramesTop()
{   
    if (scopeFrames.empty())
        return false;
    scopeFrames.pop();
    return true;
}
ScopeFrame &getScopeFramesTop()
{
    return scopeFrames.top();
}

struct LocalScope
{
    std::stack<VariableData> localVars;
};

struct FunctionData : public LocalScope
{
    unsigned int nameID;
    LangDataTypes ldType_ret;
    std::vector<LangDataTypes> ldType_args;

    // containing class reference?
};

struct ClassData
{
    unsigned int nameID;
    std::vector<FunctionData> funcs;
    
    std::vector<VariableData> staticVars;
    std::vector<VariableData> fieldVars;
};