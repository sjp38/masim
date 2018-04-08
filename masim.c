#include <argp.h>
#include <err.h>
#include <fcntl.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "misc.h"
#include "masim.h"


#define LEN_ARRAY(x) (sizeof(x) / sizeof(*x))

int quiet;

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

void pr_phase(struct phase *phase)
{
	struct access *pattern;
	int j;

	printf("Phase (%s) for %u ms\n", phase->name, phase->time_ms);
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

void pr_phases(struct phase *phases, int nr_phases)
{
	int i;

	for (i = 0; i < nr_phases; i++)
		pr_phase(&phases[i]);
}

struct access_pattern {
	struct mregion *regions;
	ssize_t nr_regions;
	struct phase *phases;
	ssize_t nr_phases;
};

#define RAND_BATCH	1000
#define RAND_ARR_SZ	1000
int rndints[RAND_BATCH][RAND_ARR_SZ];

static void init_rndints(void)
{
	int i, j;

	for (i = 0; i < RAND_BATCH; i++)
		for (j = 0; j < RAND_ARR_SZ; j++)
			rndints[i][j] = rand();
	rndints[0][0] = 1;
}

static unsigned long long __do_access(struct access *access)
{
	unsigned long long nr_access = 0;
	struct mregion *region;
	char *rr;
	size_t offset;
	unsigned rndarr = 0;
	unsigned rndofs = RAND_ARR_SZ;
	int i;

	region = access->mregion;
	rr = region->region;
	if (access->random_access && rndints[0][0] == 0)
		init_rndints();

	for (i = 0; i < access->repeats; i++) {
		for (offset = 0; offset < region->sz;
				offset += access->stride) {
			if (access->random_access) {
				if (rndofs == RAND_ARR_SZ) {
					rndarr = rand() % RAND_BATCH;
					rndofs = 0;
				}
				ACCESS_ONCE(rr[rndints[rndarr][ rndofs++] %
						region->sz]) = 1;
			} else {
				ACCESS_ONCE(rr[offset]) = 1;
			}
		}
		nr_access += region->sz / access->stride;
	}

	return nr_access;
}

void exec_pattern(struct phase *phase)
{
	unsigned long long nr_access;
	clock_t start;
	size_t j;

	start = clock();
	nr_access = 0;

	j = 0;
	while (1) {
		nr_access += __do_access(&phase->patterns[
				j++ % phase->nr_patterns]);
		if (clock() - start > CLOCKS_PER_SEC / 1000 * phase->time_ms)
			break;
	}
	if (!quiet)
		printf("%s:\t%'20llu accesses/msec, %ld msecs run\n",
				phase->name,
				nr_access /
				((clock() - start) /
				 (CLOCKS_PER_SEC / 1000)),
				((clock() - start) / (CLOCKS_PER_SEC / 1000)));
}

void exec_patterns(struct access_pattern *pattern)
{
	struct mregion *region;
	size_t i;

	for (i = 0; i < pattern->nr_regions; i++) {
		region = &pattern->regions[i];
		region->region = (char *)malloc(region->sz);
	}

	for (i = 0; i < pattern->nr_phases; i++)
		exec_pattern(&pattern->phases[i]);

	for (i = 0; i < pattern->nr_regions; i++) {
		region = &pattern->regions[i];
		free(region->region);
	}
}

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
	char **fields;
	int nr_fields;

	nr_regions = astr_split(str, '\n', &lines);
	if (nr_regions < 1)
		err(1, "Not enough lines");
	regions = (struct mregion *)malloc(sizeof(struct mregion) * nr_regions);

	for (i = 0; i < nr_regions; i++) {
		r = &regions[i];
		nr_fields = astr_split(lines[i], ',', &fields);
		if (nr_fields != 2)
			err(1, "Wrong format config file: %s", lines[i]);
		strcpy(r->name, fields[0]);
		r->sz = atoll(fields[1]);
		astr_free_str_array(fields, nr_fields);
	}

	astr_free_str_array(lines, nr_regions);
	*regions_ptr = regions;

	return nr_regions;
}

/**
 * parse_phase - Parse a phase from string lines
 *
 * Return number of lines for this phase
 */
