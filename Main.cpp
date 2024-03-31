#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include <fstream>
#include <vector>
#include <filesystem>
#include <cassert>
#include <algorithm>

#include "Utils.h"
#include "JackCompilerTypes.h"
#include "/home/synthwave09/dev/nand2tetris/projects/06/UsefulString.h"

#define DEBUG 1

namespace fs = std::filesystem;

class Parser
{
private:
    std::string curLine;
    bool moreLinesComing = true;
    unsigned int labelUniqueId = 0;

    std::string curFileName;
    std::vector<std::string> filePaths;
    bool inputIsDir = false;

private:
    bool isJackFile(const std::string &path) const
    {
        std::string pathStr(path);
        std::string fileExt = pathStr.substr(pathStr.find_last_of("."));
        return fileExt == ".jack";
    }

public:
    bool init(const char *path)
    {
        std::error_code ec;
        if (fs::is_directory(path, ec))
        {
            inputIsDir = true;
            for (const auto &entry : fs::directory_iterator(path))
            {
                if (!fs::is_directory(entry.path(), ec) && isJackFile(entry.path().native()))
                    filePaths.push_back(entry.path().native());
            }
        }
        else
        {
            filePaths.push_back(path);
        }
        return true;
    }

    bool getMoreLinesComing() const
    {
        return moreLinesComing;
    }
    const std::vector<std::string> &getFilePaths() const
    {
        return filePaths;
    }
    const bool getInputIsDir() const
    {
        return inputIsDir;
    }

    void setCurFileName(std::string curFilePath)
    {
        // +1 to discard '/'
        size_t idx = curFilePath.find_first_of("/");
        assert(idx + 1 < curFilePath.size());
        std::string curDirFp = curFilePath.substr(idx + 1);

        // discarding the extension
        const size_t fNameStartIdx = 0;
        curFileName = curDirFp.substr(fNameStartIdx, curDirFp.find_first_of(".") - fNameStartIdx);
    }

    const std::string &getCurFileName() const
    {
        return curFileName;
    }

    void resetForFile()
    {
        moreLinesComing = true;
    }

    std::string getCurLine() const
    {
        return curLine;
    }

    bool parseNextLine(std::ifstream *jackFile, parseState &pState)
    {
        std::string line;
        if (!std::getline(*jackFile, line))
        {
            moreLinesComing = false;
            return false;
        }
        std::cout << line << '\n';
        return parseLine(line, pState);
    }

    void initStateBeh(UsefulString &ustr, parseState &pState)
    {
        ustr.skipSpaces();
        if (ustr.isEol())
        {
            pState.fsmFinished = true;
            return;
        }

        char c = ustr.getChar();
        if (isalpha(c) || c == '_')
        {
            ustr.fwd();
            pState.addBuff(c);
            pState.fsmCurState = FsmStates::sLETTERS;
        }
        else if (isdigit(c))
        {
            ustr.fwd();
            pState.addBuff(c);
            pState.fsmCurState = FsmStates::sDIGITS;
        }
        else
        {
            // epsilon transition
            pState.fsmCurState = FsmStates::sSYMBOL;
        }
    }
    void lettersStateBeh(UsefulString &ustr, parseState &pState)
    {
        if (ustr.isEol())
        {
            pState.fsmFinished = true;
            return;
        }

        char c = ustr.getChar();
        if (isalpha(c) || c == '_')
        {
            ustr.fwd();
            pState.addBuff(c);
            pState.fsmCurState = FsmStates::sLETTERS;
        }
        else if (isdigit(c))
        {
            ustr.fwd();
            pState.addBuff(c);
            pState.fsmCurState = FsmStates::sDIGITS_AFTER_ALPHA;
        }
        else
        {
            // epsilon transition
            pState.fsmCurState = FsmStates::sSYMBOL;
        }
    }
    void digitsAfterAlphaStateBeh(UsefulString &ustr, parseState &pState)
    {
        if (ustr.isEol())
        {
            pState.fsmFinished = true;
            return;
        }

        char c = ustr.getChar();
        if (isdigit(c))
        {
            ustr.fwd();
            pState.addBuff(c);
            pState.fsmCurState = FsmStates::sDIGITS_AFTER_ALPHA;
        }
        else if (isalpha(c) || c == '_')
        {
            ustr.fwd();
            pState.addBuff(c);
            pState.fsmCurState = FsmStates::sLETTERS;
        }
        else
        {
            // epsilon transition
            pState.fsmCurState = FsmStates::sSYMBOL;
        }
    }
    void digitsStateBeh(UsefulString &ustr, parseState &pState)
    {
        if (ustr.isEol())
        {
            pState.fsmFinished = true;
            return;
        }

        char c = ustr.getChar();
        if (isdigit(c))
        {
            ustr.fwd();
            pState.addBuff(c);
            pState.fsmCurState = FsmStates::sDIGITS;
        }
        else
        {
            // epsilon transition
            pState.fsmCurState = FsmStates::sSYMBOL;
        }
    }

