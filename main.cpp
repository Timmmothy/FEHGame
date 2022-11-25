#include <iostream>
#include <math.h>
#include "FEHLCD.h"
#include "FEHUtility.h"
#include "main.h"

// Defines the window width and height
#define WINDOW_WIDTH 320
#define WINDOW_HEIGHT 240
#define PI 3.1415

using namespace std;
using namespace FEHIcon;

Player::Player() {
    width = 20;
    height = 20;

    x = (WINDOW_WIDTH - width) / 2;
    y = (WINDOW_HEIGHT - height) / 2;

    velX = 0;
    velY = 0;
    velG = -0.5;
    forceX = 0;
    forceY = 0;
    forceG = -0.5;

    mass = 10;
}

int* Player::getCenter() {
    int* center = new int[2];
    center[0] = x + width / 2;
    center[1] = y + height / 2;
    return center;
}

bool Player::pointInPlayer(int px, int py) {
    return x <= px && px <= x + width && y <= py && py <= y + height;
}

void Player::update() {
    float dragX = 0;
    float dragY = 0;

    // If there is a non-zero velocity, calculate a drag force
    if(velX || velY){
        float velAngle = atan2(velY, velX);
        float dragAngle = -velAngle;
        dragX = -cos(dragAngle);
        dragY = sin(dragAngle);
    }

    // Calculate net forces
    float netForceX = forceX + dragX;
    float netForceY = forceY + dragY;
    
    // Calculate net acceleration
    float netAccelX = netForceX / mass;
    float netAccelY = netForceY / mass;
    float netAccelG = forceG / mass;

    // Updates x and y position
    // y is inverted as a positive y force, which moves the player up, reduces the value of y on the screen
    x += velX;
    y -= velY;
    y -= velG;
    
    // Updates velocity
    velX += netAccelX;
    velY += netAccelY;
    velG += netAccelG;

    // Snaps velocities to 0 if it comes close enough
    float vMagnitude = sqrt(pow(velX, 2) + pow(velY, 2));
    if(vMagnitude < 0.1) {
        velX = 0;
        velY = 0;
    }  

    // If adding the drag would change the direciton of the force, set the force to 0
    // Else, add the drag to the force
    if((forceX >= 0 && forceX + dragX <= 0) || (forceX <= 0 && forceX + dragX >= 0)){
        forceX = 0;
    } else {
        forceX += dragX;
    }

    if((forceY >= 0 && forceY + dragY <= 0) || (forceY <= 0 && forceY + dragY >= 0)){
        forceY = 0;
    } else {
        forceY += dragY;
    }
}

void Player::shoot(float angle) {
    // Reset velocities after shooting
    velG = 0;
    velX = 0;
    velY = 0;

    // Add a force in the opposite direction of the shot
    forceX += -8.0 * cos(angle);
    forceY += -8.0 * sin(angle);

    // Append Bullet shooting away from player to game object's bullet vector
    int *center = getCenter();
    game->bullets.push_back(Bullet(center[0], center[1], angle));
}

void Player::render() {
    LCD.FillRectangle(x, y, width, height);
}

Bullet::Bullet(float initialX, float initialY, float a) {
    radius = 2;
    
    x = initialX;
    y = initialY;
    angle = a;
    
    velX = cos(a);
    velY = sin(a);
}

int* Bullet::getCenter() {
    int* center = new int[2];
    center[0] = x;
    center[1] = y;
    return center;
}

bool Bullet::pointInBullet(int px, int py) {
    return sqrt(pow(px - x, 2) + pow(py - y, 2)) <= radius;
}

void Bullet::update() {
    // Updates x and y position
    // y is inverted as a positive y force, which moves the player up, reduces the value of y on the screen
    x += velX;
    y -= velY;
}

void Bullet::render() {
    LCD.FillCircle(x, y, 2);
}

Enemy::Enemy(float initialX, float initialY, float a) {
    width = 15;
    height = 15;
    onScreen = false;

    x = initialX;
    y = initialY;

    velX = cos(a);
    velY = sin(a);
}

int* Enemy::getCenter() {
    int* center = new int[2];
    center[0] = x + width / 2;
    center[1] = y + height / 2;
    return center;
}

bool Enemy::pointInEnemy(int px, int py) {
    return x <= px && px <= x + width && y <= py && py <= y + height;
}

void Enemy::update() {
    // Updates x and y position
    // y is inverted as a positive y force, which moves the player up, reduces the value of y on the screen
    x += velX;
    y -= velY;
}

void Enemy::render() {
    LCD.SetFontColor(RED);
    LCD.FillRectangle(x, y, width, height);
    LCD.SetFontColor(WHITE);
}

Game::Game() {
    player.game = this;
    lastEnemySpawnTime = TimeNow();
}

Game::~Game() {
    // TODO: Figure this out
}

