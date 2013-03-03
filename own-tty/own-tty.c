/*
 * Simple program to enable running a program (such as a shell) on a tty
 * without logging in on that tty first.
 * Usage: own_tty /dev/tty6 /bin/sh -sh
 *
 * Sets the controlling tty of the process to that tty, and sets the pgrp of
 * that tty to a newly-created pgrp for that process.  I think.  The whole
 * session/pgrp thing confuses me.
 *
 * Note that you probably ought to chown the tty beforehand for best results.
 *
 * Incidentally, there's probably a better way to do all this stuff,
 * but this way seems to work, at least for Linux 2.0.30.
 *
 * $Id: own-tty.c,v 1.3 1998/03/16 05:07:14 kragen Exp $
 *
 * This program is in the public domain; I hereby relinquish all copyright
 * claim to it.
 *
 * Kragen Sitaker <kragen@pobox.com>, 15 March 1998.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/wait.h>

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * Print the pgrp of a tty on some fd -- for debugging.
 */
#if 0
static int ppgrp (int i) { printf("%d: %d\n", i, tcgetpgrp(i)); }
#endif

/*
 * dup2, and if it fails, print an error message and exit.
 */
int dup2_wrap (int src, int dst) {
	if (dup2(src, dst) == -1) {
		perror("dup2");
		exit(1);
	}
        return 0;
}

void usage(const char *argv0) {
	fprintf(stderr, "Usage: %s <ttyname> <pathname> <argv0> <args . . .>\n",
		argv0);
	fprintf(stderr, "ttyname is the filename of the tty to use for stdin,\n");
	fprintf(stderr, "    stdout, and stderr;\n");
	fprintf(stderr, "pathname is the name of the program to run;\n");
	fprintf(stderr, "argv0 is the name by which the program should know itself;\n");
	fprintf(stderr, "args are other arguments to pass to it.\n");
	fprintf(stderr, "Typical example: %s /dev/tty6 /bin/tcsh -tcsh\n", argv0);
	exit(2);
}

int main(int argc, char **argv) {

	int fd;
	int i;

	/* argv[0], tty, pathname, argv0 is 4 */
	if (argc < 4) {
		usage(argv[0]);
	}

	fd = open(argv[1], O_RDWR);

	if (fd == -1) {
		fprintf(stderr, "open: ");
		perror(argv[1]);
		exit(1);
	}

	dup2_wrap(fd, 0);
	dup2_wrap(fd, 1);
	dup2_wrap(fd, 2);
	close(fd);

	i = fork();
	if (i == -1) {
		perror("fork");
	} else if (i != 0) {
		/* wait for the child to exit.  By this means, a caller
		 * waiting for the parent to exit gets something meaningful. */
		int rv;
		wait(&rv);
		exit(rv);
	}


	/* You must be a process group leader to set your controlling tty.
	 * This makes you a process group leader if you're not one already.
	 * (The fork() makes you not a process group leader if you *are* one
	 *  already.) */
	if (setsid() == -1) {
		perror("setsid()");
	}

	/* set controlling tty */
	if (ioctl(0, TIOCSCTTY, 1) != 0) {
		perror("TIOCSCTTY");
		exit(1);
	}

	/* ppgrp(0);
	 * ppgrp(1);
	 * ppgrp(2); */

	/* Note: this assumes that the argv array is null-terminated, which
	 * is common, but not guaranteed by any standard. */
	execv(argv[2], argv+3);
	perror("execv");
	return 0;
}
