// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <bootloader.h>

int compare_guid(struct efi_guid *guid1, struct efi_guid *guid2)
{
    return (
        (guid1->data1 == guid2->data1) && (guid1->data2 == guid2->data2) &&
        (guid1->data3 == guid2->data3) &&
        (guid1->data4[0] == guid2->data4[0]) &&
        (guid1->data4[1] == guid2->data4[1]) &&
        (guid1->data4[2] == guid2->data4[2]) &&
        (guid1->data4[3] == guid2->data4[3]) &&
        (guid1->data4[4] == guid2->data4[4]) &&
        (guid1->data4[5] == guid2->data4[5]) &&
        (guid1->data4[6] == guid2->data4[6]) &&
        (guid1->data4[7] == guid2->data4[7])
    );
}