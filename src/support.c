/*
 * Irssi2_silc - a SILC module for Irssi2
 * Copyright (C) 2005 Andrej Kacian
 *
 * - Support functions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <stdlib.h>

#include "lib.h"
#include "local-presence.h"
#include "local-user.h"
#include "event.h"

#include "silc.h"
#include "silc-local-presence.h"

unsigned int verify_message_signature(SilcClientEntry sender __attr_unused__,
		SilcMessageSignedPayload sig __attr_unused__,
		SilcMessagePayload payload __attr_unused__)
{
	/* FIXME: do signature check - return "dunno" for now */
	return -1;
}

/* A convenience function for creating "silc_event" events. */
struct event *silc_event_new(const char *name)
{
        struct event *event = event_new(SILC_EVENT);
	event_add_bool(event, "raw");
        event_add(event, "event", name);
        return event;
}

char *i_silc_gen_key_path(struct local_presence *lp, bool private_key)
{
	char *path = malloc(512);

	snprintf(path, 511, "%s/.irssi2/silc-gen-%s.%s", getenv("HOME"),
			lp->name, (private_key ? "prv" : "pub") );
	return path;
}

bool i_silc_load_keys(struct i_silc_local_presence_auth *auth, SilcPKCS *pkcs,
		SilcPublicKey *public_key, SilcPrivateKey *private_key)
{
	unsigned char *cp, *old, *data, byte, tmp[32], keymat[64];
	SilcUInt32 i, data_len, len, blocklen, passphrase_len, mac_len;
	SilcCipher aes;
	SilcHash sha1;
	SilcHmac sha1hmac;

	data = strdup(auth->public_key);
	old = strdup(auth->public_key);
	if( !data || !old )
		return FALSE;
	data_len = strlen(auth->public_key);

	/* Check start of file and remove header from the data. */
	len = strlen(SILC_PKCS_PUBLIC_KEYFILE_BEGIN);
	cp = data;
	for (i = 0; i < len; i++) {
		byte = cp[0]; 
		cp++;
		if (byte != SILC_PKCS_PUBLIC_KEYFILE_BEGIN[i]) {
			memset(old, 0, data_len);
			silc_free(old);
			return FALSE;
		}
	}
	data = cp;

	/* Decode public key */
	if (public_key) {
		len = data_len - (strlen(SILC_PKCS_PUBLIC_KEYFILE_BEGIN) +
				  strlen(SILC_PKCS_PUBLIC_KEYFILE_END));

		data = silc_pem_decode(data, len, &len);
		memset(old, 0, data_len);
		silc_free(old);
		old = data;
		data_len = len;

		if (!data || !silc_pkcs_public_key_decode(data, len,
					public_key)) {
			memset(old, 0, data_len);
			silc_free(old);
			return FALSE;
		}
	}

	memset(old, 0, data_len);
	silc_free(old);

	/* ************************************ */

	data = malloc(auth->private_key->used + 1);
	old = malloc(auth->private_key->used + 1);
	memcpy(data, auth->private_key->data, auth->private_key->used);
	data_len = auth->private_key->used;

	passphrase_len = strlen(auth->passphrase);

	/* Check start of file and remove header from the data. */
	len = strlen(SILC_PKCS_PRIVATE_KEYFILE_BEGIN);
	cp = data;
	for (i = 0; i < len; i++) {
		byte = cp[0];
		cp++;
		if (byte != SILC_PKCS_PRIVATE_KEYFILE_BEGIN[i]) {
			memset(old, 0, data_len);
			silc_free(old);
			return FALSE;
		}
	}
	data = cp;

	/* Decode private key */
	len = data_len - (strlen(SILC_PKCS_PRIVATE_KEYFILE_BEGIN) +
			  strlen(SILC_PKCS_PRIVATE_KEYFILE_END));

	memset(tmp, 0, sizeof(tmp));
	memset(keymat, 0, sizeof(keymat));

	/* Allocate the AES cipher */
	if (!silc_cipher_alloc("aes-256-cbc", &aes)) {
		memset(old, 0, data_len);
		silc_free(old);
		return FALSE;
	}
	blocklen = silc_cipher_get_block_len(aes);
	if (blocklen * 2 > sizeof(tmp)) {
		memset(old, 0, data_len);
		silc_free(old);
		return FALSE;
	}

	/* Allocate SHA1 hash */
	if (!silc_hash_alloc("sha1", &sha1)) {
		silc_cipher_free(aes);
		memset(old, 0, data_len);
		silc_free(old);
		return FALSE;
	}

	/* Allocate HMAC */
	if (!silc_hmac_alloc("hmac-sha1-96", NULL, &sha1hmac)) {
		silc_hash_free(sha1);
		silc_cipher_free(aes);
		memset(old, 0, data_len);
		silc_free(old);
		return FALSE;
	}

	/* Derive the decryption key from the provided key material.  The key
	 * is 256 bits length, and derived by taking hash of the data, then
	 * re-hashing the data and the previous digest, and using the first and
	 * second digest as the key. */
	silc_hash_init(sha1);
	silc_hash_update(sha1, auth->passphrase, passphrase_len);
	silc_hash_final(sha1, keymat);
	silc_hash_init(sha1);
	silc_hash_update(sha1, auth->passphrase, passphrase_len);
	silc_hash_update(sha1, keymat, 16);
	silc_hash_final(sha1, keymat + 16);

	/* Set the key to the cipher */
	silc_cipher_set_key(aes, keymat, 256);

	/* First, verify the MAC of the private key data */
	mac_len = silc_hmac_len(sha1hmac);
	silc_hmac_init_with_key(sha1hmac, keymat, 16);
	silc_hmac_update(sha1hmac, data, len - mac_len);
	silc_hmac_final(sha1hmac, tmp, NULL);
	if (memcmp(tmp, data + (len - mac_len), mac_len)) {
		memset(keymat, 0, sizeof(keymat));
		memset(tmp, 0, sizeof(tmp));
		silc_hmac_free(sha1hmac);
		silc_hash_free(sha1);
		silc_cipher_free(aes);
		memset(old, 0, data_len);
		silc_free(old);
		return FALSE;
	}
	data += 4;
	len -= 4;

	/* Decrypt the private key buffer */
	silc_cipher_decrypt(aes, data, data, len - mac_len, NULL);
	SILC_GET32_MSB(i, data);
	if (i > len) {
		memset(keymat, 0, sizeof(keymat)); 
		memset(tmp, 0, sizeof(tmp)); 
		silc_hmac_free(sha1hmac); 
		silc_hash_free(sha1);
		silc_cipher_free(aes);
		memset(old, 0, data_len);
		silc_free(old);
		return FALSE;
	}
	data += 4;
	len = i;

	/* Cleanup */
	memset(keymat, 0, sizeof(keymat));
	memset(tmp, 0, sizeof(tmp));
	silc_hmac_free(sha1hmac);
	silc_hash_free(sha1);
	silc_cipher_free(aes);

	/* Now decode the actual private key */
	if (!silc_pkcs_private_key_decode(data, len, private_key)) {
		memset(old, 0, data_len);
		silc_free(old);
		return FALSE;
	}

	memset(old, 0, data_len);
	silc_free(old);

	/* Make SilcPKCS */
	if( pkcs ) {
		silc_pkcs_alloc((*public_key)->name, pkcs);
		silc_pkcs_public_key_set(*pkcs, *public_key);
		silc_pkcs_private_key_set(*pkcs, *private_key);
	}

	return TRUE;
}