int parse_phase(char *lines[], int nr_lines, struct phase *p,
		size_t nr_regions, struct mregion *regions)
{
	struct access *patterns;
	char **fields;
	int nr_fields;
	struct access *a;
	int j, k;

	if (nr_lines < 3)
		errx(1, "%s: Wrong number of lines! %d\n", __func__, nr_lines);

	p->name = (char *)malloc((strlen(lines[0]) + 1) * sizeof(char));
	strcpy(p->name, lines[0]);
	p->time_ms = atoi(lines[1]);
	p->nr_patterns = nr_lines - 2;
	lines += 2;
	patterns = (struct access *)malloc(p->nr_patterns *
			sizeof(struct access));
	p->patterns = patterns;
	for (j = 0; j < p->nr_patterns; j++) {
		nr_fields = astr_split(lines[0], ',', &fields);
		if (nr_fields != 4)
			err(1, "Wrong number of fields! %s\n",
					lines[0]);
		a = &patterns[j];
		a->mregion = NULL;
		for (k = 0; k < nr_regions; k++) {
			if (strcmp(fields[0], regions[k].name) == 0) {
				a->mregion = &regions[k];
			}
		}
		if (a->mregion == NULL)
			err(1, "Cannot find region with name %s",
					fields[0]);
		a->random_access = atoi(fields[1]);
		a->stride = atoi(fields[2]);
		a->repeats = atoi(fields[3]);
		lines++;
		astr_free_str_array(fields, nr_fields);
	}
	return 2 + p->nr_patterns;
}

size_t parse_phases(char *str, struct phase **phases_ptr,
		size_t nr_regions, struct mregion *regions)
{
	struct phase *phases;
	struct phase *p;
	size_t nr_phases;
	char **lines_orig;
	char **lines;
	int nr_lines;
	int nr_lines_paragraph;
	int i, j;

	nr_phases = 0;
	nr_lines = astr_split(str, '\n', &lines_orig);
	lines = lines_orig;
	if (nr_lines < 4)	/* phase name, time, nr patterns, pattern */
		err(1, "Not enough lines for phases %s", str);

	for (i = 0; i < nr_lines; i++) {
		if (lines_orig[i][0] == '\0')
			nr_phases++;
	}

	phases = (struct phase *)malloc(nr_phases * sizeof(struct phase));

	for (i = 0; i < nr_phases; i++) {
		nr_lines_paragraph = 0;
		for (j = 0; j < nr_lines; j++) {
			if (lines[j][0] == '\0')
				break;
			nr_lines_paragraph++;
		}

		p = &phases[i];
		lines += parse_phase(&lines[0], nr_lines_paragraph,
				p, nr_regions, regions) + 1;
	}
	astr_free_str_array(lines_orig, nr_lines);

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
	close(f);

	content = rm_comments(cfgstr, sb.st_size);
	free(cfgstr);

	len_paragraph = paragraph_len(content, strlen(content));
	if (len_paragraph == -1)
		err(1, "Wrong file format");
	content[len_paragraph] = '\0';
	nr_regions = parse_regions(content, &mregions);

	content += len_paragraph + 2;	/* plus 2 for '\n\n' */
	nr_phases = parse_phases(content, &phases, nr_regions, mregions);

	pattern_ptr->regions = mregions;
	pattern_ptr->nr_regions = nr_regions;
	pattern_ptr->phases = phases;
	pattern_ptr->nr_phases = nr_phases;
}

static struct argp_option options[] = {
	{
		.name = "pr_config",
		.key = 'p',
		.arg = 0,
		.flags = 0,
		.doc = "flag for configuration content print",
		.group = 0,
	},
	{
		.name = "dry_run",
		.key = 'd',
		.arg = 0,
		.flags = 0,
		.doc = "flag for dry run; If this flag is set, "
			"the program ends without real access",
		.group = 0,
	},
	{
		.name = "quiet",
		.key = 'q',
		.arg = 0,
		.flags = 0,
		.doc = "suppress all normal output",
		.group = 0,
	},
	{
		.name = "silent",
		.key = 'q',
		.arg = 0,
		.flags = OPTION_ALIAS,
		.doc = "suppress all normal output",
		.group = 0,
	},

	{}
};

char *config_file = "config";
int do_print_config;
int dryrun;

error_t parse_option(int key, char *arg, struct argp_state *state)
{
	switch(key) {
	case ARGP_KEY_ARG:
		if (state->arg_num > 0)
			argp_usage(state);
		config_file = (char *)malloc((strlen(arg) + 1 ) * sizeof(char));
		strcpy(config_file, arg);
		break;
	case 'p':
		do_print_config = 1;
		break;
	case 'd':
		dryrun = 1;
		break;
	case 'q':
		quiet = 1;
		break;
	default:
		return ARGP_ERR_UNKNOWN;
	}

	return 0;
}

int main(int argc, char *argv[])
{

	struct access_pattern apattern;
	struct argp argp = {
		.options = options,
		.parser = parse_option,
		.args_doc = "[config file]",
		.doc = "Simulate given memory access pattern\v"
			"\'config file\' argument is optional."
			"  It defaults to \'config\'",
	};
	argp_parse(&argp, argc, argv, ARGP_IN_ORDER, NULL, NULL);
	setlocale(LC_NUMERIC, "");

	read_config(config_file, &apattern);
	if (do_print_config && !quiet) {
		pr_regions(apattern.regions, apattern.nr_regions);
		pr_phases(apattern.phases, apattern.nr_phases);
	}

	if (dryrun)
		return 0;

	exec_patterns(&apattern);

	return 0;
}
