#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h>
#include <errno.h>

static void die(char *msg){
    perror(msg);
    exit(EXIT_FAILURE);
}
// hallooo


static void checkResponse(FILE *rx, char *expectedCode){
    // read status code
    char code[4];
    if (fgets(code, 4, rx) == NULL){ // error or EOF occurred
        fprintf(stderr, "No server response\n");
        exit(EXIT_FAILURE);
    }

    // print out status code
    printf("%s", code);
    
    // print out rest of server-response
    int c;
    while (!feof(rx)){
        c = fgetc(rx);
        if (c == EOF){
            break;
        }
        if (fputc(c, stdout) == EOF){
            fprintf(stderr, "Error in fputc\n");
            exit(EXIT_FAILURE);
        }
        if (c == '\n'){
            break;
        }
    }
    if (fflush(stdout)){
        die("fflush");
    }

    // check if code == expected value; exit_failure if not
    if (!(code[0] == expectedCode[0] && code[1] == expectedCode[1] && code[2] == expectedCode[2])){
        // fprintf(stderr, "Unexpected server response code\n");
        exit(EXIT_FAILURE);
    }
}


static void sendLine(FILE *tx, char *line){
    // send query to server
    if (fputs(line, tx) == EOF){
        fprintf(stderr, "Error in fputs\n");
        exit(EXIT_FAILURE);
    }
    if (fflush(tx)){
        die("fflush");
    }

    printf("%s", line);
    if (fflush(stdout)){
        die("fflush");
    }
}


// STRG-D ends Body!
static void handleBody(FILE *tx){
    int c = -2;
    int last = -2;
    while (!feof(stdin)){
        c = fgetc(stdin);
        if (c == EOF){
            break;
        }
        if (c == '\n'){
            // add '\r' if not added automatically on newline on client-system
            if (!(last == '\r')){
                if (fputc('\r', tx) == EOF){
                    fprintf(stderr, "Error in fputc\n");
                    exit(EXIT_FAILURE);
                }
            }
            // flush buffer after every newline
            if (fputc(c, tx) == EOF){
                fprintf(stderr, "Error in fputc\n");
                exit(EXIT_FAILURE);
            }
            if (fflush(tx)){
                die("fflush");
            }
        } else {
            if (fputc(c, tx) == EOF){
                fprintf(stderr, "Error in fputc\n");
                exit(EXIT_FAILURE);
            }
        }
        if (c == '.' && last == '\n'){ // send .. to server if . on beginning of line
            if (fputc('.', tx) == EOF){
                fprintf(stderr, "Error in fputc\n");
                exit(EXIT_FAILURE);
            }
        }
        last = c;
    }
    // signalize end of body to server
    sendLine(tx, "\r\n.\r\n");
    if (fflush(tx)){
        die("fflush");
    }
    if (fflush(stdout)){
        die("fflush");
    }
}


