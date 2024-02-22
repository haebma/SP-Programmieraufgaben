

# Zusammenfassung

Bitte achte genauer auf die Aufgabenstellung, und lese es ggf. auch
mehrfach durch (gilt allgemein, nicht nur für SP, oder Übungen,
sondern auch in der Klausur).  Es ist immer ärgerlich Punkte
abzuziehen, weil eine Abgabe von Anfang an in die falsche Richtung
hinarbeitet und sich damit alles erschwert. 

**Punktzahl:** 8 (Maximal: 14, Abzug: 6, Bonus: n.z.)

---


# Dateien


## Datei `mach.c`


### `make`: Misverständnisse und Komplikationen (-4)

    58          P(par->sem);
    59  
    60          if (fgets(par->cmd, 4097, par->machfile) == NULL){
    61              // EOF or error
    62              V(par->sem); // Vree, so that every thread can check on feof, queue_put(poisonpill) and break when work is done
    63              if (feof(par->machfile)){
    64                  if (queue_put(par->cmd, NULL, 4)){ // flag "4" means "work finished"
    65                      die("queue_put");
    66                  }
    67                  break;
    68              } else {
    69                  die("fgets");
    70              }
    71          }
    72  // TODO: does not work like this
    73          if (strcmp(par->cmd, "\n") == 0){ // new group of commands begins -> pause until all commands in current group are finished
    74              if (queue_put(par->cmd, NULL, 3)){ // flag "3" means "pause"
    75                  die("queue_put");
    76              }
    77              continue; // do not unlock the sem, but wait until output-thread unlocks it
    78          }
    79  
    80          V(par->sem);

Ich muss hier einige Fehler vergeben, weil einige Punkte in der
Aufgabenstellung missachtet wurden:

> Langsame Funktionen (z. B. `printf(3)`) dürfen **nicht** in kritischen Abschnitten ausgeführt werden

> Der Haupthread (`main()`) **liest die mach-Datei ein** und startet die
> benötigten Threads.

> **Pro Befehl** wird ein eigener detachter Arbeiterthread gestartet

Wenn man auf diese Punkte achtet, dann sieht man wie das Programm
grundsätzlich anders aufgebaut werden muss.  Daher ist auch die
Synchronisation von Gruppen so schwer in deiner Architektur.


