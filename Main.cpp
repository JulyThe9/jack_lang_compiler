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
#include "Parser.h"
#include "Generator.h"
#include "DEBUG_CONTROL.h"
#include "/home/synthwave09/dev/nand2tetris/projects/06/UsefulString.h"

namespace fs = std::filesystem;

class Lexer
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

    bool lexNextLine(std::ifstream *jackFile, lexerState &lexState)
    {
        std::string line;
        if (!std::getline(*jackFile, line))
        {
            moreLinesComing = false;
            return false;
        }
        std::cout << line << '\n';
        return lexLine(line, lexState);
    }

    void initStateBeh(UsefulString &ustr, lexerState &lexState)
    {
        ustr.skipSpaces();
        if (ustr.isEol())
        {
            lexState.fsmFinished = true;
            return;
        }

        char c = ustr.getChar();
        if (isalpha(c) || c == '_')
        {
            ustr.fwd();
            lexState.addBuff(c);
            lexState.fsmCurState = LexFsmStates::sLETTERS;
        }
        else if (isdigit(c))
        {
            ustr.fwd();
            lexState.addBuff(c);
            lexState.fsmCurState = LexFsmStates::sDIGITS;
        }
        else
        {
            // epsilon transition
            lexState.fsmCurState = LexFsmStates::sSYMBOL;
        }
    }
    void lettersStateBeh(UsefulString &ustr, lexerState &lexState)
    {
        if (ustr.isEol())
        {
            lexState.fsmFinished = true;
            return;
        }

        char c = ustr.getChar();
        if (isalpha(c) || c == '_')
        {
            ustr.fwd();
            lexState.addBuff(c);
            lexState.fsmCurState = LexFsmStates::sLETTERS;
        }
        else if (isdigit(c))
        {
            ustr.fwd();
            lexState.addBuff(c);
            lexState.fsmCurState = LexFsmStates::sDIGITS_AFTER_ALPHA;
        }
        else
        {
            // epsilon transition
            lexState.fsmCurState = LexFsmStates::sSYMBOL;
        }
    }
    void digitsAfterAlphaStateBeh(UsefulString &ustr, lexerState &lexState)
    {
        if (ustr.isEol())
        {
            lexState.fsmFinished = true;
            return;
        }

        char c = ustr.getChar();
        if (isdigit(c))
        {
            ustr.fwd();
            lexState.addBuff(c);
            lexState.fsmCurState = LexFsmStates::sDIGITS_AFTER_ALPHA;
        }
        else if (isalpha(c) || c == '_')
        {
            ustr.fwd();
            lexState.addBuff(c);
            lexState.fsmCurState = LexFsmStates::sLETTERS;
        }
        else
        {
            // epsilon transition
            lexState.fsmCurState = LexFsmStates::sSYMBOL;
        }
    }
    void digitsStateBeh(UsefulString &ustr, lexerState &lexState)
    {
        if (ustr.isEol())
        {
            lexState.fsmFinished = true;
            return;
        }

        char c = ustr.getChar();
        if (isdigit(c))
        {
            ustr.fwd();
            lexState.addBuff(c);
            lexState.fsmCurState = LexFsmStates::sDIGITS;
        }
        else
        {
            // epsilon transition
            lexState.fsmCurState = LexFsmStates::sSYMBOL;
        }
    }

    void handleBuffer(lexerState &lexState)
    {
        std::string inBuff(lexState.buffer, lexState.bIdx);
        tokenMapIter it = tokenLookup.find(inBuff);

        // MEMORY LEAK SOMEWHERE: fail_1.jack
        // lexState.buffer[0] != 0, but
        // lexState.bIdx == 0
        // occurs sometimes

        // known keyword
        if (it != tokenLookup.end())
        {
            lexState.tokens.push_back(it->second);
            return;
        }

        // a number
        char * pEnd = NULL;
        int num = strtol(inBuff.c_str(), &pEnd, 10);
        if (!*pEnd)
        {
            lexState.tokens.emplace_back(TokenTypes::tNUMBER, num);
        }
        else
        {
            const auto identPosNum = vectContains(lexState.identifiers, inBuff);
            // known identifier
            if (identPosNum >= 0)
            {
                lexState.tokens.emplace_back(TokenTypes::tIDENTIFIER, identPosNum);
            }
            else
            {
                lexState.identifiers.push_back(inBuff);
                lexState.tokens.emplace_back(TokenTypes::tIDENTIFIER, lexState.identifiers.size()-1);
            }
        }
        lexState.lastOperTermIsOper = false;
    }

    void symbolStateBeh(UsefulString &ustr, lexerState &lexState)
    {
        if (!lexState.buffEmpty())
        {
            handleBuffer(lexState);
            lexState.flush();
        }

        ustr.skipSpaces();
        if (ustr.isEol())
        {
            lexState.fsmFinished = true;
            return;
        }

        char c = ustr.getChar();
        if (isalpha(c) || c == '_' || isdigit(c))
        {
            // epsilon transition
            lexState.fsmCurState = LexFsmStates::sINIT;
            return;
        }

        if (c == '/')
        {
            ustr.fwd();
            lexState.fsmCurState = LexFsmStates::sCOMMENT;
            return;
        }

        std::string s(1, c);
        tokenMapIter it = tokenLookup.find(s);
        if (it != tokenLookup.end())
        {
            if (it->first[0] == ')')
                lexState.lastOperTermIsOper = false;

            if (it->second == TokenTypes::tMINUS)
            {
                if (lexState.lastOperTermIsOper)
                    lexState.tokens.push_back(TokenTypes::tNEG_MINUS);
                else
                {
                    lexState.tokens.push_back(TokenTypes::tMINUS);
                    lexState.lastOperTermIsOper = true;
                }
            }
            else
            {
                lexState.tokens.push_back(it->second);
                if (isbinaryperator(it->second))
                    lexState.lastOperTermIsOper = true;
            }   
        }
        else
        {
            lexState.tokens.push_back(TokenTypes::tUNKNOWN_SYMBOL);
        }

        ustr.fwd();
        // epsilon transition
        lexState.fsmCurState = LexFsmStates::sSYMBOL;
    }

    void commentStateBeh(UsefulString &ustr, lexerState &lexState)
    {
        if (ustr.isEol())
        {
            lexState.tokens.push_back(TokenTypes::tDIV);
            lexState.fsmFinished = true;
            return;
        }
        
        char c = ustr.getChar();
        // no more tokens on this line, finishing
        if (c == '/')
        {
            lexState.commentOpen = true;
            lexState.fsmFinished = true;
        }
        else if (c == '*')
        {
            lexState.commentOpen = true;
            lexState.mlineComment = true;

            ustr.fwd();
            lexState.fsmCurState = LexFsmStates::sMLINE_COMMENT;
        }
        else
        {
            lexState.tokens.push_back(TokenTypes::tDIV);

            ustr.fwd();
            lexState.fsmCurState = LexFsmStates::sSYMBOL;
        }
    }

    void mlineCommentStateBeh(UsefulString &ustr, lexerState &lexState)
    {
        if (ustr.isEol())
        {
            lexState.fsmFinished = true;
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
            lexState.commentOpen = false;
            lexState.mlineComment = false;
            
            ustr.fwd();
            lexState.fsmCurState = LexFsmStates::sINIT;
        }
    }

    bool lexLine(const std::string &line, lexerState &lexState)
    {
        UsefulString ustr(line);

        std::stringstream debug_strm;
        while (!lexState.fsmFinished)
        {
            debug_strm.str(std::string());
            switch (lexState.fsmCurState)
            {
            case LexFsmStates::sINIT:
                debug_strm << "sINIT hits\n";
                initStateBeh(ustr, lexState);
                break;

            case LexFsmStates::sLETTERS:
                debug_strm << "sLETTERS hits\n";
                lettersStateBeh(ustr, lexState);
                break;

            case LexFsmStates::sDIGITS_AFTER_ALPHA:
                std::cout << "sDIGITS_AFTER_ALPHA hits\n";
                digitsAfterAlphaStateBeh(ustr, lexState);
                break;

            case LexFsmStates::sDIGITS:
                debug_strm << "sDIGITS hits\n";
                digitsStateBeh(ustr, lexState);
                break;

            case LexFsmStates::sSYMBOL:
                debug_strm << "sSYMBOL hits\n";
                symbolStateBeh(ustr, lexState);
                break;

            case LexFsmStates::sCOMMENT:
                debug_strm << "sCOMMENT hits\n";
                commentStateBeh(ustr, lexState);
                break;

            case LexFsmStates::sMLINE_COMMENT:
                debug_strm << "sMLINE_COMMENT hits\n";
                mlineCommentStateBeh(ustr, lexState);
                break;
            }
#ifdef LEXER_DEBUG
            std::cout << debug_strm.str();
#endif
        }

        if (!lexState.buffEmpty())
        {
            handleBuffer(lexState);
        }
#ifdef LEXER_DEBUG
        std::cout << "Finished tokenizing line: " << line << '\n';
        std::cout << "Got this many tokens now: " << lexState.tokens.size() << '\n';
#endif
        return true;
    }
};

