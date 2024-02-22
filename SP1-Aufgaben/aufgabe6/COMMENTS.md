

# Zusammenfassung

Leider mehr Fehlerpunkte als mir lieb ist, ich nehme an weil ihr nicht
die Zeit hattet um alles anständig zu testen und ausprobieren?
Sollten es noch Fragen oder Beschwerden zur Korrektur geben, meldet
euch bei mir.

**Punktzahl:** 3 (Maximal: 8, Abzug: 6, Bonus: 1)

---


# Dateien


## Datei `Makefile`


### `clean`: Ignorieren von Fehlern

    8  	-rm -f creeper
    9  	-rm -f creeper.o

Wenn man `rm` bereits `-f` übergibt, braucht man nicht `make` sagen
die Fehler zu ignorieren, da immer ein &bdquo;erfolgreicher&ldquo; Rückgabewert
zurückgegeben wird.  Aber auch wenn das nicht so wäre, könnte man alle
Dateien in einem Befehl löschen, dann müsste man nicht auf non-POSIX
erweiterungen setzen (falls das einem wichtig sein sollte).


### `creeper`: `CFLAGS` beim Binden

    11  creeper: creeper.o argumentParser.o
    12  	gcc $(CFLAGS) -o creeper creeper.o argumentParser.o

Eigentlich nicht notwendig, da alle Optionen in `CFLAGS` nur für das
Übersetzen relevant sind:

-   **`-std=c11`:** Festlegen des C Standards
-   **`-pedantic`:** Ausstellen von GNU-C-Erweiterungen
-   **`-Wall`, `-Werror`, `-Wextra`:** Warnungen für C code
-   **`-D_XOPEN_SOURCE=700`:** Präprozessoranweisung


## Datei `creeper.c`


### `creeper`: Zu harte Fehlerbehandlung (-0.5)

    35      DIR *dir = opendir(path);
    36      if(dir == NULL){
    37          die("opendir");
    38      }

Es gibt durchaus übliche Gründe wieso man ein Verzeichnis nicht lesen
kann, bspw. weil einem die Rechte fehlen.  Diese Probleme sollten
nicht als &bdquo;Fatal&ldquo; gewertet werden, weil es keinen weiteren Einfluss
auf die Funktionalität des Programms haben muss (das nächste
Verzeichnis könnte ja funktionieren).


### `creeper`: Fehlerbehandlung von `printf` (-1.5)

    56  	    if(print == true){
    57  	    	printf("%s\n", d_name);
    58  	    }

Hier, und an anderen stellen,

    68  	    if(strcmp(type, "f") != 0 && print == true){ // check, ob nur files ausgegeben werden
    69  	            printf("%s/%s (directory)\n", path, d_name);

    75  		if(strcmp(type, "d")!= 0 && print == true){ // check, ob nur directories ausgegeben werden
    76  		     printf("%s/%s\n", path, d_name); 

    123  		if(name == NULL) printf("%s\n", path[i]);
    124  		else{
    125  			int check = fnmatch(name, basename(path[i]), FNM_PERIOD | FNM_PATHNAME);
    126  			if(check == 0) printf("%s\n", path[i]);
    127  			else if(check != FNM_NOMATCH && check != 0) die("fnmatch");

sollte man überprüfen ob `printf` einen Fehler andeutet, und dann eine
entsprechende Fehlermeldung ausgeben, weil die Ausgabe die
Kernfunktionalität des Programms ist.


### `creeper`: Ungewüschte Ausgabe (-0.5)

    68  	    if(strcmp(type, "f") != 0 && print == true){ // check, ob nur files ausgegeben werden
    69  	            printf("%s/%s (directory)\n", path, d_name);
    70  	    }

Es kann sein, dass das `(directory)` nur für debugging Zwecke noch
drin geblieben ist, aber das ist so nicht erwünscht.  Die `creeper`
(und insbesondere das Programm von dem die `creeper` inspiriert wurde,
`find`) ist primär für die Ausgabe auf dem Standard-Output
interessant, welche auch von anderen Programmen weiter verarbeitbar
sein sollte.  Da ist so eine Annotation (welche immer da ist, und
nicht *opt-in* ist wie mit `ls -F`) problematisch, da es die
Verarbeitung erschwert.


### `creeper`: Tiefensuche funktioniert nicht (-2)

    71              //TODO: richtigen pfad übergeben, sonst verlieren wir dirname
    72              creeper(d_name, maxdepth-1, name, type);

Wie ihr hier schon richtig erkennt, müsste man den gesamten Pfad
übergeben damit `opendir` in dem Unteraufruf auch eine Sinnvolle
Eingabe bekommen hat.  Ansonsten schlägt das Programm immer nach einer
Verziechnisstiefe fehl (insofern Dateinamen im Ober- und
Unterverzeichnis nicht doppelt vorkommen).

Ansonsten müsste man noch überprüfen ob die Datei wirklich ein
Verzeichnis ist, bevor es an `creeper` weitergegeben wird, weil
ansonsten das Program gleich mit einer `opendir`-Fehlermeldung
abstützt.


### `main`: Usage-Strings

    92          fprintf(stderr, "Usage: ./creeper-ref path... [-maxdepth=n] [-name=pattern] [-type={d,f}]\n");

Es wäre hier schön, anstatt `creeper-ref` fest zu kodieren (was nach
eurer `Makefile` nicht mal das Produkt ist), `argv[0]` auszugeben.


### `main`: Nicht robustest Parsen von `maxdepth` (-2)

    103  	    rektiefe = strtol(maxdepth, NULL, 10); // rekursionstiefe in long parsen
    104  	    if(rektiefe == 0 && errno) die("strtol");

Euer Aufruf hier ist im wesentlichen äquivalent zu `atoi`, einer
*verbotenen Funktion* in SP.  Eure Fehlerbehandlung kann verschiedene
Randfälle, beim interpretieren einer Zeichenkette als Zahl nicht
anständig abfangen:

-   Leere Eingaben
-   Halb-zahlen (`10foo`)
-   Negative Zahlen


### `main`: &bdquo;Everything&ldquo; is nicht everything (-1)

    111      if(name == NULL){
    112      	name = "everything"; // alles soll ausgegeben werden
    113      }

Leider funktioniert das nicht so, wenn kein `-name` angegeben wird,
sichert ihr mit diesem Fall, dass nur Dateien mit dem Namen
&bdquo;everything&ldquo; auch ausgegeben werden, was hoffe ich klar ist, nicht die
Absicht ist.


### `main`: Notwendigkeit alle Argumente zu kopieren?

    116      char** path = (char**) malloc(argsCounter * 1000);
    117      if(path == NULL) die("malloc");
    118  
    119      for(int i = 0; i < argsCounter; i++){
    120          path[i] = getArgument(i);
    121          if(path[i] == NULL) continue;

Mir nicht eure Absicht hier nicht ganz klar.  ihr greift hier jeweils
immer nur auf das jeweilige `path[i]` zu, und danach nicht mehr.
Hättet ihr nicht auch am Anfang von der Schleife einmal `path`
zwischenspeichern können, ohne den Mehraufwand mit der
Freispeicherverwaltung (und Fehlerbehandlung) auf euch zu nehmen?

---


# Compiler-Output


## Mit Notwendigen Flags


## Mit Zusätzlichen Flags

    creeper.c: In function ‘main’:
    creeper.c:116:47: warning: conversion to ‘size_t’ {aka ‘long unsigned int’} from ‘int’ may change the sign of the result [-Wsign-conversion]
      116 |     char** path = (char**) malloc(argsCounter * 1000);
          |                                   ~~~~~~~~~~~~^~~~~~

