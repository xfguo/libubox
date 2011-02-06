#include "blobmsg.h"
#include "blobmsg_json.h"

static bool blobmsg_add_object(struct blob_buf *b, json_object *obj)
{
	json_object_object_foreach(obj, key, val) {
		if (!blobmsg_add_json_element(b, key, val))
			return false;
	}
	return true;
}

static bool blobmsg_add_array(struct blob_buf *b, struct array_list *a)
{
	int i, len;

	for (i = 0, len = array_list_length(a); i < len; i++) {
		if (!blobmsg_add_json_element(b, NULL, array_list_get_idx(a, i)))
			return false;
	}

	return true;
}

bool blobmsg_add_json_element(struct blob_buf *b, const char *name, json_object *obj)
{
	bool ret = true;
	void *c;

	if (!obj)
		return false;

	switch (json_object_get_type(obj)) {
	case json_type_object:
		c = blobmsg_open_table(b, name);
		ret = blobmsg_add_object(b, obj);
		blobmsg_close_table(b, c);
		break;
	case json_type_array:
		c = blobmsg_open_array(b, name);
		ret = blobmsg_add_array(b, json_object_get_array(obj));
		blobmsg_close_array(b, c);
		break;
	case json_type_string:
		blobmsg_add_string(b, name, json_object_get_string(obj));
		break;
	case json_type_boolean:
		blobmsg_add_u8(b, name, json_object_get_boolean(obj));
		break;
	case json_type_int:
		blobmsg_add_u32(b, name, json_object_get_int(obj));
		break;
	default:
		return false;
	}
	return ret;
}

bool blobmsg_add_json_from_string(struct blob_buf *b, const char *str)
{
	json_object *obj;
	bool ret = false;

	obj = json_tokener_parse(str);
	if (is_error(obj))
		return false;

	if (json_object_get_type(obj) != json_type_object)
		goto out;

	ret = blobmsg_add_object(b, obj);

out:
	json_object_put(obj);
	return ret;
}

