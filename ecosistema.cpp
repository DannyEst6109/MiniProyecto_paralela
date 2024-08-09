#include <iostream>
#include <vector>
#include <omp.h>
#include <cstdlib>
#include <ctime>
#include <chrono>

using namespace std::chrono;
const int GRID_SIZE = 20;
const int TICKS = 2000;
const int NUM_PLANTS = 150;
const int NUM_HERBIVORES = 40;
const int NUM_CARNIVORES = 15;

enum Cell { EMPTY, PLANT, HERBIVORE, CARNIVORE };

int carnivores = 0;
int plants = 0;
int herbivores = 0;

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
                energy[x][y] = (type == HERBIVORE || type == CARNIVORE) ? 100 : 0;
                num--;
            }
        }
    }

    void display() {
        for (int i = 0; i < GRID_SIZE; i++) {
            for (int j = 0; j < GRID_SIZE; j++) {
                switch (grid[i][j]) {
                    case EMPTY: std::cout << ". "; break;
                    case PLANT: std::cout << "P " ; break;
                    case HERBIVORE: std::cout << "H "; break;
                    case CARNIVORE: std::cout << "C ";break;
                }
            }
            std::cout << std::endl;
        }
        std::cout << "plants: " << plants << std::endl;
        std::cout << "carnivores: " << carnivores << std::endl;
        std::cout << "herbivores: " << herbivores << std::endl;
        std::cout << std::endl;
    }

    void update() {
        std::vector<std::vector<Cell>> newGrid = grid;
        std::vector<std::vector<int>> newEnergy = energy;

        carnivores = 0;
        plants = 0;
        herbivores = 0;
        int j;
        int i;
        #pragma omp parallel for private(j) collapse(2) num_threads(5)
        for ( i = 0; i < GRID_SIZE; i++) {
            for (j = 0; j < GRID_SIZE; j++) {
                
                if (grid[i][j] == PLANT) {
                    updatePlant(i, j, newGrid, newEnergy);
                    #pragma omp atomic
                    plants ++;
                } else if (grid[i][j] == HERBIVORE) {
                    updateHerbivore(i, j, newGrid, newEnergy);
                    #pragma omp atomic
                    herbivores ++;
                } else if (grid[i][j] == CARNIVORE) {
                    updateCarnivore(i, j, newGrid, newEnergy);
                    #pragma omp atomic
                    carnivores ++;
                }
            }
        }

        // Update global counters using atomic operations
       

        grid = newGrid;
        energy = newEnergy;
    }

    void updatePlant(int x, int y, std::vector<std::vector<Cell>> &newGrid, std::vector<std::vector<int>> &newEnergy) {
        //check if the plant is not surrounded by plants, must have at least 4 free spaces
        /*
        checks in a grid of 3x3 sourrinding the current plant
        P P P 
        P Pc P 
        P P P
        in this case the center one is the current , should be death , becomming
        P P P 
        P . P 
        P P P
        */
        //assume is surrounded ( is easier to check if is not rather than if it is)
        bool decay = true;
        for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
                int newX = (x + dx + GRID_SIZE) % GRID_SIZE;
                int newY = (y + dy + GRID_SIZE) % GRID_SIZE;
                if (newX >= 0 && newX < GRID_SIZE && newY >= 0 && newY < GRID_SIZE) { //avoid out of bounds checking
                if (newGrid[newX][newY] != PLANT) {
                    decay = false;
                    break;
                }}
            }
        }
        
        //kill the plant if surrounded
        if (decay) {
            newGrid[x][y] = EMPTY;
        } 
        else if (rand() % 100 < 30) { // 30% probability to reproduce
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
        std::pair<int,int> newPos = move(x, y, newGrid, newEnergy, CARNIVORE, HERBIVORE);
        reproduce(newPos.first, newPos.second, newGrid, newEnergy, CARNIVORE);
        if (newEnergy[newPos.first][newPos.second] <= 0) {
            newEnergy[newPos.first][newPos.second] = EMPTY;
        }
    }

    std::pair<int, int> move(int x, int y, std::vector<std::vector<Cell>> &newGrid, std::vector<std::vector<int>> &newEnergy, Cell type, Cell prey) {
        for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
                int newX = (x + dx + GRID_SIZE) % GRID_SIZE;
                int newY = (y + dy + GRID_SIZE) % GRID_SIZE;
                if (grid[newX][newY] == prey) {
                    newGrid[newX][newY] = type;
                    newEnergy[newX][newY] = newEnergy[x][y] + (type == HERBIVORE ? 3 : 2);
                    newGrid[x][y] = EMPTY;
                    newEnergy[x][y] = 0;
                    return {newX, newY};
                }
            }
        }
        int dx = rand() % 3 - 1;
        int dy = rand() % 3 - 1;
        int newX = (x + dx + GRID_SIZE) % GRID_SIZE;
        int newY = (y + dy + GRID_SIZE) % GRID_SIZE;
        if (newGrid[newX][newY] == EMPTY) {
            newGrid[newX][newY] = type;
            newEnergy[newX][newY] = newEnergy[x][y] - 1; //energy decay
            newGrid[x][y] = EMPTY;
            newEnergy[x][y] = 0;
            return {newX,newY};
        }
        return {x,y};
    }

    void reproduce(int x, int y, std::vector<std::vector<Cell>> &newGrid, std::vector<std::vector<int>> &newEnergy, Cell type) {
        if (newEnergy[x][y] > 3 && rand() % 100 < 30) { // 30% probability to reproduce if enough energy
            bool placed = false;
            int dx = rand() % 3 - 1;
            int dy = rand() % 3 - 1;
            int newX = (x + dx + GRID_SIZE) % GRID_SIZE;
            int newY = (y + dy + GRID_SIZE) % GRID_SIZE;
            if (newGrid[newX][newY] == EMPTY) {
                newGrid[newX][newY] = type;
                newEnergy[newX][newY] = 100;
                newEnergy[x][y] -= 4;
                placed = true;
            }
        }
    }
};

int main() {
    
    Ecosystem ecosystem;
    auto start = high_resolution_clock::now();
    for (int tick = 0; tick < TICKS; tick++) {
        std::cout << "Tick " << tick + 1 << std::endl;
        ecosystem.display();
        ecosystem.update();
        std::cout << std::endl;
    }
    auto end = high_resolution_clock::now();
    std::cout << "excecution time"<< duration_cast<milliseconds>(end - start).count() << "ms" << std::endl;
    return 0;
}
