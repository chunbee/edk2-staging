/** @file
  Internal Functions for x-uefi keyword operations.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __HII_KEYWORD_LIB_H__
#define __HII_KEYWORD_LIB_H__

#include <Uefi.h>
#include <Protocol/HiiConfigKeyword.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>

typedef enum {
  KeywordTypeBuffer,
  KeywordTypeNumeric1,
  KeywordTypeNumeric2,
  KeywordTypeNumeric4,
  KeywordTypeNumeric8
} KEYWORD_TYPE;

/**
  Update the Keyword value.

  This function update value by the specified keyword. ASSERT() when result format from keyword handler
  protocol GetData is not correct.

  @param[in]   DevicePath         The Device path related to specified Keyword instance.
                                  If NULL, will retrive the first instance match to the keyword.
  @param[in]   Keyword            Pointer of the Keyword String.
  @param[in]   Value              Pointer of the New Value String which will be Setted.
  @param[in]   Type               If not NULL, the Keyword value will be processed by the type.
                                  If NULL, will be decided by predefined type.

  @retval EFI_SUCCESS             The configuration processed successfully.
  @retval EFI_INVALID_PARAMETER   Any input or configured parameter is invalid.
  @retval EFI_NOT_FOUND           The KeywordString or NamespaceId was not found.
  @retval EFI_OUT_OF_RESOURCES    Required system resources could not be allocated.
  @retval EFI_ACCESS_DENIED       The action violated system policy.
  @retval EFI_DEVICE_ERROR        An unexpected system error occurred.

**/
EFI_STATUS
KeywordConfigSetValue (
  IN   EFI_DEVICE_PATH_PROTOCOL      *DevicePath, OPTIONAL
  IN   CHAR16                        *Keyword,
  IN   CHAR16                        *Value,
  IN   KEYWORD_TYPE                   Type
  );

/**
  Get the original Keyword Value.

  This function Get value by the specified keyword. ASSERT() when result format from keyword handler
  protocol GetData is not correct.

  @param[in]   DevicePath         The Device path related to specified Keyword instance.
                                  If NULL, will retrive the first instance match to the keyword.
  @param[in]   Keyword            Pointer of the Keyword String.
  @param[out]  Value              Pointer of the Original Value String specified by keyword.Caller
                                  takes the responsibility to free memory.
  @param[out]  Length             Length of the Original Value in Bytes.

  @retval EFI_SUCCESS             The configuration processed successfully.
  @retval EFI_INVALID_PARAMETER   Any input or configured parameter is invalid.
  @retval EFI_NOT_FOUND           The KeywordString or NamespaceId was not found.
  @retval EFI_OUT_OF_RESOURCES    Required system resources could not be allocated.
  @retval EFI_ACCESS_DENIED       The action violated system policy.
  @retval EFI_DEVICE_ERROR        An unexpected system error occurred.

**/
EFI_STATUS
KeywordConfigGetValue (
  IN     EFI_DEVICE_PATH_PROTOCOL        *DevicePath, OPTIONAL
  IN     CHAR16                          *Keyword,
     OUT VOID                            **Value,
     OUT UINTN                           *Length
  );

#endif
