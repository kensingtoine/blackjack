#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <string.h>

#define NUM_FACES 13
#define NUM_SUITS 4
#define LENGTH_FACES 6
#define LENGTH_SUITS 9
#define DECK_SIZE (NUM_FACES * NUM_SUITS)
#define MAX_DECK_AMT 8
#define NUM_INIT_HANDS_DEALER 10
#define NUM_INIT_HANDS_PLAYER 32
#define ASCII_LOWER_TO_UPPER_CASE -32

typedef struct player {
  char *name;
  int score, won, lost, tied, cards_drawn, aces_drawn, aces_mitigated;
  int total_won, total_lost, total_tied;
  double winnings, total_winnings;
} player;

typedef struct card {
  int id, suit, face, bj_value, count_value;
} card;

char buf[101];

char strategy_table[NUM_INIT_HANDS_DEALER][NUM_INIT_HANDS_PLAYER];

char suits[NUM_SUITS][LENGTH_SUITS] = {"hearts","spades","clubs","diamonds"};
char faces[NUM_FACES][LENGTH_FACES] = {"ace","two","three","four",										 "five","six","seven","eight",   	 								 "nine","ten","jack","queen","king"};
									   
char* get_player_name(char *name);
int set_deck_amt();
int set_card_amt(card *c);

void strategy_table_read();
void init_decks(card *c, int card_amt);
void assign_bj_values(card *c, int card_amt);
void shuffle(card *c, int *card_num_p, int *count_p, int card_amt);

void menu(player *p, player *d, card *c, char *filename, int *card_num_p, int *count_p, int *count_values_p, int card_amt);

void play_game(player *p, player *d, card *c, int *card_num_p, int *count_p, int card_amt, int rounds, bool manual);
void split_game(player *p, player *d, card *c, double *bet_p, int *card_num_p, int *count_p, int card_amt, bool manual);

double get_bet();
double auto_bet(int card_amt, int *card_num_p, int *count_p);

void draw(player *p, card *c, int *card_num_p, int *count_p, int card_amt);
void mitigate_ace(player *p, card *c, int *card_num_p);
int strategy_table_lookup(player *p, card *c, int *card_num_p);
void advise_move(int x, int y);
char player_move(player *p, card *c, int *card_num_p);
void player_turn(player *p, player *d, card *c, double *bet_p, int *card_num_p, int *count_p, int card_amt, bool manual);
void dealer_turn(player *d, card *c, int *card_num_p, int *count_p, int card_amt);
void find_winner(player *p, player *d, double bet);
double double_up (player *p, double bet);
double surrender(player *p, double bet);
bool bust(player *p);
bool blackjack(player *p);
bool play_again();

int rounds_to_simulate();

char* get_count_cfg(player *p, player *d, card *c, char *filename, int *card_num_p, int *count_p, int *count_values_p, int card_amt);
char* custom_count_cfg(player *p, player *d, card *c, char *filename, int *card_num_p, int *count_p, int *count_values_p, int card_amt);
void read_count_cfg(char *filename, card *c, int card_amt, int *count_values_p);
bool file_exists(char *filename);

void store_stats(player *p);
void get_stats(player *p);

int main(void) {
  // Seed used in rand() based on the time at compile time
  srand( time(NULL) );

  
  // Declaring pointers with which to store user input strings
  char *name = malloc(sizeof(char) * 101);
  char *filename = malloc(sizeof(char) * 101);

  // Declaring pointers to structs and allocating memory
  card *c = malloc(sizeof(*c) * MAX_DECK_AMT * DECK_SIZE); 		  
  player *d = malloc(sizeof(player));
  player *p = malloc(sizeof(player));
  
  // Initializing th
  d->name = "Dealer";
  p->name = get_player_name(name);
  p->won = p->lost = p->tied = p->winnings = 0;  

  // Initializing central variables and pointers to their location
  int count = 0;  
  int card_num = 0;  
  int count_values[NUM_FACES];  
  int *count_p = &count;
  int *card_num_p = &card_num;
  int *count_values_p = count_values;  
  
  int deck_amt = set_deck_amt();
  int card_amt = deck_amt * DECK_SIZE; 
  
  strategy_table_read();
  init_decks(c, card_amt);
  shuffle(c, card_num_p, count_p, card_amt);
  
  filename = get_count_cfg(p, d, c, filename, card_num_p, count_p, count_values_p, card_amt);
  read_count_cfg(filename, c, card_amt, count_values_p);

  printf("\nWelcome! This program offers the possibility to play Blackjack and perform simulations %s",
          "on any number of hands with custom strategies and/or card counting systems\n");		  
  
   // Indgangs-menu
  menu(p, d, c, filename, card_num_p, count_p, count_values_p, card_amt);
  
  free(p);
  free(d);
  free(c);
  free(name);
  free(filename);
  
  return 0;
}

