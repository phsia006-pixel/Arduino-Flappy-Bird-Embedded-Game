/*        Your Name & E-mail: Peter Hsia phsia006

*          Discussion Section: 21

 *         Assignment: Custom Project

 *         Exercise Description: [optional - include for your own benefit]

 *        

 *         I acknowledge all content contained herein, excluding template or example code, is my own original work.

 *          https://youtube.com/shorts/nzXXmZ_9BvY 


 */ 
#include "timerISR.h"
#include "helper.h"
#include "periph.h"
#include "phsia006_LCD.h"
#include "phsia006_spiAVR.h"
#include "phsia006_Sprite.h"



#define NUM_TASKS 7 //TODO: Change to the number of tasks being used
int vrx = 0;
int vry = 0;
unsigned char left  = 0;
unsigned char right = 0;
unsigned char up    = 0;
unsigned char down  = 0;
unsigned char pipeX = 0;
unsigned char pipeGapY = 40;     
unsigned char pipeSpawnTimer = 0; 
unsigned char pipeActive = 0;
unsigned char game_on = 0;
unsigned char birdY = 50;
unsigned char score = 0;
unsigned char prev_pipeActive = 0;
unsigned char game_over = 0;


#define PIPE_SPEED 2     
#define PIPE_SPAWN_Rate 20  
#define BIRD_X 30
#define SW   ((PINC & (1 << PC1)) == 0)
#define PIPE_GAP_MIN 10
#define PIPE_GAP_MAX (SCREEN_H - GAP_HEIGHT - 10)
#define BIRD_GRAVITY 1     
#define BIRD_FLAP    3 

//Task struct for concurrent synchSMs implmentations
typedef struct _task{
	signed 	 char state; 		//Task's current state
	unsigned long period; 		//Task period
	unsigned long elapsedTime; 	//Time elapsed since last task tick
	int (*TickFct)(int); 		//Task tick function
} task;


//TODO: Define Periods for each task
// e.g. const unsined long TASK1_PERIOD = <PERIOD>
const unsigned long TASK0_PERIOD = 50;
const unsigned long TASK1_PERIOD = 5;
const unsigned long TASK2_PERIOD = 50;
const unsigned long TASK3_PERIOD = 50;
const unsigned long TASK4_PERIOD = 50;
const unsigned long TASK5_PERIOD = 50;  
const unsigned long TASK7_PERIOD = 50;

const unsigned long GCD_PERIOD = 1;//TODO:Set the GCD Period

task tasks[NUM_TASKS]; // declared task array with 5 tasks


void TimerISR() {
	for ( unsigned int i = 0; i < NUM_TASKS; i++ ) {                   // Iterate through each task in the task array
		if ( tasks[i].elapsedTime == tasks[i].period ) {           // Check if the task is ready to tick
			tasks[i].state = tasks[i].TickFct(tasks[i].state); // Tick and set the next state for this task
			tasks[i].elapsedTime = 0;                          // Reset the elapsed time for the next tick
		}
		tasks[i].elapsedTime += GCD_PERIOD;                        // Increment the elapsed time by GCD_PERIOD
	}
}

void ResetGame() {
    birdY = 50;            

    pipeX = 0;
    pipeActive = 0;
    pipeSpawnTimer = 0;
    pipeGapY = 40;   
    
    score = 0;
    prev_pipeActive = 0;
    game_over = 0;

    TFT_FillScreen(SKY_R, SKY_G, SKY_B);
}

enum Joy_State { Joy_Start, Wait_State };

int Tick_Joy(int state){
    vrx = ADC_read(2);  
    vry = ADC_read(3);   

    switch(state){
        case Joy_Start:
            state = Wait_State;
            break;

        case Wait_State:
            state = Wait_State;
            break;

        default:
            state = Joy_Start;
            break;
    }

    switch(state){
        case Wait_State:

            left = right = up = down = 0;

            if(vrx < 400)
                left = 1;
            else if(vrx > 600)
                right = 1;

            if(vry < 400)
                down = 1;
            else if(vry > 600)
                up = 1;


                
            break;

        case Joy_Start:
        default:
            break;
    }

    return state;
}

enum StartGame_State { StartGame_Wait };

int Tick_StartGame(int state) {
    static unsigned char sw_prev = 0;
    unsigned char sw_now = SW;

    switch (state) {
        case StartGame_Wait:
        default:
            state = StartGame_Wait;
            break;
    }

    switch (state) {
        case StartGame_Wait:
            if (game_on == 0) {
                if (sw_now && !sw_prev) {
                    ResetGame();
                    game_on = 1;
                }
            }
            break;

        default:
            break;
    }

    sw_prev = sw_now;
    return state;
}






enum LCD16_State {LCD16_Start, LCD16_Display};

