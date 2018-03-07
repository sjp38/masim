#ifndef _MASIM_H
#define _MASIM_H

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

#endif /* _MASIM_H */