char* get_player_name(char *name) {
  char *input = malloc(sizeof(char) * 101);
  
  do {
    printf("\nEnter your name (characters only):\n");
	if (fgets(buf, sizeof(buf), stdin) != NULL)  
      if (sscanf(buf, "%s", input))   		// Validerer brugerinput 
	      strcpy (name, input);
  } while (!sscanf(buf, "%[a-zA-Z]s", name));
  
  free(input);
  
  return name;
}

int set_deck_amt() {
  int input, deck_amt;
  do {
    printf("\nType in how many decks of 52 cards to use in the shoe (1-8):\n");
	if (fgets(buf, sizeof(buf), stdin) != NULL)
      if (sscanf(buf, "%d", &input))
	    deck_amt = input;
  } while (deck_amt < 1 || deck_amt > MAX_DECK_AMT);
  
  return deck_amt;
}

// 
void init_decks(card *c, int card_amt) {
  int i;
  for (i = 0; i < card_amt; i++) {
    c[i].id = i;
	c[i].suit = c[i].id % NUM_SUITS;
    c[i].face = c[i].id % NUM_FACES;
	c[i].count_value = 0;
  }
  assign_bj_values(c, card_amt);  
}

// Assign values 
void assign_bj_values(card *c, int card_amt) {
  int i;
  for (i = 0; i < card_amt; i++) {  
    switch (c[i].face) {
      case 0:
        c[i].bj_value = 11;
        break;
      case 1:
        c[i].bj_value = 2;
        break;
      case 2:
        c[i].bj_value = 3;
        break;
      case 3:
        c[i].bj_value = 4;
        break;
      case 4:
        c[i].bj_value = 5;
        break;
      case 5:
        c[i].bj_value = 6;
        break;
      case 6:
        c[i].bj_value = 7;
        break;
      case 7:
        c[i].bj_value = 8;
        break;
      case 8:
        c[i].bj_value = 9;
        break;
      default:
        c[i].bj_value = 10;
        break;
    }
  }
}

// Fisher-Yates Shuffle implemented to shuffle struct elements
void shuffle(card *c, int *card_num_p, int *count_p, int card_amt) {
  int i, j;
  card tmp;
  for (i = card_amt - 1; i >= 0 ; i--) {
    j = rand() % card_amt;
	tmp = c[j];  
	c[j] = c[i];
	c[i] = tmp;
  }  
  
  *card_num_p = 0;  // Count and card number is reset
  *count_p = 0;
}


char* get_count_cfg(player *p, player *d, card *c, char *filename, int *card_num_p, int *count_p, int *count_values_p, int card_amt) {
  int input, option;
  
  do {
    printf("\nChoose count configuration:\
	       \n1. Hi-Lo\n2. Hi-Opt I\n3. Hi-Opt II\n4. KO\n5. Omega II\
		   \n6. Zen Count\n7. Custom defined configurations from file\n");
	
	// Validerer brugerinput
	if (fgets(buf, sizeof(buf), stdin) != NULL)
      if (sscanf (buf, "%d", &input))
	    option = input;
  } while (option < 1 || option > 7);
  
  switch (option) {
    case 1:
      return "hi-lo.txt";
	  break;
    case 2:
      return "hi-opt_i.txt";
	  break;
	case 3:
	  return "hi-opt_ii.txt";
	  break;
	case 4:
	  return "ko.txt";
	  break;
	case 5:
	  return "omega_ii.txt";
	  break;	  
	case 6:
	  return "zen_count.txt";
	  break;
    case 7:
      return custom_count_cfg(p, d, c, filename, card_num_p, count_p, count_values_p, card_amt);
      break;
  }
  
  return 0;
}