### `make`: sizeof hätte hier auch funktioniert gehabt

    60          if (fgets(par->cmd, 4097, par->machfile) == NULL){

Da der Speicher für `par->cmd` teil von `struct param` ist, hätte man
hier auch `sizeof(par->cmd)` benutzen können.


### `make`: Zu literalen Zahlenwerten

    64                  if (queue_put(par->cmd, NULL, 4)){ // flag "4" means "work finished"
    65                      die("queue_put");
    66                  }

Anstatt jedes mal mit Kommentaren zu arbeiten, hätte man sich am
Anfang ein paar Makros definieren können um den Zahlenwerten
verständliche Namen zu geben:

    #define FLAG_RUNNING 1
    #define FLAG_COMPLETED 2
    #define FLAG_PAUSE 3
    #define FLAG_WORK_FINISHED 4


### `main`: Alternativ: `pthread_join`

    192      P(sem_end);
    193      semDestroy(sem_end);
    194      if (fclose(file)){
    195  	die("fclose");
    196      }

Da nur die arbeiter-Threads detached sind, hätte man hier auch auf
`output` direkt warten, und sich so eine Semaphore sparen können.


## Datei `mach_original.c`

Keine Anmerkungen.


## Datei `machfile`

Keine Anmerkungen.


## Datei `queue.c`


### Zu `errno`

    8  // TODO:
    9  //       - AUF WAS SOLL ERRNO GESETZT WERDEN?

Teilweise war das eure eigene Abwägung was in `errno.h` definiert ist
und ihr als angebracht findet.  Zum anderen Teil, konnte `errno` oft
&bdquo;vererbt&ldquo; werden, indem Funktionen wie `semCreate` oder `malloc`
bereits den Wert gesetzt haben.


### Zahlen in Variablennamen

    19  static SEM *sem1; // for adding and removing entries (one at a time)
    20  static SEM *sem2; // for removing values out of the queue; works as an element counter

Wenn man anfangen muss variablen aufzuzählen, braucht man in der Regel
ein Array.  Ansonsten wäre es besser aussagekräftige Namen zu
benutzen:

-   `sem1` &rarr; `critical_section`
-   `sem2` &rarr; `avaliable_elements`


### `queue_init`: Initialisation von der Queue

    50      head = (struct element*) malloc(sizeof(struct element));
    51      if (head == NULL){ // error in malloc
    52          semDestroy(sem1);
    53          semDestroy(sem2);
    54          errno = 3;
    55          return -1;
    56      }

Ich sehe nicht wieso das hier notwendig ist, später wird das so oder
so immer übersprungen:

    166      struct element *entry = head->next;

    178      head->next = entry->next;


### `queue_put`: Verwenden von `errno` (-1)

    113      errno = 0;
    114      if (cmd != NULL){
    115          entry->cmd = strdup(cmd);
    116      }
    117      if (out != NULL){
    118  	entry->out = strdup(out);
    119      }
    120      if (errno) { // Fehler in strdup
    121  	return -1;
    122      }

Man sollte sich allgemein nicht auf `errno` als *Fehlerindikator*
verlassen.  Ob es einen Fehler gab oder nicht wird meistens mit dem
Rückgabewert einer Funktion angedeutet.  Was der Fehler war
(*Fehlerart*) steht in `errno`, aber an sich darf eine Funktion immer
am Anfang `errno` auf einen Fehlerwert setzen auch wenn später nichts
passiert &#x2013; der Wert ist ja nur gültig wenn es dann auch einen Fehler
andeutet.

Ausnahmen sind Funktionen die explizit sagen, dass `errno` auch Fehler
andeutet, wie `strtol`.


### `queue_put`: Duplikation von `cmd` und `out` (-1)

    113      errno = 0;
    114      if (cmd != NULL){
    115          entry->cmd = strdup(cmd);
    116      }
    117      if (out != NULL){
    118  	entry->out = strdup(out);
    119      }
    120      if (errno) { // Fehler in strdup
    121  	return -1;
    122      }

In der `queue.h` Datei steht

    * The caller controls the lifespan of all pointers. The queue will not
    * duplicate or copy any arguments. It only stores the provided pointers.

d.h. es ist sowohl hier, wie auch weiter unten

    167      errno = 0;
    168      if(entry->cmd != NULL){
    169          *cmd = strdup(entry->cmd);
    170      }
    171      if(entry->out != NULL){
    172          *out = strdup(entry->out);
    173      }
    174      if(errno){ // Fehler in strdup
    175          return -1;

falsch den Speicher zu kopieren.  Vor allem weil beim zweiten mal, das
kopieren wegen dem direkt danach folgendem `free` überflüssig wird:

    179   if (entry->cmd != NULL) free(entry->cmd);


### `queue_put`: Sprachliche Anmerkung

    128      V(sem2); // notificate that element has been added

Ich glaube du willst hier &bdquo;notify&ldquo; schreiben.

---


# Compiler-Output


## Mit Notwendigen Flags

    /usr/bin/ld: /tmp/ccRuPClr.o: in function `make':
    mach.c:(.text+0x13a): undefined reference to `pthread_detach'
    /usr/bin/ld: mach.c:(.text+0x16a): undefined reference to `P'
    /usr/bin/ld: mach.c:(.text+0x19e): undefined reference to `V'
    /usr/bin/ld: mach.c:(.text+0x1cc): undefined reference to `queue_put'
    /usr/bin/ld: mach.c:(.text+0x227): undefined reference to `queue_put'
    /usr/bin/ld: mach.c:(.text+0x24f): undefined reference to `V'
    /usr/bin/ld: mach.c:(.text+0x286): undefined reference to `queue_put'
    /usr/bin/ld: mach.c:(.text+0x2ad): undefined reference to `run_cmd'
    /usr/bin/ld: mach.c:(.text+0x2c9): undefined reference to `queue_put'
    /usr/bin/ld: /tmp/ccRuPClr.o: in function `print':
    mach.c:(.text+0x318): undefined reference to `pthread_detach'
    /usr/bin/ld: mach.c:(.text+0x367): undefined reference to `queue_get'
    /usr/bin/ld: mach.c:(.text+0x3d6): undefined reference to `V'
    /usr/bin/ld: mach.c:(.text+0x44f): undefined reference to `semDestroy'
    /usr/bin/ld: mach.c:(.text+0x454): undefined reference to `queue_deinit'
    /usr/bin/ld: mach.c:(.text+0x464): undefined reference to `V'
    /usr/bin/ld: /tmp/ccRuPClr.o: in function `main':
    mach.c:(.text+0x518): undefined reference to `semCreate'
    /usr/bin/ld: mach.c:(.text+0x526): undefined reference to `semCreate'
    /usr/bin/ld: mach.c:(.text+0x675): undefined reference to `queue_init'
    /usr/bin/ld: mach.c:(.text+0x72a): undefined reference to `pthread_create'
    /usr/bin/ld: mach.c:(.text+0x805): undefined reference to `pthread_create'
    /usr/bin/ld: mach.c:(.text+0x82d): undefined reference to `P'
    /usr/bin/ld: mach.c:(.text+0x83c): undefined reference to `semDestroy'
    collect2: error: ld returned 1 exit status


## Mit Zusätzlichen Flags

    /usr/bin/ld: /tmp/cc9KT5S3.o: in function `make':
    /home/cip/2017/oj14ozun/Other/sp/mach-T05/he35nyky/mach.c:51: undefined reference to `pthread_detach'
    /usr/bin/ld: /home/cip/2017/oj14ozun/Other/sp/mach-T05/he35nyky/mach.c:58: undefined reference to `P'
    /usr/bin/ld: /home/cip/2017/oj14ozun/Other/sp/mach-T05/he35nyky/mach.c:62: undefined reference to `V'
    /usr/bin/ld: /home/cip/2017/oj14ozun/Other/sp/mach-T05/he35nyky/mach.c:64: undefined reference to `queue_put'
    /usr/bin/ld: /home/cip/2017/oj14ozun/Other/sp/mach-T05/he35nyky/mach.c:74: undefined reference to `queue_put'
    /usr/bin/ld: /home/cip/2017/oj14ozun/Other/sp/mach-T05/he35nyky/mach.c:80: undefined reference to `V'
    /usr/bin/ld: /home/cip/2017/oj14ozun/Other/sp/mach-T05/he35nyky/mach.c:83: undefined reference to `queue_put'
    /usr/bin/ld: /home/cip/2017/oj14ozun/Other/sp/mach-T05/he35nyky/mach.c:88: undefined reference to `run_cmd'
    /usr/bin/ld: /home/cip/2017/oj14ozun/Other/sp/mach-T05/he35nyky/mach.c:90: undefined reference to `queue_put'
    /usr/bin/ld: /tmp/cc9KT5S3.o: in function `print':
    /home/cip/2017/oj14ozun/Other/sp/mach-T05/he35nyky/mach.c:102: undefined reference to `pthread_detach'
    /usr/bin/ld: /home/cip/2017/oj14ozun/Other/sp/mach-T05/he35nyky/mach.c:115: undefined reference to `queue_get'
    /usr/bin/ld: /home/cip/2017/oj14ozun/Other/sp/mach-T05/he35nyky/mach.c:125: undefined reference to `V'
    /usr/bin/ld: /home/cip/2017/oj14ozun/Other/sp/mach-T05/he35nyky/mach.c:140: undefined reference to `semDestroy'
    /usr/bin/ld: /home/cip/2017/oj14ozun/Other/sp/mach-T05/he35nyky/mach.c:141: undefined reference to `queue_deinit'
    /usr/bin/ld: /home/cip/2017/oj14ozun/Other/sp/mach-T05/he35nyky/mach.c:142: undefined reference to `V'
    /usr/bin/ld: /tmp/cc9KT5S3.o: in function `main':
    /home/cip/2017/oj14ozun/Other/sp/mach-T05/he35nyky/mach.c:158: undefined reference to `semCreate'
    /usr/bin/ld: /home/cip/2017/oj14ozun/Other/sp/mach-T05/he35nyky/mach.c:159: undefined reference to `semCreate'
    /usr/bin/ld: /home/cip/2017/oj14ozun/Other/sp/mach-T05/he35nyky/mach.c:168: undefined reference to `queue_init'
    /usr/bin/ld: /home/cip/2017/oj14ozun/Other/sp/mach-T05/he35nyky/mach.c:176: undefined reference to `pthread_create'
    /usr/bin/ld: /home/cip/2017/oj14ozun/Other/sp/mach-T05/he35nyky/mach.c:187: undefined reference to `pthread_create'
    /usr/bin/ld: /home/cip/2017/oj14ozun/Other/sp/mach-T05/he35nyky/mach.c:192: undefined reference to `P'
    /usr/bin/ld: /home/cip/2017/oj14ozun/Other/sp/mach-T05/he35nyky/mach.c:193: undefined reference to `semDestroy'
    collect2: error: ld returned 1 exit status

