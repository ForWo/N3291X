/***************************************************************************
 * Copyright (c) 2011 Nuvoton Technology. All rights reserved.
 *
 * FILENAME
 *     main.c
 * DESCRIPTION
 *     The main file for AES demo code.
 * FUNCTIONS
 *     None
 **************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "wbio.h"
#include "wblib.h"
#include "w55fa95_reg.h"
#include "aes.h"

/*-----------------------------------------------------------------------------
 * For system configuration
 *---------------------------------------------------------------------------*/
// Define   CACHE_ON to locate data buffer on cache memory.
// Undefine CACHE_ON to locate data buffer on non-cache memory.
#define CACHE_ON


/*-----------------------------------------------------------------------------
 * Define Macro
 *---------------------------------------------------------------------------*/
//--- Define the debug mode to show more information for debug
#define DEBUG
#ifdef DEBUG
    #define DBG_PRINTF    sysprintf
#else
    #define DBG_PRINTF(...)
#endif

//--- Define macro for return value
#define OK      TRUE
#define FAIL    FALSE

//--- Define constant for AES
// The maximum byte count in one AES operation for AES engine. It MUST be divisible by 16 and <= 65535.
#define AES_MAX_BCNT    65520

// The buffer size for AES demo.
#define BUFFER_SIZE (AES_MAX_BCNT*3)


/*-----------------------------------------------------------------------------
 * Define Global Variables
 *---------------------------------------------------------------------------*/
__align(4) UINT8 plain_text [BUFFER_SIZE];      // original plain text
__align(4) UINT8 cipher_text[BUFFER_SIZE];      // cipher text buffer for FA95 AES
__align(4) UINT8 plain_text2[BUFFER_SIZE];      // plain text that decrypt by FA95 AES
UINT8 iv[16];
UINT8 cipher_key[32];


/*-----------------------------------------------------------------------------
 * Show data by hex format
 * INPUT:
 *      ptr:    pointer to data that want to show
 *      length: the byte number that want to show
 *---------------------------------------------------------------------------*/
void show_hex_data(unsigned char *ptr, unsigned int length)
{
    unsigned int line_len = 8;  // the hex number will show in one line
    unsigned int jj;

    for (jj=0; jj<length; jj++)
    {
        if (jj % line_len == 0)
            DBG_PRINTF("        ");
        DBG_PRINTF("0x%02x ", *(ptr+jj));
        if (jj % line_len == line_len-1)
            DBG_PRINTF("\n");
    }
    if (jj % line_len != 0)
        DBG_PRINTF("\n");
    return;
}


/*-----------------------------------------------------------------------------
 * Generate random and valid number for AES data length.
 * RETURN:
 *      random and valid number for AES data length
 *---------------------------------------------------------------------------*/
UINT32 random_AES_data_length()
{
    UINT32 data_len;

    data_len = (rand() % BUFFER_SIZE) + 1;      // value range: 1 .. BUFFER_SIZE
    data_len = data_len - (data_len % 16) + 16;     // make sure be divisible by 16
    return data_len;
}


/*-----------------------------------------------------------------------------
 * Basic AES encryption/decryption test
 * INPUT:
 *      key_len:    the code to indicate the bit number of cipher key.
 *      data_len:   the byte number of plain text that want to do AES test.
 * RETURN:
 *      OK:                 for test successful
 *      FAIL or error code: for test fail
 *---------------------------------------------------------------------------*/