void Game::render() {
    LCD.Clear();
    player.render();

    for(Bullet &bullet: bullets) {
        bullet.render();
    }

    for(Enemy &enemy: enemies) {
        if(enemy.onScreen) {
            enemy.render();
        }
    }

    LCD.Update();
}

void Game::update() {
    // Update player object
    player.update();

    for(int i = 0; i < bullets.size(); i++) {
        // Create alias to bullets[i]
        Bullet &bullet = bullets[i];

        int bRadius = bullet.radius;
        
        // Update bullet object
        bullet.update();

        // If the bullet touches the edge, remove it from bullets vector
        if(bullet.x <= bRadius || bullet.x >= WINDOW_WIDTH - bRadius || bullet.y <= bRadius || bullet.y >= WINDOW_HEIGHT - bRadius) {
            bullets.erase(bullets.begin() + i, bullets.begin() + i + 1);
            i--;
        }
    }

    for(int i = 0; i < enemies.size(); i++) {
        // Create alias for enemies[i]
        Enemy &enemy = enemies[i];

        enemy.update();

        // If an enemy exits the screen, remove it from the enemies vector
        // If an enemy enters the screen, set onScreen property to true
        if(enemy.x <= 0 || enemy.x >= WINDOW_WIDTH - enemy.width || enemy.y <= 0 || enemy.y >= WINDOW_HEIGHT - enemy.height) {
            if(enemy.onScreen) {
                enemies.erase(enemies.begin() + i, enemies.begin() + i + 1);
                i--;
            }
        } else {
            if(!enemy.onScreen) {
                enemy.onScreen = true;
            }
        }
    }

    // Gets time since last enemy was spawned
    float timeSinceNewEnemy = TimeNow() - lastEnemySpawnTime;

    // If it has been 2 seconds since an enemy was spawned, spawn a new enemy and reset the spawn timer
    if(timeSinceNewEnemy > 2) {
        float initialX, initialY, angle;

        int side = rand() % 4;
        switch(side) {
            case 0:
                initialX = 340;
                initialY = (rand() % 280) - 20;
                break;
            case 1:
                initialX = (rand() % 360) - 20;
                initialY = -20;
                break;
            case 2:
                initialX = -20;
                initialY = (rand() % 280) - 20;
                break;
            case 3:
                initialX = (rand() % 360) - 20;
                initialY = 260;
                break;
        }

        angle = atan2(initialY - player.y, player.x - initialX);

        // Append new enemy to enemies vector
        enemies.push_back(Enemy(initialX, initialY, angle));
        lastEnemySpawnTime = TimeNow();
    }
}

bool Game::hasEnded() {
    return player.x <= 0 || player.x >= WINDOW_WIDTH - player.width || player.y <= 0 || player.y >= WINDOW_HEIGHT - player.height;
}

void Play(){
    // Stores whether the screen is being pressed
    bool pressed = false;

    // Location of where the screen has been pressed
    float x, y;

    // Game object
    Game game;

    // Alias/Reference to game's player object
    Player &player = game.player;

    // Keep running the game loop if the game has not ended
    while(!game.hasEnded()){
        // Render game
        game.render();

        // On click
        if(LCD.Touch(&x, &y) && !pressed) {
            pressed = true;
            if(!player.pointInPlayer(x, y)){
                int* center = player.getCenter();
                float dx = x - center[0];
                float dy = center[1] - y;
                float angle = atan2(dy, dx);
                
                player.shoot(angle);
            }
        } 
        // On Release
        else if(!LCD.Touch(&x, &y) && pressed) {
            pressed = false;
        }

        // Update game
        game.update();
    }
}

void Statistics() {
    Icon backButton;
    char backString[] = "Back";

    // Setting button width and height as constants
    int buttonWidth = 100;
    int buttonHeight = 40;

    // Initial XY position of back button    
    int buttonStartX = (WINDOW_WIDTH - buttonWidth) / 2;
    int buttonStartY = 150;

    backButton.SetProperties(backString, buttonStartX, buttonStartY, buttonWidth, buttonHeight, RED, WHITE);
    
    LCD.Clear();

    LCD.WriteAt("Viewing Statistics", (WINDOW_WIDTH / 2) - 108, 50);

    LCD.SetFontColor(RED);
    LCD.FillRectangle(buttonStartX, buttonStartY, buttonWidth, buttonHeight);
    backButton.Draw();

    float x, y;
    while(true){
        // Waits for someone to touch the screen
        while(!LCD.Touch(&x, &y));

        // Waits for someone to release their touch
        while(LCD.Touch(&x, &y));

        if(x >= buttonStartX && x <= buttonStartX + buttonWidth && y >= buttonStartY && y <= buttonStartY + buttonHeight){
            return;
        }
    }
}

