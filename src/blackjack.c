#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#define SHMSZ 1024
#define P1 5001
#define CARDS_PER_DECK 13
#define HAND_MAX_SIZE 22
#define MAX_PLAYERS 5

struct player_hand {
    char cards[21];     // player hand
    short bust;         // player bust
    short blackjack;    // blackjack
    short n;            // number of cards player holds
};

int player_connection[MAX_PLAYERS];

int send_message_to_all_players(const char *message);       // Send a message to all the player
int send_message_to_player(const char *message, int i);     // Send a message to a single player
void print_hand(const char *hand, char *dest);              // Prints the hand into the dest buffer

// closes all player connections
void close_connections() {
    printf("Closing all connections..\n");
    for(int i = 0; i < MAX_PLAYERS; i++) {
        if( player_connection[i] > 0 ) {
            if( close(player_connection[i]) < 0 ) {
                perror("Error closing connection");
            }
        }
    }
}

// catches the Ctrl+C signal
void signal_callback_handler(int signum) {
    printf("\nCaught signal %d\n", signum);
    close_connections();
    exit(0);
}


// Sets/unsets a blocking fd.
// fd -> The file descriptor
// blocking == 1 -> sets blocking
// blocking == 2 -> sets non-blocking
// returns 1 on success, 2 on failure
int set_blocking_fd(int fd, int blocking) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return 0;

    if (blocking)
        flags &= ~O_NONBLOCK;
    else
        flags |= O_NONBLOCK;
    return fcntl(fd, F_SETFL, flags) != -1;
}

void shuffle(char *a, int d) {
    // Fisher-Yates shuffle
    
    int n = d * CARDS_PER_DECK;

    srand(time(NULL));
    for(int i = n-1; i > 0; i--) {
        int j = (int) rand() % i;
        char t = a[i];
        a[i] = a[j];
        a[j] = t;
    }

}

char *print_card(char ch) {
    if(ch == 'T') {
        return "10";
    }
    char *c = (char *) malloc(sizeof(char));
    c[0] = ch;
    return c;
}

int continue_game() {
    char choice[1];
    printf("\nNew game?[y/n]: ");
    gets(choice);
    while(choice[0] != 'y' && choice[0] != 'n') {
        printf("\nWrong choice. Please enter y or n: ");
        gets(choice);
    }
    if(choice[0] == 'y') {
        return 1;
    }
    return 0;
}

int calculate_total(const char *hand) {
    int max_total = 0;
    int min_total = 0;
    for(int i = 0; i < strlen(hand); i++) {
        switch(hand[i]) {
            case 'T':
            case 'J':
            case 'K':
            case 'Q':
                if(max_total + 10 > 21) {
                    max_total = min_total + 10;
                } else {
                    max_total += 10;
                }
                min_total += 10;
                break;
            case 'A':
                if(max_total + 11 > 21) {
                    if(min_total + 11 > 21) {
                        max_total = min_total + 1;
                    } else {
                        max_total = min_total + 11;
                    }
                } else {
                    max_total += 11;
                }
                min_total += 1;
                break;
            default:
                max_total += hand[i] - '0';
                min_total += hand[i] - '0';
        }
    }
    return max_total;
}

int is_blackjack(const char *hand) {
    if(strlen(hand) == 2 && calculate_total(hand) == 21)    return 1;
    return 0;
}

void print_hand(const char *hand, char *dest) {
    for(int i = 0; i < strlen(hand); i++) {
        char *x = print_card(hand[i]);
        strncat(dest, x, strlen(x));
    }
}

int send_message_to_all_players(const char *message) {
    int rc = 0;
    printf("sending %s to all players", message);
    for(int i = 0; i < MAX_PLAYERS; i++) {
        if(player_connection[i] > 0) {
            rc |= send_message_to_player(message, i);
        }
    }

    return rc;
}

int send_message_to_player(const char *message, int i) {
    int rc;
    if(player_connection[i] > 0) {
        rc = write(player_connection[i], message, strlen(message));
        if(rc < 0) {
            perror("Error writing message");
        }
        return 1;
    }

    return 0;
}

