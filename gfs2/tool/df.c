/******************************************************************************
*******************************************************************************
**
**  Copyright (C) Sistina Software, Inc.  1997-2003  All rights reserved.
**  Copyright (C) 2004 Red Hat, Inc.  All rights reserved.
**
**  This copyrighted material is made available to anyone wishing to use,
**  modify, copy, or redistribute it subject to the terms and conditions
**  of the GNU General Public License v.2.
**
*******************************************************************************
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <limits.h>
#include <errno.h>

#define __user
#include <linux/gfs2_ondisk.h>

#include "gfs2_tool.h"
#include "libgfs2.h"

#define SIZE (65536)

#if GFS2_TOOL_FEATURE_IMPLEMENTED
/**
 * do_df_one - print out information about one filesystem
 * @path: the path to the filesystem
 *
 */

static void
do_df_one(char *path)
{
	struct gfs2_ioctl gi;
	/* char stat_gfs2[SIZE]; */
	/* unsigned int percentage; */
	struct gfs2_sb sb;
	struct gfs2_dinode ji, ri;
	unsigned int journals = 0;
	uint64_t rgrps;
	unsigned int flags;
	char *fs, *value;
 	int error;
	struct gfs2_sbd sbd;

	sbd.path_name = path;
	check_for_gfs2(&sbd);

	sbd.device_fd = open(path, O_RDONLY);
	if (sbd.device_fd < 0)
		die("can't open %s: %s\n", path, strerror(errno));

	fs = mp2fsname(path);

	/*
	strncpy(stat_gfs2, __get_sysfs(fs, "statfs"), SIZE);
	stat_gfs2[SIZE - 1] = '\0';
	*/

	{
		char *argv[] = { "get_super" };

		gi.gi_argc = 1;
		gi.gi_argv = argv;
		gi.gi_data = (char *)&sb;
		gi.gi_size = sizeof(struct gfs2_sb);

		error = ioctl(sbd.device_fd, GFS2_IOCTL_SUPER, &gi);
		if (error != gi.gi_size)
			die("error doing get_super (%d): %s\n",
			    error, strerror(errno));
	}
	{
		char *argv[] = { "get_hfile_stat",
				 "jindex" };

		gi.gi_argc = 2;
		gi.gi_argv = argv;
		gi.gi_data = (char *)&ji;
		gi.gi_size = sizeof(struct gfs2_dinode);

		error = ioctl(sbd.device_fd, GFS2_IOCTL_SUPER, &gi);
		if (error != gi.gi_size)
			die("error doing get_hfile_stat for jindex (%d): %s\n",
			    error, strerror(errno));
	}
	{
		char *argv[] = { "get_hfile_stat",
				 "rindex" };

		gi.gi_argc = 2;
		gi.gi_argv = argv;
		gi.gi_data = (char *)&ri;
		gi.gi_size = sizeof(struct gfs2_dinode);

		error = ioctl(sbd.device_fd, GFS2_IOCTL_SUPER, &gi);
		if (error != gi.gi_size)
			die("error doing get_hfile_stat for rindex (%d): %s\n",
			    error, strerror(errno));
	}

	close(sbd.device_fd);

	journals = ji.di_entries - 2;

	rgrps = ri.di_size;
	if (rgrps % sizeof(struct gfs2_rindex))
		die("bad rindex size\n");
	rgrps /= sizeof(struct gfs2_rindex);


	printf("%s:\n", path);
	printf("  SB lock proto = \"%s\"\n", sb.sb_lockproto);
	printf("  SB lock table = \"%s\"\n", sb.sb_locktable);
	printf("  SB ondisk format = %u\n", sb.sb_fs_format);
	printf("  SB multihost format = %u\n", sb.sb_multihost_format);
	printf("  Block size = %u\n", sb.sb_bsize);
	printf("  Journals = %u\n", journals);
	printf("  Resource Groups = %"PRIu64"\n", rgrps);
	printf("  Mounted lock proto = \"%s\"\n",
	       ((value = get_sysfs(fs, "args/lockproto"))[0]) ? value :
	       sb.sb_lockproto);
	printf("  Mounted lock table = \"%s\"\n",
	       ((value = get_sysfs(fs, "args/locktable"))[0]) ? value :
	       sb.sb_locktable);
	printf("  Mounted host data = \"%s\"\n",
	       get_sysfs(fs, "args/hostdata"));
	printf("  Journal number = %s\n", get_sysfs(fs, "lockstruct/jid"));
	flags = get_sysfs_uint(fs, "lockstruct/flags");
	printf("  Lock module flags = %x", flags);
	printf("\n");
	printf("  Local flocks = %s\n",
	       (get_sysfs_uint(fs, "args/localflocks")) ? "TRUE" : "FALSE");
	printf("  Local caching = %s\n",
		(get_sysfs_uint(fs, "args/localcaching")) ? "TRUE" : "FALSE");
#if 0
	printf("\n");
	printf("  %-15s%-15s%-15s%-15s%-15s\n", "Type", "Total", "Used", "Free", "use%");
	printf("  ------------------------------------------------------------------------\n");

	percentage = (name2u64(stat_gfs2, "total")) ?
		(100.0 * (name2u64(stat_gfs2, "total") - name2u64(stat_gfs2, "free")) /
		 name2u64(stat_gfs2, "total") + 0.5) : 0;
	printf("  %-15s%-15"PRIu64"%-15"PRIu64"%-15"PRIu64"%u%%\n",
	       "data",
	       name2u64(stat_gfs2, "total"),
	       name2u64(stat_gfs2, "total") - name2u64(stat_gfs2, "free"),
	       name2u64(stat_gfs2, "free"),
	       percentage);

	percentage = (name2u64(stat_gfs2, "dinodes") + name2u64(stat_gfs2, "free")) ?
		(100.0 * name2u64(stat_gfs2, "dinodes") /
		 (name2u64(stat_gfs2, "dinodes") + name2u64(stat_gfs2, "free")) + 0.5) : 0;
	printf("  %-15s%-15"PRIu64"%-15"PRIu64"%-15"PRIu64"%u%%\n",
	       "inodes",
	       name2u64(stat_gfs2, "dinodes") + name2u64(stat_gfs2, "free"),
	       name2u64(stat_gfs2, "dinodes"),
	       name2u64(stat_gfs2, "free"),
	       percentage);
#endif
}


/**
 * print_df - print out information about filesystems
 * @argc:
 * @argv:
 *
 */

void
print_df(int argc, char **argv)
{
	if (optind < argc) {
		char buf[PATH_MAX];

		if (!realpath(argv[optind], buf))
			die("can't determine real path: %s\n", strerror(errno));

		do_df_one(buf);

		return;
	}

	{
		FILE *file;
		char buf[256], device[256], path[256], type[256];
		int first = TRUE;

		file = fopen("/proc/mounts", "r");
		if (!file)
			die("can't open /proc/mounts: %s\n", strerror(errno));

		while (fgets(buf, 256, file)) {
			if (sscanf(buf, "%s %s %s", device, path, type) != 3)
				continue;
			if (strcmp(type, "gfs2") != 0)
				continue;

			if (first)
				first = FALSE;
			else
				printf("\n");

			do_df_one(path);
		}

		fclose(file);
	}
}
#endif /* #if GFS2_TOOL_FEATURE_IMPLEMENTED */
