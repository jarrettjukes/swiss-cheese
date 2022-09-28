#include "main.h"
#include "swiss.h"

inline void AppendStringOutput(char *src, int srcLen, output *out)
{
    AppendString(src, srcLen, (out->data + out->dataLen), &out->dataLen);
}

#if 0
inline void Discard()
{
    int endOfFunction = IndexOf(charData, ';');
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
    Assert(*charData++ == ':');
    
    newF->codeBlockLength = IndexOf(charData, ';');
    
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

global char appenders[3] = {'&', '[', ':'};
global char combinators[8] = {
    ',',
    '>',
    '~',
    '+',
    ' ',
};

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
    result->name = PushArray(&state->workingMem, char, result->len + 1);
    
    return result;
}

internal selector_block *NewBlock(app_state *state, selector_block *blocks, u32 *blockCount, selector_block *parent)
{
    selector_block *block = (blocks + (*blockCount)++);
    state->totalBlockCount++;
    memset(block, 0, sizeof(selector_block));
    
    block->names = PushArray(&state->workingMem, member_name, 16);
    //block->lines = PushArray(&state->workingMem, key_value_pair, 64);
    //block->variables = PushArray(&state->workingMem, key_value_pair, 64);
    
    block->keys = PushArray(&state->workingMem, key_value_pair, 128);
    
    block->children = PushArray(&state->workingMem, selector_block, MAX_BLOCK_CHILD_COUNT);
    if(parent)
    {
        block->parent = parent;
    }
    
    return block;
}

