#include "LexerTypes.h"

void LexerState::flush()
{
    buffer[0] = '\0';
    bIdx = 0;
}
bool LexerState::addBuff(char c)
{
    if (bIdx >= 20)
        return false;
    buffer[bIdx++] = c;
    return true;
}

bool LexerState::buffEmpty()
{
    return buffer[0] == '\0' || bIdx == 0;
}

void LexerState::reset()
{
    flush();
    fsmFinished = false;
    if (commentOpen)
    {
        if (mlineComment)
        {
            fsmCurState = LexFsmStates::sMLINE_COMMENT;
            return;
        }
        else
            commentOpen = false;
    }
    fsmCurState = LexFsmStates::sINIT;
    lastOperTermIsOper = true;
    onRhs = false;
}