#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Arena Master - Full single-file program (teams, matches, round-robin, knockout, save/load)
// Input handling fixed (flushes newline before fgets)

// -------------------- Data Structures --------------------
typedef struct Team {
    int id;
    char name[64];
    int played;
    int wins;
    int losses;
    int draws;
    int points;
    struct Team *next;
} Team;

typedef struct Match {
    int id;
    char team1[64];
    char team2[64];
    int score1;
    int score2;
    int played; // 0/1
    struct Match *next;
} Match;

typedef struct KNode {
    char team[64];
    struct KNode *left;
    struct KNode *right;
} KNode;

// global heads and ids
Team *teamHead = NULL;
Match *matchHead = NULL;
int nextTeamId = 1;
int nextMatchId = 1;

// -------------------- Utilities --------------------
// trim newline at end
static void trim_newline(char *s) {
    if (!s) return;
    size_t n = strlen(s);
    while (n > 0 && (s[n-1] == '\n' || s[n-1] == '\r')) s[--n] = 0;
}

// sanitize leading/trailing spaces
static void sanitize_string(char *s) {
    if (!s) return;
    // trim leading
    size_t i = 0;
    while (s[i] && isspace((unsigned char)s[i])) i++;
    if (i > 0) memmove(s, s + i, strlen(s + i) + 1);
    // trim trailing
    size_t n = strlen(s);
    while (n > 0 && isspace((unsigned char)s[n-1])) s[--n] = 0;
}

// case-insensitive trimmed compare
int team_name_equal(const char *a, const char *b) {
    if (!a || !b) return 0;
    char ta[64] = {0}, tb[64] = {0};
    int ia = 0, ib = 0;
    // copy lower-case without internal spaces for comparison
    for (int i=0; a[i] && ia < 63; ++i) {
        if (!isspace((unsigned char)a[i])) ta[ia++] = tolower((unsigned char)a[i]);
    }
    ta[ia] = 0;
    for (int i=0; b[i] && ib < 63; ++i) {
        if (!isspace((unsigned char)b[i])) tb[ib++] = tolower((unsigned char)b[i]);
    }
    tb[ib] = 0;
    return strcmp(ta, tb) == 0;
}

// -------------------- Memory management --------------------
void free_team_list() {
    Team *cur = teamHead;
    while (cur) {
        Team *n = cur->next;
        free(cur);
        cur = n;
    }
    teamHead = NULL;
    nextTeamId = 1;
}

void free_match_list() {
    Match *cur = matchHead;
    while (cur) {
        Match *n = cur->next;
        free(cur);
        cur = n;
    }
    matchHead = NULL;
    nextMatchId = 1;
}

void free_knockout_tree(KNode *node) {
    if (!node) return;
    free_knockout_tree(node->left);
    free_knockout_tree(node->right);
    free(node);
}

// -------------------- File storage --------------------
void save_to_files(const char *teamfile, const char *matchfile) {
    FILE *tf = fopen(teamfile, "w");
    if (!tf) { printf("Error opening team file for write.\n"); return; }
    Team *t = teamHead;
    while (t) {
        fprintf(tf, "%d|%s|%d|%d|%d|%d|%d\n",
                t->id, t->name, t->played, t->wins, t->draws, t->losses, t->points);
        t = t->next;
    }
    fclose(tf);
    FILE *mf = fopen(matchfile, "w");
    if (!mf) { printf("Error opening match file for write.\n"); return; }
    Match *m = matchHead;
    while (m) {
        fprintf(mf, "%d|%s|%s|%d|%d|%d\n",
                m->id, m->team1, m->team2, m->score1, m->score2, m->played);
        m = m->next;
    }
    fclose(mf);
    printf("Saved teams to '%s' and matches to '%s'\n", teamfile, matchfile);
}

