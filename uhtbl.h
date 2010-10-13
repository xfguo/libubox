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
/**
 * uhtbl is a coalesced cellared generic hash table implementation with the aim
 * to be both small in code size and heap memory requirements. This hash table
 * uses a hybrid approach called coalesced addressing which means that pointers
 * to other buckets will be used in the case of a collisions. In this case no
 * linked lists have to be used and less allocation calls have to be done.
 * To improve performance this hash table carries a cellar for collision
 * handling which is an additional address area that lies behind the
 * hash-addressable space.
 *
 * Overhead (on x86 32bit):
 * Bookkeeping: 32 Bytes (per hash table)
 * Metadata: 12 Bytes including pointer to key and keysize (per bucket)
 *
 */

#ifndef UHTBL_H_
#define UHTBL_H_


/* compile-time configurables */

#ifndef UHTBL_PAYLOADFACTOR
	#define UHTBL_PAYLOADFACTOR 0.86
#endif

#ifndef UHTBL_GROWFACTOR
	#define UHTBL_GROWFACTOR 2
#endif

#ifndef UHTBL_MINIMUMSIZE
	#define UHTBL_MINIMUMSIZE 16
#endif


#include <stdint.h>

/* Internal flags and values */
#define UHTBL_FLAG_OCCUPIED 0x01
#define UHTBL_FLAG_STRANGER 0x02
#define UHTBL_FLAG_WITHNEXT 0x04
#define UHTBL_FLAG_LOCALKEY	0x08
#define UHTBL_MAXIMUMSIZE 2147483648

/* Status codes */
#define UHTBL_OK		0
#define UHTBL_EINVAL	-1
#define UHTBL_ENOMEM	-2
#define UHTBL_ENOENT	-3

/* API */
#if __GNUC__ >= 4
	#ifndef UHTBL_API
		#define UHTBL_API
	#endif
	#define UHTBL_INLINE static inline __attribute__((always_inline))
#else
	#ifndef UHTBL_API
		#define UHTBL_API
	#endif
	#define UHTBL_INLINE static inline
#endif


typedef union uhtbl_key uhtbl_key_t;
typedef struct uhtbl_head uhtbl_head_t;
typedef struct uhtbl_bucket uhtbl_bucket_t;
typedef struct uhtbl_config uhtbl_config_t;
typedef struct uhtbl uhtbl_t;
typedef uint32_t uhtbl_size_t;

typedef uhtbl_size_t(uhtbl_hash_t)(const void*, int len);
typedef void(uhtbl_gc_t)(void *bucket);

union uhtbl_key {
	void *ptr;
	long handle;
};

struct uhtbl_head {
	uint8_t user;
	uint8_t flags;
	uint16_t keysize;
	uhtbl_size_t next;
	uhtbl_key_t key;
};

struct uhtbl_bucket {
	uhtbl_head_t head;
};

struct uhtbl {
	uint32_t bucketsize;
	uhtbl_size_t size;
	uhtbl_size_t used;
	uhtbl_size_t payload;
	uhtbl_size_t nextfree;
	uhtbl_hash_t *fct_hash;
	uhtbl_gc_t *fct_gc;
	void *buckets;
};

