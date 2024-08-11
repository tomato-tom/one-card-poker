#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <unistd.h>

#define DECK_SIZE 13
#define INITIAL_MONEY 100
#define ANTE 1
#define MIN_BET_CARD 9
#define MAX_BLUFF_CARD 3
#define MIN_CALL_CARD 6
#define CLEAR printf("\e[H\e[2J")
#define CYAN "\033[36m"
#define MAGENTA "\033[35m"
#define RESET "\033[39m"

const char *deck[DECK_SIZE] = {"2", "3", "4", "5", "6", "7", "8", "9", "T", "J", "Q", "K", "A"};
const char *suits[4] = {"♠", "♣", "♦", "♥"};

typedef enum {
    FOLD = -1,
    PASS = 0,  // check
    CALL = 1,
    BET  = 2
} Action;

typedef enum {
    PLAYER = 1,
    COMPUTER = 2,
} Player;

// ゲームの状態を表す構造体
typedef struct {
    int suit;
    int player_card;
    int computer_card;
    int player_bet;
    int computer_bet;
    int player_money;
    int computer_money;
    int pot;
    int winner;
} GameState;

void display_ui(GameState *state, const char *message);
void initialize_game(GameState *state);
void deal_card(GameState *state);  //new
void put_ante(GameState *state);   //new
void play_round(GameState *state);
Action get_player_action(GameState *state, Action computer_action);
Action get_computer_action(GameState *state, Action player_action);
void update_pot(GameState *state, Player player, Action action);
Player showdown(GameState *state);
void determine_winner(GameState *state);


int main() {
    srand(time(NULL));

    GameState state;
    state.player_money = INITIAL_MONEY;
    state.computer_money = INITIAL_MONEY;

    while (state.player_money > 0 && state.computer_money > 0) {
        initialize_game(&state);

        char *message = "ゲームを始めます\n";
        display_ui(&state, message);
        sleep(1);

        deal_card(&state);
        put_ante(&state);
        play_round(&state);
        determine_winner(&state);

        printf("次のラウンドを始めますか？ (y/n): ");
        char choice;
        scanf(" %c", &choice);
        if (choice == 'n' || choice == 'N') break;
    }

    printf("ゲーム終了\n");
    return 0;
}

void initialize_game(GameState *state) {
        state->winner = 0;
        state->pot = 0;
        state->player_bet = 0;
        state->computer_bet = 0;
        state->player_card = -1;
        state->computer_card = -1;
}

void display_ui(GameState *state, const char *message) {
    char playerCard[7];
    char computerCard[7];
    if (state->player_card == -1) {
        sprintf(playerCard, "   ");
    } else {
        sprintf(playerCard, "%s %s", deck[state->player_card], suits[state->suit]);
    }
    if (state->computer_card == -1) {
        sprintf(computerCard, "   ");
    } else if (!state->winner) {
        sprintf(computerCard, "* *");
    } else {
        sprintf(computerCard, "%s %s", deck[state->computer_card], suits[state->suit]);
    }

    CLEAR;
    printf("\a");
    printf("\n");
    printf("       One Card Poker ♠ ♣ ♡ ♢\n");
    printf("\n");
    printf("         Dealer ($%s%d%s)\n", MAGENTA, state->computer_money, RESET);
    printf("          %s[%s]%s   $%d\n", CYAN, computerCard, RESET, state->computer_bet);
    printf("\n");
    printf("\n");
    printf("          pot ($%s%d%s)\n", MAGENTA, state->pot, RESET);
    printf("\n");
    printf("\n");
    printf("          %s[%s]%s   $%d\n", CYAN, playerCard, RESET, state->player_bet);
    printf("         Player ($%s%d%s)\n", MAGENTA, state->player_money, RESET);
    printf("\n");
    printf("\n");
    printf("%s\n", message);
}

void deal_card(GameState *state) {
    char message[100];

    state->suit = rand() % 4;

    // playerのカードを１枚配る
    state->player_card = rand() % DECK_SIZE;
    sprintf(message, "あなたのカード: %s%s", deck[state->player_card], suits[state->suit]);
    display_ui(state, message);
    sleep(1);

    // 残りの１２枚からcomputerのカードを１枚配る
    int card = rand() % (DECK_SIZE - 1);
    if (card >= state->player_card) card++;
    state->computer_card = card;
    display_ui(state, message);
    sleep(1);
}

