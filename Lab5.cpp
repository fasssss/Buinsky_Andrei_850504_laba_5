#include <dos.h>
#include <stdio.h>
#include <iostream.h>
#include <conio.h>
#include <io.h>

struct VIDEO
{
	unsigned char symb;
	unsigned char attr;
};
VIDEO  far* screen = (VIDEO far*)MK_FP(0xB800, 0);
int msCounter = 0;

int flag = 0;
int flagMin = 0;
int flagHour = 0;
int flagDay = 0;
int flagNoPause = 1;
int flagStopSW = 0;
static int h = 0;
static int m = 0;
int s;
int oldValue = 100;

void interrupt far(*oldInt70h)(...);		// OLD RTC handler
void interrupt far(*oldInt4Ah)(...);		// old alarm handler
void interrupt far newInt4Ahandler(...);	// new rtc handler
void interrupt far newInt70handler(...);	// new alarm handler

void freeze(void);
void unfreeze(void);
int BCDToInteger(int bcd);
unsigned char IntToBCD(int value);
void getCurrentTime(void);
void setTimeManually(void);


void delay(void);
void wait(void);
void enableAlarm(void);
void disableAlarm(void);
void stopwatch(void);
void stopwatchHandler(void);
void printAlarm(void);

void menu() {
	printf("\n1. Show time");
	printf("\n2. Set time");
	printf("\n3. Delay time");
	printf("\n4. Enable alarm");
	printf("\n5. Disable alarm");
	printf("\n6. Refresh screen");
	printf("\n0. Exit\n\n");
}

void main() {
	char c;
	clrscr();
	menu();
	while (c != 0) {
		c = getch();
		switch (c) {
		case '1': getCurrentTime(); break;
		case '2': setTimeManually(); break;
		case '3': delay(); break;
		case '4': enableAlarm(); break;
		case '5': disableAlarm(); break;
		case '6': clrscr(); menu(); break;
		case '0': return;
		}
	}
}

void wait(void) {
	do {
		outp(0x70, 0x0A);               // Time setting
	} while (inp(0x71) & 0x80);			// 0x80 = 10000000 while 7-th bit - 1, rtc is busy
}

void getValues() {
	int value;
	wait();
	outp(0x70, 0x00);					// Register 0 - current Second
	value = inp(0x71);                  // Capture of current time
	s = BCDToInteger(value);
	m = 0;
	h = 0;
}

void getCurrentTime(void) {
	int value;

	wait();
	outp(0x70, 0x04);					// Register 4 - current Hour
	value = inp(0x71);					// Read Value
	printf("%d:", BCDToInteger(value));

	wait();
	outp(0x70, 0x02);					// Register 2 - current Minute
	value = inp(0x71);
	printf("%d:", BCDToInteger(value));

	wait();
	outp(0x70, 0x00);					// Register 0 - current Second
	value = inp(0x71);
	printf("%d   ", BCDToInteger(value));

	wait();
	outp(0x70, 0x07);					// Register 7 - current Day
	value = inp(0x71);
	printf("%d.", BCDToInteger(value));

	wait();
	outp(0x70, 0x08);					// Register 8 - current Month
	value = inp(0x71);
	printf("%d.", BCDToInteger(value));

	wait();
	outp(0x70, 0x09);					// year
	value = inp(0x71);
	printf("%d   ", BCDToInteger(value));

	wait();
	outp(0x70, 0x06);					// weekday
	value = inp(0x71);
	switch (BCDToInteger(value)) {
	case 2: printf("Monday\n"); break;
	case 3: printf("Tuesday\n"); break;
	case 4: printf("Wednesday\n"); break;
	case 5: printf("Thursday\n"); break;
	case 6: printf("Friday\n"); break;
	case 7: printf("Saturday\n"); break;
	case 1: printf("Sunday\n"); break;           // 1 - is Sunday
	default: printf("Day of week is not set\n"); break;
	}
}


void setTimeManually(void) {
	int value;
	freeze();							// disable rtc update

	do {
		printf("Enter hour: ");
		fflush(stdin);
		scanf("%d", &value);
	} while (value > 23 || value < 0);
	outp(0x70, 0x04);					// Register of current hour
	outp(0x71, IntToBCD(value));        // Write value

	do {
		printf("Enter minute: ");
		fflush(stdin);
		scanf("%d", &value);
	} while (value > 59 || value < 0);
	outp(0x70, 0x02);					// Register of current minute
	outp(0x71, IntToBCD(value));		

	do {
		printf("Enter second: ");
		fflush(stdin);
		scanf("%d", &value);
	} while (value > 59 || value < 0);
	outp(0x70, 0x00);					// Register of current second
	outp(0x71, IntToBCD(value));		

	do {
		printf("Enter day week number: ");
		fflush(stdin);
		scanf("%d", &value);
	} while (value > 7 || value < 1);
	outp(0x70, 0x06);					// Register of current day of week
	outp(0x71, IntToBCD(value));		

	do {
		printf("Enter number day of month: ");
		fflush(stdin);
		scanf("%d", &value);
	} while (value > 31 || value < 1);
	outp(0x70, 0x07);					// Register of current day of month
	outp(0x71, IntToBCD(value));

	do {
		printf("Enter mounth: ");
		fflush(stdin);
		scanf("%d", &value);
	} while (value > 12 || value < 1);
	outp(0x70, 0x08);					// Register of current month
	outp(0x71, IntToBCD(value));

	do {
		printf("Enter year: ");
		fflush(stdin);
		scanf("%d", &value);
	} while (value > 99 || value < 0);
	outp(0x70, 0x09);					// Register of current year(last two numbers)
	outp(0x71, IntToBCD(value));

	unfreeze();							// enable rtc update
}

