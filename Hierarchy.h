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
private:
    std::stack<LocalScopeFrame> locScopeFrames;
    void addNewFrame()
    {
        // pushing empty
        locScopeFrames.push({});
    }
    // ScopeTypes scopeType, 
    void addScopeFramesTopVar(unsigned int nameID, LangDataTypes valueType)
    {
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
        return locScopeFrames.top();
    }
};

struct FunctionData : public LocalScopeManager
{
    unsigned int nameID;
    LangDataTypes ldType_ret;
    std::vector<VariableData> ldType_pars;


public:
    FunctionData() {}
    FunctionData(unsigned int nameID, LangDataTypes ldType_ret) 
        : nameID(nameID), ldType_ret(ldType_ret)
    {}

    void addPar(unsigned int nameID, LangDataTypes ldType_par)
    {
        ldType_pars.emplace_back(nameID, ldType_par);
    }

    // containing class reference?
};

struct ClassData
{
    // keeping the default constructor too
    ClassData() {}
    ClassData(unsigned int nameID) : nameID(nameID) {}
    unsigned int nameID;
private:
    std::vector<FunctionData> funcs;
    std::vector<VariableData> staticVars;
    std::vector<VariableData> fieldVars;

public:
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

    bool addFunc(unsigned int nameID, LangDataTypes ldType_ret)
    {
        funcs.emplace_back(nameID, ldType_ret);
    }

    void addFuncPar(unsigned int nameID, LangDataTypes ldType_par)
    {
        if (!funcs.empty())
        {
            funcs.back().addPar(nameID, ldType_par);
        }
    }
};