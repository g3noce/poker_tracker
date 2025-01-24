#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <windows.h>
#include <dirent.h>

#define COLOR_DEFAULT 7
#define COLOR_VPIP 12    // Rouge clair
#define COLOR_PFR 10     // Vert clair
#define COLOR_RFI 9      // Bleu clair
#define COLOR_3BET 13    // Magenta clair

typedef struct PlayerStat {
    char name[50];
    int seat;
    int hands;
    int vpip_count;
    int pfr_count;
    int rfi_count;
    int threebet_count;
    int current_vpip;
    int current_pfr;
    int current_rfi;
    int current_threebet;
    int current_position;
    int absence_count;
    struct PlayerStat *next;
} PlayerStat;

typedef struct {
    char name[50];
    int seat;
} PlayerInfo;

// Prototypes
unsigned long long extract_hand_id(const char *line);
void skip_hand(FILE *file);
void extract_table_name(const char *line, char *table_name);
void parse_seat_line(const char *line, PlayerInfo *current_players, int *num_players);
int calculate_position(int seat, int button_seat);
const char* position_to_str(int position);

PlayerStat *find_player(PlayerStat *head, const char *name) {
    while (head != NULL) {
        if (strcmp(head->name, name) == 0) return head;
        head = head->next;
    }
    return NULL;
}

void add_player(PlayerStat **head, const char *name, int seat) {
    PlayerStat *new_player = malloc(sizeof(PlayerStat));
    snprintf(new_player->name, sizeof(new_player->name), "%s", name);
    new_player->seat = seat;
    new_player->hands = 0;
    new_player->vpip_count = 0;
    new_player->pfr_count = 0;
    new_player->rfi_count = 0;
    new_player->threebet_count = 0;
    new_player->current_vpip = 0;
    new_player->current_pfr = 0;
    new_player->current_rfi = 0;
    new_player->current_threebet = 0;
    new_player->current_position = -1;
    new_player->absence_count = 0;
    new_player->next = *head;
    *head = new_player;
}

void remove_players_not_in_current(PlayerStat **head, PlayerInfo *current_players, int num_players) {
    PlayerStat **current = head;
    const int max_absences = 3;

    while (*current) {
        bool found = false;
        for (int i = 0; i < num_players; i++) {
            if (strcmp((*current)->name, current_players[i].name) == 0) {
                found = true;
                break;
            }
        }

        if (found) {
            (*current)->absence_count = 0;
            current = &(*current)->next;
        } else {
            (*current)->absence_count += 1;
            if ((*current)->absence_count >= max_absences) {
                PlayerStat *temp = *current;
                *current = temp->next;
                free(temp);
            } else {
                current = &(*current)->next;
            }
        }
    }
}

void parse_seat_line(const char *line, PlayerInfo *current_players, int *num_players) {
    int seat;
    char player_name[50] = {0};
    
    if (sscanf(line, "Seat %d:", &seat) != 1) return;
    
    const char *start = strchr(line, ':') + 1;
    while (*start && isspace(*start)) start++;
    
    const char *end = strchr(start, '(');
    if (!end) return;
    
    size_t name_len = end - start;
    if (name_len >= sizeof(player_name) - 1)
        name_len = sizeof(player_name) - 1;
    strncpy(player_name, start, name_len);
    player_name[name_len] = '\0';
    
    if (strlen(player_name) > 0) {
        char *trim = player_name + strlen(player_name) - 1;
        while (trim >= player_name && isspace(*trim)) trim--;
        *(trim + 1) = '\0';
    }
    
    if (!strstr(line, "is sitting out") && *num_players < 10) {
        strncpy(current_players[*num_players].name, player_name, sizeof(current_players[*num_players].name));
        current_players[*num_players].seat = seat;
        (*num_players)++;
    }
}

unsigned long long extract_hand_id(const char *line) {
    const char *p = strstr(line, "PokerStars Hand #");
    if (!p) return 0;
    p += strlen("PokerStars Hand #");
    unsigned long long id = 0;
    while (*p && isdigit(*p)) {
        id = id * 10 + (*p - '0');
        p++;
    }
    return id;
}

void skip_hand(FILE *file) {
    char line[1024];
    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, "PokerStars Hand #")) {
            fseek(file, -strlen(line), SEEK_CUR);
            break;
        }
    }
}

void extract_table_name(const char *line, char *table_name) {
    const char *start = strchr(line, '\'') + 1;
    if (!start) return;
    const char *end = strchr(start, '\'');
    if (!end) return;
    size_t name_len = end - start;
    if (name_len >= 255) name_len = 254;
    strncpy(table_name, start, name_len);
    table_name[name_len] = '\0';
}

int calculate_position(int seat, int button_seat) {
    return (seat - button_seat + 6) % 6;
}

