#ifndef RANDOM_WALK_H
#define RANDOM_WALK_H

typedef struct {
    double barrier;    /* absorbing barrier at +/- barrier */
    int n_walkers;     /* ensemble size */
    unsigned int seed; /* base seed; walker i uses seed + i */
} WalkConfig;

WalkConfig walk_default_config(void);

/*
 * Simulate one 1D random walk until |pos| >= barrier.
 * Uses erand48 with a per-walker seed state (thread-safe).
 * Returns the number of steps taken.
 */
int walk_until_barrier(double barrier, unsigned short xsubi[3]);

/*
 * Run n_walkers independent walks. Results are written into out (length n_walkers).
 * When use_omp is non-zero, walkers are distributed across OpenMP threads.
 * Returns wall-clock seconds for the ensemble loop.
 */
double walk_ensemble(const WalkConfig *cfg, double *out, int use_omp);

int walk_write_results(const char *path, const double *data, int n);

#endif /* RANDOM_WALK_H */
