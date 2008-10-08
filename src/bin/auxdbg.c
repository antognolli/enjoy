/**
 * @file
 *
 * Copyright (C) 2008 by ProFUSION embedded systems
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the  GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA.
 *
 * @author Rafael Antognolli <antognolli@profusion.mobi>
 */

#include <execinfo.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <error.h>

const char *
show_backtrace(const char *msg)
{
#define MAX_BACKTRACE_SIZE 256
    static void *buffer[MAX_BACKTRACE_SIZE];
    static char logname[PATH_MAX];
    const char *tmpdir;
    time_t now;
    int len, fd;

    len = backtrace(buffer, MAX_BACKTRACE_SIZE);

    fprintf(stderr, "\n\nBEGIN: backtrace with %d addresses: \"%s\"\n",
            len, msg);

    fflush(stderr);
    fd = fileno(stderr);

    backtrace_symbols_fd(buffer, len, fd);

    tmpdir = getenv("TMPDIR");
    if (!tmpdir)
        tmpdir = "/tmp";

    now = time(NULL);
    snprintf(logname, sizeof(logname), "%s/enjoy-%lu.bt", tmpdir, now);
    fprintf(stderr, "INFO: saving backtrace to %s\n", logname);
    fd = open(logname, O_WRONLY | O_CREAT | O_TRUNC);
    if (fd < 0) {
        fprintf(stderr, "ERROR: could not open for write \"%s\": %s\n",
                logname, strerror(errno));
        logname[0] = '\0';
        goto end;
    }

    write(fd, "User message: ", sizeof("User message: ") - 1);
    write(fd, msg, strlen(msg));
    write(fd, "\n", 1);
    backtrace_symbols_fd(buffer, len, fd);
    close(fd);

end:
    fprintf(stderr, "END: backtrace with %d addresses: \"%s\"\n\n\n", len, msg);

    return logname;
#undef MAX_BACKTRACE_SIZE
}

static char *
strsicode(const siginfo_t *info)
{
    switch (info->si_code) {
    case SI_USER:
        return "kill() or raise()";
    case SI_KERNEL:
        return "sent by kernel";
    case SI_QUEUE:
        return "sigqueue()";
    case SI_TIMER:
        return "POSIX timer expired";
    case SI_MESGQ:
        return "POSIX message queue state changed";
    case SI_ASYNCIO:
        return "AIO completed";
    case SI_SIGIO:
        return "queued SIGIO";
    case SI_TKILL:
        return "tkill() or tgkill()";
    }

    if (info->si_signo == SIGILL) {
        switch (info->si_code) {
        case ILL_ILLOPC:
            return "illegal opcode";
        case ILL_ILLOPN:
            return "illegal operand";
        case ILL_ILLADR:
            return "illegal addressing mode";
        case ILL_ILLTRP:
            return "illegal trap";
        case ILL_PRVOPC:
            return "privileged opcode";
        case ILL_PRVREG:
            return "privileged register";
        case ILL_COPROC:
            return "coprocessor error";
        case ILL_BADSTK:
            return "internal stack error";
        }
    }

    if (info->si_signo == SIGFPE) {
        switch (info->si_code) {
        case FPE_INTDIV:
            return "integer divide by zero";
        case FPE_INTOVF:
            return "integer overflow";
        case FPE_FLTDIV:
            return "floating point divide by zero";
        case FPE_FLTOVF:
            return "floating point overflow";
        case FPE_FLTUND:
            return "floating point underflow";
        case FPE_FLTRES:
            return "floating point inexact result";
        case FPE_FLTINV:
            return "floating point invalid operation";
        case FPE_FLTSUB:
            return "subscript out of range";
        }
    }

    if (info->si_signo == SIGSEGV) {
        switch (info->si_code) {
        case SEGV_MAPERR:
            return "address not mapped to object";
        case SEGV_ACCERR:
            return "invalid permissions for mapped object";
        }
    }

    if (info->si_signo == SIGBUS) {
        switch (info->si_code) {
        case BUS_ADRALN:
            return "invalid address alignment";
        case BUS_ADRERR:
            return "nonexistent physical address";
        case BUS_OBJERR:
            return "object-specific hardware error";
        }
    }

    if (info->si_signo == SIGTRAP) {
        switch (info->si_code) {
        case TRAP_BRKPT:
            return "process breakpoint";
        case TRAP_TRACE:
            return "process trace trap";
        }
    }

    if (info->si_signo == SIGCHLD) {
        switch (info->si_code) {
        case CLD_EXITED:
            return "child has exited";
        case CLD_KILLED:
            return "child was killed";
        case CLD_DUMPED:
            return "child terminated abnormally";
        case CLD_TRAPPED:
            return "traced child has trapped";
        case CLD_STOPPED:
            return "child has stopped";
        case CLD_CONTINUED:
            return "stopped  child  has  continued";
        }
    }

    return "?";
}

