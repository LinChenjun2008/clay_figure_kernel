#ifndef __EFI_SIMPLE_TEXT_OUT__
#define __EFI_SIMPLE_TEXT_OUT__

typedef struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

typedef
EFI_STATUS
(EFIAPI *EFI_TEXT_STRING)
(
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    CHAR16                          *String
);

typedef
EFI_STATUS
(EFIAPI *EFI_TEXT_TEST_STRING)
(
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    CHAR16                          *String
);

typedef
EFI_STATUS
(EFIAPI *EFI_TEXT_QUERY_MODE)
(
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    UINTN                            ModeNumber,
    UINTN                           *Columns,
    UINTN                           *Rows
);

typedef
EFI_STATUS
(EFIAPI *EFI_TEXT_SET_MODE)
(
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    UINTN                            ModeNumber
);

typedef
EFI_STATUS
(EFIAPI *EFI_TEXT_SET_ATTRIBUTE)
(
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    UINTN                            Attribute
);

typedef
EFI_STATUS
(EFIAPI *EFI_TEXT_CLEAR_SCREEN)
(
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This
);

typedef struct
{
    INT32  MaxMode;
    INT32  Mode;
    INT32  Attribute;
    INT32  CursorColumn;
    INT32  CursorRow;
    BOOLEN CursorVisible;
} SIMPLE_TEXT_OUTPUT_MODE;

struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL
{
    UINTN _buf;
    EFI_TEXT_STRING          OutputString;
    EFI_TEXT_TEST_STRING     TestString;
    EFI_TEXT_QUERY_MODE      QueryMode;
    EFI_TEXT_SET_MODE        SetMode;
    EFI_TEXT_SET_ATTRIBUTE   SetAttribute;
    EFI_TEXT_CLEAR_SCREEN    ClearScreen;
    UINTN _buf4[2];
    SIMPLE_TEXT_OUTPUT_MODE *Mode;
};

extern EFI_GUID  gEfiSimpleTextOutProtocolGuid;

#endif