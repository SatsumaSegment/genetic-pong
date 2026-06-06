#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include <stdlib.h>

#define PADDLE_HEIGHT 80
#define PADDLE_WIDTH 20
#define PADDLE_SPEED 5.0f

#define BALL_RADIUS 10.0f

#define MAX_POPULATION 100

#define MUTATION_RATE 0.05f

#define INPUT_WEIGHTS 5
#define OUTPUT_WEIGHTS 2
#define HIDDEN_WEIGHTS 8

// Collision detection substeps help prevent tunneling when ball gets insanely fast - 6 is probably the best number of sub steps (managed to get 473 hits by generation 106)
#define SUB_STEPS 6


typedef struct {
    Rectangle rect;
    float speed;

    float weights[(INPUT_WEIGHTS*HIDDEN_WEIGHTS) + (HIDDEN_WEIGHTS*OUTPUT_WEIGHTS)];

    int hits;
    int isAlive;
} GAPaddle;

typedef struct {
    Vector2 ballPos;
    Vector2 ballDir;
    float ballSpeed;
} Ball;

typedef struct {
    GAPaddle paddle;
    Ball ball;
} Agent;


void getState(Ball *ball, GAPaddle *paddle, float inputs[INPUT_WEIGHTS]) {
    inputs[0] = paddle->rect.y / GetScreenHeight();
    inputs[1] = ball->ballPos.x / GetScreenWidth();
    inputs[2] = ball->ballPos.y / GetScreenHeight();
    inputs[3] = ball->ballDir.x / ball->ballSpeed;
    inputs[4] = ball->ballDir.y / ball->ballSpeed;
}


void think(Ball *ball, GAPaddle *paddle, float outputs[OUTPUT_WEIGHTS]) {
    float input[INPUT_WEIGHTS];
    getState(ball, paddle, input);

    float hidden[HIDDEN_WEIGHTS];

    // Populate hidden weights array
    int weightCounter = 0;
    for (int i = 0; i < HIDDEN_WEIGHTS; i++) {
        float calculated = 0;
        for (int j = 0; j < INPUT_WEIGHTS; j++) {
            calculated += input[j] * paddle->weights[weightCounter];
            weightCounter++;
        }
        if (calculated< 0) calculated = 0;
        hidden[i] = calculated;
    }

    // Calculate the outputs
    for (int i = 0; i < OUTPUT_WEIGHTS; i++) {
        float calculated = 0;
        for (int j = 0; j < HIDDEN_WEIGHTS; j++) {
            calculated += hidden[j] * paddle->weights[weightCounter];
            weightCounter++;
        }
        outputs[i] = calculated;
    }
}


