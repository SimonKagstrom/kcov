#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <sched.h>

void catch_hup(int sig)
{
	printf("HUP\n");
}

void catch_int(int sig)
{
	printf("INT\n");
}

void catch_quit(int sig)
{
	printf("QUIT\n");
}

void catch_ill(int sig)
{
	printf("ILL\n");
}

void catch_trap(int sig)
{
	printf("TRAP\n");
}

void catch_abrt(int sig)
{
	printf("ABRT\n");
}

void catch_bus(int sig)
{
	printf("BUS\n");
}

void catch_fpe(int sig)
{
	printf("FPE\n");
}

void catch_kill(int sig)
{
	printf("KILL\n");
}

void catch_usr1(int sig)
{
	printf("USR1\n");
}

void catch_segv(int sig)
{
	printf("SEGV\n");
}

void catch_usr2(int sig)
{
	printf("USR2\n");
}

void catch_pipe(int sig)
{
	printf("PIPE\n");
}

void catch_alarm(int sig)
{
	printf("ALARM\n");
}

void catch_term(int sig)
{
	printf("TERM\n");
}

void catch_stkflt(int sig)
{
	printf("STKFLT\n");
}

void catch_chld(int sig)
{
	printf("CHLD\n");
}

void catch_cont(int sig)
{
	printf("CONT\n");
}

void catch_stop(int sig)
{
	printf("STOP\n");
}

void catch_stp(int sig)
{
	printf("STP\n");
}

void catch_ttin(int sig)
{
	printf("TTIN\n");
}

void catch_ttou(int sig)
{
	printf("TTOU\n");
}

void catch_urg(int sig)
{
	printf("URG\n");
}

void catch_xcpu(int sig)
{
	printf("XCPU\n");
}

void catch_xfsz(int sig)
{
	printf("XFSZ\n");
}

void catch_vtalrm(int sig)
{
	printf("VTALRM\n");
}

void catch_prof(int sig)
{
	printf("PROF\n");
}

void catch_winch(int sig)
{
	printf("WINCH\n");
}

void catch_gio(int sig)
{
	printf("GIO\n");
}

void catch_pwr(int sig)
{
	printf("PWR\n");
}

void catch_sys(int sig)
{
	printf("SYS\n");
}

static const char *handler_names[] =
{
	"zero has no handler",
	"hup",
	"int",
	"quit",
	"ill",
	"trap",
	"abrt",
	"bus",
	"fpe",
	"kill",
	"usr1",
	"segv",
	"usr2",
	"pipe",
	"alarm",
	"term",
	"stkflt",
	"chld",
	"cont",
	"stop",
	"stp",
	"ttin",
	"ttou",
	"urg",
	"xcpu",
	"xfsz",
	"vtalrm",
	"prof",
	"winch",
	"gio",
	"pwr",
	"sys",
};

static void (*handler_table[])(int) =
{
	NULL,
	catch_hup,  //SIGHUP
	catch_int,  //SIGINT
	catch_quit, //SIGQUIT
	catch_ill,  //SIGILL
	catch_trap, //SIGTRAP
	catch_abrt, //SIGABRT
	catch_bus,  //SIGBUS
	catch_fpe,  //SIGFPE
	catch_kill, //SIGKILL
	catch_usr1, //SIGUSR1
	catch_segv, //SIGSEGV
	catch_usr2, //SIGUSR2
	catch_pipe, //SIGPIPE
	catch_alarm,//SIGALRM
	catch_term, //SIGTERM
	catch_stkflt,//SIGSTKFLT
	catch_chld, //SIGCHLD
	catch_cont, //SIGCONT
	catch_stop, //SIGSTOP
	catch_stp,  //SIGTSTP
	catch_ttin, //SIGTTIN
	catch_ttou, //SIGTTOU
	catch_urg,  //SIGURG
	catch_xcpu, //SIGXCPU
	catch_xfsz, //SIGXFSZ
	catch_vtalrm,//SIGVTALRM
	catch_prof, //SIGPROF
	catch_winch,//SIGWINCH
	catch_gio,  //SIGIO
	catch_pwr,  //SIGPWR
	catch_sys,  //SIGSYS
};



int main(int argc, const char *argv[])
{
	pid_t child;
	int status;
	int sig;
	unsigned self = 0;
	size_t n_handlers = sizeof(handler_names) / sizeof(handler_names[0]);

	if (argc <= 1) {
		printf("Usage: xxx <signal> [self]\n");
		exit(1);
	}

	if (argc == 3)
		self = 1;

	for (sig = 0; sig < n_handlers; sig++) {
			if (strcmp(argv[1], handler_names[sig]) == 0)
				break;
	}
	if (sig == n_handlers) {
		printf("Unknown handler %s\n", argv[1]);
		exit(1);
	}

	child = fork();
	if (child < 0) {
		fprintf(stderr, "fork failed!\n");
		return -1;
	}

	if (child == 0) {
		signal(sig, handler_table[sig]);

		sleep(2);
		return 0;
	} else {
		// Parent
		sleep(1);
		if (self)
			kill(getpid(), sig);
		else
			kill(child, sig);
	}

	wait(&status);

	return 0;
}
