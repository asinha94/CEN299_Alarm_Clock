//This program uses the RTC to display time on a 4 digit 7 segment display
//When the alarm triggers, it plays mp3 files through a USB connected on the
//micro USB port

#include "stm32f4xx_rtc.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_pwr.h"
#include "stm32f4xx_dac.h"
#include "stm32f4xx_tim.h"
#include "misc.h"
#include "stm32f4xx_exti.h"
#include "audioMP3.h"
#include "main.h"

//Pin States
#define HIGH 1
#define LOW 0

//Boolean Logic
#define TRUE 1
#define FALSE 0

//Segment de-selectors
#define A_Bar GPIO_Pin_7
#define B_Bar GPIO_Pin_8
#define C_Bar GPIO_Pin_9
#define D_Bar GPIO_Pin_10
#define E_Bar GPIO_Pin_11
#define F_Bar GPIO_Pin_12
#define G_Bar GPIO_Pin_13

// Types
#define BUTTON int

typedef enum D_STATES { CLOCK_TIME = 0, ALARM_TIME } D_STATE;
typedef enum T_STATES { T12_TIME = 0, T24_TIME } T_STATE;
typedef enum S_STATES {SET_TIME = 0, SHOW_TIME} S_STATE;

//structures
RCC_ClocksTypeDef RCC_Clocks;
GPIO_InitTypeDef	GPIOInitStruct;
TIM_TimeBaseInitTypeDef TIM_InitStruct;
NVIC_InitTypeDef NVIC_InitStructure;
EXTI_InitTypeDef EXTI_InitStructure;


//function prototypes
void configuration(void);

void SetDigit(int, int, int);

void SetSegment(int);

void Clock_Time_Increment(uint8_t, uint8_t);

void Alarm_Time_Increment(uint8_t, uint8_t);

//global variables
int interruptOccurred = 0;
int interupt_flag = 0;

extern volatile int exitMp3 = 0;
extern volatile int mp3PlayingFlag = 0;
extern volatile int snoozeMemory = 0;

/*for testing
uint32_t *ptr;
uint32_t *ptr2;*/

int main(void)
{

  configuration();

  while ( 1 )
  {
	//	mp3PlayingFlag = 1;
	//	audioToMp3();

	  if (interupt_flag)
	  {
		  interruptOccurred = 0;
		  interupt_flag = 0;
		  mp3PlayingFlag = 1;
		  audioToMp3();
	  }

  }
}

//timer interrupt handler that is called at a rate of 500Hz
//this function gets the time and displays it on the 7 segment display
//it also checks for button presses, debounces, and handles each case
void TIM5_IRQHandler(void)
{
	int previousState = 0;

	//double checks that interrupt has occurred
	if( TIM_GetITStatus( TIM5, TIM_IT_Update ) != RESET )
	{
	     //clears interrupt flag
	     TIM5->SR = (uint16_t)~TIM_IT_Update;

    }
/*
 * CURRENT_STATE defines whether the the clock will display Clock time or Alarm time (Literal values ar 0 or 1)
 * TIME_STATE defines whether display time is 24 hours or 12 hour time
 * SET_STATE defines whether the clock will display the time, or will be in the mode to set the time
 * */
	static D_STATE CURRENT_STATE = CLOCK_TIME;
	static T_STATE TIME_STATE = T24_TIME;
	static S_STATE SET_STATE = SHOW_TIME;

	//These variables will tell me if the buttons are currently being held down or not (True for yes)
	static int Button1AlreadyPressed = FALSE;
	static int Button2AlreadyPressed = FALSE;
	static int Button3AlreadyPressed = FALSE;
	static int Button4AlreadyPressed = FALSE;
	static int Button5AlreadyPressed = FALSE;
																	// Show     ||  Set
	BUTTON Button_1 = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_4);		// Snooze   ||  HourIncrement    4
	BUTTON Button_2 = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_5);		// Turn Off ||	MinuteIncrement  5
	BUTTON Button_3 = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_8);		// Set		||	Show			 8
	BUTTON Button_4 = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_9);		// 24Hr - 12Hr					 9
	BUTTON Button_5 = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_11);	// Alarm- Clock Time     		 11

	//defines whether the digits are flashing or not. Either all digits are flashing or none
	static int Digit_flash = FALSE;