    void handleBuffer(parseState &pState)
    {
        std::string inBuff(pState.buffer, pState.bIdx);
        tokenMapIter it = tokenLookup.find(inBuff);
        
        // known keyword
        if (it != tokenLookup.end())
        {
            pState.tokens.push_back(it->second);
            return;
        }

        // a number
        char * pEnd = NULL;
        int num = strtol(inBuff.c_str(), &pEnd, 10);
        if (!*pEnd)
        {
            pState.tokens.emplace_back(TokenTypes::tNUMBER, num);
            return;
        }

        const auto identPosNum = vectContains(pState.identifiers, inBuff);
        // known identifier
        if (identPosNum >= 0)
        {
            pState.tokens.emplace_back(TokenTypes::tIDENTIFIER, identPosNum);
        }
        else
        {
            pState.identifiers.push_back(inBuff);
            pState.tokens.emplace_back(TokenTypes::tIDENTIFIER, pState.identifiers.size()-1);
        }
    }

    void symbolStateBeh(UsefulString &ustr, parseState &pState)
    {
        if (!pState.buffEmpty())
        {
            handleBuffer(pState);
            pState.flush();
        }

        ustr.skipSpaces();
        if (ustr.isEol())
        {
            pState.fsmFinished = true;
            return;
        }

        char c = ustr.getChar();
        if (isalpha(c) || c == '_' || isdigit(c))
        {
            // epsilon transition
            pState.fsmCurState = FsmStates::sINIT;
            return;
        }

        if (c == '/')
        {
            ustr.fwd();
            pState.fsmCurState = FsmStates::sCOMMENT;
            return;
        }

        std::string s(1, c);
        tokenMapIter it = tokenLookup.find(s);
        //tUNKNOWN_SYMBOL
        if (it != tokenLookup.end())
        {
            pState.tokens.push_back(it->second);
        }
        else
        {
            pState.tokens.push_back(TokenTypes::tUNKNOWN_SYMBOL);
        }

        ustr.fwd();
        // epsilon transition
        pState.fsmCurState = FsmStates::sSYMBOL;
    }

    void commentStateBeh(UsefulString &ustr, parseState &pState)
    {
        if (ustr.isEol())
        {
            pState.tokens.push_back(TokenTypes::tDIV);
            pState.fsmFinished = true;
            return;
        }
        
        char c = ustr.getChar();
        // no more tokens on this line, finishing
        if (c == '/')
        {
            pState.commentOpen = true;
            pState.fsmFinished = true;
        }
        else if (c == '*')
        {
            pState.commentOpen = true;
            pState.mlineComment = true;

            ustr.fwd();
            pState.fsmCurState = FsmStates::sMLINE_COMMENT;
        }
        else
        {
            pState.tokens.push_back(TokenTypes::tDIV);

            ustr.fwd();
            pState.fsmCurState = FsmStates::sSYMBOL;
        }
    }

