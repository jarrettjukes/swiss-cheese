#include "main.h"
#include "swiss.h"

//note(jarrett): these two includes are for organizational purposes only
#include "swiss_parser.c"
#include "swiss_generate_output.c"

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

internal char *GenerateOutputFile(memory_arena *arena, char *filename)
{
    int fileExtIndex = IndexOf(filename, '.');
    if(fileExtIndex < 0)
    {
        fileExtIndex = StringLength(filename);
    }
    char *outFileExt = ".css";
    u32 extensionLen = StringLength(outFileExt);
    char *outFileName = PushArray(arena, char, (u32)fileExtIndex + extensionLen + 1);
    WriteString(filename, fileExtIndex, outFileName);
    
    int len = fileExtIndex;
    AppendString(outFileExt, outFileName, &len);
    
    return outFileName;
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
    
    platform->beginTimer(&platform->lastCounter);
    ParseData(state, file, error);
    float msElapsed = ToMS(platform->outTimerAndDiscard(&platform->lastCounter, "ParseData"));
    
    if(!error)
    {
        //output
        char *outFileName = GenerateOutputFile(&state->arena, file.fileName);
        
        //out?
        output out = {0};
        out.data = PushArray(&state->arena, char, (u32)(1.5f * file.size));
        out.flags |= Output_NewLine;
        
        if(state->encoding)
        {
            AppendStringOutput("@charset \"", &out);
            
            AppendStringOutput(state->encoding, &out);
            
            AppendStringOutput("\";\n\n", &out);
        }
        
        {
            selector_block *root = state->blocks + 1;
            if(root->keyCount)
            {
                AppendStringOutput(":root {\n", &out);
                
                OutVariableDeclaration(state, root, &out);
                
                AppendStringOutput("}\n\n", &out);
            }
        }
        for(u32 masterBlockIndex = 2; masterBlockIndex < state->blockCount; ++masterBlockIndex)
        {
            selector_block *block = state->blocks + masterBlockIndex;
            
            OutBlock(state, block, &out);
        }
        
        msElapsed = ToMS(platform->outTimerAndDiscard(&platform->lastCounter, "CSS Output"));
        
        if(out.dataLen)
        {
            platform->writeFile(outFileName, out.data, StringLength(out.data) - 1);
        }
        
        msElapsed = ToMS(platform->outTimerAndDiscard(&platform->lastCounter, "File I/O (Output)"));
    }
}