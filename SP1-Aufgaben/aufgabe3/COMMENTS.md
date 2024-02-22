

# Zusammenfassung

An einigen stellen scheint mir euer Code etwas komplizierter als es
wirklich notwendig ist, aber das ist ja nicht der primäre Fokus.
Alles in allem ist es eine gute Abgabe.  Bei Fragen oder Beschwerden
zur Korrektur, könnt ihr wie immer fragen oder euch beschweren.

**Punktzahl:** 11 (Maximal: 14, Abzug: 4, Bonus: 1)

---


# Dateien


## Datei `Makefile`


### `CFLAGS`: Zu Git flags

    4  CFLAGS = -std=c11 -pedantic -Wall -Werror -D_XOPEN_SOURCE=700

Ich sehe in der Git geschichte, ihr habt kurzzetig `-Wextra`
hinzugefügt gehabt, und dann wieder entfernt.  Von mir aus ist es kein
Problem wenn ihr die oder weitere Optionen bei den Abgaben dabei
lässt.


### `plist.o`: Fehlende Abhänigkeit (-0.5)

    20  plist.o: plist.c

Sollte sich die Schnittstelle ändern `plist.h`, will man auch das
Modul dahinter neu bauen lassen damit man sicher keine Diskrepanzen
zwischen den beiden hat.  Die Allgemeine regel ist, wenn man ein Datei
`foo.c` und will daraus `foo.o` bauen, sind alle Dateien welche in
`foo.c` mit

    #include "bar.h"

d.h., direkt lokal eingebunden werden auch Abhängigkeiten in der
Makefile.


## Datei `clash.c`


### Globale Variablen

    18  static int wordCounter;
    19  static char* cmd;

Ich sehe nicht wieso diese globalen Variablen hier angelegt werden
mussten.  Der Hauptgrund scheint die &bdquo;Kommunikation&ldquo; zwischen
`getArgs` und `main` zu sein, aber das hätte man ja auch mit pointern
erledigen können.