void evolvePopulation(Agent currentGen[MAX_POPULATION], Agent nextGen[MAX_POPULATION]) {
    // Sort the current generation by hits (most hits to least hits)
    for (int i = 0; i < MAX_POPULATION; i++) {
        for (int j = 0; j < MAX_POPULATION-1; j++) {
            GAPaddle swap;
            if (currentGen[j].paddle.hits < currentGen[j+1].paddle.hits) {
                swap = currentGen[j].paddle;
                currentGen[j].paddle = currentGen[j+1].paddle;
                currentGen[j+1].paddle = swap;
            }
        }
    }

    printf("Top paddle weights:\n");
    for (int i = 0; i < (INPUT_WEIGHTS*HIDDEN_WEIGHTS) + (HIDDEN_WEIGHTS*OUTPUT_WEIGHTS); i++) {
        printf("%f, ", currentGen[0].paddle.weights[i]);
    }
    printf("\n\n");
    printf("Top paddle hits: %d", currentGen[0].paddle.hits);
    printf("\n------------------------------------------------------------------------------------------------------\n");

    
    // Pass top 10% to next generation
    int nElites = MAX_POPULATION / 10;
    GAPaddle parents[nElites];
    for (int i = 0; i < nElites; i++) {
        nextGen[i].paddle = currentGen[i].paddle;
        parents[i] = currentGen[i].paddle;
        nextGen[i].paddle.hits = 0;
        nextGen[i].paddle.isAlive = 1;
    }

    const int max_weights = (INPUT_WEIGHTS*HIDDEN_WEIGHTS) + (HIDDEN_WEIGHTS*OUTPUT_WEIGHTS);
    // Fill the rest of the next generation's population with children
    for (int i = nElites; i < MAX_POPULATION; i++) {
        // Get two random parents
        int parentA = GetRandomValue(0, nElites - 1);
        int parentB = GetRandomValue(0, nElites - 1);
        while (parentA == parentB) {
            parentB = GetRandomValue(0, nElites - 1);
        }
        
        // Get random slice point of the DNA
        int sliceIdx = GetRandomValue(0, max_weights-1);
        GAPaddle child;
        for (int j = 0; j < sliceIdx; j++) {
            child.weights[j] = parents[parentA].weights[j];
        }
        for (int j = sliceIdx; j < max_weights; j++) {
            child.weights[j] = parents[parentB].weights[j];
        }

        for (int j = 0; j < max_weights; j++) {
            float mutate = (float)GetRandomValue(0, 1000) / 1000.0f;
            if (mutate < MUTATION_RATE) {
                float mutationAmnt = (float)GetRandomValue(-500, 500) / 1000.0f;
                child.weights[j] += mutationAmnt;
            }
        }
        child.hits = 0;
        child.isAlive = 1;
        child.speed = PADDLE_SPEED;
        nextGen[i].paddle = child;
    }
}


void initializeAgent(Agent *agent, int generation) {
    agent->paddle.rect.y = (GetScreenHeight() - PADDLE_HEIGHT) / 2;
    agent->paddle.rect.x = (GetScreenWidth() - PADDLE_WIDTH) - 10;
    agent->paddle.rect.width = PADDLE_WIDTH;
    agent->paddle.rect.height = PADDLE_HEIGHT;
    agent->paddle.hits = 0;
    agent->paddle.isAlive = 1;
    agent->paddle.speed = 5.0f;

    agent->ball.ballPos.x = GetScreenWidth()/2 ;
    agent->ball.ballPos.y = GetScreenHeight()/2;
    agent->ball.ballSpeed = 3.0f;
    agent->ball.ballDir.x = agent->ball.ballSpeed;
    agent->ball.ballDir.y = agent->ball.ballSpeed;

    if (generation == 0) {
        for (int i = 0; i < (INPUT_WEIGHTS*HIDDEN_WEIGHTS) + (HIDDEN_WEIGHTS*OUTPUT_WEIGHTS); i++) {
            agent->paddle.weights[i] = (float)GetRandomValue(-1000, 1000) / 1000.0f;
        }
    }
}

