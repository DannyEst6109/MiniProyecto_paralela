#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <omp.h>
#include <chrono>
#include <fstream>  // Para guardar los resultados en un archivo

const int GRID_SIZE = 10;
const int INITIAL_PLANTS = 150;
const int INITIAL_HERBIVORES = 40;
const int INITIAL_CARNIVORES = 15;
const int TICKS = 20;

enum Species { EMPTY, PLANT, HERBIVORE, CARNIVORE };

struct Cell {
    Species species;
    int energy;  // Para herbívoros y carnívoros
    int noFoodTicks; // Contador de ticks sin comida para herbívoros
};

std::vector<std::vector<Cell>> grid(GRID_SIZE, std::vector<Cell>(GRID_SIZE, { EMPTY, 0, 0 }));

void initializeGrid() {
    srand(time(0));
    
    // Función para colocar especies en la cuadrícula de forma aleatoria
    auto placeSpecies = [](Species species, int count) {
        while (count > 0) {
            int x = rand() % GRID_SIZE;
            int y = rand() % GRID_SIZE;
            if (grid[x][y].species == EMPTY) {
                grid[x][y].species = species;
                if (species == HERBIVORE || species == CARNIVORE) {
                    grid[x][y].energy = 5;  // Energía inicial para herbívoros y carnívoros
                }
                count--;
            }
        }
    };

    // Inicialización de plantas, herbívoros y carnívoros
    placeSpecies(PLANT, INITIAL_PLANTS);
    placeSpecies(HERBIVORE, INITIAL_HERBIVORES);
    placeSpecies(CARNIVORE, INITIAL_CARNIVORES);
}

// Función para imprimir la cuadrícula en la consola
void printGrid(int tick, std::ofstream &outputFile) {
    outputFile << "Tick " << tick << ":\n";
    for (int i = 0; i < GRID_SIZE; ++i) {
        for (int j = 0; j < GRID_SIZE; ++j) {
            char c = ' ';
            switch (grid[i][j].species) {
                case PLANT: c = 'P'; break;
                case HERBIVORE: c = 'H'; break;
                case CARNIVORE: c = 'C'; break;
                case EMPTY: c = '.'; break;
            }
            outputFile << c << ' ';
        }
        outputFile << '\n';
    }
    outputFile << "\n";
}

// Función para mover a un herbívoro o carnívoro
void move(int x, int y, Species species) {
    int newX = x + (rand() % 3 - 1);
    int newY = y + (rand() % 3 - 1);
    if (newX >= 0 && newX < GRID_SIZE && newY >= 0 && newY < GRID_SIZE) {
        if (grid[newX][newY].species == EMPTY) {
            grid[newX][newY] = grid[x][y];
            grid[x][y].species = EMPTY;
        }
    }
}

// Función para actualizar el estado de un herbívoro
void updateHerbivore(int x, int y) {
    bool ate = false;
    // Buscar plantas adyacentes
    for (int dx = -1; dx <= 1; ++dx) {
        for (int dy = -1; dy <= 1; ++dy) {
            int nx = x + dx;
            int ny = y + dy;
            if (nx >= 0 && nx < GRID_SIZE && ny >= 0 && ny < GRID_SIZE) {
                if (grid[nx][ny].species == PLANT) {
                    grid[nx][ny].species = EMPTY;  // Comer la planta
                    grid[x][y].energy++;
                    ate = true;
                    grid[x][y].noFoodTicks = 0;  // Resetear el contador de ticks sin comida
                    break;
                }
            }
        }
    }
    if (!ate) {
        move(x, y, HERBIVORE);
        grid[x][y].energy--;
        grid[x][y].noFoodTicks++;  // Incrementar el contador de ticks sin comida
    }

    if (grid[x][y].energy <= 0 || grid[x][y].noFoodTicks >= 3) {
        grid[x][y].species = EMPTY;  // El herbívoro muere por falta de comida
    }
}

