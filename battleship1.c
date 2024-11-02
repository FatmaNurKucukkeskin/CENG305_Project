#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#define GRID_SIZE 8
#define EMPTY '.'
#define HIT 'X'
#define MISS 'O'
#define SHIP 'S'
#define SHIP_GAP 1

typedef struct {
    int size;
    int count;
} Ship;

Ship ships[] = {
    {4, 1}, // 1 Battleship (4 cells)
    {3, 2}, // 2 Cruisers (3 cells)
    {2, 2}  // 2 Destroyers (2 cells)
};

char grid1[GRID_SIZE][GRID_SIZE]; // Child's grid
char grid2[GRID_SIZE][GRID_SIZE]; // Parent's grid

void initializeGrid(char grid[GRID_SIZE][GRID_SIZE]) {
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            grid[i][j] = EMPTY;
        }
    }
}

void printGrid(char grid[GRID_SIZE][GRID_SIZE]) {
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            printf("%c ", grid[i][j]);
        }
        printf("\n");
    }
}

int canPlaceShip(char grid[GRID_SIZE][GRID_SIZE], int row, int col, int size, int orientation) {
    for (int i = 0; i < size; i++) {
        int r = row + (orientation == 0 ? 0 : i); // Horizontal: row remains same
        int c = col + (orientation == 0 ? i : 0); // Vertical: column remains same
        if (r < 0 || r >= GRID_SIZE || c < 0 || c >= GRID_SIZE || grid[r][c] != EMPTY) {
            return 0; // Cannot place ship
        }
        // Check for gaps
        if (r > 0 && grid[r - 1][c] == SHIP) return 0; // Above
        if (c > 0 && grid[r][c - 1] == SHIP) return 0; // Left
        if (r < GRID_SIZE - 1 && grid[r + 1][c] == SHIP) return 0; // Below
        if (c < GRID_SIZE - 1 && grid[r][c + 1] == SHIP) return 0; // Right
    }
    return 1; // Can place ship
}

void placeShip(char grid[GRID_SIZE][GRID_SIZE], int size) {
    int row, col, orientation;
    do {
        orientation = rand() % 2; // 0 for horizontal, 1 for vertical
        row = rand() % GRID_SIZE;
        col = rand() % GRID_SIZE;
    } while (!canPlaceShip(grid, row, col, size, orientation));

    for (int i = 0; i < size; i++) {
        grid[row + (orientation == 0 ? 0 : i)][col + (orientation == 0 ? i : 0)] = SHIP;
    }
}

void placeShips(char grid[GRID_SIZE][GRID_SIZE]) {
    for (int i = 0; i < sizeof(ships) / sizeof(ships[0]); i++) {
        for (int j = 0; j < ships[i].count; j++) {
            placeShip(grid, ships[i].size);
        }
    }
}

int checkHit(char grid[GRID_SIZE][GRID_SIZE], int row, int col) {
    if (grid[row][col] == SHIP) {
        grid[row][col] = HIT;
        return 1; // Hit
    } else {
        grid[row][col] = MISS;
        return 0; // Miss
    }
}

int allShipsSunk(char grid[GRID_SIZE][GRID_SIZE]) {
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            if (grid[i][j] == SHIP) return 0; // Ship still present
        }
    }
    return 1; // All ships sunk
}

void turn(char opponentGrid[GRID_SIZE][GRID_SIZE], char playerGrid[GRID_SIZE][GRID_SIZE], const char* playerName) {
    int row, col;
    do {
        row = rand() % GRID_SIZE;
        col = rand() % GRID_SIZE;
    } while (playerGrid[row][col] == HIT || playerGrid[row][col] == MISS);
    
    if (checkHit(playerGrid, row, col)) {
        printf("%s hits at (%d, %d)!\n", playerName, row, col);
    } else {
        printf("%s misses at (%d, %d).\n", playerName, row, col);
    }
}

int main() {
    int pipefds[2];
    pipe(pipefds);
    pid_t pid = fork();

    if (pid < 0) {
        perror("Fork failed");
        return 1;
    }

    if (pid == 0) { // Child process
        srand(getpid()); // Seed random number generator with process ID
        initializeGrid(grid1);
        placeShips(grid1);
        write(pipefds[1], grid1, sizeof(grid1)); // Send grid to parent
        close(pipefds[1]); // Close write end
        exit(0);
    } else { // Parent process
        srand(getpid() ^ time(NULL)); // Different seed for parent process
        initializeGrid(grid2);
        placeShips(grid2);

        read(pipefds[0], grid1, sizeof(grid1)); // Receive grid from child
        close(pipefds[0]); // Close read end

        printf("Grid of Child Process:\n");
        printGrid(grid1);
        printf("Grid of Parent Process:\n");
        printGrid(grid2);

        while (1) {
            // Parent's turn
            turn(grid1, grid2, "Parent");
            if (allShipsSunk(grid1)) {
                printf("Child process wins!\n");
                break;
            }

            // Child's turn
            turn(grid2, grid1, "Child");
            if (allShipsSunk(grid2)) {
                printf("Parent process wins!\n");
                break;
            }
        }
    }
    return 0;
}
