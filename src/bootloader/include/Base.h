#ifndef __BASE_H__
#define __BASE_H__

#define CONST  const
#define STATIC static

#define VOID   void

#ifdef EFIAPI
    #elif defined (_MSC_EXTENSIONS)
        #define EFIAPI  __cdecl
    #elif defined (__GNUC__)
        #define EFIAPI __attribute__((ms_abi))
    #else
        #define EFIAPI
#endif

#define IN
#define OUT

#define NULL ((void*) 0)

#define TRUE  (1 == 1)
#define FALSE (1 == 0)

typedef signed char          INT8;
typedef signed short         INT16;
typedef signed int           INT32;
typedef signed long long int INT64;

typedef unsigned char          BOOLEN;
typedef unsigned char          UINT8;
typedef unsigned short         UINT16;
typedef unsigned int           UINT32;
typedef unsigned long long int UINT64;

typedef unsigned char  CHAR8;
typedef unsigned short CHAR16;

/* UINTN 和 INTN
* 32 位 CPU -> 32 bit
* 64 位 CPU -> 64 bit
*/
typedef UINT64 UINTN;
typedef INT64  INTN;

typedef struct
{
    UINT32 Data1;
    UINT16 Data2;
    UINT16 Data3;
    UINT8  Data4[8];
} GUID;

typedef UINTN RETURN_STATUS;

#endif