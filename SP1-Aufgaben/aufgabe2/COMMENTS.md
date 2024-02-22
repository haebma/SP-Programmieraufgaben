

# Zusammenfassung

Schöne Abgabe, aber ihr seit am ende doch auf ein paar Fallen
hineingetappt.  Ich hoffe meine Erklärungen können euch helfen zu
verstehen was die Probleme waren, so dass ihr diese Perspektive und
Denkweise bei den nächsten Aufgaben effektiv selbst nutzen könnt.

Sollte es noch Fragen oder Unstimmigkeiten mit der Korrektur geben,
schreibt mir eine Email oder fragt in der RÜ/TÜ nacht.

**Punktzahl:** 11 (Maximal: 12, Abzug: 2, Bonus: 1)

---


# Dateien


## Datei `wsort.c`


### `LENGTH`: Konstanten und Ausdrucksstärke

    6  #define LENGTH 102 // max string length = 100 + '/n/0'

Was man genre mal macht, ist es Konstanten dieser Art aufzuteilen um
die Absicht hinter einem Wert besser zu kommunizieren.   Bspw. hätte
man in diesem Fall auch

    #define LENGTH (100 + 1 + 1) // max string length = 100 + '/n' + '/0'

schreiben können (was ein Compiler eh zu 102 optimieren wird).  Die
Klammern würde ich allgemein bei Makros empfehlen, weil damit Probleme
mit der Bindungsstärke von anderen Operatoren vermieden werden.


