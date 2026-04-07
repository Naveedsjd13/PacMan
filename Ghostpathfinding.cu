#include "GhostPathfinding.cuh"
#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <float.h>
#include <cstdio>

// ─────────────────────────────────────────────────────────────────────────────
//  A* Node
// ─────────────────────────────────────────────────────────────────────────────
struct AStarNode
{
    float g, f;
    int   parent;
    bool  open, closed;
};

// ─────────────────────────────────────────────────────────────────────────────
//  GPU helpers
// ─────────────────────────────────────────────────────────────────────────────
__device__ int to_tile(short px, short py)
{
    int tx = px / GPU_CELL_SIZE;
    int ty = py / GPU_CELL_SIZE;
    if (tx < 0 || tx >= GPU_MAP_W || ty < 0 || ty >= GPU_MAP_H) return -1;
    return tx * GPU_MAP_H + ty;
}

__device__ bool is_wall(int tile, const int* map)
{
    if (tile < 0 || tile >= MAX_NODES) return true;
    return map[tile] == GPU_WALL;
}

__device__ float heuristic(int a, int b)
{
    return (float)(abs(a / GPU_MAP_H - b / GPU_MAP_H) +
                   abs(a % GPU_MAP_H - b % GPU_MAP_H));
}

__device__ int neighbour(int tile, int d)
{
    int tx = tile / GPU_MAP_H;
    int ty = tile % GPU_MAP_H;
    if (d == 0) tx++;
    if (d == 1) ty--;
    if (d == 2) tx--;
    if (d == 3) ty++;
    if (tx < 0)          tx = GPU_MAP_W - 1;
    if (tx >= GPU_MAP_W) tx = 0;
    if (ty < 0 || ty >= GPU_MAP_H) return -1;
    return tx * GPU_MAP_H + ty;
}

// ─────────────────────────────────────────────────────────────────────────────
//  A* — one thread per ghost on GPU
// ─────────────────────────────────────────────────────────────────────────────
__device__ unsigned char astar(
    int start, int goal, unsigned char cur_dir,
    const int* map, AStarNode* nodes)
{
    for (int i = 0; i < MAX_NODES; i++)
        nodes[i] = { FLT_MAX, FLT_MAX, -1, false, false };

    nodes[start].g    = 0.0f;
    nodes[start].f    = heuristic(start, goal);
    nodes[start].open = true;

    for (int iter = 0; iter < MAX_NODES; iter++)
    {
        int current = -1; float best = FLT_MAX;
        for (int i = 0; i < MAX_NODES; i++)
            if (nodes[i].open && nodes[i].f < best)
            { best = nodes[i].f; current = i; }

        if (current == -1 || current == goal) break;

        nodes[current].open   = false;
        nodes[current].closed = true;

        for (int d = 0; d < 4; d++)
        {
            int nb = neighbour(current, d);
            if (nb == -1 || is_wall(nb, map) || nodes[nb].closed) continue;
            float ng = nodes[current].g + 1.0f;
            if (ng < nodes[nb].g)
            {
                nodes[nb].g      = ng;
                nodes[nb].f      = ng + heuristic(nb, goal);
                nodes[nb].parent = current;
                nodes[nb].open   = true;
            }
        }
    }

    // Trace back to find first step
    int step = goal;
    while (step != -1 && nodes[step].parent != start && nodes[step].parent != -1)
        step = nodes[step].parent;

    unsigned char opp = (cur_dir + 2) % 4;
    for (int d = 0; d < 4; d++)
        if (d != opp && neighbour(start, d) == step)
            return (unsigned char)d;

    return cur_dir;
}

// ─────────────────────────────────────────────────────────────────────────────
//  CUDA kernel — 1 thread per ghost
// ─────────────────────────────────────────────────────────────────────────────
__global__ void ghostAStarKernel(
    GhostGPUData*        ghosts,
    const PacmanGPUData* pacman,
    const int*           map,
    unsigned char*       out_dirs,
    int                  count)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= count) return;

    GhostGPUData g = ghosts[i];

    if (g.status != 2 || g.x % GPU_CELL_SIZE != 0 || g.y % GPU_CELL_SIZE != 0)
    { out_dirs[i] = g.direction; return; }

    int start = to_tile(g.x, g.y);
    int goal  = to_tile(pacman->x, pacman->y);
    if (start == -1 || goal == -1 || start == goal)
    { out_dirs[i] = g.direction; return; }

    AStarNode nodes[MAX_NODES];
    out_dirs[i] = astar(start, goal, g.direction, map, nodes);
}

// ─────────────────────────────────────────────────────────────────────────────
//  GPU memory — file-scope static (no C++ class needed)
// ─────────────────────────────────────────────────────────────────────────────
static GhostGPUData*  d_ghosts = nullptr;
static PacmanGPUData* d_pacman = nullptr;
static int*           d_map    = nullptr;
static unsigned char* d_dirs   = nullptr;
static int            g_count  = 0;

// ─────────────────────────────────────────────────────────────────────────────
//  Plain C exports — called from GhostManager.cpp via g++
// ─────────────────────────────────────────────────────────────────────────────
extern "C"
{
    void cuda_init(int ghost_count)
    {
        g_count = ghost_count;
        cudaMalloc(&d_ghosts, g_count * sizeof(GhostGPUData));
        cudaMalloc(&d_pacman, sizeof(PacmanGPUData));
        cudaMalloc(&d_map,    MAX_NODES * sizeof(int));
        cudaMalloc(&d_dirs,   g_count * sizeof(unsigned char));
        printf("[CUDA] A* pathfinder ready for %d ghosts\n", g_count);
    }

    void cuda_upload_map(const int* cpu_map)
    {
        cudaMemcpy(d_map, cpu_map, MAX_NODES * sizeof(int), cudaMemcpyHostToDevice);
        printf("[CUDA] Map uploaded to GPU (%d tiles)\n", MAX_NODES);
    }
    
    void cuda_compute(GhostGPUData* cpu_ghosts, short px, short py,
                      unsigned char* out, int count)
    {
        if (!d_ghosts) return;

        cudaMemcpy(d_ghosts, cpu_ghosts, count * sizeof(GhostGPUData), cudaMemcpyHostToDevice);
        PacmanGPUData pac{px, py};
        cudaMemcpy(d_pacman, &pac, sizeof(PacmanGPUData), cudaMemcpyHostToDevice);

        ghostAStarKernel<<<1, count>>>(d_ghosts, d_pacman, d_map, d_dirs, count);
        cudaDeviceSynchronize();

        cudaMemcpy(out, d_dirs, count * sizeof(unsigned char), cudaMemcpyDeviceToHost);
    }

    void cuda_cleanup()
    {
        if (d_ghosts) { cudaFree(d_ghosts); d_ghosts = nullptr; }
        if (d_pacman) { cudaFree(d_pacman); d_pacman = nullptr; }
        if (d_map)    { cudaFree(d_map);    d_map    = nullptr; }
        if (d_dirs)   { cudaFree(d_dirs);   d_dirs   = nullptr; }
        printf("[CUDA] GPU memory freed\n");
    }
}
