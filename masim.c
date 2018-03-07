#include <argp.h>
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "misc.h"
#include "masim.h"


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
					pattern->mregion == NULL ?
						"..." : pattern->mregion->name,
					pattern->stride);
		}
	}
}

struct access_pattern {
	struct mregion *regions;
	ssize_t nr_regions;
	struct phase *phases;
	ssize_t nr_phases;
};

struct access_pattern apattern;

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

char *rm_comments(char *orig, ssize_t origsz)
{
	char *read_cursor;
	char *write_cursor;
	size_t len;
	size_t offset;
	char *result;

	result = (char *)malloc(origsz);
	read_cursor = orig;
	write_cursor = result;
	offset = 0;
	while (1) {
		read_cursor = orig + offset;
		len = len_line(read_cursor, origsz - offset);
		if (len == -1)
			break;
		if (read_cursor[0] == '#')
			goto nextline;
		strncpy(write_cursor, read_cursor, len + 1);
		write_cursor += len + 1;
nextline:
		offset += len + 1;
	}
	*write_cursor = '\0';

	return result;
}

ssize_t paragraph_len(char *str, ssize_t len)
{
	ssize_t i;

	for (i = 0; i < len - 1; i++) {
		if (str[i] == '\n' && str[i + 1] == '\n')
			return i;
	}
	return -1;
}

size_t parse_regions(char *str, struct mregion **regions_ptr)
{
	int i;
	struct mregion *regions;
	struct mregion *r;
	size_t nr_regions;
	char **lines;
	int nr_lines;
	char **fields;
	int nr_fields;

	nr_lines = astr_split(str, '\n', &lines);
	if (nr_lines < 2)
		err(1, "Not enough lines");
	nr_regions = atoi(lines[0]);
	regions = (struct mregion *)malloc(sizeof(struct mregion) * nr_regions);

	for (i = 0; i < nr_regions; i++) {
		r = &regions[i];
		nr_fields = astr_split(lines[i + 1], ',', &fields);
		if (nr_fields != 2)
			err(1, "Wrong format config file: %s", lines[i]);
		strcpy(r->name, fields[0]);
		r->sz = atoi(fields[1]);
		astr_free_str_array(fields, nr_fields);
	}

	astr_free_str_array(lines, nr_lines);
	*regions_ptr = regions;

	return nr_regions;
}

size_t parse_phases(char *str, struct phase **phases_ptr)
{
	struct phase *phases;
	struct phase *p;
	struct access *patterns;
	struct access *a;
	size_t nr_phases;
	char **lines;
	int nr_lines;
	char **fields;
	int nr_fields;
	int i, j;

	nr_lines = astr_split(str, '\n', &lines);
	if (nr_lines < 4)
		err(1, "Not enough lines for phases %s", str);
	nr_phases = atoi(lines[0]);

	phases = (struct phase *)malloc(nr_phases * sizeof(struct phase));

	lines++;
	for (i = 0; i < nr_phases; i++) {
		p = &phases[i];
		p->time_ms = atoi(lines[0]);
		p->nr_patterns = atoi(lines[1]);
		patterns = (struct access *)malloc(p->nr_patterns *
				sizeof(struct access));
		p->patterns = patterns;
		lines += 2;
		for (j = 0; j < p->nr_patterns; j++) {
			nr_fields = astr_split(lines[0], ',', &fields);
			if (nr_fields != 3)
				err(1, "Wrong number of fields! %s\n",
						lines[0]);
			a = &patterns[j];
			/* TODO: Find regions by name */
			a->mregion = NULL;
			a->random_access = atoi(fields[1]);
			a->stride = atoi(fields[2]);
			lines++;
		}
	}

	*phases_ptr = phases;
	return nr_phases;
}

void read_config(char *cfgpath, struct access_pattern *pattern_ptr)
{
	struct stat sb;
	char *cfgstr;
	int f;
	char *content;
	int len_paragraph;
	size_t nr_regions;
	struct mregion *mregions;

	size_t nr_phases;
	struct phase *phases;

	f = open(cfgpath, O_RDONLY);
	if (f == -1)
		err(1, "open(\"%s\") failed", cfgpath);
	if (fstat(f, &sb))
		err(1, "fstat() for config file (%s) failed", cfgpath);
	cfgstr = (char *)malloc(sb.st_size * sizeof(char));
	readall(f, cfgstr, sb.st_size);

	content = rm_comments(cfgstr, sb.st_size);
	free(cfgstr);

	len_paragraph = paragraph_len(content, strlen(content));
	if (len_paragraph == -1)
		err(1, "Wrong file format");
	content[len_paragraph] = '\0';
	nr_regions = parse_regions(content, &mregions);

	content += len_paragraph + 2;	/* plus 2 for '\n\n' */
	nr_phases = parse_phases(content, &phases);

	close(f);

	pattern_ptr->regions = mregions;
	pattern_ptr->nr_regions = nr_regions;
	pattern_ptr->phases = phases;
	pattern_ptr->nr_phases = nr_phases;
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

	read_config(config_file, &apattern);
	if (do_print_config) {
		pr_regions(apattern.regions, apattern.nr_regions);
		pr_phases(apattern.phases, apattern.nr_phases);
	}

	return 0;
}