void enableAlarm(void) {
	int value, second, minute, hour;

	do {								// input time
		printf("Enter second: ");
		fflush(stdin);
		scanf("%d", &second);
	} while (second > 59 || second < 0);

	do {
		printf("Enter minute: ");
		fflush(stdin);
		scanf("%d", &minute);
	} while (minute > 59 || minute < 0);

	do {
		printf("Enter hour: ");
		fflush(stdin);
		scanf("%d", &hour);
	} while (hour > 23 || hour < 0);

	outp(0x70, 0x01);					// Register 1 - alarm second
	outp(0x71, IntToBCD(second));

	outp(0x70, 0x03);					// Register 3 - alarm minute
	outp(0x71, IntToBCD(minute));

	outp(0x70, 0x05);					// Register 5 - alarm hour
	outp(0x71, IntToBCD(hour));

	_disable();
	oldInt4Ah = getvect(0x4A);
	setvect(0x4A, newInt4Ahandler);     // Set new handler of interrupt 
	_enable();

	outp(0x70, 0xB);
	outp(0x71, inp(0x71) | 0x20);


	outp(0xA1, inp(0xA1) & 0xFE);

	// 0xFE = 11111110, ICW1
}

void disableAlarm() {
	if (oldInt4Ah == NULL) {
		printf("Alarm is already disable or not set.\n");
		return;
	}
	_disable();
	setvect(0x4A, oldInt4Ah);
	oldInt4Ah = NULL;
	outp(0xA1, (inp(0xA1) | 0x01));
	outp(0x70, 0x0B);
	outp(0x71, inp(0x71) & 0xDF);		// 0xDF = 11011111, bit 5 to 0 to disable alarm (Alarm Interrupt disable)
	_enable();

}

void freeze(void) {						// Rtc update disabling
	unsigned char value;
	wait();								// until rtc are busy
	outp(0x70, 0x0B);
	value = inp(0x71);					// get B register
	value |= 0x80;						// change bit 7 to 1 to disable rtc update (disables the counter)
	outp(0x71, value);					// write new B register value
}

void unfreeze(void) {					// rtc update enabling
	unsigned char value;
	wait();								// until rtc are busy
	outp(0x70, 0x0B);
	value = inp(0x71);					// get B register
	value -= 0x80;						// change bit 7 to 0 to allow rtc update (enable the counter)
	outp(0x70, 0x0B);
	outp(0x71, value);					// write new B register value
}

void interrupt far newInt70handler(...) { // Set new interrupt
	msCounter++;
	oldInt70h();
}

void interrupt far newInt4Ahandler(...) { // The prototype of the alarm interrupt handler program

	printAlarm();
	oldInt4Ah();
}

void printAlarm() {

	screen->symb = 'W';
	screen->attr = 0x5E;
	screen++;

	screen->symb = 'A';
	screen->attr = 0x5E;
	screen++;

	screen->symb = 'K';
	screen->attr = 0x5E;
	screen++;

	screen->symb = 'E';
	screen->attr = 0x5E;
	screen++;

	screen->symb = ' ';
	screen->attr = 0x5E;
	screen++;

	screen->symb = 'U';
	screen->attr = 0x5E;
	screen++;

	screen->symb = 'P';
	screen->attr = 0x5E;
	screen++;

	screen->symb = '!';
	screen->attr = 0x5E;
	screen++;
}

void delay(void) {		
	unsigned long delayPeriod;
	unsigned char value;

	do {                              // Input delay in milliseconds
		printf("Enter delay time in milliseconds: ");
		fflush(stdin);
		scanf("%ld", &delayPeriod);
	} while (delayPeriod < 1);
	printf("Time before delay: ");
	getCurrentTime();					// Current Time before delay
	printf("Delaying ...\n");

	_disable();							//restrict interrupts
	oldInt70h = getvect(0x70);
	setvect(0x70, newInt70handler);
	_enable();							// allow interrupts

	// enable IRQ8 CMOS 
	outp(0xA1, inp(0xA1) & 0xFE);		// 0xFE = 11111110(ICW1)

	// enable periodic interrupts
	outp(0x70, 0x0B);
	outp(0x71, inp(0x71) | 0x40);		// 0x40 = 01000000, set bit 6 to 1 to enable (Enable interruptions)

	msCounter = 0;
	while (msCounter != delayPeriod);	// ms counting
	printf("End delay of %d ms\n", msCounter);
	_disable();
	setvect(0x70, oldInt70h);			// get previous handler
	_enable();
	printf("Time after delay: ");
	getCurrentTime();                   // Current time after delay
}

int BCDToInteger(int bcd) {
	return bcd % 16 + bcd / 16 * 10;
}

unsigned char IntToBCD(int value) {
	return (unsigned char)((value / 10) << 4) | (value % 10);
}