void Instructions() {
    Icon backButton;
    char backString[] = "Back";

    // Setting button width and height as constants
    int buttonWidth = 100;
    int buttonHeight = 40;

    // Initial XY position of back button    
    int buttonStartX = (WINDOW_WIDTH - buttonWidth) / 2;
    int buttonStartY = 150;

    backButton.SetProperties(backString, buttonStartX, buttonStartY, buttonWidth, buttonHeight, RED, WHITE);
    
    LCD.Clear();

    LCD.WriteAt("Viewing Instructions", (WINDOW_WIDTH / 2) - 120, 50);

    LCD.SetFontColor(RED);
    LCD.FillRectangle(buttonStartX, buttonStartY, buttonWidth, buttonHeight);
    backButton.Draw();

    float x, y;
    while(true){
        // Waits for someone to touch the screen
        while(!LCD.Touch(&x, &y));

        // Waits for someone to release their touch
        while(LCD.Touch(&x, &y));

        if(x >= buttonStartX && x <= buttonStartX + buttonWidth && y >= buttonStartY && y <= buttonStartY + buttonHeight){
            return;
        }
    }
}

void Credits() {
    Icon backButton;
    char backString[] = "Back";

    // Setting button width and height as constants
    int buttonWidth = 100;
    int buttonHeight = 40;

    // Initial XY position of back button    
    int buttonStartX = (WINDOW_WIDTH - buttonWidth) / 2;
    int buttonStartY = 150;

    backButton.SetProperties(backString, buttonStartX, buttonStartY, buttonWidth, buttonHeight, RED, WHITE);
    
    LCD.Clear();

    LCD.WriteAt("Viewing Credits", (WINDOW_WIDTH / 2) - 90, 50);

    LCD.SetFontColor(RED);
    LCD.FillRectangle(buttonStartX, buttonStartY, buttonWidth, buttonHeight);
    backButton.Draw();

    float x, y;
    while(true){
        // Waits for someone to touch the screen
        while(!LCD.Touch(&x, &y));

        // Waits for someone to release their touch
        while(LCD.Touch(&x, &y));

        if(x >= buttonStartX && x <= buttonStartX + buttonWidth && y >= buttonStartY && y <= buttonStartY + buttonHeight){
            return;
        }
    }
}

void Menu() {
    // Icons for all the menu options
    Icon playButton;
    char playString[] = "Play Game";
    
    Icon viewStatistics;
    char statsString[] = "View Statistics";

    Icon viewInstructions;
    char instructionsString[] = "View Instructions";

    Icon viewCredits;
    char creditsString[] = "View Credits";

    // Setting button width and height as constants
    int buttonWidth = 220;
    int buttonHeight = 40;
    
    // All buttons start at the same X position
    int buttonStartX = (WINDOW_WIDTH - buttonWidth) / 2;

    // Initial Y positions for all the buttons
    int playStartY = 25, statsStartY = 75, instructionsStartY = 125, creditsStartY = 175;

    // Sets properties of all the buttons
    playButton.SetProperties(playString, buttonStartX, playStartY, buttonWidth, buttonHeight, GREEN, WHITE);
    viewStatistics.SetProperties(statsString, buttonStartX, statsStartY, buttonWidth, buttonHeight, BLUE, WHITE);
    viewInstructions.SetProperties(instructionsString, buttonStartX, instructionsStartY, buttonWidth, buttonHeight, WHITE, BLACK);
    viewCredits.SetProperties(creditsString, buttonStartX, creditsStartY, buttonWidth, buttonHeight, RED, WHITE);

    float x, y;
    while(true){
        // Clears the screen then draws all the buttons
        LCD.Clear();

        LCD.SetFontColor(GREEN);
        LCD.FillRectangle(buttonStartX, playStartY, buttonWidth, buttonHeight);
        playButton.Draw();

        LCD.SetFontColor(BLUE);
        LCD.FillRectangle(buttonStartX, statsStartY, buttonWidth, buttonHeight);
        viewStatistics.Draw();

        LCD.SetFontColor(WHITE);
        LCD.FillRectangle(buttonStartX, instructionsStartY, buttonWidth, buttonHeight);
        viewInstructions.Draw();

        LCD.SetFontColor(RED);
        LCD.FillRectangle(buttonStartX, creditsStartY, buttonWidth, buttonHeight);
        viewCredits.Draw();

        // Waits for someone to touch the screen
        while(!LCD.Touch(&x, &y));

        // Waits for someone to release their touch
        while(LCD.Touch(&x, &y));

        // Depending on where the touch was at the end, do the following actions
        if(x >= buttonStartX && x <= buttonStartX + buttonWidth){
            if(y >= playStartY && y <= playStartY + buttonHeight){
                Play();
                Sleep(1.0);
            } else if(y >= statsStartY && y <= statsStartY + buttonHeight){
                Statistics();
            } else if(y >= instructionsStartY && y <= instructionsStartY + buttonHeight){
                Instructions();
            } else if(y >= creditsStartY && y <= creditsStartY + buttonHeight){
                Credits();
            }
        }
    }
}

/* Entry point to the application */
int main() {
    // Draws the menu
    Menu();

    return 0;
}