#ifndef __BLOBMSG_JSON_H
#define __BLOBMSG_JSON_H

#include <json/json.h>
#include <stdbool.h>

struct blob_buf;

bool blobmsg_add_json_element(struct blob_buf *b, const char *name, json_object *obj);
bool blobmsg_add_json_from_string(struct blob_buf *b, const char *str);

#endif