void load_from_files(const char *teamfile, const char *matchfile) {
    free_team_list();
    free_match_list();
    FILE *tf = fopen(teamfile, "r");
    if (tf) {
        Team *last = NULL;
        char line[256];
        while (fgets(line, sizeof(line), tf)) {
            trim_newline(line);
            int id, pl, w, d, l, pts;
            char name[64];
            if (sscanf(line, "%d|%63[^|]|%d|%d|%d|%d|%d",
                       &id, name, &pl, &w, &d, &l, &pts) == 7) {
                Team *t = malloc(sizeof(Team));
                t->id = id;
                strncpy(t->name, name, 63); t->name[63]=0;
                t->played = pl; t->wins = w; t->draws = d; t->losses = l; t->points = pts;
                t->next = NULL;
                if (!teamHead) teamHead = t; else last->next = t;
                last = t;
                if (id >= nextTeamId) nextTeamId = id + 1;
            }
        }
        fclose(tf);
    }
    FILE *mf = fopen(matchfile, "r");
    if (mf) {
        Match *last = NULL;
        char line[256];
        while (fgets(line, sizeof(line), mf)) {
            trim_newline(line);
            int id, s1, s2, played;
            char a[64], b[64];
            if (sscanf(line, "%d|%63[^|]|%63[^|]|%d|%d|%d",
                       &id, a, b, &s1, &s2, &played) == 6) {
                Match *m = malloc(sizeof(Match));
                m->id = id;
                strncpy(m->team1, a, 63); m->team1[63]=0;
                strncpy(m->team2, b, 63); m->team2[63]=0;
                m->score1 = s1; m->score2 = s2; m->played = played;
                m->next = NULL;
                if (!matchHead) matchHead = m; else last->next = m;
                last = m;
                if (id >= nextMatchId) nextMatchId = id + 1;
            }
        }
        fclose(mf);
    }
    //printf("Loaded data (if files existed).\n");
}

// -------------------- Team operations --------------------
Team* find_team_by_name(const char *name) {
    Team *t = teamHead;
    while (t) {
        if (team_name_equal(t->name, name)) return t;
        t = t->next;
    }
    return NULL;
}

Team* find_team_by_id(int id) {
    Team *t = teamHead;
    while (t) {
        if (t->id == id) return t;
        t = t->next;
    }
    return NULL;
}

void add_team(const char *name_raw) {
    char name[64];
    strncpy(name, name_raw, 63); name[63]=0;
    sanitize_string(name);
    if (strlen(name) == 0) { printf("Name empty.\n"); return; }
    if (find_team_by_name(name)) { printf("Team with name '%s' already exists.\n", name); return; }
    Team *t = malloc(sizeof(Team));
    t->id = nextTeamId++;
    strncpy(t->name, name, 63);
    t->name[63] = 0;
    t->played = t->wins = t->losses = t->draws = t->points = 0;
    t->next = teamHead;
    teamHead = t;
    printf("Added team '%s' with id %d\n", t->name, t->id);
}

void delete_team_by_name(const char *name_raw) {
    char name[64]; strncpy(name, name_raw, 63); name[63]=0; sanitize_string(name);
    Team *cur = teamHead, *prev = NULL;
    while (cur) {
        if (team_name_equal(cur->name, name)) {
            if (prev) prev->next = cur->next; else teamHead = cur->next;
            free(cur);
            printf("Deleted team '%s'\n", name);
            return;
        }
        prev = cur; cur = cur->next;
    }
    printf("Team '%s' not found.\n", name);
}

void list_teams() {
    if (!teamHead) { printf("No teams registered.\n"); return; }
    Team *t = teamHead;
    printf("Teams:\n");
    printf("%-4s %-20s %6s %6s %6s %6s %6s\n", "ID", "Name", "Pl", "W", "D", "L", "Pts");
    while (t) {
        printf("%-4d %-20s %6d %6d %6d %6d %6d\n",
               t->id, t->name, t->played, t->wins, t->draws, t->losses, t->points);
        t = t->next;
    }
}

// -------------------- Match operations --------------------
int match_exists(const char *a, const char *b) {
    Match *m = matchHead;
    while (m) {
        if ((team_name_equal(m->team1, a) && team_name_equal(m->team2, b)) ||
            (team_name_equal(m->team1, b) && team_name_equal(m->team2, a))) return 1;
        m = m->next;
    }
    return 0;
}

void append_match(const char *a_raw, const char *b_raw) {
    char a[64], b[64];
    strncpy(a, a_raw, 63); a[63]=0; sanitize_string(a);
    strncpy(b, b_raw, 63); b[63]=0; sanitize_string(b);
    if (match_exists(a,b)) return;
    Match *m = malloc(sizeof(Match));
    m->id = nextMatchId++;
    strncpy(m->team1, a, 63); m->team1[63]=0;
    strncpy(m->team2, b, 63); m->team2[63]=0;
    m->score1 = m->score2 = 0; m->played = 0; m->next = NULL;
    if (!matchHead) matchHead = m; else {
        Match *p = matchHead; while (p->next) p = p->next; p->next = m;
    }
}

void clear_matches() { free_match_list(); nextMatchId = 1; }

void generate_round_robin() {
    int n = 0; Team *t = teamHead;
    while (t) { n++; t = t->next; }
    if (n < 2) { printf("Need at least 2 teams to generate fixtures.\n"); return; }
    clear_matches();
    Team **arr = malloc(n * sizeof(Team*));
    t = teamHead;
    for (int i = 0; i < n; ++i) { arr[i] = t; t = t->next; }
    for (int i = 0; i < n; ++i)
        for (int j = i+1; j < n; ++j)
            append_match(arr[i]->name, arr[j]->name);
    free(arr);
    printf("Round-robin fixtures generated (%d matches).\n", nextMatchId - 1);
}

