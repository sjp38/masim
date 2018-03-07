/*
 * misc - collection of misclellaneous functions
 *
 * This file is a collection of miscellaneous functions that might be reused.
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "misc.h"

void *dbuf_at(struct dynbuf *dbuf, size_t offset)
{
	return dbuf->buf + offset;
}

void *dbuf_buf(struct dynbuf *dbuf)
{
	return dbuf_at(dbuf, 0);
}

char *dbuf_str(struct dynbuf *dbuf)
{
	return (char *)dbuf_at(dbuf, 0);
}

/**
 * dbuf_free_str - Free dbuf and return its content in char[]
 *
 * This function is similar with dbuf_str() but it do free dbuf inside itself.
 * As a result, client do not need to call dbuf_destroy() manually.  However,
 * client still should free returned region.
 */
char *dbuf_free_str(struct dynbuf *dbuf)
{
	char *ret;

	ret = dbuf_str(dbuf);
	free(dbuf);

	return ret;
}

size_t dbuf_len(struct dynbuf *dbuf)
{
	return dbuf->len;
}

int dbuf_set_head(struct dynbuf *dbuf, size_t offset)
{
	if (offset < 0 || offset > dbuf_len(dbuf))
		return 1;
	dbuf->head = offset;
	return 0;
}

/**
 * dbuf_read - read specific bytes from dbuf into a buffer
 *
 * This function is very similar with read() function.  Differences are:
 * - If requested size for read is too big, it returns -1.
 * - It always read requested bytes if the size is appropriate (read() can read
 *   smaller than requested in case of interrupt or something else).
 */
size_t dbuf_read(struct dynbuf *dbuf, void *buf, size_t sz)
{
	if (dbuf->head + sz > dbuf_len(dbuf))
		return -1;

	memcpy(buf, dbuf_at(dbuf, dbuf->head), sz);
	dbuf->head += sz;

	return sz;
}

void dbuf_append(struct dynbuf *dbuf, void *bytes, size_t sz)
{
	size_t new_cap;

	while (dbuf->cap < dbuf->len + sz) {
		new_cap = dbuf->cap + dbuf->grow_step;
		dbuf->buf = realloc(dbuf->buf, new_cap);
		dbuf->cap = new_cap;
	}
	memcpy(dbuf->buf + dbuf->len, bytes, sz);
	dbuf->len += sz;
}

/**
 * dbuf_append_strf - Append formatted string to dynbuf
 */
void dbuf_append_strf(struct dynbuf *dstr, const char *fmt, ...)
{
	char *buf;
	size_t sz_buf;
	va_list arg_ptr;
	char success;
	int written;

	for (success = 0, sz_buf = 256, buf = NULL;
			success == 0; sz_buf = sz_buf * 2) {
		if (sz_buf > 1024 * 1024 * 2) {
			fprintf(stderr,
				"%s use more than 2 MiB! something wrong!\n",
				__func__);
			exit(1);
		}
		buf = (char *)realloc(buf, sizeof(char) * sz_buf);
		va_start(arg_ptr, fmt);
		written = vsnprintf(buf, sz_buf, fmt, arg_ptr);
		va_end(arg_ptr);
		if (written < sz_buf)
			success = 1;
	}

	/* remove terminating NULL */
	if (dstr->len > 0)
		dstr->len -= 1;
	dbuf_append(dstr, buf, written + 1 /* + 1 for NULL */);
	free(buf);
}

void dbuf_destroy(struct dynbuf *dbuf)
{
	free(dbuf->buf);
	free(dbuf);
}

struct dynbuf *dbuf_create(size_t capacity, size_t grow_step)
{
	struct dynbuf *dynb;

	dynb = (struct dynbuf *)malloc(sizeof(struct dynbuf));
	dynb->buf = malloc(sizeof(char) * capacity);
	dynb->len = 0;
	dynb->cap = capacity;
	dynb->grow_step = grow_step;

	return dynb;
}


