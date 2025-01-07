#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <pwd.h>
#include <errno.h>

/* Note: in order for this program to work properly it may be necessary
to have an account at cip.cs.fau.de of the FAU Erlangen and to run the
program on one of the CIP computers there (local username needed for
determination of sender address)*/

static void die(char *s){
    perror(s);
    exit(EXIT_FAILURE);
}

/* reads from rx until a '\n' occurs. If @param outstream is not NULL, prints the response to @outstream */
static void finish_line(FILE *rx, FILE *outstream){
    int c;
    while ((c = fgetc(rx)) != EOF) {
        if (outstream) {
            if (fputc(c, outstream) == EOF){
                fprintf(stderr, "Error fputc\n");
                exit(EXIT_FAILURE);
            }
        }
        if (c == '\n') break;
    }
    if (c == EOF){
        if (ferror(rx)) die("fgetc");
    }
}

// reads the first 4 chars from rx and checks for alignment with @param expected_code
static void check_rsp(FILE *rx, char *expected_code){
    char rsp_code[4+1];
    if (!fgets(rsp_code, sizeof(rsp_code), rx)){
        // EOF or error
        if (ferror(rx)){
            fprintf(stderr, "Error fgets\n");
            exit(EXIT_FAILURE);
        } else {
            // unexpected EOF - Codes differ
            fprintf(stderr, "Wrong response Code from Server: EOF\n");
            exit(EXIT_FAILURE);
        }
    }
    char exp_code[strlen(expected_code) + 1 + 1];  
    sprintf(exp_code, "%s ", expected_code); // add space to test for three digit number

    if (!strcmp(rsp_code, exp_code)){
        // Codes align
        finish_line(rx, NULL);
        return;
    } else {
        // Codes differ
        // print last response from server and terminate with error
        printf("%s", rsp_code);
        finish_line(rx, stdout);
        fprintf(stderr, "Wrong response Code from Server\n");
        exit(EXIT_FAILURE);
    }
}

static void send_line(FILE *tx, char *line){
    if (fputs(line, tx) == EOF){
        fprintf(stderr, "Error fputs\n");
        exit(EXIT_FAILURE);
    }
    if (fflush(tx)) die("fflush");
}

// reads mail body from stdin and sends it to server
static void process_body(FILE *tx){
    int c;
    int last = -1;
    while ((c = fgetc(stdin)) != EOF){
        if (c == '\n'){
            // always ensure "\r\n" at end of line
            if (last != '\r'){
                if (fputc('\r', tx) == EOF){
                    fprintf(stderr, "Error fputc\n");
                    exit(EXIT_FAILURE);
                }
            }
        } else if (c == '.'){
            // always duplicate '.' when new line begins with '.'
            if (last == '\n'){
                if (fputc('.', tx) == EOF){
                    fprintf(stderr, "Error fputc\n");
                    exit(EXIT_FAILURE);
                }
            }
        }
        if (fputc(c, tx) == EOF){
            fprintf(stderr, "Error fputc\n");
            exit(EXIT_FAILURE);
        }
        last = c;
    }
    if (ferror(stdin)){
        fprintf(stderr, "Error fgetc\n");
        exit(EXIT_FAILURE);
    }

    // signal end of mail body to Server
    if (last != '\n'){
        if (fputs("\r\n.\r\n", tx) == EOF){
            fprintf(stderr, "Error fputs\n");
            exit(EXIT_FAILURE);
        }
    } else {
        if (fputs(".\r\n", tx) == EOF){
            fprintf(stderr, "Error fputs\n");
            exit(EXIT_FAILURE);
        }
    }
    fflush(tx);
}


