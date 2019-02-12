#include <SoftwareSerial.h>

SoftwareSerial BTSerial(10, 11); // RX | TX

// Map logical rows and columns to arduino pins
const byte row[8] = {7, 2, 12, 4, 19, 13, 18, 15};
const byte col[8] = {3, 17, 16, 6, 14, 5, 1, 0};

const int LPD = 1000; //Number of draws per call to draw methods (lower -> more difficult)

// The logical game board
byte matrix[8][8];

byte zero[][8]  = {{0,0,0,0},{0,1,1,0},{1,0,0,1},{1,0,0,1},{1,0,0,1},{1,0,0,1},{0,1,1,0},{0,0,0,0}};
byte one[][8]   = {{0,0,0,0},{0,0,1,0},{0,0,1,0},{0,0,1,0},{0,0,1,0},{0,0,1,0},{0,0,1,0},{0,0,0,0}};
byte two[][8]   = {{0,0,0,0},{0,1,1,0},{1,0,0,1},{0,0,0,1},{0,1,1,0},{1,0,0,0},{1,1,1,1},{0,0,0,0}};
byte three[][8] = {{0,0,0,0},{0,1,1,0},{0,0,0,1},{0,0,0,1},{0,1,1,0},{0,0,0,1},{0,1,1,0},{0,0,0,0}};
byte four[][8]  = {{0,0,0,0},{1,0,0,1},{1,0,0,1},{1,1,1,1},{0,0,0,1},{0,0,0,1},{0,0,0,1},{0,0,0,0}};
byte five[][8]  = {{0,0,0,0},{0,1,1,1},{1,0,0,0},{0,1,1,1},{0,0,0,1},{0,0,0,1},{1,1,1,0},{0,0,0,0}};
byte six[][8]   = {{0,0,0,0},{0,0,1,1},{0,1,0,0},{0,1,0,0},{0,1,1,1},{0,1,0,1},{0,1,1,1},{0,0,0,0}};
byte seven[][8] = {{0,0,0,0},{1,1,1,0},{0,0,1,0},{0,0,1,0},{0,0,1,0},{0,0,1,0},{0,0,1,0},{0,0,0,0}};
byte eight[][8] = {{0,0,0,0},{1,1,1,1},{1,0,0,1},{1,1,1,1},{1,0,0,1},{1,0,0,1},{1,1,1,1},{0,0,0,0}};
byte nine[][8]  = {{0,0,0,0},{1,1,1,0},{1,0,1,0},{1,1,1,0},{0,0,1,0},{0,0,1,0},{0,0,1,0},{0,0,0,0}};

// Nodes in the linked list
struct snake_block {
  byte px; // x position
  byte py; // y position
  snake_block *next;
};

struct snake_food {
  byte px;
  byte py;
};

snake_food *food = new snake_food();
snake_block *tail; // Root of the snake
snake_block *original_tail;

void spawn_food() {
  byte x = rand()%8;
  byte y = rand()%8;
  while (matrix[y][x] != 0) {
    x = rand()%8;
    y = rand()%8;
  }
  food->px = x;
  food->py = y;
  matrix[y][x] = 1;
}

// Link the snake blocks together into a snake
void init_snake() {
  snake_block *head = new snake_block();
  head->px = 3;
  head->py = 3;
  head->next = 0;

  snake_block *middle = new snake_block();
  middle->px = 3;
  middle->py = 4;
  middle->next = head;

  tail = new snake_block();
  original_tail = tail;
  tail->px = 3;
  tail->py = 5;
  tail->next = middle;

  // Put the snake onto the board
  matrix[head->py][head->px] = 1;
  matrix[middle->py][middle->px] = 1;
  matrix[tail->py][tail->px] = 1;
}

void grow_snake() {
  snake_block *new_tail = new snake_block();
  new_tail->px = tail->px;
  new_tail->py = tail->py;
  new_tail->next = tail;
  tail = new_tail;
}

byte dx = 0;  // "horizontal direction"
byte dy = 0;  // "vertical direction"
short score = 0;
bool move_chosen= false;

