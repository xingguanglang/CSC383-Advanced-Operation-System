/*
 * =============================================================================
 *  COEN 383 - Advanced Operating Systems
 *  Project 3: Multi-threaded Ticket Sellers
 *
 *  Group: <FILL IN GROUP NUMBER>
 *  Members:
 *    - Member 1: <name> -- Core Synchronization & Thread Lifecycle
 *    - Member 2: <name> -- Customer Queues & Arrival Generation
 *    - Member 3: <name> -- Seat Assignment Policy & Output Formatting
 *    - Member 4: <name> -- Metrics, Testing & Simulation Runs
 *    - Member 5: <name> -- Design Report, Integration & Submission
 *
 *  Build:  gcc -Wall -O2 -pthread ticket_sim.c -o ticket_sim
 *  Run:    ./ticket_sim <N>        (e.g. ./ticket_sim 10)
 *
 *  Each function / block below is tagged with [MEMBER X] so that each teammate
 *  knows which sections they own.  Shared helpers used by more than one module
 *  are tagged [SHARED].
 * =============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

/* =============================================================================
 *  [SHARED] Simulation-wide constants
 * ============================================================================= */
#define ROWS              10
#define COLS              10
#define TOTAL_SEATS       (ROWS * COLS)
#define SIM_MINUTES       60          /* one-hour simulation                 */
#define NUM_SELLERS       10          /* 1 H + 3 M + 6 L                     */
#define NUM_H             1
#define NUM_M             3
#define NUM_L             6
#define SLEEP_PER_MIN_US  50000       /* 50 ms = 1 simulated minute (tunable)*/

typedef enum { SELLER_H = 0, SELLER_M = 1, SELLER_L = 2 } seller_type_t;

/* =============================================================================
 *  [MEMBER 2] Customer struct and per-seller FIFO queue
 * ============================================================================= */
typedef struct customer {
    char             id[16];         /* e.g. "H001", "M102", "L305"          */
    int              arrival_time;   /* minute 0..59                         */
    int              service_time;   /* random, depends on seller type       */
    int              start_time;     /* when seller began serving  (-1 if N/A)*/
    int              finish_time;    /* when ticket purchase completed       */
    int              seat_row;       /* assigned row  (-1 if turned away)    */
    int              seat_col;       /* assigned col  (-1 if turned away)    */
    int              got_seat;       /* 1 if served with a seat, 0 otherwise */
    struct customer *next;
} customer_t;

typedef struct {
    char          name[4];           /* "H", "M1", "L3", ...                 */
    seller_type_t type;
    int           seller_index;      /* 0..9, used for ID generation         */
    customer_t   *queue_head;        /* sorted by arrival_time ascending     */
    int           queue_len;
} seller_t;

/* =============================================================================
 *  [MEMBER 1] Shared data and synchronization primitives
 * ============================================================================= */

/* Seat matrix: holds customer ID string per seat, or "----" when empty.      */
static char seat_matrix[ROWS][COLS][16];
static int  seats_sold = 0;

/* Global simulation clock in minutes.  Incremented by the main thread.       */
static volatile int sim_clock = 0;
static volatile int simulation_running = 1;

/* One mutex protects the seat matrix + seats_sold + the event-log printing.
 * We intentionally use a SINGLE coarse-grained mutex for correctness and
 * simplicity.  The report should discuss this trade-off.                     */
static pthread_mutex_t seat_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Start barrier: seller threads wait on `start_cond` until main broadcasts.  */
static pthread_mutex_t start_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  start_cond  = PTHREAD_COND_INITIALIZER;
static int             start_flag  = 0;

static seller_t sellers[NUM_SELLERS];

/* =============================================================================
 *  [MEMBER 3] Seat-assignment row order (per seller type)
 *
 *  H : row 1..10       -> indices 0,1,2,...,9
 *  L : row 10..1       -> indices 9,8,7,...,0
 *  M : row 5,6,4,7,3,8,2,9,1,10 -> indices 4,5,3,6,2,7,1,8,0,9
 * ============================================================================= */
