#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

#define ROWS 10
#define COLS 10
#define TOTAL_SEATS 100
#define TOTAL_SELLERS 10
#define SIM_MINUTES 60

typedef enum { HIGH = 0, MEDIUM = 1, LOW = 2 } seller_type_t;

typedef struct {
    char id[16];
    int arrival_time;
    int service_time;
    int start_time;
    int finish_time;
    int assigned_row;
    int assigned_col;
    int got_seat;
    int turned_away;
} customer_t;

typedef struct {
    char name[3];
    seller_type_t type;
    int seller_no_within_type;
    customer_t *customers;
    int customer_count;

    customer_t **queue;
    int q_head;
    int q_tail;
    int q_len;

    int next_arrival_idx;
    customer_t *current_customer;
    int service_end_minute;
    int closed;

    unsigned int rng_seed;
} seller_t;

typedef struct {
    int sold;
    int turned_away;
    double total_response;
    double total_turnaround;
} stats_t;

static seller_t sellers[TOTAL_SELLERS];
static char seats[ROWS][COLS][8];
static int sold_seat_count = 0;
static int concert_sold_out = 0;

static pthread_t threads[TOTAL_SELLERS];
static pthread_mutex_t seat_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t stats_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t clock_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t tick_cv = PTHREAD_COND_INITIALIZER;
static pthread_cond_t tick_done_cv = PTHREAD_COND_INITIALIZER;

static stats_t stats_by_type[3];
static int current_minute = -1;
static int current_tick = 0;
static int sellers_finished_this_tick = 0;
static int shutdown_threads = 0;

static int medium_row_order[ROWS] = {4, 5, 3, 6, 2, 7, 1, 8, 0, 9}; /* 0-based rows: 5,6,4,7,3,8,2,9,1,10 */

static void format_time(int minute, char *buf, size_t sz) {
    if (minute < 0) minute = 0;
    snprintf(buf, sz, "0:%02d", minute);
}

static void print_chart_locked(void) {
    int r, c;
    printf("Seating chart:\n");
    for (r = 0; r < ROWS; r++) {
        for (c = 0; c < COLS; c++) {
            printf("%-6s", seats[r][c][0] ? seats[r][c] : "-");
            if (c != COLS - 1) printf(" ");
        }
        printf("\n");
    }
}

static int choose_service_time(seller_t *s) {
    switch (s->type) {
        case HIGH:
            return (rand_r(&s->rng_seed) % 2) + 1;               /* 1..2 */
        case MEDIUM:
            return (rand_r(&s->rng_seed) % 3) + 2;               /* 2..4 */
        case LOW:
        default:
            return (rand_r(&s->rng_seed) % 4) + 4;               /* 4..7 */
    }
}

static void enqueue_customer(seller_t *s, customer_t *c) {
    s->queue[s->q_tail] = c;
    s->q_tail = (s->q_tail + 1) % s->customer_count;
    s->q_len++;
}

static customer_t *dequeue_customer(seller_t *s) {
    customer_t *c;
    if (s->q_len == 0) return NULL;
    c = s->queue[s->q_head];
    s->q_head = (s->q_head + 1) % s->customer_count;
    s->q_len--;
    return c;
}

static int assign_seat_for_type(seller_type_t type, int *out_row, int *out_col) {
    int r, c, idx;
    if (sold_seat_count >= TOTAL_SEATS) {
        concert_sold_out = 1;
        return 0;
    }

    if (type == HIGH) {
        for (r = 0; r < ROWS; r++) {
            for (c = 0; c < COLS; c++) {
                if (seats[r][c][0] == '\0') {
                    *out_row = r;
                    *out_col = c;
                    return 1;
                }
            }
        }
    } else if (type == LOW) {
        for (r = ROWS - 1; r >= 0; r--) {
            for (c = 0; c < COLS; c++) {
                if (seats[r][c][0] == '\0') {
                    *out_row = r;
                    *out_col = c;
                    return 1;
                }
            }
        }
    } else {
        for (idx = 0; idx < ROWS; idx++) {
            r = medium_row_order[idx];
            for (c = 0; c < COLS; c++) {
                if (seats[r][c][0] == '\0') {
                    *out_row = r;
                    *out_col = c;
                    return 1;
                }
            }
        }
    }

    concert_sold_out = 1;
    return 0;
}

static void update_sold_stats(seller_type_t type, int response, int turnaround) {
    pthread_mutex_lock(&stats_mutex);
    stats_by_type[type].sold++;
    stats_by_type[type].total_response += response;
    stats_by_type[type].total_turnaround += turnaround;
    pthread_mutex_unlock(&stats_mutex);
}