### `getAndPrintCWD`: Beliebige Obergrenze für CWD (-1)

    39  	while(getcwd(cwdBuffer, pathsize * i) == NULL){
    40  		if(errno == ERANGE){ // Arbeitsverzeichnis ist zu lang für unseren puffer
    41  			if(i > 10){ // manuell festgelegtes Ende -> cwd zu lang
    42  				fprintf(stderr, "Das Verzeichnis ist zu lang, um zu es speichern.\n");
    43  				exit(EXIT_FAILURE);
    44  			}

Ich versehe nicht wieso ihr $10 \cdot \mathtt{PATH\_MAX}$ als ein
&bdquo;manuell festgelegtes Ende&ldquo; ansehen wollt, und aus meiner Sicht ist es
vollkommen beliebig.  Wieso nicht gleich $10 \cdot \mathtt{PATH\_MAX}$
Speicher anfordern?  Die Absicht hinter dieser Schleife ist es eben
*kein* beliebiges Limit anzunehmen, sondern flexibel die verfügbaren
Ressourcen des Systems auszunutzen.

Außerdem wäre es eine nette Idee gewesen `pathsize` und `cwdBuffer`
mit `static` zu deklarieren, damit der angeforderte Speicher
wieder-benutzt werden kann und nicht bei jedem Prompt neu angefordert
wird.


### `getAndPrintCWD`: Mögliche Vereinfachung mit `fprintf`

    57  	if(fputs(cwdBuffer, stdout) == EOF){ // cwd und : ausgeben
    58  		die("fputs");
    59  	}
    60  	free(cwdBuffer);
    61  	if(fflush(stdout) == EOF){
    62  		die("fflush");
    63  	}
    64  	printf(": ");

Ihr habt die Aufgabenstellung sehr interessant interpretiert:

> Fehlerbehandlung für die Ausgabe ist bei dieser Aufgabe optional, da
> die Ausgabe einer interaktiven Shell nicht auf die Platte geschrieben
> wird.

indem `fputs` fehlerbehandelt wird, aber `printf` nicht.  Worauf ich
aber eigentlich hinaus will, ist das man nur ein `printf` Aufruf
gebraucht hätte.  Die Idee von [Formatted Output](https://www.gnu.org/software/libc/manual/html_node/Formatted-Output.html) ist es eine feste
&bdquo;Form&ldquo; von einer Ausgabe mit dynamischen Inhalt zu füllen (falls
hilfreich, kann es mit [f-string](https://docs.python.org/3/tutorial/inputoutput.html#formatted-string-literals) in Python vergleichen werden).  Daher
ist es eigentlich Sinnlos `printf` mit einem festen String aufzurufen,
da man dann auch gleich `fputs` benutzen kann, und den
Lauf-Zeit-Aufwand vermeidet den Format-String zu interpretieren &#x2013; das
ist allerdings meistens egal weil moderne Compiler in der Regel das
[Optimieren können](https://www.netmeister.org/blog/return-printf.html).

Also hätte man auch

    printf("%s: ", cwdBuffer);

schreiben können.


### `getArgs`: Speicherleck (-2)

    106  	cmd = strdup(cmdline);
    107  	if(cmd == NULL) die("strdup");
    108  	cmd[strlen(cmd) -1] = '\0'; // newline aus cmd entfernen
    109  
    110  	char** args = (char** ) malloc(2 * sizeof(char*)); // Speicher fuer die ersten 2 Argumente reserviert
    111  	if(args == NULL) die("malloc");
    112  	args[0] = strtok(cmdline, " \t\n"); // durch '\n' werden leere Zeilen direkt ignoriert und des macht ja eh keinen unterschied, weil des \n immer des ende einer eingabe ist
    113  	wordCounter = 1;
    114  	while((args[wordCounter] = strtok(NULL, " \t\n")) != NULL){
    115  		char** newArgs = realloc(args, sizeof(char*) * (++wordCounter+1)); // Speicher fuer ein weiteres Argument reservieren
    116  		if(newArgs == NULL){
    117  			die("realloc");
    118  		}
    119  		args = newArgs;
    120  	}

Der Speicherverbrauch eurer Shell ist proportional zu der Anzahl von
Befehlen welche Ausgeführt werden.  Später wird die Funktion ja nur
aufgerufen, aber dann nie aufgeräumt:

    162  

Es wäre jedoch möglich gewesen, am Ende von der Befehlsschleife den
Speicher und alle Token wieder Freizugeben.


### `main`: Tipp: `stdbool.h`

    133  	while(1){

Seit C99 gibt es ein Header namens `stdbool.h` in dem der Typ `bool`
und die Werte `true` wie auch `false` definiert werden, was hier
ggf. die Syntax lesbarer machen könnte.  (Traditionell bevorzugen
C/C++ Leute aber die `for (;;)` Syntax um &bdquo;forever&ldquo; schleifen zu
definieren).


### `main`: Setzen von `status`

    138  			int status = 0;
    139  			if((exitedChild = waitpid(-1, &status, WNOHANG)) == -1){ // WNOHANG -> wartet nicht bis Kindprozess terminiert

Streng genommen nicht notwendig, da man mit dem übergeben an `waitpid`
den Wert initialisiert, und bis dahin nur den verfügbaren Speicher
braucht.


### `main`: Newline vergessen (-0.5)

    150  				fprintf(stderr, "removeElement: a pair with the given pid does not exit");

Auch bei `stderr` mit `fprintf` muss man darauf achten, dass die
Ausgabe mit einem Newline endet.


### `main`: Lange Zeilen

    156  				printf("Exitstatus [%s] = %s\n", cmdLineBuffer, "child didn't exit properly"); // WIFEXITED == 0 -> kann nicht WEXITSTATUS benutzen 

Ich wäre dafür Dankbar, wenn man Zeilen auf 80-100 Zeichen beschränken
könnte.  Irgendwann ist die Informationsdichte zu groß, und man
verliert die Übersicht.  *Ich* würde auch Kommentare von dieser Art
vor der Anweisung schreiben, und nicht auf der gleichen Zeile.

(Bei mir kommt noch dazu, dass Tab-Zeichen (wie ihr ja einrückt)
standardmäßig eine Breite von 8 Zeichen haben was die Lesbarkeit nicht
verbessert.)


### `main`: Allgemein: Fehlerüberprüfung mit `errno`

    162  		char** args = getArgs();
    163  		if(args == NULL){
    164  			if(feof(stdin)){ // STRG + D betätigt
    165  				break;
    166  			}
    167  			else if(errno != 0){ // Fehler 
    168  				die("fgets");
    169  			}

Viele Funktionen setzen `errno` *wenn* es einen Fehler gab.  Der
Kehrsatz gilt aber nicht zwingend:  Nur weil `errno != 0`, heißt es
nicht das die letzte Funktion fehlerhaft ausgeführt wurde.


### `main`: Elegantere Behandlung von `&`

    198  		if(strcmp(args[0], "jobs") == 0 || strcmp(args[0], "jobs&") == 0){ // jobs -> print working backgrund processes
    199  			if(cmd[strlen(cmd)-1] == '&'){ // letztes Zeichen in cmd ist '"&" -> Hintergrundprozess
    200  				if(strcmp(args[wordCounter-1], "&") == 0){ // letztes Argument ist nur "&" -> letztes Token entfernen
    201  					args[wordCounter-1] = '\0';

Anstatt jedes mal mit `&` umzugehen, hätte man auch nach dem Einlesen
direkt schauen können ob das letzte Token mit einem `&` endet, und es
dann entfernen können.


### `main`: Visible Confusion

    234  					fprintf(stderr, "pair already existst");
    235  					errno = 0;

Was ist ein &bdquo;pair&ldquo;?  Und wieso setzt ihr hier `errno` auf 0?


## Datei `plist.c`


### `walkList`: 1-Step-Loop-Unroling

    27  	if(head == NULL)
    28  		return;
    29  	
    30  	struct qel *current = head;
    31  
    32  	while(current->next != NULL){
    33  		if(callback(current->pid, current->cmdLine) != 0) return;
    34  		current = current->next;
    35  	}
    36  
    37  	if(callback(current->pid, current->cmdLine) != 0) return;

Anstatt zu prüfen ob die Liste leer ist, um dann bis zu dem letzten
Element zu laufen, damit man noch sicher die `callback`-Funktion auf
dem letzten Element einer nicht-leeren Liste aufzurufen, hätte man
nicht einfach eine `while` schleife Bauen können welche prüft ob
`current` nicht NULL ist?

    while (current != NULL, callback(current->pid, current->cmdLine) == 0)
         current = current->next;

---


# Compiler-Output


## Mit Notwendigen Flags


## Mit Zusätzlichen Flags

    clash.c: In function ‘getAndPrintCWD’:
    clash.c:39:42: warning: conversion to ‘size_t’ {aka ‘long unsigned int’} from ‘int’ may change the sign of the result [-Wsign-conversion]
       39 |         while(getcwd(cwdBuffer, pathsize * i) == NULL){
          |                                          ^
    clash.c:45:71: warning: conversion to ‘size_t’ {aka ‘long unsigned int’} from ‘int’ may change the sign of the result [-Wsign-conversion]
       45 |                         char* newBuffer = realloc(cwdBuffer, PATH_MAX * (++i));
          |                                                                       ^
    clash.c: In function ‘getArgs’:
    clash.c:115:62: warning: conversion to ‘long unsigned int’ from ‘int’ may change the sign of the result [-Wsign-conversion]
      115 |                 char** newArgs = realloc(args, sizeof(char*) * (++wordCounter+1)); // Speicher fuer ein weiteres Argument reservieren
          |                                                              ^
    clash.c: In function ‘main’:
    clash.c:131:14: warning: unused parameter ‘argc’ [-Wunused-parameter]
      131 | int main(int argc, char* argv[]){
          |          ~~~~^~~~
    clash.c:131:26: warning: unused parameter ‘argv’ [-Wunused-parameter]
      131 | int main(int argc, char* argv[]){
          |                    ~~~~~~^~~~~~
    clash.c: In function ‘getArgs’:
    clash.c:110:33: warning: leak of ‘<unknown>’ [CWE-401] [-Wanalyzer-malloc-leak]
      110 |         char** args = (char** ) malloc(2 * sizeof(char*)); // Speicher fuer die ersten 2 Argumente reserviert
          |                                 ^~~~~~~~~~~~~~~~~~~~~~~~~
      ‘main’: events 1-2
        |
        |  131 | int main(int argc, char* argv[]){
        |      |     ^~~~
        |      |     |
        |      |     (1) entry to ‘main’
        |......
        |  139 |                         if((exitedChild = waitpid(-1, &status, WNOHANG)) == -1){ // WNOHANG -> wartet nicht bis Kindprozess terminiert
        |      |                           ~
        |      |                           |
        |      |                           (2) following ‘true’ branch (when ‘exitedChild == -1’)...
        |
      ‘main’: event 3
        |
        |  140 |                                 if(errno == ECHILD){ // keine Zombies gefunden
        |      |                                    ^~~~~
        |      |                                    |
        |      |                                    (3) ...to here
        |
      ‘main’: event 4
        |
        |  140 |                                 if(errno == ECHILD){ // keine Zombies gefunden
        |      |                                   ^
        |      |                                   |
        |      |                                   (4) following ‘true’ branch...
        |
      ‘main’: event 5
        |
        |  141 |                                         errno = 0;
        |      |                                         ^~~~~
        |      |                                         |
        |      |                                         (5) ...to here
        |
      ‘main’: event 6
        |
        |  160 |                 getAndPrintCWD();
        |      |                 ^~~~~~~~~~~~~~~~
        |      |                 |
        |      |                 (6) calling ‘getAndPrintCWD’ from ‘main’
        |
        +--> ‘getAndPrintCWD’: events 7-9
               |
               |   29 | static void getAndPrintCWD(){
               |      |             ^~~~~~~~~~~~~~
               |      |             |
               |      |             (7) entry to ‘getAndPrintCWD’
               |......
               |   32 |         if(cwdBuffer == NULL){
               |      |           ~  
               |      |           |
               |      |           (8) following ‘false’ branch (when ‘cwdBuffer’ is non-NULL)...
               |......
               |   36 |         size_t pathsize = PATH_MAX;
               |      |                ~~~~~~~~
               |      |                |
               |      |                (9) ...to here
               |
             ‘getAndPrintCWD’: events 10-15
               |
               |   39 |         while(getcwd(cwdBuffer, pathsize * i) == NULL){
               |      |                                               ^
               |      |                                               |
               |      |                                               (10) following ‘false’ branch...
               |......
               |   57 |         if(fputs(cwdBuffer, stdout) == EOF){ // cwd und : ausgeben
               |      |           ~~~~~~~~~~~~~~~~~~~~~~~~~            
               |      |           ||
               |      |           |(11) ...to here
               |      |           (12) following ‘false’ branch...
               |......
               |   60 |         free(cwdBuffer);
               |      |         ~~~~~~~~~~~~~~~                        
               |      |         |
               |      |         (13) ...to here
               |   61 |         if(fflush(stdout) == EOF){
               |      |           ~                                    
               |      |           |
               |      |           (14) following ‘false’ branch...
               |......
               |   64 |         printf(": ");
               |      |         ~~~~~~~~~~~~                           
               |      |         |
               |      |         (15) ...to here
               |
        <------+
        |
      ‘main’: events 16-17
        |
        |  160 |                 getAndPrintCWD();
        |      |                 ^~~~~~~~~~~~~~~~
        |      |                 |
        |      |                 (16) returning to ‘main’ from ‘getAndPrintCWD’
        |  161 | 
        |  162 |                 char** args = getArgs();
        |      |                               ~~~~~~~~~
        |      |                               |
        |      |                               (17) calling ‘getArgs’ from ‘main’
        |
        +--> ‘getArgs’: events 18-31
               |
               |   74 | static char** getArgs(){
               |      |               ^~~~~~~
               |      |               |
               |      |               (18) entry to ‘getArgs’
               |......
               |   77 |         if((cmdline = (char *) malloc(LENGTH)) == NULL){
               |      |           ~    
               |      |           |
               |      |           (19) following ‘false’ branch (when ‘cmdline’ is non-NULL)...
               |......
               |   81 |         cmdline = fgets(cmdline, LENGTH, stdin);
               |      |                   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
               |      |                   |
               |      |                   (20) ...to here
               |   82 |         if(cmdline == NULL){
               |      |           ~    
               |      |           |
               |      |           (21) following ‘false’ branch (when ‘cmdline’ is non-NULL)...
               |......
               |   87 |         char lastChar = cmdline[strlen(cmdline) -1];
               |      |                                 ~~~~~~~~~~~~~~~
               |      |                                 |
               |      |                                 (22) ...to here
               |......
               |  107 |         if(cmd == NULL) die("strdup");
               |      |           ~    
               |      |           |
               |      |           (23) following ‘false’ branch...
               |  108 |         cmd[strlen(cmd) -1] = '\0'; // newline aus cmd entfernen
               |      |            ~   
               |      |            |
               |      |            (24) ...to here
               |  109 | 
               |  110 |         char** args = (char** ) malloc(2 * sizeof(char*)); // Speicher fuer die ersten 2 Argumente reserviert
               |      |                                 ~~~~~~~~~~~~~~~~~~~~~~~~~
               |      |                                 |
               |      |                                 (25) allocated here
               |  111 |         if(args == NULL) die("malloc");
               |      |           ~    
               |      |           |
               |      |           (26) assuming ‘args’ is non-NULL
               |      |           (27) following ‘false’ branch (when ‘args’ is non-NULL)...
               |  112 |         args[0] = strtok(cmdline, " \t\n"); // durch '\n' werden leere Zeilen direkt ignoriert und des macht ja eh keinen unterschied, weil des \n immer des ende einer eingabe ist
               |      |                   ~~~~~~~~~~~~~~~~~~~~~~~~
               |      |                   |
               |      |                   (28) ...to here
               |......
               |  115 |                 char** newArgs = realloc(args, sizeof(char*) * (++wordCounter+1)); // Speicher fuer ein weiteres Argument reservieren
               |      |                                  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
               |      |                                  |
               |      |                                  (29) when ‘realloc’ succeeds, without moving buffer
               |  116 |                 if(newArgs == NULL){
               |      |                   ~
               |      |                   |
               |      |                   (30) following ‘false’ branch (when ‘newArgs’ is non-NULL)...
               |......
               |  119 |                 args = newArgs;
               |      |                 ~~~~~~~~~~~~~~
               |      |                      |
               |      |                      (31) ...to here
               |
        <------+
        |
      ‘main’: events 32-35
        |
        |  139 |                         if((exitedChild = waitpid(-1, &status, WNOHANG)) == -1){ // WNOHANG -> wartet nicht bis Kindprozess terminiert
        |      |                           ~    
        |      |                           |
        |      |                           (35) following ‘true’ branch (when ‘exitedChild == -1’)...
        |......
        |  162 |                 char** args = getArgs();
        |      |                               ^~~~~~~~~
        |      |                               |
        |      |                               (32) returning to ‘main’ from ‘getArgs’
        |  163 |                 if(args == NULL){
        |      |                   ~            
        |      |                   |
        |      |                   (33) following ‘false’ branch (when ‘args’ is non-NULL)...
        |......
        |  175 |                 if(args[0] == NULL || strcmp(args[0], "&")== 0) continue; // leere Zeilen oder Hintergrundprozess ohne Kommando
        |      |                    ~~~~~~~     
        |      |                        |
        |      |                        (34) ...to here
        |
      ‘main’: event 36
        |
        |  140 |                                 if(errno == ECHILD){ // keine Zombies gefunden
        |      |                                    ^~~~~
        |      |                                    |
        |      |                                    (36) ...to here
        |
      ‘main’: event 37
        |
        |  140 |                                 if(errno == ECHILD){ // keine Zombies gefunden
        |      |                                   ^
        |      |                                   |
        |      |                                   (37) following ‘true’ branch...
        |
      ‘main’: event 38
        |
        |  141 |                                         errno = 0;
        |      |                                         ^~~~~
        |      |                                         |
        |      |                                         (38) ...to here
        |
      ‘main’: event 39
        |
        |  160 |                 getAndPrintCWD();
        |      |                 ^~~~~~~~~~~~~~~~
        |      |                 |
        |      |                 (39) calling ‘getAndPrintCWD’ from ‘main’
        |
        +--> ‘getAndPrintCWD’: events 40-42
               |
               |   29 | static void getAndPrintCWD(){
               |      |             ^~~~~~~~~~~~~~
               |      |             |
               |      |             (40) entry to ‘getAndPrintCWD’
               |......
               |   32 |         if(cwdBuffer == NULL){
               |      |           ~  
               |      |           |
               |      |           (41) following ‘false’ branch (when ‘cwdBuffer’ is non-NULL)...
               |......
               |   36 |         size_t pathsize = PATH_MAX;
               |      |                ~~~~~~~~
               |      |                |
               |      |                (42) ...to here
               |
             ‘getAndPrintCWD’: events 43-48
               |
               |   39 |         while(getcwd(cwdBuffer, pathsize * i) == NULL){
               |      |                                               ^
               |      |                                               |
               |      |                                               (43) following ‘false’ branch...
               |......
               |   57 |         if(fputs(cwdBuffer, stdout) == EOF){ // cwd und : ausgeben
               |      |           ~~~~~~~~~~~~~~~~~~~~~~~~~            
               |      |           ||
               |      |           |(44) ...to here
               |      |           (45) following ‘false’ branch...
               |......
               |   60 |         free(cwdBuffer);
               |      |         ~~~~~~~~~~~~~~~                        
               |      |         |
               |      |         (46) ...to here
               |   61 |         if(fflush(stdout) == EOF){
               |      |           ~                                    
               |      |           |
               |      |           (47) following ‘false’ branch...
               |......
               |   64 |         printf(": ");
               |      |         ~~~~~~~~~~~~                           
               |      |         |
               |      |         (48) ...to here
               |
        <------+
        |
      ‘main’: events 49-50
        |
        |  160 |                 getAndPrintCWD();
        |      |                 ^~~~~~~~~~~~~~~~
        |      |                 |
        |      |                 (49) returning to ‘main’ from ‘getAndPrintCWD’
        |  161 | 
        |  162 |                 char** args = getArgs();
        |      |                               ~~~~~~~~~
        |      |                               |
        |      |                               (50) calling ‘getArgs’ from ‘main’
        |
        +--> ‘getArgs’: events 51-58
               |
               |   74 | static char** getArgs(){
               |      |               ^~~~~~~
               |      |               |
               |      |               (51) entry to ‘getArgs’
               |......
               |   77 |         if((cmdline = (char *) malloc(LENGTH)) == NULL){
               |      |           ~    
               |      |           |
               |      |           (52) following ‘false’ branch (when ‘cmdline’ is non-NULL)...
               |......
               |   81 |         cmdline = fgets(cmdline, LENGTH, stdin);
               |      |                   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
               |      |                   |
               |      |                   (53) ...to here
               |   82 |         if(cmdline == NULL){
               |      |           ~    
               |      |           |
               |      |           (54) following ‘false’ branch (when ‘cmdline’ is non-NULL)...
               |......
               |   87 |         char lastChar = cmdline[strlen(cmdline) -1];
               |      |                                 ~~~~~~~~~~~~~~~
               |      |                                 |
               |      |                                 (55) ...to here
               |......
               |  107 |         if(cmd == NULL) die("strdup");
               |      |           ~    
               |      |           |
               |      |           (56) following ‘false’ branch...
               |  108 |         cmd[strlen(cmd) -1] = '\0'; // newline aus cmd entfernen
               |      |            ~   
               |      |            |
               |      |            (57) ...to here
               |  109 | 
               |  110 |         char** args = (char** ) malloc(2 * sizeof(char*)); // Speicher fuer die ersten 2 Argumente reserviert
               |      |                                 ~~~~~~~~~~~~~~~~~~~~~~~~~
               |      |                                 |
               |      |                                 (58) ‘<unknown>’ leaks here; was allocated at (25)
               |
    clash.c:121:16: warning: leak of ‘cmdline’ [CWE-401] [-Wanalyzer-malloc-leak]
      121 |         return args;
          |                ^~~~
      ‘main’: events 1-2
        |
        |  131 | int main(int argc, char* argv[]){
        |      |     ^~~~
        |      |     |
        |      |     (1) entry to ‘main’
        |......
        |  139 |                         if((exitedChild = waitpid(-1, &status, WNOHANG)) == -1){ // WNOHANG -> wartet nicht bis Kindprozess terminiert
        |      |                           ~
        |      |                           |
        |      |                           (2) following ‘true’ branch (when ‘exitedChild == -1’)...
        |
      ‘main’: event 3
        |
        |  140 |                                 if(errno == ECHILD){ // keine Zombies gefunden
        |      |                                    ^~~~~
        |      |                                    |
        |      |                                    (3) ...to here
        |
      ‘main’: event 4
        |
        |  140 |                                 if(errno == ECHILD){ // keine Zombies gefunden
        |      |                                   ^
        |      |                                   |
        |      |                                   (4) following ‘true’ branch...
        |
      ‘main’: event 5
        |
        |  141 |                                         errno = 0;
        |      |                                         ^~~~~
        |      |                                         |
        |      |                                         (5) ...to here
        |
      ‘main’: event 6
        |
        |  160 |                 getAndPrintCWD();
        |      |                 ^~~~~~~~~~~~~~~~
        |      |                 |
        |      |                 (6) calling ‘getAndPrintCWD’ from ‘main’
        |
        +--> ‘getAndPrintCWD’: events 7-9
               |
               |   29 | static void getAndPrintCWD(){
               |      |             ^~~~~~~~~~~~~~
               |      |             |
               |      |             (7) entry to ‘getAndPrintCWD’
               |......
               |   32 |         if(cwdBuffer == NULL){
               |      |           ~  
               |      |           |
               |      |           (8) following ‘false’ branch (when ‘cwdBuffer’ is non-NULL)...
               |......
               |   36 |         size_t pathsize = PATH_MAX;
               |      |                ~~~~~~~~
               |      |                |
               |      |                (9) ...to here
               |
             ‘getAndPrintCWD’: events 10-15
               |
               |   39 |         while(getcwd(cwdBuffer, pathsize * i) == NULL){
               |      |                                               ^
               |      |                                               |
               |      |                                               (10) following ‘false’ branch...
               |......
               |   57 |         if(fputs(cwdBuffer, stdout) == EOF){ // cwd und : ausgeben
               |      |           ~~~~~~~~~~~~~~~~~~~~~~~~~            
               |      |           ||
               |      |           |(11) ...to here
               |      |           (12) following ‘false’ branch...
               |......
               |   60 |         free(cwdBuffer);
               |      |         ~~~~~~~~~~~~~~~                        
               |      |         |
               |      |         (13) ...to here
               |   61 |         if(fflush(stdout) == EOF){
               |      |           ~                                    
               |      |           |
               |      |           (14) following ‘false’ branch...
               |......
               |   64 |         printf(": ");
               |      |         ~~~~~~~~~~~~                           
               |      |         |
               |      |         (15) ...to here
               |
        <------+
        |
      ‘main’: events 16-17
        |
        |  160 |                 getAndPrintCWD();
        |      |                 ^~~~~~~~~~~~~~~~
        |      |                 |
        |      |                 (16) returning to ‘main’ from ‘getAndPrintCWD’
        |  161 | 
        |  162 |                 char** args = getArgs();
        |      |                               ~~~~~~~~~
        |      |                               |
        |      |                               (17) calling ‘getArgs’ from ‘main’
        |
        +--> ‘getArgs’: events 18-28
               |
               |   74 | static char** getArgs(){
               |      |               ^~~~~~~
               |      |               |
               |      |               (18) entry to ‘getArgs’
               |......
               |   77 |         if((cmdline = (char *) malloc(LENGTH)) == NULL){
               |      |           ~                    ~~~~~~~~~~~~~~
               |      |           |                    |
               |      |           |                    (19) allocated here
               |      |           (20) assuming ‘cmdline’ is non-NULL
               |      |           (21) following ‘false’ branch (when ‘cmdline’ is non-NULL)...
               |......
               |   81 |         cmdline = fgets(cmdline, LENGTH, stdin);
               |      |                   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
               |      |                   |
               |      |                   (22) ...to here
               |   82 |         if(cmdline == NULL){
               |      |           ~    
               |      |           |
               |      |           (23) following ‘false’ branch (when ‘cmdline’ is non-NULL)...
               |......
               |   87 |         char lastChar = cmdline[strlen(cmdline) -1];
               |      |                                 ~~~~~~~~~~~~~~~
               |      |                                 |
               |      |                                 (24) ...to here
               |......
               |  107 |         if(cmd == NULL) die("strdup");
               |      |           ~    
               |      |           |
               |      |           (25) following ‘false’ branch...
               |  108 |         cmd[strlen(cmd) -1] = '\0'; // newline aus cmd entfernen
               |      |            ~   
               |      |            |
               |      |            (26) ...to here
               |......
               |  111 |         if(args == NULL) die("malloc");
               |      |           ~    
               |      |           |
               |      |           (27) following ‘false’ branch (when ‘args’ is non-NULL)...
               |  112 |         args[0] = strtok(cmdline, " \t\n"); // durch '\n' werden leere Zeilen direkt ignoriert und des macht ja eh keinen unterschied, weil des \n immer des ende einer eingabe ist
               |      |                   ~~~~~~~~~~~~~~~~~~~~~~~~
               |      |                   |
               |      |                   (28) ...to here
               |
             ‘getArgs’: events 29-31
               |
               |  114 |         while((args[wordCounter] = strtok(NULL, " \t\n")) != NULL){
               |      |                                                           ^
               |      |                                                           |
               |      |                                                           (29) following ‘false’ branch...
               |......
               |  121 |         return args;
               |      |                ~~~~                                        
               |      |                |
               |      |                (30) ...to here
               |      |                (31) ‘cmdline’ leaks here; was allocated at (19)
               |
    clash.c: In function ‘main’:
    clash.c:162:31: warning: leak of ‘args’ [CWE-401] [-Wanalyzer-malloc-leak]
      162 |                 char** args = getArgs();
          |                               ^~~~~~~~~
      ‘main’: events 1-2
        |
        |  131 | int main(int argc, char* argv[]){
        |      |     ^~~~
        |      |     |
        |      |     (1) entry to ‘main’
        |......
        |  139 |                         if((exitedChild = waitpid(-1, &status, WNOHANG)) == -1){ // WNOHANG -> wartet nicht bis Kindprozess terminiert
        |      |                           ~
        |      |                           |
        |      |                           (2) following ‘true’ branch (when ‘exitedChild == -1’)...
        |
      ‘main’: event 3
        |
        |  140 |                                 if(errno == ECHILD){ // keine Zombies gefunden
        |      |                                    ^~~~~
        |      |                                    |
        |      |                                    (3) ...to here
        |
      ‘main’: event 4
        |
        |  140 |                                 if(errno == ECHILD){ // keine Zombies gefunden
        |      |                                   ^
        |      |                                   |
        |      |                                   (4) following ‘true’ branch...
        |
      ‘main’: event 5
        |
        |  141 |                                         errno = 0;
        |      |                                         ^~~~~
        |      |                                         |
        |      |                                         (5) ...to here
        |
      ‘main’: event 6
        |
        |  160 |                 getAndPrintCWD();
        |      |                 ^~~~~~~~~~~~~~~~
        |      |                 |
        |      |                 (6) calling ‘getAndPrintCWD’ from ‘main’
        |
        +--> ‘getAndPrintCWD’: events 7-9
               |
               |   29 | static void getAndPrintCWD(){
               |      |             ^~~~~~~~~~~~~~
               |      |             |
               |      |             (7) entry to ‘getAndPrintCWD’
               |......
               |   32 |         if(cwdBuffer == NULL){
               |      |           ~  
               |      |           |
               |      |           (8) following ‘false’ branch (when ‘cwdBuffer’ is non-NULL)...
               |......
               |   36 |         size_t pathsize = PATH_MAX;
               |      |                ~~~~~~~~
               |      |                |
               |      |                (9) ...to here
               |
             ‘getAndPrintCWD’: events 10-15
               |
               |   39 |         while(getcwd(cwdBuffer, pathsize * i) == NULL){
               |      |                                               ^
               |      |                                               |
               |      |                                               (10) following ‘false’ branch...
               |......
               |   57 |         if(fputs(cwdBuffer, stdout) == EOF){ // cwd und : ausgeben
               |      |           ~~~~~~~~~~~~~~~~~~~~~~~~~            
               |      |           ||
               |      |           |(11) ...to here
               |      |           (12) following ‘false’ branch...
               |......
               |   60 |         free(cwdBuffer);
               |      |         ~~~~~~~~~~~~~~~                        
               |      |         |
               |      |         (13) ...to here
               |   61 |         if(fflush(stdout) == EOF){
               |      |           ~                                    
               |      |           |
               |      |           (14) following ‘false’ branch...
               |......
               |   64 |         printf(": ");
               |      |         ~~~~~~~~~~~~                           
               |      |         |
               |      |         (15) ...to here
               |
        <------+
        |
      ‘main’: events 16-17
        |
        |  160 |                 getAndPrintCWD();
        |      |                 ^~~~~~~~~~~~~~~~
        |      |                 |
        |      |                 (16) returning to ‘main’ from ‘getAndPrintCWD’
        |  161 | 
        |  162 |                 char** args = getArgs();
        |      |                               ~~~~~~~~~
        |      |                               |
        |      |                               (17) calling ‘getArgs’ from ‘main’
        |
        +--> ‘getArgs’: events 18-28
               |
               |   74 | static char** getArgs(){
               |      |               ^~~~~~~
               |      |               |
               |      |               (18) entry to ‘getArgs’
               |......
               |   77 |         if((cmdline = (char *) malloc(LENGTH)) == NULL){
               |      |           ~    
               |      |           |
               |      |           (19) following ‘false’ branch (when ‘cmdline’ is non-NULL)...
               |......
               |   81 |         cmdline = fgets(cmdline, LENGTH, stdin);
               |      |                   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
               |      |                   |
               |      |                   (20) ...to here
               |   82 |         if(cmdline == NULL){
               |      |           ~    
               |      |           |
               |      |           (21) following ‘false’ branch (when ‘cmdline’ is non-NULL)...
               |......
               |   87 |         char lastChar = cmdline[strlen(cmdline) -1];
               |      |                                 ~~~~~~~~~~~~~~~
               |      |                                 |
               |      |                                 (22) ...to here
               |......
               |  107 |         if(cmd == NULL) die("strdup");
               |      |           ~    
               |      |           |
               |      |           (23) following ‘false’ branch...
               |  108 |         cmd[strlen(cmd) -1] = '\0'; // newline aus cmd entfernen
               |      |            ~   
               |      |            |
               |      |            (24) ...to here
               |  109 | 
               |  110 |         char** args = (char** ) malloc(2 * sizeof(char*)); // Speicher fuer die ersten 2 Argumente reserviert
               |      |                                 ~~~~~~~~~~~~~~~~~~~~~~~~~
               |      |                                 |
               |      |                                 (25) allocated here
               |  111 |         if(args == NULL) die("malloc");
               |      |           ~    
               |      |           |
               |      |           (26) assuming ‘args’ is non-NULL
               |      |           (27) following ‘false’ branch (when ‘args’ is non-NULL)...
               |  112 |         args[0] = strtok(cmdline, " \t\n"); // durch '\n' werden leere Zeilen direkt ignoriert und des macht ja eh keinen unterschied, weil des \n immer des ende einer eingabe ist
               |      |                   ~~~~~~~~~~~~~~~~~~~~~~~~
               |      |                   |
               |      |                   (28) ...to here
               |
             ‘getArgs’: events 29-30
               |
               |  114 |         while((args[wordCounter] = strtok(NULL, " \t\n")) != NULL){
               |      |                                                           ^
               |      |                                                           |
               |      |                                                           (29) following ‘false’ branch...
               |......
               |  121 |         return args;
               |      |                ~~~~                                        
               |      |                |
               |      |                (30) ...to here
               |
        <------+
        |
      ‘main’: events 31-36
        |
        |  139 |                         if((exitedChild = waitpid(-1, &status, WNOHANG)) == -1){ // WNOHANG -> wartet nicht bis Kindprozess terminiert
        |      |                           ~    
        |      |                           |
        |      |                           (36) following ‘true’ branch (when ‘exitedChild == -1’)...
        |......
        |  162 |                 char** args = getArgs();
        |      |                               ^~~~~~~~~
        |      |                               |
        |      |                               (31) returning to ‘main’ from ‘getArgs’
        |  163 |                 if(args == NULL){
        |      |                   ~            
        |      |                   |
        |      |                   (32) following ‘false’ branch (when ‘args’ is non-NULL)...
        |......
        |  175 |                 if(args[0] == NULL || strcmp(args[0], "&")== 0) continue; // leere Zeilen oder Hintergrundprozess ohne Kommando
        |      |                   ~~~~~~~~     
        |      |                   |    |
        |      |                   |    (33) ...to here
        |      |                   (34) following ‘true’ branch...
        |      |                   (35) ...to here
        |
      ‘main’: event 37
        |
        |  140 |                                 if(errno == ECHILD){ // keine Zombies gefunden
        |      |                                    ^~~~~
        |      |                                    |
        |      |                                    (37) ...to here
        |
      ‘main’: event 38
        |
        |  140 |                                 if(errno == ECHILD){ // keine Zombies gefunden
        |      |                                   ^
        |      |                                   |
        |      |                                   (38) following ‘true’ branch...
        |
      ‘main’: event 39
        |
        |  141 |                                         errno = 0;
        |      |                                         ^~~~~
        |      |                                         |
        |      |                                         (39) ...to here
        |
      ‘main’: event 40
        |
        |  160 |                 getAndPrintCWD();
        |      |                 ^~~~~~~~~~~~~~~~
        |      |                 |
        |      |                 (40) calling ‘getAndPrintCWD’ from ‘main’
        |
        +--> ‘getAndPrintCWD’: events 41-43
               |
               |   29 | static void getAndPrintCWD(){
               |      |             ^~~~~~~~~~~~~~
               |      |             |
               |      |             (41) entry to ‘getAndPrintCWD’
               |......
               |   32 |         if(cwdBuffer == NULL){
               |      |           ~  
               |      |           |
               |      |           (42) following ‘false’ branch (when ‘cwdBuffer’ is non-NULL)...
               |......
               |   36 |         size_t pathsize = PATH_MAX;
               |      |                ~~~~~~~~
               |      |                |
               |      |                (43) ...to here
               |
             ‘getAndPrintCWD’: events 44-49
               |
               |   39 |         while(getcwd(cwdBuffer, pathsize * i) == NULL){
               |      |                                               ^
               |      |                                               |
               |      |                                               (44) following ‘false’ branch...
               |......
               |   57 |         if(fputs(cwdBuffer, stdout) == EOF){ // cwd und : ausgeben
               |      |           ~~~~~~~~~~~~~~~~~~~~~~~~~            
               |      |           ||
               |      |           |(45) ...to here
               |      |           (46) following ‘false’ branch...
               |......
               |   60 |         free(cwdBuffer);
               |      |         ~~~~~~~~~~~~~~~                        
               |      |         |
               |      |         (47) ...to here
               |   61 |         if(fflush(stdout) == EOF){
               |      |           ~                                    
               |      |           |
               |      |           (48) following ‘false’ branch...
               |......
               |   64 |         printf(": ");
               |      |         ~~~~~~~~~~~~                           
               |      |         |
               |      |         (49) ...to here
               |
        <------+
        |
      ‘main’: events 50-51
        |
        |  160 |                 getAndPrintCWD();
        |      |                 ^~~~~~~~~~~~~~~~
        |      |                 |
        |      |                 (50) returning to ‘main’ from ‘getAndPrintCWD’
        |  161 | 
        |  162 |                 char** args = getArgs();
        |      |                               ~~~~~~~~~
        |      |                               |
        |      |                               (51) ‘args’ leaks here; was allocated at (25)
        |

