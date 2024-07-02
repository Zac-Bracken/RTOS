#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <Windows.h>
#include <time.h>
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include "timers.h"
#include "queue.h"
#include "WM392.h"
#include "semphr.h"


TaskHandle_t elecHandler = NULL;
TaskHandle_t gasHandler = NULL;
TaskHandle_t usageHandler = NULL;
TaskHandle_t displayHandler = NULL;
TaskHandle_t keyHandler = NULL;

SemaphoreHandle_t electricMutex;
SemaphoreHandle_t userPressMutex;
SemaphoreHandle_t gasMutex;
SemaphoreHandle_t clockMutex;
// Global Vars

// defaults
double elec_lower = 0.01;
double elec_upper = 0.04;
double gas_lower = 0.02;
double gas_upper = 0.05;
double gasAccum;
double elecAccum;
// display vars
char warn_level_display[255];
char display_gas_usage[255];
char display_electric_usage[255];
char display_gas_price[255];
char display_electric_price[255];
char display_default_gas_price[255];
char display_default_electric_price[255];
char display_current_gas_usage[255];
char display_current_electricity_usage[255];
char display_electric_warning_level[255];
char display_gas_warning_level[255];
double default_gas_price = 0.5;
double default_electric_price = 0.2;
double total_gas_cost;
double total_electric_cost;
// defaults
double low_electric_warning_level = 0.2;
double medium_electric_warning_level = 0.5;
double high_electric_warning_level = 1;
double default_gas_warning_level = 1;
double low_gas_warning_level = 0.2;
double medium_gas_warning_level = 0.5;
double high_gas_warning_level = 1;
BOOL electric_mode = FALSE;
BOOL gas_and_electric_mode = FALSE;
BOOL gas_mode = FALSE;
BOOL price_mode = FALSE;
BOOL is_electric_warning_mode = FALSE;
BOOL is_gas_warning_mode = FALSE;
// 0 = gas mode and electric mode, 1 = gas mode, 2 = electricity mode
int current_mode = 0; 
int gas_warning_mode;
int electric_warning_mode;
int display_mode;
int warn_mode;
double current_electricity_used;
double current_gas_used;


struct QMessage { struct tm sensorValue; char taskName[20]; };
TimerHandle_t xTimer1;
QueueHandle_t xQueue;

// Digits for LED clock 

char digits[11][5][4] = {
{
		// 0
	{'1','1','1','1'},
	{'1','0','0','1'},
	{'1','0','0','1'},
	{'1','0','0','1'},
	{'1','1','1','1'}
},
{
	// 1
	{'0','0','1','0'},
	{'0','0','1','0'},
	{'0','0','1','0'},
	{'0','0','1','0'},
	{'0','0','1','0'}
},
{
	// 2
	{'1','1','1','1'},
	{'0','0','0','1'},
	{'1','1','1','1'},
	{'1','0','0','0'},
	{'1','1','1','1'}
},
{
	// 3
	{'1','1','1','1'},
	{'0','0','0','1'},
	{'0','1','1','1'},
	{'0','0','0','1'},
	{'1','1','1','1'}
},
{
	// 4
	{'1','0','0','0'},
	{'1','0','0','1'},
	{'1','1','1','1'},
	{'0','0','1','0'},
	{'0','0','1','0'}
},
{
	// 5
	{'1','1','1','1'},
	{'1','0','0','0'},
	{'1','1','1','1'},
	{'0','0','0','1'},
	{'1','1','1','1'}
},
{
	// 6
	{'1','0','0','0'},
	{'1','0','0','0'},
	{'1','1','1','1'},
	{'1','0','0','1'},
	{'1','1','1','1'}
},
{
	// 7
	{'1','1','1','1'},
	{'1','0','0','1'},
	{'0','0','0','1'},
	{'0','0','0','1'},
	{'0','0','0','1'}

},
{
	// 8
	{'1','1','1','1'},
	{'1','0','0','1'},
	{'1','1','1','1'},
	{'1','0','0','1'},
	{'1','1','1','1'}
},
{
	// 9
	{'1','1','1','1'},
	{'1','0','0','1'},
	{'1','1','1','1'},
	{'0','0','0','1'},
	{'0','0','0','1'}
}, {
	// :
	{'0','0','1','0'},
	{'0','0','0','0'},
	{'0','0','0','0'},
	{'0','0','0','0'},
	{'0','0','1','0'}
	},

};


