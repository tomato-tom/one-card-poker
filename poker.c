#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <unistd.h>

#define DECK_SIZE 13
#define INITIAL_MONEY 100
#define ANTE 1
#define MIN_BET_CARD 8
#define MIN_CALL_CARD 6

const char *deck[DECK_SIZE] = {"2", "3", "4", "5", "6", "7", "8", "9", "T", "J", "Q", "K", "A"};
const char *suits[4] = {"♠", "♣", "♢", "♡"};

typedef enum {
    PASS = 0,
    BET = 1,
    CALL = 2,
    FOLD = -1
} Action;

typedef enum {
    PLAYER = 1,
    COMPUTER = 2,
    DRAW = 0
} Player;

// ゲームの状態を表す構造体
typedef struct {
    int suit;
    int player_card;
    int computer_card;
    int pot;
    int player_bet;
    int computer_bet;
    int player_money;
    int computer_money;
} GameState;

void display_ui(GameState *state, const char *message, int winner);
void initialize_game(GameState *state);
Action get_player_action(GameState *state, Action computer_action, int winner);
Action get_computer_action(GameState *state, Action player_action, int winner);
void update_pot(GameState *state, Player player, Action action);
Player determine_winner(GameState *state);
void play_round(GameState *state);


int main() {
    GameState state;
    srand(time(NULL));

    char *message = "ゲームを始めます\n";
    int winner = 0;
    initialize_game(&state);
    display_ui(&state, message, winner);

    while (state.player_money > 0 && state.computer_money > 0) {
        play_round(&state);
        printf("次のラウンドを始めますか？ (y/n): ");
        char choice;
        scanf(" %c", &choice);
        if (choice != 'y' && choice != 'Y') break;
    }

    printf("ゲーム終了\n");
    return 0;
}

void initialize_game(GameState *state) {
    state->player_money = INITIAL_MONEY;
    state->computer_money = INITIAL_MONEY;
    state->computer_card = -1;
    state->player_card = -1;
}

void display_ui(GameState *state, const char *message, int winner) {
    char playerCard[5];
    char computerCard[5];
    if (state->player_card == -1) {
        sprintf(playerCard, "  ");
    } else {
        sprintf(playerCard, "%s%s", deck[state->player_card], suits[state->suit]);
    }
    if (state->computer_card == -1) {
        sprintf(computerCard, "  ");
    } else if (!winner) {
        sprintf(computerCard, "**");
    } else {
        sprintf(computerCard, "%s%s", deck[state->computer_card], suits[state->suit]);
    }

    system("clear");
    printf("\n  One Card Poker ♠ ♣ ♡ ♢\n\n");
    printf("    Dealer ($%d)\n", state->computer_money);
    printf("     [%s]   $%d\n\n\n", computerCard, state->computer_bet);
    printf("       ($%d)\n\n\n", state->pot);
    printf("     [%s]   $%d\n", playerCard, state->player_bet);
    printf("    Player ($%d)\n\n", state->player_money);
    printf("%s\n", message);
}

void play_round(GameState *state) {
    state->player_card = rand() % DECK_SIZE;
    int card = rand() % (DECK_SIZE - 1);
    if (card >= state->player_card) card++;
    state->computer_card = card;
    state->suit = rand() % 4;

    state->pot = ANTE * 2;
    state->player_bet = ANTE;
    state->computer_bet = ANTE;
    state->player_money -= ANTE;
    state->computer_money -= ANTE;

    Player winner = 0;
    char message[100];
    sprintf(message, "");
    display_ui(state, message, winner);

    Action player_action = get_player_action(state, PASS, winner);

    if (player_action == FOLD) {
        winner = COMPUTER;
    } else {
        Action computer_action = get_computer_action(state, player_action, winner);
        sleep(1);
        if (computer_action == FOLD) {
            winner = PLAYER;
        } else if (computer_action == BET && player_action != FOLD) {
            player_action = get_player_action(state, BET, winner);
            if (player_action == FOLD) {
                winner = COMPUTER;
            }
        }
    }

    if (winner == 0) {
        winner = determine_winner(state);
    }

    if (winner == PLAYER) {
        sprintf(message, "あなたの勝ちです！ 獲得: $%d\n", state->pot);
        state->player_money += state->pot;
    } else if (winner == COMPUTER) {
        sprintf(message, "コンピューターの勝ちです。獲得: $%d\n", state->pot);
        state->computer_money += state->pot;
    } else {
        sprintf(message, "引き分けです。\n");
        state->player_money += state->pot / 2;
        state->computer_money += state->pot / 2;
    }
    display_ui(state, message, winner);
}

Action get_player_action(GameState *state, Action computer_action, int winner) {
    char action;
    char *message;
    if (computer_action == BET) {
        if (!state->player_money) return FOLD;
        printf("アクションを選択してください (c: コール, f: フォールド): ");
    } else {
        if (!state->player_money) return PASS;
        printf("アクションを選択してください (p: パス, b: ベット): ");
    }

    scanf(" %c", &action);

    switch (action) {
        case 'c': 
            message = "あなたはコールしました\n";
            update_pot(state, PLAYER, CALL);
            display_ui(state, message, winner);
            return CALL;
        case 'f': 
            message = "あなたはフォールドしました\n";
            update_pot(state, PLAYER, FOLD);
            display_ui(state, message, winner);
            return FOLD;
        case 'b': 
            message = "あなたはベットしました\n";
            update_pot(state, PLAYER, BET);
            display_ui(state, message, winner);
            return BET;
        case 'p': 
            message = "あなたはパスしました\n";
            update_pot(state, PLAYER, PASS);
            display_ui(state, message, winner);
            return PASS;
        default:
            message = "無効な入力です。パスとして扱います。\n";
            update_pot(state, PLAYER, PASS);
            display_ui(state, message, winner);
            return PASS;
    }
}

Action get_computer_action(GameState *state, Action player_action, int winner) {
    // 簡単な戦略: 8以上のカードを持っている場合、またはプレイヤーがパスした場合にベット
    char *message;
    if (player_action == BET) {
        if (!state->computer_money) return FOLD;
        if (state->computer_card >= MIN_CALL_CARD) {
        message = "コンピューターはコールしました\n";
        update_pot(state, COMPUTER, CALL);
        display_ui(state, message, winner);
        return CALL;
        } else {
            message = "コンピューターはフォールドしました\n";
            update_pot(state, COMPUTER, FOLD);
            display_ui(state, message, winner);
            return FOLD;
        }
    }
    if (player_action == PASS) {
        if (!state->computer_money) return PASS;
        if (state->computer_card >= MIN_BET_CARD) {
        message = "コンピューターはベットしました\n";
        update_pot(state, COMPUTER, BET);
        display_ui(state, message, winner);
        return BET;
        } else {
        message = "コンピューターはパスしました\n";
        update_pot(state, COMPUTER, PASS);
        display_ui(state, message, winner);
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

Player determine_winner(GameState *state) {
    printf("コンピューターのカード: %s\n", deck[state->computer_card]);

    if (state->player_card > state->computer_card) {
        return PLAYER;
    } else if (state->player_card < state->computer_card) {
        return COMPUTER;
    }
    return DRAW;
}