static void update_turned_away_stats(seller_type_t type, int count) {
    pthread_mutex_lock(&stats_mutex);
    stats_by_type[type].turned_away += count;
    pthread_mutex_unlock(&stats_mutex);
}

static int flush_remaining_customers(seller_t *s, int minute, const char *reason) {
    int count = 0;
    char tbuf[8];
    format_time(minute, tbuf, sizeof(tbuf));

    while (s->q_len > 0) {
        customer_t *c = dequeue_customer(s);
        c->turned_away = 1;
        pthread_mutex_lock(&print_mutex);
        printf("[%s] %s: %s leaves queue (%s).\n", tbuf, s->name, c->id, reason);
        pthread_mutex_unlock(&print_mutex);
        count++;
    }

    while (s->next_arrival_idx < s->customer_count) {
        customer_t *c = &s->customers[s->next_arrival_idx++];
        c->turned_away = 1;
        pthread_mutex_lock(&print_mutex);
        printf("[%s] %s: %s will not be served (%s).\n", tbuf, s->name, c->id, reason);
        pthread_mutex_unlock(&print_mutex);
        count++;
    }

    return count;
}

static void process_minute(seller_t *s, int minute) {
    char tbuf[8];
    format_time(minute, tbuf, sizeof(tbuf));

    if (minute < SIM_MINUTES && !s->closed) {
        while (s->next_arrival_idx < s->customer_count &&
               s->customers[s->next_arrival_idx].arrival_time == minute) {
            customer_t *c = &s->customers[s->next_arrival_idx++];
            pthread_mutex_lock(&print_mutex);
            printf("[%s] %s: %s arrives and joins the queue.\n", tbuf, s->name, c->id);
            pthread_mutex_unlock(&print_mutex);

            if (concert_sold_out) {
                c->turned_away = 1;
                update_turned_away_stats(s->type, 1);
                pthread_mutex_lock(&print_mutex);
                printf("[%s] %s: %s is told the concert is sold out and leaves.\n",
                       tbuf, s->name, c->id);
                pthread_mutex_unlock(&print_mutex);
            } else {
                enqueue_customer(s, c);
            }
        }
    }

    if (s->current_customer != NULL && s->service_end_minute == minute) {
        customer_t *c = s->current_customer;
        c->finish_time = minute;
        update_sold_stats(s->type, c->start_time - c->arrival_time,
                          c->finish_time - c->arrival_time);
        pthread_mutex_lock(&print_mutex);
        printf("[%s] %s: %s completes purchase and leaves.\n", tbuf, s->name, c->id);
        pthread_mutex_unlock(&print_mutex);
        s->current_customer = NULL;
    }

    if ((concert_sold_out || minute >= SIM_MINUTES) && !s->closed && s->current_customer == NULL) {
        int turned = flush_remaining_customers(
            s, minute, concert_sold_out ? "concert sold out / window closed" : "one-hour simulation ended"
        );
        if (turned > 0) {
            update_turned_away_stats(s->type, turned);
        }
        s->closed = 1;
        return;
    }

    if (s->current_customer == NULL && !s->closed && minute < SIM_MINUTES && s->q_len > 0) {
        customer_t *c = dequeue_customer(s);
        int row = -1, col = -1;

        pthread_mutex_lock(&seat_mutex);
        if (!concert_sold_out && assign_seat_for_type(s->type, &row, &col)) {
            strcpy(seats[row][col], c->id);
            sold_seat_count++;
            if (sold_seat_count >= TOTAL_SEATS) {
                concert_sold_out = 1;
            }
            c->got_seat = 1;
            c->assigned_row = row;
            c->assigned_col = col;
            c->start_time = minute;
            c->service_time = choose_service_time(s);
            s->current_customer = c;
            s->service_end_minute = minute + c->service_time;

            pthread_mutex_lock(&print_mutex);
            printf("[%s] %s: %s is assigned seat R%dS%d; service time = %d minute(s).\n",
                   tbuf, s->name, c->id, row + 1, col + 1, c->service_time);
            print_chart_locked();
            pthread_mutex_unlock(&print_mutex);
        } else {
            concert_sold_out = 1;
            c->turned_away = 1;
            update_turned_away_stats(s->type, 1);
            pthread_mutex_lock(&print_mutex);
            printf("[%s] %s: %s is told the concert is sold out and leaves.\n",
                   tbuf, s->name, c->id);
            pthread_mutex_unlock(&print_mutex);
        }
        pthread_mutex_unlock(&seat_mutex);
    }

    if (concert_sold_out && !s->closed && s->current_customer == NULL) {
        int turned = flush_remaining_customers(s, minute, "concert sold out / window closed");
        if (turned > 0) {
            update_turned_away_stats(s->type, turned);
        }
        s->closed = 1;
    }
}

