#include "platform.h"
#include "swiss.h"
inline void SetFlag(selector_block *block, u8 flag)
{
    block->flags |= flag;
}


inline b32 IsWhiteSpace(char str)
{
    b32 result = (str == ' ' ||
                  str == '\t' ||
                  str == '\n');
    return result;
}

inline member_name *NewName(app_state *state, selector_block *block, int len)
{
    member_name *result = block->names + block->nameCount++;
    result->len = len;
    result->name = PushArray(&state->arena, char, result->len + 1);
    
    return result;
}

internal selector_block *NewBlock(app_state *state, int parentIndex)
{
    int blockIndex = state->blockCount++;
    selector_block *block = state->blocks + blockIndex;
    state->totalBlockCount++;
    memset(block, 0, sizeof(selector_block));
    
    block->blockIndex = blockIndex;
    block->parentBlockIndex = parentIndex;
    block->names = PushArray(&state->arena, member_name, 16);
    
    //block->keys = PushArray(&state->arena, key_value_pair, 128);
#if 0
    block->children = PushArray(&state->arena, selector_block, MAX_BLOCK_CHILD_COUNT);
    if(parent)
    {
        block->parent = parent;
    }
#endif
    return block;
}

internal key_value_pair GetCode(app_state *state, char *c, u8 flags)
{
    key_value_pair result = {0};
    
    result.nameLength = IndexOf(c, CHARACTER_START_VALUE_CAPTURE);
    result.name = PushArray(&state->arena, char, result.nameLength + 1);
    result.flags = flags;
    
    WriteString(c, result.nameLength, result.name);
    c += result.nameLength + 1;
    
    if(*c == ' ')
    {
        c++;
    }
    
    result.valueLength = IndexOf(c, CHARACTER_END_VALUE_CAPTURE);
    int quoteIndex = IndexOf(c, '"');
    if(quoteIndex >= 0 && quoteIndex < result.valueLength)
    {
        result.flags |= KVP_VariableReplace;
        c++;
        result.valueLength--;
    }
    quoteIndex = IndexOf(c, '"');
    if(quoteIndex >= 0 && quoteIndex < result.valueLength)
    {
        result.valueLength--;
    }
    
    result.value = PushArray(&state->arena, char, result.valueLength + 1);
    WriteString(c, result.valueLength, result.value);
    
    return result;
}



