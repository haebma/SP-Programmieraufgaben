#include "plist.h"
#include "shellutils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <limits.h>

static volatile bool foreGroundProc = false; // zeigt ob ein Vordergrundprozess läuft/
											 // true = ja
											 // false = nein

static void printProcEvent(pid_t pid, const char cmdLine[], int event) {
	fprintf(stderr, "[%d] \"%s\" ", pid, cmdLine);
	if (WIFEXITED(event) != 0)
		fprintf(stderr, "exited with status %d", WEXITSTATUS(event));
	else if (WIFSIGNALED(event) != 0)
		fprintf(stderr, "terminated by signal %d", WTERMSIG(event));
	else if (WIFSTOPPED(event) != 0)
		fprintf(stderr, "stopped by signal %d", WSTOPSIG(event));
	else if (WIFCONTINUED(event) != 0)
		fputs("continued", stderr);
	putc('\n', stderr);
}


static void die(const char message[]) {
	perror(message);
	exit(EXIT_FAILURE);
}

// Signale vom Typ 'signal' werden 'how' behandelt (setzt Signalbehandlung dafür entsprechend)
static void handleSig(int signal, int how){
	sigset_t set;
	if(sigemptyset(&set) == -1) die("sigemptyset");
	if(sigaddset(&set, signal) == -1) die("sigaddset");
	if(sigprocmask(how, &set, NULL) == -1) die("sigprocmask");
}

/* Zombie-Prozesse sollen sofort aufgesammelt werden */
static void handleZombies(){

	int errnoRestore = errno; // errno speichern
	pid_t pid;
	int   event;
	while ((pid = waitpid(-1, &event, WNOHANG)) > 0) {
		if(plistGet(pid)->background == false) foreGroundProc = false; // kein Vordergrundprozess läuft mehr (es gibt max 1 V-Prozess in plist)
		if(plistNotifyEvent(pid, event) == -1) {} // kein printf wegen Nebenläufigkeit
	}
	errno = errnoRestore;
}

// signalhandler für SIGCHLD
static void handleSigchld(){
	struct sigaction act = { //  
		.sa_handler = handleZombies,
		.sa_flags = SA_RESTART,
	};

	if(sigemptyset(&act.sa_mask) == -1) die("sigemptyset");
	if(sigaction(SIGCHLD, &act, NULL) == -1) die("sigaction");
}

static void changeCwd(char *argv[]) {
	if (argv[1] == NULL || argv[2] != NULL) {
		fprintf(stderr, "Usage: %s <dir>\n", argv[0]);
		return;
	}
	if (chdir(argv[1]) != 0)
		perror(argv[0]);
}

static int printProcInfo(const ProcInfo *info) {
	if(info->background){
		fprintf(stderr, "[%d] \"%s\"\n", info->pid, info->cmdLine);
	}
	return 0;
}

/* ist safe, da in Funktion blockiert und freigegeben wird */
static void printJobs(char *argv[]) {
	if (argv[1] != NULL) {
		fprintf(stderr, "Usage: %s\n", argv[0]);
		return;
	}

	handleSig(SIGCHLD, SIG_BLOCK);
	plistIterate(printProcInfo);
	handleSig(SIGCHLD, SIG_UNBLOCK);
}

/* printet die Zombies und entfernt sie aus plist, falls sie terminiert sind */
static int printZombies(const ProcInfo *info, int event){

	printProcEvent(info->pid, info->cmdLine, event);

	if(WIFSIGNALED(event) || WIFEXITED(event)){
		handleSig(SIGCHLD, SIG_BLOCK);
		if(plistRemove(info->pid) == -1){
			fprintf(stderr, "No process with the given PID was found in the list.\n");
		}
		handleSig(SIGCHLD, SIG_UNBLOCK);
	}

	return 0;
}

static int nukeEveryone(const ProcInfo *infos){
	if(kill(infos->pid, SIGTERM) == -1) {
		perror("kill");
		return -1;
	}
	return 0;
}

static void nuke(char *argv[]){
	if(argv[1] != NULL && argv[2] != NULL){
		fprintf(stderr, "Usage: %s [pid]\n", argv[0]);
		return;
	}
	if(argv[1] == NULL){ // allen Kindprozessen SIGTERM zustellen
		handleSig(SIGCHLD, SIG_BLOCK);
		int check = plistIterate(nukeEveryone);
		if(check == -1){
			fprintf(stderr, "nuke: couldn't terminate all child processes");
			handleSig(SIGCHLD, SIG_UNBLOCK);
			return;
		}
		handleSig(SIGCHLD, SIG_UNBLOCK);
	} else { // Kindprozess mit [pid] SIGTERM zustellen
		char *endptr; 
		errno = 0;
		long childPid = strtol(argv[1], &endptr, 10);
		if(errno){
			perror("strtol");
			return;
		}
		if(strcmp(argv[1],"") == 0){ // check for leeres Wort (argv[1] == endptr)
			fprintf(stderr, "nuke: Invalid Pid\n");
		}
		if(*endptr != '\0'){ // check for buchstaben
			fprintf(stderr, "nuke: Invalid Pid\n");
			return;
		}
		if(childPid > LONG_MAX){ // max value for pid_t (laut man pid_t)
			fprintf(stderr, "nuke: Pid argument is out of range\n");
			return;
		}
		if(childPid < 0){ // min value for pid_t
			fprintf(stderr, "nuke: Pid argument is out of range\n");
			return;
		}

		handleSig(SIGCHLD, SIG_BLOCK);
		if (plistGet(childPid) == NULL){ // checken, ob pid existiert
			fprintf(stderr, "nuke: No such child process\n");
			handleSig(SIGCHLD, SIG_UNBLOCK);
			return;
		}

		if(kill(childPid, SIGTERM) == -1){
			perror("kill");
			handleSig(SIGCHLD, SIG_UNBLOCK);
			return;
		}
		handleSig(SIGCHLD, SIG_UNBLOCK);

	}
	
	// kein Signal darf sleep() unterbrechen
	sigset_t sleepSet;
	if(sigfillset(&sleepSet) == -1) {
		perror("sigfillset");
		return;
	}
	if(sigprocmask(SIG_BLOCK, &sleepSet, NULL)) {
		perror("sigprocmask");
		return;
	} 

	sleep(1); // warten, um Kindprozessen Zeit zum Terminieren zu geben

	if(sigprocmask(SIG_UNBLOCK, &sleepSet, NULL) == -1){
		die("sigprocmask"); // hier sterben, weil Signale nicht unblocked -> sind weiterhin blockiert
	}
}