// Returns a given filename if it exists
char* custom_count_cfg(player *p, player *d, card *c, char *filename, int *card_num_p, int *count_p, int *count_values_p, int card_amt) {
  char *input = malloc(sizeof(char) * 101);
  char *escape = {"menu"};
  
  do {
    printf("\nProvide path to the configuration file or enter \"menu\" to go back:\n");
    if (fgets (buf, sizeof(buf), stdin) != NULL) {
	  if (sscanf(buf, "%s", input)) {
	    strcpy(filename, input);
		
	    if (strcmp(input, escape) == 0) {
		  menu(p, d, c, filename, card_num_p, count_p, count_values_p, card_amt);
		  break;
		}
      }
	}
  } while (!file_exists(filename));
  
  free(input);
  
  return filename;
}

// Reads integer values from the given filename a value for each card face and stores them in array
void read_count_cfg(char *filename, card *c, int card_amt, int *count_values_p) {
  FILE *file = fopen(filename, "r");

  int i;
  for (i = 0; i < NUM_FACES; i++) {
    fscanf(file, "%d", &count_values_p[i]);
  }
  int j, tmp;
  for (j = 0; j < card_amt; j++) {
    tmp = c[j].id % NUM_FACES;
    c[j].count_value = count_values_p[tmp];
  }  
  fclose(file);
}

// Check if it's possible to open a file from compile directory with the name of a given string
bool file_exists(char *filename) {
  FILE *file;
  file = fopen(filename, "r");
  fclose(file);
  
  if (file != NULL) {
    return true;
  }    
  return false;
}

// Main menu that lets the player access all of the intended program functionality
void menu(player *p, player *d, card *c, char *filename, int *card_num_p, int *count_p, int *count_values_p, int card_amt) {

  int option, input, rounds; 
  
  do {
    printf("\nChoose from the menu below:\n1: Play game\n2: Simulate %s",
	        "\n3: Choose count configuration\n4: Change amount of decks \n5: See stats\n6: Exit\n");
	if (fgets(buf, sizeof(buf), stdin) != NULL)
      if (sscanf(buf, "%d", &input))
	      option = input;
  } while (option < 1 || option > 6);
  
  switch (option) {
    case 1:
	  do {
        play_game(p, d, c, card_num_p, count_p, card_amt, 1, true);
      } while (play_again());
	  
	  store_stats(p);
	  break;
    case 2:	  
	  rounds = rounds_to_simulate();
      play_game(p, d, c, card_num_p, count_p, card_amt, rounds, false);
	  store_stats(p);
	  break;
	case 3:
      filename = get_count_cfg(p, d, c, filename, card_num_p, count_p, count_values_p, card_amt);
	  read_count_cfg(filename, c, card_amt, count_values_p);	  
	  break;
	case 4:
	  card_amt = set_card_amt(c);
	  init_decks(c, card_amt);
	  shuffle(c, card_num_p, count_p, card_amt);
	  break;
	case 5:
	  get_stats(p);
	  break;
	case 6:
	  printf("\nHave a nice day!\n");
	  return;
  }  
  menu(p, d, c, filename, card_num_p, count_p, count_values_p, card_amt);
  return;
}

// Prompts the player to decide how many decks to use, and calculates amount of cards based on this input
int set_card_amt(card *c) {
  int deck_amt = set_deck_amt();
  int card_amt = deck_amt * DECK_SIZE;
  
  return card_amt;
}