// BY COPY BECAUSE WE DON'T WANT TO RELY ON LEXSTATE AT THIS POINT
void parserCtrl(tokensVect tokens, identifierVect identifiers)
{
    Parser parser;
    Generator generator("main.jack", identifiers);
     auto *astRoot = parser.buildAST(tokens, identifiers);
#ifdef DEBUG
    parser.printAST();
#endif
    generator.generateCode(astRoot);
    generator.writeFile();
}

bool compilerCtrl(const char *pathIn, const char *pathOut)
{
    Lexer lexer;
    if (!lexer.init(pathIn))
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

    // TODO: make sure multiple files (still) work
    lexerState lexState;
    for (const auto &filePath : lexer.getFilePaths())
    {
        std::ifstream jackFile;
        jackFile.open(filePath, std::ios::in);
        if (!jackFile)
            continue;

        std::cout << filePath << '\n';
        lexer.setCurFileName(filePath);

        // NOTE: used to be here
        //lexerState lexState;
        while (lexer.getMoreLinesComing())
        {
            const bool res = lexer.lexNextLine(&jackFile, lexState);

            // TODO: needed?
            // parsing of the current line failed
            if (!res)
            {
                lexState.reset();
                continue;
            }

#ifdef LEXER_DEBUG
            printTokens(lexState.tokens);
            printIdentifiers(lexState.identifiers);
#endif
            lexState.reset();
        }
        lexer.resetForFile();
        jackFile.close();
    }

#ifndef LEXER_ONLY
    parserCtrl(lexState.tokens, lexState.identifiers);
#endif

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