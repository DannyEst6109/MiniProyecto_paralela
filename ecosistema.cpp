#include <iostream>
#include <vector>
#include <omp.h>
#include <cstdlib>
#include <ctime>

const int GRID_SIZE = 20;
const int TICKS = 20;
const int NUM_PLANTS = 150;
const int NUM_HERBIVORES = 40;
const int NUM_CARNIVORES = 15;

enum Cell { EMPTY, PLANT, HERBIVORE, CARNIVORE };

struct Ecosystem {
    std::vector<std::vector<Cell>> grid;
    std::vector<std::vector<int>> energy;

    Ecosystem() : grid(GRID_SIZE, std::vector<Cell>(GRID_SIZE, EMPTY)),
                  energy(GRID_SIZE, std::vector<int>(GRID_SIZE, 0)) {
        srand(time(0));
        initialize(NUM_PLANTS, PLANT);
        initialize(NUM_HERBIVORES, HERBIVORE);
        initialize(NUM_CARNIVORES, CARNIVORE);
    }

    void initialize(int num, Cell type) {
        while (num > 0) {
            int x = rand() % GRID_SIZE;
            int y = rand() % GRID_SIZE;
            if (grid[x][y] == EMPTY) {
                grid[x][y] = type;
                energy[x][y] = (type == HERBIVORE || type == CARNIVORE) ? 10 : 0;
                num--;
            }
        }
    }

    void display() {
        for (int i = 0; i < GRID_SIZE; i++) {
            for (int j = 0; j < GRID_SIZE; j++) {
                switch (grid[i][j]) {
                    case EMPTY: std::cout << ". "; break;
                    case PLANT: std::cout << "P "; break;
                    case HERBIVORE: std::cout << "H "; break;
                    case CARNIVORE: std::cout << "C "; break;
                }
            }
            std::cout << std::endl;
        }
    }

    void update() {
        std::vector<std::vector<Cell>> newGrid = grid;
        std::vector<std::vector<int>> newEnergy = energy;

        #pragma omp parallel for collapse(2)
        for (int i = 0; i < GRID_SIZE; i++) {
            for (int j = 0; j < GRID_SIZE; j++) {
                if (grid[i][j] == PLANT) {
                    updatePlant(i, j, newGrid, newEnergy);
                } else if (grid[i][j] == HERBIVORE) {
                    updateHerbivore(i, j, newGrid, newEnergy);
                } else if (grid[i][j] == CARNIVORE) {
                    updateCarnivore(i, j, newGrid, newEnergy);
                }
            }
        }

        grid = newGrid;
        energy = newEnergy;
    }

    void updatePlant(int x, int y, std::vector<std::vector<Cell>> &newGrid, std::vector<std::vector<int>> &newEnergy) {
        if (rand() % 100 < 30) { // 30% probability to reproduce
            int dx = rand() % 3 - 1;
            int dy = rand() % 3 - 1;
            int newX = (x + dx + GRID_SIZE) % GRID_SIZE;
            int newY = (y + dy + GRID_SIZE) % GRID_SIZE;
            if (newGrid[newX][newY] == EMPTY) {
                newGrid[newX][newY] = PLANT;
            }
        }
    }

    void updateHerbivore(int x, int y, std::vector<std::vector<Cell>> &newGrid, std::vector<std::vector<int>> &newEnergy) {
        move(x, y, newGrid, newEnergy, HERBIVORE, PLANT);
        reproduce(x, y, newGrid, newEnergy, HERBIVORE);
        if (newEnergy[x][y] <= 0) {
            newGrid[x][y] = EMPTY;
        }
    }

    void updateCarnivore(int x, int y, std::vector<std::vector<Cell>> &newGrid, std::vector<std::vector<int>> &newEnergy) {
        move(x, y, newGrid, newEnergy, CARNIVORE, HERBIVORE);
        reproduce(x, y, newGrid, newEnergy, CARNIVORE);
        if (newEnergy[x][y] <= 0) {
            newGrid[x][y] = EMPTY;
        }
    }

    void move(int x, int y, std::vector<std::vector<Cell>> &newGrid, std::vector<std::vector<int>> &newEnergy, Cell type, Cell prey) {
        for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
                int newX = (x + dx + GRID_SIZE) % GRID_SIZE;
                int newY = (y + dy + GRID_SIZE) % GRID_SIZE;
                if (grid[newX][newY] == prey) {
                    newGrid[newX][newY] = type;
                    newEnergy[newX][newY] = newEnergy[x][y] + (type == HERBIVORE ? 1 : 2);
                    newGrid[x][y] = EMPTY;
                    newEnergy[x][y] = 0;
                    return;
                }
            }
        }
        int dx = rand() % 3 - 1;
        int dy = rand() % 3 - 1;
        int newX = (x + dx + GRID_SIZE) % GRID_SIZE;
        int newY = (y + dy + GRID_SIZE) % GRID_SIZE;
        if (newGrid[newX][newY] == EMPTY) {
            newGrid[newX][newY] = type;
            newEnergy[newX][newY] = newEnergy[x][y];
            newGrid[x][y] = EMPTY;
            newEnergy[x][y] = 0;
        }
    }

    void reproduce(int x, int y, std::vector<std::vector<Cell>> &newGrid, std::vector<std::vector<int>> &newEnergy, Cell type) {
        if (newEnergy[x][y] > 5 && rand() % 100 < 20) { // 20% probability to reproduce if enough energy
            int dx = rand() % 3 - 1;
            int dy = rand() % 3 - 1;
            int newX = (x + dx + GRID_SIZE) % GRID_SIZE;
            int newY = (y + dy + GRID_SIZE) % GRID_SIZE;
            if (newGrid[newX][newY] == EMPTY) {
                newGrid[newX][newY] = type;
                newEnergy[newX][newY] = 5;
                newEnergy[x][y] -= 5;
            }
        }
    }
};

int main() {
    Ecosystem ecosystem;

    for (int tick = 0; tick < TICKS; tick++) {
        std::cout << "Tick " << tick + 1 << std::endl;
        ecosystem.display();
        ecosystem.update();
        std::cout << std::endl;
    }

    return 0;
}
