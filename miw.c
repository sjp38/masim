#include <stdio.h>

#include "miw.h"
#include "config.h"


#define LEN_ARRAY(x) (sizeof(x) / sizeof(*x))

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
			printf("\t\taccess region %s with stride %zu ",
					pattern->mregion->name, pattern->stride);
			printf("%s\n", pattern->random_access ?
					"randomly" : "sequentially");
		}
	}
}

int main(void)
{
	pr_phases(phases, LEN_ARRAY(phases));

	return 0;
}
