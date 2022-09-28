/* date = July 14th 2022 4:01 pm */

#ifndef MAIN_H
#define MAIN_H

typedef struct file_contents
{
    char *fileName;
    u32 size;
    void *contents;
} file_contents;

typedef struct memory_pool
{
    u8 *memory;
    u32 memorySize;
} memory_pool;

struct app_platform;
typedef file_contents read_entire_file(char *fileName);
typedef void write_entire_file(char *fileName, char *data, int dataLen);
typedef void begin_timer(struct app_platform *platform);
typedef float out_timer_and_discard(struct app_platform *platform);
typedef struct app_platform
{
    memory_pool permanentMemoryPool;
    memory_pool temporaryMemoryPool;
    LARGE_INTEGER lastCounter;
    
    begin_timer *beginTimer;
    out_timer_and_discard *outTimerAndDiscard;
    
    read_entire_file *readFile;
    write_entire_file *writeFile;
} app_platform;

#endif //MAIN_H