// Función para actualizar el estado de un carnívoro
void updateCarnivore(int x, int y) {
    bool ate = false;
    // Buscar herbívoros adyacentes
    for (int dx = -1; dx <= 1; ++dx) {
        for (int dy = -1; ++dy) {
            int nx = x + dx;
            int ny = y + dy;
            if (nx >= 0 && nx < GRID_SIZE && ny >= 0 && ny < GRID_SIZE) {
                if (grid[nx][ny].species == HERBIVORE) {
                    grid[nx][ny].species = EMPTY;  // Comer el herbívoro
                    grid[x][y].energy += 2;
                    ate = true;
                    break;
                }
            }
        }
    }
    if (!ate) {
        move(x, y, CARNIVORE);
        grid[x][y].energy--;
    }

    if (grid[x][y].energy <= 0) {
        grid[x][y].species = EMPTY;  // El carnívoro muere por falta de comida
    }
}

void runSimulation(bool useOpenMP) {
    initializeGrid();

    // Abrir un archivo para registrar los resultados
    std::ofstream outputFile("ecosystem_simulation.txt");
    if (!outputFile.is_open()) {
        std::cerr << "No se pudo abrir el archivo para escribir los resultados.\n";
        return;
    }

    auto start = std::chrono::high_resolution_clock::now(); // Iniciar medición del tiempo

    for (int tick = 1; tick <= TICKS; ++tick) {
        if (useOpenMP) {
            #pragma omp parallel for
            for (int i = 0; i < GRID_SIZE; ++i) {
                for (int j = 0; j < GRID_SIZE; ++j) {
                    switch (grid[i][j].species) {
                        case PLANT:
                            // Reproducción de plantas con probabilidad del 30%
                            if (rand() % 100 < 30) {
                                for (int dx = -1; dx <= 1; ++dx) {
                                    for (int dy = -1; dy <= 1; ++dy) {
                                        int nx = i + dx;
                                        int ny = j + dy;
                                        if (nx >= 0 && nx < GRID_SIZE && ny >= 0 && ny < GRID_SIZE) {
                                            if (grid[nx][ny].species == EMPTY) {
                                                grid[nx][ny].species = PLANT;
                                                break;
                                            }
                                        }
                                    }
                                }
                            }
                            break;
                        case HERBIVORE:
                            updateHerbivore(i, j);
                            break;
                        case CARNIVORE:
                            updateCarnivore(i, j);
                            break;
                        default:
                            break;
                    }
                }
            }
        } else {
            // Código secuencial sin OpenMP
            for (int i = 0; i < GRID_SIZE; ++i) {
                for (int j = 0; j < GRID_SIZE; ++j) {
                    switch (grid[i][j].species) {
                        case PLANT:
                            // Reproducción de plantas con probabilidad del 30%
                            if (rand() % 100 < 30) {
                                for (int dx = -1; dx <= 1; ++dx) {
                                    for (int dy = -1; dy <= 1; ++dy) {
                                        int nx = i + dx;
                                        int ny = j + dy;
                                        if (nx >= 0 && nx < GRID_SIZE && ny >= 0 && ny < GRID_SIZE) {
                                            if (grid[nx][ny].species == EMPTY) {
                                                grid[nx][ny].species = PLANT;
                                                break;
                                            }
                                        }
                                    }
                                }
                            }
                            break;
                        case HERBIVORE:
                            updateHerbivore(i, j);
                            break;
                        case CARNIVORE:
                            updateCarnivore(i, j);
                            break;
                        default:
                            break;
                    }
                }
            }
        }

        // Mostrar el estado del ecosistema después de cada tick
        printGrid(tick, outputFile);
    }

    auto end = std::chrono::high_resolution_clock::now(); // Finalizar medición del tiempo
    std::chrono::duration<double> duration = end - start;
    std::cout << "Tiempo de ejecución " << (useOpenMP ? "con" : "sin") << " OpenMP: " << duration.count() << " segundos.\n";

    outputFile.close();
}

int main() {
    std::cout << "Ejecutando simulación sin OpenMP...\n";
    runSimulation(false);

    std::cout << "Ejecutando simulación con OpenMP...\n";
    runSimulation(true);

    return 0;
}
