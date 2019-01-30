/** @file
  APIs for JSON operations.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __BASE_JSON_LIB_H__
#define __BASE_JSON_LIB_H__

#include <Uefi.h>

#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>

typedef    VOID*    EDKII_JSON_VALUE;
typedef    VOID*    EDKII_JSON_ARRAY;
typedef    VOID*    EDKII_JSON_OBJECT;

#define EDKII_JSON_ARRAY_FOREACH(Array, Index, Value) \
  for(Index = 0; \
    Index < JsonArrayCount(Array) && (Value = JsonArrayGetValue(Array, Index)); \
    Index++)

/**
  The function is used to convert a NULL terminated UTF8 encoded string to a JSON
  value. Only object and array represented strings can be converted successfully,
  since they are the only valid root values of a JSON text for UEFI usage.

  Real number and number with exponent part are not supportted by UEFI.

  @param[in]   Text             The NULL terminated UTF8 encoded string to convert

  @retval      Array JSON value or object JSON value, or NULL when any error occurs.

**/
EDKII_JSON_VALUE
EFIAPI
TextToJson (
  IN    CHAR8*    Text
  );

/**
  The function is used to convert the JSON root value to a UTF8 encoded string which
  is terminated by NULL, or return NULL on error.

  Only array JSON value or object JSON value is valid for UEFI usage.

  @param[in]   Json               The JSON value to be converted

  @retval      The JSON value converted UTF8 string or NULL.

**/
CHAR8*
EFIAPI
JsonToText (
  IN    EDKII_JSON_VALUE    Json
  );

/**
  The function is used to initialize a JSON value which contains a new JSON array,
  or NULL on error. Initially, the array is empty.

  @retval      The created JSON value which contains a JSON array or NULL.

**/
EDKII_JSON_VALUE
EFIAPI
JsonValueInitArray (
  VOID
  );

/**
  The function is used to initialize a JSON value which contains a new JSON object,
  or NULL on error. Initially, the object is empty.

  @retval      The created JSON value which contains a JSON object or NULL.

**/
EDKII_JSON_VALUE
EFIAPI
JsonValueInitObject (
  VOID
  );

/**
  The function is used to initialize a JSON value which contains a new JSON string,
  or NULL on error.

  The input string must be NULL terminated Ascii format, non-Ascii characters will
  be processed as an error. Unicode characters can also be represented by Ascii string
  as the format: \u + 4 hexadecimal digits, like \u3E5A, or \u003F.

  @param[in]   String      The Ascii string to initialize to JSON value

  @retval      The created JSON value which contains a JSON string or NULL. Select a
               Getter API for a specific encoding format.

**/
EDKII_JSON_VALUE
EFIAPI
JsonValueInitAsciiString (
  IN    CHAR8    *String
  );

/**
  The function is used to initialize a JSON value which contains a new JSON string,
  or NULL on error.

  The input string must be NULL terminated UCS2 format.

  @param[in]   String      The UCS2 string to initialize to JSON value

  @retval      The created JSON value which contains a JSON string or NULL. Select a
               Getter API for a specific encoding format.

**/
EDKII_JSON_VALUE
EFIAPI
JsonValueInitUCS2String (
  IN    CHAR16    *String
  );

/**
  The function is used to initialize a JSON value which contains a new JSON number,
  or NULL on error.

  The input number must be a positive integer.

  @param[in]   Value       The positive integer to initialize to JSON value

  @retval      The created JSON value which contains a JSON number or NULL.

**/
EDKII_JSON_VALUE
EFIAPI
JsonValueInitNumber (
  IN    UINT64    Value
  );

/**
  The function is used to initialize a JSON value which contains a new JSON boolean,
  or NULL on error.

  @param[in]   Value       The boolean value to initialize.

  @retval      The created JSON value which contains a JSON boolean or NULL.

**/
EDKII_JSON_VALUE
EFIAPI
JsonValueInitBoolean (
  IN    BOOLEAN    Value
  );

/**
  The function is used to initialize a JSON value which contains a new JSON NULL,
  or NULL on error.

  @retval      The created NULL JSON value.

**/
EDKII_JSON_VALUE
EFIAPI
JsonValueInitNull (
  VOID
  );

/**
  The function is used to free a JSON value. If it is an array JSON value or object JSON
  value, all containing elements will also be freed iterately.

  The boolean JSON value and null JSON value won't be freed since they are static values
  kept in memory.

  @param[in]   Json             The JSON value to be freed.

**/
VOID
EFIAPI
JsonValueFree (
  IN    EDKII_JSON_VALUE    Json
  );

