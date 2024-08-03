#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <cassert>
#include <algorithm>

#include "JackCompilerTypes.h"
#include "DEBUG_CONTROL.h"

class Generator
{
private:
    std::vector<std::string> outputLines;
    std::string outFilePath;
public:
    bool init(const sourceFileNameType &srcFileName)
    {
        std::size_t dotPos = srcFileName.find_last_of(".");
        if (dotPos == std::string::npos)
        {
            outFilePath = srcFileName + outFileExt;
            return true;
        }

        std::string srcFileNameNoExt = srcFileName.substr(0,dotPos + 1);
        outFilePath = srcFileNameNoExt + outFileExt;
        return true;
    }

    Generator(const sourceFileNameType &srcFileName)
    {
        init(srcFileName);
    }

    void writeCode()
    {
        std::ifstream outFile;
        outFile.open(outFilePath, std::ios::in);
        if (!outFile)
            return; 

        // printing

        outFile.close();
    }
    
    void genForNode(AstNode *astNode)
    {
        
    }

    void generateCode(AstNode *curRoot)
    {
        for (auto childNode : curRoot->nChildNodes)
        {
            generateCode(childNode);
        }
        if (curRoot->generatesCode)
            genForNode(curRoot);
    }
};