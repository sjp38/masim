#include <stdio.h>

#include "miw.h"
#include "config.h"


#define LEN_ARRAY(x) (sizeof(x) / sizeof(*x))

void pr_regions(struct mregion *regions, size_t nr_regions)
{
	struct mregion *region;
	int i;

	printf("memory regions\n");
	for (i = 0; i < nr_regions; i++) {
		region = &regions[i];
		printf("\t%s: %zu bytes\n", region->name, region->sz);
	}
	printf("\n");
}


void pr_phases(struct phase *phases, int nr_phases)
{
	struct phase *phase;
	struct access *pattern;
	int i, j;

	for (i = 0; i < nr_phases; i++) {
		phase = &phases[i];
		printf("Phase %d for %u ms\n", i, phase->time_ms);
		for (j = 0; j < phase->nr_patterns; j++) {
			pattern = &phase->patterns[j];
			printf("\tPattern %d\n", j);
			printf("\t\t%s access region %s with stride %zu\n",
					pattern->random_access ?
					"randomly" : "sequentially",
					pattern->mregion->name,
					pattern->stride);
		}
	}
}

extern struct mregion mregions[];
extern struct phase phases[];

int main(void)
{
	pr_regions(mregions, LEN_ARRAY(mregions));
	pr_phases(phases, LEN_ARRAY(phases));

	return 0;
}
