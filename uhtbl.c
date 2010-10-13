/**
 *   uhtbl - Generic coalesced hash table implementation
 *   Copyright (C) 2010 Steven Barth <steven@midlink.org>
 *   Copyright (C) 2010 John Crispin <blogic@openwrt.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "uhtbl.h"

/* Forward static helpers */
UHTBL_INLINE uhtbl_size_t _uhtbl_address(uhtbl_t *tbl, uhtbl_bucket_t *bucket);
UHTBL_INLINE uhtbl_bucket_t* _uhtbl_bucket(uhtbl_t *tbl, uhtbl_size_t address);
static uhtbl_bucket_t* _uhtbl_allocate(uhtbl_t *tbl);
static uhtbl_bucket_t* _uhtbl_find(uhtbl_t *tbl, const void *key,
long len, uhtbl_bucket_t **previous, uhtbl_size_t *mainaddress);



UHTBL_API int uhtbl_init(uhtbl_t *tbl, uint32_t bucketsize,
uhtbl_size_t sizehint, uhtbl_hash_t *fct_hash, uhtbl_gc_t *fct_gc) {
	sizehint = (sizehint) ? sizehint : UHTBL_MINIMUMSIZE;
	tbl->bucketsize = bucketsize;
	tbl->fct_hash = fct_hash;
	tbl->fct_gc = fct_gc;
	if (!tbl->fct_hash || tbl->bucketsize < sizeof(uhtbl_bucket_t)) {
		return UHTBL_EINVAL;
	}
	tbl->payload = 0;
	tbl->buckets = NULL;
	tbl->used = 0;

	return uhtbl_resize(tbl, sizehint);
}

UHTBL_API void uhtbl_clear(uhtbl_t *tbl) {
	for (uhtbl_size_t i = 0; i < tbl->size; i++) {
		uhtbl_bucket_t *bucket = _uhtbl_bucket(tbl, i);
		if (tbl->fct_gc && bucket->head.flags & UHTBL_FLAG_OCCUPIED) {
			tbl->fct_gc(bucket);
		}
		bucket->head.flags = 0;
	}
	tbl->used = 0;
	tbl->nextfree = tbl->size - 1;
}

UHTBL_API void* uhtbl_get (uhtbl_t *tbl, const void *key, long len) {
	return _uhtbl_find(tbl, key, len, NULL, NULL);
}

UHTBL_API void* uhtbl_set (uhtbl_t *tbl, void *key, long len) {
	int localkey = 0;
	uint16_t keysize = len, flags;
	if (!key) {		/* Check whether key is treated as a pointer */
		key = &len;
		keysize = sizeof(len);
		localkey = 1;
	}

	uhtbl_size_t mainaddr;
	uhtbl_bucket_t *resolve, *match, *prev = NULL;
	uhtbl_bucket_t *lookup = _uhtbl_find(tbl, key, keysize, &prev, &mainaddr);

	if (lookup) {
		match = lookup;
		flags = match->head.flags;
		if (flags & UHTBL_FLAG_OCCUPIED) {	/* We are replacing an entry */
			if (tbl->fct_gc) {
				tbl->fct_gc(match);
			}
			tbl->used--;
		}
	} else {
		match = prev;
		flags = match->head.flags;
		if ((flags & UHTBL_FLAG_STRANGER)
		&& _uhtbl_address(tbl, match) == mainaddr) {
		/* Mainposition occupied by key with different hash -> move it away */

			/* Find old prev and update its next pointer */
			if ((flags & UHTBL_FLAG_LOCALKEY)) {
				_uhtbl_find(tbl, 0,
				 match->head.key.handle, &prev, NULL);
			} else {
				_uhtbl_find(tbl, match->head.key.ptr,
							 match->head.keysize, &prev, NULL);
			}

			if (!(resolve = _uhtbl_allocate(tbl))) {
				if (!uhtbl_resize(tbl, tbl->payload * UHTBL_GROWFACTOR)) {
					return uhtbl_set(tbl, (localkey) ? NULL : key, len);
				} else {
					return NULL;
				}
			}
			memcpy(resolve, match, tbl->bucketsize);	/* Copy bucket data */
			prev->head.next = _uhtbl_address(tbl, resolve);
			flags = 0;
		} else if ((flags & UHTBL_FLAG_OCCUPIED) &&
		(match->head.keysize != keysize || memcmp((flags & UHTBL_FLAG_LOCALKEY)
			? &match->head.key.handle : match->head.key.ptr, key, keysize))) {
			/* Mainposition has different key (but same hash) */
			if (!(resolve = _uhtbl_allocate(tbl))) {
				if (!uhtbl_resize(tbl, tbl->payload * UHTBL_GROWFACTOR)) {
					return uhtbl_set(tbl, (localkey) ? NULL : key, len);
				} else {
					return NULL;
				}
			}

			prev = match;
			match = resolve;
			flags = UHTBL_FLAG_STRANGER; /* We will not be in main position */
			prev->head.flags |= UHTBL_FLAG_WITHNEXT; /* Main now has next */
			prev->head.next = _uhtbl_address(tbl, match); /* main->next = us */
		}
	}

	if (localkey) {
		match->head.key.handle = len;
		flags |= UHTBL_FLAG_LOCALKEY;
	} else {
		match->head.key.ptr = key;
		flags &= ~UHTBL_FLAG_LOCALKEY;
	}
	match->head.keysize = keysize;
	flags |= UHTBL_FLAG_OCCUPIED;
	match->head.flags = flags;

	tbl->used++;
	return match;
}