void put_ante(GameState *state) {
    char message[100];

    state->player_bet += ANTE;
    state->player_money -= ANTE;
    state->pot += ANTE;
    sprintf(message, "あなたはアンティ$1置きました");
    display_ui(state, message);
    sleep(1);

    state->computer_bet += ANTE;
    state->computer_money -= ANTE;
    state->pot += ANTE;
    sprintf(message, "ディーラーがアンティ$1置きました");
    display_ui(state, message);
    sleep(1);
}

void play_round(GameState *state) {
    char message[100];

    Action player_action = get_player_action(state, 0);
    sleep(1);

    if (player_action == FOLD) {
        state->winner = COMPUTER;
    } else {
        Action computer_action = get_computer_action(state, player_action);
        sleep(1);
        if (computer_action == FOLD) {
            state->winner = PLAYER;
        } else if (computer_action == BET && player_action != FOLD) {
            player_action = get_player_action(state, BET);
            sleep(1);
            if (player_action == FOLD) {
                state->winner = COMPUTER;
            }
        }
    }

}

Action get_player_action(GameState *state, Action computer_action) {
    char action;
    char *message;

    if (computer_action == BET) {
        if (!state->player_money) return FOLD;
        message = "アクションを選択してください (c: コール, f: フォールド): ";
    } else {
        if (!state->player_money) return PASS;
        message = "アクションを選択してください (p: パス, b: ベット): ";
    }

    display_ui(state, message);
    scanf(" %c", &action);

    switch (action) {
        case 'c': 
            message = "あなたはコールしました";
            update_pot(state, PLAYER, CALL);
            display_ui(state, message);
            return CALL;
        case 'f': 
            message = "あなたはフォールドしました";
            update_pot(state, PLAYER, FOLD);
            display_ui(state, message);
            return FOLD;
        case 'b': 
            message = "あなたはベットしました";
            update_pot(state, PLAYER, BET);
            display_ui(state, message);
            return BET;
        case 'p': 
            message = "あなたはパスしました";
            update_pot(state, PLAYER, PASS);
            display_ui(state, message);
            return PASS;
        default:
            message = "無効な入力はパスとして扱います";
            update_pot(state, PLAYER, PASS);
            display_ui(state, message);
            return PASS;
    }
}

Action get_computer_action(GameState *state, Action player_action) {
    char *message;

    if (player_action == BET) {
        if (!state->computer_money) return FOLD;
        if (state->computer_card >= MIN_CALL_CARD) {
        message = "コンピューターはコールしました\n";
        update_pot(state, COMPUTER, CALL);
        display_ui(state, message);
        return CALL;
        } else {
            message = "コンピューターはフォールドしました\n";
            update_pot(state, COMPUTER, FOLD);
            display_ui(state, message);
            return FOLD;
        }
    }

    if (player_action == PASS) {
        if (!state->computer_money) return PASS;
        if (state->computer_card >= MIN_BET_CARD || state->computer_card <= MAX_BLUFF_CARD) {
        message = "コンピューターはベットしました\n";
        update_pot(state, COMPUTER, BET);
        display_ui(state, message);
        return BET;
        } else {
        message = "コンピューターはパスしました\n";
        update_pot(state, COMPUTER, PASS);
        display_ui(state, message);
        return PASS;
        }
    }
}

void update_pot(GameState *state, Player player, Action action) {
    if (player == PLAYER && (action == BET || action == CALL)) {
        state->player_bet += 1;
        state->player_money -= 1;
        state->pot += 1;
    }
    if (player == COMPUTER && (action == BET || action == CALL)) {
        state->computer_bet += 1;
        state->computer_money -= 1;
        state->pot += 1;
    }
}

Player showdown(GameState *state) {
    if (state->player_card > state->computer_card) {
        return PLAYER;
    } else if (state->player_card < state->computer_card) {
        return COMPUTER;
    }
}

void determine_winner(GameState *state) {
    char message[100];

    if (state->winner == 0) {
        printf("コンピューターのカード: %s\n", deck[state->computer_card]);
        usleep(300000);
        printf("プレイヤーのカード: %s\n", deck[state->player_card]);
        usleep(300000);
        state->winner = showdown(state);
    }
    
    state->player_bet = 0;
    state->computer_bet = 0;
    int pot = state->pot;

    for (int chip = 1; chip <= pot; chip++) {
        if (state->winner == PLAYER) {
            sprintf(message, "あなたの勝ちです！ 獲得: $%d\n", chip);
            state->player_money++;
        } else if (state->winner == COMPUTER) {
            sprintf(message, "コンピューターの勝ちです。獲得: $%d\n", chip);
            state->computer_money++;
        }

        state->pot--;
        display_ui(state, message);
        usleep(300000);
    }
}
