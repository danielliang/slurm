/*****************************************************************************\
 *  get_mach_stat.c - Get the status of the current machine 
 *
 *  NOTE: Some of these functions are system dependent. Built on RedHat2.4
 *  NOTE: While not currently used by SLURM, this code can also get a node's
 *       OS name and CPU speed. See code ifdef'ed out via USE_OS_NAME and 
 *       USE_CPU_SPEED
 *****************************************************************************
 *  Copyright (C) 2002 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by moe jette <jette1@llnl.gov>.
 *  UCRL-CODE-2002-040.
 *  
 *  This file is part of SLURM, a resource management program.
 *  For details, see <http://www.llnl.gov/linux/slurm/>.
 *  
 *  SLURM is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *  
 *  SLURM is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *  
 *  You should have received a copy of the GNU General Public License along
 *  with SLURM; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
\*****************************************************************************/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <errno.h>
#include <fcntl.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/utsname.h>
#include <sys/vfs.h>
#include <unistd.h>

#include <src/common/hostlist.h>
#include <src/common/log.h>
#include <src/common/parse_spec.h>
#include <src/slurmctld/slurmctld.h>
#include <src/slurmd/get_mach_stat.h>

#define BUF_SIZE 1024

char *get_tmp_fs_name (void);

#if DEBUG_MODULE
/* main is used here for testing purposes only */
int 
main(int argc, char * argv[]) 
{
	int error_code;
	struct config_record this_node;
	char node_name[MAX_NAME_LEN];

	error_code = get_mach_name(node_name);
	if (error_code != 0) 
		exit(1);    /* The show is all over without a node name */

	error_code += get_procs(&this_node.cpus);
	error_code += get_memory(&this_node.real_memory);
	error_code += get_tmp_disk(&this_node.tmp_disk);

	printf("NodeName=%s CPUs=%d RealMemory=%d TmpDisk=%d\n", 
		node_name, this_node.cpus, this_node.real_memory, 
		this_node.tmp_disk);
	if (error_code != 0) 
		printf("get_mach_stat error_code=%d encountered\n", error_code);
	exit (error_code);
}
#endif


/*
 * get_procs - Return the count of procs on this system 
 * Input: procs - buffer for the CPU count
 * Output: procs - filled in with CPU count, "1" if error
 *         return code - 0 if no error, otherwise errno
 */
int 
get_procs(uint32_t *procs) 
{
	int my_proc_tally;

	*procs = 1;
	my_proc_tally = (int)sysconf(_SC_NPROCESSORS_ONLN);
	if (my_proc_tally < 1) {
		error ("get_procs: error running sysconf(_SC_NPROCESSORS_ONLN)\n");
		return EINVAL;
	} 

	*procs = my_proc_tally;
	return 0;
}


#ifdef USE_OS_NAME
/*
 * get_os_name - Return the operating system name and version 
 * Input: os_name - buffer for the OS name, must be at least MAX_OS_LEN characters
 * Output: os_name - filled in with OS name, "UNKNOWN" if error
 *         return code - 0 if no error, otherwise errno
 */
int 
get_os_name(char *os_name) 
{
	int error_code;
	struct utsname sys_info;

	strcpy(os_name, "UNKNOWN");
	error_code = uname(&sys_info);
	if (error_code != 0) {
		error ("get_os_name: uname error %d\n", error_code);
		return error_code;
	} 

	if ((strlen(sys_info.sysname) + strlen(sys_info.release) + 2) >= 
		MAX_OS_LEN) {
		error ("get_os_name: OS name too long\n");
		return error_code;
	} 

	strcpy(os_name, sys_info.sysname);
	strcat(os_name, ".");
	strcat(os_name, sys_info.release);
	return 0;
}
#endif


/*
 * get_mach_name - Return the name of this node 
 * Input: node_name - buffer for the node name, must be at least MAX_NAME_LEN characters
 * Output: node_name - filled in with node name
 *         return code - 0 if no error, otherwise errno
 */
int 
get_mach_name(char *node_name) 
{
    int error_code;

    error_code = getnodename(node_name, MAX_NAME_LEN);
    if (error_code != 0)
	error ("get_mach_name: getnodename error %d\n", error_code);

    return error_code;
}


/*
 * get_memory - Return the count of procs on this system 
 * Input: real_memory - buffer for the Real Memory size
 * Output: real_memory - the Real Memory size in MB, "1" if error
 *         return code - 0 if no error, otherwise errno
 */
