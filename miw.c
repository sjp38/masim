#include <stdio.h>

struct mregion {
	char name[256];
	size_t sz;
};

struct access {
	struct mregion *mregion;
	int random_access;
	size_t stride;
};

struct phase {
	unsigned time_ms;
	struct access *patterns;
	int nr_patterns;
};

struct mregion mregions[] = {
	{
		.name = "a",
		.sz = 256,
	},
	{
		.name = "b",
		.sz = 64,
	},
	{
		.name = "c",
		.sz = 128,
	},
};

struct phase phases[] = {
	{
		.time_ms = 1000,
		.nr_patterns = 1,
		.patterns = (struct access[]) {
			{
				.mregion = &mregions[0],
				.random_access = 1,
				.stride = 64,
			},
		},
	},
};

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
