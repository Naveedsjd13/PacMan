#ifndef GHOST_PATHFINDING_CUH
#define GHOST_PATHFINDING_CUH

// ─────────────────────────────────────────────────────────────────────────────
//  Plain C interface — no C++ classes, no name mangling
//  This is what g++ sees. The actual CUDA code is in GhostPathfinding.cu
//  compiled separately by nvcc.
// ─────────────────────────────────────────────────────────────────────────────

#define GPU_CELL_SIZE 16
#define GPU_MAP_W     21
#define GPU_MAP_H     21
#define GPU_WALL      4
#define MAX_NODES     (GPU_MAP_W * GPU_MAP_H)

struct GhostGPUData
{
    short         x, y;
    unsigned char direction;
    unsigned char status;
};

struct PacmanGPUData
{
    short x, y;
};

// Plain C functions — no name mangling, g++ and MSVC agree on these
#ifdef __cplusplus
extern "C" {
#endif

void cuda_init(int ghost_count);
void cuda_upload_map(const int* cpu_map);
void cuda_compute(GhostGPUData* ghosts, short px, short py,
                  unsigned char* out_dirs, int count);
void cuda_cleanup();

#ifdef __cplusplus
}
#endif

#endif
