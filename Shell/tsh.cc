/////////////////////////////////////////////////////////////////////////////
//
// Student: Ben Adams
// Id: 1810672
// Course: Operating Systems
//
/////////////////////////////////////////////////////////////////////////////// 

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include "globals.h"
#include "jobs.h"
#include "helper-routines.h"
#include <iostream>

/////////////////////////////////////////////////////////////////////////////// 
//
// GLOBALS
//
/////////////////////////////////////////////////////////////////////////////// 
static char prompt[] = "tsh> ";
int verbose = 0;
char *cmdline;

/////////////////////////////////////////////////////////////////////////////// 
//
// PROTOTYPES
// 
/////////////////////////////////////////////////////////////////////////////// 
int eval(int argc, char *cmdline);
int builtin_cmd(int argc, char **argv);
void do_bgfg(char **argv);
void waitfg(pid_t pid);
void change_directory(char**argv);
void sigchld_handler(int sig);
void sigtstp_handler(int sig);
void sigint_handler(int sig);
void welcome();


///////////////////////////////////////////////////////////////////////////////
//
// main - The shell's main routine 
//
///////////////////////////////////////////////////////////////////////////////
int main(int argc, char **argv) 
{
  int emit_prompt = 1; // emit prompt (default)

  // Redirect stderr to stdout (so that 
  // driver will get all outputon the 
  // pipe connected to stdout)
  dup2(1, 2);

  // Install the signal handlers
  Signal(SIGINT,  sigint_handler);   // ctrl-c
  Signal(SIGTSTP, sigtstp_handler);  // ctrl-z
  Signal(SIGCHLD, sigchld_handler);  // Terminated or stopped child
  Signal(SIGQUIT, sigquit_handler);   // This one provides a clean way to kill the shell

  // Initialize the job list
  initjobs(jobs);

  welcome();

  // Execute the shell's read/eval loop
  for(;;) {
    // Read command line
    if (emit_prompt) {
      printf("%s", prompt);
      fflush(stdout);
    }

    int initial_size = sizeof(char)*10;
    if ((cmdline = (char *)malloc(initial_size)) == NULL) {
      perror("error: malloc failed in main");
    }

    // Read the command line
    int i = 0; int next = 0;

    // Handle case where user enters nothing
    cmdline[0] = '\n';

    // Use fgetc to parse and store user input
    while((next = fgetc(stdin)) != '\n' && next != EOF) {
      cmdline[i++] = next; // store character
      if(i == initial_size) { // when capacity is reached
        initial_size *= 2; // double it
        if((cmdline = (char *)realloc(cmdline, sizeof(char)*(initial_size))) == NULL) {
          perror("Error: realloc failed in main");
        }
      }
      cmdline[i] = '\n'; // append the null terminating character
    } 

    // End of file? (did user type ctrl-d?)
    if (feof(stdin)) {
      fflush(stdout);
      exit(0);
    }

    // Evaluate command line
    if(eval(argc, cmdline)==0){
      free(cmdline);
      fflush(stdout);
    }
  } 

  exit(0); //control never reaches here
}

/////////////////////////////////////////////////////////////////////////////
//
// welcome - Print a welcome message
//
/////////////////////////////////////////////////////////////////////////////

void welcome() {
  printf("Hello, welcome to the shell!\n");
}
  
