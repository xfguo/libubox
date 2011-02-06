/*
 * blobmsg - library for generating/parsing structured blob messages
 *
 * Copyright (C) 2010 Felix Fietkau <nbd@openwrt.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2.1
 * as published by the Free Software Foundation
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "blobmsg.h"

struct strbuf {
	int len;
	int pos;
	char *buf;
};

static bool blobmsg_puts(struct strbuf *s, const char *c, int len)
{
	if (len <= 0)
		return true;

	if (s->pos + len >= s->len) {
		s->len += 16;
		s->buf = realloc(s->buf, s->len);
		if (!s->buf)
			return false;
	}
	memcpy(s->buf + s->pos, c, len);
	s->pos += len;
	return true;
}

static void blobmsg_format_string(struct strbuf *s, const char *str)
{
	const char *p, *last = str, *end = str + strlen(str);
	char buf[8] = "\\u00";

	blobmsg_puts(s, "\"", 1);
	for (p = str; *p; p++) {
		char escape = '\0';
		int len;

		switch(*p) {
		case '\b':
			escape = 'b';
			break;
		case '\n':
			escape = 'n';
			break;
		case '\t':
			escape = 't';
			break;
		case '\r':
			escape = 'r';
			break;
		case '"':
		case '\\':
		case '/':
			escape = *p;
			break;
		default:
			if (*p < ' ')
				escape = 'u';
			break;
		}

		if (!escape)
			continue;

		if (p > last)
			blobmsg_puts(s, last, p - last);
		last = p + 1;
		buf[1] = escape;

		if (escape == 'u') {
			sprintf(buf + 4, "%02x", (unsigned char) *p);
			len = 4;
		} else {
			len = 2;
		}
		blobmsg_puts(s, buf, len);
	}

	blobmsg_puts(s, last, end - last);
	blobmsg_puts(s, "\"", 1);
}

static void blobmsg_format_json_list(struct strbuf *s, struct blob_attr *attr, int len, bool array);

static void blobmsg_format_element(struct strbuf *s, struct blob_attr *attr, bool array, bool head)
{
	char buf[32];
	void *data;
	int len;

	if (!array && blobmsg_name(attr)[0]) {
		blobmsg_format_string(s, blobmsg_name(attr));
		blobmsg_puts(s, ":", 1);
	}
	if (head) {
		data = blob_data(attr);
		len = blob_len(attr);
	} else {
		data = blobmsg_data(attr);
		len = blobmsg_data_len(attr);
	}

	switch(blob_id(attr)) {
	case BLOBMSG_TYPE_INT8:
		sprintf(buf, "%d", *(uint8_t *)data);
		break;
	case BLOBMSG_TYPE_INT16:
		sprintf(buf, "%d", *(uint16_t *)data);
		break;
	case BLOBMSG_TYPE_INT32:
		sprintf(buf, "%d", *(uint32_t *)data);
		break;
	case BLOBMSG_TYPE_INT64:
		sprintf(buf, "%lld", *(uint64_t *)data);
		break;
	case BLOBMSG_TYPE_STRING:
		blobmsg_format_string(s, data);
		return;
	case BLOBMSG_TYPE_ARRAY:
		blobmsg_format_json_list(s, data, len, true);
		return;
	case BLOBMSG_TYPE_TABLE:
		blobmsg_format_json_list(s, data, len, false);
		return;
	}
	blobmsg_puts(s, buf, strlen(buf));
}

static void blobmsg_format_json_list(struct strbuf *s, struct blob_attr *attr, int len, bool array)
{
	struct blob_attr *pos;
	bool first = true;
	int rem = len;

	blobmsg_puts(s, (array ? "[ " : "{ "), 2);
	__blob_for_each_attr(pos, attr, rem) {
		if (!first)
			blobmsg_puts(s, ", ", 2);

		blobmsg_format_element(s, pos, array, false);
		first = false;
	}
	blobmsg_puts(s, (array ? " ]" : " }"), 2);
}

char *blobmsg_format_json(struct blob_attr *attr, bool list)
{
	struct strbuf s;

	s.len = blob_len(attr);
	s.buf = malloc(s.len);
	s.pos = 0;

	if (list)
		blobmsg_format_json_list(&s, blob_data(attr), blob_len(attr), false);
	else
		blobmsg_format_element(&s, attr, false, false);

	if (!s.len)
		return NULL;

	s.buf = realloc(s.buf, s.pos + 1);
	return s.buf;
}

static const int blob_type[__BLOBMSG_TYPE_LAST] = {
	[BLOBMSG_TYPE_INT8] = BLOB_ATTR_INT8,
	[BLOBMSG_TYPE_INT16] = BLOB_ATTR_INT16,
	[BLOBMSG_TYPE_INT32] = BLOB_ATTR_INT32,
	[BLOBMSG_TYPE_INT64] = BLOB_ATTR_INT64,
	[BLOBMSG_TYPE_STRING] = BLOB_ATTR_STRING,
};

bool blobmsg_check_attr(const struct blob_attr *attr, bool name)
{
	const struct blobmsg_hdr *hdr;
	const char *data;
	int id, len;

	if (blob_len(attr) < sizeof(struct blobmsg_hdr))
		return false;

	hdr = (void *) attr->data;
	if (!hdr->namelen && name)
		return false;

	if (hdr->namelen > blob_len(attr) - sizeof(struct blobmsg_hdr))
		return false;

	if (hdr->name[hdr->namelen] != 0)
		return false;

	id = blob_id(attr);
	len = blobmsg_data_len(attr);
	data = blobmsg_data(attr);

	if (!id || id > BLOBMSG_TYPE_LAST)
		return false;

	if (!blob_type[id])
		return true;

	return blob_check_type(data, len, blob_type[id]);
}

int blobmsg_parse(const struct blobmsg_policy *policy, int policy_len,
                  struct blob_attr **tb, void *data, int len)
{
	struct blobmsg_hdr *hdr;
	struct blob_attr *attr;
	uint8_t *pslen;
	int i;

	memset(tb, 0, policy_len * sizeof(*tb));
	pslen = alloca(policy_len);
	for (i = 0; i < policy_len; i++) {
		if (!policy[i].name)
			continue;

		pslen[i] = strlen(policy[i].name);
	}

	__blob_for_each_attr(attr, data, len) {
		hdr = blob_data(attr);
		for (i = 0; i < policy_len; i++) {
			if (!policy[i].name)
				continue;

			if (policy[i].type != BLOBMSG_TYPE_UNSPEC &&
			    blob_id(attr) != policy[i].type)
				continue;

			if (hdr->namelen != pslen[i])
				continue;

			if (!blobmsg_check_attr(attr, true))
				return -1;

			if (tb[i])
				return -1;

			if (strcmp(policy[i].name, (char *) hdr->name) != 0)
				continue;

			tb[i] = attr;
		}
	}

	return 0;
}


static struct blob_attr *
blobmsg_new(struct blob_buf *buf, int type, const char *name, int payload_len, void **data)
{
	struct blob_attr *attr;
	struct blobmsg_hdr *hdr;
	int attrlen, namelen;

	if (!name)
		name = "";

	namelen = strlen(name);
	attrlen = blobmsg_hdrlen(namelen) + payload_len;
	attr = blob_new(buf, type, attrlen);
	if (!attr)
		return NULL;

	hdr = blob_data(attr);
	hdr->namelen = namelen;
	strcpy((char *) hdr->name, (const char *)name);
	*data = blobmsg_data(attr);

	return attr;
}

static inline int
attr_to_offset(struct blob_buf *buf, struct blob_attr *attr)
{
	return (char *)attr - (char *) buf->buf;
}


void *
blobmsg_open_nested(struct blob_buf *buf, const char *name, bool array)
{
	struct blob_attr *head = buf->head;
	int type = array ? BLOBMSG_TYPE_ARRAY : BLOBMSG_TYPE_TABLE;
	unsigned long offset = attr_to_offset(buf, buf->head);
	void *data;

	if (!name)
		name = "";

	head = blobmsg_new(buf, type, name, 0, &data);
	blob_set_raw_len(buf->head, blob_pad_len(buf->head) - blobmsg_hdrlen(strlen(name)));
	buf->head = head;
	return (void *)offset;
}

void *
blobmsg_alloc_string_buffer(struct blob_buf *buf, const char *name, int maxlen)
{
	struct blob_attr *attr;
	void *data_dest;

	attr = blobmsg_new(buf, BLOBMSG_TYPE_STRING, name, maxlen, &data_dest);
	if (!attr)
		return NULL;

	data_dest = blobmsg_data(attr);
	blob_set_raw_len(buf->head, blob_pad_len(buf->head) - blob_pad_len(attr));
	blob_set_raw_len(attr, blob_raw_len(attr) - maxlen);

	return data_dest;
}

void
blobmsg_add_string_buffer(struct blob_buf *buf)
{
	struct blob_attr *attr;
	int len, attrlen;

	attr = blob_next(buf->head);
	len = strlen(blobmsg_data(attr)) + 1;

	attrlen = blob_raw_len(attr) + len;
	blob_set_raw_len(attr, attrlen);
	blob_set_raw_len(buf->head, blob_raw_len(buf->head) + blob_pad_len(attr));
}

int
blobmsg_add_field(struct blob_buf *buf, int type, const char *name,
                  const void *data, int len)
{
	struct blob_attr *attr;
	void *data_dest;

	if (type == BLOBMSG_TYPE_ARRAY ||
	    type == BLOBMSG_TYPE_TABLE)
		return -1;

	attr = blobmsg_new(buf, type, name, len, &data_dest);
	if (!attr)
		return -1;

	if (len > 0)
		memcpy(data_dest, data, len);

	return 0;
}