static void handle_dialogue(FILE *rx, FILE *tx, char *subject, char *address){
    check_rsp(rx, "220");

    // determine hostname and fqdn for HELO statement
    char hostname[253 + 1]; // len(hostname) <= 253 according to DNS standards
    if (gethostname(hostname, sizeof(hostname))){
        die("gethostname");
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
    // res->ai_canonname holds fqdn now
    char helo[strlen("HELO ") + strlen(res->ai_canonname) + 2 + 1]; // + \r\n\0
    sprintf(helo, "HELO %s\r\n", res->ai_canonname);
    freeaddrinfo(res);

    send_line(tx, helo);
    check_rsp(rx, "250");
    
    // create sender address
    uid_t uid = getuid();
    errno = 0;
    struct passwd *passwd = getpwuid(uid);
    if (!passwd) die("getpwuid");
    char mail[strlen("MAIL FROM:") + strlen(passwd->pw_name) + strlen("@cip.cs.fau.de\r\n") + 1];
    sprintf(mail, "MAIL FROM:%s@cip.cs.fau.de\r\n", passwd->pw_name);
    
    send_line(tx, mail);
    check_rsp(rx, "250");

    char rcpt[strlen("RCPT TO:") + strlen(address) + 2 + 1];
    sprintf(rcpt, "RCPT TO:%s\r\n", address);

    send_line(tx, rcpt);
    check_rsp(rx, "250");

    send_line(tx, "DATA\r\n");
    check_rsp(rx, "354");

    // get full user name
    char user_info[strlen(passwd->pw_gecos) + 1];
    strcpy(user_info, passwd->pw_gecos);
    char *full_user = strtok(user_info, ",");

    char from[strlen("From: ") + strlen(full_user) + 1 + strlen(passwd->pw_name) + strlen("@cip.cs.fau.de\r\n") + 1];
    sprintf(from, "From: %s %s@cip.cs.fau.de\r\n", full_user, passwd->pw_name);
    char to[strlen("To: ") + strlen(address) + 2 + 1];
    sprintf(to, "To: %s\r\n", address);
    
    send_line(tx, from);
    send_line(tx, to);
    if (subject){
        char subj[strlen("Subject: " + strlen(subject) + 2 + 1)];
        sprintf(subj, "Subject: %s\r\n", subject);
        send_line(tx, subj);
    }
    send_line(tx,"\r\n");

    // read mail body from stdin and send it
    process_body(tx);
    check_rsp(rx, "250");
    send_line(tx, "QUIT\r\n");
    check_rsp(rx, "221");
}

int main(int argc, char *argv[]){
    if (argc != 2 && argc != 3){
        fprintf(stderr, "usage: snail [-s <subject>] <address>\n");
        exit(EXIT_FAILURE);
    }

    // establish connection to server
    struct addrinfo hints = {
        .ai_socktype = SOCK_STREAM,
        .ai_family = AF_UNSPEC,
        .ai_flags = AI_ADDRCONFIG,
    };
    struct addrinfo *head;
    int error = getaddrinfo("faui03.cs.fau.de", "25", &hints, &head);
    if (error) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(error));
        exit(EXIT_FAILURE);
    }
    // traverse list for usable address
    int sock;
    struct addrinfo *curr;
    for (curr = head; curr != NULL; curr = curr->ai_next){
        sock = socket(curr->ai_family, curr->ai_socktype, curr->ai_protocol);
        if (sock < 0) continue;
        if (connect(sock, curr->ai_addr, curr->ai_addrlen) == 0) break;
        close(sock);
    }
    if (curr == NULL){
        fprintf(stderr, "No usable address was found\n");
        exit(EXIT_FAILURE);
    }
    freeaddrinfo(head);
    // begin communication with server
    FILE *rx = fdopen(sock, "r");
    if (!rx) die("fdopen");
    sock = dup(sock);
    if (sock == -1) die("dup");
    FILE *tx = fdopen(sock, "w");
    if (!tx) die("fdopen");
    // delete when connection to server successful again
    // FILE *rx = fopen("server_simulation.txt", "r");
    // if (!rx) die("fopen");
    // FILE *tx = fopen("client_resp_simulation.txt", "w");
    // if (!tx) die("fopen");

    if (argc == 2) {
        handle_dialogue(rx, tx, NULL, argv[1]);
    } else {
        handle_dialogue(rx, tx, argv[1], argv[2]);
    }

    // clean up
    fclose(rx);
    fclose(tx);
    exit(EXIT_SUCCESS);
}

// TODO:
// - hton(?) verwenden (network byte order)
// - how to deal with -s
// - make sender address generic (use gmail or sth, but passwd may be necessary)
//   - in the MAIL FROM section real name might be necessary (try your@cip.cs.fau.de first)
//   - in the From: section I suppose you can use any name you want (use define statements or global vars!)
// - timeout bei connect() einführen