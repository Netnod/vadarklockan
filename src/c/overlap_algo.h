#ifndef OVERLAP_ALGO_H
#define OVERLAP_ALGO_H

#ifdef __cplusplus
extern "C" {
#endif

/** The type for "value" used by the overlap algorithm.
 *
 * This can be changed if an implementation wants to use a different
 * type for "value".  For example, on a small platform which does not
 * support floating point, it could be changed to an uint32_t which
 * counts only seconds.
 */
typedef double overlap_value_t;

/** The type of "chime" used by the overlap algorithm.
 *
 * This can be changed if an implementation wants to use a different
 * type for "chime".  Normally this won't be neccesarry.
 */
typedef int overlap_chime_t;

/** An edge used internally by the overlap algorithm.
 */
struct overlap_edge {
    struct overlap_edge *_prev;
    struct overlap_edge *_next;
    overlap_value_t _value;
    overlap_chime_t _chime;
};

/** An instance of the overlap algorithm.
 */
struct overlap_algo {
    struct overlap_edge *_head;
    struct overlap_edge *_tail;

    /* Number of possible overlaps. */
    unsigned _wanted;
};

/** Allocate and initialize an overlap_algo instance.
 *
 * \returns the a pointer to the newly allocated instance.
 */
struct overlap_algo *overlap_new(void);

/** Clean up and free an overlap_algo instance.
 *
 * \param algo pointer to the algorithm instance
 */
void overlap_del(struct overlap_algo *algo);

/** Add a range the overlap algorithm.
 *
 * Note, that for each call to process a new edge structure will be
 * allocated which will not be freed until overlap_del is called on
 * the whole algorithm instance.  On a small platform, make sure to
 * limit the number of of calls to a sensible number before giving up
 * and restarting.
 *
 * If the function returns 0 indicating an error, the algorithm
 * instance should not be used any more, delete it and start over.
 *
 * \param algo pointer to the algorithm instance
 * \param lo the low value for range
 * \param hi the high value for range
 * \returns 1 on success or 0 on failure (memory allocation failed)
 */
int overlap_add(struct overlap_algo *algo, overlap_value_t lo, overlap_value_t hi);

/** Find the overlap of all added ranges.
 *
 * The pointers at lo and hi of the overlap algorithm instance will be
 * updated with the overlap that has been found.
 *
 * If returned number of overlaps is 0 the nothing will be written to
 * the values pointed to by lo and hi.
 *
 * \param algo pointer to the algorithm instance
 * \param lo the low value of the overlap is written to this pointer
 * \param hi the high value of the overlap is written to this pointer
 * \returns the number of ranges in the returned overlap
 */
int overlap_find(struct overlap_algo *algo, overlap_value_t *lo, overlap_value_t *hi);

#ifdef __cplusplus
}
#endif

#endif /* OVERLAP_ALGO_H */