int Tickfct_LCD16(int state) {
    static unsigned char lcd_mode = 0;

    switch (state) {
        case LCD16_Start:
            lcd_create_char(0, smiley);

            if (lcd_mode == 0) {
                lcd_clear();
                lcd_goto_xy(0, 2);
                lcd_write_str("Flappy Birds");

                lcd_goto_xy(1, 1);
                lcd_write_str("Press to Start");
                lcd_write_character(0);  
            }

            state = LCD16_Display;
            break;

        case LCD16_Display:
            state = LCD16_Display;
            break;

        default:
            state = LCD16_Start;
            break;
    }

        switch (state) {
        case LCD16_Display:
            if (lcd_mode == 0 && game_on == 1) {
                lcd_clear();
                lcd_goto_xy(0, 0);
                lcd_write_str("Score: ");
                lcd_mode = 1;
            }
            else if (lcd_mode == 1 && game_over == 1) {
                lcd_clear();

                unsigned char tens = score / 10;
                unsigned char ones = score % 10;
                char buf[3];
                buf[0] = '0' + tens;
                buf[1] = '0' + ones;
                buf[2] = '\0';

                lcd_goto_xy(0, 0);
                lcd_write_str("Game Over Sc:");
                lcd_write_str(buf);

                lcd_goto_xy(1, 0);
                lcd_write_str("Press to Restart");

                lcd_mode = 2;
            }
            else if (lcd_mode == 2 && game_on == 1) {
                lcd_clear();
                lcd_goto_xy(0, 0);
                lcd_write_str("Score: ");
                lcd_mode = 1;
            }
            break;

        case LCD16_Start:
        default:
            break;
    }

    return state;
}




enum Pipes_State { Pipes_Start, Pipes_Create, Pipes_Move };

int Tick_Pipes(int state) {

    if(!game_on){
        return state;
    }
    switch (state) {
        case Pipes_Start:
            pipeX = 0;
            pipeGapY = 40;
            pipeSpawnTimer = 0;
            pipeActive = 0;
            state = Pipes_Create;
            break;

        case Pipes_Create:
            pipeSpawnTimer++;

            if (pipeSpawnTimer >= PIPE_SPAWN_Rate) {
                pipeSpawnTimer = 0;

                pipeX = SCREEN_W - PIPE_WIDTH;  

                {
                    unsigned char rVal = rand8();
                    unsigned char range = (unsigned char)(PIPE_GAP_MAX - PIPE_GAP_MIN + 1);
                    pipeGapY = PIPE_GAP_MIN + (rVal % range);
                }

                pipeActive = 1;
                state = Pipes_Move;
            } else {
                state = Pipes_Create;
            }
            break;

        case Pipes_Move:
            if (pipeActive && pipeX > 0) {
                state = Pipes_Move;
            } else {
                pipeActive = 0;
                state = Pipes_Create;
            }
            break;

        default:
            state = Pipes_Start;
            break;
    }

    switch (state) {
        case Pipes_Create:
            break;

        case Pipes_Move:
            if (pipeActive) {
                unsigned char oldX0 = pipeX;
                unsigned char oldX1 = pipeX + PIPE_WIDTH - 1;
                if (oldX1 >= SCREEN_W) {
                    oldX1 = SCREEN_W - 1;
                }
                DrawRect(oldX0, 0,
                               oldX1, SCREEN_H - 1,
                               SKY_R, SKY_G, SKY_B);

                if (pipeX > PIPE_SPEED) {
                    pipeX -= PIPE_SPEED;

                    DrawPipe(pipeX, pipeGapY);
                } else {
                    pipeX = 0;
                    pipeActive = 0;
                }

            }
            break;

        case Pipes_Start:
        default:
            break;
    }

    return state;
}

enum Bird_State { Bird_Start, Bird_Idle, Bird_Play };

int Tick_Bird(int state) {

    switch (state) {
        case Bird_Start:
            birdY = 50;        
            state = Bird_Idle;
            break;

        case Bird_Idle:
            if (game_on) {
                state = Bird_Play;
            } else {
                state = Bird_Idle;
            }
            break;

        case Bird_Play:
            if (!game_on) {
                state = Bird_Idle;
            } else {
                state = Bird_Play;
            }
            break;

        default:
            state = Bird_Start;
            break;
    }

    switch (state) {
        case Bird_Idle:
            DrawBird(BIRD_X, birdY);
            break;

        case Bird_Play:
            if (birdY < (SCREEN_H - BIRD_H)) {
                birdY += BIRD_GRAVITY;
            }

            if (up) {
                if (birdY > BIRD_FLAP) {
                    birdY -= BIRD_FLAP;
                } else {
                    birdY = 0;
                }
            }

            if (birdY > (SCREEN_H - BIRD_H)) {
                birdY = SCREEN_H - BIRD_H;
            }

            if (birdY > 0 && birdY < BIRD_FLAP && up) {
                birdY = 0;
            }

            DrawBird(BIRD_X, birdY);
            break;

        case Bird_Start:
        default:
            break;
    }

    return state;
}