// Simulates a game of Blackjack for any number of rounds
void play_game(player *p, player *d, card *c, int *card_num_p, int *count_p, int card_amt, int rounds, bool manual) {
  double bet;
  double *bet_p = &bet;
  
  int i;
  for (i = 0; i < rounds; i++) {
	p->score = p->cards_drawn = p->aces_drawn = p->aces_mitigated = 0;
    d->score = d->cards_drawn = d->aces_drawn = d->aces_mitigated = 0;
	
	manual ? (bet = get_bet()) : (bet = auto_bet(card_amt, card_num_p, count_p), printf("\nSimulating game #%d with bet: %.1lf$\n", i+1, bet));
    
    draw(d, c, card_num_p, count_p, card_amt);
    draw(p, c, card_num_p, count_p, card_amt);
	draw(p, c, card_num_p, count_p, card_amt);
	
	player_turn(p, d, c, bet_p, card_num_p, count_p, card_amt, manual);  
    dealer_turn(d, c, card_num_p, count_p, card_amt); 
	
    find_winner(p, d, bet);
	
	if (*count_p / ((card_amt - *card_num_p)/DECK_SIZE) < 0) {
	  shuffle(c, card_num_p, count_p, card_amt);
	}
  }
}

// Gives advice on which move to make and prompts the player to make a decision if manual play.
// Otherwise move is made automatically based on this advice.
void player_turn(player *p, player *d, card *c, double *bet_p, int *card_num_p, int *count_p, int card_amt, bool manual) {  
  char move = 'S';
  int x, y;
  
  y = d->score - 2;	
  x = strategy_table_lookup(p, c, card_num_p);
  
  if (p->score < 21) {
    manual ? ( advise_move(x, y), move = player_move(p, c, card_num_p) ) : ( move = strategy_table[x][y] );
  }

  if (move != 'S') {	
	if (move == 'P') {
      split_game(p, d, c, bet_p, card_num_p, count_p, card_amt, manual);
	  
	  x = strategy_table_lookup(p, c, card_num_p);
	  manual ? ( advise_move(x, y), move = player_move(p, c, card_num_p) ) : ( move = strategy_table[x][y] );
	}
	
	while (move == 'H') {
      draw(p, c, card_num_p, count_p, card_amt);
	  
	  if (p->score >= 21) {
	    break;
      }
      
	  x = strategy_table_lookup(p, c, card_num_p);
	  manual ? ( advise_move(x, y), move = player_move(p, c, card_num_p) ) : ( move = strategy_table[x][y] );  
	}
	
    if (move == 'D') {
      *bet_p = double_up(p, *bet_p);
	  draw(p, c, card_num_p, count_p, card_amt);
	}
	  
	if (move == 'Y') {
      *bet_p = surrender(p, *bet_p);
      d->score = 21;
    }
  }
}

void split_game(player *p, player *d, card *c, double *bet_p, int *card_num_p, int *count_p, int card_amt, bool manual) {
  int tmp_player_score = c[*card_num_p - 1].bj_value;
  int tmp_dealer_score = d->score;
  double tmp_bet = *bet_p;
  
  p->score = tmp_player_score;
  
  draw(p, c, card_num_p, count_p, card_amt);
  
  player_turn(p, d, c, bet_p, card_num_p, count_p, card_amt, manual);
  dealer_turn(d, c, card_num_p, count_p, card_amt); 
	  
  find_winner(p, d, *bet_p);
	  
  p->score = tmp_player_score;
  d->score = tmp_dealer_score;
  
  *bet_p = tmp_bet;
  
  draw(p, c, card_num_p, count_p, card_amt);
}  

void dealer_turn(player *d, card *c, int *card_num_p, int *count_p, int card_amt) {
  while (d->score < 17) {
    draw(d, c, card_num_p, count_p, card_amt);
	if (d->score >= 17 && d->score < 21) {
	  printf("\n%s stands\n", d->name);
	}
  }
}

