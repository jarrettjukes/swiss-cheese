/* date = July 25th 2022 4:29 pm */

#ifndef PLATFORM_H
#define PLATFORM_H

#include "stdint.h"
#include "stdio.h"
//#include "string.h"

#define Assert(expr) if(!(expr)) { *(int *)0 = 1;}

#define internal static
#define global static
#define local static

#define ArrayCount(a) (sizeof(a) / sizeof((a)[0]))

#define Kilobytes(b) b * 1024
#define Megabytes(b) b * Kilobytes(1024)

#define ToMS(s) s * 1000.0f
#define TOUS(ms) ms * 1000.0f

typedef uint32_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

typedef u32 b32;
typedef size_t mem_index;

#define true 1
#define false 0

typedef struct memory_arena
{
    mem_index size;
    u8 *base;
    mem_index used;
} memory_arena;

#define PushStruct(memStruct, type) (type *)PushStruct_(memStruct, sizeof(type))
#define PushArray(memStruct, type, size) (type *)PushStruct_(memStruct, sizeof(type) * size)

inline void *PushStruct_(memory_arena *mem, mem_index size)
{
    Assert((mem->used + size) <= mem->size);
    void *result = mem->base + mem->used;
    mem->used += size;
    return result;
}

#define PopArray(memStruct, charP, type, size) PopStruct_(memStruct, charP, sizeof(type) * size)
internal void PopStruct_(memory_arena *mem, void *p, size_t size)
{
    //Assert(size);
    char *a = (char *)p;
    for(size_t i = 0; i < size; ++i)
    {
        *a++ = 0;
    }
    
    mem->used -= size;
}

inline int StringLength(char *str)
{
    int result = 0;
    while(str[result]) result++;
    return result;
}

internal int IndexOf(char *str, char c)
{
    int result = 0;
    int len = StringLength(str);
    
    while(str[result] != c)
    {
        result++;
        if(result > len) return -1;
    }
    
    //if(result > len) return -1;
    return result;
}

#if 0
internal void AppendString(char *src, int srcLen, char *dest, int *destLen)
{
    Assert(src && srcLen && dest);
    
    for(int srcIndex = 0; srcIndex < srcLen; ++srcIndex)
    {
        *dest++ = *src++;
        if(destLen) (*destLen)++;
    }
}
#endif
internal void AppendString(char *src, char *dest, int *destLen)
{
    Assert(src && dest);
    
    for(char *s = src; *s; ++s)
    {
        *(dest + (*destLen)++) = *s;
        //if(destLen) (*destLen)++;
    }
}

internal b32 StringExactMatch(char *a, int aLen, char *b, int bLen)
{
    if(aLen != bLen) return false;
    for(int aIndex = 0; aIndex < aLen; ++aIndex)
    {
        if(a[aIndex] != b[aIndex]) return false;
    }
    
    return true;
}

internal b32 Contains(char *table, char target)
{
    u32 tableLen = ArrayCount(table);
    
    while(*table)
    {
        if(*table++ == target)
        {
            return true;
        }
    }
    
    return false;
}

typedef struct error_details
{
    u32 code;//?
    char description[1024];
} error_details;

internal b32 StringContains(char *a, int aLen, char *b, int bLen)
{
    for(int bIndex = 0; bIndex < bLen; ++bIndex)
    {
        for(int aIndex = 0; aIndex < aLen; ++aIndex)
        {
            if((*(a + aIndex)) == (*(b + bIndex)))
            {
                return true;
            }
        }
    }
    
    return false;
}

internal int EarliestOccurrenceOf(char *a, int aLen, char *b, int bLen)
{
    for(int bIndex = 0; bIndex < bLen; ++bIndex)
    {
        for(int aIndex = 0; aIndex < aLen; ++aIndex)
        {
            if((*(a + aIndex)) == (*(b + bIndex)))
            {
                return bIndex;
            }
        }
    }
    
    return -1;
}


internal void WriteString(char *src, int srcLen, char *dest)
{
    for(int srcIndex = 0; srcIndex < srcLen; ++srcIndex)
    {
        *dest++ = *src++;
    }
    
    *(dest + srcLen) = '\0';
}

#if 0
inline void *memset(void *pointer, int bytes, size_t size)
{
    char *data = (char *)pointer;
    for(char byteIndex = 0; byteIndex < size; ++byteIndex)
    {
        *data++ = (char)bytes;
    }
    
    return pointer;
}
#endif
#endif //PLATFORM_H