int main(void) {
    InitWindow(800, 450, "PONG");
    SetTargetFPS(60);

    int p1Y = (GetScreenHeight() - PADDLE_HEIGHT) / 2;
    int p1X = 10;
    Rectangle p1 = {p1X, p1Y, PADDLE_WIDTH, PADDLE_HEIGHT};

    int generation = 0;
    Agent currentGen[MAX_POPULATION];
    Agent nextGen[MAX_POPULATION];
    
    for(int i = 0; i < MAX_POPULATION; i++) {
        initializeAgent(&currentGen[i], generation); 
    }

    int simulationSpeed = 1;
    int subSteps = 6;

    while (!WindowShouldClose()) {

        if (IsKeyDown(KEY_UP)) {
            simulationSpeed++;
        } else if (IsKeyDown(KEY_DOWN) && simulationSpeed > 1) {
            simulationSpeed--;
        }
        

        for (int step = 0; step < simulationSpeed; step++) {

            int aliveCount = 0;

            for (int i = 0; i < MAX_POPULATION; i++) {
                if (currentGen[i].paddle.isAlive) {
                    aliveCount++;

                    // Paddles that are still alive think about what to do next
                    float outputs[OUTPUT_WEIGHTS];
                    think(&currentGen[i].ball, &currentGen[i].paddle, outputs);

                    // Translate thoughts into paddle action
                    if (outputs[0] > 0.5f && outputs[0] > outputs[1] && currentGen[i].paddle.rect.y > 0) {
                        currentGen[i].paddle.rect.y -= currentGen[i].paddle.speed;
                    } else if (outputs[1] > 0.5f && outputs[1] > outputs[0] && currentGen[i].paddle.rect.y < (GetScreenHeight()-PADDLE_HEIGHT)) {
                        currentGen[i].paddle.rect.y += currentGen[i].paddle.speed;
                    }

                    // Check ball collisions with walls
                    if ((currentGen[i].ball.ballPos.y + BALL_RADIUS) >= GetScreenHeight()) {
                        currentGen[i].ball.ballDir.y = -currentGen[i].ball.ballSpeed;
                    } else if ((currentGen[i].ball.ballPos.y - BALL_RADIUS) <= 0) {
                        currentGen[i].ball.ballDir.y = currentGen[i].ball.ballSpeed;
                    }
                    // Reset ball to centre if left or right wall is hit. Right wall hit means paddle is dead
                    if ((currentGen[i].ball.ballPos.x + BALL_RADIUS) >= GetScreenWidth()) {
                        currentGen[i].paddle.isAlive = 0;
                        continue; // Skip extra processing if paddle died
                    } else if ((currentGen[i].ball.ballPos.x - BALL_RADIUS) <= 0) {
                        currentGen[i].ball.ballDir.x = currentGen[i].ball.ballSpeed;
                    }

                    for (int s = 0; s < SUB_STEPS; s++) {

                        // Update the ball for the specific Agent
                        currentGen[i].ball.ballPos.x += (currentGen[i].ball.ballDir.x / SUB_STEPS);
                        currentGen[i].ball.ballPos.y += (currentGen[i].ball.ballDir.y / SUB_STEPS);
                        
                        // Check for collision with paddle
                        if (CheckCollisionCircleRec(currentGen[i].ball.ballPos, BALL_RADIUS, currentGen[i].paddle.rect)) {
                            currentGen[i].paddle.hits++;
                            currentGen[i].paddle.speed += 0.2f;
                            currentGen[i].ball.ballPos.x = currentGen[i].paddle.rect.x - BALL_RADIUS - 1;
                            currentGen[i].ball.ballSpeed += 0.2f;
                            currentGen[i].ball.ballDir.x = -currentGen[i].ball.ballSpeed;
                            break;
                        }
                    }

                }
            }

            if (aliveCount == 0) {
                // Evolve the next generation of paddles
                printf("------------------------------------------------------------------------------------------------------\nGeneration: %d\n\n", generation);
                evolvePopulation(currentGen, nextGen);
                generation++;

                // Reset paddle and ball positions
                for(int i = 0; i < MAX_POPULATION; i++) {
                    currentGen[i] = nextGen[i];
                    initializeAgent(&currentGen[i], generation); 
                }
            }

        }
        

        BeginDrawing();
            
            ClearBackground(BLACK);

            for (int i = 0; i < MAX_POPULATION; i++) {
                if (currentGen[i].paddle.isAlive) {
                    DrawRectangleRec(currentGen[i].paddle.rect, (Color){255, 255, 155+i, 100});
                    DrawCircleV(currentGen[i].ball.ballPos, BALL_RADIUS, (Color){0, 255, 0, 100});
                }
            }

            DrawText(TextFormat("Generation: %d", generation), 10, 10, 20, GRAY);
            DrawText(TextFormat("Simulation Speed: %d", simulationSpeed), 250, 10, 20, GRAY);

        EndDrawing();
                
    }

    CloseWindow();        

    return 0;
}