const char* position_to_str(int position) {
    switch(position) {
        case 0: return "BTN";
        case 1: return "SB";
        case 2: return "BB";
        case 3: return "UTG";
        case 4: return "MP";
        case 5: return "CO";
        default: return "UNK";
    }
}

void process_hand(FILE *file, PlayerStat **global_head, char *current_table_name) {
    char line[1024];
    PlayerInfo current_players[10] = {0};
    int num_players = 0;
    bool limped_preflop = false;
    int preflop_raises = 0;
    int button_seat = 0;

    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, "*** HOLE CARDS ***")) break;
        if (strncmp(line, "Seat ", 5) == 0) parse_seat_line(line, current_players, &num_players);
        if (strstr(line, "Table '")) extract_table_name(line, current_table_name);
        if (strstr(line, "is the button")) {
            const char *seat_start = strstr(line, "Seat #");
            if (seat_start) button_seat = atoi(seat_start + 6);
        }
    }

    for (int i = 0; i < num_players; i++) {
        PlayerStat *player = find_player(*global_head, current_players[i].name);
        if (!player) {
            add_player(global_head, current_players[i].name, current_players[i].seat);
            player = *global_head;
        }
        player->current_position = calculate_position(current_players[i].seat, button_seat);
    }

    remove_players_not_in_current(global_head, current_players, num_players);

    PlayerStat *current = *global_head;
    while (current) {
        current->hands++;
        current->current_vpip = 0;
        current->current_pfr = 0;
        current->current_rfi = 0;
        current->current_threebet = 0;
        current = current->next;
    }

    while (fgets(line, sizeof(line), file) && !strstr(line, "*** FLOP ***")) {
        char *colon = strchr(line, ':');
        if (!colon) continue;
        *colon = '\0';
        
        char *player_name = line;
        char *action = colon + 1;
        while (isspace(*player_name)) player_name++;
        char *end_name = player_name + strlen(player_name) - 1;
        while (end_name > player_name && isspace(*end_name)) end_name--;
        *(end_name + 1) = '\0';

        PlayerStat *player = find_player(*global_head, player_name);
        if (!player) continue;

        if (strstr(action, "calls")) {
            player->current_vpip = 1;
            limped_preflop = true;
        }
        else if (strstr(action, "bets") || strstr(action, "raises")) {
            player->current_vpip = 1;
            player->current_pfr = 1;

            if (strstr(action, "raises")) {
                if (preflop_raises == 0 && !limped_preflop) {
                    if (player->current_position == 0 || player->current_position >= 3) {
                        player->current_rfi = 1;
                    }
                }
                if (preflop_raises >= 1) {
                    player->current_threebet = 1;
                }
                preflop_raises++;
            }
        }
    }

    current = *global_head;
    while (current) {
        current->vpip_count += current->current_vpip;
        current->pfr_count += current->current_pfr;
        current->rfi_count += current->current_rfi;
        current->threebet_count += current->current_threebet;
        current = current->next;
    }
}

void print_results(PlayerStat *head) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    
    printf("Joueur          Mains  VPIP   PFR    RFI    3Bet\n");
    printf("-------------------------------------------------\n");
    
    while (head) {
        float vpip = head->hands ? (head->vpip_count * 100.0f) / head->hands : 0;
        float pfr = head->hands ? (head->pfr_count * 100.0f) / head->hands : 0;
        float rfi = head->hands ? (head->rfi_count * 100.0f) / head->hands : 0;
        float threebet = head->hands ? (head->threebet_count * 100.0f) / head->hands : 0;

        // Nom et mains
        printf("%-15s %-5d ", head->name, head->hands);

        // VPIP
        SetConsoleTextAttribute(hConsole, COLOR_VPIP);
        printf("%-6.1f%%", vpip);
        SetConsoleTextAttribute(hConsole, COLOR_DEFAULT);
        printf(" ");

        // PFR
        SetConsoleTextAttribute(hConsole, COLOR_PFR);
        printf("%-6.1f%%", pfr);
        SetConsoleTextAttribute(hConsole, COLOR_DEFAULT);
        printf(" ");

        // RFI
        SetConsoleTextAttribute(hConsole, COLOR_RFI);
        printf("%-6.1f%%", rfi);
        SetConsoleTextAttribute(hConsole, COLOR_DEFAULT);
        printf(" ");

        // 3Bet
        SetConsoleTextAttribute(hConsole, COLOR_3BET);
        printf("%-6.1f%%", threebet);
        SetConsoleTextAttribute(hConsole, COLOR_DEFAULT);
        
        printf("\n");
        head = head->next;
    }
}

void free_players(PlayerStat *head) {
    while (head) {
        PlayerStat *temp = head;
        head = head->next;
        free(temp);
    }
}

