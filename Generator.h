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

    void writeFile()
    {
        std::ofstream outFile;
        outFile.open(outFilePath, std::ios::out);
        if (!outFile)
            return; 

        for (const auto &line : outputLines)
        {
            outFile << line;
        }
        outFile << "\r\n";

        outFile.close();
    }
    
    void genForNode(AstNode *astNode)
    {
        genMapIter it = generationLookup.find(astNode->aType);
        if (it == generationLookup.end())
            return;
        
        const std::string &codeLine = it->second;
        size_t wcardPos = codeLine.find('$');
        if (wcardPos == std::string::npos)
        {
            outputLines.push_back(codeLine);
            return;
        }

        // replacing $ with node value(data)
        std::string res = codeLine.substr(0, wcardPos) 
            + std::to_string(astNode->getNodeValue()) 
            + codeLine.substr(wcardPos + 1);
        outputLines.push_back(res);
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