/**
 * uhtbl_init() - Initialize a hash table.
 * @tbl:		hash table
 * @bucketsize:	size of a bucket
 * @sizehint:	estimated maximum of needed buckets (optional)
 * @fct_hash:	hash function
 * @fct_gc:		bucket garbage collector (optional)
 *
 * Initializes a new hash table and preallocates memory.
 *
 * bucketsize is the size in Bytes each bucket will use but note the following:
 * Each bucket needs to begin with a struct uhtbl_head_t that keeps its metadata
 * in addition to the payload you want it to carry. You are advised to define a
 * bucket struct with the first element being a uhtbl_head_t followed by your
 * desired payload and pass the size of this struct to bucketsize.
 *
 * sizehint is a hint on how many distinct entries will be stored in the hash
 * table. This will be used to preallocate space for the buckets and is useful
 * if you know how many entries will be stored in the hash table as it avoids
 * expensive rehashing cycles. sizehint should be a power of 2.
 *
 * fct_hash is the hash function used. It takes a constant void pointer and a
 * integer as size parameter and returns an unsigned (32bit) int.
 *
 * fct_gc is the garbage collector for buckets. Every time a bucket gets unset
 * or the hash table gets cleared or finalized the garbage collector function
 * taking a pointer to a bucket will take care of doing any finalization for
 * the buckets' payload and key data. You may use uhtbl_key() to get a reference
 * to your key pointer or handle for deallocation or cleaning up any other
 * references. There is an optionally selectable garbage collector that will
 * take care of free()ing key pointers if your keys point to memory areas.
 * You have to pass uhtbl_gc_key as fct_gc parameter to use it. You may also
 * call this function in your custom garbage collector.
 *
 * WARNING: Your garbage collector function must otherwise not change the
 * metadata in the uhtbl_head_t structure of the bucket else behaviour will be
 * undefined for all subsequent actions.
 *
 *
 * Example:
 * struct mybucket {
 *   uhtbl_head_t head;
 *   int mypayload1;
 *   int mypayload2;
 * }
 *
 * uhtbl_t table;
 * uhtbl_init(&table, sizeof(struct mybucket), 32, MurmurHash2, NULL);
 *
 * Returns 0 on success or a negative error code.
 */
UHTBL_API int uhtbl_init(uhtbl_t *tbl, uint32_t bucketsize,
uhtbl_size_t sizehint, uhtbl_hash_t *fct_hash, uhtbl_gc_t *fct_gc);

/**
 * uhtbl_get() - Get a bucket by its key.
 * @tbl:		hash table
 * @key:		key
 * @len:		length of key
 *
 * Finds and returns the bucket with a given key.
 *
 * Key can either be:
 *  1. A pointer to a memory area then len is its length (must be < 64KB)
 *  2. A NULL-pointer then len is a locally stored numerical key
 *
 *
 * Example:
 * struct mybucket *bucket;
 * bucket = uhtbl_get(table, "foo", sizeof("foo"));
 * printf("%i", bucket->mypayload1);
 *
 * bucket = uhtbl_get(table, NULL, 42);
 * printf("%i", bucket->mypayload1);
 *
 * Returns the bucket or NULL if no bucket with given key was found.
 */
UHTBL_API void* uhtbl_get(uhtbl_t *tbl, const void *key, long len);

/**
 * uhtbl_set() - Sets a bucket for given key.
 * @tbl:		hash table
 * @key:		key
 * @len:		length of key
 *
 * Sets a new bucket for the given key and returns a pointer to the bucket for
 * you to assign your payload data. If there is already a bucket with that key
 * it will be unset first.
 *
 * Key can either be:
 *  1. A pointer to a memory area then len is its length (must be < 64KB)
 *  2. A NULL-pointer then len is a locally stored numerical key
 *
 * NOTE: If key is a pointer memory management of it will be your business.
 * You might want to use a garbage collection function (see uhtbl_init())
 *
 * NOTE: The payload area of your bucket is NOT initialized to zeroes.
 *
 * WARNING: Note the following side effects when setting previously unset keys:
 *   1. A set may trigger several moving actions changing the order of buckets.
 *   2. A set may trigger a rehashing cycle if all buckets are occupied.
 * Therefore accessing any previously acquired pointers to any bucket results in
 * undefined behaviour. In addition iterations which have started before may
 * result in unwanted behaviour (e.g. buckets may be skipped or visited twice).
 *
 *
 * Example:
 * struct mybucket *bucket;
 * bucket = uhtbl_set(table, "foo", sizeof("foo"));
 * bucket->mypayload1 = 42:
 *
 * bucket = uhtbl_set(table, NULL, 42);
 * bucket->mypayload1 = 1337;
 *
 *
 * Returns the bucket or NULL if no bucket could be allocated (out of memory).
 */
UHTBL_API void* uhtbl_set(uhtbl_t *tbl, void *key, long len);

/**
 * uhtbl_next() - Iterates over all entries of the hash table.
 * @tbl:		hash table
 * @iter:		Iteration counter
 *
 * Iterates over all entries of the hash table.
 *
 * iter is a pointer to a numeric variable that should be set to zero before
 * the first call and will save the iteration state.
 *
 * NOTE: You may safely do several iterations in parallel. You may also safely
 * unset any buckets of the hashtable or set keys that are currently in the
 * hash table. However setting buckets with keys that don't have an assigned
 * bucket yet results in undefined behaviour.
 *
 * Example:
 * uint32_t iter = 0;
 * struct mybucket *bucket;
 * while ((bucket = uhtbl_next(table, &iter))) {
 *   printf("%i", bucket->mypayload1);
 * }
 *
 * Return the next bucket or NULL if all buckets were already visited.
 */