/* astr: _a str_ing utilities */
#include "string.h"

static size_t astr_nrof(char ch, char *str, size_t sz)
{
	size_t count;
	int i;

	count = 0;
	for (i = 0; i < sz; i++) {
		if (str[i] == ch)
			count++;
	}

	return count;
}

char *astr_locof(char ch, char *str, size_t sz)
{
	int i;

	for (i = 0; i < sz; i++) {
		if (str[i] == ch)
			return &str[i];
	}
	return NULL;
}

/**
 * astr_free_str_array - Free an array of strings
 *
 * @arr		Array of strings to be freed.
 * @nr_entries	Number of entries inside the array.
 *
 * Returns	Zero if success, Non-zero else.
 */
int astr_free_str_array(char *arr[], size_t nr_entries)
{
	int i;

	for (i = 0; i < nr_entries; i++)
		free(arr[i]);
	free(arr);

	return 0;
}

/**
 * astr_split -  Split a string with a delimiter
 *
 * @str		String to be splitted.  It should have terminating NULL.
 * @delim	Delimiter to be used for splitting.
 * @ptr_arr	Pointer to store the start point of parsed array
 *
 * Caller should free the resulted array itself.  For the purpose,
 * astr_free_str_array() may be helpful.
 *
 * Returns number of entries in the *ptr_arr if success, -1 if not.
 */
int astr_split(char *str, char delim, char ***ptr_arr)
{
	char **narr;
	size_t sz;
	size_t sz_arr;
	char *strcopy;
	char *iter, *nextcomma;
	int i;

	sz = strlen(str);
	strcopy = (char *)malloc(sz + 1);
	strncpy(strcopy, str, sz + 1);

	sz_arr = astr_nrof(delim, strcopy, sz) + 1;
	narr = malloc(sizeof(char *) * sz_arr);

	for (iter = strcopy, i = 0; i < sz_arr; i++) {
		nextcomma = astr_locof(delim, iter, strcopy + sz - iter);
		if (nextcomma != NULL)
			*nextcomma = '\0';
		narr[i] = (char *)malloc(strlen(iter) + 1);
		strncpy(narr[i], iter, strlen(iter) + 1);
		iter = nextcomma + 1;
		if (nextcomma != NULL)
			*nextcomma = delim;
	}
	free(strcopy);

	*ptr_arr = narr;
	return sz_arr;
}

/**
 * astr_to_int_array - Transform a string into an array of integers
 *
 * @str		String to be used as input.
 * @delim	Delimiter to be used for split.
 * @ptr_arr	pointer to store the start point of parsed array
 *
 * Returns number of entries in the *ptr_arr if success, -1 if not.
 */
int astr_to_int_array(char *str, char delim, int *ptr_arr[])
{
	char **strarr;
	int nr_tokens;
	int i;

	nr_tokens = astr_split(str, delim, &strarr);
	*ptr_arr = malloc(sizeof(int) * nr_tokens);
	for (i = 0; i < nr_tokens; i++) {
		(*ptr_arr)[i] = atoi(strarr[i]);
		free(strarr[i]);
	}
	free(strarr);

	return nr_tokens;
}

/**
 * astr_strf - Create a formatted string
 */
char *astr_strf(const char *fmt, ...)
{
	char *buf;
	size_t sz_buf;
	va_list arg_ptr;
	char success;
	int written;

	for (success = 0, sz_buf = 256, buf = NULL;
			success == 0; sz_buf = sz_buf * 2) {
		if (sz_buf > 1024 * 1024 * 2) {
			fprintf(stderr,
				"%s use more than 2 MiB! something wrong!\n",
				__func__);
			exit(1);
		}
		buf = (char *)realloc(buf, sizeof(char) * sz_buf);
		va_start(arg_ptr, fmt);
		written = vsnprintf(buf, sz_buf, fmt, arg_ptr);
		va_end(arg_ptr);
		if (written < sz_buf)
			success = 1;
	}

	return buf;
}


/* acop: a commanline option parser utilities */
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>