static int simulation_done(void) {
    int i;
    int any_active = 0;
    int all_closed = 1;

    for (i = 0; i < TOTAL_SELLERS; i++) {
        if (sellers[i].current_customer != NULL) {
            any_active = 1;
        }
        if (!sellers[i].closed) {
            all_closed = 0;
        }
    }

    if (concert_sold_out) {
        return all_closed && !any_active;
    }

    return current_minute >= SIM_MINUTES && all_closed && !any_active;
}

static void *seller_thread(void *arg) {
    seller_t *s = (seller_t *)arg;
    int local_tick = 0;

    for (;;) {
        int minute;

        pthread_mutex_lock(&clock_mutex);
        while (current_tick == local_tick) {
            pthread_cond_wait(&tick_cv, &clock_mutex);
        }
        if (shutdown_threads) {
            pthread_mutex_unlock(&clock_mutex);
            break;
        }
        local_tick = current_tick;
        minute = current_minute;
        pthread_mutex_unlock(&clock_mutex);

        process_minute(s, minute);

        pthread_mutex_lock(&clock_mutex);
        sellers_finished_this_tick++;
        if (sellers_finished_this_tick == TOTAL_SELLERS) {
            pthread_cond_signal(&tick_done_cv);
        }
        pthread_mutex_unlock(&clock_mutex);
    }
    return NULL;
}

static int cmp_customer_arrival(const void *a, const void *b) {
    const customer_t *ca = (const customer_t *)a;
    const customer_t *cb = (const customer_t *)b;
    if (ca->arrival_time != cb->arrival_time) {
        return ca->arrival_time - cb->arrival_time;
    }
    return strcmp(ca->id, cb->id);
}

static void init_seller(seller_t *s, const char *name, seller_type_t type, int seller_no_within_type, int n, unsigned int seed) {
    int i;
    memset(s, 0, sizeof(*s));
    strncpy(s->name, name, sizeof(s->name) - 1);
    s->type = type;
    s->seller_no_within_type = seller_no_within_type;
    s->customer_count = n;
    s->customers = (customer_t *)calloc((size_t)n, sizeof(customer_t));
    s->queue = (customer_t **)calloc((size_t)n, sizeof(customer_t *));
    s->rng_seed = seed;

    if (!s->customers || !s->queue) {
        fprintf(stderr, "Memory allocation failed.\n");
        exit(1);
    }

    for (i = 0; i < n; i++) {
        customer_t *c = &s->customers[i];
        c->arrival_time = rand_r(&s->rng_seed) % SIM_MINUTES;
        c->start_time = -1;
        c->finish_time = -1;
        c->assigned_row = -1;
        c->assigned_col = -1;

        if (type == HIGH) {
            snprintf(c->id, sizeof(c->id), "H%03d", i + 1);
        } else if (type == MEDIUM) {
            snprintf(c->id, sizeof(c->id), "M%d%02d", seller_no_within_type, i + 1);
        } else {
            snprintf(c->id, sizeof(c->id), "L%d%02d", seller_no_within_type, i + 1);
        }
    }

    qsort(s->customers, (size_t)n, sizeof(customer_t), cmp_customer_arrival);
}

static void init_all_sellers(int n) {
    unsigned int base_seed = (unsigned int)time(NULL);
    int i;

    init_seller(&sellers[0], "H", HIGH, 1, n, base_seed + 11U);
    init_seller(&sellers[1], "M1", MEDIUM, 1, n, base_seed + 21U);
    init_seller(&sellers[2], "M2", MEDIUM, 2, n, base_seed + 31U);
    init_seller(&sellers[3], "M3", MEDIUM, 3, n, base_seed + 41U);
    init_seller(&sellers[4], "L1", LOW, 1, n, base_seed + 51U);
    init_seller(&sellers[5], "L2", LOW, 2, n, base_seed + 61U);
    init_seller(&sellers[6], "L3", LOW, 3, n, base_seed + 71U);
    init_seller(&sellers[7], "L4", LOW, 4, n, base_seed + 81U);
    init_seller(&sellers[8], "L5", LOW, 5, n, base_seed + 91U);
    init_seller(&sellers[9], "L6", LOW, 6, n, base_seed + 101U);

    for (i = 0; i < ROWS; i++) {
        int j;
        for (j = 0; j < COLS; j++) {
            seats[i][j][0] = '\0';
        }
    }
}

