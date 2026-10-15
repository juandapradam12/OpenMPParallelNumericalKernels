#include "advection.h"
#include "timing.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(const char *prog)
{
    fprintf(stderr,
            "Usage: %s [--omp] [--out DIR] [--steps N] [--points N] [--dt DT] [--no-snapshots]\n"
            "  Solves the 1D advection equation with a first-order upwind scheme.\n"
            "  Writes the spatial grid and field snapshots under DIR (default: results).\n",
            prog);
}

int main(int argc, char **argv)
{
    AdvectionConfig cfg = advection_default_config();
    AdvectionState state;
    int use_omp = 0;
    int write_snapshots = 1;
    const char *out_dir = "results";
    char grid_path[512];
    char prefix_buf[512];
    const char *prefix = NULL;
    int snapshots[3];
    double elapsed;
    int i;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--omp") == 0) {
            use_omp = 1;
        } else if (strcmp(argv[i], "--out") == 0 && i + 1 < argc) {
            out_dir = argv[++i];
        } else if (strcmp(argv[i], "--steps") == 0 && i + 1 < argc) {
            cfg.n_steps = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--points") == 0 && i + 1 < argc) {
            cfg.n_points = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--dt") == 0 && i + 1 < argc) {
            cfg.dt = atof(argv[++i]);
        } else if (strcmp(argv[i], "--no-snapshots") == 0) {
            write_snapshots = 0;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            usage(argv[0]);
            return 0;
        } else {
            usage(argv[0]);
            return 1;
        }
    }

    if (advection_init(&state, &cfg) != 0) {
        fprintf(stderr, "error: failed to initialize advection state\n");
        return 1;
    }

    if (state.alpha > 1.0) {
        fprintf(stderr,
                "warning: CFL number alpha = %.4f > 1; scheme may be unstable\n",
                state.alpha);
    }

    snapshots[0] = 0;
    snapshots[1] = cfg.n_steps / 2;
    snapshots[2] = cfg.n_steps;

    if (write_snapshots) {
        snprintf(grid_path, sizeof(grid_path), "%s/advection_x.dat", out_dir);
        snprintf(prefix_buf, sizeof(prefix_buf), "%s/advection_%s", out_dir,
                 use_omp ? "omp" : "serial");
        prefix = prefix_buf;

        if (advection_write_grid(&state, grid_path) != 0) {
            fprintf(stderr, "error: could not write grid to %s\n", grid_path);
            advection_free(&state);
            return 1;
        }
    }

    elapsed = advection_run(&state, &cfg, use_omp, snapshots, 3, prefix);

    printf("advection mode=%s points=%d steps=%d cfl=%.6f time_s=%.6f\n",
           use_omp ? "omp" : "serial",
           cfg.n_points,
           cfg.n_steps,
           state.alpha,
           elapsed);

    advection_free(&state);
    return 0;
}