static struct acop_option **reg_opts_buf;
static int nr_reg_opts;
static int sz_reg_opts_buf;
#define DEFAULT_OPTIONS_BUF_SIZE 10

int acop_register_option(struct acop_option *opt)
{
	struct acop_option *iop;
	size_t sz_name;
	int i;

	for (i = 0; i < nr_reg_opts; i++) {
		iop = reg_opts_buf[i];
		if (iop->short_name == opt->short_name)
			return 1;
		sz_name = strlen(iop->name) > strlen(opt->name) ?
			strlen(iop->name) : strlen(opt->name);
		if (strncmp(iop->name, opt->name, sz_name) == 0)
			return 2;
		/* arg should not duplicates except NULL */
		if (iop->arg != NULL && iop->arg == opt->arg)
			return 3;
	}

	if (nr_reg_opts == sz_reg_opts_buf) {
		reg_opts_buf = realloc(reg_opts_buf,
				(nr_reg_opts + DEFAULT_OPTIONS_BUF_SIZE) *
				sizeof(struct acop_option *));
		sz_reg_opts_buf += DEFAULT_OPTIONS_BUF_SIZE;
	}
	reg_opts_buf[nr_reg_opts++] = opt;

	return 0;
}

int acop_register_options(struct acop_option *opts, int nr_ops)
{
	int ret;
	int i;

	for (i = 0; i < nr_ops; i++) {
		ret = acop_register_option(&opts[i]);
		if (ret)
			return ret;
	}
	return 0;
}

static struct acop_option help_opt = {
	.short_name = 'h', .name = "help", .arg_handler = acop_handle_help,
	.description = "print out help message.\n"};

int acop_register_help_opt(void)
{
	return acop_register_option(&help_opt);
}

/*
 * acop_clear_options - clear registered options
 *
 * Client should free() the option objects itself because it allocated them.
 */
int acop_clear_options(void)
{
	int i;

	for (i = 0; i < nr_reg_opts; i++)
		reg_opts_buf[i] = NULL;
	nr_reg_opts = 0;

	return 0;
}

static struct option *acop_opts_to_options(struct acop_option **opts, int nr_ops)
{
	int i;
	struct acop_option *opt;
	struct option *option;
	struct option *options;

	options = (struct option *)malloc(sizeof(struct option) * nr_ops);
	for (i = 0; i < nr_ops; i++) {
		opt = opts[i];
		option = &options[i];

		option->name = opt->name;
		option->has_arg = opt->has_arg;
		option->flag = NULL;
		option->val = opt->short_name;
	}
	return options;
}

static char *acop_opts_to_optarg(struct acop_option **opts, int nr_ops)
{
	int i, j;
	struct acop_option *opt;
	char *optarg;
	int optarg_len;

	/* optarg may have one or two `:`s */
	optarg = (char *)malloc(sizeof(char) * (nr_ops * 3));
	optarg_len = 0;
	for (i = 0; i < nr_ops; i++) {
		opt = opts[i];
		optarg[optarg_len++] = opt->short_name;
		for (j = 0; j < opt->has_arg; j++)
			optarg[optarg_len++] = ':';
	}
	optarg[optarg_len] = '\0';
	return optarg;
}

char *acop_opts_description(void)
{
	struct acop_option *opt;
	char *buf = NULL;
	int sz_buf = 0;
	int len_buf_cont = 0;
	int i;
	size_t desc_len;
	static char flag[256];
	size_t flag_len;

	for (i = 0; i < nr_reg_opts; i++) {
		opt = reg_opts_buf[i];
		flag_len = strlen(opt->name) +
				strlen("\t") + strlen(" or ") + strlen(": ") +
				1 + 1;	/* for short_name and null */
		if (flag_len >= 256) {
			fprintf(stderr, "too long opt name %s\n", opt->name);
			exit(1);
		}
		snprintf(flag, flag_len, "\t%c or %s: ",
				opt->short_name, opt->name);

		desc_len = strlen(flag) + strlen(opt->description) + 1;
		if (sz_buf < len_buf_cont + desc_len) {
			buf = (char *)realloc(buf, sz_buf + desc_len * 2);
			sz_buf = sz_buf + desc_len * 2;
		}
		snprintf(buf + len_buf_cont, desc_len, "%s%s", flag, opt->description);
		len_buf_cont += desc_len;
		buf[len_buf_cont - 1] = '\n';
	}
	buf[len_buf_cont] = '\0';
	return buf;
}

