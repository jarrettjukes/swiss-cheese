#include "windows.h"
#include "platform.h"

#include "main.h"
#include "swiss.h"
#include "swiss.cpp"

internal file_contents Win32ReadEntireFile(char *fileName)
{
    file_contents result = {};
    
    HANDLE fileHandle = CreateFileA(fileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    
    if(fileHandle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER fileSize;
        if(GetFileSizeEx(fileHandle, &fileSize))
        {
            u32 fileSize32 = (u32)fileSize.QuadPart;
            
            if(fileSize32)
            {
                result.contents = VirtualAlloc(0, fileSize32, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
                DWORD bytesRead;
                if(ReadFile(fileHandle, result.contents, fileSize32, &bytesRead, 0))
                {
                    result.fileName = fileName;
                    result.size = fileSize32;
                }
                else
                {
                    VirtualFree(result.contents, 0, MEM_RELEASE);
                    result = {};
                }
            }
        }
    }
    
    return result; //29
}

internal void Win32WriteEntireFile(char *fileName, char *data, int dataLen)
{
    HANDLE file = CreateFileA(fileName, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
    if(file != INVALID_HANDLE_VALUE)
    {
        DWORD bytesWritten = 0;
        if(WriteFile(file, (void *)data, dataLen, &bytesWritten, 0) && bytesWritten > 0)
        {
        }
        
        CloseHandle(file);
    }
}

inline LARGE_INTEGER Win32GetWallclock()
{
    LARGE_INTEGER result;
    QueryPerformanceCounter(&result);
    
    return result;
}

global LARGE_INTEGER GlobalPerfFrequency;

inline float Win32GetSecondsElapsed(LARGE_INTEGER start, LARGE_INTEGER end)
{
    float result = (float)(end.QuadPart - start.QuadPart) / GlobalPerfFrequency.QuadPart;
    return result;
}

internal void beginTimer(app_platform *platform)
{
    platform->lastCounter = Win32GetWallclock();
}

internal float outTimerAndDiscard(app_platform *platform)
{
    LARGE_INTEGER clock = Win32GetWallclock();
    float result = Win32GetSecondsElapsed(platform->lastCounter, clock);
    platform->lastCounter = clock;
    return result;
}

int main(int argC, char **args)
{
    int fileIndex = argC - (argC - 1);
    char *fileName = *(args + fileIndex);
    
    b32 recompileFlag = false;
    b32 always = false;
    
    QueryPerformanceFrequency(&GlobalPerfFrequency); //counts per second
    
    for(int argIndex = fileIndex; argIndex < argC; ++argIndex)
    {
        char *d = *(args + argIndex);
        for(int compilerSwitchIndex = 0; compilerSwitchIndex < StringLength(d); ++compilerSwitchIndex)
        {
            char switchChar = *d;
            if(switchChar == '-' || switchChar == '/')
            {
                d++;
            }
            else
            {
                break;
            }
        }
        
        if(strcmp(d, "watch") == 0)
        {
            recompileFlag = true;
        }
        
        if(strcmp(d, "ignore-time") == 0)
        {
            always = true;
        }
    }
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    
    float currentMS = 1000.0f * (t.QuadPart / GlobalPerfFrequency.QuadPart);
    
    if(fileName)
    {
        app_platform platform = {};
        platform.writeFile = Win32WriteEntireFile;
        platform.readFile = Win32ReadEntireFile;
        platform.beginTimer = beginTimer;
        platform.outTimerAndDiscard = outTimerAndDiscard;
        
        platform.permanentMemoryPool.memorySize = Megabytes(500);
        platform.permanentMemoryPool.memory = (u8 *)VirtualAlloc(0, platform.permanentMemoryPool.memorySize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        
        
        platform.temporaryMemoryPool.memorySize = Megabytes(250);
        platform.temporaryMemoryPool.memory = (u8 *)VirtualAlloc(0, platform.temporaryMemoryPool.memorySize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        
        file_contents file = {};
        FILETIME lastWriteTime = {0};
        error_details *error = 0;
        
        LARGE_INTEGER workCounter = Win32GetWallclock();
        do
        {
            WIN32_FILE_ATTRIBUTE_DATA fileAttributes = {};
            GetFileAttributesExA(fileName, GetFileExInfoStandard, &fileAttributes);
            b32 inputFileChanged = CompareFileTime(&fileAttributes.ftLastWriteTime, &lastWriteTime) > 0;
            if(inputFileChanged || always)
            {
                if(file.contents)
                {
                    VirtualFree(file.contents, 0, MEM_RELEASE);
                }
                file = Win32ReadEntireFile(fileName);
                lastWriteTime = fileAttributes.ftLastWriteTime;
            }
            
            if(file.contents && inputFileChanged)
            {
                ProcessData(&platform, file, error);
                float secondsElapsed = Win32GetSecondsElapsed(workCounter, Win32GetWallclock());
                printf("File process time: %.4f s, %.4f ms", secondsElapsed, ToMS(secondsElapsed));
                workCounter = Win32GetWallclock();
            }
            
            if(error) break;
        } while(recompileFlag);
    }
    
    return 0;
}