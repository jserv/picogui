/*
 * ssl.c v0.0.3
 * Copyright (C) 2000  --  DaP <profeta@freemail.c3.hu>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#include <openssl/ssl.h>		  /* SSL_() */
#include <openssl/err.h>		  /* ERR_() */
#include <time.h>					  /* asctime() */
#include <string.h>				  /* strncpy() */
#include "ssl.h"					  /* struct cert_info */
#include "../../config.h"		  /* HAVE_SNPRINTF */

#ifndef HAVE_SNPRINTF
#define snprintf g_snprintf
#endif

/* globals */
static struct chiper_info chiper_info;		/* static buffer for _SSL_get_cipher_info() */
static char err_buf[256];			/* generic error buffer */


/* +++++ Internal functions +++++ */


#if 0
static void *
mmalloc(size_t size)
{
	void *addr;


	if (!(addr = malloc(size))) {
		perror("malloc");
		/* FATAL */
		exit(1);
	}
	return (addr);
}
#endif


static void
__SSL_fill_err_buf (char *funcname)
{
	int err;
	char buf[256];


	err = ERR_get_error ();
	ERR_error_string (err, buf);
	snprintf (err_buf, sizeof (err_buf), "%s: %s (%d)\n", funcname, buf, err);
}


static void
__SSL_critical_error (char *funcname)
{
	__SSL_fill_err_buf (funcname);
	fprintf (stderr, "%s\n", err_buf);

	exit (1);
}


/* +++++ Cipher functions +++++ */


/*
int
_SSL_EVP_encode(char *data, int len)
{
	EVP_ENCODE_CTX ctx;
	int i, j, n, outl;
	char tbuf[PEM_BUFSIZE * 5];
	char *buf;
	char *pt;


	buf = malloc(len);
	*buf = 0;

	EVP_EncodeInit(&ctx);
	i = j = 0;
	while (len > 0) {
		n = (len > (PEM_BUFSIZE * 5)) ? (PEM_BUFSIZE * 5) : len;
		EVP_EncodeUpdate(&ctx, tbuf, &outl, &(data[j]), n);

fprintf(stderr, "_SSL_EVP_encode :: loop give %d bytes\n", outl);
		if (!outl) {
			free (buf);
			return (0);
		}
		strcat(buf, tbuf);

		i += outl;
		len -= n;
		j += n;
	}
	EVP_EncodeFinal(&ctx, tbuf, &outl);
fprintf(stderr, "_SSL_EVP_encode :: encoded data is %d bytes\n", i);
	for (pt = buf; *pt; pt++)
		if (*pt == '\n')
			*pt = '_';

	memcpy(data, buf, i + 1);	// + NULL
	free (buf);

	return (1);
}
*/


#if 0
#define	ALG	EVP_des_ede3_cbc()
#define	MAXBLK	512
/* FIXME */
static int
_SSL_do_cipher(char *buf, int buf_len, char *key, int operation, char **pt)
{
	EVP_CIPHER_CTX ectx;
        unsigned char iv[EVP_MAX_IV_LENGTH];
	char ebuf[MAXBLK];
	int ebuflen;
	int n;
	int i;


	memset(iv, 0, EVP_MAX_IV_LENGTH);
	EVP_CipherInit(&ectx, ALG, key, iv, operation);

	*pt = mmalloc(buf_len + EVP_CIPHER_CTX_block_size(&ectx));	/* + PAD */

	i = 0;
	while (buf_len - i > 0) {
		n = (buf_len - i < MAXBLK) ? buf_len - i : MAXBLK;
		EVP_CipherUpdate(&ectx, ebuf, &ebuflen, buf + i, n);
		printf("EVP_CipherUpdate[%d] ebl %d i %d T %d (%d)\n", operation, ebuflen, i, buf_len, n);

		if (!ebuflen)	/* last block needs padding */
			break;

		memcpy(*pt + i, ebuf, ebuflen);
		i += ebuflen;
break;
	}
	/* append/check CRC block */
        if (!EVP_CipherFinal(&ectx, ebuf, &ebuflen))
		fprintf(stderr, "_SSL_do_cipher :: EVP_CipherFinal failed\n");
	memcpy(*pt + i, ebuf, ebuflen);
	i += ebuflen;
	printf("EVP_CipherFinal %d (%d)\n", ebuflen, i);

	return (i);
}
#endif


