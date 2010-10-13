/*
 *   ucix
 *   Copyright (C) 2010 John Crispin <blogic@openwrt.org>
 *   Copyright (C) 2010 Steven Barth <steven@midlink.org>
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

#ifndef _UCI_H__
#define _UCI_H__
#include <uci.h>
#include "list.h"

struct ucilist {
	struct list_head list;
	char *val;
};

extern struct uci_ptr uci_ptr;

int ucix_get_ptr(struct uci_context *ctx, const char *p,
	const char *s, const char *o, const char *t);
struct uci_context* ucix_init(const char *config_file, int state);
struct uci_context* ucix_init_path(const char *vpath, const char *config_file, int state);
int ucix_save_state(struct uci_context *ctx, const char *p);
char* ucix_get_option(struct uci_context *ctx,
	const char *p, const char *s, const char *o);
int ucix_get_option_list(struct uci_context *ctx, const char *p,
	const char *s, const char *o, struct list_head *l);
void ucix_for_each_section_type(struct uci_context *ctx,
	const char *p, const char *t,
	void (*cb)(const char*, void*), void *priv);
void ucix_for_each_section_option(struct uci_context *ctx,
	const char *p, const char *s,
	void (*cb)(const char*, const char*, void*), void *priv);
void ucix_add_list(struct uci_context *ctx, const char *p,
	const char *s, const char *o, struct list_head *vals);

static inline void ucix_del(struct uci_context *ctx, const char *p, const char *s, const char *o)
{
	if (!ucix_get_ptr(ctx, p, s, o, NULL))
		uci_delete(ctx, &uci_ptr);
}

static inline void ucix_revert(struct uci_context *ctx, const char *p, const char *s, const char *o)
{
	if (!ucix_get_ptr(ctx, p, s, o, NULL))
		uci_revert(ctx, &uci_ptr);
}

static inline void ucix_add_list_single(struct uci_context *ctx, const char *p, const char *s, const char *o, const char *t)
{
	if (ucix_get_ptr(ctx, p, s, o, t))
		return;
	uci_add_list(ctx, &uci_ptr);
}

static inline void ucix_add_option(struct uci_context *ctx, const char *p, const char *s, const char *o, const char *t)
{
	if (ucix_get_ptr(ctx, p, s, o, t))
		return;
	uci_set(ctx, &uci_ptr);
}

static inline void ucix_add_section(struct uci_context *ctx, const char *p, const char *s, const char *t)
{
	if(ucix_get_ptr(ctx, p, s, NULL, t))
		return;
	uci_set(ctx, &uci_ptr);
}

static inline void ucix_add_option_int(struct uci_context *ctx, const char *p, const char *s, const char *o, int t)
{
	char tmp[64];
	snprintf(tmp, 64, "%d", t);
	ucix_add_option(ctx, p, s, o, tmp);
}

static inline void ucix_add_list_single_int(struct uci_context *ctx, const char *p, const char *s, const char *o, const int t)
{
	char tmp[64];
	snprintf(tmp, 64, "%d", t);
	ucix_add_list_single(ctx, p, s, o, tmp);
}

static inline int ucix_get_option_int(struct uci_context *ctx, const char *p, const char *s, const char *o, int def)
{
	char *tmp = ucix_get_option(ctx, p, s, o);
	int ret = def;

	if (tmp)
	{
		ret = atoi(tmp);
		free(tmp);
	}
	return ret;
}

static inline int ucix_save(struct uci_context *ctx, const char *p)
{
	if(ucix_get_ptr(ctx, p, NULL, NULL, NULL))
		return 1;
	uci_save(ctx, uci_ptr.p);
	return 0;
}

static inline int ucix_commit(struct uci_context *ctx, const char *p)
{
	if(ucix_get_ptr(ctx, p, NULL, NULL, NULL))
		return 1;
	return uci_commit(ctx, &uci_ptr.p, false);
}

static inline void ucix_cleanup(struct uci_context *ctx)
{
	uci_free_context(ctx);
}


#endif
