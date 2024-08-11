#include <stack>
#include <vector>

enum class TokenTypes : unsigned int;
struct FunctionData
{
    unsigned int nameID;
    TokenTypes tType;
};