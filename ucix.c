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

#include <string.h>
#include <stdlib.h>

#include "ucix.h"

struct uci_ptr uci_ptr;

int ucix_get_ptr(struct uci_context *ctx, const char *p, const char *s, const char *o, const char *t)
{
	memset(&uci_ptr, 0, sizeof(uci_ptr));
	uci_ptr.package = p;
	uci_ptr.section = s;
	uci_ptr.option = o;
	uci_ptr.value = t;
	return uci_lookup_ptr(ctx, &uci_ptr, NULL, true);
}

struct uci_context* ucix_init(const char *config_file, int state)
{
	struct uci_context *ctx = uci_alloc_context();
	uci_set_confdir(ctx, "/etc/config");
	if(state)
		uci_set_savedir(ctx, "/var/state/");
	else
		uci_set_savedir(ctx, "/tmp/.uci/");
	if(uci_load(ctx, config_file, NULL) != UCI_OK)
	{
		printf("%s/%s is missing or corrupt\n", ctx->confdir, config_file);
		return NULL;
	}
	return ctx;
}

struct uci_context* ucix_init_path(const char *vpath, const char *config_file, int state)
{
	struct uci_context *ctx;
	char buf[256];
	if(!vpath)
		return ucix_init(config_file, state);
	ctx = uci_alloc_context();
	buf[255] = '\0';
	snprintf(buf, 255, "%s%s", vpath, "/etc/config");
	uci_set_confdir(ctx, buf);
	snprintf(buf, 255, "%s%s", vpath, (state)?("/var/state"):("/tmp/.uci"));
	uci_add_delta_path(ctx, buf);
	if(uci_load(ctx, config_file, NULL) != UCI_OK)
	{
		printf("%s/%s is missing or corrupt\n", ctx->confdir, config_file);
		return NULL;
	}
	return ctx;
}

int ucix_get_option_list(struct uci_context *ctx, const char *p,
	const char *s, const char *o, struct list_head *l)
{
	struct uci_element *e = NULL;
	if(ucix_get_ptr(ctx, p, s, o, NULL))
		return 1;
	if (!(uci_ptr.flags & UCI_LOOKUP_COMPLETE))
		return 1;
	e = uci_ptr.last;
	switch (e->type)
	{
	case UCI_TYPE_OPTION:
		switch(uci_ptr.o->type) {
			case UCI_TYPE_LIST:
				uci_foreach_element(&uci_ptr.o->v.list, e)
				{
					struct ucilist *ul = malloc(sizeof(struct ucilist));
					ul->val = strdup((e->name)?(e->name):(""));
					list_add_tail(&ul->list, l);
				}
				break;
			default:
				break;
		}
		break;
	default:
		return 1;
	}

	return 0;
}

char* ucix_get_option(struct uci_context *ctx, const char *p, const char *s, const char *o)
{
	struct uci_element *e = NULL;
	const char *value = NULL;
	if(ucix_get_ptr(ctx, p, s, o, NULL))
		return NULL;
	if (!(uci_ptr.flags & UCI_LOOKUP_COMPLETE))
		return NULL;
	e = uci_ptr.last;
	switch (e->type)
	{
	case UCI_TYPE_SECTION:
		value = uci_to_section(e)->type;
		break;
	case UCI_TYPE_OPTION:
		switch(uci_ptr.o->type) {
			case UCI_TYPE_STRING:
				value = uci_ptr.o->v.string;
				break;
			default:
				value = NULL;
				break;
		}
		break;
	default:
		return 0;
	}

	return (value) ? (strdup(value)):(NULL);
}

void ucix_add_list(struct uci_context *ctx, const char *p, const char *s, const char *o, struct list_head *vals)
{
	struct list_head *q;
	list_for_each(q, vals)
	{
		struct ucilist *ul = container_of(q, struct ucilist, list);
		if(ucix_get_ptr(ctx, p, s, o, (ul->val)?(ul->val):("")))
			return;
		uci_add_list(ctx, &uci_ptr);
	}
}

void ucix_for_each_section_type(struct uci_context *ctx,
	const char *p, const char *t,
	void (*cb)(const char*, void*), void *priv)
{
	struct uci_element *e;
	if(ucix_get_ptr(ctx, p, NULL, NULL, NULL))
		return;
	uci_foreach_element(&uci_ptr.p->sections, e)
		if (!strcmp(t, uci_to_section(e)->type))
			cb(e->name, priv);
}

void ucix_for_each_section_option(struct uci_context *ctx,
	const char *p, const char *s,
	void (*cb)(const char*, const char*, void*), void *priv)
{
	struct uci_element *e;
	if(ucix_get_ptr(ctx, p, s, NULL, NULL))
		return;
	uci_foreach_element(&uci_ptr.s->options, e)
	{
		struct uci_option *o = uci_to_option(e);
		cb(o->e.name, o->v.string, priv);
	}
}