internal void ParseData(app_state *state, file_contents file, error_details *error)
{
    state->blockCount++;
    selector_block *workingBlock = NewBlock(state, 0);
    u32 blockBlockIndex = 1;
    b32 toNext = false;
    int column = 0;
    char selectors[10] = {
        '>',
        '#',
        '.',
        CHARACTER_APPEND_SELECTOR,
        '@',
        '~',
        '+',
        '[',
        CHARACTER_START_VALUE_CAPTURE,
        '*'
    };
    
    char appenders[3] = {CHARACTER_APPEND_SELECTOR, '[', CHARACTER_START_VALUE_CAPTURE};
    char combinators[8] = {
        ',',
        '>',
        '~',
        '+',
        ' ',
    };
    
    char *specialSelectors[5] = {
        "@media",
        "@charset",
        //todo(jarrett): implement these
        "@import",
        "@keyframes",
        "@font-face",
    };
    
    for(char *charData = (char *)file.contents; *charData && !error; ++charData)
    {
        column++;
        if(toNext)
        {
            while(IsWhiteSpace(*charData)) 
            {
                if(*charData == '\n')
                {
                    state->linesOfCode++;
                    column = 0;
                }
                charData++;
            }
            
            toNext = false;
        }
        
        b32 isSelector = (Contains(selectors, *charData) || (*charData >= 'a' && *charData <= 'z'));
        
        if(IsWhiteSpace(*charData))
        {
            continue;
        }
        else if(*charData == '/')
        {
            char next = *(charData + 1);
            b32 isComment = (next == '/' || next == '*');
            if(isComment)
            {
                charData += 2;
                int endIndex = 0;
                if(next == '*')
                {
                    endIndex = IndexOf(charData++, '/');
                }
                else if(next == '/')
                {
                    endIndex = IndexOf(charData++, '\n');
                }
                if(endIndex) --endIndex;
                //int hashSlot = (state->blockCount * workingBlock->blockIndex) & (ArrayCount(state->variables) - 1);
                //key_value_pair *key = (state->variables + hashSlot);
                
                //key->valueLength = endIndex;
                //key->flags |= KVP_Comment;
                //AppendString(charData, key->valueLength, key->value, 0);
                charData += endIndex;
            }
        }
        else if(*charData == CHARACTER_END_BLOCK)
        {
            SetFlag(workingBlock, Block_completed);
            
#if 0
            if(!workingBlock->parent)
            {
                Error(error, "Extra closing bracket.", 0);
                break;
            }
#endif
            if(workingBlock->parentBlockIndex == 1 && blockBlockIndex > 1) blockBlockIndex = workingBlock->parentBlockIndex;
            workingBlock = (state->blocks + workingBlock->parentBlockIndex);
            //workingBlock = workingBlock->parent;
            
            toNext = true;
        }
        else if (isSelector || *charData == CHARACTER_VARIABLE)
        {
            int openBracketIndex = IndexOf(charData, CHARACTER_START_BLOCK);
            int valueSeparator = IndexOf(charData, CHARACTER_START_VALUE_CAPTURE);
            if((openBracketIndex <= valueSeparator && openBracketIndex >= 0) || Contains(appenders, *charData) || *charData == '@')
            {
                b32 hasMediaQuery = false;
                b32 leaveThisPlace = false;
                for(int specialIndex = 0; specialIndex < ArrayCount(specialSelectors); ++specialIndex)
                {
                    char *specialSelector = specialSelectors[specialIndex];
                    int len = StringLength(specialSelector);
                    
                    if(StringExactMatch(charData, len, specialSelector, len))
                    {
                        if(StringExactMatch("@media", len, specialSelector, len))
                        {
                            hasMediaQuery = true;
                            break;
                        }
                        else if(StringExactMatch("@charset", len, specialSelector, len))
                        {
                            if(state->encoding)
                            {
                                //int ababab = 0;
                                //todo(jarrett): throw an error
                                __debugbreak();
                            }
                            charData += IndexOf(charData, '"') + 1;
                            
                            state->encodingLen = IndexOf(charData, '"');
                            state->encoding = PushArray(&state->arena, char, state->encodingLen + 1);
                            WriteString(charData, state->encodingLen, state->encoding);
                            
                            charData += IndexOf(charData, CHARACTER_END_VALUE_CAPTURE);
                            leaveThisPlace = true;
                            break;
                        }
                        else if(StringExactMatch("@import", len, specialSelector, len))
                        {
                            //todo(jarrett): import can use media query text
                            charData += IndexOf(charData, ' ') + 1;
                        }
                    }
                }
                if(leaveThisPlace) continue;
                workingBlock = NewBlock(state, !IsFlagSet(workingBlock->flags, Block_completed) ? workingBlock->blockIndex : 0);
                if(workingBlock->parentBlockIndex == 1) 
                {
                    blockBlockIndex = workingBlock->blockIndex;
                }
                workingBlock->blockBlockIndex = ((u32)workingBlock->blockIndex == blockBlockIndex) ? 1 : blockBlockIndex;
                
                
                if(hasMediaQuery)
                {
                    SetFlag(workingBlock, Block_wrapper);
                }
                
                if(StringContains(appenders, StringLength(appenders), charData, openBracketIndex)) //entire string
                {
                    if(Contains(appenders, *charData)) //first most character
                    {
                        charData++;
                        openBracketIndex--;
                        SetFlag(workingBlock, Block_append);
                    }
                    else
                    {
                        int prependCharIndex = IndexOf(charData, CHARACTER_APPEND_SELECTOR);
                        
                        if(openBracketIndex > prependCharIndex)
                        {
                            SetFlag(workingBlock, Block_prepend);
                            
                            openBracketIndex -= (openBracketIndex - prependCharIndex);
                        }
                    }
                }
                
                int combinatorCount = ArrayCount(combinators);
                b32 hasCombination = StringContains(combinators, combinatorCount, charData, openBracketIndex);
                member_name *newName = NewName(state, workingBlock, openBracketIndex);
                if(hasCombination && !hasMediaQuery && !Contains(combinators, *charData))
                {
                    int eOcc = EarliestOccurrenceOf(combinators, combinatorCount, charData, openBracketIndex);
                    newName->len = eOcc;
                    
                    while(eOcc > 0)
                    {
                        WriteString(charData, newName->len, newName->name);
                        charData += eOcc;
                        openBracketIndex -= eOcc;
                        
                        if(!IsWhiteSpace(*charData))
                        {
                            newName->combinationChar = *charData++;
                            openBracketIndex--;
                        }
                        
                        while(IsWhiteSpace(*charData))
                        {
                            charData++;
                            openBracketIndex--;
                        }
                        eOcc = EarliestOccurrenceOf(combinators, combinatorCount, charData, openBracketIndex);
                        
                        if(eOcc > 0) newName = NewName(state, workingBlock, eOcc);
                    }
                }
                else
                {
                    newName->len--;
                    WriteString(charData, newName->len, newName->name);
                    charData += openBracketIndex;
                }
            }
            else
            {
                key_value_pair code = GetCode(state, charData, KVP_Line);
                //collapse all of the blocks into a hash function? block index * id (global = 0, block chunks > 0)
                
                //16 is the amount of variables we want to store for each block atm
                int hashSlot = ((16 + workingBlock->keyCount) * workingBlock->blockIndex) & (ArrayCount(state->variables) - 1);
                //code.blockIndex = workingBlock->blockIndex;
                *(state->variables + hashSlot) = code;
                state->variableCount++;
                workingBlock->keys[workingBlock->keyCount++] = hashSlot;
                //workingBlock->keyCount++;
                
                charData += code.nameLength + code.valueLength + 3; //' ' + ';' + ':' == 3 chars
                
                toNext = true;
            }
        }
    }
    
    for(u32 blockIndex = 0; blockIndex < state->blockCount; ++blockIndex)
    {
        selector_block *block = state->blocks + blockIndex;
        if(IsFlagSet(block->flags, Block_prepend) && block->parentBlockIndex != 1)
        {
            selector_block *prevBlock = (state->blocks + block->blockIndex - 1);
            
            selector_block temp = *prevBlock;
            
            *prevBlock = *block;
            prevBlock->blockIndex--;
            prevBlock->parentBlockIndex--;
            
            *(state->blocks + block->blockIndex) = temp;
            block->blockIndex++;
            block->parentBlockIndex++;
        }
    }
}