void list_fixtures() {
    if (!matchHead) { printf("No fixtures.\n"); return; }
    Match *m = matchHead;
    printf("Fixtures:\n");
    printf("%-4s %-20s vs %-20s %-6s\n", "ID", "Team1", "Team2", "Result");
    while (m) {
        if (m->played)
            printf("%-4d %-20s vs %-20s %2d-%-2d\n",
                   m->id, m->team1, m->team2, m->score1, m->score2);
        else
            printf("%-4d %-20s vs %-20s    -\n",
                   m->id, m->team1, m->team2);
        m = m->next;
    }
}

// -------------------- Points update & record --------------------
void update_team_stats_after_match(const char *a, const char *b, int sa, int sb) {
    Team *ta = find_team_by_name(a);
    Team *tb = find_team_by_name(b);
    if (!ta || !tb) return;
    ta->played++; tb->played++;
    if (sa > sb) { ta->wins++; ta->points += 3; tb->losses++; }
    else if (sb > sa) { tb->wins++; tb->points += 3; ta->losses++; }
    else { ta->draws++; tb->draws++; ta->points += 1; tb->points += 1; }
}

void record_match_result(int matchId, int s1, int s2) {
    Match *m = matchHead;
    while (m) {
        if (m->id == matchId) {
            if (m->played) { printf("Match already recorded.\n"); return; }
            Team *ta = find_team_by_name(m->team1);
            Team *tb = find_team_by_name(m->team2);
            if (!ta || !tb) { printf("One of the teams not found.\n"); return; }
            m->score1 = s1; m->score2 = s2; m->played = 1;
            update_team_stats_after_match(m->team1, m->team2, s1, s2);
            printf("Result recorded: %s %d - %d %s\n", m->team1, s1, s2, m->team2);
            return;
        }
        m = m->next;
    }
    printf("Match id %d not found.\n", matchId);
}

// -------------------- Sorting & Points table --------------------
int team_count() {
    int c = 0; Team *t = teamHead; while (t) { c++; t = t->next; } return c;
}

Team** teams_to_array(int *outN) {
    int n = team_count();
    *outN = n;
    if (n == 0) return NULL;
    Team **arr = malloc(n * sizeof(Team*));
    Team *t = teamHead; int i = 0;
    while (t) { arr[i++] = t; t = t->next; }
    return arr;
}

int cmp_team_ptr(const void *a, const void *b) {
    Team *ta = *(Team**)a;
    Team *tb = *(Team**)b;
    if (ta->points != tb->points) return tb->points - ta->points;
    if (ta->wins != tb->wins) return tb->wins - ta->wins;
    return strcmp(ta->name, tb->name);
}

void display_points_table() {
    int n; Team **arr = teams_to_array(&n);
    if (!arr) { printf("No teams.\n"); return; }
    qsort(arr, n, sizeof(Team*), cmp_team_ptr);
    printf("Points Table:\n");
    printf("%-4s %-20s %6s %6s %6s %6s %6s\n", "Pos", "Name", "Pl", "W", "D", "L", "Pts");
    for (int i = 0; i < n; ++i) {
        Team *t = arr[i];
        printf("%-4d %-20s %6d %6d %6d %6d %6d\n",
               i+1, t->name, t->played, t->wins, t->draws, t->losses, t->points);
    }
    free(arr);
}

// -------------------- Knockout --------------------
KNode* new_knode(const char *name) {
    KNode *n = malloc(sizeof(KNode));
    strncpy(n->team, name, 63); n->team[63]=0;
    n->left = n->right = NULL;
    return n;
}

KNode* build_knockout_tree(char teams[][64], int count) {
    if (count == 0) return NULL;
    if (count == 1) return new_knode(teams[0]);
    int mid = count / 2;
    KNode *root = new_knode("");
    root->left = build_knockout_tree(teams, mid);
    root->right = build_knockout_tree(teams + mid, count - mid);
    return root;
}

char* simulate_knockout(KNode *node) {
    if (!node) return NULL;
    if (!node->left && !node->right) return node->team;
    char *l = simulate_knockout(node->left);
    char *r = simulate_knockout(node->right);
    printf("Match: %s vs %s\n", l, r);
    int s1 = 0, s2 = 0;
    printf("Enter score for %s: ", l);
    if (scanf("%d", &s1) != 1) { s1 = 0; while (getchar() != '\n'); }
    printf("Enter score for %s: ", r);
    if (scanf("%d", &s2) != 1) { s2 = 0; while (getchar() != '\n'); }
    if (s1 > s2) { strncpy(node->team, l, 63); node->team[63]=0; printf("Winner: %s\n", l); return node->team; }
    else if (s2 > s1) { strncpy(node->team, r, 63); node->team[63]=0; printf("Winner: %s\n", r); return node->team; }
    else { strncpy(node->team, l, 63); node->team[63]=0; printf("Tie-breaker winner chosen: %s\n", l); return node->team; }
}