UHTBL_API void* uhtbl_next(uhtbl_t *tbl, uhtbl_size_t *iter);

/**
 * uhtbl_unset() - Unsets the bucket with given key.
 * @tbl:		hash table
 * @key:		key
 * @len:		length of key	(optional)
 *
 * Unsets the bucket with given key and calls the garbage collector to free
 * any payload resources - if any.
 *
 * Key can either be:
 *  1. A pointer to a memory area then len is its length (must be < 64KB)
 *  2. A NULL-pointer then len is a locally stored numerical key
 *
 * Example:
 * uhtbl_unset(table, NULL, 42);
 *
 * Returns 0 on success or a negative error code if there was no matching bucket
 */
UHTBL_API int uhtbl_unset(uhtbl_t *tbl, const void *key, long len);

/**
 * uhtbl_remove() - Unsets a bucket.
 * @tbl:		hash table
 * @head:		bucket head
 *
 * Unsets the bucket with given address and calls the garbage collector to free
 * any payload resources - if any.
 *
 * Example:
 * uhtbl_remove(table, &bucket->head);
 *
 * Returns 0 on success or a negative error code if the bucket was not found
 */
UHTBL_API int uhtbl_remove(uhtbl_t *tbl, uhtbl_head_t *head);

/**
 * uhtbl_clear() - Clears the hashtable without freeing its memory.
 * @tbl:		hash table
 *
 * Clears all buckets of the hashtable invoking the garbage collector - if any
 * but does not free the memory of the hash table. This is usually more
 * efficient than iterating and using unset.
 *
 * Returns nothing.
 */
UHTBL_API void uhtbl_clear(uhtbl_t *tbl);

/**
 * uhtbl_resize() - Resizes and rehashes the hash table.
 * @tbl:		hash table
 * @payload:	Buckets to reserve.
 *
 * Resizes the hash table and rehashes its entries.
 *
 * payload is the number of buckets the hashtable should allocate. It must be
 * greater or at least equal to the number of buckets currently occupied.
 *
 * NOTE: Rehashing is an expensive process which should be avoided if possible.
 * However resizing will be automatically done if you try to set a new bucket
 * but all buckets are already occupied. In this case the payload bucket count
 * is usually doubled. There is currently no automatic resizing done when the
 * bucket usage count decreases. You have to take care of this by yourself.
 *
 * Returns 0 on success or a negative error code if out of memory.
 */
UHTBL_API int uhtbl_resize(uhtbl_t *tbl, uhtbl_size_t payload);

/**
 * uhtbl_clear() - Clears the hashtable and frees the bucket memory.
 * @tbl:		hash table
 *
 * Clears all buckets of the hashtable invoking the garbage collector - if any
 * and frees the memory occupied by the buckets.
 *
 * Returns nothing.
 */
UHTBL_API void uhtbl_finalize(uhtbl_t *tbl);

/**
 * uhtbl_key() - Returns the key parameters as passed to set.
 * @head:		Bucket head
 * @key:		Pointer where key pointer should be stored (optional)
 * @len:		Pointer where key len should be stored (optional)
 *
 * This function might be useful to obtain the key parameters of a bucket
 * when doing garbage collection.
 *
 * Returns nothing.
 */
UHTBL_API void uhtbl_key(uhtbl_head_t *head, void **key, long *len);

/**
 * uhtbl_gc_key() - Basic garbage collector that frees key memory.
 * @bucket:		Bucket
 *
 * This function is a basic garbage collector that will free any key pointers.
 * However it will not touch your payload data.
 *
 * WARNING: You must not call this function directly on any bucket otherwise
 * behaviour will be unspecified. Instead you may pass this function to the
 * uhtbl_init function. You may also call this function from inside a custom
 * garbage collector.
 *
 * Returns nothing.
 */
UHTBL_API void uhtbl_gc_key(void *bucket);

#endif /* UHTBL_H_ */