/**
  The function is used to return if the provided JSON value contains a JSON array.

  @param[in]   Json             The provided JSON value.

  @retval      TRUE             The JSON value contains a JSON array.
  @retval      FALSE            The JSON value doesn't contain a JSON array.

**/
BOOLEAN
EFIAPI
JsonValueIsArray (
  IN    EDKII_JSON_VALUE    Json
  );

/**
  The function is used to return if the provided JSON value contains a JSON object.

  @param[in]   Json             The provided JSON value.

  @retval      TRUE             The JSON value contains a JSON object.
  @retval      FALSE            The JSON value doesn't contain a JSON object.

**/
BOOLEAN
EFIAPI
JsonValueIsObject (
  IN    EDKII_JSON_VALUE    Json
  );

/**
  The function is used to return if the provided JSON Value contains a string, Ascii or
  UCS2 format is not differentiated.

  @param[in]   Json             The provided JSON value.

  @retval      TRUE             The JSON value contains a JSON string.
  @retval      FALSE            The JSON value doesn't contain a JSON string.

**/
BOOLEAN
EFIAPI
JsonValueIsString (
  IN    EDKII_JSON_VALUE    Json
  );

/**
  The function is used to return if the provided JSON value contains a JSON number.

  @param[in]   Json             The provided JSON value.

  @retval      TRUE             The JSON value is contains JSON number.
  @retval      FALSE            The JSON value doesn't contain a JSON number.

**/
BOOLEAN
EFIAPI
JsonValueIsNumber (
  IN    EDKII_JSON_VALUE    Json
  );

/**
  The function is used to return if the provided JSON value contains a JSON boolean.

  @param[in]   Json             The provided JSON value.

  @retval      TRUE             The JSON value contains a JSON boolean.
  @retval      FALSE            The JSON value doesn't contain a JSON boolean.

**/
BOOLEAN
EFIAPI
JsonValueIsBoolean (
  IN    EDKII_JSON_VALUE    Json
  );

/**
  The function is used to return if the provided JSON value contains a JSON NULL.

  @param[in]   Json             The provided JSON value.

  @retval      TRUE             The JSON value contains a JSON NULL.
  @retval      FALSE            The JSON value doesn't contain a JSON NULL.

**/
BOOLEAN
EFIAPI
JsonValueIsNull (
  IN    EDKII_JSON_VALUE    Json
  );

/**
  The function is used to retrieve the associated value of JSON array.

  @param[in]   Json             The provided JSON value.

  @retval      Return the associated value of JSON array or NULL.

**/
EDKII_JSON_ARRAY
EFIAPI
JsonValueGetArray (
  IN    EDKII_JSON_VALUE    Json
  );

/**
  The function is used to retrieve the associated value of JSON object.

  @param[in]   Json             The provided JSON value.

  @retval      Return the associated value of JSON object or NULL.

**/
EDKII_JSON_OBJECT
EFIAPI
JsonValueGetObject (
  IN    EDKII_JSON_VALUE    Json
  );

/**
  The function is used to retrieve the associated value of JSON string as Ascii
  format.

  The retuned string is read only and must not be modified or freed by the caller.

  @param[in]   Json             The provided JSON value.

  @retval      Return the associated value of JSON string or NULL.

**/
CONST CHAR8*
EFIAPI
JsonValueGetAsciiString (
  IN    EDKII_JSON_VALUE    Json
  );

/**
  The function is used to retrieve the associated value of JSON string as UCS2
  format.

  Caller is responsible to free the returned string.

  @param[in]   Json             The provided JSON value.

  @retval      Return the associated value of JSON string or NULL.

**/
CHAR16*
EFIAPI
JsonValueGetUCS2String (
  IN    EDKII_JSON_VALUE    Json
  );

/**
  The function is used to retrieve the associated value of JSON number.

  The input JSON value should not be NULL or contain no JSON number, otherwise
  it will ASSERT() and return 0.

  @param[in]   Json             The provided JSON value.

  @retval      Return the associated value of JSON number.

**/
UINT64
EFIAPI
JsonValueGetNumber (
  IN    EDKII_JSON_VALUE    Json
  );

/**
  The function is used to retrieve the associated value of JSON boolean.

  The input JSON value should not be NULL or contain no JSON boolean, otherwise
  it will ASSERT() and return FALSE.

  @param[in]   Json             The provided JSON value.

  @retval      Return the associated value of JSON boolean.

**/
BOOLEAN
EFIAPI
JsonValueGetBoolean (
  IN    EDKII_JSON_VALUE    Json
  );