void draw(player *p, card *c, int *card_num_p, int *count_p, int card_amt) {
  (p->cards_drawn)++;
  p->score += c[*card_num_p].bj_value;
  *count_p += c[*card_num_p].count_value;

  if (c[*card_num_p].bj_value == 11) {
    (p->aces_drawn)++;
  }
  
  mitigate_ace(p, c, card_num_p);
  
  printf("\n%s was dealt %s of %s, giving a score of %d",
         p->name, faces[c[*card_num_p].face], suits[c[*card_num_p].suit], p->score);
		 
  if (*count_p != 0) {
    printf(" and leaving the count at %d\n", *count_p);
  }
  
  else {
    printf("\n");
  }

  if (bust(p)) {
    printf("\n%s is bust\n", p->name);
  }
  
  if (blackjack(p)) {
    printf("\n%s has Blackjack!\n", p->name);
  }
  
  (*card_num_p)++;
  
  if (*card_num_p == card_amt - DECK_SIZE) {
    shuffle(c, card_num_p, count_p, card_amt);
    printf("\nShoe has been shuffled\n");
  }
}

void mitigate_ace(player *p, card *c, int *card_num_p) {
  if (p->score > 21 && p->aces_mitigated < p->aces_drawn) {
    p->score -= 10;
    (p->aces_mitigated)++;
  }
}

void advise_move(int x, int y) {
  char *advice = {"\nThe best odds of winning are attained by"};

  switch (strategy_table[x][y]) {
    case 'H':
      printf("%s hitting\n", advice);
	  break;
	case 'P':
	  printf("\n%s splitting\n", advice);
	  break;
	case 'D':
	  printf("%s doubling\n", advice);
	  break;
	case 'S':
	  printf("%s standing\n", advice);
	  break;
	case 'Y':
	  printf("%s surrendering\n", advice);
	  break;
	default:
	  printf("%s hitting\n", advice);
	  break;
  }
}

double surrender(player *p, double bet) {
  printf("\n%s surrendered, losing half the bet\n", p->name);
  
  bet = bet / 2;
  return bet;
}

// Bust if score excedes 21
bool bust (player *p) {
  if (p->score > 21) {
    return true;
  }
  return false; 
}

// On players turn, ask whether to hit, double, split, stand or surrender
char player_move(player *p, card *c, int *card_num_p) {
  char input, move;
  int last_card = *card_num_p - 1;
  
  do {
  	printf("\nEnter \"h\" to hit, \"d\" to double, \"p\" to split, \"s\" to stand or \"y\" to surrender\n");

    while ((input = getchar()) != '\n' && input != EOF) {
      move = input;
	  if (move == 'p' && !(p->cards_drawn == 2 && c[last_card].face == c[last_card - 1].face)) {
	    printf("\nSplitting is only allowed on hands of two cards with the same face\n");
		move = 0;
	  }
	}
  }	while (move != 'h' && move != 'd' && move != 'p' && move != 's' && move != 'y');

  return move += ASCII_LOWER_TO_UPPER_CASE;
}

bool blackjack(player *p) {
  if (p->score == 21) {
    if (p->cards_drawn == 2) {
	  return true;
	}
  }
  return false;
}

bool play_again() {
  char ch, replay;
  do {
    printf("\nEnter \"y\" to play again or \"n\" to return to menu\n"); 
    while ((ch = getchar()) != '\n' && ch != EOF) {
	  replay = ch;
	}
  }	while (replay != 'y' && replay != 'n');
  
  if (replay == 'y') {
    return true;
  }
  
  return false;
}

double get_bet() {
  double bet, input;  
  do {
    printf("\nEnter amount to bet (5$ - 250$, multiples of 5 only):\n");
    if (fgets(buf, sizeof(buf), stdin) != NULL)
      if (sscanf(buf, "%lf", &input))
        bet = input;
  } while (bet < 5 || bet > 250 || fmod(bet, 5) != 0);
  
  return bet;
}

// Double the bet and print string to inform that this has happened
double double_up(player *p, double bet) {
  printf("\n%s has chosen to double bet and draw exactly more card\n", p->name);
  
  bet = bet * 2;
  return bet;
}