enum Touch_State { Touch_Check };

int Tick_Touch(int state) {

    switch (state) {
        case Touch_Check:
            state = Touch_Check;
            break;

        default:
            state = Touch_Check;
            break;
    }

    switch(state){

        case Touch_Check:
    unsigned char birdTop    = birdY;
    unsigned char birdBottom = birdY + BIRD_H - 1;
    unsigned char birdLeft   = BIRD_X;
    unsigned char birdRight  = BIRD_X + BIRD_W - 1;

    unsigned char collided = 0;

    if (birdTop == 0) {
        collided = 1;
    }

    if (birdBottom >= (SCREEN_H - 1)) {
        collided = 1;
    }

    if (!collided && game_on && pipeActive) {
        unsigned char pipeLeft  = pipeX;
        unsigned char pipeRight = pipeX + PIPE_WIDTH - 1;
        if (pipeRight >= SCREEN_W) {
            pipeRight = SCREEN_W - 1;
        }

        unsigned char horizOverlap =
            (birdRight >= pipeLeft) && (birdLeft <= pipeRight);

        if (horizOverlap) {
            unsigned char gapTop    = pipeGapY;
            unsigned char gapBottom = pipeGapY + GAP_HEIGHT;
            if (gapBottom > SCREEN_H) {
                gapBottom = SCREEN_H;
            }

            unsigned char fullyInGap =
                (birdTop   >= gapTop) &&
                (birdBottom <  gapBottom);

            if (!fullyInGap) {
                collided = 1;
            }
        }
    }

    if (collided) {
        game_on = 0;   
        game_over = 1;
    }

    break;


    }
    return state;
}

enum Score_State { Score_Check };

int Tick_Score(int state) {

    switch (state) {
        case Score_Check:
        default:
            state = Score_Check;
            break;
    }switch(state){
    case Score_Check:
        if (!game_on) {
        prev_pipeActive = pipeActive;   
        return state;
        }

 

    if (prev_pipeActive == 1 && pipeActive == 0) {
        score++;
    }

    unsigned char tens = score / 10;
    unsigned char ones = score % 10;
    char buf[3];
    buf[0] = '0' + tens;
    buf[1] = '0' + ones;
    buf[2] = '\0';

    lcd_goto_xy(0, 7);      
    lcd_write_str(buf);

    prev_pipeActive = pipeActive;
    break;
}
    return state;
}




//TODO: Create your tick functions for each task





int main(void) {
    //TODO: initialize all your inputs and ouputs

    DDRC = 0x00; 
    PORTC = (1 << PC1);


    ADC_init();   // initializes ADC

    lcd_init();
    lcd_clear();

    ST7735_init();
    TFT_FillScreen(235, 206, 135);


    unsigned char i = 0;

    //TODO: Initialize tasks here
    // e.g. 
    // tasks[0].period = ;
    // tasks[0].state = ;
    // tasks[0].elapsedTime = ;
    // tasks[0].TickFct = ;

    tasks[i].period = TASK0_PERIOD;
    tasks[i].state = StartGame_Wait;
    tasks[i].elapsedTime = TASK0_PERIOD;
    tasks[i].TickFct = Tick_StartGame;

    i++; 

    tasks[i].period = TASK1_PERIOD;
    tasks[i].state = Joy_Start;
    tasks[i].elapsedTime = TASK1_PERIOD;
    tasks[i].TickFct = Tick_Joy;

    i++; 

    tasks[i].period = TASK2_PERIOD;
    tasks[i].state = LCD16_Start;
    tasks[i].elapsedTime = TASK2_PERIOD;
    tasks[i].TickFct = Tickfct_LCD16;

    i++;

    tasks[i].period = TASK3_PERIOD;
    tasks[i].state = Pipes_Start;
    tasks[i].elapsedTime = TASK3_PERIOD;
    tasks[i].TickFct = Tick_Pipes;

    i++;

    tasks[i].period = TASK4_PERIOD;
    tasks[i].state = Bird_Start;
    tasks[i].elapsedTime = TASK4_PERIOD;
    tasks[i].TickFct = Tick_Bird;

    i++;

    tasks[i].period = TASK5_PERIOD;
    tasks[i].state = Touch_Check;
    tasks[i].elapsedTime = TASK5_PERIOD;
    tasks[i].TickFct = Tick_Touch;

    i++;

    tasks[i].period = TASK7_PERIOD;
    tasks[i].state = Score_Check;
    tasks[i].elapsedTime = TASK7_PERIOD;
    tasks[i].TickFct = Tick_Score;

    TimerSet(GCD_PERIOD);
    TimerOn();

    while (1) {}

    return 0;
}