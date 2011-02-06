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

#ifndef __BLOBMSG_JSON_H
#define __BLOBMSG_JSON_H

#include <json/json.h>
#include <stdbool.h>

struct blob_buf;

bool blobmsg_add_json_element(struct blob_buf *b, const char *name, json_object *obj);
bool blobmsg_add_json_from_string(struct blob_buf *b, const char *str);

#endif