static const int H_ROW_ORDER[ROWS] = {0,1,2,3,4,5,6,7,8,9};
static const int L_ROW_ORDER[ROWS] = {9,8,7,6,5,4,3,2,1,0};
static const int M_ROW_ORDER[ROWS] = {4,5,3,6,2,7,1,8,0,9};

/* =============================================================================
 *  [MEMBER 4] Per-customer metric storage (one global list, appended under lock)
 * ============================================================================= */
typedef struct {
    seller_type_t type;
    int arrival_time;
    int start_time;
    int finish_time;
    int got_seat;
} metric_record_t;

#define MAX_CUSTOMERS  (NUM_SELLERS * 15 + 16)   /* N<=15 per spec            */
static metric_record_t metrics[MAX_CUSTOMERS];
static int metrics_count = 0;

/* =============================================================================
 *  [MEMBER 3] Output helpers (timestamps, event lines, seating chart)
 * ============================================================================= */
static void print_time_prefix(int minute) {
    printf("%d:%02d  ", minute / 60, minute % 60);
}

static void print_seat_chart(void) {
    printf("        ");
    for (int c = 0; c < COLS; c++) printf(" S%-2d ", c + 1);
    printf("\n");
    for (int r = 0; r < ROWS; r++) {
        printf("  R%-2d  ", r + 1);
        for (int c = 0; c < COLS; c++) printf(" %-4s", seat_matrix[r][c]);
        printf("\n");
    }
    printf("\n");
}

/* =============================================================================
 *  [MEMBER 2] Customer ID generation and random service time
 * ============================================================================= */
static void make_customer_id(char *out, seller_type_t t, int seller_idx, int n) {
    /* H:  "H001", "H002", ...
     * M1: "M101", "M102"; M2: "M201"; M3: "M301"
     * L1: "L101"; L2: "L201"; ... ; L6: "L601"
     */
    if (t == SELLER_H) {
        snprintf(out, 8, "H%03d", n + 1);
    } else if (t == SELLER_M) {
        /* seller_idx in sellers[] is 1,2,3 for M1,M2,M3 */
        int m_num = seller_idx;                 /* 1..3 */
        snprintf(out, 8, "M%d%02d", m_num, n + 1);
    } else {
        /* seller_idx in sellers[] is 4..9 for L1..L6 */
        int l_num = seller_idx - NUM_H - NUM_M + 1;  /* 1..6 */
        snprintf(out, 8, "L%d%02d", l_num, n + 1);
    }
}

static int random_service_time(seller_type_t t) {
    switch (t) {
        case SELLER_H: return 1 + (rand() % 2);          /* 1..2  */
        case SELLER_M: return 2 + (rand() % 3);          /* 2..4  */
        case SELLER_L: return 4 + (rand() % 4);          /* 4..7  */
    }
    return 1;
}

/* =============================================================================
 *  [MEMBER 2] Build a FIFO queue of N customers for one seller,
 *             sorted by random arrival time in [0, SIM_MINUTES-1].
 * ============================================================================= */
static void insert_sorted(customer_t **head, customer_t *node) {
    if (*head == NULL || node->arrival_time < (*head)->arrival_time) {
        node->next = *head;
        *head = node;
        return;
    }
    customer_t *cur = *head;
    while (cur->next && cur->next->arrival_time <= node->arrival_time)
        cur = cur->next;
    node->next = cur->next;
    cur->next = node;
}

static void build_queue_for_seller(seller_t *s, int N) {
    s->queue_head = NULL;
    s->queue_len  = N;
    for (int i = 0; i < N; i++) {
        customer_t *c = (customer_t *)calloc(1, sizeof(customer_t));
        make_customer_id(c->id, s->type, s->seller_index, i);
        c->arrival_time = rand() % SIM_MINUTES;
        c->service_time = random_service_time(s->type);
        c->start_time   = -1;
        c->finish_time  = -1;
        c->seat_row     = -1;
        c->seat_col     = -1;
        c->got_seat     = 0;
        insert_sorted(&s->queue_head, c);
    }
}

