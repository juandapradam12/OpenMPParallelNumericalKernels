#include "random_walk.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(const char *prog)
{
    fprintf(stderr,
            "Usage: %s [--omp] [--out PATH] [--walkers N] [--barrier B] [--seed S]\n"
            "  Runs an ensemble of 1D random walks until an absorbing barrier is hit.\n"
            "  Writes one hitting-time sample per walker to PATH (default: results/walk_serial.dat).\n",
            prog);
}

int main(int argc, char **argv)
{
    WalkConfig cfg = walk_default_config();
    int use_omp = 0;
    const char *out_path = NULL;
    char default_path[256];
    double *samples;
    double elapsed;
    int i;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--omp") == 0) {
            use_omp = 1;
        } else if (strcmp(argv[i], "--out") == 0 && i + 1 < argc) {
            out_path = argv[++i];
        } else if (strcmp(argv[i], "--walkers") == 0 && i + 1 < argc) {
            cfg.n_walkers = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--barrier") == 0 && i + 1 < argc) {
            cfg.barrier = atof(argv[++i]);
        } else if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc) {
            cfg.seed = (unsigned int)strtoul(argv[++i], NULL, 10);
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            usage(argv[0]);
            return 0;
        } else {
            usage(argv[0]);
            return 1;
        }
    }

    if (!out_path) {
        snprintf(default_path, sizeof(default_path),
                 "results/walk_%s.dat", use_omp ? "omp" : "serial");
        out_path = default_path;
    }

    if (cfg.n_walkers <= 0 || cfg.barrier <= 0.0) {
        fprintf(stderr, "error: walkers and barrier must be positive\n");
        return 1;
    }

    samples = (double *)malloc((size_t)cfg.n_walkers * sizeof(double));
    if (!samples) {
        fprintf(stderr, "error: out of memory\n");
        return 1;
    }

    elapsed = walk_ensemble(&cfg, samples, use_omp);

    if (walk_write_results(out_path, samples, cfg.n_walkers) != 0) {
        fprintf(stderr, "error: could not write %s\n", out_path);
        free(samples);
        return 1;
    }

    printf("walk mode=%s walkers=%d barrier=%.3f time_s=%.6f out=%s\n",
           use_omp ? "omp" : "serial",
           cfg.n_walkers,
           cfg.barrier,
           elapsed,
           out_path);

    free(samples);
    return 0;
}