#if 0
static char *
_SSL_do_cipher_base64(char *buf, int buf_len, char *key, int operation)
{
	char *pt;
	char *pt2;
	int i;


	if (operation) {
		i = _SSL_do_cipher(buf, buf_len, key, operation, &pt);
		pt2 = mmalloc(i * 2 + 1);		/* + NULL */
		memset(pt2, 0, i * 2 + 1);	/* FIXME: need it? */
		if ((i = EVP_EncodeBlock(pt2, pt, i)) == -1) {
			fprintf(stderr, "_SSL_do_cipher_base64 :: EVP_EncodeBlock failed\n");
			exit(1);
		}
fprintf(stderr, "_SSL_do_cipher_base64 :: EVP_EncodeBlock %d [%24s]\n", i, key);
	} else {
		pt = mmalloc(buf_len / 2 * 2 + 1);		/* + NULL */
		memset(pt, 0, buf_len / 2 * 2 + 1);	/* FIXME: need it? */
		if ((i = EVP_DecodeBlock(pt, buf, buf_len)) == -1) {
			fprintf(stderr, "_SSL_do_cipher_base64 :: EVP_DecodeBlock failed\n");
			exit(1);
		}
fprintf(stderr, "_SSL_do_cipher_base64 :: EVP_DecodeBlock %d [%24s]\n", i, key);
		i -= i % 8;	/* cut padding */
		i = _SSL_do_cipher(pt, i, key, operation, &pt2);
	}
	free (pt);

	return (pt2);
}
#endif


/* +++++ Object functions +++++ */


#if 0
static void *
_SSL_get_sess_obj(SSL *ssl, int type)
{
	void *obj = NULL;


	switch (type) {
	    case 0:
		obj = X509_get_pubkey(SSL_get_certificate(ssl));
		break;
	    case 1:
		obj = SSL_get_privatekey(ssl);
		break;
	    case 2:
		obj = SSL_get_certificate(ssl);
		break;
	}
	return (obj);
}
#endif


#if 0
static char *
_SSL_get_obj_base64(void *s, int type)
{
	unsigned char *pt, *ppt;
	unsigned char *t;
	int len = 0;
	int i;


	switch (type) {
	    case 0:
		len = i2d_PublicKey(s, NULL);
		break;
	    case 1:
		len = i2d_PrivateKey(s, NULL);
		break;
	    case 2:
		len = i2d_X509(s, NULL);
		break;
	}
	if (len < 0)
		return (NULL);

	pt = ppt = mmalloc(len);

	switch (type) {
	    case 0:
		i2d_PublicKey(s, &pt);
		break;
	    case 1:
		i2d_PrivateKey(s, &pt);
		break;
	    case 2:
		i2d_X509(s, &pt);
		break;
	}

	t = mmalloc(len * 2 + 1);	/* + NULL */
	if ((i = EVP_EncodeBlock(t, ppt, len)) == -1) {
		fprintf(stderr, "_SSL_get_key_base64 :: EVP_EncodeBlock failed\n");
		exit(1);
	}
	free (ppt);

	return (t);
}
#endif


#if 0
static char *
_SSL_get_ctx_obj_base64(SSL_CTX *ctx, int type)
{
	void *obj;
	unsigned char *pt;
	SSL *ssl;


	if (!(ssl = SSL_new(ctx)))
		__SSL_critical_error("_SSL_get_ctx_obj_base64 :: SSL_new");

	obj = _SSL_get_sess_obj(ssl, type);	/* it's just a pointer into ssl! */
	pt = _SSL_get_obj_base64(obj, type);

	SSL_free(ssl);

	return (pt);
}
#endif


#if 0
static int
_SSL_verify_x509(X509 *x509)
{
	X509_STORE *cert_ctx = NULL;
	X509_LOOKUP *lookup = NULL;
	X509_STORE_CTX csc;
	int i;


	if (!(cert_ctx = X509_STORE_new())) {
		fprintf(stderr, "_SSL_verify_x509 :: X509_STORE_new failed\n");
		exit(1);
	}
	/* X509_STORE_set_verify_cb_func(cert_ctx, cb); */

/*
	if (!(lookup = X509_STORE_add_lookup(cert_ctx,X509_LOOKUP_file()))) {
		fprintf(stderr, "_SSL_verify_x509 :: X509_STORE_add_lookup failed\n");
		exit(1);
	}
	if (!X509_LOOKUP_load_file(lookup, NULL, X509_FILETYPE_DEFAULT)) {
		fprintf(stderr, "_SSL_verify_x509 :: X509_LOOKUP_load_file failed\n");
		exit(1);
	}
*/

	if (!(lookup = X509_STORE_add_lookup(cert_ctx,X509_LOOKUP_hash_dir()))) {
		fprintf(stderr, "_SSL_verify_x509 :: X509_STORE_add_lookup failed\n");
		exit(1);
	}
	if (!!X509_LOOKUP_add_dir(lookup, NULL, X509_FILETYPE_DEFAULT)) {
		fprintf(stderr, "_SSL_verify_x509 :: X509_LOOKUP_add_dir failed\n");
		exit(1);
	}

	/* ... */
	X509_STORE_CTX_init(&csc, cert_ctx, x509, NULL);
	i = X509_verify_cert(&csc);
	X509_STORE_CTX_cleanup(&csc);
	/* ... */

	X509_STORE_free(cert_ctx);

	return (i);
}
#endif