/* =============================================================================
 *  [MEMBER 3] Seat-assignment logic.  MUST be called while holding seat_mutex.
 *             Returns 1 if a seat was assigned, 0 if sold out.
 * ============================================================================= */
static int try_assign_seat(seller_type_t t, const char *cust_id,
                           int *out_row, int *out_col)
{
    const int *order;
    switch (t) {
        case SELLER_H: order = H_ROW_ORDER; break;
        case SELLER_M: order = M_ROW_ORDER; break;
        case SELLER_L: order = L_ROW_ORDER; break;
        default: return 0;
    }
    for (int i = 0; i < ROWS; i++) {
        int r = order[i];
        for (int c = 0; c < COLS; c++) {
            if (seat_matrix[r][c][0] == '-') {
                strncpy(seat_matrix[r][c], cust_id, 7);
                seat_matrix[r][c][7] = '\0';
                seats_sold++;
                *out_row = r;
                *out_col = c;
                return 1;
            }
        }
    }
    return 0;
}

/* =============================================================================
 *  [MEMBER 4] Metric recording.  Called under seat_mutex at completion.
 * ============================================================================= */
static void record_metric(seller_type_t t, customer_t *c) {
    if (metrics_count < MAX_CUSTOMERS) {
        metrics[metrics_count].type         = t;
        metrics[metrics_count].arrival_time = c->arrival_time;
        metrics[metrics_count].start_time   = c->start_time;
        metrics[metrics_count].finish_time  = c->finish_time;
        metrics[metrics_count].got_seat     = c->got_seat;
        metrics_count++;
    }
}

/* =============================================================================
 *  [MEMBER 1] Seller thread function.
 *
 *  Each seller thread:
 *   1) Waits on the start-barrier until main broadcasts.
 *   2) Loops minute-by-minute: dequeues customers whose arrival_time has
 *      arrived, serves them one at a time (the spec says each seller serves
 *      customers FIFO from their own queue -- no intra-seller concurrency).
 *   3) During the service, the seat-assignment step is guarded by seat_mutex.
 *   4) Terminates when either the hour is up or the concert is sold out.
 * ============================================================================= */