void build_and_run_knockout(int k) {
    int n; Team **arr = teams_to_array(&n);
    if (!arr || n < k || k < 2) {
        printf("Not enough teams to create knockout of size %d.\n", k);
        if (arr) free(arr);
        return;
    }
    qsort(arr, n, sizeof(Team*), cmp_team_ptr);
    char (*topk)[64] = malloc(k * 64);
    for (int i = 0; i < k; ++i) strncpy(topk[i], arr[i]->name, 63);
    KNode *root = build_knockout_tree(topk, k);
    char *winner = simulate_knockout(root);
    printf("Champion: %s\n", winner ? winner : "N/A");
    free_knockout_tree(root);
    free(topk); free(arr);
}

// -------------------- Reset --------------------
void reset_all_stats() {
    Team *t = teamHead;
    while (t) {
        t->played = t->wins = t->losses = t->draws = t->points = 0;
        t = t->next;
    }
    Match *m = matchHead;
    while (m) {
        m->score1 = m->score2 = 0;
        m->played = 0;
        m = m->next;
    }
    printf("All stats reset.\n");
}

// -------------------- Menu & I/O fix --------------------
void print_menu() {
    printf("\n=== Arena Master ===\n");
    printf("1. Add team\n");
    printf("2. Delete team\n");
    printf("3. List teams\n");
    printf("4. Generate round-robin fixtures\n");
    printf("5. List fixtures\n");
    printf("6. Record match result\n");
    printf("7. Display points table\n");
    printf("8. Save data\n");
    printf("9. Load data\n");
    printf("10. Create and run knockout (top K teams)\n");
    printf("11. Reset all stats & fixtures\n");
    printf("0. Exit\n");
    printf("Enter choice: ");
}

// safe input: clear leftover newline before fgets
static void flush_input_line() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

int main() {
    // try loading previous data
    load_from_files("teams.txt", "matches.txt");

    int choice;
    while (1) {
        print_menu();
        if (scanf("%d", &choice) != 1) { flush_input_line(); printf("Invalid input.\n"); continue; }
        if (choice == 0) break;
        if (choice == 1) {
            char name[64];
            flush_input_line(); // fix: remove leftover newline
            printf("Enter team name: ");
            if (!fgets(name, sizeof(name), stdin)) continue;
            trim_newline(name);
            sanitize_string(name);
            if (strlen(name) == 0) { printf("Name empty.\n"); continue; }
            add_team(name);
        } else if (choice == 2) {
            char name[64];
            flush_input_line();
            printf("Enter team name to delete: ");
            if (!fgets(name, sizeof(name), stdin)) continue;
            trim_newline(name);
            sanitize_string(name);
            delete_team_by_name(name);
        } else if (choice == 3) {
            list_teams();
        } else if (choice == 4) {
            generate_round_robin();
        } else if (choice == 5) {
            list_fixtures();
        } else if (choice == 6) {
            int id;
            printf("Enter match id: ");
            if (scanf("%d", &id) != 1) { flush_input_line(); printf("Bad id.\n"); continue; }
            int s1, s2;
            printf("Enter score for team1: ");
            if (scanf("%d", &s1) != 1) { flush_input_line(); printf("Bad score.\n"); continue; }
            printf("Enter score for team2: ");
            if (scanf("%d", &s2) != 1) { flush_input_line(); printf("Bad score.\n"); continue; }
            record_match_result(id, s1, s2);
        } else if (choice == 7) {
            display_points_table();
        } else if (choice == 8) {
            save_to_files("teams.txt", "matches.txt");
        } else if (choice == 9) {
            load_from_files("teams.txt", "matches.txt");
        } else if (choice == 10) {
            int k;
            printf("Enter knockout size (power of 2 recommended, <= number of teams): ");
            if (scanf("%d", &k) != 1) { flush_input_line(); printf("Bad input.\n"); continue; }
            build_and_run_knockout(k);
        } else if (choice == 11) {
            reset_all_stats();
            clear_matches();
        } else {
            printf("Unknown choice.\n");
        }
    }

    // before exit, save automatically
    save_to_files("teams.txt", "matches.txt");
    free_team_list();
    free_match_list();
    return 0;
}