// Umleiten des Standardein-/ ausgabekanals
static void redirect(int inFile, int outFile){	
	if(inFile > -1){ // Inhalt von inFile in stdin umleiten
		dup2(inFile, STDIN_FILENO);
	}
	if(outFile > -1){ // Inhalt von stdout in outFile umleiten
		dup2(outFile, STDOUT_FILENO);
	}
}

static void execute(ShCommand *cmd) {

	handleSig(SIGCHLD, SIG_BLOCK); // blockieren, bis PID in plist eingetragen wurde

	pid_t pid = fork();
	if (pid == -1){
		handleSig(SIGCHLD, SIG_UNBLOCK);
		die("fork");
	}
	if (pid == 0) { // Child process

		handleSig(SIGCHLD, SIG_UNBLOCK);

		// Signalbehandlung von SIGINT zurücksetzen, falls Kind im Vordergrund läuft
		if(cmd->background == false){
			handleSig(SIGINT, SIG_UNBLOCK);
		}

		// check for in-/outFile
		int inFile, outFile = -1;
		if(cmd->inFile != NULL){
			inFile = open(cmd->inFile, O_RDONLY,  0666);
			if(inFile == -1) die("open");
		}
		if(cmd->outFile != NULL){
			outFile = open(cmd->outFile, O_WRONLY | O_CREAT | O_TRUNC,
										 S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
			if(outFile == -1) die("open");
		}
		redirect(inFile, outFile);
		
		// Execute the desired program
		execvp(cmd->argv[0], cmd->argv);

		if(close(inFile) == -1 && inFile != -1) die("close");
		if(close(outFile) == -1 && outFile != -1) die("close");
		
		die(cmd->argv[0]);
	} else {		// Parent process

		/* alle Prozesse werden in plist eingetragen */
		const ProcInfo *info = plistAdd(pid, cmd->cmdLine, cmd->background);
		if (info == NULL)
			die("plistAdd");
		handleSig(SIGCHLD, SIG_UNBLOCK);

		if (cmd->background == true) { // Child runs in background

			handleSig(SIGCHLD, SIG_BLOCK);
			printProcInfo(info);
			handleSig(SIGCHLD, SIG_UNBLOCK);

		} else {					   // Child runs in foreground

			/* hier mit sigsuspend auf Vordergrundprozess warten*/
			sigset_t suspendMask;

			if(sigfillset(&suspendMask) < 0) die("sigfillset");
			if(sigdelset(&suspendMask, SIGCHLD) < 0) die("sigdelset");

			handleSig(SIGCHLD, SIG_BLOCK);
			foreGroundProc = true;
			handleSig(SIGCHLD, SIG_UNBLOCK);

			while(foreGroundProc){ // solange schlafen, bis der Vordergrundprozess nicht mehr läuft
				int errnoRestore = 0;
				errno = 0;

				sigsuspend(&suspendMask);
				
				if(errno != EINTR){ // kein Fehler wenn durch Signal interrupted
					die("sigsuspend");
				}
				errno = errnoRestore;
			}
		}
	}
}

int main(void) {

	/* Signalbehandlungen einstellen */
	handleSig(SIGINT, SIG_BLOCK);
	handleSigchld();

	for (;;) {

		// print Zombie processes
		handleSig(SIGCHLD, SIG_BLOCK);
		plistHandleEvents(printZombies);
		handleSig(SIGCHLD, SIG_UNBLOCK);

		shPrompt();

		// Read command line
		char cmdLine[MAX_CMDLINE_LEN];
		if (fgets(cmdLine, sizeof(cmdLine), stdin) == NULL) {
			if (ferror(stdin) != 0)
				die("fgets");
			break;
		}

		// Parse command line
		ShCommand *cmd = shParseCmdLine(cmdLine);
		if (cmd == NULL) {
			perror("shParseCmdLine");
			continue;
		}
		
		// Execute command
		if (cmd->parseError != NULL) {
			if (strlen(cmdLine) > 0)
				fprintf(stderr, "Syntax error: %s\n", cmd->parseError);
		} else if (strcmp("cd", cmd->argv[0]) == 0) {
			changeCwd(cmd->argv);
		} else if (strcmp("jobs", cmd->argv[0]) == 0) {
			printJobs(cmd->argv);
		} else if (strcmp("nuke", cmd->argv[0]) == 0){
			nuke(cmd->argv);
		} else {
			execute(cmd);
		}
	}

	putchar('\n');
	return EXIT_SUCCESS;
}