void strategy_table_read() {
  FILE *file;
  file = fopen("basic-strategy.txt", "r");
  
  fscanf(file, "%*[^\n]\n");  // Ignore the first line in "basic-strategy.txt"

  int i, j;
  for (i = 0; i < NUM_INIT_HANDS_PLAYER + 1; i++) {
    fscanf(file, "%*s ");  // Ignore the first string of every line in "basic-strategy.txt"
    for (j = 0; j < NUM_INIT_HANDS_DEALER; j++) {
      fscanf(file, "%c ", &strategy_table[i][j]);
	}
  }
}

int strategy_table_lookup(player *p, card *c, int *card_num_p) {
  int first_card = *card_num_p - p->cards_drawn;
  int last_card = *card_num_p - 1;
  int x = p->score - 4;

  if (c[last_card].bj_value == c[last_card - 1].bj_value) {
	x = 23 + c[last_card].bj_value - 2;
  }

  int i, j;  // Iterates through all cards the player has drawn
  for (i = first_card; i <= last_card; i++) {
    if(c[i].bj_value == 11 && p->score <= 17) {
	  x = 17;
	  for (j = first_card; j <= last_card; j++) {
	    if(c[j].bj_value != 11) {
		  x += c[j].bj_value - 2;
		}
	  }
	}
  }
  return x;
}

int rounds_to_simulate() {
  int input, rounds;
  printf("\nEnter number of rounds to simulate (1 - 100.000):\n");
  do {
	if (fgets(buf, sizeof(buf), stdin) != NULL)
      if (sscanf(buf, "%d", &input))
	      rounds = input;
  } while (rounds < 0 || rounds > 100000);
  
  return rounds;
}

double auto_bet(int card_amt, int *card_num_p, int *count_p) {
  int true_count = *count_p / ((card_amt - *card_num_p)/DECK_SIZE);
 
  switch (true_count) {
    case 2:
      return 100;
    case 3:
      return 100;
    case 4:
      return 100;
    case 5:
      return 100;
    case 6:
      return 100;
    case 7:
      return 100;
    case 8:
      return 100;
    case 9:
      return 200;
    case 10:
      return 225;
    default:
      return 5;
	}
}

void find_winner(player *p, player *d, double bet) {
  if ((p->score > d->score && p->score <= 21) || (bust(d) && p->score <= 21)
  || (blackjack(p) && !blackjack(d))) {
    if (blackjack(p) && !blackjack(d)) {
	  bet = bet * 3 / 2;
	}
    p->winnings += bet;
	(p->won)++;
	printf("\n%s wins %.1lf$\n", p->name, bet);
  }
  
  else if ((d->score > p->score && d->score <= 21) || (bust(p) && d->score <= 21)
  || (blackjack(d) && !blackjack(p))) {
    printf("\n%s loses %.1lf$\n", p->name, bet);
	(p->lost)++;
	p->winnings -= bet;
  }
  
  else {
    printf("\nStandoff between %s and %s - bets are off\n", p->name, d->name);
	(p->tied)++;
  }
}

void store_stats(player *p) { 
  FILE *file;
  file = fopen("stats.txt", "a");
  fprintf(file, "Won: %d Lost: %d Tied: %d Winnings: %.1lf$\n", p->won, p->lost, p->tied, p->winnings);
  fclose(file);
  printf("\nStatistics have been appended to stats.txt\n");
  p->won = p->lost = p->tied = p->winnings = 0;
}

void get_stats(player *p) {
  p->total_won = p->total_lost = p->total_tied = p->total_winnings = 0;
  int temp_won, temp_lost, temp_tied, temp_winnings;
  
  FILE *file;
  file = fopen("stats.txt", "r");
  
  while (fscanf(file, "%*s %d %*s %d %*s %d %*s %d$",
         &temp_won, &temp_lost, &temp_tied, &temp_winnings) != EOF) {
    p->total_won += temp_won;
	p->total_lost += temp_lost;
	p->total_tied += temp_tied;
	p->total_winnings += temp_winnings;
  }
  fclose(file);
  printf("\nTotal games won: %d\nTotal games lost: %d\nTotal games tied: %d\nAll time winnings: %.1lf$\n",
         p->total_won, p->total_lost, p->total_tied, p->total_winnings);
		 
  getchar();
}
