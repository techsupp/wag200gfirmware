/*
 * ipsec_alg TWOFISH cipher stubs
 *
 * Author: JuanJo Ciarlante <jjo-ipsec@mendoza.gov.ar>
 * 
 * $Id: ipsec_alg_twofish.c,v 1.1.1.1 2006-01-16 08:12:14 jeff Exp $
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.  See <http://www.fsf.org/copyleft/gpl.txt>.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 */
#include <linux/config.h>
#include <linux/version.h>

/*	
 *	special case: ipsec core modular with this static algo inside:
 *	must avoid MODULE magic for this file
 */
#if CONFIG_IPSEC_MODULE && CONFIG_IPSEC_ALG_TWOFISH
#undef MODULE
#endif

#include <linux/module.h>
#include <linux/init.h>

#include <linux/kernel.h> /* printk() */
#include <linux/errno.h>  /* error codes */
#include <linux/types.h>  /* size_t */
#include <linux/string.h>

/* Check if __exit is defined, if not null it */
#ifndef __exit
#define __exit
#endif

/*	Low freeswan header coupling	*/
#include "ipsec_alg.h"
#include "libtwofish/twofish.h"
#include "libtwofish/twofish_cbc.h"

#define ESP_TWOFISH		253	/* from ipsec drafts */

/* 128, 192 or 256 */
#define ESP_TWOFISH_KEY_SZ_MIN	16 	/* 128 bit secret key */
#define ESP_TWOFISH_KEY_SZ_MAX	32 	/* 256 bit secret key */
#define ESP_TWOFISH_CBC_BLK_LEN	16	/* TWOFISH-CBC block size */

MODULE_AUTHOR("JuanJo Ciarlante <jjo-ipsec@mendoza.gov.ar>");
static int debug=0;
MODULE_PARM(debug, "i");
static int test=0;
MODULE_PARM(test, "i");
static int excl=0;
MODULE_PARM(excl, "i");
static int keyminbits=0;
MODULE_PARM(keyminbits, "i");
static int keymaxbits=0;
MODULE_PARM(keymaxbits, "i");

static int _twofish_set_key(struct ipsec_alg_enc *alg, __u8 * key_e, const __u8 * key, size_t keysize) {
	twofish_context *ctx=(twofish_context *)key_e;
	if (debug > 0)
		printk(KERN_DEBUG "klips_debug:_twofish_set_key:"
				"key_e=%p key=%p keysize=%d\n",
				key_e, key, keysize);
	twofish_set_key(ctx, key,  keysize);
	return 0;
}
static int _twofish_cbc_encrypt(struct ipsec_alg_enc *alg, __u8 * key_e, __u8 * in, int ilen, const __u8 * iv, int encrypt) {
	twofish_context *ctx=(twofish_context *)key_e;
	if (debug > 0)
		printk(KERN_DEBUG "klips_debug:_twofish_cbc_encrypt:"
				"key_e=%p in=%p ilen=%d iv=%p encrypt=%d\n",
				key_e, in, ilen, iv, encrypt);
	twofish_cbc_encrypt(ctx, in, in, ilen, iv, encrypt);
	return ilen;
}
static struct ipsec_alg_enc ipsec_alg_TWOFISH = {
	ixt_version:	IPSEC_ALG_VERSION,
	ixt_module:	THIS_MODULE,
	ixt_refcnt:	ATOMIC_INIT(0),
	ixt_alg_type:	IPSEC_ALG_TYPE_ENCRYPT,
	ixt_alg_id: 	ESP_TWOFISH,
	ixt_name: 	"twofish",
	ixt_blocksize:	ESP_TWOFISH_CBC_BLK_LEN, 
	ixt_keyminbits:	ESP_TWOFISH_KEY_SZ_MIN * 8,
	ixt_keymaxbits:	ESP_TWOFISH_KEY_SZ_MAX * 8,
	ixt_e_keylen:	ESP_TWOFISH_KEY_SZ_MAX,
	ixt_e_ctx_size:	sizeof(twofish_context),
	ixt_e_set_key:	_twofish_set_key,
	ixt_e_cbc_encrypt:_twofish_cbc_encrypt,
};
	
IPSEC_ALG_MODULE_INIT( ipsec_twofish_init )
{
	int ret, test_ret;
	if (keyminbits)
		ipsec_alg_TWOFISH.ixt_keyminbits=keyminbits;
	if (keymaxbits) {
		ipsec_alg_TWOFISH.ixt_keymaxbits=keymaxbits;
		if (keymaxbits*8>ipsec_alg_TWOFISH.ixt_keymaxbits)
			ipsec_alg_TWOFISH.ixt_e_keylen=keymaxbits*8;
	}
	if (excl) ipsec_alg_TWOFISH.ixt_state |= IPSEC_ALG_ST_EXCL;
	ret=register_ipsec_alg_enc(&ipsec_alg_TWOFISH);
	printk("ipsec_twofish_init(alg_type=%d alg_id=%d name=%s): ret=%d\n", 
				ipsec_alg_TWOFISH.ixt_alg_type, 
				ipsec_alg_TWOFISH.ixt_alg_id, 
				ipsec_alg_TWOFISH.ixt_name, ret);
	if (ret==0 && test) {
		test_ret=ipsec_alg_test(
				ipsec_alg_TWOFISH.ixt_alg_type,
				ipsec_alg_TWOFISH.ixt_alg_id, 
				test);
		printk("ipsec_twofish_init(alg_type=%d alg_id=%d): test_ret=%d\n", 
			ipsec_alg_TWOFISH.ixt_alg_type, 
			ipsec_alg_TWOFISH.ixt_alg_id, 
			ret);
	}
	return ret;
}
IPSEC_ALG_MODULE_EXIT( ipsec_twofish_fini )
{
	unregister_ipsec_alg_enc(&ipsec_alg_TWOFISH);
	return;
}
#ifdef MODULE_LICENSE
MODULE_LICENSE("GPL");

EXPORT_NO_SYMBOLS;
#endif