static void *seller_thread(void *arg) {
    seller_t *me = (seller_t *)arg;

    /* ---- wait on start barrier ---- */
    pthread_mutex_lock(&start_mutex);
    while (!start_flag) pthread_cond_wait(&start_cond, &start_mutex);
    pthread_mutex_unlock(&start_mutex);

    while (simulation_running && me->queue_head != NULL) {
        customer_t *c = me->queue_head;

        /* Wait (in simulated time) until this customer has arrived. */
        while (simulation_running && sim_clock < c->arrival_time) {
            usleep(SLEEP_PER_MIN_US / 4);
        }
        if (!simulation_running) break;

        /* ---- Event: customer arrives at the tail of this seller's queue ----
         * Because we pre-built the queue sorted by arrival_time, the "arrival"
         * event is logically emitted at c->arrival_time.                     */
        pthread_mutex_lock(&seat_mutex);
        print_time_prefix(c->arrival_time);
        printf("ARRIVAL    %-4s joins %s's queue\n", c->id, me->name);
        pthread_mutex_unlock(&seat_mutex);

        /* ---- Serve the customer ---- */
        pthread_mutex_lock(&seat_mutex);
        c->start_time = sim_clock;

        if (seats_sold >= TOTAL_SEATS) {
            /* Concert is sold out -- customer is turned away immediately.   */
            c->got_seat    = 0;
            c->finish_time = sim_clock;
            print_time_prefix(sim_clock);
            printf("SOLD-OUT   %-4s turned away by %s\n", c->id, me->name);
            record_metric(me->type, c);
            pthread_mutex_unlock(&seat_mutex);
        } else {
            int r, col;
            int ok = try_assign_seat(me->type, c->id, &r, &col);
            if (ok) {
                c->got_seat = 1;
                c->seat_row = r;
                c->seat_col = col;
                print_time_prefix(sim_clock);
                printf("ASSIGN     %-4s -> row %d seat %d  by %s (service=%d min)\n",
                       c->id, r + 1, col + 1, me->name, c->service_time);
                pthread_mutex_unlock(&seat_mutex);

                /* Service takes c->service_time simulated minutes. */
                int target = sim_clock + c->service_time;
                while (simulation_running && sim_clock < target) {
                    usleep(SLEEP_PER_MIN_US / 4);
                }

                /* Completion -- re-enter the critical region to log & chart. */
                pthread_mutex_lock(&seat_mutex);
                c->finish_time = sim_clock;
                print_time_prefix(sim_clock);
                printf("COMPLETE   %-4s leaves (served by %s)\n", c->id, me->name);
                print_seat_chart();
                record_metric(me->type, c);
                pthread_mutex_unlock(&seat_mutex);
            } else {
                /* Shouldn't happen because we checked seats_sold, but be safe. */
                c->got_seat    = 0;
                c->finish_time = sim_clock;
                print_time_prefix(sim_clock);
                printf("SOLD-OUT   %-4s turned away by %s\n", c->id, me->name);
                record_metric(me->type, c);
                pthread_mutex_unlock(&seat_mutex);
            }
        }

        /* Pop the served customer and free. */
        me->queue_head = c->next;
        free(c);
    }

    /* Drain any remaining customers when the hour ends or concert sells out. */
    pthread_mutex_lock(&seat_mutex);
    customer_t *c = me->queue_head;
    while (c) {
        print_time_prefix(sim_clock);
        printf("LEFT       %-4s abandoned %s's queue (window closed)\n",
               c->id, me->name);
        c->got_seat    = 0;
        c->start_time  = sim_clock;
        c->finish_time = sim_clock;
        record_metric(me->type, c);
        customer_t *nxt = c->next;
        free(c);
        c = nxt;
    }
    me->queue_head = NULL;
    pthread_mutex_unlock(&seat_mutex);

    return NULL;
}

/* =============================================================================
 *  [MEMBER 1] Global clock ticker thread.
 *             Advances sim_clock every SLEEP_PER_MIN_US microseconds,
 *             stops the simulation at 60 minutes or when sold out.
 * ============================================================================= */
static void *clock_thread(void *arg) {
    (void)arg;
    while (simulation_running) {
        usleep(SLEEP_PER_MIN_US);
        sim_clock++;
        if (sim_clock >= SIM_MINUTES) {
            simulation_running = 0;
            break;
        }
        pthread_mutex_lock(&seat_mutex);
        int done = (seats_sold >= TOTAL_SEATS);
        pthread_mutex_unlock(&seat_mutex);
        if (done) { simulation_running = 0; break; }
    }
    return NULL;
}

/* =============================================================================
 *  [MEMBER 4] Final metric reporting
 * ============================================================================= */
static void print_final_report(int N) {
    double sum_resp[3] = {0,0,0}, sum_turn[3] = {0,0,0};
    int    served[3]   = {0,0,0}, turned[3]   = {0,0,0};
    const char *labels[3] = {"H (High)  ", "M (Medium)", "L (Low)   "};

    for (int i = 0; i < metrics_count; i++) {
        int t = metrics[i].type;
        int resp = metrics[i].start_time  - metrics[i].arrival_time;
        int turn = metrics[i].finish_time - metrics[i].arrival_time;
        if (resp < 0) resp = 0;
        if (turn < 0) turn = 0;
        sum_resp[t] += resp;
        sum_turn[t] += turn;
        if (metrics[i].got_seat) served[t]++;
        else                     turned[t]++;
    }

    printf("\n============================================================\n");
    printf(" SIMULATION REPORT  (N = %d customers per seller)\n", N);
    printf("============================================================\n");
    printf("%-12s  %-7s %-7s  %-10s %-10s %-10s\n",
           "Seller type", "Served", "Turned", "AvgResp", "AvgTurn", "Thruput");
    for (int t = 0; t < 3; t++) {
        int total = served[t] + turned[t];
        double avg_r = total ? sum_resp[t] / total : 0.0;
        double avg_t = total ? sum_turn[t] / total : 0.0;
        double thr   = (double)served[t] / (double)SIM_MINUTES;  /* served/min */
        printf("%-12s  %-7d %-7d  %-10.2f %-10.2f %-10.4f\n",
               labels[t], served[t], turned[t], avg_r, avg_t, thr);
    }
    printf("------------------------------------------------------------\n");
    printf("Total seats sold : %d / %d\n", seats_sold, TOTAL_SEATS);
    printf("Total customers  : %d\n", metrics_count);
    printf("Simulation ended at minute %d\n", sim_clock);
    printf("============================================================\n");
}

