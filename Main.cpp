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

    static std::tuple<unsigned int, unsigned int, unsigned int> loadSysLibIdents(lexerState &lexState)
    {
        lexState.identifiers.push_back("Array");
        lexState.identifiers.push_back("new");
        lexState.identifiers.push_back("this");
        return {0,1,2};
    }

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
            lexState.tokens.emplace_back(lexState.lexedLineIdx, it->second);
            return;
        }

        // a number
        char * pEnd = NULL;
        int num = strtol(inBuff.c_str(), &pEnd, 10);
        if (!*pEnd)
        {
            lexState.tokens.emplace_back(lexState.lexedLineIdx, TokenTypes::tNUMBER, num);
        }
        else
        {
            const auto identPosNum = vectContains(lexState.identifiers, inBuff);
            // known identifier
            if (identPosNum >= 0)
            {
                lexState.tokens.emplace_back(lexState.lexedLineIdx, 
                    TokenTypes::tIDENTIFIER, identPosNum);
            }
            else
            {
                lexState.identifiers.push_back(inBuff);
                lexState.tokens.emplace_back(lexState.lexedLineIdx, 
                    TokenTypes::tIDENTIFIER, lexState.identifiers.size()-1);
            }
        }
        // Only considered a term (alhpanumeric)
        // if we are on the right handside.
        // TODO: see if can be moved too Parser,
        // otherwise we are bringing semantics
        // to syntactic analyzer.
        if (lexState.onRhs)
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
            if (ustr.getChar() == '/')
            {
                lexState.fsmCurState = LexFsmStates::sCOMMENT;
                return;
            }
            ustr.bwd();
        }

        std::string s(1, c);
        tokenMapIter it = tokenLookup.find(s);
        if (it != tokenLookup.end())
        {   
            // only updating if on the right hand side
            if (lexState.onRhs)
            {
                if (it->first[0] == ')')
                    lexState.lastOperTermIsOper = false;
                // a hack to treat - as negation and not subtraction after ','
                // e.g. calc(14, -a);
                else if (it->first[0] == ',')
                    lexState.lastOperTermIsOper = true;
            }

            // can't happen when the above triggered
            // so the order doesn't matter
            if (it->second == TokenTypes::tEQUAL 
                // expr/term start withing ()
                // e.g. calc(-25)
                || it->second == TokenTypes::tLPR)
            {
                lexState.onRhs = true;
            }

            if (it->second == TokenTypes::tMINUS)
            {
                if (lexState.lastOperTermIsOper)
                    lexState.tokens.emplace_back(lexState.lexedLineIdx, TokenTypes::tNEG_MINUS);
                else
                {
                    lexState.tokens.emplace_back(lexState.lexedLineIdx, TokenTypes::tMINUS);
                    lexState.lastOperTermIsOper = true;
                }
            }
            else
            {
                lexState.tokens.emplace_back(lexState.lexedLineIdx, it->second);
                if (isbinaryperator(it->second))
                    lexState.lastOperTermIsOper = true;
            }   
        }
        else
        {
            lexState.tokens.emplace_back(lexState.lexedLineIdx, TokenTypes::tUNKNOWN_SYMBOL);
        }

        ustr.fwd();
        // epsilon transition
        lexState.fsmCurState = LexFsmStates::sSYMBOL;
    }

    void commentStateBeh(UsefulString &ustr, lexerState &lexState)
    {
        if (ustr.isEol())
        {
            lexState.tokens.emplace_back(lexState.lexedLineIdx, TokenTypes::tDIV);
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
            lexState.tokens.emplace_back(lexState.lexedLineIdx, TokenTypes::tDIV);

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

        lexState.lexedLineIdx++;

#ifdef LEXER_DEBUG
        std::cout << "Finished tokenizing line: " << line << '\n';
        std::cout << "Got this many tokens now: " << lexState.tokens.size() << '\n';
#endif
        return true;
    }
};

bool tokenize(const std::string &filePath, Lexer &lexer, lexerState &lexState)
{
    #ifdef LEXER_DEBUG
    auto printTokens = [](const tokensVect &tokens)
    {
        for (auto elem : tokens)
        {
            std::cout << "Token type: "  << tType_to_string(elem.tType) << '\n';
            if (elem.tVal.has_value())    
                std::cout << "Token val: " << elem.tVal.value() << '\n';
            else
                std::cout << "Token val: none\n";
        }
        std::cout << '\n';
    };
#endif
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

    std::ifstream jackFile;
    jackFile.open(filePath, std::ios::in);
    if (!jackFile)
        return false;

    std::cout << filePath << '\n';
    lexer.setCurFileName(filePath);

    while (lexer.getMoreLinesComing())
    {
        const bool res = lexer.lexNextLine(&jackFile, lexState);
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

    jackFile.close();
    return true;
}

bool compilerCtrl(const char *pathIn, const char *pathOut)
{
    Lexer lexer;
    lexerState lexState;
    auto [arrayLib_className_id, newName_id, thisName_id] = Lexer::loadSysLibIdents(lexState);

    Parser parser;
    parser.thisNameID = thisName_id;
    // TODO: the whole loading is temp solution
    parser.loadSysLibSymbols(arrayLib_className_id, newName_id);

    if (!lexer.init(pathIn))
    {
        // TODO: error
        return false;
    }

    unsigned int tokensOffset = 0;
    for (const auto &filePath : lexer.getFilePaths())
    {
        if (!tokenize(filePath, lexer, lexState))
        {
            continue;
        }

        lexer.resetForFile();

#ifndef LEXER_ONLY

        Generator generator(lexer.getCurFileName(), lexState.identifiers);
        auto *astRoot = parser.buildAST(lexState.tokens, lexState.identifiers, tokensOffset);
        tokensOffset = lexState.tokens.size();
    #ifdef DEBUG
        parser.printAST();
    #endif
        generator.generateAndWrite(astRoot);

        parser.resetState();
#endif

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