    void mlineCommentStateBeh(UsefulString &ustr, parseState &pState)
    {
        if (ustr.isEol())
        {
            pState.fsmFinished = true;
            return;
        }

        char c = ustr.getChar();
        std::cout << c << '\n';
        
        bool commentClosed = false;
        do
        {
            std::cout << "Enter fwd?\n";
            c = ustr.getChar();
            if (c == '*')
            {
                if (!ustr.fwd())
                    break;
                c = ustr.getChar();
                // comment end
                if (c == '/')
                {
                    commentClosed = true;
                    break;
                }
            }
        } while (ustr.fwd());

        if (commentClosed)
        {
            pState.commentOpen = false;
            pState.mlineComment = false;
            
            ustr.fwd();
            pState.fsmCurState = FsmStates::sINIT;
        }
    }

    bool parseLine(const std::string &line, parseState &pState)
    {
        UsefulString ustr(line);
        while (!pState.fsmFinished)
        {
            switch (pState.fsmCurState)
            {
            case FsmStates::sINIT:
                std::cout << "sINIT hits\n";
                initStateBeh(ustr, pState);
                break;

            case FsmStates::sLETTERS:
                std::cout << "sLETTERS hits\n";
                lettersStateBeh(ustr, pState);
                break;

            case FsmStates::sDIGITS_AFTER_ALPHA:
                std::cout << "sDIGITS_AFTER_ALPHA hits\n";
                digitsAfterAlphaStateBeh(ustr, pState);
                break;

            case FsmStates::sDIGITS:
                std::cout << "sDIGITS hits\n";
                digitsStateBeh(ustr, pState);
                break;

            case FsmStates::sSYMBOL:
                std::cout << "sSYMBOL hits\n";
                symbolStateBeh(ustr, pState);
                break;

            case FsmStates::sCOMMENT:
                std::cout << "sCOMMENT hits\n";
                commentStateBeh(ustr, pState);
                break;

            case FsmStates::sMLINE_COMMENT:
                std::cout << "sMLINE_COMMENT hits\n";
                mlineCommentStateBeh(ustr, pState);
                break;
            }
        }

        if (!pState.buffEmpty())
        {
            handleBuffer(pState);
        }

        std::cout << "Finished parsing\n";
        std::cout << "Got this many tokens now: " << pState.tokens.size() << '\n';\
        return true;
    }
};

bool compilerCtrl(const char *pathIn, const char *pathOut)
{
    Parser parser;
    if (!parser.init(pathIn))
    {
        // error
        return 1;
    }

    auto printTokens = [](const tokensVect &tokens)
    {
        for (auto elem : tokens)
        {
            std::cout << "Token type: "  << (unsigned int)elem.tType << '\n';
            if (elem.tVal.has_value())    
                std::cout << "Token val: " << elem.tVal.value() << '\n';
            else
                std::cout << "Token val: none\n";
        }
        std::cout << '\n';
    };
    auto printIdentifiers = [](const identifierVect &identifiers)
    {
        int i = 0;
        for (auto elem : identifiers)
        {
            std::cout << "Identifier idx: "  << i << '\n';
            std::cout << "Identifier val: "  << elem << '\n';            
            i++;
        }
        std::cout << '\n';
    };

    for (const auto &filePath : parser.getFilePaths())
    {
        std::ifstream jackFile;
        jackFile.open(filePath, std::ios::in);
        if (!jackFile)
            continue;

        std::cout << filePath << '\n';
        parser.setCurFileName(filePath);

        parseState pState;
        while (parser.getMoreLinesComing())
        {
            const bool res = parser.parseNextLine(&jackFile, pState);

            // TODO: needed?
            // parsing of the current line failed
            if (!res)
            {
                pState.reset();
                continue;
            }

#ifdef DEBUG
            printTokens(pState.tokens);
            printIdentifiers(pState.identifiers);
#endif
            pState.reset();
        }
        parser.resetForFile();
        jackFile.close();
    }

    return true;
}

int main(int argc, char *argv[])
{
    if (argc < 3)
        return 1;

    if (!compilerCtrl(argv[1], argv[2]))
    {
        return 1;
    }

    return 0;
}