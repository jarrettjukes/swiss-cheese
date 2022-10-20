#include "main.h"
#include "swiss.h"

inline void AppendStringOutput(char *src, int srcLen, output *out)
{
    AppendString(src, srcLen, (out->data + out->dataLen), &out->dataLen);
}

#if 0
inline void Discard()
{
    int endOfFunction = IndexOf(charData, CHARACTER_END_VALUE_CAPTURE);
#if 1
    
    charData++;
    
    function *newF = state->userDefinedFunctions + state->functionCount++;
    newF->nameLength = IndexOf(charData, '(');
    for(int nameIndex = 0; nameIndex < newF->nameLength; ++nameIndex)
    {
        *(newF->name + nameIndex) = *charData++;
    }
    
    if(*charData == '(') 
    {
        charData++;
    }
    else
    {
        Error(error, "");
        break;
    }
    
    int paramCount = 0;
    char paramNames[8][256] = {0, 0};
    while(*charData != ')')
    {
        int charIndex = 0;
        while(*charData != ')')
        {
            paramNames[paramCount][charIndex++] = *charData++;
            
            if(*charData == ',')
            {
                paramCount++;
            }
        }
    }
    
    charData++;
    
    //code block
    Assert(*charData++ == CHARACTER_START_VALUE_CAPTURE);
    
    newF->codeBlockLength = IndexOf(charData, CHARACTER_END_VALUE_CAPTURE);
    
    for(int codeIndex = 0; codeIndex < newF->codeBlockLength; ++codeIndex)
    {
        *(newF->codeBlock + codeIndex) = *charData++;
    }
    charData++;
    
    //execute function code block
    //todo(jarrett): move this somewhere else
    char operand = 0;
    int operandIndex = 0;
    int operand1Length = 0, operand2Length = 0;
    char operand1[256] = {0};
    char operand2[256] = {0};
    for(int c = 0; c < newF->codeBlockLength; ++c)
    {
        char currentChar = *(newF->codeBlock + c);
        if(currentChar - '0' >= 0)
        {
            if(operandIndex == 0)
            {
                operand1[operand1Length++] = currentChar;
            }
            else if(operandIndex == 1)
            {
                operand2[operand2Length++] = currentChar;
            }
        }
        switch(currentChar)
        {
            case '+':
            case '-':
            case '*':
            case '/':
            {
                operand = currentChar;
                operandIndex++;
            } break;
            case ' ':
            {
                continue;
            } break;
        }
    }
#else
    charData += endOfFunction;
#endif
}
#endif

inline b32 IsWhiteSpace(char str)
{
    b32 result = (str == ' ' ||
                  str == '\t' ||
                  str == '\n');
    return result;
}

inline b32 IsFlagSet(u8 flags, u8 flag)
{
    b32 result = flags & flag;
    return result;
}