/**
  The function is used to get the number of elements in a JSON object, or 0 if it is NULL or
  not a JSON object.

  @param[in]   JsonObject              The provided JSON object.

  @retval      Return the number of elements in this JSON object or 0.

**/
UINTN
EFIAPI
JsonObjectSize (
  IN    EDKII_JSON_OBJECT    JsonObject
  );

/**
  The function is used to enumerate all keys in a JSON object.

  Caller should be responsible to free the returned key array refference. But contained keys
  are read only and must not be modified or freed.

  @param[in]   JsonObj                The provided JSON object for enumeration.
  @param[out]  KeyCount               The count of keys in this JSON object.

  @retval      Return an array of the enumerated keys in this JSON object or NULL.

**/
CONST CHAR8**
JsonObjectGetKeys (
  IN    EDKII_JSON_OBJECT    JsonObj,
  OUT   UINTN                *KeyCount
  );

/**
  The function is used to get a JSON value corresponding to the input key from a JSON object.

  The key must be a valid NULL terminated UTF8 encoded string. Returns NULL if key is not found
  or any error occurs.

  @param[in]   JsonObj           The provided JSON object.
  @param[in]   Key               The key of the JSON value to be retrieved.

  @retval      Return the corresponding JSON value mapped by key, or NULL on error.

**/
EDKII_JSON_VALUE
EFIAPI
JsonObjectGetValue (
  IN    EDKII_JSON_OBJECT    JsonObj,
  IN    CHAR8                *Key
  );

/**
  The function is used to set a JSON value corresponding to the input key from a JSON object.

  The key must be a valid NULL terminated UTF8 encoded string. If there already is a value for
  key, it is replaced by the new value.

  The JSON value to be set should not be referred as an element by other JSON objects or JSON
  arrays.

  @param[in]   JsonObj                The provided JSON object.
  @param[in]   Key                    The key of the JSON value to be set.
  @param[in]   Json                   The JSON Value to set to this JSON object defined by key.

  @retval      EFI_ABORTED            Some error occur and operation aborted.
  @retval      EFI_SUCCESS            The JSON value has been set to this JSON object.

**/
EFI_STATUS
EFIAPI
JsonObjectSetValue (
  IN    EDKII_JSON_OBJECT    JsonObj,
  IN    CHAR8                *Key,
  IN    EDKII_JSON_VALUE     Json
  );

/**
  The function is used to get the number of elements in a JSON array, or 0 if it is NULL or
  not a JSON array.

  @param[in]   JsonArray              The provided JSON array.

  @retval      Return the number of elements in this JSON array or 0.

**/
UINTN
EFIAPI
JsonArrayCount (
  IN    EDKII_JSON_ARRAY    JsonArray
  );

/**
  The function is used to return the JSON value in the array at position index. The valid range
  for this index is from 0 to the return value of JsonArrayCount() minus 1.

  If this array is NULL or not a JSON array, or if index is out of range, NULL will be returned.

  @param[in]   JsonArray         The provided JSON Array.

  @retval      Return the JSON value located in the Index position or NULL.

**/
EDKII_JSON_VALUE
EFIAPI
JsonArrayGetValue (
  IN    EDKII_JSON_ARRAY    JsonArray,
  IN    UINTN               Index
  );

/**
  The function is used to append a JSON value to the end of the JSON array, and grow the size of
  array by 1.

  The JSON value to be appended should not be referred as an element by other JSON objects or JSON
  arrays.

  @param[in]   JsonArray              The provided JSON object.
  @param[in]   Json                   The JSON value to append.

  @retval      EFI_ABORTED            Some error occur and operation aborted.
  @retval      EFI_SUCCESS            JSON value has been appended to the end of the JSON array.

**/
EFI_STATUS
EFIAPI
JsonArrayAppendValue (
  IN    EDKII_JSON_ARRAY    JsonArray,
  IN    EDKII_JSON_VALUE    Json
  );

/**
  The function is used to remove a JSON value at position index, shifting the elements after index
  one position towards the start of the array.

  @param[in]   JsonArray              The provided JSON array.
  @param[in]   Index                  The Index position before removement.

  @retval      EFI_ABORTED            Some error occur and operation aborted.
  @retval      EFI_SUCCESS            The JSON array has been removed at position index.

**/
EFI_STATUS
EFIAPI
JsonArrayRemoveValue (
  IN    EDKII_JSON_ARRAY    JsonArray,
  IN    UINTN               Index
  );

#endif
