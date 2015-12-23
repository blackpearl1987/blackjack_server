#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#define CARDS_PER_DECK 13
#define HAND_MAX_SIZE 22

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

void print_hand(const char *hand) {
    printf("\n");
    for(int i = 0; i < strlen(hand); i++) {
        printf("%s ", print_card(hand[i]));
    }
    printf("Total: %d", calculate_total(hand));
}

int main(int argc, char *argv[]) {
    int number_of_decks = 1;
    char *welcome_message = "**************************\n* WELCOME TO BLACKJACK!! *\n**************************\n";
    char *rules = "Dealer hits a soft 17";
    char choice[1];
    char deck[] = {'2', '3', '4', '5', '6', '7', '8', '9', 'T', 'J', 'K', 'Q', 'A'};
    char *playing_deck = NULL;
    char *player_hand = NULL;
    char *dealer_hand = NULL;
    int I = 0, P = 0, D = 0;
    int third = 0;
    char ch;

    printf("%s\n", welcome_message);
    
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
    printf("Start game? [y/n] ");
    gets(choice);
    while(choice[0] != 'y' && choice[0] != 'n') {
        printf("Sorry that was an invalid choice. Please enter 'y' or 'n':");
        gets(choice);
    }
    if(choice[0] == 'n') {
        printf("Bye!\n");
        return 0;
    }

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
    third = (number_of_decks * CARDS_PER_DECK) / 3;
    do {
        if((number_of_decks*CARDS_PER_DECK - I) <= third) {
            printf("\nRe-shuffling deck..\n");
            shuffle(playing_deck, number_of_decks);
            I = 0;
        }
        if(!player_hand)    free(player_hand);
        if(!dealer_hand)    free(dealer_hand);
        player_hand = (char *) malloc(sizeof(char) * HAND_MAX_SIZE); memset(player_hand, 0, HAND_MAX_SIZE);
        dealer_hand = (char *) malloc(sizeof(char) * HAND_MAX_SIZE); memset(dealer_hand, 0, HAND_MAX_SIZE);
        P = 0; D = 0;
        player_hand[P++] = playing_deck[I++];
        dealer_hand[D++] = playing_deck[I++];
        player_hand[P++] = playing_deck[I++];
        dealer_hand[D++] = playing_deck[I++];

        printf("Player: %s, %s   Total: %d\n", print_card(player_hand[0]), print_card(player_hand[1]), calculate_total(player_hand));
        printf("Dealer: %s, X\n", print_card(dealer_hand[0]));
        int pbj = is_blackjack(player_hand);
        int dbj = is_blackjack(dealer_hand);

        if(pbj && dbj) {
            printf("Dealer and Player Blackjack. Stay!");
            continue;
        } else if(pbj) {
            printf("BLACKJACK!! CONGRATULATIONS!!");
            continue;
        } else if(dbj) {
            printf("DEALER BLACKJACK! Sorry you lose.");
            continue;
        }
        
        // Player turn
        choice[0] = 'h';
        int player_total = calculate_total(player_hand);
        while(choice[0] == 'h' && player_total != 21) {
            printf("\nHit/Stand? [h/s]: ");
            gets(choice);
            while(choice[0] != 'h' && choice[0] != 's') {
                printf("\nWrong choice. Please enter h or s: ");
                gets(choice);
            }
            if(choice[0] == 'h') {
                player_hand[P++] = playing_deck[I++];
                print_hand(player_hand);
                player_total = calculate_total(player_hand);
                if(player_total > 21) {
                    printf("\nBUST!!! Dealer wins.");
                    break;
                }
            }
        }

        if(player_total > 21) continue;

        // Dealer turn
        printf("\nDealer turn..");
        int dealer_total = calculate_total(dealer_hand);
        print_hand(dealer_hand);
        while(dealer_total <= 17) {
            dealer_hand[D++] = playing_deck[I++];
            print_hand(dealer_hand);
            dealer_total = calculate_total(dealer_hand);
        }
        if(dealer_total > 21) {
            printf("\nDEALER BUST!! YOU WIN!!!");
        } else if(dealer_total > player_total) {
            printf("\nDEALER WINS. SORRY. :(");
        } else if(dealer_total < player_total) {
            printf("\nCONGRATULATIONS!! YOU WIN!!");
        } else {
            printf("\nSTAY...");
        }
    } while(continue_game());

    return 0;
}