inline void SetFlag(selector_block *block, u8 flag)
{
    block->flags |= flag;
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

internal void OutVariableDeclaration(app_state *state, selector_block *block, output *out)
{
    for(u32 variableIndex = 0; variableIndex < block->keyCount; ++variableIndex)
    {
        //int hashSlot = (16 + variableIndex) * blockIndex;// & (ArrayCount() - 1);
        key_value_pair *variable = state->variables + block->keys[variableIndex];
        
        //if(variable->blockIndex != blockIndex) continue;
        if(*variable->name != CHARACTER_VARIABLE) continue;
        if(IsFlagSet(variable->flags, KVP_VariableReplace)) continue;
        if(IsFlagSet(out->flags, Output_Indent))
        {
            AppendStringOutput("\t", 1, out);
        }
        
        AppendStringOutput("\t--", 3, out);
        
        AppendStringOutput(variable->name + 1, variable->nameLength - 1, out);
        
        AppendStringOutput(": ", 2, out);
        
        AppendStringOutput(variable->value, variable->valueLength, out);
        
        AppendStringOutput(";\n", 2, out);
    }
}


internal char *FindKeyStr(key_value_pair *keys, u32 keyCount, char *targetStr, int sourceBlockIndex)
{
    for(u32 varIndex = 0; varIndex < keyCount; ++varIndex)
    {
        key_value_pair *variable = keys + varIndex + (sourceBlockIndex * 16);
        
        if(!variable) continue;
        //if(variable->blockIndex > sourceBlockIndex) break;
        if(*variable->name != CHARACTER_VARIABLE) continue;
        
        b32 stringMatch = StringExactMatch(targetStr, variable->nameLength, variable->name, variable->nameLength);
        
        if(stringMatch)
        {
            return variable->value;
        }
    }
    
    return 0;
}

internal void OutCode(app_state *state, selector_block *block, output *out)
{
    AppendStringOutput("{\n", 2, out);
    
    OutVariableDeclaration(state, block, out);
    
    for(u32 opIndex = 0; opIndex < block->keyCount; ++opIndex)
    {
        //int hashSlot = (16 + opIndex) * block->blockIndex;
        key_value_pair *line = state->variables + block->keys[opIndex];
        
        //if(line->blockIndex != block->blockIndex) continue;
        if(*line->name == CHARACTER_VARIABLE) continue;
        if(IsFlagSet(out->flags, Output_Indent))
        {
            AppendStringOutput("\t", 1, out);
        }
        
        AppendStringOutput("\t", 1, out);
        
        AppendStringOutput(line->name, line->nameLength, out);
        
        AppendStringOutput(": ", 2, out);
        
        int varStartIndex = IndexOf(line->value, CHARACTER_VARIABLE);
        if(varStartIndex >= 0)
        {
            char *keyVal = (state->variables + block->keys[opIndex])->value;
            
            AppendString("var(--", 6, (out->data + out->dataLen), &out->dataLen);
            
            AppendString(keyVal + 1, StringLength(keyVal) - 1, (out->data + out->dataLen), &out->dataLen);
            
            AppendString(")", 1, (out->data + out->dataLen), &out->dataLen);
        }
        else
        {
            AppendStringOutput(line->value, line->valueLength, out);
        }
        
        AppendStringOutput(";\n", 2, out);
    }
    
    //for(int pad = 0; pad < paddingLen; ++pad)
    if(IsFlagSet(out->flags,  Output_Indent))
    {
        AppendStringOutput("\t", 1, out);
    }
    
    AppendStringOutput("}\n", 2, out);
    if(IsFlagSet(out->flags, Output_NewLine)) 
    {
        AppendStringOutput("\n", 1, out);
    }
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

#if 0
internal void Error(error_details *error, char *desc, u32 code = 0)
{
    int descLen = StringLength(desc);
    AppendString(desc, descLen, error->description);
    //error->description = desc;
    error->code = code;
}
#endif

internal void OutWrapper(app_state *state, member_name *name, selector_block *block, output *out)
{
    int varChar = IndexOf(name->name, CHARACTER_VARIABLE);
    
    int nameLen = varChar > 0 ? varChar : name->len;
    AppendStringOutput(name->name, nameLen, out);
    
    char *workingName = name->name + nameLen;
    while(varChar >= 0)
    {
        char *variableValue = FindKeyStr(state->variables, state->variableCount, workingName, block->parentBlockIndex);
        
        if(!variableValue)
        {
            variableValue = FindKeyStr(state->variables, state->variableCount, workingName, block->blockIndex);
        }
        
        if(variableValue)
        {
            AppendStringOutput(variableValue, StringLength(variableValue), out);
        }
        
        varChar = IndexOf(workingName + 1, CHARACTER_VARIABLE);
        workingName += varChar;
    }
    AppendStringOutput(" {", 2, out);
    
    out->flags |= Output_Indent;
}

internal void OutNames(selector_block *block, output *out)
{
    for(int nameIndex = 0; nameIndex < block->nameCount; ++nameIndex)
    {
        member_name *name = block->names + nameIndex;
        AppendStringOutput(name->name, name->len, out);
        
        if(!IsWhiteSpace(name->combinationChar) && name->combinationChar)
        {
            *(out->data + out->dataLen++) = name->combinationChar;
            *(out->data + out->dataLen++) = ' ';
        }
    }
}

internal void OutBlock(app_state *state, selector_block *block, output *out)
{
    Assert(IsFlagSet(block->flags, Block_completed));
    member_name *name = block->names + 0;
    
    if(IsFlagSet(block->flags, Block_wrapper))
    {
        //todo(jarrett): media queries are broken (99% sure)
        OutWrapper(state, name, block, out);
        AppendStringOutput("\n", 1, out);
        return;
    }
    b32 hasLines = block->keyCount > 0;
    selector_block *blockParent = state->blocks + block->parentBlockIndex;
    if((hasLines && !IsFlagSet(block->flags, Block_prepend)) || (!hasLines && blockParent->keyCount > 0 && IsFlagSet(blockParent->flags, Block_prepend)))
    {
        if(IsFlagSet(block->flags, Block_prepend))
        {
            AppendStringOutput(name->name, name->len, out);
            
            *(out->data + out->dataLen++) = ' ';
        }
        
        if(IsFlagSet(out->flags, Output_Indent))
        {
            *(out->data + out->dataLen++) = '\t';
        }
        
        //b32 outName = false;
        if(block->parentBlockIndex > 1)
        {
            for(u32 blockIndex = block->blockBlockIndex; blockIndex < state->blockCount; ++blockIndex)
            {
                if(blockIndex > (u32)block->blockIndex) continue;
                selector_block *parent = state->blocks + blockIndex;
                
                if(parent->blockIndex == block->blockIndex) break;
                if(parent->parentBlockIndex == block->parentBlockIndex) continue;
                if(IsFlagSet(parent->flags, Block_wrapper) && parent->parentBlockIndex == 1) continue;
                
                for(int parentNameIndex = 0; parentNameIndex < parent->nameCount; ++parentNameIndex)
                {
                    member_name *parentName = parent->names + parentNameIndex;
                    
                    AppendStringOutput(parentName->name, parentName->len, out);
                    //outName = true;
                    if(!IsWhiteSpace(parentName->combinationChar) && parentName->combinationChar)
                    {
                        if(!IsFlagSet(block->flags, Block_append)) 
                        {
                            *(out->data + out->dataLen++) = ' ';
                        }
                        
                        OutNames(block, out);
                        
                        *(out->data + out->dataLen++) = parentName->combinationChar;
                        *(out->data + out->dataLen++) = ' ';
                    }
                    
                    if(parent->nameCount && !IsFlagSet(parent->flags, Block_append) && parent->blockIndex != block->parentBlockIndex && !IsFlagSet(blockParent->flags, Block_append)) 
                    {
                        *(out->data + out->dataLen++) = ' ';
                    }
                }
            }
        }
        
        if(!IsFlagSet(block->flags, Block_append) && block->parentBlockIndex != 1 && !IsFlagSet(blockParent->flags, Block_wrapper))
        {
            *(out->data + out->dataLen++) = ' ';
        }
        
        if(!IsFlagSet(block->flags, Block_prepend))// && !outName)
        {
            OutNames(block, out);
        }
        
        *(out->data + out->dataLen++) = ' ';
        
        if(hasLines)
        {
            OutCode(state, block, out);
        }
        else if(!hasLines && blockParent->keyCount > 0 && block->parentBlockIndex > 1 && IsFlagSet(blockParent->flags, Block_prepend))
        {
            OutCode(state, blockParent, out);
        }
    }
    
    selector_block *nextBlock = block + 1;
    if(nextBlock->blockBlockIndex != block->blockBlockIndex && block->blockIndex != nextBlock->blockBlockIndex && IsFlagSet(out->flags, Output_Indent))
    {
        //warning(jarrett): I have observed stack corruption issues with this code (state get mangled), but deleting binaries seemed to have fixed it?
        //might be a problem later
        if(IsFlagSet(out->flags, Output_NewLine))
        {
            out->dataLen--;
            //out->flags &= ~Output_NewLine;
            out->flags ^= Output_NewLine;
        }
        AppendStringOutput("}\n", 2, out);
        out->flags &= ~Output_Indent;
    }
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

void InitMem(memory_arena *arena, memory_pool *pool, mem_index size)
{
    arena->size = pool->memorySize - size;
    arena->used += size;
    arena->base = pool->memory + size;
}

void ProcessData(app_platform *platform, file_contents file, error_details *error)
{
    app_state *state = (app_state *)platform->permanentMemoryPool.memory;
    
    if(!state->isInitialized)
    {
        InitMem(&state->arena, &platform->permanentMemoryPool, sizeof(app_state));
        
        //state->variables = PushArray(&state->arena, key_value_pair, 256);
        //state->blockCount++;
        state->isInitialized = true;
    }
    
    platform->beginTimer(platform);
    ParseData(state, file, error);
    float msElapsed = ToMS(platform->outTimerAndDiscard(platform));
    
    if(!error)
    {
        //output
        int fileExtIndex = IndexOf(file.fileName, '.');
        if(fileExtIndex < 0)
        {
            fileExtIndex = StringLength(file.fileName);
        }
        char *outFileExt = ".css";
        u32 extensionLen = StringLength(outFileExt);
        char *outFileName = PushArray(&state->arena, char, (u32)fileExtIndex + extensionLen + 1);
        WriteString(file.fileName, fileExtIndex, outFileName);
        
        AppendString(outFileExt, extensionLen, outFileName + fileExtIndex, 0);
        
        //out?
        output out = {0};
        out.data = PushArray(&state->arena, char, (u32)(1.5f * file.size));
        out.flags |= Output_NewLine;
        
        if(state->encoding)
        {
            AppendStringOutput("@charset \"", 10, &out);
            
            AppendStringOutput(state->encoding, state->encodingLen, &out);
            
            AppendStringOutput("\";\n\n", 4, &out);
        }
        
        {
            selector_block *root = state->blocks + 1;
            if(root->keyCount)
            {
                AppendStringOutput(":root {\n", 8, &out);
                
                OutVariableDeclaration(state, root, &out);
                
                AppendStringOutput("}\n\n", 3, &out);
            }
        }
        for(u32 masterBlockIndex = 2; masterBlockIndex < state->blockCount; ++masterBlockIndex)
        {
            selector_block *block = state->blocks + masterBlockIndex;
            
            OutBlock(state, block, &out);
        }
        
        msElapsed = ToMS(platform->outTimerAndDiscard(platform));
        
        if(out.dataLen)
        {
            platform->writeFile(outFileName, out.data, StringLength(out.data) - 1);
        }
    }
}