void generateTimeTask(TimerHandle_t xTimer) {

	/* Provides functionality for getting the current time to send to the clock display*/

	// queue
	struct QMessage aMessage;
	
	// for storing timeinfo
	time_t current_time;
	struct tm time_info;
	time(&current_time);
	// get current local time
	localtime_s(&time_info, &current_time);
	char* timerName = pcTimerGetName(xTimer);
	strcpy_s(aMessage.taskName, sizeof(aMessage.taskName), timerName);
	aMessage.sensorValue = time_info;
	// send time through queue
	xQueueSend(xQueue, (void*)&aMessage, (TickType_t)0);

}



void displayNumbers(void) {

	/* Provides the functionality for displaying the clock numbers */

	while (1) {
		// local vars
		int i, j, k, xInc, yInc;
		struct QMessage aMessage;
		// i < the number + 1 for iteration
		int cnt = 0;
		xInc = 0;
		yInc = 0;
		int digit = 0;
		int counter = 0;
		int thisx = 10;
		int thisy = 40;
		int xdefault = 10;
		int hourDigit1;
		int hourDigit2;
		int minuteDigit1;
		int minuteDigit2;
		int secondDigit1;
		int secondDigit2;
		char time_string[20];

		if (xQueue != 0) {
			// receive time through queue
			if (xQueueReceive(xQueue, (void*)&aMessage, (TickType_t)1)) {
				strftime(time_string, sizeof(time_string), "%H:%M:%S'", &aMessage.sensorValue);
				// extract the correct numbers for each clock digit
				hourDigit1 = ((time_string[0]));
				hourDigit1 = hourDigit1 - '0';
				hourDigit2 = (time_string[1]);
				hourDigit2 = hourDigit2 - '0';
				minuteDigit1 = (time_string[3]);
				minuteDigit1 = minuteDigit1 - '0';
				minuteDigit2 = (time_string[4]);
				minuteDigit2 = minuteDigit2 - '0';
				secondDigit1 = (time_string[6]);
				secondDigit1 = secondDigit1 - '0';
				secondDigit2 = (time_string[7]);
				secondDigit2 = secondDigit2 - '0';
				// for managing the loop
				boolean showing_clock = TRUE;
				while (showing_clock) {
					for (i = 0; i < 1; i++) {
						for (j = 0; j < 5; j++) {
							for (k = 0; k < 4; k++) {
								switch (counter) {
								// go through each digit and set its screen position
								case 0:
									digit = hourDigit1;
									xdefault = 10;
									thisy = 6;
									yInc = 0;
									break;
								case 20:
									digit = hourDigit2;
									thisx = 16;
									yInc = 0;
									thisy = 5;
									xdefault = 16;
									break;
								case 40:
									digit = minuteDigit1;
									thisx = 25;
									yInc = 0;
									thisy = 5;
									xdefault = 25;
									break;
								case 60:
									digit = minuteDigit2;
									thisx = 31;
									yInc = 0;
									thisy = 5;
									xdefault = 31;
									break;
								case 80:
									digit = secondDigit1;
									thisx = 40;
									yInc = 0;
									thisy = 5;
									xdefault = 40;
									break;
								case 100:
									digit = secondDigit2;
									thisx = 46;
									yInc = 0;
									thisy = 5;
									xdefault = 46;
									break;
								case 120:
									digit = 10;
									thisx = 20;
									thisy = 5;
									yInc = 0;
									xdefault = 20;
									break;
								case 140:
									digit = 10;
									thisx = 35;
									thisy = 5;
									yInc = 0;
									xdefault = 35;
									showing_clock = FALSE;
									counter = 0;
									break;
								}

								switch (xInc) {
								case 4:
									thisx = xdefault;
									yInc++;
									xInc = 0;
									break;
								}
								// prints a * when a 1 is present
								if (digits[digit][j][k] == '1') {
									//printf("*");
									printXY(thisx + xInc, thisy + yInc, "*");
								}
								// print a " " when a 0 is present
								if (digits[digit][j][k] == '0') {
									printXY(thisx + xInc, thisy + yInc, " ");
								}
								xInc++;
								counter++;
							};

						};


					}
				}
			}
		}

	}
}

