#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <err.h>
#include <unistd.h>
#include <stdlib.h>

#include "masim.h"
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

size_t len_line(char *str, size_t lim_seek)
{
	size_t i;

	for (i = 0; i < lim_seek; i++) {
		if (str[i] == '\n')
			break;
	}

	if (i == lim_seek)
		return -1;

	return i;
}

void read_config(char *cfgpath)
{
	struct stat sb;
	char *cfgstr;
	char *line;
	size_t len;
	size_t offset;
	int f;
	f = open(cfgpath, O_RDONLY);
	if (f == -1)
		err(1, "open(\"%s\") failed", cfgpath);
	if (fstat(f, &sb))
		err(1, "fstat() for config file (%s) failed", cfgpath);
	cfgstr = (char *)malloc(sb.st_size * sizeof(char));
	read(f, cfgstr, sb.st_size);
	offset = 0;
	line = cfgstr;
	while (1) {
		line = cfgstr + offset;
		len = len_line(line, sb.st_size - offset);
		if (len == -1)
			break;
		line[len] = '\0';
		if (line[0] == '#')
			goto nextline;
		printf("line: %s\n", line);
nextline:
		offset += len + 1;
	}
	close(f);
}

int main(void)
{
	read_config("config");
	pr_regions(mregions, LEN_ARRAY(mregions));
	pr_phases(phases, LEN_ARRAY(phases));

	return 0;
}
