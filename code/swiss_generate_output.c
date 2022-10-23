
inline void AppendStringOutput(char *src, output *out)
{
    AppendString(src, out->data, &out->dataLen);
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
            AppendStringOutput("\t", out);
        }
        
        AppendStringOutput("\t--", out);
        
        AppendStringOutput(variable->name + 1, out);
        
        AppendStringOutput(": ", out);
        
        AppendStringOutput(variable->value, out);
        
        AppendStringOutput(";\n", out);
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
    AppendStringOutput("{\n", out);
    
    OutVariableDeclaration(state, block, out);
    
    for(u32 opIndex = 0; opIndex < block->keyCount; ++opIndex)
    {
        //int hashSlot = (16 + opIndex) * block->blockIndex;
        key_value_pair *line = state->variables + block->keys[opIndex];
        
        //if(line->blockIndex != block->blockIndex) continue;
        if(*line->name == CHARACTER_VARIABLE) continue;
        if(IsFlagSet(out->flags, Output_Indent))
        {
            AppendStringOutput("\t", out);
        }
        
        AppendStringOutput("\t", out);
        
        AppendStringOutput(line->name, out);
        
        AppendStringOutput(": ", out);
        
        int varStartIndex = IndexOf(line->value, CHARACTER_VARIABLE);
        if(varStartIndex >= 0)
        {
            char *keyVal = (state->variables + block->keys[opIndex])->value;
            
            AppendString("var(--", out->data, &out->dataLen);
            
            AppendString(keyVal + 1, out->data, &out->dataLen);
            
            AppendString(")", out->data, &out->dataLen);
        }
        else
        {
            AppendStringOutput(line->value, out);
        }
        
        AppendStringOutput(";\n", out);
    }
    
    //for(int pad = 0; pad < paddingLen; ++pad)
    if(IsFlagSet(out->flags,  Output_Indent))
    {
        AppendStringOutput("\t", out);
    }
    
    AppendStringOutput("}\n", out);
    if(IsFlagSet(out->flags, Output_NewLine)) 
    {
        AppendStringOutput("\n", out);
    }
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
    AppendStringOutput(name->name, out);
    
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
            AppendStringOutput(variableValue, out);
        }
        
        varChar = IndexOf(workingName + 1, CHARACTER_VARIABLE);
        workingName += varChar;
    }
    AppendStringOutput(" {", out);
    
    out->flags |= Output_Indent;
}

internal void OutNames(selector_block *block, output *out)
{
    for(int nameIndex = 0; nameIndex < block->nameCount; ++nameIndex)
    {
        member_name *name = block->names + nameIndex;
        AppendStringOutput(name->name, out);
        
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
        AppendStringOutput("\n", out);
        return;
    }
    b32 hasLines = block->keyCount > 0;
    selector_block *blockParent = state->blocks + block->parentBlockIndex;
    if((hasLines && !IsFlagSet(block->flags, Block_prepend)) || (!hasLines && blockParent->keyCount > 0 && IsFlagSet(blockParent->flags, Block_prepend)))
    {
        if(IsFlagSet(block->flags, Block_prepend))
        {
            AppendStringOutput(name->name, out);
            
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
                    
                    AppendStringOutput(parentName->name, out);
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
        AppendStringOutput("}\n", out);
        out->flags &= ~Output_Indent;
    }
}
