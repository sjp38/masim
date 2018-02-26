#ifndef _MIW_H
#define _MIW_H

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

#endif /* _MIW_H */
