#ifndef _GENERATOR_
#define _GENERATOR

#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <fstream>

#include "LexerTypes.h"
#include "CheckerTypes.h"
#include "ParserTypes.h"
#include "GeneratorTypes.h"
#include "DEBUG_CONTROL.h"

class Generator
{
private:
    std::vector<std::string> outputLines;
    std::string outFilePath;

    const identifierVect &identifiers;
public:
    bool init(const sourceFileNameType &srcFileName);

    Generator(const sourceFileNameType &srcFileName, const identifierVect &identifiers);

    void writeFile();
    
    void genForNode(AstNode *astNode);

    void generateCode(AstNode *curRoot);

    void generateAndWrite(AstNode *curRoot);
};

#endif