#define _XOPEN_SOURCE 600

#include "random_walk.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _OPENMP
#include <omp.h>
#endif

#include "timing.h"

WalkConfig walk_default_config(void)
{
    WalkConfig cfg;
    cfg.barrier = 10.0;
    cfg.n_walkers = 500000;
    cfg.seed = 1u;
    return cfg;
}

int walk_until_barrier(double barrier, unsigned short xsubi[3])
{
    double pos = 0.0;
    int n_steps = 0;

    do {
        pos += (erand48(xsubi) - 0.5) * 2.0;
        n_steps++;
    } while (fabs(pos) < barrier);

    return n_steps;
}

static void seed_state(unsigned int seed, unsigned short xsubi[3])
{
    xsubi[0] = (unsigned short)(seed & 0xffffu);
    xsubi[1] = (unsigned short)((seed >> 8) & 0xffffu);
    xsubi[2] = (unsigned short)((seed >> 16) & 0xffffu);
    if (xsubi[0] == 0 && xsubi[1] == 0 && xsubi[2] == 0) {
        xsubi[2] = 1;
    }
}

double walk_ensemble(const WalkConfig *cfg, double *out, int use_omp)
{
    int i;
    double t0, t1;

    t0 = wall_time_sec();

#if defined(_OPENMP)
    if (use_omp) {
#pragma omp parallel for default(none) shared(cfg, out) schedule(static)
        for (i = 0; i < cfg->n_walkers; i++) {
            unsigned short xsubi[3];
            seed_state(cfg->seed + (unsigned int)i, xsubi);
            out[i] = (double)walk_until_barrier(cfg->barrier, xsubi);
        }
    } else
#endif
    {
        (void)use_omp;
        for (i = 0; i < cfg->n_walkers; i++) {
            unsigned short xsubi[3];
            seed_state(cfg->seed + (unsigned int)i, xsubi);
            out[i] = (double)walk_until_barrier(cfg->barrier, xsubi);
        }
    }

    t1 = wall_time_sec();
    return t1 - t0;
}

int walk_write_results(const char *path, const double *data, int n)
{
    FILE *fp;
    int i;

    fp = fopen(path, "w");
    if (!fp) {
        return -1;
    }

    for (i = 0; i < n; i++) {
        fprintf(fp, "%.10e\n", data[i]);
    }
    fclose(fp);
    return 0;
}