static void handleDialogue(FILE *rx, FILE *tx, char *recipient, char *subject){
    checkResponse(rx, "220");

    // get client-FQDN for HELO statement
    char hostname[256];
    if (gethostname(hostname, 256) == -1){
        die("hostname");
    }
    struct addrinfo hints = {
        .ai_flags = AI_CANONNAME,
    };
    struct addrinfo *res;
    int error = getaddrinfo(hostname, NULL, &hints, &res);
    if (error){
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(error));
        exit(EXIT_FAILURE);
    }
    // now res->ai_canonname holds the FQDN
    size_t size = strlen("HELO ") + strlen(res->ai_canonname) + 2 + 1; // +\r\n\0
    char *helo = malloc(size);
    if (helo == NULL){
        die("malloc");
    }

    strcpy(helo, "HELO ");
    strcat(helo, res->ai_canonname);
    strcat(helo, "\r\n");
    freeaddrinfo(res);

    // continue interaction with server
    sendLine(tx, helo);
    free(helo);

    checkResponse(rx, "250");

    // get local username
    uid_t uid = getuid();
    errno = 0;
    struct passwd *pwuid = getpwuid(uid);
    char *username = pwuid->pw_name;
    if (username == NULL){
        if (errno){
            die("getpwuid");
        } else {
            fprintf(stderr, "Matching entry not found\n");
        }
    }
    
    size = strlen("MAIL FROM:") + strlen(username) + strlen("@cip.cs.fau.de\r\n") + 1;
    char *mail_from = malloc(size);
    if (mail_from == NULL){
        die("malloc");
    }

    strcpy(mail_from, "MAIL FROM:");
    strcat(mail_from, username);
    strcat(mail_from, "@cip.cs.fau.de\r\n");
    sendLine(tx, mail_from);
    free(mail_from);

    checkResponse(rx, "250");

    size = strlen("RCPT TO:") + strlen(recipient) + 2 + 1;
    char *rcpt_to = malloc(size);
    if (rcpt_to == NULL) {
        die("malloc");
    }

    strcpy(rcpt_to, "RCPT TO:");
    strcat(rcpt_to, recipient);
    strcat(rcpt_to, "\r\n");
    sendLine(tx, rcpt_to);
    free(rcpt_to);

    checkResponse(rx, "250");   

    sendLine(tx, "DATA\r\n");

    checkResponse(rx, "354");

    // extended username is substring in front of first comma
    char *username_ext = strtok(pwuid->pw_gecos, ",");

    size = strlen("From: ") + strlen(pwuid->pw_gecos) + 1 + strlen(username_ext) + strlen("@cip.cs.fau.de\r\n");    
    char *from = malloc(size);
    if (from == NULL){
        die("malloc");
    }

    strcpy(from, "From: ");
    strcat(from, username_ext);
    strcat(from, " ");
    strcat(from, username);
    strcat(from, "@cip.cs.fau.de\r\n");
    sendLine(tx, from);
    free(from);

    char *to = malloc(4 + strlen(recipient) + 2 + 1);
    if (to == NULL){
        die("malloc");
    }
    strcpy(to, "To: ");
    strcat(to, recipient);
    strcat(to, "\r\n");
    sendLine(tx, to);
    free(to);
    
    if (subject != NULL){
        char *sub = malloc(strlen("Subject: ") + strlen(subject));
        if (sub == NULL){
            die("subject");
        }
        strcpy(sub, "Subject: ");
        strcat(sub, subject);
        strcat(sub, "\r\n");
        sendLine(tx, sub);
        free(sub);
    }

    sendLine(tx, "\r\n");

    // body of mail
    handleBody(tx);

    checkResponse(rx, "250");

    sendLine(tx, "QUIT\r\n");

    checkResponse(rx, "221");
}


int main(int argc, char* argv[]){
    if (!(argc == 2 || argc == 3)){
        fprintf(stderr, "Usage: %s [-s <subject>] <address>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // DNS-Lookup server
    struct addrinfo hints = {
        .ai_socktype = SOCK_STREAM,
        .ai_family = AF_UNSPEC,
        .ai_flags = AI_ADDRCONFIG,
    }; // all other elements of struct are getting nulled in C
    struct addrinfo *head;
    int error = getaddrinfo("faui03.cs.fau.de", "25", &hints, &head);
    if (error){
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(error));
        exit(EXIT_FAILURE);
    }

    // create socket and connect to SMTP-Server
    int sock;
    struct addrinfo *curr;
    for (curr = head; curr != NULL; curr = curr->ai_next){
        sock = socket(curr->ai_family, curr->ai_socktype, curr->ai_protocol);
        if (sock == -1){ // on error no exit_failure necessary
            continue;
        }
        if (connect(sock, curr->ai_addr, curr->ai_addrlen) == 0){
            break;
        }
        // on error continue
        if (close(sock) == -1){ // die if close fails (waste of ressources, better try again)
            die("close");
        }
    }
    if (curr == NULL){
        die("No address found. Connection failed.\n");
    }
    freeaddrinfo(head);   

    // begin communication with server
    // create file* for reading
    FILE *rx = fdopen(sock, "r");

    // duplicate sock
    int sock_copy = dup(sock);
    if (sock_copy == -1){
        die("dup");
    }

    // create file* for writing
    FILE *tx = fdopen(sock_copy, "w");
    if (rx == NULL || tx == NULL){
        die("fdopen");
    }
    
    if (argc == 2){
        handleDialogue(rx, tx, argv[1], NULL); // mail without subject
    } else {
        handleDialogue(rx, tx, argv[2], argv[1]); // mail with subject
    }
    
    // close everything and quit
     if (fclose(rx)){
        die("fclose");
    }
    if (fclose(tx)) {
        die("fclose");
    }
    
    exit(EXIT_SUCCESS);
}

// TODO:
//      - was ist gute Fehlerbehandlung fuer fflush und fuer fgetc?
//      - Weitere Hilfsmethoden sinnvoll?