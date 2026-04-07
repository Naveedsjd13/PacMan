#ifndef MPI_BRIDGE_CPP
#define MPI_BRIDGE_CPP

#include <mpi.h>
#include "Global.cpp"

// ─────────────────────────────────────────────────────────────
//  MPI Tags — each message type gets its own tag so they
//  never get mixed up
// ─────────────────────────────────────────────────────────────
constexpr int TAG_PACMAN_POS   = 1;  // Rank 0 → Rank 1: pacman position
constexpr int TAG_GHOST_POS    = 2;  // Rank 1 → Rank 0: all ghost positions
constexpr int TAG_GAME_STATE   = 3;  // Rank 0 → Rank 1: game state flags

// ─────────────────────────────────────────────────────────────
//  Data structures sent over MPI
// ─────────────────────────────────────────────────────────────

// Sent every frame from Rank 0 → Rank 1
struct PacmanData
{
    short x;           // pacman x position
    short y;           // pacman y position
    bool  energized;   // is pacman powered up?
};

// Sent every frame from Rank 1 → Rank 0
// 4 ghosts × 2 shorts (x, y) = 8 shorts total
struct GhostPositions
{
    short x[4];
    short y[4];
};

// Sent from Rank 0 → Rank 1 when game state changes
struct GameStateData
{
    bool running;    // false = tell Rank 1 to shut down
    bool reset;      // true  = reset ghost positions
};

// ─────────────────────────────────────────────────────────────
//  Send functions  (called by Rank 0)
// ─────────────────────────────────────────────────────────────

inline void mpi_send_pacman(const PacmanData& data)
{
    MPI_Send(&data, sizeof(PacmanData), MPI_BYTE, 1, TAG_PACMAN_POS, MPI_COMM_WORLD);
}

inline void mpi_send_game_state(const GameStateData& state)
{
    MPI_Send(&state, sizeof(GameStateData), MPI_BYTE, 1, TAG_GAME_STATE, MPI_COMM_WORLD);
}

// ─────────────────────────────────────────────────────────────
//  Receive functions  (called by Rank 0)
// ─────────────────────────────────────────────────────────────

inline GhostPositions mpi_recv_ghost_positions()
{
    GhostPositions pos{};
    MPI_Status status;
    MPI_Recv(&pos, sizeof(GhostPositions), MPI_BYTE, 1, TAG_GHOST_POS, MPI_COMM_WORLD, &status);
    return pos;
}

// ─────────────────────────────────────────────────────────────
//  Send functions  (called by Rank 1)
// ─────────────────────────────────────────────────────────────

inline void mpi_send_ghost_positions(const GhostPositions& pos)
{
    MPI_Send(&pos, sizeof(GhostPositions), MPI_BYTE, 0, TAG_GHOST_POS, MPI_COMM_WORLD);
}

// ─────────────────────────────────────────────────────────────
//  Receive functions  (called by Rank 1)
// ─────────────────────────────────────────────────────────────

inline PacmanData mpi_recv_pacman()
{
    PacmanData data{};
    MPI_Status status;
    MPI_Recv(&data, sizeof(PacmanData), MPI_BYTE, 0, TAG_PACMAN_POS, MPI_COMM_WORLD, &status);
    return data;
}

inline GameStateData mpi_recv_game_state()
{
    GameStateData state{};
    MPI_Status status;
    MPI_Recv(&state, sizeof(GameStateData), MPI_BYTE, 0, TAG_GAME_STATE, MPI_COMM_WORLD, &status);
    return state;
}

#endif