void setup() {

  // Turn on all pins except the ones used for by Bluetooth module
  for (byte i = 0; i <= 7; i++)
    pinMode(i, OUTPUT);
  for (byte i = 12; i <= 19; i++)
    pinMode(i, OUTPUT);

  BTSerial.begin(38400);
  game_begin();
}

void loop() {
  game_update();
  draw();
}

void game_begin() {
  init_snake();
  spawn_food();
}

void game_end() {
  clear_screen();
  display_score();
  while (true) draw();
}


void display_score() {
  short d1 = score / 10;
  short d2 = score % 10;
  switch(d1) {
    case 0: set_digit(0,zero); break;
    case 1: set_digit(0,one); break;
    case 2: set_digit(0,two); break;
    case 3: set_digit(0,three); break;
    case 4: set_digit(0,four); break;
    case 5: set_digit(0,five); break;
    case 6: set_digit(0,six); break;
    case 7: set_digit(0,seven); break;
    case 8: set_digit(0,eight); break;
    case 9: set_digit(0,nine); break;
  }

  switch(d2) {
    case 0: set_digit(1,zero); break;
    case 1: set_digit(1,one); break;
    case 2: set_digit(1,two); break;
    case 3: set_digit(1,three); break;
    case 4: set_digit(1,four); break;
    case 5: set_digit(1,five); break;
    case 6: set_digit(1,six); break;
    case 7: set_digit(1,seven); break;
    case 8: set_digit(1,eight); break;
    case 9: set_digit(1,nine); break;
  }
}

void set_digit(byte i, byte a[][8]) {
  for (short r = 0; r < 8; r++) {
    for (short c = 0; c < 4; c++) {
      matrix[r][c+(i*4)] = a[r][c];
    }
  }
}

void game_update() {

  if (dx == 0 && dy == 0)
    return;

  // Remove the tail from the matrix
  matrix[tail->py][tail->px] = 0;
  
  // Iterate through the snake from the tail until we reach head
  snake_block *tmp = tail;
  while (tmp->next != 0) {
     // Change position of each block to the position of the next block
    (tmp->px) = (tmp->next)->px;
    (tmp->py) = (tmp->next)->py;
    tmp = tmp->next;
  }
  // Move the head and update the matrix
  tmp->px = ((tmp->px) + dx)%8;
  tmp->py = ((tmp->py) + dy)%8;
  move_chosen= false;
  
  // Check for collision
  if (matrix[tmp->py][tmp->px] == 1) {
    // Check food
    if (tmp->px == food->px && tmp->py == food->py) {
      spawn_food();
      grow_snake();
      score++;
    } 
    else { // Colliding with snake
      if (tmp->px == original_tail->px && tmp->py == original_tail->py) { // Handle snake going into itself
        dx *= -1;
        dy *= -1;
        tmp->px = ((tmp->px) + (dx))%8;
        tmp->py = ((tmp->py) + (dy))%8;
        game_update();
      } else {
        game_end();
      }
    }
  }
  matrix[tmp->py][tmp->px] = 1;
  
}

void poll_input() {
  if (move_chosen || !BTSerial.available())
    return;
  int i = BTSerial.read();
    switch(i) {
      case '1': 
        if (dx == 0) {
          dx = -1;
          dy = 0;
          move_chosen= true;
        } break;
       
     case '5': 
       if (dy == 0) {
        dy = -1;
        dx = 0;
        move_chosen= true;
       } break;
               
     case '3': 
       if (dx == 0) {
         dx = 1;
         dy = 0;
         move_chosen= true;
       } break;
             
     case '7': 
       if (dy == 0) {
         dy = 1;
         dx = 0;
         move_chosen= true;
       } break;
  }
}

void draw() {
  for (int i = 0; i < LPD; i++) {
    poll_input();
    for(int r = 0; r < 8; r++) {
      // Set LED Matrix columns according to the bits in that row
      for(int c = 0; c < 8; c++)
        digitalWrite(col[c], matrix[r][c] == 0 ? 1 : 0);
      // Turn row on and off
      digitalWrite(row[r], HIGH);
      digitalWrite(row[r], LOW);
    }
  }
}

void clear_screen(){
  for(int r = 0; r < 8; r++)
    for(int c = 0; c < 8; c++)
      matrix[c][r] = 0;
}