int main(int argc, char *argv[]) {
    int number_of_decks = 1;
    char welcome_message[1024] = "**************************\n* WELCOME TO BLACKJACK!! *\n**************************\n";
    char *rules = "Dealer hits a soft 17";
    char choice[1];
    char deck[] = {'2', '3', '4', '5', '6', '7', '8', '9', 'T', 'J', 'K', 'Q', 'A'};
    char *playing_deck = NULL, *dealer_hand = NULL;
    struct player_hand *pHands;
    int I = 0, P = 0, D = 0, third = 0, t = 1;
    char ch;

    // connections variables
    int listenfd = 0, fd = -1, accepting = 1;
    char recvBuff[4], sendBuff[1024], smallBuff[16];
    struct sockaddr_in serv_addr;
    int sockfd = 0, rc = -1, player = -1;

    signal(SIGINT, signal_callback_handler);

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0))< 0) {
        printf("\n Error : Could not create socket \n");
        return 1;
    }

    // initialze player connections
    for(int i = 0; i < MAX_PLAYERS; i++) {
        player_connection[i] = -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(P1);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
         
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    // set TCP_NODELAY
    if(setsockopt(listenfd, IPPROTO_TCP, TCP_NODELAY, &t, sizeof(int)) < 0) {
        perror("TCP No delay cannot be set");
        exit(1);
    }
    bind(listenfd, (struct sockaddr*)&serv_addr,sizeof(serv_addr));
    if(listen(listenfd, MAX_PLAYERS) == -1) {
        perror("listen");
        printf("Failed to listen\n");
        return -1;
    }

    printf("Waiting for players to join..\n Ctrl+C to exit\n");
    player_connection[++player] = accept(listenfd, (struct sockaddr*)NULL , NULL); // accept awaiting request
    printf("Player %d joined\n", player + 1);
    rc = write(player_connection[player], welcome_message, strlen(welcome_message));
    if(rc < 0) {
        perror("write failure");
    }
    snprintf(sendBuff, sizeof(sendBuff), "You are player %d", player+1);
    rc = write(player_connection[player], sendBuff, strlen(sendBuff));
    set_blocking_fd(listenfd, 0); // set to non-blocking to listen for more players
    printf("Waiting 5 seconds for another player to join..\n");
    for(int i = 0; i < 5 && player < 5; i++) {
        if(fd > 0 && player < 5) {
            player_connection[++player] = fd;
            rc = write(player_connection[player], welcome_message, strlen(welcome_message));
            snprintf(sendBuff, sizeof(sendBuff), "You are player %d", player+1);
            rc = write(player_connection[player], sendBuff, strlen(sendBuff));
            if(rc < 0) {
                perror("write failure");
            }
            printf("Player %d joined\n", player + 1);
            printf("Waiting 5 seconds for another player to join..\n");
            i = 0;
        }
        fd = accept(listenfd, (struct sockaddr*)NULL , NULL);
        sleep(1);
    }

    printf("Starting game\n");
    send_message_to_all_players("Starting game..");

    if(argc > 1) {
        number_of_decks = (int) strtol(argv[1], NULL, 10);
        if(errno == ERANGE || number_of_decks > 8) {
            printf("Sorry! We cannot play Blackjack with those many decks! Please restart with 1-8 decks\n");
            return 1;
        }
    }
    if(number_of_decks == 1) {
        printf("You are playing single deck blackjack\n%s\n", rules);
    } else {
        printf("You are playing %d deck blackjack\n%s\n", number_of_decks, rules);
    }
    /* printf("Start game? [y/n] ");
    gets(choice);
    while(choice[0] != 'y' && choice[0] != 'n') {
        printf("Sorry that was an invalid choice. Please enter 'y' or 'n':");
        gets(choice);
    }
    if(choice[0] == 'n') {
        printf("Bye!\n");
        close_connections();
        return 0;
    } */

    // Create playing deck
    playing_deck = (char *) malloc(sizeof(char) * number_of_decks * CARDS_PER_DECK);
    for(int i = 0; i < number_of_decks; i++) {
       for(int j = 0; j < CARDS_PER_DECK; j++) {
           playing_deck[i*CARDS_PER_DECK + j] = deck[j];
       }
    }
    printf("\nShuffling deck..\n");
    shuffle(playing_deck, number_of_decks);
    printf("\nDealing..\n");
    send_message_to_all_players("Dealing..");
    third = (number_of_decks * CARDS_PER_DECK) / 3;
    do {
        if((number_of_decks*CARDS_PER_DECK - I) <= third) {
            printf("\nRe-shuffling deck..\n");
            send_message_to_all_players("Re-shuffling deck..");
            shuffle(playing_deck, number_of_decks);
            I = 0;
        }
        if(!pHands)    free(pHands);
        if(!dealer_hand)    free(dealer_hand);
        pHands = (struct player_hand*) malloc(sizeof(struct player_hand) * (player + 1)); memset(pHands, 0, sizeof(struct player_hand) * (player + 1));
        dealer_hand = (char *) malloc(sizeof(char) * HAND_MAX_SIZE); memset(dealer_hand, 0, HAND_MAX_SIZE);
        P = 0; D = 0;

        for(int j = 1; j <= 2; j++) {
            // deal 1 card to each player
            for(int i = 0; i <= player; i++) {
                t = pHands[i].n;
                char card = playing_deck[I++];
                pHands[i].cards[t++] = card;
                pHands[i].n = t;
                memset(smallBuff, 0, sizeof(smallBuff));
                snprintf(smallBuff, sizeof(smallBuff), "%s ", print_card(card));
                rc = write(player_connection[i], smallBuff, strlen(smallBuff));
                if(rc < 0) {
                    perror("sending card");
                }
            }
            // dealer takes a card
            dealer_hand[D++] = playing_deck[I++];
            
            // Only show dealer's second card to the players. (Or should it be the first?)
            if(D == 2) {
                memset(sendBuff, 0, sizeof(sendBuff));
                snprintf(sendBuff, sizeof(sendBuff), "\nD: X %s", print_card(dealer_hand[1]));
                send_message_to_all_players(sendBuff);
            }
        }
        printf("1\n");

        int dbj = is_blackjack(dealer_hand);

        printf("2\n");

        // check for player blackjacks
        for(int i = 0; i <= player; i++) {
            printf("3\n");
            if(is_blackjack(pHands[i].cards)) {
                pHands[i].blackjack = 1;
                memset(sendBuff, 0, sizeof(sendBuff));
                if(dbj) {
                    snprintf(sendBuff, sizeof(sendBuff), "DEALER BLACKJACK! sTAY!");
                } else {
                    snprintf(sendBuff, sizeof(sendBuff), "BLACKJACK");
                }
                rc = write(player_connection[i], sendBuff, strlen(sendBuff));
            } else if(dbj) {
            }
        } 
        printf("4\n");

        if(dbj) {
            printf("I have blackjack. Tehehehehehe >;-)");
            continue;
        }
        
        // Player(s) turn
        for(int i = 0; i <= player; i++) {
            if(pHands[i].blackjack) {
                continue;
            }
            memset(smallBuff, 0, sizeof(smallBuff));
            snprintf(smallBuff, sizeof(smallBuff), "Your turn");
            write(player_connection[i], smallBuff, strlen(smallBuff));
            memset(recvBuff, '0' ,sizeof(recvBuff));
            printf("5\n");
            read(player_connection[i], recvBuff, sizeof(recvBuff));
            printf("6\n");
            while(recvBuff[0] == 'h') {
                t = pHands[i].n;
                char card = playing_deck[I++];
                pHands[i].cards[t++] = card;
                pHands[i].n = t;
                if(calculate_total(pHands[i].cards) > 21) {
                    // bust!
                    memset(smallBuff, 0, sizeof(smallBuff));
                    snprintf(smallBuff, sizeof(smallBuff), "%s\nBUST",print_card(card));
                    rc = write(player_connection[i], smallBuff, strlen(smallBuff));
                    break;
                }
                memset(smallBuff, 0, sizeof(smallBuff));
                snprintf(smallBuff, sizeof(smallBuff), "\n%s ", print_card(card));
                rc = write(player_connection[i], smallBuff, strlen(smallBuff));
                read(player_connection[i], recvBuff, sizeof(recvBuff));
            }
            printf("7\n");
        }

        printf("8\n");
        // Dealer turn
        send_message_to_all_players("Dealer turn");
        printf("\nDealer turn..");
        int dealer_total = calculate_total(dealer_hand);
        memset(smallBuff, 0, sizeof(smallBuff));
        print_hand(dealer_hand, smallBuff);
        send_message_to_all_players(smallBuff);
        while(dealer_total <= 17) {
            dealer_hand[D++] = playing_deck[I++];
            memset(smallBuff, 0, sizeof(smallBuff));
            print_hand(dealer_hand, smallBuff);
            send_message_to_all_players(smallBuff);
            dealer_total = calculate_total(dealer_hand);
        }
        if(dealer_total > 21) {
            send_message_to_all_players("DEALER BUST!! YOU WIN!!!");
        } else {
            for(int i = 0; i <= player; i++) {
                int ptotal = calculate_total(pHands[i].cards);
                if( ptotal == dealer_total) {
                    send_message_to_player("STAY", i);
                } else if( ptotal < dealer_total) {
                    send_message_to_player("Sorry! You lose!", i);
                } else {
                    send_message_to_player("YOU WIN!", i);
                }
            }
        }
    } while(0); // continue_game());

    close_connections();
    return 0;
}
