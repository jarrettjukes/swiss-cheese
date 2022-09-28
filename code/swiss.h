/* date = July 25th 2022 4:31 pm */

#ifndef SWISS_H
#define SWISS_H

#define MAX_BLOCK_CHILD_COUNT 16

enum output_flags
{
    Output_NewLine = (1 << 0),
    Output_Indent = (1 << 1),
};

typedef struct output
{
    char *data;
    int dataLen;
    u8 flags;
} output;

//void AppendString(char *src, int srcLen, output *out);

typedef struct member_name
{
    char *name;
    int len;
    
    char combinationChar;
} member_name;

typedef struct function
{
    char name[256];
    int nameLength;
    
    char codeBlock[256];
    int codeBlockLength;
} function;

enum variable_flags
{
    Variable_NoReplace = (1 << 0),
};

enum key_value_pair_flags
{
    KVP_VariableNoReplace = (1 << 0),
    KVP_Variable = (1 << 1),
    KVP_Line = (1 << 2),
    KVP_Comment = (1 << 3),
};

typedef struct key_value_pair
{
    char *name;
    int nameLength;
    
    char *value;
    int valueLength;
    
    u8 flags;
} key_value_pair;

enum block_flags
{
    Block_wrapper = (1 << 0),
    Block_append = (1 << 1),
    Block_prepend = (1 << 2),
    Block_completed = (1 << 3),
};

typedef struct selector_block
{
    member_name *names;
    int nameCount;
    
    //key_value_pair *lines;
    //u32 lineCount;
    
    //key_value_pair *variables;
    //u32 variableCount;
    
    key_value_pair *keys;
    u32 keyCount;
    
    char *commentData;
    int commentDataLen;
    
    u8 flags;
    
    //u32 parentCount;
    struct selector_block *parent;
    
    struct selector_block *children;
    u32 childCount;
} selector_block;

typedef struct app_state
{
    mem_struct workingMem;
    
    char *encoding;
    int encodingLen;
    
    function userDefinedFunctions[16];
    u32 functionCount;
    
    u32 linesOfCode; //idk if we want this at all?
    
    key_value_pair *variables;
    u32 variableCount;
    
    char *commentData;
    int commentDataLen;
    
    selector_block blocks[256];
    u32 blockCount;
    u32 totalBlockCount;
    
    
    b32 isInitialized;
} app_state;

#endif //SWISS_H