/* +++++ SSL functions +++++ */


SSL_CTX *
_SSL_context_init (void (*info_cb_func), int server)
{
	SSL_CTX *ctx;
#ifdef WIN32
	int i, r;
#endif

	SSLeay_add_ssl_algorithms ();
	SSL_load_error_strings ();
	ctx = SSL_CTX_new (server ? SSLv3_server_method() : SSLv3_client_method ());

	SSL_CTX_set_session_cache_mode (ctx, SSL_SESS_CACHE_BOTH);
	SSL_CTX_set_timeout (ctx, 300);

	/* used in SSL_connect(), SSL_accept() */
	SSL_CTX_set_info_callback (ctx, info_cb_func);

#ifdef WIN32
	/* under win32, OpenSSL needs to be seeded with some randomness */
	srand (time (0));
	for (i = 0; i < 128; i++)
	{
		r = rand ();
		RAND_seed ((unsigned char *)&r, sizeof (r));
	}
#endif

	return(ctx);
}


#if 0
static void
_SSL_add_random_keypair(SSL_CTX *ctx, int bits)
{
	RSA *rsa;


	rsa = RSA_generate_key(bits, RSA_F4, NULL, NULL);
	if (!SSL_CTX_set_tmp_rsa(ctx, rsa))
		__SSL_critical_error("SSL_CTX_set_tmp_rsa");
	RSA_free(rsa);

	/* force use of this key for key exchange */
	SSL_CTX_set_options(ctx, SSL_OP_EPHEMERAL_RSA);
}
#endif


#if 0
static char *
_SSL_add_keypair (SSL_CTX *ctx, char *privkey, char *cert)
{
	if (SSL_CTX_use_PrivateKey_file (ctx, privkey, SSL_FILETYPE_PEM) <= 0)
	{
		__SSL_fill_err_buf ("SSL_CTX_use_PrivateKey_file");
		return (err_buf);
	}
	if (SSL_CTX_use_certificate_file (ctx, cert, SSL_FILETYPE_PEM) <= 0)
	{
		__SSL_fill_err_buf ("SSL_CTX_use_certificate_file");
		return (err_buf);
	}

	if (!SSL_CTX_check_private_key (ctx))
	{
		__SSL_fill_err_buf
			("Private key does not match the certificate public key\n");
		return (err_buf);
	}

	return (NULL);
}
#endif


static struct tm tmtm;
static struct tm *
ASN1_GENERALIZEDTIME_snprintf (ASN1_GENERALIZEDTIME * tm)
{
	char *v;
	int gmt = 0;
	int i;
	int y = 0, M = 0, d = 0, h = 0, m = 0, s = 0;


	i = tm->length;
	v = (char *) tm->data;

	if (i < 12)
		return (NULL);
	if (v[i - 1] == 'Z')
		gmt = 1;
	for (i = 0; i < 12; i++)
		if ((v[i] > '9') || (v[i] < '0'))
			return (NULL);
	y =
		(v[0] - '0') * 1000 + (v[1] - '0') * 100 + (v[2] - '0') * 10 + (v[3] -
																							 '0');
	M = (v[4] - '0') * 10 + (v[5] - '0');
	if ((M > 12) || (M < 1))
		return (NULL);
	d = (v[6] - '0') * 10 + (v[7] - '0');
	h = (v[8] - '0') * 10 + (v[9] - '0');
	m = (v[10] - '0') * 10 + (v[11] - '0');
	if ((v[12] >= '0') && (v[12] <= '9') && (v[13] >= '0') && (v[13] <= '9'))
		s = (v[12] - '0') * 10 + (v[13] - '0');

	tmtm.tm_sec = s;
	tmtm.tm_min = m;
	tmtm.tm_hour = h;
	tmtm.tm_mday = d;
	tmtm.tm_mon = M - 1;
	tmtm.tm_year = y;

	/* snprintf (buf, buf_len, "%s %2d %02d:%02d:%02d %d%s", mon[M - 1], d, h, m, s, y, (gmt) ? " GMT" : ""); */
	return (&tmtm);
}