static void print_arrival_queues(void) {
    int i, j;
    printf("\nInitial sorted customer queues by arrival minute:\n");
    for (i = 0; i < TOTAL_SELLERS; i++) {
        printf("%s: ", sellers[i].name);
        for (j = 0; j < sellers[i].customer_count; j++) {
            printf("%s@%02d", sellers[i].customers[j].id, sellers[i].customers[j].arrival_time);
            if (j != sellers[i].customer_count - 1) printf(", ");
        }
        printf("\n");
    }
    printf("\n");
}

static void print_final_summary(int n) {
    int i;
    (void)n;

    printf("\n================ FINAL SUMMARY ================\n");
    printf("Seats sold total      : %d\n", sold_seat_count);
    printf("Customers turned away : %d\n",
           stats_by_type[HIGH].turned_away + stats_by_type[MEDIUM].turned_away + stats_by_type[LOW].turned_away);
    printf("Concert sold out      : %s\n", concert_sold_out ? "YES" : "NO");
    printf("Simulation end minute : %d\n\n", current_minute);

    for (i = 0; i < 3; i++) {
        const char *label = (i == HIGH ? "H" : (i == MEDIUM ? "M" : "L"));
        int sold = stats_by_type[i].sold;
        double avg_resp = sold ? stats_by_type[i].total_response / sold : 0.0;
        double avg_tat = sold ? stats_by_type[i].total_turnaround / sold : 0.0;
        double throughput = sold / 60.0; /* customers/minute during selling hour */

        printf("Type %s customers\n", label);
        printf("  Sold               : %d\n", sold);
        printf("  Turned away        : %d\n", stats_by_type[i].turned_away);
        printf("  Avg response time  : %.2f minutes\n", avg_resp);
        printf("  Avg turnaround time: %.2f minutes\n", avg_tat);
        printf("  Throughput         : %.2f customers/minute\n\n", throughput);
    }

    printf("Per-seller breakdown:\n");
    for (i = 0; i < TOTAL_SELLERS; i++) {
        int sold = 0, turned = 0, j;
        for (j = 0; j < sellers[i].customer_count; j++) {
            if (sellers[i].customers[j].got_seat) sold++;
            if (sellers[i].customers[j].turned_away) turned++;
        }
        printf("  %-2s -> sold: %2d, turned away: %2d\n", sellers[i].name, sold, turned);
    }
    printf("===============================================\n");
}

static void free_all_memory(void) {
    int i;
    for (i = 0; i < TOTAL_SELLERS; i++) {
        free(sellers[i].customers);
        free(sellers[i].queue);
    }
}

int main(int argc, char *argv[]) {
    int i;
    int n;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <N customers per seller>\n", argv[0]);
        return 1;
    }

    n = atoi(argv[1]);
    if (n <= 0) {
        fprintf(stderr, "N must be a positive integer.\n");
        return 1;
    }

    init_all_sellers(n);
    print_arrival_queues();

    for (i = 0; i < TOTAL_SELLERS; i++) {
        if (pthread_create(&threads[i], NULL, seller_thread, &sellers[i]) != 0) {
            fprintf(stderr, "Failed to create seller thread %s\n", sellers[i].name);
            free_all_memory();
            return 1;
        }
    }

    for (current_minute = 0; ; current_minute++) {
        pthread_mutex_lock(&clock_mutex);
        sellers_finished_this_tick = 0;
        current_tick++;
        pthread_cond_broadcast(&tick_cv);
        while (sellers_finished_this_tick < TOTAL_SELLERS) {
            pthread_cond_wait(&tick_done_cv, &clock_mutex);
        }
        pthread_mutex_unlock(&clock_mutex);

        if (simulation_done()) {
            break;
        }
    }

    pthread_mutex_lock(&clock_mutex);
    shutdown_threads = 1;
    current_tick++;
    pthread_cond_broadcast(&tick_cv);
    pthread_mutex_unlock(&clock_mutex);

    for (i = 0; i < TOTAL_SELLERS; i++) {
        pthread_join(threads[i], NULL);
    }

    print_final_summary(n);
    free_all_memory();
    return 0;
}
