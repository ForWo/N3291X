/***************************************************************************
 * Copyright (c) 2011 Nuvoton Technology. All rights reserved.
 *
 * FILENAME
 *     aes.h
 * DESCRIPTION
 *     The header file of AES library.
 * FUNCTIONS
 *     None
 **************************************************************************/
#ifndef _AES_H
#define _AES_H

#include "wbio.h"
#include "wbtypes.h"
#include "wberrcode.h"

//--- Define Error Code for AES
#define AES_ERR_FAIL        (AES_ERR_ID|0x01)
#define AES_ERR_DATA_LEN    (AES_ERR_ID|0x02)
#define AES_ERR_DATA_BUF    (AES_ERR_ID|0x03)
#define AES_ERR_CIPHER_KEY  (AES_ERR_ID|0x04)
#define AES_ERR_IV          (AES_ERR_ID|0x05)
#define AES_ERR_MODE        (AES_ERR_ID|0x06)
#define AES_ERR_BUS_ERROR   (AES_ERR_ID|0x07)


//--- Define valid macro for AES cipher key length
typedef enum
{
    KEY_128 = 0,
    KEY_192,
    KEY_256
} KEYSIZE;


//--- Declare the API prototype for AES driver
// Initial AES
VOID AES_Initial(VOID);

// Encrypt input_buf by AES CBC mode.
int AES_Encrypt(
    UINT8*  input_buf,      // pointer to plain text buffer
    UINT8*  output_buf,     // pointer to cipher text buffer
    UINT32  input_len,      // length of plain text
    UINT8*  iv,             // pointer to initial vector
    UINT8*  key,            // pointer to cipher key
    KEYSIZE key_size        // length of cipher key
    );

// Decrypt input_buf by AES CBC mode.
int AES_Decrypt(
    UINT8*  input_buf,      // pointer to cipher text buffer
    UINT8*  output_buf,     // pointer to plain text buffer
    UINT32  input_len,      // length of cipher text
    UINT8*  iv,             // pointer to initial vector
    UINT8*  key,            // pointer to cipher key
    KEYSIZE key_size        // length of cipher key
    );

// Enable AES interrupt feature
VOID AES_Enable_Interrupt(VOID);

// Disable AES interrupt feature
VOID AES_Disable_Interrupt(VOID);

#endif  // end of _AES_H