/* =============================================================================
 *  [MEMBER 1 & MEMBER 5] main() -- wires everything together
 * ============================================================================= */
int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <N>\n", argv[0]);
        return 1;
    }
    int N = atoi(argv[1]);
    if (N <= 0 || N > 15) {
        fprintf(stderr, "N must be in [1, 15] (spec uses 5, 10, 15)\n");
        return 1;
    }

    srand((unsigned)time(NULL));

    /* ---- [MEMBER 3] Initialize the seat matrix to "----". ---- */
    for (int r = 0; r < ROWS; r++)
        for (int c = 0; c < COLS; c++)
            strcpy(seat_matrix[r][c], "----");

    /* ---- [MEMBER 2] Configure the 10 sellers and build their queues. ---- */
    strcpy(sellers[0].name, "H");   sellers[0].type = SELLER_H; sellers[0].seller_index = 0;
    strcpy(sellers[1].name, "M1");  sellers[1].type = SELLER_M; sellers[1].seller_index = 1;
    strcpy(sellers[2].name, "M2");  sellers[2].type = SELLER_M; sellers[2].seller_index = 2;
    strcpy(sellers[3].name, "M3");  sellers[3].type = SELLER_M; sellers[3].seller_index = 3;
    strcpy(sellers[4].name, "L1");  sellers[4].type = SELLER_L; sellers[4].seller_index = 4;
    strcpy(sellers[5].name, "L2");  sellers[5].type = SELLER_L; sellers[5].seller_index = 5;
    strcpy(sellers[6].name, "L3");  sellers[6].type = SELLER_L; sellers[6].seller_index = 6;
    strcpy(sellers[7].name, "L4");  sellers[7].type = SELLER_L; sellers[7].seller_index = 7;
    strcpy(sellers[8].name, "L5");  sellers[8].type = SELLER_L; sellers[8].seller_index = 8;
    strcpy(sellers[9].name, "L6");  sellers[9].type = SELLER_L; sellers[9].seller_index = 9;

    for (int i = 0; i < NUM_SELLERS; i++)
        build_queue_for_seller(&sellers[i], N);

    printf("Simulation starting: N = %d customers per seller, %d sellers, %d seats.\n\n",
           N, NUM_SELLERS, TOTAL_SEATS);

    /* ---- [MEMBER 1] Create 10 suspended seller threads + 1 clock thread. ---- */
    pthread_t tids[NUM_SELLERS];
    pthread_t clk_tid;

    for (int i = 0; i < NUM_SELLERS; i++)
        pthread_create(&tids[i], NULL, seller_thread, &sellers[i]);

    pthread_create(&clk_tid, NULL, clock_thread, NULL);

    /* Broadcast: wake all seller threads to start in parallel. */
    pthread_mutex_lock(&start_mutex);
    start_flag = 1;
    pthread_cond_broadcast(&start_cond);
    pthread_mutex_unlock(&start_mutex);

    /* ---- Wait for all sellers and the clock to finish. ---- */
    for (int i = 0; i < NUM_SELLERS; i++)
        pthread_join(tids[i], NULL);
    simulation_running = 0;
    pthread_join(clk_tid, NULL);

    /* ---- [MEMBER 4] Final metrics. ---- */
    print_final_report(N);

    return 0;
}