int aes_basic_test(KEYSIZE key_len, UINT32 data_len)
{
    long result, i;
    long start_time, end_time;
    UINT8 *ptr_plain_text, *ptr_cipher_text, *ptr_plain_text2;
    UINT8 *ptr_iv, *ptr_cipher_key;

    //--- check data_len
    if(data_len > BUFFER_SIZE)
    {
        sysprintf("ERROR: data length MUST <= %d. Wrong length %d !!\n", BUFFER_SIZE, data_len);
        return FAIL;
    }
    else
        DBG_PRINTF("    Do AES with data length %d bytes...\n", data_len);

#ifdef CACHE_ON
    ptr_plain_text  = plain_text;
    ptr_cipher_text = cipher_text;
    ptr_plain_text2 = plain_text2;
    ptr_iv          = iv;
    ptr_cipher_key  = cipher_key;
#else
    ptr_plain_text  = (UINT8 *)((UINT32)plain_text  | 0x80000000);  // non-cache
    ptr_cipher_text = (UINT8 *)((UINT32)cipher_text | 0x80000000);
    ptr_plain_text2 = (UINT8 *)((UINT32)plain_text2 | 0x80000000);
    ptr_iv          = (UINT8 *)((UINT32)iv          | 0x80000000);
    ptr_cipher_key  = (UINT8 *)((UINT32)cipher_key  | 0x80000000);
#endif

    //--- generate random data for IV key, cipher key, and plain text
    for (i=0; i<16; i++)    // 16 bytes = 128 bits for IV
        ptr_iv[i] = rand() & 0xFF;
    DBG_PRINTF("    IV (128 bits):\n");
    show_hex_data(ptr_iv, 16);

    for (i=0; i<32; i++)    // 32 bytes = 256 bits for cipher key
        ptr_cipher_key[i] = rand() & 0xFF;
    DBG_PRINTF("    Cipher Key (256 bits):\n");
    show_hex_data(ptr_cipher_key, 32);

    memset(ptr_plain_text,  0, BUFFER_SIZE);
    memset(ptr_plain_text2, 0, BUFFER_SIZE);
    memset(ptr_cipher_text, 0, BUFFER_SIZE);
    for (i=0; i<data_len; i++)
        ptr_plain_text[i] = rand() & 0xFF;
    DBG_PRINTF("    Plain Text (first 16 bytes):\n");
    show_hex_data(ptr_plain_text, 16);

    /* clock() requires semihosting. Remove it. */
    //--- do AES encryption
    //start_time = clock();
    result = AES_Encrypt(ptr_plain_text, ptr_cipher_text, data_len, ptr_iv, ptr_cipher_key, key_len);
    //end_time = clock();
    //DBG_PRINTF("    AES hardware encryption need %d ms\n", end_time - start_time);
    if(result != 0)
    {
        sysprintf("ERROR: AES_Encrypt() Error with return code 0x%X!!\n", result);
        return result;
    }
    DBG_PRINTF("    Cipher Text (first 16 bytes):\n");
    show_hex_data(ptr_cipher_text, 16);

    //--- do AES decryption
    //start_time = clock();
    result = AES_Decrypt(ptr_cipher_text, ptr_plain_text2, data_len, ptr_iv, ptr_cipher_key, key_len);
    //end_time = clock();
    //DBG_PRINTF("    AES hardware decryption need %d ms\n", end_time - start_time);
    if(result != 0)
    {
        sysprintf("ERROR: AES_Decrypt() Error with return code 0x%X!!\n", result);
        return result;
    }
    DBG_PRINTF("    Plain Text after Decryption (first 16 bytes):\n");
    show_hex_data(ptr_plain_text2, 16);

    //--- compare original plain text with FA95 AES decrypted
    if (memcmp(ptr_plain_text, ptr_plain_text2, data_len) != 0)
    {
        sysprintf("ERROR: Decryption error !!\n");
        return FAIL;
    }

    DBG_PRINTF("    AES Encryption / Decryption is SUCCESSFUL!!!\n");
    return OK;
}


/*-----------------------------------------------------------------------------*/
int main(void)
{
    UINT32  data_len;
    KEYSIZE key_len;

    /* time() requires semihosting. Remove it. */
    //srand(time(NULL));
    srand(0);
    
    //--- Initial FA95 AES engine
    AES_Initial();

    //--- Do AES encryption / decryption for each cipher key length
    sysprintf("--- AES-128 testing ...\n");
    key_len = KEY_128;
    data_len = random_AES_data_length();
    aes_basic_test(key_len, data_len);

    sysprintf("--- AES-192 testing ...\n");
    key_len = KEY_192;
    data_len = random_AES_data_length();
    aes_basic_test(key_len, data_len);

    sysprintf("--- AES-256 testing ...\n");
    key_len = KEY_256;
    data_len = random_AES_data_length();
    aes_basic_test(key_len, data_len);

    sysprintf("=== BYE ===\n\n");
    while(1);
}