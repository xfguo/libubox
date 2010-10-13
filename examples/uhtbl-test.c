#include "../hash.h"
#include "../uhtbl.h"
#include <stdio.h>
#include <string.h>

struct mybucket {
	uhtbl_head_t __head;
	long double mydata;
};

int main() {
	printf("%i\n", (int)sizeof(struct mybucket));

	uhtbl_t tbl;
	uhtbl_init(&tbl, sizeof(struct mybucket), 16, hash_murmur2, NULL);
	struct mybucket *bucket;

	const char *t[] = {"null", "eins", "zwei", "drei", "vier", "fünf", "sechs",
			"sieben", "acht", "neun", "zehn", "elf", "zwölf", "dreizehn",
			"vierzehn", "fünfzehn", "sechzehn", "siebzehn", "achtzehn",
			"neunzehn", "zwanzig", "einundzwanzig", "zweiundzwanzig",
			"dreiundzwanzig", "virundzwanzig", "fünfundzwanzig", "sechsundzwanzig",
			"siebenundzwanzig", "achtundzwanzig", "neunundzwanzig", "dreißig",
			"einunddreißig", "zweiunddreißig"};
	for (int i = 0; i < 33; i++) {
		bucket = (struct mybucket*)uhtbl_set(&tbl, t[i], strlen(t[i]));
		bucket->mydata = i;
	}

	uint32_t iter = 0;
	while ((bucket = (struct mybucket*)uhtbl_next(&tbl, &iter))) {
		printf("%i\t", (int)bucket->mydata);
	}
	printf("\nSize: %i, Used: %i\n\n", tbl.size, tbl.used);

	for (int i = 0; i < 33; i++) {
		bucket = (struct mybucket*)uhtbl_set(&tbl, 0, i);
		bucket->mydata = i;
	}

	iter = 0;
	while ((bucket = (struct mybucket*)uhtbl_next(&tbl, &iter))) {
		printf("%i\t", (int)bucket->mydata);
	}
	printf("\nSize: %i, Used: %i\n\n", tbl.size, tbl.used);

	for (int i = 0; i < 33; i++) {
		if (uhtbl_unset(&tbl, 0, i)) {
			printf("Unset failed %i\n", i);
		}
		if (uhtbl_unset(&tbl, t[i], strlen(t[i]))) {
			printf("Unset failed %s\n", t[i]);
		}
	}

	iter = 0;
	while ((bucket = (struct mybucket*)uhtbl_next(&tbl, &iter))) {
		printf("%i\t", (int)bucket->mydata);
	}
	printf("\nSize: %i, Used: %i\n\n", tbl.size, tbl.used);

	for (int i = 0; i < 33; i++) {
		bucket = (struct mybucket*)uhtbl_set(&tbl, t[i], strlen(t[i]));
		bucket->mydata = i;
	}

	for (int i = 0; i < 33; i++) {
		bucket =(struct mybucket*) uhtbl_set(&tbl, 0, i);
		bucket->mydata = i;
	}

	for (int i = 0; i < 33; i++) {
		bucket = (struct mybucket*)uhtbl_set(&tbl, t[i], strlen(t[i]));
		bucket->mydata = i;
	}

	for (int i = 0; i < 33; i++) {
		bucket = (struct mybucket*)uhtbl_set(&tbl, 0, i);
		bucket->mydata = i;
	}

	iter = 0;
	while ((bucket = (struct mybucket*)uhtbl_next(&tbl, &iter))) {
		printf("%i\t", (int)bucket->mydata);
	}
	printf("\nSize: %i, Used: %i\n\n", tbl.size, tbl.used);

	for (int i = 0; i < 33; i++) {
		bucket = (struct mybucket*)uhtbl_get(&tbl, t[i], strlen(t[i]));
		printf("%i\t", (int)bucket->mydata);
		bucket = (struct mybucket*)uhtbl_get(&tbl, 0, i);
		printf("%i\t", (int)bucket->mydata);
	}
	printf("\nSize: %i, Used: %i\n\n", tbl.size, tbl.used);

	uhtbl_finalize(&tbl);

	return 0;
}
