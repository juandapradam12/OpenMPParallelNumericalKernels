#include "advection.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _OPENMP
#include <omp.h>
#endif

#include "timing.h"

AdvectionConfig advection_default_config(void)
{
    AdvectionConfig cfg;
    cfg.c = 1.0;
    cfg.x_min = -2.0;
    cfg.x_max = 2.0;
    cfg.n_points = 1000;
    cfg.dt = 1e-3;
    cfg.n_steps = 700;
    return cfg;
}

int advection_init(AdvectionState *state, const AdvectionConfig *cfg)
{
    int i;

    if (!state || !cfg || cfg->n_points < 3) {
        return -1;
    }

    memset(state, 0, sizeof(*state));
    state->n_points = cfg->n_points;
    state->dx = (cfg->x_max - cfg->x_min) / (double)(cfg->n_points - 1);
    state->alpha = cfg->c * cfg->dt / state->dx;

    state->x = (double *)malloc((size_t)cfg->n_points * sizeof(double));
    state->u = (double *)calloc((size_t)cfg->n_points, sizeof(double));
    state->u_next = (double *)calloc((size_t)cfg->n_points, sizeof(double));
    if (!state->x || !state->u || !state->u_next) {
        advection_free(state);
        return -1;
    }

    for (i = 0; i < cfg->n_points; i++) {
        state->x[i] = cfg->x_min + (double)i * state->dx;
        if (fabs(state->x[i]) <= 0.5) {
            state->u[i] = 0.25 - state->x[i] * state->x[i];
        }
    }

    return 0;
}

void advection_free(AdvectionState *state)
{
    if (!state) {
        return;
    }
    free(state->x);
    free(state->u);
    free(state->u_next);
    memset(state, 0, sizeof(*state));
}

void advection_step(AdvectionState *state, int use_omp)
{
    int i;
    const int n = state->n_points;
    const double alpha = state->alpha;
    double *restrict u = state->u;
    double *restrict u_next = state->u_next;

    /* Fixed Dirichlet boundaries. */
    u_next[0] = u[0];
    u_next[n - 1] = u[n - 1];

    /*
     * First-order upwind for c > 0:
     *   u^{n+1}_i = u^n_i - alpha * (u^n_i - u^n_{i-1})
     * Time levels are sequential; only the spatial loop is parallel-safe.
     */
#if defined(_OPENMP)
    if (use_omp) {
#pragma omp parallel for default(none) shared(u, u_next, n, alpha)
        for (i = 1; i < n - 1; i++) {
            u_next[i] = u[i] - alpha * (u[i] - u[i - 1]);
        }
    } else
#endif
    {
        (void)use_omp;
        for (i = 1; i < n - 1; i++) {
            u_next[i] = u[i] - alpha * (u[i] - u[i - 1]);
        }
    }

    /* Swap buffers without copying. */
    {
        double *tmp = state->u;
        state->u = state->u_next;
        state->u_next = tmp;
    }
}

static int write_snapshot(const char *prefix, int step, const double *u, int n)
{
    char path[512];
    FILE *fp;
    int i;

    if (snprintf(path, sizeof(path), "%s_t%04d.dat", prefix, step) >= (int)sizeof(path)) {
        return -1;
    }

    fp = fopen(path, "w");
    if (!fp) {
        return -1;
    }

    for (i = 0; i < n; i++) {
        fprintf(fp, "%.10e\n", u[i]);
    }
    fclose(fp);
    return 0;
}

static int should_snapshot(int step, const int *snapshot_steps, int n_snapshots)
{
    int k;
    for (k = 0; k < n_snapshots; k++) {
        if (snapshot_steps[k] == step) {
            return 1;
        }
    }
    return 0;
}

double advection_run(AdvectionState *state,
                     const AdvectionConfig *cfg,
                     int use_omp,
                     const int *snapshot_steps,
                     int n_snapshots,
                     const char *snapshot_prefix)
{
    int step;
    double t0, t1;

    if (snapshot_prefix && should_snapshot(0, snapshot_steps, n_snapshots)) {
        if (write_snapshot(snapshot_prefix, 0, state->u, state->n_points) != 0) {
            fprintf(stderr, "error: failed to write snapshot at step 0\n");
        }
    }

    /* Time only the numerical update; snapshot I/O is excluded. */
    t0 = wall_time_sec();
    for (step = 1; step <= cfg->n_steps; step++) {
        advection_step(state, use_omp);
        if (snapshot_prefix && should_snapshot(step, snapshot_steps, n_snapshots)) {
            t1 = wall_time_sec();
            if (write_snapshot(snapshot_prefix, step, state->u, state->n_points) != 0) {
                fprintf(stderr, "error: failed to write snapshot at step %d\n", step);
            }
            t0 += wall_time_sec() - t1;
        }
    }
    t1 = wall_time_sec();
    return t1 - t0;
}

int advection_write_grid(const AdvectionState *state, const char *path)
{
    FILE *fp;
    int i;

    fp = fopen(path, "w");
    if (!fp) {
        return -1;
    }

    for (i = 0; i < state->n_points; i++) {
        fprintf(fp, "%.10e\n", state->x[i]);
    }
    fclose(fp);
    return 0;
}
