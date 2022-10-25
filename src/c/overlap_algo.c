#include <stdlib.h>

#include "overlap_algo.h"

struct overlap_algo *overlap_new(void)
{
    struct overlap_algo *algo;

    algo = malloc(sizeof(*algo));
    if (!algo)
        return NULL;

    algo->_head = algo->_tail = NULL;
    algo->_wanted = 0;

    /* note, the fields _lo and _hi are not initialized here */

    return algo;
}

void overlap_del(struct overlap_algo *algo)
{
    while (algo->_head != NULL) {
        struct overlap_edge *edge = algo->_head;
        algo->_head = algo->_head->_next;
        free(edge);
    }

    free(algo);
}

/* Allocate and insert a new edge in sorted edge list.
 *
 * \returns 1 on success, 0 if the memory allocation failed
 */
static int overlap_insert(struct overlap_algo *algo, overlap_value_t value, overlap_chime_t chime)
{
    struct overlap_edge *edge;

    edge = malloc(sizeof(*edge));
    if (!edge)
        return 0;

    edge->_value = value;
    edge->_chime = chime;

    if (!algo->_head) {
        edge->_next = NULL;
        edge->_prev = NULL;
        algo->_head = edge;
        algo->_tail = edge;
    } else {
        struct overlap_edge *ptr;

        for (ptr = algo->_head; ptr; ptr = ptr->_next) {
            if (ptr->_value >= edge->_value)
                break;
        }

        if (ptr) {
            edge->_prev = ptr->_prev;
            edge->_next = ptr;
            if (ptr->_prev)
                ptr->_prev->_next = edge;
            else
                algo->_head = edge;
            ptr->_prev = edge;
        } else {
            edge->_prev = algo->_tail;
            edge->_next = NULL;
            algo->_tail->_next = edge;
            algo->_tail = edge;
        }
    }

    return 1;
}

int overlap_add(struct overlap_algo *algo, overlap_value_t lo, overlap_value_t hi)
{
    /* Sanity check */
    if (hi < lo)
        return 0;

    overlap_insert(algo, lo, -1);
    overlap_insert(algo, hi, +1);
    algo->_wanted++;

    return 1;
}

int overlap_find(struct overlap_algo *algo, overlap_value_t *lo, overlap_value_t *hi)
{
    while (algo->_wanted) {
        struct overlap_edge *edge, *lo_edge, *hi_edge;
        overlap_chime_t chime;

        chime = 0;
        lo_edge = NULL;
        for (edge = algo->_head; edge; edge = edge->_next) {
            chime -= edge->_chime;
            if (chime >= algo->_wanted) {
                lo_edge = edge;
                break;
            }
        }

        chime = 0;
        hi_edge = NULL;
        for (edge = algo->_tail; edge; edge = edge->_prev) {
            chime += edge->_chime;
            if (chime >= algo->_wanted) {
                hi_edge = edge;
                break;
            }
        }

        if (lo_edge && hi_edge) {
            *lo = lo_edge->_value;
            *hi = hi_edge->_value;
            break;
        }

        algo->_wanted--;
    }

    return algo->_wanted;
}

/*
    Local variables:
        compile-command: "gcc -Wall -g -shared -o liboverlap_algo.so overlap_algo.c "
    End:
*/