/*Check if button has been pushed. First 'if' is for button debouncing, if the Buttonalreadypushed is true,
 * none of the button functionality will be run
 * */
	if (Button_1 == LOW)
	{
		Button1AlreadyPressed = FALSE;
	}

	if (Button_1 == HIGH)
	{
		if (!Button1AlreadyPressed)
		{
			Button1AlreadyPressed = TRUE;

			if (mp3PlayingFlag)
			{
				exitMp3 = 1;

				Alarm_Time_Increment(0x00, 0x01);
			}

			if (SET_STATE == SET_TIME)
			{
				if (CURRENT_STATE == CLOCK_TIME)
				{
					Clock_Time_Increment(0x01, 0x00);
				}

				else
				{
					Alarm_Time_Increment(0x01, 0x00);
				}
			}
		}
	}

	if (Button_2 == LOW)
	{
		Button2AlreadyPressed = FALSE;
	}

	if (Button_2 == HIGH)
	{
		if (!Button2AlreadyPressed)
		{
			Button2AlreadyPressed = TRUE;

			if (mp3PlayingFlag)
			{
				exitMp3 = 1;
			}

			if (SET_STATE == SET_TIME)
			{
				if (CURRENT_STATE == CLOCK_TIME)
				{
					Clock_Time_Increment(0x00, 0x01);
				}

				else
				{
					Alarm_Time_Increment(0x00, 0x01);
				}
			}
		}
	}

	if (Button_3 == LOW)
	{
		Button3AlreadyPressed = FALSE;
	}

	if (Button_3 == HIGH)
	{
		if (!Button3AlreadyPressed)
		{
			Button3AlreadyPressed = TRUE;
			Digit_flash = !Digit_flash;
			SET_STATE = !SET_STATE;
		}
	}

	if (Button_4 == LOW)
	{
		Button4AlreadyPressed = FALSE;
	}

	if (Button_4 == HIGH)
		{
			if (!Button4AlreadyPressed)
			{
				Button4AlreadyPressed = TRUE;

			 	TIME_STATE = !TIME_STATE;
			}
		}

	if (Button_5 == LOW)
	{
		Button5AlreadyPressed = FALSE;
	}

	if (Button_5 == HIGH)
	{
		if (!Button5AlreadyPressed)
		{
			Button5AlreadyPressed = TRUE;

			CURRENT_STATE = !CURRENT_STATE;
		}
	}

	int Hours, Minutes;

	if (CURRENT_STATE == CLOCK_TIME)
	{
		RTC_GetTime(RTC_Format_BIN, &myclockTimeStruct);

		Hours = (int) myclockTimeStruct.RTC_Hours;
		Minutes = (int) myclockTimeStruct.RTC_Minutes;
	}
	else
	{
		RTC_GetAlarm(RTC_Format_BIN, RTC_Alarm_A, &AlarmStruct);

		Hours = (int) AlarmStruct.RTC_AlarmTime.RTC_Hours;
		Minutes = (int) AlarmStruct.RTC_AlarmTime.RTC_Minutes;
 	}

	int FIVE_OR_SIX = 5;

	if (TIME_STATE == T12_TIME && Hours >= 12)
		{
			Hours -= 12;
			FIVE_OR_SIX = 6;
		}

	int H1 = Hours / 10;
	int H2 = Hours % 10;
	int M1 = Minutes / 10;
	int M2 = Minutes % 10;

	static int time_counter = 0;
	static int digit_counter = 1;

	if (time_counter++ < 1)
	{
		switch (digit_counter)
		{
		case 1: SetDigit(1, H1, Digit_flash);
				break;
		case 2: SetDigit(2, H2, Digit_flash);
				break;
		case 3:	SetDigit(3, M1, Digit_flash);
				break;
		case 4:	SetDigit(4, M2, Digit_flash);
				break;
		case 5: SetDigit(5, 10, Digit_flash);
				break;
		case 6: SetDigit(5, 11, FALSE);
				break;
		default: SetDigit(0, 12, FALSE); //if an error occurs, turn off whole display
				break;

		}
	}
	else {
		time_counter = 0;
		digit_counter++;
		if (digit_counter > FIVE_OR_SIX)
			digit_counter = 1;
	}

}