/////////////////////////////////////////////////////////////////////////////
//
// eval - Evaluate the command line that the user has just typed in
// 
// If the user has requested a built-in command (quit, jobs, bg or fg)
// then execute it immediately. Otherwise, fork a child process and
// run the job in the context of the child. If the job is running in
// the foreground, wait for it to terminate and then return.  Note:
// each child process must have a unique process group ID so that our
// background children don't receive SIGINT (SIGTSTP) from the kernel
// when we type ctrl-c (ctrl-z) at the keyboard.
//
/////////////////////////////////////////////////////////////////////////////
int eval(int argc, char *cmdline) 
{
  // Parse command line
  // The 'argv' vector is filled in by the parseline
  // routine below. It provides the arguments needed
  // for the execve() routine, which you'll need to
  // use below to launch a process.
  char *argv[MAXARGS];
  sigset_t mask_all, mask1, prev_mask;
  sigemptyset(&mask1);
  sigaddset(&mask1, SIGCHLD);
  sigfillset(&mask_all);

  // The 'bg' variable is TRUE if the job should run
  // in background mode or FALSE if it should run in FG
  pid_t pid; 
  struct job_t *job;
  int bg = parseline(cmdline, argv); 
  if (strcmp(argv[0],"DONE")==0) {
    free(cmdline);
    exit(0);
  }

  if (!builtin_cmd(argc, argv)){
   sigprocmask(SIG_BLOCK, &mask1, &prev_mask);
    pid = fork();
    setpgid(0,0);
    if (pid == 0){ // child
      // exec in child
      sigprocmask(SIG_SETMASK, &prev_mask, NULL);
      execvp(argv[0], argv); // this should not return
      printf("%s: Command not found\n", argv[0]);
      exit(0);
    } else if (!bg) { // parent and foreground
        sigprocmask(SIG_BLOCK, &mask_all, NULL);
        addjob(jobs, pid, bg ? BG : FG, cmdline);
        sigprocmask(SIG_SETMASK, &prev_mask, NULL);
        job = getjobpid(jobs, pid);
        int pid = job->pid;
        waitfg(pid); // parent waits for child
      } else { // parent and background
      job = getjobpid(jobs, pid);
      sigprocmask(SIG_BLOCK, &mask_all, NULL);
      addjob(jobs, pid, bg ? BG : FG, cmdline);
      sigprocmask(SIG_SETMASK, &prev_mask, NULL);
      job = getjobpid(jobs,pid);
      printf("[%d] (%d) %s", job->jid, job->pid, cmdline);   
    }
  } 
  if (argv[0] == NULL)  {
    return 0;   /* ignore empty lines */
  }
  return 0;
}


/////////////////////////////////////////////////////////////////////////////
//
// builtin_cmd - If the user has typed a built-in command then execute
// it immediately. The command name would be in argv[0] and
// is a C string. We've cast this to a C++ string type to simplify
// string comparisons; however, the do_bgfg routine will need 
// to use the argv array as well to look for a job number.
//
/////////////////////////////////////////////////////////////////////////////
int builtin_cmd(int argc, char **argv) 
{
  if (strcmp(argv[0], "quit")==0){
    free(cmdline);
    exit(0);
  } else if (strcmp(argv[0], "fg")==0) {
    do_bgfg(argv);
    return 1;
  } else if (strcmp(argv[0], "bg")==0) {
    do_bgfg(argv);
    return 1;
  } else if (strcmp(argv[0], "jobs")==0){
    listjobs(jobs);
    return 1;
  } else if(strcmp(argv[0], "cd")==0) {
    change_directory(argv);
    return 1;
  }
  return 0;     /* not a builtin command */
}

/////////////////////////////////////////////////////////////////////////////
//
// change_directory - Helper function to assist with "cd" command
//
/////////////////////////////////////////////////////////////////////////////
void change_directory(char**argv) {
  if(chdir(argv[1]) == -1) {
    perror("error: changing directories in builtin_cmd()");
  }
}

