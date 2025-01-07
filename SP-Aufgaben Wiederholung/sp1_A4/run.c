/* Beispielimplementierung für das run-Modul um das Bearbeiten der
 * Übungsaufgaben auf nicht x86-64-Platformen zu ermöglichen.  Beachten Sie, dass
 * ihr Programm mit jeder möglichen Implementierung der Schnittstelle
 * funktionieren muss.  Verlassen Sie sich nicht auf konkretes Verhalten dieser
 * Implementierung.
 *
 * Der CIP ist weiterhin Referenz-System. Testen Sie Ihr Programm in jedem Fall
 * auch dort. */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "run.h"

int run_cmd(const char *cmd, char **out) {
     int save;
     size_t len = 0;
     *out = NULL;
      
     FILE *p = popen(cmd, "r");
     if (!p) {
	  goto fail;
     }

     /* Note: this will not read the entire output of the process, but
      * only up to the first NUL-byte.  This is acceptable, as the
      * output is stored in a C-string, which cannot represent
      * NUL-bytes as content. */
     if (-1 == getdelim(out, &len, 0, p)) {
	  free(*out);
	  *out = NULL;
	  if (ferror(p)) {
	       goto fail;
	  }
     }
     return pclose(p);

fail:
     save = errno;
     if (p != NULL) pclose(p);
     errno = save;
     return -1;
}
