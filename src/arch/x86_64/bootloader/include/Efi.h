#ifndef _EFI_H_
#define _EFI_H_


//*******************************************************
// Attributes
//*******************************************************

// #define EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL  0x00000001
#define EFI_OPEN_PROTOCOL_GET_PROTOCOL        0x00000002
// #define EFI_OPEN_PROTOCOL_TEST_PROTOCOL       0x00000004
// #define EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER 0x00000008
// #define EFI_OPEN_PROTOCOL_BY_DRIVER           0x00000010
// #define EFI_OPEN_PROTOCOL_EXCLUSIVE           0x00000020

// #define EVT_TIMER                         0x80000000
// #define EVT_RUNTIME                       0x40000000
// #define EVT_NOTIFY_WAIT                   0x00000100
// #define EVT_NOTIFY_SIGNAL                 0x00000200
// #define EVT_SIGNAL_EXIT_BOOT_SERVICES     0x00000201
// #define EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE 0x60000202

/*************************************************
* 基本类型
*************************************************/

#include <Uefi.h>

#define RETURN_ERROR(StatusCode)  (((INTN)(RETURN_STATUS)(StatusCode)) < 0)
#define EFI_ERROR(A)  RETURN_ERROR(A)

#define EFI_SUCCESS     0
#define EFI_ERR         0x8000000000000000
#define EFI_UNSUPPORTED (EFI_ERR | 3)


#include <Procotol/GraphicsOutput.h>
#include <Procotol/SimpleFileSystem.h>

extern EFI_HANDLE                         gImageHandle;
extern EFI_SYSTEM_TABLE                   *gST;
extern EFI_BOOT_SERVICES                  *gBS;

#endif