int
get_memory(uint32_t *real_memory)
{
	long pages;

	*real_memory = 1;
	pages = sysconf(_SC_PHYS_PAGES);
	if (pages < 1) {
		error ("get_memory: error running sysconf(_SC_PHYS_PAGES)\n");
		return EINVAL;
	} 

	*real_memory = (int)((float)pages * getpagesize() / 
			1048576.0); /* Megabytes of memory */
	return 0;
}

#ifdef USE_CPU_SPEED
/*
 * get_speed - Return the speed of procs on this system (MHz clock)
 * Input: procs - buffer for the CPU speed
 * Output: procs - filled in with CPU speed, "1.0" if error
 *         return code - 0 if no error, otherwise errno
 */
int 
get_speed(float *speed) 
{
	char buffer[128];
	FILE *cpu_info_file;
	char *buf_ptr1, *buf_ptr2;

	*speed = 1.0;
	cpu_info_file = fopen("/proc/cpuinfo", "r");
	if (cpu_info_file == NULL) {
		error ("get_speed: error %d opening /proc/cpuinfo\n", errno);
		return errno;
	} 

	while (fgets(buffer, sizeof(buffer), cpu_info_file) != NULL) {
		if ((buf_ptr1 = strstr(buffer, "cpu MHz")) != NULL)
			continue;
		buf_ptr1 += 7;
		buf_ptr2 = strstr(buf_ptr1, ":");
		if (buf_ptr2 != NULL) 
			buf_ptr1 = buf_ptr2 + 1;
		*speed = (float) strtod (buf_ptr1, (char **)NULL);
		break;
	} 

	fclose(cpu_info_file);
	return 0;
} 

#endif


/*
 * get_tmp_disk - Return the total size of /tmp file system on 
 *    this system 
 * Input: tmp_disk - buffer for the disk space size
 * Output: tmp_disk - filled in with disk space size in MB, zero if error
 *         return code - 0 if no error, otherwise errno
 */
int 
get_tmp_disk(uint32_t *tmp_disk) 
{
	struct statfs stat_buf;
	long   total_size;
	int error_code;
	float page_size;
	static char *tmp_fs_name = NULL;

	error_code = 0;
	*tmp_disk = 0;
	total_size = 0;
	page_size = (getpagesize() / 1048576.0); /* Megabytes per page */

	if (tmp_fs_name == NULL)
		tmp_fs_name = get_tmp_fs_name ();

	if (statfs(tmp_fs_name, &stat_buf) == 0) {
		total_size = (long)stat_buf.f_blocks;
	}
	else if (errno != ENOENT) {
		error_code = errno;
		error ("get_tmp_disk: error %d executing statfs on %s\n", 
			errno, tmp_fs_name);
	}

	*tmp_disk += (long)(total_size * page_size);
	return error_code;
}


/* Get temporary file system's name from TmpFS in config file */
char *
get_tmp_fs_name (void)
{
	FILE *slurm_spec_file;
	char in_line[BUF_SIZE];	/* input line */
	char *dir = NULL;
	int i, j, error_code, line_num = 0;

        slurm_spec_file = fopen (SLURM_CONFIG_FILE, "r");
	if (slurm_spec_file == NULL) {
		error ( "state_save_location error %d opening file %s: %m",
			errno, SLURM_CONFIG_FILE);
		return NULL ;
	}

	while (fgets (in_line, BUF_SIZE, slurm_spec_file) != NULL) {
		line_num++;
		if (strlen (in_line) >= (BUF_SIZE - 1)) {
			error ("state_save_location line %d, of input file %s too long\n",
				 line_num, SLURM_CONFIG_FILE);
			fclose (slurm_spec_file);
			return NULL;
		}		

		/* everything after a non-escaped "#" is a comment */
		/* replace comment flag "#" with an end of string (NULL) */
		for (i = 0; i < BUF_SIZE; i++) {
			if (in_line[i] == (char) NULL)
				break;
			if (in_line[i] != '#')
				continue;
			if ((i > 0) && (in_line[i - 1] == '\\')) {	/* escaped "#" */
				for (j = i; j < BUF_SIZE; j++) {
					in_line[j - 1] = in_line[j];
				}	
				continue;
			}	
			in_line[i] = (char) NULL;
			break;
		}		

		/* parse what is left */
		/* overall slurm configuration parameters */
		error_code = slurm_parser(in_line,
			"TmpFS=", 's', &dir, 
			"END");
		if (error_code) {
			error ("error parsing configuration file input line %d", line_num);
			fclose (slurm_spec_file);
			return NULL;
		}		

		if ( dir ) {
			fclose (slurm_spec_file);
			return dir;	
		}
	}			
	return DEFAULT_TMP_FS;
}
