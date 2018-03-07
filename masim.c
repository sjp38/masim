#include <argp.h>
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

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

/* Read exactly required bytes from a file */
void readall(int file, char *buf, ssize_t sz)
{
	ssize_t sz_read;
	ssize_t sz_total_read = 0;
	while (1) {
		sz_read = read(file, buf + sz_total_read, sz - sz_total_read);
		if (sz_read == -1)
			err(1, "read() failed!");
		sz_total_read += sz_read;
		if (sz_total_read == sz)
			return;
	}
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
	readall(f, cfgstr, sb.st_size);
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
nextline:
		offset += len + 1;
	}
	close(f);
}

static struct argp_option options[] = {
	{
		.name = "config",
		.key = 'c',
		.arg = "<config file>",
		.flags = 0,
		.doc = "path to memory access configuration file",
		.group = 0,
	},
	{
		.name = "pr_config",
		.key = 'p',
		.arg = 0,
		.flags = 0,
		.doc = "flag for configuration content print",
		.group = 0,
	},
	{}
};

char *config_file = "config";
int do_print_config;

error_t parse_option(int key, char *arg, struct argp_state *state)
{
	switch(key) {
	case 'c':
		config_file = (char *)malloc((strlen(arg) + 1 ) * sizeof(char));
		strcpy(config_file, arg);
		break;
	case 'p':
		do_print_config = 1;
		break;
	default:
		return ARGP_ERR_UNKNOWN;
	}

	return 0;
}

int main(int argc, char *argv[])
{
	struct argp argp = {
		.options = options,
		.parser = parse_option,
		.args_doc = "",
		.doc = "Simulate given memory access pattern",
	};
	argp_parse(&argp, argc, argv, ARGP_IN_ORDER, NULL, NULL);

	read_config(config_file);
	if (do_print_config) {
		pr_regions(mregions, LEN_ARRAY(mregions));
		pr_phases(phases, LEN_ARRAY(phases));
	}

	return 0;
}
