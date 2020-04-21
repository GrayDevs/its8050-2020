/**
 * Test file
 * 
 * %userprofile%\esp\esp-idf\export.bat
 * idf.py set-target esp32
 * idf.py menuconfig
 * or modifying sdk.cfg
 * %userprofile%\esp\esp-contactless-access\idf.py build
 * %userprofile%\esp\esp-contactless-access\idf.py -p COM4 flash
 * %userprofile%\esp\esp-contactless-access\idf.py -p COM4 monitor
*/

// Libs
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// #include "contactless_main.h" // unfortunately useless because of the delay function used everywhere in the program

//Function Signatures
int dummy_hall_sensor_read(int i, int test_set[]);
void dummy_calibrate(int without_m, int with_m);

void update_button_state(bool current_bs);

// Global Vars
int without_magnet = 100;
int with_magnet = 0;
bool access_system_state;
bool current_button_state;
bool previous_button_state;
bool previous_button_state_2;

// int test_set[10]
int test_set_0[] = {100, 100, 100, 100, 100, 100, 100, 100, 100, 100};      /* Nothing happens */
int test_set_1[] = {100, 100, 100, 40, -70, -94, -95, -105, -100, 100};     /* Button pressed  */
int test_set_2[] = {100, -100, 80, -100, 100, 100, 100, 100, 100, 100};     /* Access open and closed normally */
int test_set_3[] = {100, 100, -100, 100, 100, 100, 100, 100, 100, 100};     /* Access open and reset for inactivity */
int test_set_4[] = {100, 100, 100, 100, 100, 100, 100, 100, 100, 100};      /* Extra: close everything if let the card long enough */

// Constant
#define PRECISION 5 /* defines the interval where the button will be considered to be pressed */
#define MAX_INACTIVITY_DELAY 5 /* delay of inactivity to close the system (in s) */
#define TEST_SET test_set_2


int dummy_hall_sensor_read(int i, int test_set[]){
    return test_set[i];
}


void dummy_calibrate(int without_m, int with_m){
    without_magnet = without_m;
    with_magnet = with_m;
}

void update_button_state(bool current_bs){
    previous_button_state_2 = previous_button_state;
    previous_button_state = current_button_state;
    current_button_state = current_bs;
}

int main(int argc, const char *argv[]){

    printf("[i] Sensor Calibration...\n");
    dummy_calibrate(100, -100);
    printf("[+] Calibration sucessfull:\n");
    printf("\t> Standard Value    = %d;\n", without_magnet);
    printf("\t> Value with Magnet =%d;\n\n", with_magnet);


    printf("[i] Starting main programm...\n");
    int current_hall = without_magnet;
    int timer_inactivity = 0;
    access_system_state = false;
    current_button_state = false;
    previous_button_state = false;
    previous_button_state_2 = false;

    // emulate the task
    for(int i=0; i < 10; i++){
        printf("%ds: ", i);
         
        current_hall = dummy_hall_sensor_read(i, TEST_SET);
        printf("ESP32 Hall average = %d mT\n", current_hall);
        
        /* button is pressed, magnet is close to the hall sensor */
        if(current_hall <= (with_magnet + PRECISION)){
            update_button_state(true);
            timer_inactivity = 0;
            printf("++ Button state: Pressed\n");

            // system is closed
            if(!access_system_state){
                access_system_state = true;
                printf("++ System state: Open\n");
                printf("[i] A user openned the system.\n");
            }
            // system is open
            else if(previous_button_state_2 && !previous_button_state){
                access_system_state = false;
                printf("-- System state: Closed\n");
                printf("[i] A user closed the system.\n");
            } 
            else {
                printf("++ System state: Open\n");
            }
        /* button is not pressed, magnets is far from the hall sensor */
        } 
        else {
            update_button_state(false);
            printf("-- Button state: Not Pressed\n");

            /* button is not pressed, but system is open */
            if(access_system_state){
                // close the system after a certain amount of time (a number of loop, 1 round ~ 1s)
                if(timer_inactivity == MAX_INACTIVITY_DELAY){
                    access_system_state = false;
                    timer_inactivity = 0;
                    printf("-- System state: Closed\n");
                    printf("[!] System automatically closed due to inactivity\n");
                } 
                else {
                    printf("++ System state: Open\n");
                    timer_inactivity++;
                }
            }
            else {
                printf("-- System state: Closed\n");
            }
        } 

        // update_button_state(false); // forced button reset
    }

    return 0;
}