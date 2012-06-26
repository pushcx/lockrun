/*
 * See README.markdown for build, install, and usage instructions.
 */

#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/file.h>

#ifndef __GNUC__
# define __attribute__(x)	/* nothing */
#endif


#define STRMATCH(a,b)		(strcmp((a),(b)) == 0)

#define UNUSED_PARAMETER(v)	((void)(v))

#define	TRUE	1
#define	FALSE	0

static const char	*lockfile = 0;
static int		wait_for_lock = FALSE;
static mode_t		openmode = 0666;
static int		sleeptime = 10;		/* seconds */
static int		Verbose = FALSE;
static int		Maxtime  = 0;
static int		idempotent = FALSE;

static char *getarg(char *opt, char ***pargv);

static void die(const char *format, ...)
		__attribute__((noreturn))
		__attribute__((format(printf, 1, 2)));

#ifdef __sun
# define WAIT_AND_LOCK(fd) lockf(fd, F_TLOCK,0)
#else
# define WAIT_AND_LOCK(fd) flock(fd, LOCK_EX | LOCK_NB)
#endif

int main(int argc, char **argv)
{
	char	*Argv0 = *argv;
	int	rc;
	int	lfd;
	pid_t	childpid;
	time_t	starttime;

	UNUSED_PARAMETER(argc);

	time(&starttime);

	for ( argv++ ; *argv ; argv++ )
	{
		char    *arg = *argv;
		char	*opt = strchr(arg, '=');

		/* the -- token marks the end of the list */

		if ( strcmp(*argv, "--") == 0 )
		{
			argv++;
			break;
		}

		if (opt) *opt++ = '\0'; /* pick off the =VALUE part */

		if ( STRMATCH(arg, "-L") || STRMATCH(arg, "--lockfile"))
		{
			lockfile = getarg(opt, &argv);
		}

		else if ( STRMATCH(arg, "-W") || STRMATCH(arg, "--wait"))
		{
			wait_for_lock = TRUE;
		}

		else if ( STRMATCH(arg, "-S") || STRMATCH(arg, "--sleep"))
		{
			sleeptime = atoi(getarg(opt, &argv));
		}

		else if ( STRMATCH(arg, "-T") || STRMATCH(arg, "--maxtime"))
		{
			Maxtime = atoi(getarg(opt, &argv));
		}

		else if ( STRMATCH(arg, "-V") || STRMATCH(arg, "--verbose"))
		{
			Verbose++;
		}
		
		else if ( STRMATCH(arg, "-I") || STRMATCH(arg, "--idempotent"))
		{
			idempotent = TRUE;
		}

		else
		{
			die("ERROR: \"%s\" is an invalid cmdline param", arg);
		}
	}

	/*----------------------------------------------------------------
	 * SANITY CHECKING
	 *
	 * Make sure that we have all the parameters we require
	 */
	if (*argv == 0)
		die("ERROR: missing command to %s (must follow \"--\" marker) ", Argv0);

	if (lockfile == 0)
		die("ERROR: missing --lockfile=F parameter");

	/*----------------------------------------------------------------
	 * Open or create the lockfile, then try to acquire the lock. If
	 * the lock is acquired immediately (==0), then we're done, but
	 * if the lock is not available, we have to wait for it.
	 *
	 * We can either loop trying for the lock (for --wait), or exit
	 * with error.
	 */

	if ( (lfd = open(lockfile, O_RDWR|O_CREAT|O_TRUNC|O_SYNC, openmode)) < 0)
		die("ERROR: cannot open(%s) [err=%s]", lockfile, strerror(errno));

	while ( WAIT_AND_LOCK(lfd) != 0 )
	{
		if ( ! wait_for_lock )
		{
			
			if(idempotent) /* given the idempotent flag, we treat contention as a no-op */
			{
				exit(EXIT_SUCCESS);
			}
			else
			{
				die("ERROR: cannot launch %s - run is locked", argv[0]);
			}
		}

		/* waiting */
		if ( Verbose ) printf("(locked: sleeping %d secs)\n", sleeptime);

		sleep(sleeptime);
	}

	fflush(stdout);

	/* run the child */


	if ( (childpid = fork()) == 0 )
	{
		close(lfd);		// don't need the lock file

		execvp(argv[0], argv);
	}
	else if ( childpid > 0 )
	{
		time_t endtime;
		pid_t  pid;
		int    status;
		char   buffer[41];
		int    size;

		// write the childpid to the lockfile
		size = snprintf(buffer, 40, "%ld", (long) childpid);
		write(lfd, buffer, size);

		if ( Verbose )
		    printf("Waiting for process %ld\n", (long) childpid);

		pid = waitpid(childpid, &status, 0);
		rc = WEXITSTATUS(status);

		time(&endtime);

		endtime -= starttime;

		if ( Verbose || (Maxtime > 0  &&  endtime > Maxtime) )
		    printf("pid %d exited with status %d, exit code: %d (time=%ld sec)\n",
			   pid, status, rc, endtime);

		// remove the childpid from the lock file
		ftruncate(lfd, 0);
	}
	else
	{
		die("ERROR: cannot fork [%s]", strerror(errno));
	}

	exit(rc);
}


/*! \fn static char *getarg(char *opt, char ***pargv)
 *  \brief A function to parse calling parameters
 *
 *	This is a helper for the main arg-processing loop: we work with
 *	options which are either of the form "-X=FOO" or "-X FOO"; we
 *	want an easy way to handle either one.
 *
 *	The idea is that if the parameter has an = sign, we use the rest
 *	of that same argv[X] string, otherwise we have to get the *next*
 *	argv[X] string. But it's an error if an option-requiring param
 *	is at the end of the list with no argument to follow.
 *
 *	The option name could be of the form "-C" or "--conf", but we
 *	grab it from the existing argv[] so we can report it well.
 *
 * \return character pointer to the argument
 *
 */
static char *getarg(char *opt, char ***pargv)
{
	const char *const optname = **pargv;

	/* option already set? */
	if (opt) return opt;

	/* advance to next argv[] and try that one */
	if ((opt = *++(*pargv)) == 0)
		die("ERROR: option %s requires a parameter", optname);

	return opt;
}

/*
 * die()
 *
 *	Given a printf-style argument list, format it to the standard error,
 *	append a newline, then exit with error status.
 */

static void die(const char *format, ...)
{
va_list	args;

	va_start(args, format);
	vfprintf(stderr, format, args);
	putc('\n', stderr);
	va_end(args);

	exit(EXIT_FAILURE);
}
