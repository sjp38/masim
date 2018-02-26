#ifndef _CONFIG_H
#define _CONFIG_H

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
		.nr_patterns = 2,
		.patterns = (struct access[]) {
			{
				.mregion = &mregions[0],
				.random_access = 1,
				.stride = 64,
			},
			{
				.mregion = &mregions[1],
				.random_access = 0,
				.stride = 4096,
			},

		},
	},
};

#endif