internal void OutVariables(key_value_pair *variables, u32 variableCount, output *out)
{
    for(u32 variableIndex = 0; variableIndex < variableCount; ++variableIndex)
    {
        key_value_pair *variable = variables + variableIndex;
        //if(IsFlagSet(variable->flags, Variable_NoReplace)) continue;
        if(!IsFlagSet(variable->flags, KVP_Variable)) continue;
        if(IsFlagSet(variable->flags, KVP_VariableNoReplace)) continue;
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

internal void OutCode(app_state *state, selector_block *block, output *out)
{
    AppendStringOutput("{\n", 2, out);
    
    OutVariables(block->keys, block->keyCount, out);
    
    for(u32 opIndex = 0; opIndex < block->keyCount; ++opIndex)
    {
        key_value_pair *line = block->keys + opIndex;
        if(!IsFlagSet(line->flags, KVP_Line)) continue;
        if(IsFlagSet(out->flags, Output_Indent))
        {
            AppendStringOutput("\t", 1, out);
        }
        
        AppendStringOutput("\t", 1, out);
        
        AppendStringOutput(line->name, line->nameLength, out);
        
        AppendStringOutput(": ", 2, out);
        
        int varStartIndex = IndexOf(line->value, '$');
        if(varStartIndex >= 0)
        {
            b32 findVariable = true;
            selector_block *varBlock = block;
            key_value_pair *varKey = 0;
            do
            {
                for(u32 keyIndex = 0; keyIndex < varBlock->keyCount; ++keyIndex)
                {
                    key_value_pair *key = varBlock->keys + keyIndex;
                    if(!IsFlagSet(key->flags, KVP_Variable)) continue;
                    
                    b32 stringMatch = StringExactMatch(line->value, line->valueLength, key->name, key->nameLength);
                    if(stringMatch)
                    {
                        varKey = key;
                        findVariable = false;
                        break;
                    }
                }
                
                varBlock = varBlock->parent;
            } while(findVariable && varBlock);
            
            char *str = 0;
            if(varKey)
            {
                str = varKey->name;
            }
            else
            {
                str = line->value;
            }
            
            AppendString("var(--", 6, (out->data + out->dataLen), &out->dataLen);
            
            AppendString(str + 1, StringLength(str) - 1, (out->data + out->dataLen), &out->dataLen);
            
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

inline void GetVariableSources(b32 expr, app_state *state, selector_block *workingBlock, key_value_pair **variables, u32 **variableCount)
{
    if(!expr)
    {
        *variables = state->variables;
        *variableCount = &state->variableCount;
    }
    else
    {
        *variables = workingBlock->keys;
        *variableCount = &workingBlock->keyCount;
    }
}

internal void GetCode(app_state *state, char *c, key_value_pair *lines, u32 *lineCount, u8 flags)
{
    key_value_pair *code = (lines + (*lineCount)++);
    int sep = IndexOf(c, ':');
    code->nameLength = sep;
    code->name = PushArray(&state->workingMem, char, code->nameLength + 1);
    
    WriteString(c, code->nameLength, code->name);
    c += code->nameLength + 1;
    
    if(*c == ' ')
    {
        c++;
    }
    
    code->valueLength = IndexOf(c, ';');
    code->value = PushArray(&state->workingMem, char, code->valueLength + 1);
    for(int i = 0; i < code->valueLength; ++i)
    {
        if(*c == '"')
        {
            c++;
            code->valueLength--;
            i--;
            continue;
        }
        code->value[i] = *c++;
    }
    
    code->flags = flags;
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

internal void OutWrapper(member_name *name, key_value_pair *variables, int variableCount, output *out)
{
    int varChar = IndexOf(name->name, '$');
    
    int nameLen = varChar > 0 ? varChar : name->len;
    AppendStringOutput(name->name, nameLen, out);
    
    if(varChar > 0)
    {
        for(int varIndex = 0; varIndex < variableCount; ++varIndex)
        {
            key_value_pair *variable = variables + varIndex;
            if(!IsFlagSet(variable->flags, KVP_Variable)) continue;
            b32 stringMatch = StringExactMatch(name->name + varChar, name->len - varChar, variable->name, variable->nameLength);
            
            if(stringMatch)
            {
                AppendStringOutput(variable->value, variable->valueLength, out);
            }
        }
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
    member_name *name = block->names + 0;
    
    if(IsFlagSet(block->flags, Block_wrapper))
    {
        int varCharIndex = IndexOf(name->name, '$');
        selector_block *parent = block;
        key_value_pair *variables = 0;
        int variableCount = 0;
        //MULTIPLE VARIABLES????/?/?/
        while(parent && varCharIndex > 0)
        {
            for(u32 variableIndex = 0; variableIndex < parent->keyCount; ++variableIndex)
            {
                key_value_pair *variable = parent->keys + variableIndex;
                if(!IsFlagSet(variable->flags, KVP_Variable)) continue;
                b32 stringMatch = StringExactMatch(variable->name, variable->nameLength, name->name, name->len);
                if(stringMatch)
                {
                    variables = parent->keys;
                    variableCount = parent->keyCount;
                    //int newChar = IndexOf(name->name + varCharIndex + 1, '$');
                    varCharIndex = 0;
                    break;
                }
            }
            parent = parent->parent;
        }
        
        if(!variables && !variableCount)
        {
            variables = state->variables;
            variableCount = state->variableCount;
        }
        //AppendString("\n", 1, out->data + (*out->dataLen), out->dataLen);
        OutWrapper(name, variables, variableCount, out);
        AppendString("\n", 1, out->data + out->dataLen, &out->dataLen);
    }
    b32 hasLines = false;
    
    for(u32 keyIndex = 0; keyIndex < block->keyCount; ++keyIndex)
    {
        key_value_pair *key = block->keys + keyIndex;
        if(IsFlagSet(key->flags, KVP_Line))
        {
            hasLines = true;
            break;
        }
    }
    
    if(hasLines)
    {
        //AppendString("\n", 1, out->data + (*out->dataLen), out->dataLen);
        if(IsFlagSet(block->flags, Block_prepend))
        {
            AppendStringOutput(name->name, name->len, out);
            
            *(out->data + out->dataLen++) = ' ';
        }
        
        u32 parentCount = 0;
        {
            selector_block *parent = block->parent;
            while(parent)
            {
                if(IsFlagSet(parent->flags, Block_wrapper)) break;
                parentCount++;
                parent = parent->parent;
            }
        }
        
        if(IsFlagSet(out->flags, Output_Indent))
        {
            *(out->data + out->dataLen++) = '\t';
        }
        
        if(parentCount)
        {
            selector_block *parentList = PushArray(&state->workingMem, selector_block, parentCount);
            {
                selector_block *blockParent = block->parent;
                
                for(u32 parentIndex = parentCount; parentIndex > 0; --parentIndex)
                {
                    if(blockParent)
                    {
                        if(IsFlagSet(blockParent->flags, Block_wrapper)) break;
                        if(IsFlagSet(blockParent->flags, Block_prepend))
                        {
                            *(parentList + parentIndex-- - 1) = *blockParent->parent;
                            *(parentList + parentIndex - 1) = *blockParent;
                            blockParent = blockParent->parent;
                        }
                        else
                        {
                            *(parentList + parentIndex - 1) = *blockParent;
                        }
                        blockParent = blockParent->parent;
                    }
                }
            }
            
            for(u32 parentIndex = 0; parentIndex < parentCount; ++parentIndex)
            {
                selector_block *parent = parentList + parentIndex;
                selector_block *nextParent = parentList + parentIndex + 1;
                
                for(int parentNameIndex = 0; parentNameIndex < parent->nameCount; ++parentNameIndex)
                {
                    member_name *parentName = parent->names + parentNameIndex;
                    
                    AppendStringOutput(parentName->name, parentName->len, out);
                    
                    if(!IsWhiteSpace(parentName->combinationChar) && parentName->combinationChar)
                    {
                        if(!IsFlagSet(block->flags, Block_append)) *(out->data + out->dataLen++) = ' ';
                        OutNames(block, out);
                        *(out->data + out->dataLen++) = parentName->combinationChar;
                        *(out->data + out->dataLen++) = ' ';
                    }
                    
                    if(nextParent->nameCount && !IsFlagSet(nextParent->flags, Block_append)) *(out->data + out->dataLen++) = ' ';
                }
            }
            
            PopArray(&state->workingMem, parentList, selector_block, parentCount);
            
            if(!IsFlagSet(block->flags, Block_append))
            {
                *(out->data + out->dataLen++) = ' ';
            }
        }
        
        if(!IsFlagSet(block->flags, Block_prepend))
        {
            OutNames(block, out);
        }
        
        *(out->data + out->dataLen++) = ' ';
        
        OutCode(state, block, out);
    }
    
    for(u32 childIndex = 0; childIndex < block->childCount; ++childIndex)
    {
        OutBlock(state, block->children + childIndex, out);
    }
    
    if(IsFlagSet(block->flags, Block_wrapper))
    {
        //warning(jarrett): I have observed stack corruption issues with this code (state get mangled), but deleting binaries seemed to have fixed it?
        //might be a problem later
        if(IsFlagSet(out->flags, Output_NewLine))
        {
            out->dataLen--;
            out->flags &= ~Output_NewLine;
        }
        AppendStringOutput("}\n", 2, out);
        out->flags &= ~Output_Indent;
    }
}

internal void ParseData(app_state *state, file_contents file, error_details *error)
{
    selector_block *workingBlock = 0;
    b32 toNext = false;
    int column = 0;
    char selectors[10] = {
        '>',
        '#',
        '.',
        '&',
        '@',
        '~',
        '+',
        '[',
        ':',
        '*'
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
            while(*charData++ != '\n')
            {
                if(*charData == '\n') 
                {
                    state->linesOfCode++;
                }
            }
            column = 0;
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
                state->commentDataLen = endIndex;
                AppendString(charData, state->commentDataLen, state->commentData, 0);
                charData += endIndex;
            }
        }
        else if(*charData == '$' || *charData == '-')
        {
            //char next = *(charData + 1);
            //if(*charData == '-' && next != '-') continue;
            //if(*charData == '$') charData++;
            //if(*charData == '-' && next == '-') charData += 2;
            //hmmm
            key_value_pair *variables = 0;
            u32 *variableCount = 0;
            GetVariableSources((workingBlock ? 1 : 0), state, workingBlock, &variables, &variableCount);
            
            GetCode(state, charData, variables, variableCount, KVP_Variable);
            
            toNext = true;
        }
        else if(*charData == '}')
        {
            SetFlag(workingBlock, Block_completed);
#if 0
            if(!workingBlock->parent)
            {
                Error(error, "Extra closing bracket.", 0);
                break;
            }
#endif
            workingBlock = workingBlock->parent;
            toNext = true;
        }
        else if (isSelector)
        {
            int openBracketIndex = IndexOf(charData, '{');
            int valueSeparator = IndexOf(charData, ':');
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
                        int eol = IndexOf(charData, ';');
                        
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
                            charData += IndexOf(charData, '\"') + 1;
                            
                            state->encodingLen = IndexOf(charData, '\"');
                            state->encoding = PushArray(&state->workingMem, char, state->encodingLen + 1);
                            WriteString(charData, state->encodingLen, state->encoding);
                            
                            charData += eol + 1;
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
                if(!workingBlock)
                {
                    workingBlock = NewBlock(state, state->blocks, &state->blockCount, 0);
                }
                else
                {
                    workingBlock = NewBlock(state, workingBlock->children, &workingBlock->childCount, workingBlock);
                }
                
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
                        int prependCharIndex = IndexOf(charData, '&');
                        
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
                    int varIndex = IndexOf(newName->name, '$');
                    if(varIndex >= 0)
                    {
                        key_value_pair *variables = 0;
                        u32 *variableCount = 0;
                        
                        GetVariableSources((workingBlock->parent ? 1 : 0), state, workingBlock, &variables, &variableCount);
                        
                        for(u32 variableIndex = 0; variableIndex < *variableCount; ++variableIndex)
                        {
                            key_value_pair *variable = variables + variableIndex;
                            b32 stringMatch = StringExactMatch(newName->name + varIndex, newName->len - varIndex, variable->name, variable->nameLength);
                            if(stringMatch)
                            {
                                variable->flags |= Variable_NoReplace;
                                //variable->flags |= name_replace;
                            }
                        }
                    }
                }
            }
            else
            {
                if(!IsFlagSet(workingBlock->flags, Block_completed))
                {
                    GetCode(state, charData, workingBlock->keys, &workingBlock->keyCount, KVP_Line);
                }
                toNext = true;
            }
        }
    }
}

void InitMem(mem_struct *mem, memory_pool *pool, mem_index size)
{
    mem->size = pool->memorySize - size;
    mem->used += size;
    mem->base = pool->memory + size;
}

void ProcessData(app_platform *platform, file_contents file, error_details *error)
{
    app_state *state = (app_state *)platform->permanentMemoryPool.memory;
    
    if(!state->isInitialized)
    {
        InitMem(&state->workingMem, &platform->permanentMemoryPool, sizeof(app_state));
        
        //state->blocks = PushArray(&state->workingMem, selector_block, 256);
        
        state->variables = PushArray(&state->workingMem, key_value_pair, 256);
        
        state->commentData = PushArray(&state->workingMem, char, 256);
        state->commentDataLen = 256;
        
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
        char *outFileName = PushArray(&state->workingMem, char, (u32)fileExtIndex + extensionLen + 1);
        WriteString(file.fileName, fileExtIndex, outFileName);
        
        AppendString(outFileExt, extensionLen, outFileName + fileExtIndex, 0);
        
        //out?
        output out;
        out.data = PushArray(&state->workingMem, char, (u32)(1.5f * file.size));
        out.dataLen = 0;
        out.flags |= Output_NewLine;
        
        if(state->encoding)
        {
            AppendStringOutput("@charset \"", 10, &out);
            
            AppendStringOutput(state->encoding, state->encodingLen, &out);
            
            AppendStringOutput("\";\n\n", 4, &out);
        }
        
        if(state->variableCount)
        {
            AppendStringOutput(":root {\n", 8, &out);
            
            OutVariables(state->variables, state->variableCount, &out);
            
            AppendStringOutput("}\n\n", 3, &out);
        }
        
        for(u32 masterBlockIndex = 0; masterBlockIndex < state->blockCount; ++masterBlockIndex)
        {
            selector_block *block = state->blocks + masterBlockIndex;
            
            OutBlock(state, block, &out);
            //AppendString("\n", 1, outData + (outDataLen), &outDataLen);
        }
        
        msElapsed = ToMS(platform->outTimerAndDiscard(platform));
        
        if(out.dataLen)
        {
            platform->writeFile(outFileName, out.data, StringLength(out.data) - 1);
        }
    }
}