#include "Generator.h"

bool Generator::init(const sourceFileNameType &srcFileName)
{
    std::size_t dotPos = srcFileName.find_last_of(".");
    if (dotPos == std::string::npos)
    {
        outFilePath = srcFileName + "." + outFileExt;
        return true;
    }

    std::string srcFileNameNoExt = srcFileName.substr(0,dotPos + 1);
    outFilePath = srcFileNameNoExt + outFileExt;
    return true;
}

Generator::Generator(const sourceFileNameType &srcFileName, const identifierVect &identifiers)
    : identifiers(identifiers) 
{
    init(srcFileName);
}

void Generator::writeFile()
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

void Generator::genForNode(AstNode *astNode)
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
        + astNode->getNodeValueAsString()
        + codeLine.substr(wcardPos + 1);
    outputLines.push_back(res);
}

void Generator::generateCode(AstNode *curRoot)
{
    for (auto childNode : curRoot->nChildNodes)
    {
        generateCode(childNode);
    }
    if (curRoot->generatesCode)
        genForNode(curRoot);
}

void Generator::generateAndWrite(AstNode *curRoot)
{
    generateCode(curRoot);
    writeFile();
}