UHTBL_API void* uhtbl_next(uhtbl_t *tbl, uhtbl_size_t *iter) {
	for (; *iter < tbl->size; (*iter)++) {
		if (_uhtbl_bucket(tbl, *iter)->head.flags & UHTBL_FLAG_OCCUPIED) {
			return _uhtbl_bucket(tbl, (*iter)++);
		}
	}
	return NULL;
}

UHTBL_API int uhtbl_remove(uhtbl_t *tbl, uhtbl_head_t *head) {
	void *key;
	long len;
	uhtbl_key(head, &key, &len);
	return uhtbl_unset(tbl, key, len);
}

UHTBL_API int uhtbl_unset(uhtbl_t *tbl, const void *key, long len) {
	uhtbl_bucket_t *prev = NULL;
	uhtbl_bucket_t *bucket = _uhtbl_find(tbl, key, len, &prev, NULL);
	if (!bucket) {
		return UHTBL_ENOENT;
	}

	if (tbl->fct_gc) {
		tbl->fct_gc(bucket);
	}

	bucket->head.flags &= ~UHTBL_FLAG_OCCUPIED;
	tbl->used--;

	/* If not in main position, get us out of the next-list */
	if (bucket->head.flags & UHTBL_FLAG_STRANGER) {
		if (bucket->head.flags & UHTBL_FLAG_WITHNEXT) {/* We had next buckets */
			prev->head.next = bucket->head.next;	/* Give them to out prev */
		} else {
			prev->head.flags &= ~UHTBL_FLAG_WITHNEXT;/* We were the only next */
		}
		bucket->head.flags = 0;
	}

	uhtbl_size_t address = _uhtbl_address(tbl, bucket);
	if (address > tbl->nextfree) {
		tbl->nextfree = address;
	}

	return UHTBL_OK;
}

UHTBL_API int uhtbl_resize(uhtbl_t *tbl, uhtbl_size_t payload) {
	uhtbl_size_t size = payload / UHTBL_PAYLOADFACTOR;
	if (size < payload || size < tbl->used) {
		return UHTBL_EINVAL;
	}
	if (payload == tbl->payload) {
		return UHTBL_OK;
	}

	void *buckets = calloc(size, tbl->bucketsize);
	if (!buckets) {
		return UHTBL_ENOMEM;
	}

	uhtbl_t oldtbl;	/* Save essentials of table for rehashing */
	oldtbl.buckets = tbl->buckets;
	oldtbl.bucketsize = tbl->bucketsize;
	oldtbl.size = tbl->size;
	oldtbl.used = tbl->used;

	tbl->buckets = buckets;
	tbl->payload = payload;
	tbl->size = size;
	tbl->used = 0;
	tbl->nextfree = size - 1;

	if (oldtbl.used) {	/* Rehash if table had entries before */
		uhtbl_size_t iter = 0;
		uhtbl_bucket_t *old, *new;
		while ((old = uhtbl_next(&oldtbl, &iter))) {
			new = uhtbl_set(tbl, (old->head.flags & UHTBL_FLAG_LOCALKEY)
					? NULL : old->head.key.ptr,
			(old->head.flags & UHTBL_FLAG_LOCALKEY)
					? old->head.key.handle : old->head.keysize);
			new->head.user = old->head.user;
			memcpy(((char*)new) + sizeof(uhtbl_head_t),
				((char*)old) + sizeof(uhtbl_head_t),
				tbl->bucketsize - sizeof(uhtbl_head_t));
		}
	}

	free(oldtbl.buckets);
	return UHTBL_OK;
}