void acop_handle_char_no_arg(struct acop_option *opt, char *arg)
{
	*((char *)opt->arg) = atoi(arg);
}

void acop_handle_int_arg(struct acop_option *opt, char *arg)
{
	*((int *)opt->arg) = atoi(arg);
}

void acop_handle_ull_arg(struct acop_option *opt, char *arg)
{
	*((unsigned long long *)opt->arg) = atoll(arg);
}

void acop_handle_str_arg(struct acop_option *opt, char *arg)
{
	size_t arglen;
	char *str;

	arglen = strlen(arg) + 1;
	str = (char *)malloc(arglen * sizeof(char));
	strncpy(str, arg, arglen);

	*(char **)opt->arg = str;
}

void acop_handle_bool_arg(struct acop_option *opt, char *arg)
{
	*((char *)opt->arg) = 1;
}

void acop_handle_help(struct acop_option *opt, char *arg)
{
	printf("Options of this program are as below:\n");
	printf("%s\n", acop_opts_description());
	exit(0);
}

static void acop_set_optval(int o, char *val,
		struct acop_option **opts, int nr_ops)
{
	struct acop_option *opt;
	int i;

	/*
	 * Wrong option or missing argument.  We do print out just options
	 * description and allow getopt_long() to print out the reason of
	 * problem (wrong option or missing argument) briefly.
	 */
	if (o == '?') {
		fprintf(stderr, "Options:\n%s\n", acop_opts_description());
		exit(1);
	}

	for (i = 0; i < nr_ops; i++) {
		opt = opts[i];
		if (opt->short_name != o)
			continue;
		opt->arg_handler(opt, val);
		return;
	}
}

int acop_parse_opts(int argc, char *argv[])
{
	int c;
	struct option *options = acop_opts_to_options(
			reg_opts_buf, nr_reg_opts);

	optind = 0;
	while (1)
	{
		int option_index = 0;
		c = getopt_long(argc, argv,
				acop_opts_to_optarg(reg_opts_buf, nr_reg_opts),
				options, &option_index);

		/* end of the options */
		if (c == -1)
			break;

		acop_set_optval(c, optarg, reg_opts_buf, nr_reg_opts);
	}
	return 0;
}


/* a value generator */

unsigned long long avgn_make_val(struct avgn_prob_dist *dist,
				unsigned precision)
{
	double prob, cum_prob;
	unsigned long long ret;
	struct avgn_prob_dist_entry *entry;
	int i;

	prob = (unsigned)rand() % precision / (double)precision;

	cum_prob = 0;
	for (i = 0; i < dist->nr_entries; i++) {
		entry = &(dist->entries[i]);
		cum_prob += entry->prob;
		if (prob <= cum_prob) {
			unsigned long long gap;
			gap = entry->end_val - entry->start_val;
			ret = entry->start_val + rand() % gap;
			return ret;
		}
	}
	fprintf(stderr, "[%s] No probability! Check it again\n", __func__);
	exit(1);

	return 0;
}

int yamemcmp(const void *s1, const void *s2, size_t n)
{
	size_t i;

	for (i = 0; i < n; i++) {
		if (((char *)s1)[i] != ((char *)s2)[i])
			return 1;
	}
	return 0;
}

void *yamemcpy(void *dest, const void *src, size_t n)
{
	size_t i;

	for (i = 0; i < n; i++) {
		((char *)dest)[i] = ((char *)src)[i];
	}

	return dest;
}