/////////////////////////////////////////////////////////////////////////////
//
// do_bgfg - Execute the builtin bg and fg commands
//
/////////////////////////////////////////////////////////////////////////////
void do_bgfg(char **argv) 
{
  struct job_t *jobp=NULL;
    
  // Ignore command if no argument 
  if (argv[1] == NULL) {
    printf("%s command requires PID or %%jobid argument\n", argv[0]);
    return;
  }

  // Parse the required PID or %JID arg 
  if (isdigit(argv[1][0])) {
    pid_t pid = atoi(argv[1]);
    if (!(jobp = getjobpid(jobs, pid))) {
      printf("(%d): No such process\n", pid);
      return;
    }
  }
  else if (argv[1][0] == '%') {
    int jid = atoi(&argv[1][1]);
    if (!(jobp = getjobjid(jobs, jid))) {
      printf("%s: No such job\n", argv[1]);
      return;
    }
  }	    
  else {
    printf("%s: argument must be a PID or %%jobid\n", argv[0]);
    return;
  }

  if (strcmp(argv[0], "bg")==0) { // stopped  bg job -> running bg job
    printf("[%d] (%d) %s", jobp->jid, jobp->pid, jobp->cmdline);
    jobp->state = BG;
    kill(-jobp->pid, SIGCONT);
  } else if (strcmp(argv[0], "fg") ==0) { // stopped or running bg job -> running fg job
    jobp->state= FG;
    kill(-jobp->pid, SIGCONT);
    waitfg(jobp->pid); // can't have more than 1 process in fg
  }
  return;
}

/////////////////////////////////////////////////////////////////////////////
//
// waitfg - Block until process pid is no longer the foreground process
//
/////////////////////////////////////////////////////////////////////////////
void waitfg(pid_t pid)
{
  while(pid == fgpid(jobs)) { // sleep while pid in fg 
    sleep(1);
  }
  return;
}

/////////////////////////////////////////////////////////////////////////////
//
// Signal handlers
//
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//
// sigchld_handler - The kernel sends a SIGCHLD to the shell whenever
//     a child job terminates (becomes a zombie), or stops because it
//     received a SIGSTOP or SIGTSTP signal. The handler reaps all
//     available zombie children, but doesn't wait for any other
//     currently running children to terminate.  
//
/////////////////////////////////////////////////////////////////////////////
void sigchld_handler(int sig) 
{
  int olderrno = errno;
  pid_t pid;
  int status;
  sigset_t mask_all, prev_all;
  
  sigfillset(&mask_all); 

  if ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
    sigprocmask(SIG_BLOCK, &mask_all, &prev_all);
    struct job_t *job = getjobpid(jobs, pid);
    if (WIFSIGNALED(status)) { // handles interrupt signal
      printf("Job [%d] (%d)\n", job->jid, job->pid);
      sigprocmask(SIG_BLOCK, &mask_all, &prev_all);
      deletejob(jobs,pid);
      sigprocmask(SIG_SETMASK, &prev_all, NULL);
    } else if (WIFSTOPPED(status)){ // handles stopped processes
      job->state = ST;
      printf("Job [%d] (%d) stopped by signal 20\n", job->jid, job->pid);
    } else if (WIFEXITED(status)){ // handle normal termination
      sigprocmask(SIG_BLOCK, &mask_all, &prev_all);
      deletejob(jobs,pid);
      sigprocmask(SIG_SETMASK, &prev_all, NULL);
    }
    sigprocmask(SIG_SETMASK, &prev_all, NULL);
  }
  errno = olderrno;
}

/////////////////////////////////////////////////////////////////////////////
//
// sigint_handler - The kernel sends a SIGINT to the shell whenver the
//    user types ctrl-c at the keyboard.  Catch it and send it along
//    to the foreground job.  
//
/////////////////////////////////////////////////////////////////////////////
void sigint_handler(int sig) 
{
  pid_t pid; 
  if ((pid = fgpid(jobs)) > 0) { // if there is a fg process
    kill(-pid, sig); // kill it
    free(cmdline);
  }
}

/////////////////////////////////////////////////////////////////////////////
//
// sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
//     the user types ctrl-z at the keyboard. Catch it and suspend the
//     foreground job by sending it a SIGTSTP.  
//
/////////////////////////////////////////////////////////////////////////////
void sigtstp_handler(int sig) 
{
  pid_t pid;
  if ((pid = fgpid(jobs)) > 0) { // if there is a fg process
    kill(-pid, sig); // stop it
  }
}

/////////////////////////////////////////////////////////////////////////////
//
// End signal handlers
//
/////////////////////////////////////////////////////////////////////////////