//Function will set the given numeric value onto the given digit of the LED display
void SetDigit(int Digit, int Value, int Blink)
{
	GPIO_ResetBits(GPIOD, GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11);
	static int DIGIT_BLINK_RATE = 0;


	switch (Digit)
	{
	case 1:	if (Blink)
			{
				if (DIGIT_BLINK_RATE++ < 125)
				{
					GPIO_SetBits(GPIOD, GPIO_Pin_8);
				}

				if (DIGIT_BLINK_RATE >= 250)
				{
					DIGIT_BLINK_RATE = 0;
				}
			}
			else
			{
				GPIO_SetBits(GPIOD, GPIO_Pin_8);
			}

			break;
	case 2: if (Blink)
			{
				if (DIGIT_BLINK_RATE++ < 125)
				{
					GPIO_SetBits(GPIOD, GPIO_Pin_9);
				}

				if (DIGIT_BLINK_RATE >= 250)
				{
					DIGIT_BLINK_RATE = 0;
				}
			}
			else
			{
				GPIO_SetBits(GPIOD, GPIO_Pin_9);
			}
			break;
	case 3: if (Blink)
			{
				if (DIGIT_BLINK_RATE++ < 125)
				{
					GPIO_SetBits(GPIOD, GPIO_Pin_10);
				}

				if (DIGIT_BLINK_RATE >= 250)
				{
					DIGIT_BLINK_RATE = 0;
				}
			}
			else
			{
				GPIO_SetBits(GPIOD, GPIO_Pin_10);
			}
			break;
	case 4: if (Blink)
			{
				if (DIGIT_BLINK_RATE++ < 125)
				{
					GPIO_SetBits(GPIOD, GPIO_Pin_11);
				}

				if (DIGIT_BLINK_RATE >= 250)
				{
					DIGIT_BLINK_RATE = 0;
				}
			}
			else
			{
				GPIO_SetBits(GPIOD, GPIO_Pin_11);
			}
			break;
	case 5: if (Blink)
			{
				if (DIGIT_BLINK_RATE++ < 125)
				{
					GPIO_SetBits(GPIOD, GPIO_Pin_7);
				}

				if (DIGIT_BLINK_RATE >= 250)
				{
					DIGIT_BLINK_RATE = 0;
				}
			}
			else
			{
				GPIO_SetBits(GPIOD, GPIO_Pin_7);
			}
			break;
	default:break;
	}


	SetSegment(Value);
}
/*Sends signal to display segments of given number
 *      A
 *    _____
 *   |     |
 * F |     | B
 *   |__G__|
 *   |	   |
 * E |     | C
 *   |_____|
 *      D
 *
 *Segments are all first de-selected, then the segments wanted are selected (by turning off voltage)
 *
 * */
void SetSegment(int Value)
{
	GPIO_SetBits(GPIOE, A_Bar | B_Bar | C_Bar | D_Bar | E_Bar | F_Bar | G_Bar); // turn off everything
	switch (Value) //turns on selected segments
	{
	case 0:	GPIO_ResetBits(GPIOE, A_Bar | B_Bar | C_Bar | D_Bar | E_Bar | F_Bar);
			break;
	case 1:	GPIO_ResetBits(GPIOE, B_Bar | C_Bar);
			break;
	case 2:	GPIO_ResetBits(GPIOE, A_Bar | B_Bar | D_Bar | E_Bar | G_Bar);
			break;
	case 3:	GPIO_ResetBits(GPIOE, A_Bar | B_Bar | C_Bar | D_Bar | G_Bar);
			break;
	case 4:	GPIO_ResetBits(GPIOE, B_Bar | C_Bar | F_Bar | G_Bar);
			break;
	case 5:	GPIO_ResetBits(GPIOE, A_Bar | C_Bar | D_Bar | F_Bar | G_Bar);
			break;
	case 6:	GPIO_ResetBits(GPIOE, A_Bar | C_Bar | D_Bar | E_Bar | F_Bar | G_Bar);
			break;
	case 7:	GPIO_ResetBits(GPIOE, A_Bar | B_Bar | C_Bar);
			break;
	case 8:	GPIO_ResetBits(GPIOE, A_Bar | B_Bar | C_Bar | D_Bar | E_Bar | F_Bar | G_Bar);
			break;
	case 9:	GPIO_ResetBits(GPIOE, A_Bar | B_Bar | C_Bar | F_Bar | G_Bar);
			break;
	case 10:GPIO_ResetBits(GPIOE, A_Bar | B_Bar);
			break;
	case 11:GPIO_ResetBits(GPIOE, C_Bar);
			break;
	default:break;

	}
}

