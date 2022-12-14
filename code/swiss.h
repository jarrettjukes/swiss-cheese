/* date = July 25th 2022 4:31 pm */

#ifndef SWISS_H
#define SWISS_H

#define MAX_BLOCK_COUNT 4096
#define MAX_BLOCK_CHILD_COUNT 256

#define CHARACTER_VARIABLE    '$'
#define CHARACTER_START_BLOCK '{'
#define CHARACTER_END_BLOCK   '}'
#define CHARACTER_START_VALUE_CAPTURE ':'
#define CHARACTER_END_VALUE_CAPTURE ';'
#define CHARACTER_APPEND_SELECTOR '&'

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

#if 0
typedef struct function
{
    char name[256];
    int nameLength;
    
    char codeBlock[256];
    int codeBlockLength;
} function;
#endif

enum key_value_pair_flags
{
    KVP_VariableReplace = (1 << 0),
    KVP_Variable = (1 << 1),
    KVP_Line = (1 << 2),
    KVP_Comment = (1 << 3),
};

typedef struct key_value_pair
{
    //int blockIndex;
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
    int blockIndex;
    int parentBlockIndex;
    int blockBlockIndex;
    member_name *names;
    int nameCount;
    
    u8 flags;
    
    //key_value_pair keys[16];
    int keys[16];
    u32 keyCount;
    
    //u32 parentCount;
    //struct selector_block *parent;
    
    //struct selector_block *children;
    //u32 childCount;
} selector_block;

typedef struct app_state
{
    memory_arena arena;
    
    char *encoding;
    int encodingLen;
#if 0
    function userDefinedFunctions[16];
    u32 functionCount;
#endif
    u32 linesOfCode; //idk if we want this at all?
    
    key_value_pair variables[MAX_BLOCK_COUNT * 16];
    u32 variableCount;
    
    selector_block blocks[MAX_BLOCK_COUNT];
    u32 blockCount;
    u32 totalBlockCount;
    
    b32 isInitialized;
} app_state;


inline b32 IsFlagSet(u8 flags, u8 flag)
{
    b32 result = flags & flag;
    return result;
}

#endif //SWISS_H
