lockrun(1) - run cron job with overrun protection
==============================================

## SYNOPSIS

`lockrun` [-Q|--quiet] [-L lockfile-location] -- <command> <args>

## DESCRIPTION

Lockrun makes sure that the given command is executed only when the specified lockfile is not set.

## OPTIONS

`lockrun` supports GNU-style command-line options, and this includes
using `--` to mark their end:

    $ lockrun [options] -- [command]

The actual command after `--` can have any arguments it likes, and they
are entirely uninterpreted by `lockrun`.

We'll note that command-line redirection (`> /dev/null`, etc.) is not
supported by this or the command which follows -- it's handled **by the
calling shell**. This is the case whether it's run from cron or not.

 * `--idempotent`:

    Allows silent successful exit when lock contention is encountered.

 * `--lockfile=[filename]`

    Specify the name of a file which is used for locking. This filename
    is created if necessary (with mode 0666), and no I/O of any kind is
    done. This file is never removed.

 * `--maxtime=[N]`

    The script being controlled ought to run for no more than *N*
    seconds, and if it's beyond that time, we should report it to the
    standard error stream (which probably gets routed to the user via
    cron's email).


 * `--wait`

    When a pre-existing lock is found, this program normally exits with
    error, but adding the `--wait` parameter causes it to loop, waiting
    for the prior lock to be released.

 * `--verbose`

    Show a bit more runtime debugging.

  * `--`

    Mark the end of the options, the actual command to run follows.

## RATIONALE

When doing network monitoring, it's common to run a cron job every five
minutes (the standard interval) to roam around the network gathering
data. Smaller installations may have no trouble running within this
limit, but in larger networks or those where devices are often
unreachable, running past the five-minute mark could happen frequently.

The effect of running over depends on the nature of the monitoring
application: it could be of no consequence or it could be catastrophic.
What's in common is that running two jobs at once (the old one which ran
over, plus the new one) slows down the new one, increasing the risk that
*it* will run long as well.

This is commonly a cascading failure which can take many polling
sessions to right itself, which may include lost data in the interim.

Our response has been to create this tool, `lockrun`, which serves as a
protective wrapper. Before launching the given command, it insures that
another instance of the same command is not already running.

## BUILDING AND INSTALLING

This tool is published in the form of portable C source code, and it can
be compiled on any Linux/UNIX platform that provides a development
environment.

To build and install it from the command line:

    $ make
    $ sudo make install # or
    $ sudo make install PREFIX=/usr/local/

Now we'll find `lockrun` in the usual place: `/usr/bin/` or whatever you specify.

To install the manpages, you need [ronn](https://github.com/rtomayko/ronn). To 
install ronn on Debian, use `apt-get install ruby-ronn`. You can also use `gem
install ronn`.

## PORTING

We'll note that though portable, this program is nevertheless designed
only to run on UNIX or Linux systems: it certainly won't build and run
properly on a Windows computer.

Furthermore, file locking has always been one of the more problemtic
areas of portability, there being several mechanisms in place. `lockrun`
uses the `flock()` system call, and this of course requires low-level OS
support.

We've tested this in FreeBSD and Linux, but other operating systems
might trip over compilation issues. We welcome portability reports (for
good or bad).

We've also received a report that this works on Apple's OS X.

## EXAMPLES

Once `lockrun` has been built and installed, it's time to put it
to work. This is virtually always used in a crontab entry, and the
command line should include the name of the lockfile to use as well
as the command to run.

This entry in a crontab file runs the Cacti poller script every five
minutes, protected by a lockfile:

    */5 * * * * /usr/local/bin/lockrun --lockfile=/tmp/cacti.lockrun -- /usr/local/bin/cron-cacti-poller

The file used, `/tmp/cacti.lockrun`, is created (if necessary), the lock
acquired, and closed when finished. At no time does `lockrun` perform
any file I/O: the file exists only to be the subject of locking
requests.

Note that everything up to the standalone `--` is considered an option
to `lockrun`, but everything after is the literal command to run.

The example provided here is a run-or-nothing instance: if the lock
cannot be acquired, the program exits with a failure message to the
standard error stream, which hopefully is routed back to the user via an
email notification:

`ERROR: cannot launch [command line] - run is locked`

This mechanism effectively *skips a polling run*, but this may be the
only option when polling runs long periodically. If one polling run goes
quite long, it's conceivable that multiple subsequent jobs could be
stacked behind the slow one, and never getting caught up.

But if most jobs complete very rapidly, adding the `--wait`
parameter might allow the system to catch up after a lone straggler
runs long.


However one organizes this, one can't avoid being concerned with runs
which are locked often. An inability to complete a polling run on time
indicates a resource-allocation problem which is not actually fixed by
skipping some data.

If this happens regularly, it's important to track down what's causing
the overruns: lack of memory? inadequate CPU? serialized jobs which
could benefit from parallelization or asynchronous processing?

There is no substitute for actual human observation of important
systems, and though `lockrun` may forestall a monitoring meltdown, it
doesn't replace paying attention. It is **not** an advanced command
queuing system.

## LOCKING BEHAVIOUR

We've been asked why we do this in a C program and not a simple shell
script: the answer is that we require bulletproof, no-maintenance
protection, and that's very hard to do with shell scripting.

With touch-a-file locking, there's a chance that the lockfile can be
left around after everything is done: what if the cron job has run long,
and the administrator killed everything associated with the job? What
about a system crash leaving the lockfile around? What if there's a
fatal error in `lockrun` itself? All of these leave the lockfile around
in the system for the next run to trip over.

One could make this mechanism smarter by including the `PID` of the
locking process inside the file, and then using `kill(*pid*,0)` to see
if that process exists, but PIDs are reused, and it's possible to have a
false positive (i.e., when the previous `lockrun` has finished, but some
*other* process has taken that PID slot). We've always disliked the
nondeterminism of this mechanism.

So we required a mechanism which provided guaranteed, bulletproof
cleanup at program exit, and no chance of false positives. Though one
can find numerous mechanisms for this, use of file locks is the easiest
to code and understand. Setting a lock automatically tests for the
previous lock, and this means no race conditions to worry about. When
the file is closed, locks evaporate.

Note that file locking under UNIX is typically *advisory* only: A lock
placed by one process is only honored by other processes who chose to
check the lock first. Any process with suitable permissions is free to
read or write anything without regard to locks.

Advisory locking works on the honor system, but they're entirely
appropriate for our use here.

Finally, we'll note that our locking mechanism is only designed to
prevent two lock-protected processes from running at once; It is *not* a
queuing system.

When using the `--wait` parameter, it's entirely possible to have many
processes stacked up in line behind a prior long-running process. When
the long-running process exists, it's impossible to predict which of the
waiting processes will run next, and it's probably not going to be done
in the order in which they were launched. Users with more sophisticated
queuing requirements probably need to find a different mechanism.

History
=======

 * 2013/08/02 - return execvp's value if running child process fails (Allard Hoeve)
 * 2010/10/04 - added idempotency to allow run lock contention to be treated as a no-op (Mike Cerna, Groupon)
 * 2009/06/25 — added lockf() support for Solaris 10 (thanks to Michal Bella)
 * 2009/03/09 — Tracked on GitHub by Peter Harkins.
 * 2006/06/03 — initial release by Stephen J. Friedl. <http://unixwiz.net/archives/2006/06/new_tool_lockru.html>

License
=======

This software is public domain.