void Clock_Time_Increment(uint8_t Hours, uint8_t Minutes)
{
	RTC_GetTime(RTC_Format_BCD, &myclockTimeStruct);

	myclockTimeStruct.RTC_Hours += Hours;
	myclockTimeStruct.RTC_Minutes += Minutes;


	if ( myclockTimeStruct.RTC_Minutes >= 0x3c ) //is the minutes less than 60 in (in hex)
	{
		myclockTimeStruct.RTC_Minutes = 0x00;
		myclockTimeStruct.RTC_Hours += 0x01;
	}

	if ( myclockTimeStruct.RTC_Hours >= 0x18 ) //is the hour less than 24 (in hex)
	{
		myclockTimeStruct.RTC_Hours = 0x00;
	}

	RTC_SetTime(RTC_Format_BCD, &myclockTimeStruct);
}

void Alarm_Time_Increment(uint8_t Hours, uint8_t Minutes)
{
	RTC_AlarmCmd(RTC_Alarm_A,DISABLE);
	RTC_GetAlarm(RTC_Format_BCD, RTC_Alarm_A, &AlarmStruct);

	AlarmStruct.RTC_AlarmTime.RTC_Hours += Hours;
	AlarmStruct.RTC_AlarmTime.RTC_Minutes += Minutes;


	if (AlarmStruct.RTC_AlarmTime.RTC_Minutes >= 0x60)
	{
		AlarmStruct.RTC_AlarmTime.RTC_Minutes = 0x00;
		AlarmStruct.RTC_AlarmTime.RTC_Hours += 0x01;
	}

	if (AlarmStruct.RTC_AlarmTime.RTC_Hours >= 0x24)
	{
		AlarmStruct.RTC_AlarmTime.RTC_Hours = 0x00;
	}

	RTC_SetAlarm(RTC_Format_BCD,RTC_Alarm_A,&AlarmStruct);
	RTC_AlarmCmd(RTC_Alarm_A,ENABLE);
}

//alarm A interrupt handler
//when alarm occurs, clear all the interrupt bits and flags
//then set the flag to play mp3
void RTC_Alarm_IRQHandler(void)
{

	//resets alarm flags and sets flag to play mp3
	  if(RTC_GetITStatus(RTC_IT_ALRA) != RESET)
	  {
    	RTC_ClearFlag(RTC_FLAG_ALRAF);
	    RTC_ClearITPendingBit(RTC_IT_ALRA);
	    EXTI_ClearITPendingBit(EXTI_Line17);
		interruptOccurred = 1;
	  }
	  interupt_flag = 1;
}


