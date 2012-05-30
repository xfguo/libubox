# functions for parsing and generating json

jshn_append() {
	local var="$1"
	local value="$2"
	local sep="${3:- }"

	eval "export -- \"$var=\${$var:+\${$var}\${value:+\$sep}}\$value\""
}

json_init() {
	[ -n "$JSON_UNSET" ] && eval "unset $JSON_UNSET"
	export -- JSON_SEQ=0 JSON_STACK= JSON_CUR="JSON_VAR" JSON_UNSET="" KEYS_JSON_VAR= TYPE_JSON_VAR=
}

json_add_generic() {
	local type="$1"
	local var="$2"
	local val="$3"
	local cur="${4:-$JSON_CUR}"

	if [ "${cur%%[0-9]*}" = "JSON_ARRAY" ]; then
		eval "local aseq=\"\${SEQ_$cur}\""
		var=$(( ${aseq:-0} + 1 ))
		export -- "SEQ_$cur=$var"
	else
		local name="${var//[^a-zA-Z0-9_]/_}"
		[[ "$name" == "$var" ]] || export -- "NAME_${cur}_${name}=$var"
		var="$name"
	fi

	export -- "${cur}_$var=$val"
	export -- "TYPE_${cur}_$var=$type"
	jshn_append JSON_UNSET "${cur}_$var TYPE_${cur}_$var"
	jshn_append "KEYS_${cur}" "$var"
}

json_add_table() {
	local TYPE="$1"
	JSON_SEQ=$(($JSON_SEQ + 1))
	jshn_append JSON_STACK "$JSON_CUR"
	local table="JSON_$TYPE$JSON_SEQ"
	export -- "UP_$table=$JSON_CUR"
	export -- "KEYS_$table="
	jshn_append JSON_UNSET "KEYS_$table UP_$table"
	[ "$TYPE" = "ARRAY" ] && jshn_append JSON_UNSET "SEQ_$table"
	JSON_CUR="$table"
}

json_add_object() {
	local cur="$JSON_CUR"
	json_add_table TABLE
	json_add_generic object "$1" "$JSON_CUR" "$cur"
}

json_close_object() {
	local oldstack="$JSON_STACK"
	JSON_CUR="${JSON_STACK##* }"
	JSON_STACK="${JSON_STACK% *}"
	[[ "$oldstack" == "$JSON_STACK" ]] && JSON_STACK=
}

json_add_array() {
	local cur="$JSON_CUR"
	json_add_table ARRAY
	json_add_generic array "$1" "$JSON_CUR" "$cur"
}

json_close_array() {
	json_close_object
}

json_add_string() {
	json_add_generic string "$1" "$2"
}

json_add_int() {
	json_add_generic int "$1" "$2"
}

json_add_boolean() {
	json_add_generic boolean "$1" "$2"
}

# functions read access to json variables

json_load() {
	eval `jshn -r "$1"`
}

json_dump() {
	jshn "$@" -w
}

json_get_type() {
	local dest="$1"
	local var="TYPE_${JSON_CUR}_$2"
	eval "export -- \"$dest=\${$var}\"; [ -n \"\${$var+x}\" ]"
}

json_get_var() {
	local dest="$1"
	local var="${JSON_CUR}_${2//[^a-zA-Z0-9_]/_}"
	eval "export -- \"$dest=\${$var}\"; [ -n \"\${$var+x}\" ]"
}

json_get_vars() {
	while [ "$#" -gt 0 ]; do
		local _var="$1"; shift
		json_get_var "$_var" "$_var"
	done
}

json_select() {
	local target="$1"
	local type

	[ -z "$1" ] && {
		JSON_CUR="JSON_VAR"
		return 0
	}
	[[ "$1" == ".." ]] && {
		eval "JSON_CUR=\"\${UP_$JSON_CUR}\""
		return 0
	}
	json_get_type type "$target"
	case "$type" in
		object|array)
			json_get_var JSON_CUR "$target"
		;;
		*)
			echo "WARNING: Variable '$target' does not exist or is not an array/object"
			return 1
		;;
	esac
}
