/* camellia.h	ver 1.2.0
 *
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Camellia code.
 *
 * The Initial Developer of the Original Code is
 * NTT(Nippon Telegraph and Telephone Corporation).
 * Portions created by the Initial Developer are Copyright (C) 2006,2007
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef HEADER_CAMELLIA_H
#define HEADER_CAMELLIA_H

#ifdef  __cplusplus
extern "C" {
#endif

#define CAMELLIA_BLOCK_SIZE 16
#define CAMELLIA_TABLE_BYTE_LEN 272
#define CAMELLIA_TABLE_WORD_LEN (CAMELLIA_TABLE_BYTE_LEN / 4)

typedef unsigned int KEY_TABLE_TYPE[CAMELLIA_TABLE_WORD_LEN];


void Camellia_Ekeygen(const int keyBitLength,
		      const unsigned char *rawKey, 
		      KEY_TABLE_TYPE keyTable);

void Camellia_EncryptBlock(const int keyBitLength,
			   const unsigned char *plaintext, 
			   const KEY_TABLE_TYPE keyTable, 
			   unsigned char *cipherText);

void Camellia_DecryptBlock(const int keyBitLength, 
			   const unsigned char *cipherText, 
			   const KEY_TABLE_TYPE keyTable, 
			   unsigned char *plaintext);


#ifdef  __cplusplus
}
#endif

#endif /* HEADER_CAMELLIA_H */