//configures the clocks, gpio, alarm, interrupts etc.
void configuration(void)
{
	  //lets the system clocks be viewed
	  RCC_GetClocksFreq(&RCC_Clocks);

	  //enable peripheral clocks
	  RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
	  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);
	  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
	  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
	  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE);

	  //enable the RTC
	  PWR_BackupAccessCmd(DISABLE);
	  PWR_BackupAccessCmd(ENABLE);
	  RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);
	  RCC_RTCCLKCmd(ENABLE);
	  RTC_AlarmCmd(RTC_Alarm_A,DISABLE);

	  //Enable the LSI OSC
	  RCC_LSICmd(ENABLE);

	  //Wait till LSI is ready
	  while(RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET);

	  //enable the external interrupt for the RTC to use the Alarm
	  /* EXTI configuration */
	  EXTI_ClearITPendingBit(EXTI_Line17);
	  EXTI_InitStructure.EXTI_Line = EXTI_Line17;
	  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
	  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	  EXTI_Init(&EXTI_InitStructure);

	  //set timer 5 to interrupt at a rate of 500Hz
	  TIM_TimeBaseStructInit(&TIM_InitStruct);
	  TIM_InitStruct.TIM_Period	=  8000;	// 500Hz
	  TIM_InitStruct.TIM_Prescaler = 20;
	  TIM_TimeBaseInit(TIM5, &TIM_InitStruct);

	  // Enable the TIM5 global Interrupt
	  NVIC_Init( &NVIC_InitStructure );
	  NVIC_InitStructure.NVIC_IRQChannel = TIM5_IRQn;
	  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
      NVIC_Init( &NVIC_InitStructure );

      //setup the RTC for 24 hour format
	  myclockInitTypeStruct.RTC_HourFormat = RTC_HourFormat_24; //change to 12
	  myclockInitTypeStruct.RTC_AsynchPrediv = 127;
	  myclockInitTypeStruct.RTC_SynchPrediv = 0x00FF;
	  RTC_Init(&myclockInitTypeStruct);

	  //set the time displayed on power up to 12PM
	  myclockTimeStruct.RTC_H12 = RTC_H12_PM;
	  myclockTimeStruct.RTC_Hours = 0x11;
	  myclockTimeStruct.RTC_Minutes = 0x59;
	  myclockTimeStruct.RTC_Seconds = 0x00;
	  RTC_SetTime(RTC_Format_BCD, &myclockTimeStruct);


	  //sets alarmA for 12:00AM, date doesn't matter
	  AlarmStruct.RTC_AlarmTime.RTC_H12 = RTC_H12_PM; //change to AM
	  AlarmStruct.RTC_AlarmTime.RTC_Hours = 0x11;
	  AlarmStruct.RTC_AlarmTime.RTC_Minutes = 0x59;
	  AlarmStruct.RTC_AlarmTime.RTC_Seconds = 0x15;
	  AlarmStruct.RTC_AlarmMask = RTC_AlarmMask_DateWeekDay;
	  RTC_SetAlarm(RTC_Format_BCD,RTC_Alarm_A,&AlarmStruct);

	  // Enable the Alarm global Interrupt
	  NVIC_Init( &NVIC_InitStructure );
	  NVIC_InitStructure.NVIC_IRQChannel = RTC_Alarm_IRQn;
	  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	  NVIC_Init( &NVIC_InitStructure );

	  //IO for push buttons using internal pull-up resistors
	  GPIO_StructInit( &GPIOInitStruct );
	  GPIOInitStruct.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_11;
	  GPIOInitStruct.GPIO_Speed = GPIO_Speed_2MHz;
	  GPIOInitStruct.GPIO_Mode = GPIO_Mode_IN;
	  GPIOInitStruct.GPIO_OType = GPIO_OType_PP;
	  GPIOInitStruct.GPIO_PuPd = GPIO_PuPd_UP;
	  GPIO_Init(GPIOC, &GPIOInitStruct);

	  //configure GPIO for digits
	  GPIO_StructInit( &GPIOInitStruct );
	  GPIOInitStruct.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	  GPIOInitStruct.GPIO_Speed = GPIO_Speed_2MHz;
	  GPIOInitStruct.GPIO_Mode = GPIO_Mode_OUT;
	  GPIOInitStruct.GPIO_OType = GPIO_OType_PP;
	  GPIOInitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
	  GPIO_Init(GPIOE, &GPIOInitStruct);

	  //configure GPIO for multiplexing
	  GPIO_StructInit( &GPIOInitStruct );
	  GPIOInitStruct.GPIO_Pin = GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11;
	  GPIOInitStruct.GPIO_Speed = GPIO_Speed_2MHz;
	  GPIOInitStruct.GPIO_Mode = GPIO_Mode_OUT;
	  GPIOInitStruct.GPIO_OType = GPIO_OType_PP;
	  GPIOInitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
	  GPIO_Init(GPIOD, &GPIOInitStruct);

	  //enables RTC alarm A interrupt
	  RTC_ITConfig(RTC_IT_ALRA, ENABLE);

	  //enables timer interrupt
	  TIM5->DIER |= TIM_IT_Update;

	  //enables timer
	  TIM5->CR1 |= TIM_CR1_CEN;

	  RTC_AlarmCmd(RTC_Alarm_A,ENABLE);
}