char* get_latest_handhistory(const char *player_folder) {
    char path[MAX_PATH];
    snprintf(path, MAX_PATH, "C:\\Users\\user\\AppData\\Local\\PokerStars.FR\\HandHistory\\%s\\*.txt", player_folder);
    
    WIN32_FIND_DATA ffd;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    char *latest_file = NULL;
    FILETIME latest_time = {0};

    hFind = FindFirstFile(path, &ffd);
    if (hFind == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Aucun fichier d'historique trouve\n");
        return NULL;
    }

    do {
        if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            if (CompareFileTime(&ffd.ftLastWriteTime, &latest_time) > 0) {
                latest_time = ffd.ftLastWriteTime;
                if (latest_file) free(latest_file);
                latest_file = _strdup(ffd.cFileName);
            }
        }
    } while (FindNextFile(hFind, &ffd) != 0);

    FindClose(hFind);

    if (latest_file) {
        char full_path[MAX_PATH];
        snprintf(full_path, MAX_PATH, 
                 "C:\\Users\\user\\AppData\\Local\\PokerStars.FR\\HandHistory\\%s\\%s", 
                 player_folder, latest_file);
        free(latest_file);
        return _strdup(full_path);
    }

    return NULL;
}

int select_platform() {
    int choice = 0;
    printf("Selectionnez la plateforme :\n");
    printf("1. PokerStars\n");
    // Ajoutez d'autres plateformes ici si necessaire
    printf("Votre choix : ");
    scanf("%d", &choice);
    return choice;
}

char* select_player_folder() {
    WIN32_FIND_DATA ffd;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    char path[MAX_PATH] = "C:\\Users\\user\\AppData\\Local\\PokerStars.FR\\HandHistory\\*";
    char *selected_folder = NULL;
    char *folders[100];
    int count = 0;

    hFind = FindFirstFile(path, &ffd);
    if (hFind == INVALID_HANDLE_VALUE) {
        perror("Repertoire non trouve");
        return NULL;
    }

    printf("Selectionnez l'identifiant du joueur :\n");
    do {
        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY &&
            strcmp(ffd.cFileName, ".") != 0 &&
            strcmp(ffd.cFileName, "..") != 0) {
            folders[count] = _strdup(ffd.cFileName);
            printf("%d. %s\n", count + 1, folders[count]);
            count++;
        }
    } while (FindNextFile(hFind, &ffd) != 0 && count < 100);

    FindClose(hFind);

    if (count > 0) {
        int choice;
        printf("Votre choix : ");
        scanf("%d", &choice);
        if (choice > 0 && choice <= count) {
            selected_folder = _strdup(folders[choice - 1]);
        }
    }

    for (int i = 0; i < count; i++) free(folders[i]);
    return selected_folder;
}

int main() {
    PlayerStat *global_head = NULL;
    unsigned long long last_hand_id = 0;
    static char current_table_name[256] = {0};

    int platform = select_platform();
    if (platform != 1) {
        printf("Plateforme non supportee.\n");
        return 1;
    }

    char *player_folder = select_player_folder();
    if (!player_folder) {
        printf("Aucun dossier de joueur selectionne.\n");
        return 1;
    }

    while (1) {
        char *filename = get_latest_handhistory(player_folder);
        if (!filename) {
            Sleep(3000);
            continue;
        }

        FILE *file = fopen(filename, "rb");
        if (!file) {
            free(filename);
            Sleep(3000);
            continue;
        }

        unsigned char bom[3];
        int bom_skipped = 0;
        if (fread(bom, 1, 3, file) == 3 && 
            bom[0] == 0xEF && bom[1] == 0xBB && bom[2] == 0xBF) {
            bom_skipped = 1;
        } else {
            fseek(file, 0, SEEK_SET);
        }

        char detected_table_name[256] = {0};
        char line[1024];
        long initial_pos = ftell(file);
        while (fgets(line, sizeof(line), file)) {
            if (strstr(line, "Table '")) {
                extract_table_name(line, detected_table_name);
                break;
            } else if (strstr(line, "*** HOLE CARDS ***")) {
                break;
            }
        }
        fseek(file, initial_pos, SEEK_SET);

        if (strlen(detected_table_name) > 0 && strcmp(detected_table_name, current_table_name) != 0) {
            free_players(global_head);
            global_head = NULL;
            strcpy(current_table_name, detected_table_name);
        }

        while (fgets(line, sizeof(line), file)) {
            if (strstr(line, "PokerStars Hand #")) {
                unsigned long long current_id = extract_hand_id(line);
                
                if (current_id > last_hand_id) {
                    char table_name_buffer[256] = {0};
                    process_hand(file, &global_head, table_name_buffer);
                    last_hand_id = current_id;
                } else {
                    skip_hand(file);
                }
            }
        }

        fclose(file);
        free(filename);

        system("cls");
        print_results(global_head);
        
        Sleep(3000);
    }

    free_players(global_head);
    free(player_folder);
    return 0;
}