### `main`: Ungenutzte Argumente

    28  int main(int argc, char **argv){

Es ist zwar kein Fehler an sich, aber allgemein sollte man in C
entweder die Argumentliste leer lassen wenn man keine der Argumente
benutzen will:

    int main() {
         /* ... */

oder explizit angeben, dass man die ungenutzten Argumente nicht
verwenden will.  Das macht man entweder mit [variable attributes](gcc-12#Common Variable Attributes), oder
&bdquo;händisch&ldquo; mit:

    (void) argc;
    (void) argv;

was die Variablen zwar &bdquo;benutzt&ldquo;, aber die Werte gleich verwirft.


### `main`: Überflüssiges `malloc`

    35  	char *word_buf = (char *) malloc(LENGTH);
    36  	if (word_buf == NULL){
    37  		die("malloc");
    38  	}

Wie in der TÜ besprochen, ist hier `malloc` nicht notwendig.  Wenn ihr

    char word_buf[LENGTH];

schreibt, ist dieser Speicher für den Rest des Programmablaufs gültig,
weil man die `main` Funktion nicht verlassen kann, ohne das Programm
zu beenden.  Damit würde man sich die Fehlerbehandlung und unten das
`free` sparen.


### `main`: Rückgabetyp von `fgetc` (-1)

    71  				char next = fgetc(stdin); // next mallocen??
    72  				if(next == EOF){

Aufpassen, schaut euch den Prototypen von `fgetc` an:

    int getc(FILE *stream);

Ihr weist jedoch den Rückgabewert in ein `char`, womit Daten verloren
gehen!  Der Hintergrund ist, dass `fgetc` alle möglichen Zeichen
zurückgeben kann (man kann ja alle möglichen Zeichen schreiben oder in
einer Datei stehen haben), und daher kein Wert in `char` zur Verfügung
steht um Fehler anzudeuten.  Daher wird ein &bdquo;größerer&ldquo; Typ genommen (`int`),
in dem nun auch ein Wert enthalten ist, welcher nicht in `char` liegt,
d.h. EOF, as eigentlich nur -1 ist wie man in `stdio.h` sieht:

    /* The value returned by fgetc and similar functions to indicate the
       end of the file.  */
    #define EOF (-1)

Also in Zweier-Komplement-Darstellung (angenommen 32-bit `int` Werte):

    11111111 11111111 11111111 11111111

wenn wir das aber auch ein `char` herunter-casten (in C ja einfach so
möglich weil wir ein statisches, aber [schwaches Typ System](https://en.wikipedia.org/wiki/Strong_and_weak_typing) haben),
wird der Wert zu

    11111111

reduziert, was jetzt mit dem ASCII-Wert 0xFF nicht mehr unterschieden
Werden kann.  Wenn wir jedoch ein `char` (kleiner) mit einem `int` (größer)
direkt vergleichen, wird der kleinere Wert zu dem größeren vergrößert,
wo dann der Unterschied zu erkennen ist:

    11111111 11111111 11111111 11111111
                     ≠
    00000000 00000000 00000000 11111111


### `main`: Leading Newline?

    76  						if(fprintf(stderr, "\nWort ist zu lang - wird ignoriert\n") < 0) die("fprintf");

Wozu beginnt die Nachricht hier mit einem newline Zeichen?  Das ist
etwas ungewöhlich.  Interaktiv mag das ggf. etwas besser aussehen,
aber wenn die Eingabe nicht angezeigt wird hat man ein sporadisches
Newline auftauchen.


### `main`: Erweiterungsstrategie

    88  		if(wordCounter % 20 == 0 && wordCounter != 0){
    89  			char **wordsNew = realloc(words, (wordCounter + 20) * sizeof(char*)); // alle 20 zeilen mehr speicherplatz allokieren
    90  			if(wordsNew == NULL){
    91  				die("realloc");
    92  			}
    93  
    94  			words = wordsNew;
    95  		}

Indem ihr alle 20 Zeilen `realloc` aufruft, ist das Programm etwas
langsam bei den großeren Dateien (Man könnte schauen ob das wirklich
das Problem ist mit einem Werkzeug wie [gprof](https://sourceware.org/binutils/docs/gprof/)).  Was man hier
üblicherweise macht, ist es den Speicher mindestens &bdquo;Polynomisch&ldquo; zu
vergrößern.  Eine populäre Wahl ist ein &bdquo;mild-exponentieller&ldquo; Ansatz,
d.h. in O(1.5^n), realloc aufruzrufen, da man bei immer länger
werdenden Dateien die Wahrscheinlichkeit auch immer weiter sinkt, dass
diese in 20 (oder ein beliebiges, festes n) Zeilen enden wird.


### `main`: Newlines werden mitsortiert (-1)

    97  		char* word_to_insert = strdup(word_buf); // allokiert Speicher fuer Wort && dupliziert
    98  		if(word_to_insert == NULL) die("strdup");

Laut der Angabe, werden Newline Zeichen nicht mit zu &bdquo;Wörtern&ldquo; gezählt
(was hier sortiert wird):

> Ein Wort umfasst **alle** Zeichen einer Zeile. Zeilen sind durch ein
> Zeilenumbruch-Zeichen (`\n`) voneinander getrennt, das selbst nicht Teil
> des Wortes ist.

D.h. man muss sicherstellen, dass diese nicht an `qsort` bzw. `strcmp`
weitergegeben werden, da die Anwesenheit von Newline Zeichen die
Ausgabe beeinflussen kann.  Es wäre stattdessen notwendig gewesen
diese Abzuschneiden, bspw.

    char *eol = strchr(word_buf);
    if (NULL != eol) *eol = '\0';

und dann bei der Ausgabe wieder anzufügen.  Weil ihr das nicht macht,
konntet ihr `fputs` benutzen:

    115  	for(int i = 0; i < wordCounter; i++){
    116  		if(fputs(words[i], stdout) < 0) die("fputs");
    117  		free(words[i]);

aber werdet Probleme haben wenn die letzte Zeile in der Eingabe nicht
auch die letzte Zeile in der Ausgabe bleibt.

---


# Compiler-Output


## Mit Notwendigen Flags


## Mit Zusätzlichen Flags

    /home/philip/Documents/uni/spt/subm/wsort-T05/er04yjek/wsort.c: In function ‘main’:
    /home/philip/Documents/uni/spt/subm/wsort-T05/er04yjek/wsort.c:71:45: warning: conversion from ‘int’ to ‘char’ may change value [-Wconversion]
       71 |                                 char next = fgetc(stdin); // next mallocen??
          |                                             ^~~~~
    /home/philip/Documents/uni/spt/subm/wsort-T05/er04yjek/wsort.c:89:77: warning: conversion to ‘long unsigned int’ from ‘int’ may change the sign of the result [-Wsign-conversion]
       89 |                         char **wordsNew = realloc(words, (wordCounter + 20) * sizeof(char*)); // alle 20 zeilen mehr speicherplatz allokieren
          |                                                                             ^
    /home/philip/Documents/uni/spt/subm/wsort-T05/er04yjek/wsort.c:107:22: warning: conversion to ‘size_t’ {aka ‘long unsigned int’} from ‘int’ may change the sign of the result [-Wsign-conversion]
      107 |         qsort(words, wordCounter, sizeof (char*), compare);
          |                      ^~~~~~~~~~~
    /home/philip/Documents/uni/spt/subm/wsort-T05/er04yjek/wsort.c:28:14: warning: unused parameter ‘argc’ [-Wunused-parameter]
       28 | int main(int argc, char **argv){
          |          ~~~~^~~~
    /home/philip/Documents/uni/spt/subm/wsort-T05/er04yjek/wsort.c:28:27: warning: unused parameter ‘argv’ [-Wunused-parameter]
       28 | int main(int argc, char **argv){
          |                    ~~~~~~~^~~~

