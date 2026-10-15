#ifndef ADVECTION_H
#define ADVECTION_H

#include <stddef.h>

typedef struct {
    double c;          /* wave speed (m/s) */
    double x_min;      /* domain left bound (m) */
    double x_max;      /* domain right bound (m) */
    int n_points;      /* spatial grid size */
    double dt;         /* time step (s) */
    int n_steps;       /* number of time steps */
} AdvectionConfig;

typedef struct {
    double *x;         /* length n_points */
    double *u;         /* length n_points, current field */
    double *u_next;    /* length n_points, scratch buffer */
    int n_points;
    double dx;
    double alpha;      /* CFL number: c * dt / dx */
} AdvectionState;

/* Default configuration matching the original experiment. */
AdvectionConfig advection_default_config(void);

/* Allocate grids and apply the initial pulse u(x,0) = 0.25 - x^2 for |x| <= 0.5. */
int advection_init(AdvectionState *state, const AdvectionConfig *cfg);

void advection_free(AdvectionState *state);

/*
 * Advance one time step with a first-order upwind finite-difference scheme.
 * Boundary values are held fixed (Dirichlet).
 * When use_omp is non-zero, the spatial update is parallelized with OpenMP.
 */
void advection_step(AdvectionState *state, int use_omp);

/*
 * Integrate for cfg->n_steps steps. Snapshots are written at steps listed in
 * snapshot_steps (length n_snapshots). Each snapshot file receives one column
 * of u values. Returns wall-clock seconds spent in the time loop.
 */
double advection_run(AdvectionState *state,
                     const AdvectionConfig *cfg,
                     int use_omp,
                     const int *snapshot_steps,
                     int n_snapshots,
                     const char *snapshot_prefix);

int advection_write_grid(const AdvectionState *state, const char *path);

#endif /* ADVECTION_H */