static void
sig_info(const siginfo_t *info)
{
    fprintf(stderr,
            "Signal Information:\n"
            "\t number = %d\n"
            "\t errno = %d \"%s\"\n"
            "\t code = %d \"%s\"\n",
            info->si_signo,
            info->si_errno, strerror(info->si_errno),
            info->si_code, strsicode(info));

    if (info->si_signo == SIGILL ||
        info->si_signo == SIGFPE ||
        info->si_signo == SIGSEGV ||
        info->si_signo == SIGBUS)
        fprintf(stderr, "\t address = %p\n", info->si_addr);

    if (info->si_signo == SIGCHLD)
        fprintf(stderr,
                "\t sending process id = %d\n"
                "\t real user id = %d\n"
                "\t status = %d\n"
                "\t user time = %ld\n"
                "\t system time = %ld\n",
                info->si_pid,
                info->si_uid,
                info->si_status,
                info->si_utime,
                info->si_stime);
}

static void
sig_action(int number, siginfo_t *info, void *data, const char *signame, const char *sigmsg)
{
    const char *logname;

    fputs(signame, stderr);
    fputc('\n', stderr);
    fputs(sigmsg, stderr);
    fputs("\nTrying to print backtrace, mail it to your technical support.\n",
          stderr);

    logname = show_backtrace("SIGSEGV");

    sig_info(info);

    error(-number, 0, "%s! check backtrace at \"%s\".\n", signame, logname);
}

static void
sigsegv_action(int number, siginfo_t *info, void *data)
{
    sig_action(number, info, data,
               "Segmentation Fault",
               "Your application crashed due an invalid memory access.");
}

static void
sigill_action(int number, siginfo_t *info, void *data)
{
    sig_action(number, info, data,
               "Illegal Instruction",
               "Your application crashed due an invalid instruction.");
}

static void
sigfpe_action(int number, siginfo_t *info, void *data)
{
    sig_action(number, info, data,
               "Floating Point Exception",
               "Your application crashed due a floating point exception.");
}

static void
sigbus_action(int number, siginfo_t *info, void *data)
{
    sig_action(number, info, data,
               "Bus Error",
               "Your application crashed due an improper memory handling.");
}

static void
sigabrt_action(int number, siginfo_t *info, void *data)
{
    sig_action(number, info, data,
               "Abort",
               "Your application aborted.");
}

void
set_death_handlers(void)
{
    struct sigaction action;

    action.sa_sigaction = sigsegv_action;
    action.sa_flags = SA_NODEFER | SA_RESETHAND | SA_SIGINFO;
    sigemptyset(&action.sa_mask);
    sigaction(SIGSEGV, &action, NULL);

    action.sa_sigaction = sigill_action;
    action.sa_flags = SA_NODEFER | SA_RESETHAND | SA_SIGINFO;
    sigemptyset(&action.sa_mask);
    sigaction(SIGILL, &action, NULL);

    action.sa_sigaction = sigfpe_action;
    action.sa_flags = SA_NODEFER | SA_RESETHAND | SA_SIGINFO;
    sigemptyset(&action.sa_mask);
    sigaction(SIGFPE, &action, NULL);

    action.sa_sigaction = sigbus_action;
    action.sa_flags = SA_NODEFER | SA_RESETHAND | SA_SIGINFO;
    sigemptyset(&action.sa_mask);
    sigaction(SIGBUS, &action, NULL);

    action.sa_sigaction = sigabrt_action;
    action.sa_flags = SA_NODEFER | SA_RESETHAND | SA_SIGINFO;
    sigemptyset(&action.sa_mask);
    sigaction(SIGABRT, &action, NULL);
}
