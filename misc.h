#ifndef _MISC_H
#define _MISC_H

#include <stddef.h>
#include <time.h>
#include <unistd.h>

#define ACCESS_ONCE(x) (*(volatile typeof(x) *)&(x))
#define barrier()	__asm__ __volatile__("": : :"memory")

#if defined(__x86_64__)

#define smp_rmb()	__asm__ __volatile__("lfence" ::: "memory")
#define smp_wmb()	__asm__ __volatile__("sfence" ::: "memory")
#define smp_mb()	__asm__ __volatile__("mfence" ::: "memory")

#else

#define smp_rmb()	__sync_synchronize()
#define smp_wmb()	__sync_synchronize()
#define smp_mb()	__sync_synchronize()

#endif

#define DELAY(x)						\
	for (volatile unsigned ___i = 0; ___i < x; ___i++);

#define TEN_TIMES(x)	\
	x;		\
	x;		\
	x;		\
	x;		\
	x;		\
	x;		\
	x;		\
	x;		\
	x;		\
	x;


/* dynamic buffer */
struct dynbuf {
	void *buf;
	size_t len;
	size_t cap;
	size_t grow_step;
	size_t head;
};

void *dbuf_at(struct dynbuf *dbuf, size_t offset);
void *dbuf_buf(struct dynbuf *dbuf);
char *dbuf_str(struct dynbuf *dbuf);
char *dbuf_free_str(struct dynbuf *dbuf);
size_t dbuf_len(struct dynbuf *dbuf);
int dbuf_set_head(struct dynbuf *dbuf, size_t offset);
size_t dbuf_read(struct dynbuf *dbuf, void *buf, size_t sz);
void dbuf_append(struct dynbuf *dbuf, void *bytes, size_t sz);
void dbuf_append_strf(struct dynbuf *str, const char *format, ...);
void dbuf_destroy(struct dynbuf *dbuf);
struct dynbuf *dbuf_create(size_t capacity, size_t grow_step);


/* a clock */

/*
 * Reference [1] for assembly usage, [2] for architecture predefinition
 *
 * [1] https://www.mcs.anl.gov/~kazutomo/rdtsc.html
 * [2] https://sourceforge.net/p/predef/wiki/Architectures/
 */
#if defined(__i386__)
#define ACLK_HW_CLOCK
static __inline__ unsigned long long aclk_clock(void)
{
	unsigned long long int x;
	__asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
	return x;
}

#elif defined(__x86_64__)
#define ACLK_HW_CLOCK
static __inline__ unsigned long long aclk_clock(void)
{
	unsigned hi, lo;
	__asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
	return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}

#elif defined(__powerpc__)
#define ACLK_HW_CLOCK
static __inline__ unsigned long long aclk_clock(void)
{
	unsigned long long int result=0;
	unsigned long int upper, lower,tmp;
	__asm__ volatile(
			"0:                  \n"
			"\tmftbu   %0           \n"
			"\tmftb    %1           \n"
			"\tmftbu   %2           \n"
			"\tcmpw    %2,%0        \n"
			"\tbne     0b         \n"
			: "=r"(upper),"=r"(lower),"=r"(tmp)
			);
	result = upper;
	result = result<<32;
	result = result|lower;

	return(result);
}
#elif defined(__aarch64__)
#define ACLK_HW_CLOCK
static __inline__ unsigned long long aclk_clock(void)
{
	unsigned long long int val;
	__asm__ __volatile__("mrs %0, cntvct_el0" : "=r"(val));
	return val;
}

#else
static inline unsigned long long aclk_clock(void)
{
	return (unsigned long long)clock();
}

#endif

/*
 * aclk_freq - return clock frequency
 *
 * This function get cpu frequency in two ways.  If aclk does not support
 * hardware rdtsc instruction for the architecture this code is running, it
 * fallbacks to CLOCKS_PER_SEC.  Otherwise, it calculates the frequency by
 * measuring hardware rdtsc before and after sleep of 0.1 second.
 */
static inline unsigned long long aclk_freq(void)
{
/*
 * clock() cannot used with sleep. Refer to [1] for more information
 * [1] http://cboard.cprogramming.com/linux-programming/91589-using-clock-sleep.html
 */
#ifndef ACLK_HW_CLOCK
	return CLOCKS_PER_SEC;
#endif
	unsigned long long start;

	start = aclk_clock();
	usleep(1000 * 100);
	return (aclk_clock() - start) * 10;
}


/* astr */

/* This function is defined as macro to be used without type problem */
#define astr_pr_list(name, list, sz)			\
	printf("%s [", name);				\
	for (int i = 0; i < sz; i++) {			\
		if (i < sz - 1) {			\
			printf("%s, ", list[i]);	\
			continue;			\
		}					\
		printf("%s]\n", list[i]);		\
	}

/* This function is defined as macro to be used without type problem */
#define astr_pr_int_list(name, list, sz)			\
	printf("%s [", name);				\
	for (int i = 0; i < sz; i++) {			\
		if (i < sz - 1) {			\
			printf("%d, ", list[i]);	\
			continue;			\
		}					\
		printf("%d]\n", list[i]);		\
	}

char *astr_locof(char ch, char *str, size_t sz);
int astr_free_str_array(char *arr[], size_t nr_entries);
int astr_split(char *str, char delim, char ***ptr_arr);
int astr_to_int_array(char *str, char delim, int *ptr_arr[]);
char *astr_strf(const char *fmt, ...);


/* acop */
struct acop_option {
	char short_name;
	char *name;
	int has_arg;
	char *description;
	void *arg;
	void (*arg_handler)(struct acop_option *opt, char *arg);
};

char *acop_opts_description(void);
int acop_register_option(struct acop_option *opt);
int acop_register_options(struct acop_option *opts, int nr_ops);
int acop_register_help_opt(void);
int acop_clear_options(void);
int acop_parse_opts(int argc, char *argv[]);
void acop_handle_char_no_arg(struct acop_option *opt, char *arg);
void acop_handle_int_arg(struct acop_option *opt, char *arg);
void acop_handle_ull_arg(struct acop_option *opt, char *arg);
void acop_handle_str_arg(struct acop_option *opt, char *arg);
void acop_handle_bool_arg(struct acop_option *opt, char *arg);
void acop_handle_help(struct acop_option *opt, char *arg);


/* athr */

struct athr_arg {
	unsigned stop;
	void *thr_arg;
};

int athr_run_for(void *(*func)(void *), struct athr_arg *arg,
			unsigned runtime);


/* avgn: a value generator */

/*
 * An entry for probability distribution.
 *
 * Values in half-open range [@start_val:@end_val) have @prob appearance
 * probability.  Note that it is half-open.  The range does not include
 * @end_val value.
 */
struct avgn_prob_dist_entry {
	unsigned long long start_val;
	unsigned long long end_val;
	double prob;
};

struct avgn_prob_dist {
	struct avgn_prob_dist_entry *entries;
	int nr_entries;
};

unsigned long long avgn_make_val(struct avgn_prob_dist *dist,
				unsigned precision);

int yamemcmp(const void *s1, const void *s2, size_t n);

void *yamemcpy(void *dest, const void *src, size_t n);

#if CONFIG_YAMEMOP
#define memcmp yamemcmp
#define memcpy yamemcpy
#endif

#endif