void handleUserKey(void) {
	/* Functionality for handling user keypresses */

	while (1) {
		// get user input
		char user_character = _getch();
		system("cls");
		if (xSemaphoreTake(userPressMutex, (TickType_t)0xFFFFFFFF) == 1) {
			// for toggling between screen views both -> gas -> electric -> both
			if (user_character == 'a' || user_character == 'A') {
				// log button press
				printXY(1,0, "a pressed");
				display_mode = (display_mode + 1) % 3;  
				electric_warning_mode = (electric_warning_mode + 1) % 3; 
			}
			// for toggling between the low, medium and high warning modes
			if (user_character == 'm' || user_character == 'M') {
				// log button press
				printXY(1,0,"m pressed");
				gas_warning_mode = (gas_warning_mode + 1) % 3;
				electric_warning_mode = (electric_warning_mode + 1) % 3;
			}
			// for entering price mode
			else if (user_character == 'p' || user_character == 'P') {
				// log button press
				printXY(1,0, "p pressed");
				price_mode = TRUE;
				is_electric_warning_mode = FALSE;
				is_gas_warning_mode = FALSE;
			}
			// for increasing price or warning levels
			else if (user_character == '+') {
				// log button press
				printXY(1,0, "+ pressed");
				if (display_mode == 1) {
					if (price_mode) {
						default_gas_price += 0.1;
					}
					else if (is_gas_warning_mode) {
						switch (gas_warning_mode) {
						case 0:
							// low warning mode
							low_gas_warning_level += 0.1;
							break;
						case 1:
							// medium warning mode
							medium_gas_warning_level += 0.5;
						case 2:
							// high warning mode
							high_gas_warning_level += 1;
						default:
							low_gas_warning_level += 0.01;
							break;
						}
					}
				}
				if (display_mode == 2) {
					if (price_mode) {
						default_electric_price += 0.1;
					}
					else if (is_electric_warning_mode) {
						switch (electric_warning_mode) {
						case 0:
							// low warning mode
							low_electric_warning_level += 0.1;
							break;
						case 1:
							// medium warning mode
							medium_electric_warning_level += 0.5;
						case 2:
							// high warning mode
							high_electric_warning_level += 1;
						default:
							low_electric_warning_level += 0.01;
							break;
						}
					}
				}
			}
			// for reducing price or warning levels
			else if (user_character == '-') {
				// log button press
				printXY(1,0,"- pressed");
				if (display_mode == 1) {
					if (price_mode) {
						default_gas_price -= 0.1;
					}
					// for gas warning mode
					else if (is_gas_warning_mode) {
						switch (gas_warning_mode) {
						case 0:
							// low warning mode
							low_gas_warning_level -= 0.01;
							break;
						case 1:
							// medium warning mode
							medium_gas_warning_level -= 0.5;
							break;
						case 2:
							// large warning mode
							high_gas_warning_level -= 1;
							break;
						default:
							low_gas_warning_level -= 0.01;
							break;
						}
					}
				}
				if (display_mode == 2) {
					if (price_mode) {
						default_electric_price -= 0.1;
					}
					// for electric warning mode
					else if (is_electric_warning_mode) {
						switch (electric_warning_mode) {
						case 0:
							// low warning mode
							low_electric_warning_level -= 0.1;
							break;
						case 1:
							// medium warning mode
							medium_electric_warning_level -= 0.5;
							break;
						case 2:
							// high warning mode
							high_electric_warning_level -= 1;
							break;
						default:
							low_electric_warning_level -= 0.01;
							break;

						}
					}
				}

			}
			// for warning mode
			else if (user_character == 'w' || user_character == 'W') {
			// log button press
				printf("w pressed");
				is_electric_warning_mode = TRUE;
				is_gas_warning_mode = TRUE;
				price_mode = FALSE;
			}
			// to exit any modes using esc esc(asci=27)
			else if (user_character == 27) {
				printf("esc pressed");
				price_mode = FALSE;
				is_gas_warning_mode = FALSE;
				is_electric_warning_mode = FALSE;
			}
			xSemaphoreGive(userPressMutex);
		}
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}


void usageDisplayTask(void) {

	/* Functionality for displaying the core information of SME */

	while (1) {
		// Show default displays
		printXY(10, 2, "+----------------------------------------+\n");
		printXY(10, 3, "|              Welcome User!             |\n");
		printXY(10, 4, "+----------------------------------------+\n");

		printXY(60, 2, "+----------------------------------------+\n");
		printXY(60, 3, "|               Warning levels           |\n");
		printXY(60, 4, "+----------------------------------------+\n");

		// Take the semaphore to read the gas and electricity usage 
		if ((xSemaphoreTake(gasMutex, (TickType_t)0xFFFFFFFF) == 1) && (xSemaphoreTake(electricMutex, (TickType_t)0xFFFFFFFF) == 1) && (xSemaphoreTake(userPressMutex, (TickType_t)0xFFFFFFFF) == 1))
		{
			if (display_mode == 1 || display_mode == 0) {
				sprintf_s(display_gas_usage, 255, "Accumulated Gas: %.2f kw", gasAccum);
				sprintf_s(display_current_gas_usage, 255, "Gas used per second: %.2f kw", current_gas_used);
				sprintf_s(display_gas_price, 255, "Total Gas Spend: \x9C %.2f", total_gas_cost);
				sprintf_s(display_default_gas_price, 255, "Default Gas Price: \x9C %.2f", default_gas_price);
				switch (gas_warning_mode) {
				case 0:
					// low warning mode
					sprintf_s(display_gas_warning_level, 255, "LOW Gas Warning Level: \x9C %.2f", low_gas_warning_level);
					break;
				case 1:
					// medium warning mode
					sprintf_s(display_gas_warning_level, 255, "MEDIUM Gas Warning Level: \x9C %.2f", medium_gas_warning_level);
					break;
				case 2:
					// high warning mode
					sprintf_s(display_gas_warning_level, 255, "HIGH Gas Warning Level: \x9C %.2f", high_gas_warning_level);
					break;
				default:
					sprintf_s(display_gas_warning_level, 255, "LOW Gas Warning Level: \x9C %.2f", low_gas_warning_level);	
				}
				// Display Gas Details
				printXY(11, 17, display_current_gas_usage);
				printXY(11, 18, display_gas_usage);
				printXY(11, 19, display_gas_price);
				printXY(60, 16, display_default_gas_price);
				printXY(60, 6, display_gas_warning_level);
				printXY(10, 12, "+----------------------------------------+\n");
				printXY(10, 13, "|                   Gas                  |\n");
				printXY(10, 14, "+----------------------------------------+\n");
				printXY(60, 12, "+----------------------------------------+\n");
				printXY(60, 13, "|              Default Gas Price         |\n");
				printXY(60, 14, "+----------------------------------------+\n");
				// For the different gas warning levels
				if (total_gas_cost >= high_gas_warning_level) {
					printXY(11, 16, "\033[31mATTENTION\033[0m Warning Level!        ");
				}
				else if (total_gas_cost >= medium_gas_warning_level) {
					printXY(11, 16, "\033[38;5;202mATTENTION\033[0m Warning Level!     ");
				}
				else if (total_gas_cost >= low_gas_warning_level) {
					printXY(11, 16, "\033[33mATTENTION\033[0m Warning Level!         ");
				}
				else {
					printXY(11, 16, "\033[32mBelow Price Warning Level\033[0m       ");
				}
			}


			if (display_mode == 2 || display_mode == 0) {
				// default electric position on screen
				int i = 25;
				int j = 26;
				int k = 27;
				int l = 24;
				int m = 20;
				int n = 21;
				int o = 22;
				int p = 20;
				int q = 21;
				int r = 22;
				int s = 24;
				if (display_mode == 2 && display_mode != 0) {
					// shift electric position when gas is not on screen
					i = 17;
					j = 18;
					k = 19;
					l = 16;
					m = 12;
					n = 13;
					o = 14;
					p = 12;
					q = 13;
					r = 14;
					s = 16;
				}
				// Prepare Electricity Details 
				sprintf_s(display_electric_usage, 255, "Accumulated Electricity: %.2f kw", elecAccum);
				sprintf_s(display_electric_price, 255, "Total Electric Spend: \x9C %.2f", total_electric_cost);
				sprintf_s(display_current_electricity_usage, 255, "Electric used per second: %.2f kw", current_electricity_used);
				sprintf_s(display_default_electric_price, 255, "Default Electric Price: \x9C %.2f", default_electric_price);
				switch (electric_warning_mode) {
				case 0:
					// low warning mode
					sprintf_s(display_electric_warning_level, 255, "LOW Electric Warning Level: \x9C %.2f", low_electric_warning_level);
					break;
				case 1:
					// medium warning mode
					sprintf_s(display_electric_warning_level, 255, "MEDIUM Electric Warning Level: \x9C %.2f", medium_electric_warning_level);
					break;
				case 2:
					// high warning mode
					sprintf_s(display_electric_warning_level, 255, "HIGH Electric Warning Level: \x9C %.2f", high_electric_warning_level);
					break;
				default:
					sprintf_s(display_electric_warning_level, 255, "LOW Electric Warning Level: \x9C %.2f", low_electric_warning_level);

				}
				// Display Electricity Detail
				printXY(11, i, display_current_electricity_usage);
				printXY(11, j, display_electric_usage);
				printXY(11, k, display_electric_price);
				printXY(60, l, display_default_electric_price);
				printXY(60, 7, display_electric_warning_level);
				printXY(10, m, "+----------------------------------------+\n");
				printXY(10, n, "|               Electricity              |\n");
				printXY(10, o, "+----------------------------------------+\n");
				printXY(60, p, "+----------------------------------------+\n");
				printXY(60, q, "|          Default Electricity Price     |\n");
				printXY(60, r, "+----------------------------------------+\n");
				// For the different electric warning levels
				if (total_electric_cost > low_electric_warning_level && total_electric_cost < medium_electric_warning_level && total_electric_cost < high_electric_warning_level) {
					printXY(11, s, "\033[33mATTENTION\033[0m Warning Level!        ");

				}
				else if (total_electric_cost > medium_electric_warning_level && total_electric_cost < high_electric_warning_level) {
					printXY(11, s, "\033[38;5;202mATTENTION\033[0m Warning Level!       ");
				}
				else if (total_electric_cost > high_electric_warning_level) {
					printXY(11, s, "\033[31mATTENTION\033[0m Warning Level!       ");
				}
				else {
					printXY(11, s, "\033[32mBelow Price Warning Level\033[0m      ");
				}
			}
		}
		// release 
		xSemaphoreGive(gasMutex);
		xSemaphoreGive(electricMutex);
		xSemaphoreGive(userPressMutex);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}

void gasTask(void) {

	/*  provides functionality for generating the random values for the gas task */

	srand(3);
	while (1) {
		// Take the semaphore to write to gas accumulation
		if (xSemaphoreTake(gasMutex, (TickType_t)0xFFFFFFFF) == 1) {
			// Accumulate gas usage
			current_gas_used = ((double)rand() / RAND_MAX) * (gas_upper - gas_lower) + gas_lower;
			gasAccum += current_gas_used;
			total_gas_cost = gasAccum * default_gas_price;
			// Check if gas warning level has been reached
		
			default_gas_price = default_gas_price;
			// Give back the semaphore mutex to release the lock
			xSemaphoreGive(gasMutex);
			vTaskDelay(1000 / portTICK_PERIOD_MS);
		}

	}
}

void electricityTask(void) {

	/* Provides functionality for providing the random values for the electricity task */

	srand(2);
	while (1) {
		// Take the semaphore 
		if (xSemaphoreTake(electricMutex, (TickType_t)0xFFFFFFFF) == 1) {
			// for current per second
			current_electricity_used = ((double)rand() / RAND_MAX) * (elec_upper - elec_lower) + elec_lower;
			// Accumulate electricity usage and accumulation
			elecAccum += current_electricity_used;
			// accuulated cost
			total_electric_cost = elecAccum * default_electric_price;
			default_electric_price = default_electric_price;
			// give semaphore mutex to release the lock
			xSemaphoreGive(electricMutex);
			vTaskDelay(1000 / portTICK_PERIOD_MS);
			// Give back the semaphore to release the lock
		}

	}
}

void hideCursor() {

	/* Provides functionality for hiding the CMD cursor */

    // https://stackoverflow.com/questions/30126490/how-to-hide-console-cursor-in-c reference in bibiography 
	HANDLE cursor_console_handle = GetStdHandle(STD_OUTPUT_HANDLE);
	// cursor information
	CONSOLE_CURSOR_INFO cursor_info;
	// set size
	cursor_info.dwSize = 100;
	// set visibility
	cursor_info.bVisible = FALSE;
	SetConsoleCursorInfo(cursor_console_handle, &cursor_info);
}

int main(void) {

	/* Main Function */
	
	hideCursor();
	// queue for clock
	xQueue = xQueueCreate(10, sizeof(struct QMessage));
	// Create the tasks
	xTaskCreate(electricityTask, "Electricity", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, &elecHandler);
	xTaskCreate(gasTask, "Gas", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, &gasHandler);
	xTaskCreate(usageDisplayTask, "DisplayUsage", configMINIMAL_STACK_SIZE, NULL, 1, &usageHandler);
	xTaskCreate(displayNumbers, "DisplayNums", configMINIMAL_STACK_SIZE, NULL, 2, NULL, &displayHandler);
	xTaskCreate(handleUserKey, "handleKey", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, &keyHandler);
	// Create timer for clock
	xTimer1 = xTimerCreate("Timer_1", 1000 / portTICK_PERIOD_MS, pdTRUE, NULL, generateTimeTask);
	// locks
	electricMutex = xSemaphoreCreateMutex();
	gasMutex = xSemaphoreCreateMutex();
	userPressMutex = xSemaphoreCreateMutex();
	// timer for clock
	xTimerStart(xTimer1, 0);
	vTaskStartScheduler();
	// to ensure no memory leaks
	if (elecHandler != NULL && gasHandler != NULL && usageHandler != NULL && displayHandler != NULL && keyHandler != NULL) {
		vTaskDelete(elecHandler);
		vTaskDelete(gasHandler);
		vTaskDelete(usageHandler);
		vTaskDelete(displayHandler);
		vTaskDelete(keyHandler);
	}

}