static struct tm *
ASN1_UTCTIME_snprintf (ASN1_UTCTIME * tm)
{
	char *v;
	int gmt = 0;
	int i;
	int y = 0, M = 0, d = 0, h = 0, m = 0, s = 0;


	i = tm->length;
	v = (char *) tm->data;

	if (i < 10)
		return (NULL);
	if (v[i - 1] == 'Z')
		gmt = 1;
	for (i = 0; i < 10; i++)
		if ((v[i] > '9') || (v[i] < '0'))
			return (NULL);
	y = (v[0] - '0') * 10 + (v[1] - '0');
	if (y < 50)
		y += 100;
	M = (v[2] - '0') * 10 + (v[3] - '0');
	if ((M > 12) || (M < 1))
		return (NULL);
	d = (v[4] - '0') * 10 + (v[5] - '0');
	h = (v[6] - '0') * 10 + (v[7] - '0');
	m = (v[8] - '0') * 10 + (v[9] - '0');
	if ((v[10] >= '0') && (v[10] <= '9') && (v[11] >= '0') && (v[11] <= '9'))
		s = (v[10] - '0') * 10 + (v[11] - '0');

	tmtm.tm_sec = s;
	tmtm.tm_min = m;
	tmtm.tm_hour = h;
	tmtm.tm_mday = d;
	tmtm.tm_mon = M - 1;
	tmtm.tm_year = y;

	/* snprintf (buf, buf_len, "%s %2d %02d:%02d:%02d %d%s", mon[M - 1], d, h, m, s, y + 1900, (gmt) ? " GMT" : ""); */
	return (&tmtm);
}


static void
ASN1_TIME_snprintf (char *buf, int buf_len, ASN1_TIME * tm)
{
	struct tm *tmtm;


	switch (tm->type)
	{
	    case V_ASN1_UTCTIME:
		tmtm = ASN1_UTCTIME_snprintf (tm);
		break;
	    case V_ASN1_GENERALIZEDTIME:
		tmtm = ASN1_GENERALIZEDTIME_snprintf (tm);
		break;
	    default:
		tmtm = NULL;
	}

	if (!tmtm)
	{
		snprintf (buf, buf_len, "ASN1_TIME_snprintf :: Invalid date");
		return;
	}

	snprintf (buf, buf_len, "%s", asctime (tmtm));
	*(strchr (buf, '\n')) = 0;
}


static void
broke_oneline (char *oneline, char *parray[])
{
	char *pt, *ppt;
	int i;


	i = 0;
	ppt = pt = oneline + 1;
	while ((pt = strchr (pt, '/')))
	{
		*pt = 0;
		parray[i++] = ppt;
		ppt = ++pt;
	}
	parray[i++] = ppt;
	parray[i] = NULL;
}


/*
    FIXME: Master-Key, Extensions, CA bits
	    (openssl x509 -text -in servcert.pem)
*/
int
_SSL_get_cert_info (struct cert_info *cert_info, SSL * ssl)
{
	X509 *peer_cert;
	EVP_PKEY *peer_pkey;
	/* EVP_PKEY *ca_pkey; */
	/* EVP_PKEY *tmp_pkey; */
	char notBefore[64];
	char notAfter[64];
	int alg;
	int sign_alg;


	if (!(peer_cert = SSL_get_peer_certificate (ssl)))
		return (1);				  /* FATAL? */

	X509_NAME_oneline (X509_get_subject_name (peer_cert), cert_info->subject,
							 sizeof (cert_info->subject));
	X509_NAME_oneline (X509_get_issuer_name (peer_cert), cert_info->issuer,
							 sizeof (cert_info->issuer));
	broke_oneline (cert_info->subject, cert_info->subject_word);
	broke_oneline (cert_info->issuer, cert_info->issuer_word);

	alg = OBJ_obj2nid (peer_cert->cert_info->key->algor->algorithm);
	sign_alg = OBJ_obj2nid (peer_cert->sig_alg->algorithm);
	ASN1_TIME_snprintf (notBefore, sizeof (notBefore),
							  X509_get_notBefore (peer_cert));
	ASN1_TIME_snprintf (notAfter, sizeof (notAfter),
							  X509_get_notAfter (peer_cert));

	peer_pkey = X509_get_pubkey (peer_cert);

	strncpy (cert_info->algorithm,
				(alg == NID_undef) ? "UNKNOWN" : OBJ_nid2ln (alg),
				sizeof (cert_info->algorithm));
	cert_info->algorithm_bits = EVP_PKEY_bits (peer_pkey);
	strncpy (cert_info->sign_algorithm,
				(sign_alg == NID_undef) ? "UNKNOWN" : OBJ_nid2ln (sign_alg),
				sizeof (cert_info->sign_algorithm));
	/* EVP_PKEY_bits(ca_pkey)); */
	cert_info->sign_algorithm_bits = 0;
	strncpy (cert_info->notbefore, notBefore, sizeof (cert_info->notbefore));
	strncpy (cert_info->notafter, notAfter, sizeof (cert_info->notafter));

	EVP_PKEY_free (peer_pkey);

	/* SSL_SESSION_print_fp(stdout, SSL_get_session(ssl)); */
/*
	if (ssl->session->sess_cert->peer_rsa_tmp) {
		tmp_pkey = EVP_PKEY_new();
		EVP_PKEY_assign_RSA(tmp_pkey, ssl->session->sess_cert->peer_rsa_tmp);
		cert_info->rsa_tmp_bits = EVP_PKEY_bits (tmp_pkey);
		EVP_PKEY_free(tmp_pkey);
	} else
		fprintf(stderr, "REMOTE SIDE DOESN'T PROVIDES ->peer_rsa_tmp\n");
*/

	X509_free (peer_cert);

	return (0);
}