UHTBL_API void uhtbl_finalize(uhtbl_t *tbl) {
	uhtbl_clear(tbl);
	free(tbl->buckets);
	tbl->buckets = NULL;
}

UHTBL_API void uhtbl_key(uhtbl_head_t *head, void **key, long *len) {
	if (key) {
		*key = (head->flags & UHTBL_FLAG_LOCALKEY)
				? NULL : head->key.ptr;
	}
	if (len) {
		*len = (head->flags & UHTBL_FLAG_LOCALKEY)
				? head->key.handle : head->keysize;
	}
}

UHTBL_API void uhtbl_gc_key(void *bucket) {
	void *key;
	uhtbl_key(bucket, &key, NULL);
	free(key);
}


/* Static auxiliary */

UHTBL_INLINE uhtbl_size_t _uhtbl_address(uhtbl_t *tbl, uhtbl_bucket_t *bucket) {
	return ((uint8_t*)bucket - (uint8_t*)tbl->buckets) / tbl->bucketsize;
}

UHTBL_INLINE uhtbl_bucket_t* _uhtbl_bucket(uhtbl_t *tbl, uhtbl_size_t address) {
	return (uhtbl_bucket_t*)
			((uint8_t*)tbl->buckets + (address * tbl->bucketsize));
}

static uhtbl_bucket_t* _uhtbl_allocate(uhtbl_t *tbl) {
	uhtbl_size_t address = tbl->nextfree;
	do {
		uhtbl_bucket_t *bucket = _uhtbl_bucket(tbl, address);
		if (!(bucket->head.flags & UHTBL_FLAG_OCCUPIED)) {
			if (bucket->head.flags & UHTBL_FLAG_WITHNEXT) {
				/* Empty bucket still has a successor -> swap it with its */
				/* successor and return the old successor-bucket as free */
				uhtbl_bucket_t *old = bucket;
				bucket = _uhtbl_bucket(tbl, old->head.next);
				memcpy(old, bucket, tbl->bucketsize);
				old->head.flags &= ~UHTBL_FLAG_STRANGER; /* sucessor now main */
			}
			/* WARN: If set will ever fail in the future we'd take care here */
			tbl->nextfree = (address) ? address - 1 : 0;
			return bucket;
		}
	} while(address--);
	return NULL;
}

static uhtbl_bucket_t* _uhtbl_find(uhtbl_t *tbl, const void *key,
long len, uhtbl_bucket_t **previous, uhtbl_size_t *mainaddress) {
	uint16_t keysize = len;
	if (!key) {
		key = &len;
		keysize = sizeof(len);
	}
	uhtbl_size_t address = tbl->fct_hash(key, keysize) % tbl->payload;
	uhtbl_bucket_t *buck = _uhtbl_bucket(tbl, address);
	if (mainaddress) {
		*mainaddress = address;
		if (!(buck->head.flags & UHTBL_FLAG_OCCUPIED)) {
			return buck;
		}
	}
	if (buck->head.flags & UHTBL_FLAG_STRANGER) {
		if (previous) {
			*previous = buck;
		}
		return NULL;
	}
	for (;; buck = _uhtbl_bucket(tbl, address)) {
		if (buck->head.flags & UHTBL_FLAG_OCCUPIED
		&& buck->head.keysize == keysize
		&& !memcmp((buck->head.flags & UHTBL_FLAG_LOCALKEY)
		? &buck->head.key.handle : buck->head.key.ptr, key, keysize)) {
			return buck;
		}
		if (!(buck->head.flags & UHTBL_FLAG_WITHNEXT)) {
			if (previous) {
				*previous = buck;
			}
			return NULL;
		}

		address = buck->head.next;
		if (previous) {
			*previous = buck;
		}
	}
}
