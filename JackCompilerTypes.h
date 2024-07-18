#ifndef _JACK_COMPILER_TYPES_
#define _JACK_COMPILER_TYPES_

#include <vector>
#include <map>
#include <string>
#include <optional>

enum class TokenTypes : unsigned int;
struct TokenData;

typedef std::vector<TokenData> tokensVect;
typedef std::vector<std::string> identifierVect;
typedef std::map<std::string, TokenTypes>::iterator tokenMapIter;

#include "LexerTypes.h"
#include "ParserTypes.h"

#endif