struct chiper_info *
_SSL_get_cipher_info (SSL * ssl)
{
	SSL_CIPHER *c;


	c = SSL_get_current_cipher (ssl);
	strncpy (chiper_info.version, SSL_CIPHER_get_version (c),
				sizeof (chiper_info.version));
	strncpy (chiper_info.chiper, SSL_CIPHER_get_name (c),
				sizeof (chiper_info.chiper));
	SSL_CIPHER_get_bits (c, &chiper_info.chiper_bits);

	return (&chiper_info);
}


int
_SSL_send (SSL * ssl, char *buf, int len)
{
	int num;


	num = SSL_write (ssl, buf, len);

	switch (SSL_get_error (ssl, num))
	{
	case SSL_ERROR_SSL:			  /* setup errno! */
		/* ??? */
		__SSL_fill_err_buf ("SSL_write");
		fprintf (stderr, "%s\n", err_buf);
		break;
	case SSL_ERROR_SYSCALL:
		/* ??? */
		perror ("SSL_write/write");
		break;
	case SSL_ERROR_ZERO_RETURN:
		/* fprintf(stderr, "SSL closed on write\n"); */
		break;
	}

	return (num);
}


int
_SSL_recv (SSL * ssl, char *buf, int len)
{
	int num;


	num = SSL_read (ssl, buf, len);

	switch (SSL_get_error (ssl, num))
	{
	case SSL_ERROR_SSL:
		/* ??? */
		__SSL_fill_err_buf ("SSL_read");
		fprintf (stderr, "%s\n", err_buf);
		break;
	case SSL_ERROR_SYSCALL:
		/* ??? */
		perror ("SSL_read/read");
		break;
	case SSL_ERROR_ZERO_RETURN:
		/* fprintf(stdeerr, "SSL closed on read\n"); */
		break;
	}

	return (num);
}


SSL *
_SSL_socket (SSL_CTX *ctx, int sd)
{
	SSL *ssl;


	if (!(ssl = SSL_new (ctx)))
		/* FATAL */
		__SSL_critical_error ("SSL_new");

	SSL_set_fd (ssl, sd);
	if (ctx->method == SSLv3_client_method())
		SSL_set_connect_state (ssl);
	else
	        SSL_set_accept_state(ssl);

	return (ssl);
}


char *
_SSL_set_verify (SSL_CTX *ctx, void *verify_callback, char *cacert)
{
	if (!SSL_CTX_set_default_verify_paths (ctx))
	{
		__SSL_fill_err_buf ("SSL_CTX_set_default_verify_paths");
		return (err_buf);
	}

	if (cacert)
	{
		if (!SSL_CTX_load_verify_locations (ctx, cacert, NULL))
		{
			__SSL_fill_err_buf ("SSL_CTX_load_verify_locations");
			return (err_buf);
		}
	}

	SSL_CTX_set_verify (ctx, SSL_VERIFY_PEER, verify_callback);

	return (NULL);
}


void
_SSL_close (SSL * ssl)
{
	SSL_set_shutdown (ssl, SSL_SENT_SHUTDOWN | SSL_RECEIVED_SHUTDOWN);
	SSL_free (ssl);
	ERR_remove_state (0);		  /* free state buffer */
}
