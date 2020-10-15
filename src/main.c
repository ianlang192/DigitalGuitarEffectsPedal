
#include "main.h"

int main(void)
{
	Initialize_Peripherals();	// Call initialization functions for each peripheral
	// Erase Flash memory
	FLASH_Erase_Blocks_IT(LOOPER_STARTING_FLASH_ADDRESS, LOOPER_MAX_FLASH_ADDRESS);
	LED_Blink_Heartbeat(HEARTBEAT_LED_PERIOD_MS);	// Set up heartbeat LED
	Audio_Enable();

	uint8_t systemState = SYSTEM_STATE_DEFAULT;		// Initialize the state machine

	//Main program loop, calls specific functions for different system states
	while (1)
	{
		switch(systemState)
		{
		case SYSTEM_STATE_DEFAULT:
			systemState = Run_State_Default();
			break;
		case SYSTEM_STATE_ADJUST:
			systemState = Run_State_Adjust();
			break;
		case SYSTEM_STATE_TUNER:
			systemState = Run_State_Tuner();
			break;
		case SYSTEM_STATE_LOOPER:
			systemState = Run_State_Looper();
			break;
		case SYSTEM_STATE_TEMPO:
			systemState = Run_State_Tempo();
			break;
		}
	}
}

/*
 * Initializes all peripherals required by the Digital Guitar Effects Pedal
 */
void Initialize_Peripherals(void)
{
	HAL_Init(); 						// Reset of all peripherals
	SystemClock_Config();				// Configure the system clock
	Audio_Init(TIMER_CLK_FREQ_MHZ);		// Initialize audio processing functions
	Delay_Init(TIMER_CLK_FREQ_MHZ);		// Initialize internal delay functions
	SPI1_Init();						// Initialize SPI1 for external flash memory
	SPI2_Init();						// Initialize SPI2 for ADC and DAC communication
	ADC_Init(TIMER_CLK_FREQ_MHZ);		// Initialize the ADC
	DAC_Init();							// Initialize the DAC
	FLASH_Init(TIMER_CLK_FREQ_MHZ);		// Initialize the external flash memory
	LEDS_Init(TIMER_CLK_FREQ_MHZ);		// Initialize indication LEDs
	Encoder_Init(TIMER_CLK_FREQ_MHZ);	// Initialize encoder
	Buttons_Init(TIMER_CLK_FREQ_MHZ);	// Initialize buttons
	Display_Init(TIMER_CLK_FREQ_MHZ);	// Initialize display
}

uint8_t Run_State_Default(void)
{
	static uint8_t initialized = 0;				// Store if state has been initialized
	uint8_t nextSystemState = SYSTEM_STATE_DEFAULT;

	// Initialize the display if state has not been previously initialized
	if(initialized == 0)
	{
		Display_Clear();	// Clear the entire display

		// Line 1
		Display_Write_Number(Effect_Get_Current_Number(), 2, 0x01,
								DISPLAY_ALIGN_RIGHT);
		Display_Write_Char('.', 0x02);
		Display_Write_String(Effect_Get_Current_Name(), 0x04);

		// Line 2
		Display_Write_String(Effect_Param_Get_Previous_Name(), 0x41);
		// Only draw up arrow if previous parameter exists
		if(strlen(Effect_Param_Get_Previous_Name()) > 0)
			Display_Write_Char(0x01, 0x53);

		// Line 3
		Display_Write_Char(0x7E, 0x14);	// Draw right pointing arrow
		Display_Write_String(Effect_Param_Get_Current_Name(),  0x15);
		Display_Write_Number(Effect_Param_Get_Current_Value(), 3, 0x27,
								DISPLAY_ALIGN_RIGHT);

		// Line 4
		Display_Write_String(Effect_Param_Get_Next_Name(), 0x55);
		// Only draw arrow if next parameter exists
		if(strlen(Effect_Param_Get_Next_Name()) > 0)
			Display_Write(0x00, 1, 0x67);

		// Turn on the status LED if the current effect is active
		if(Effect_Get_Current_Activated_Status())
			LED_Status(1);
		else
			LED_Status(0);

		initialized = 1;	// Signify that state has been initialized
	}
	// Check if user has interacted with front panel, and process the button press
	// or encoder rotation
	if(Front_Panel_Event())
	{
		uint16_t buttonStatus = Front_Panel_Button_Status();	// Get status of buttons
		switch(buttonStatus)
		{
		case BUTTON_STATUS_MENU_UP_PRESS:
			Effect_Param_Previous(); 				// Go to previous parameter
			initialized = 0; 						// Reinitialize screen
			break;
		case BUTTON_STATUS_MENU_OK_PRESS:
			nextSystemState = SYSTEM_STATE_ADJUST;	// Change to adjust state
			break;
		case BUTTON_STATUS_MENU_DOWN_PRESS:
			Effect_Param_Next(); 					// Go to next parameter
			initialized = 0;						// Reinitialize screen
			break;
		case BUTTON_STATUS_STOMP_DOWN_PRESS:
			Effect_Next();							// Change to next effect
			initialized = 0;						// Reinitialize screen
			break;
		case BUTTON_STATUS_STOMP_DOWN_HOLD:
			nextSystemState = SYSTEM_STATE_TUNER;	// Change to tuner state
			break;
		case BUTTON_STATUS_STOMP_UP_PRESS:
			Effect_Previous();
			initialized = 0;						// Reinitialize screen
			break;
		case BUTTON_STATUS_STOMP_UP_HOLD:
			nextSystemState = SYSTEM_STATE_LOOPER;	// Change to looper state
			break;
		case BUTTON_STATUS_STOMP_EFFECT_PRESS:
			if(Effect_Get_Current_Activated_Status())	// Toggle effect activation
			{
				Effect_Deactivate_Current();
				LED_Status(0);						// Turn off LED if effect off
			}
			else
			{
				Effect_Activate_Current();
				LED_Status(1);						// Turn on LED if effect on
			}
			break;
		case BUTTON_STATUS_STOMP_EFFECT_HOLD:
			nextSystemState = SYSTEM_STATE_TEMPO;	// Switch to tempo state
			break;
		default:
			// Change effect depending on direction of encoder rotation
			if(Front_Panel_Encoder_Rotation() == ENCODER_COUNTERCLOCKWISE)
			{
				Effect_Previous();
				initialized = 0;					// Reinitialize screen
			}
			else if(Front_Panel_Encoder_Rotation() == ENCODER_CLOCKWISE)
			{
				Effect_Next();
				initialized = 0;					// Reinitialize screen
			}
			Front_Panel_Reset_Encoder_Rotation();
			break;
		}
		Clear_Front_Panel_Event();					// Signify button press registered
		Reset_Front_Panel_Button_Status();			// Reset button status
	}
	if (nextSystemState != SYSTEM_STATE_DEFAULT)	// Detect state transitions
	{
		initialized = 0;	// Reinitialize next time state is entered
	}
	return nextSystemState;
}

uint8_t Run_State_Adjust(void)
{
	static uint8_t initialized = 0;			// Store if state has been initialized
	uint8_t nextSystemState = SYSTEM_STATE_ADJUST;

	// Initialize the display if state has not been previously initialized
	if(initialized == 0)
	{
		Display_Clear();	// Clear the entire display

		// Line 1
		Display_Write_Number(Effect_Get_Current_Number(), 2, 0x01, DISPLAY_ALIGN_RIGHT);
		Display_Write_Char('.', 0x02);
		Display_Write_String(Effect_Get_Current_Name(), 0x04);

		// Line 2
		Display_Write_Char(0x7E, 0x40); // Draw right arrow
		char* paramName = Effect_Param_Get_Current_Name();
		// Place name at center of screen
		Display_Write_String(paramName,  0x49 - strlen(paramName)/2);

		// Line 3
		Display_Write_Number(Effect_Param_Get_Current_Value(), 4, 0x1C,
								DISPLAY_ALIGN_LEFT);
		Display_Write_Number(Effect_Param_Get_Current_Min_Value(), 2, 0x14,
								DISPLAY_ALIGN_LEFT);
		Display_Write_Number(Effect_Param_Get_Current_Max_Value(), 4, 0x27,
								DISPLAY_ALIGN_RIGHT);

		// Line 4
		// Draw bar graph of parameter value
		Display_Write_Bar_Graph(0x54, 20, Effect_Param_Get_Current_Value(),
				Effect_Param_Get_Current_Min_Value(),
				Effect_Param_Get_Current_Max_Value());

		// Set status LED if current effect is activated
		if(Effect_Get_Current_Activated_Status())
			LED_Status(1);
		else
			LED_Status(0);

		initialized = 1;			// Signify that state has been initialized
	}
	// Check if user has interacted with front panel, and process the correct button press
	// or encoder rotation
	if(Front_Panel_Event())
	{
		uint16_t buttonStatus = Front_Panel_Button_Status();
		switch(buttonStatus)
		{
		case BUTTON_STATUS_MENU_UP_PRESS:
			Effect_Param_Previous(); 				// Go to previous parameter
			initialized = 0; 						// Reinitialize screen
			break;
		case BUTTON_STATUS_MENU_OK_PRESS:
			nextSystemState = SYSTEM_STATE_DEFAULT;	// Return to default state
			break;
		case BUTTON_STATUS_MENU_DOWN_PRESS:
			Effect_Param_Next(); 					// Go to next parameter
			initialized = 0;						// Redraw screen
			break;
		case BUTTON_STATUS_STOMP_DOWN_PRESS:
			nextSystemState = SYSTEM_STATE_DEFAULT;	// Return to default state
			break;
		case BUTTON_STATUS_STOMP_DOWN_HOLD:
			nextSystemState = SYSTEM_STATE_TUNER;	// Change to tuner state
			break;
		case BUTTON_STATUS_STOMP_UP_PRESS:
			nextSystemState = SYSTEM_STATE_DEFAULT;	// Return to default state
			break;
		case BUTTON_STATUS_STOMP_UP_HOLD:
			nextSystemState = SYSTEM_STATE_LOOPER;	// Change to looper state
			break;
		case BUTTON_STATUS_STOMP_EFFECT_PRESS:
			if(Effect_Get_Current_Activated_Status())	// Toggle effect activation
			{
				Effect_Deactivate_Current();
				LED_Status(0);						// Turn off LED if effect off
			}
			else
			{
				Effect_Activate_Current();
				LED_Status(1);						// Turn on LED if effect on
			}
			break;
		case BUTTON_STATUS_STOMP_EFFECT_HOLD:
			nextSystemState = SYSTEM_STATE_TEMPO;	// Change to tempo state
			break;
		default:
			// Change parameter value depending on rotation of encoder
			if(Front_Panel_Encoder_Rotation() == ENCODER_COUNTERCLOCKWISE)
				Effect_Param_Decrease_Current_Value();
			else if(Front_Panel_Encoder_Rotation() == ENCODER_CLOCKWISE)
				Effect_Param_Increase_Current_Value();
			Front_Panel_Reset_Encoder_Rotation();
			// Only update the current value to avoid having to redraw the entire screen
			Display_Erase_Area(0x1C,4);
			Display_Write_Number(Effect_Param_Get_Current_Value(), 4, 0x1C,
									DISPLAY_ALIGN_LEFT);
			// Update bar graph
			Display_Write_Bar_Graph(0x54, 20, Effect_Param_Get_Current_Value(),
					Effect_Param_Get_Current_Min_Value(),
					Effect_Param_Get_Current_Max_Value());
			break;
		}
		Clear_Front_Panel_Event();
		Reset_Front_Panel_Button_Status();
	}
	if (nextSystemState != SYSTEM_STATE_ADJUST)		// Detect transitions out of state
	{
		initialized = 0;	// Reinitialize next time state is entered
	}
	return nextSystemState;
}

uint8_t Run_State_Tuner(void)
{
	static uint8_t initialized = 0;		// Store if state has been initialized
	uint8_t nextSystemState = SYSTEM_STATE_TUNER;

	// Initialize the display if state has not been previously initialized
	if(initialized == 0)
	{
		Audio_Tuner_Activate();	// Changes the audio processing to the tuner state
		// Turn on status LED depending on whether output has been turned on
		if(Audio_Tuner_Get_Output_Status())
			LED_Status(1);
		else
			LED_Status(0);
		Display_Clear();		// Clear the entire display

		// Line 1
		Display_Write_String("Tuner", 0);
		Display_Write_String("Ref = ", 0x09);
		Display_Write_Number(Audio_Tuner_Get_Reference_Freq(), 3, 0x0F,
								DISPLAY_ALIGN_LEFT);
		Display_Write_String("Hz", 0x12);

		// Line 2

		// Line 3
		Display_Write(0x00, 1, 0x1D); // Down arrow

		// Line 4
		// Initialize bar graph to half length
		Display_Write_Bar_Graph(0x54, 20, 50, 0, 100);

		initialized = 1;			// Signify that state has been initialized


	}
	// When audio buffer is full, start the tuner to calculate the correct note
	if(Audio_Tuner_Get_Ready_Flag())
	{
		Audio_Tuner();	// Run the tuner function
		uint8_t note;	// Variable to store detected note
		int8_t cents;	// Variable to store number of cents
		char* noteName = Audio_Tuner_Get_Note(&note, &cents);	// Get the tuner result

		// Write the note name to the screen
		Display_Erase_Area(0x49,2);
		Display_Write_String(noteName,0x49);

		// Update the bar graph depending on the cents variable
		Display_Write_Bar_Graph(0x54, 20, (uint16_t)(cents + 50), 0, 100);

		// Write the number of cents to the screen
		Display_Erase_Area(0x21,3);
		// Draw negative sign if cents < 0
		if(cents < 0)
		{
			Display_Write_Char('-', 0x21);
			cents = -1 * cents;
		}
		Display_Write_Number((uint16_t)cents, 2, 0x22, DISPLAY_ALIGN_LEFT);
		Audio_Tuner_Reset_Ready_Flag();
	}
	// Check if user has interacted with front panel, and process the correct button
	// press or encoder rotation
	if(Front_Panel_Event())
	{
		uint16_t buttonStatus = Front_Panel_Button_Status();
		switch(buttonStatus)
		{
		case BUTTON_STATUS_MENU_UP_PRESS:
			Audio_Tuner_Increase_Reference_Freq();
			Display_Erase_Area(0x4F,3);
			Display_Write_Number(Audio_Tuner_Get_Reference_Freq(), 3, 0x0F,
									DISPLAY_ALIGN_LEFT);
			break;
		case BUTTON_STATUS_MENU_DOWN_PRESS:
			Audio_Tuner_Decrease_Reference_Freq();
			Display_Erase_Area(0x4F,3);
			Display_Write_Number(Audio_Tuner_Get_Reference_Freq(), 3, 0x0F,
									DISPLAY_ALIGN_LEFT);
			break;
		case BUTTON_STATUS_STOMP_DOWN_HOLD:
			nextSystemState = SYSTEM_STATE_DEFAULT;	// Return to default state
			break;
		case BUTTON_STATUS_STOMP_EFFECT_PRESS:
			// Turn audio output on and off, and update status LED
			if(Audio_Tuner_Get_Output_Status())
			{
				Audio_Tuner_Disable_Output();
				LED_Status(0);
			}
			else
			{
				Audio_Tuner_Enable_Output();
				LED_Status(1);
			}
			break;
		default:
			break;
		}
		Clear_Front_Panel_Event();
		Reset_Front_Panel_Button_Status();
	}
	if (nextSystemState != SYSTEM_STATE_TUNER)		// Detect transitions out of state
	{
		Audio_Tuner_Deactivate();
		initialized = 0;	// Reinitialize next time state is entered
	}
	return nextSystemState;
}

uint8_t Run_State_Looper(void)
{
	static uint8_t initialized = 0;		// Store if state has been initialized
	static uint8_t eraseInProgress = 0;	// Store if an erase operation is ongoing
	uint8_t nextSystemState = SYSTEM_STATE_LOOPER;

	// Initialize the display if state has not been previously initialized
	if(initialized == 0)
	{
		// If an erase operation is in progress,
		// indicate that on the screen and continue to check
		if(FLASH_Get_Busy_Flag())
		{
			// Only update the screen on the first check
			if(eraseInProgress == 0)
			{
				Display_Clear();	// Clear the entire display
				Display_Write_String("Looper", 0);
				Display_Write_String("Erase in progress...", 0x14);
				eraseInProgress = 1;
			}
		}
		else
		{
			Display_Clear();		// Clear the entire display
			Display_Write_String("Looper", 0);
			if(Audio_Looper_Get_Flash_Status() == 0)
				Display_Write_String("Ready to record", 0x14);
			else
			{
				Display_Write_String("Hold ", 0x14);
				Display_Write_Char(0x00, 0x19);
				Display_Write_String("To Erase", 0x1B);
			}
			eraseInProgress = 0;
			// Blink LED if playback is active
			if(Audio_Get_State() == AUDIO_STATE_PLAYBACK)
				LED_Blink_Status(500);
			else
				LED_Blink_Status(0);
			initialized = 1;			// Signify that state has been initialized
		}
	}
	// Check if user has interacted with front panel, and process the correct button
	// press or encoder rotation
	if(Front_Panel_Event())
	{
		uint16_t buttonStatus = Front_Panel_Button_Status();
		switch(buttonStatus)
		{
		case BUTTON_STATUS_STOMP_DOWN_HOLD:
			Audio_Looper_Stop_Record();					// Stop recording if in progress
			Audio_Looper_Stop_Playback();				// Stop playback if in progress
			Audio_Looper_Delete();						// Delete recording
			LED_Blink_Status(0);						// Turn off blink
			initialized = 0;							// Reinitialize screen
			break;

		case BUTTON_STATUS_STOMP_UP_HOLD:
			nextSystemState = SYSTEM_STATE_DEFAULT;			// Return to default state
			break;
		case BUTTON_STATUS_STOMP_EFFECT_PRESS:
			switch(Audio_Get_State())
			{
			case AUDIO_STATE_DEFAULT:
				if(Audio_Looper_Get_Flash_Status() == 0)	// If flash memory available
				{
					Audio_Looper_Start_Record();		// Start recording
					LED_Status(1);						// Status LED on
				}
				else
				{
					Audio_Looper_Start_Playback();		// Start playback
					LED_Blink_Status(500);				// Blink LED
				}
				break;
			case AUDIO_STATE_RECORD:
				Audio_Looper_Stop_Record();
				LED_Status(0);							// Status LED on
				Audio_Looper_Start_Playback();
				LED_Blink_Status(500);
				initialized = 0;						// Reinitialize screen
				break;
			case AUDIO_STATE_PLAYBACK:
				Audio_Looper_Stop_Playback();
				LED_Blink_Status(0);					// Turn off blink
				break;
			}
			break;
			default:
				break;
		}
		Clear_Front_Panel_Event();
		Reset_Front_Panel_Button_Status();
	}
	if (nextSystemState != SYSTEM_STATE_LOOPER)			// Detect transitions out of state
	{
		if(Audio_Get_State() == AUDIO_STATE_RECORD)
			Audio_Looper_Stop_Record();					// Stop recording
		LED_Blink_Status(0);							// Turn off LED
		initialized = 0;	// Reinitialize next time state is entered
	}
	return nextSystemState;
}

uint8_t Run_State_Tempo(void)
{
	static uint8_t initialized = 0;			// Store if state has been initialized
	uint8_t nextSystemState = SYSTEM_STATE_TEMPO;

	// Initialize the display if state has not been previously initialized
	if(initialized == 0)
	{
		Display_Clear();	// Clear the entire display

		// Line 1
		Display_Write_String("Tap To Set Tempo", 0);

		// Line 3
		Display_Write_String("Tempo = ", 0x14);
		Display_Write_Number(Effect_Get_Tempo(), 3, 0x1E, DISPLAY_ALIGN_RIGHT);
		Display_Write_String("BPM", 0x1F);

		LED_Blink_Status(60000/Effect_Get_Tempo());	// Blink status LED in time with tempo
		initialized = 1;			// Signify that state has been initialized
	}
	// Check if user has interacted with front panel, and process the correct button
	// press or encoder rotation
	if(Front_Panel_Event())
	{
		uint16_t buttonStatus = Front_Panel_Button_Status();
		switch(buttonStatus)
		{
		case BUTTON_STATUS_MENU_UP_PRESS:
			Effect_Increase_Tempo();
			initialized = 0;						// Reinitialize screen
			break;
		case BUTTON_STATUS_MENU_DOWN_PRESS:
			Effect_Decrease_Tempo();
			initialized = 0;						// Reinitialize screen
			break;
		case BUTTON_STATUS_STOMP_DOWN_PRESS:
			Effect_Decrease_Tempo();
			initialized = 0;						// Reinitialize screen
			break;
		case BUTTON_STATUS_STOMP_UP_PRESS:
			Effect_Increase_Tempo();
			initialized = 0;						// Reinitialize screen
			break;
		case BUTTON_STATUS_STOMP_EFFECT_PRESS:
			Effect_Calculate_Tempo();
			initialized = 0;						// Reinitialize screen
			break;
		case BUTTON_STATUS_STOMP_EFFECT_HOLD:
			nextSystemState = SYSTEM_STATE_DEFAULT;	// Return to default state
			break;
		default:
			// Change tempo depending on encoder rotation
			if(Front_Panel_Encoder_Rotation() == ENCODER_COUNTERCLOCKWISE)
			{
				Effect_Decrease_Tempo();
				initialized = 0;
			}
			else if(Front_Panel_Encoder_Rotation() == ENCODER_CLOCKWISE)
			{
				Effect_Increase_Tempo();
				initialized = 0;
			}
			Front_Panel_Reset_Encoder_Rotation();
			break;
		}
		Clear_Front_Panel_Event();
		Reset_Front_Panel_Button_Status();
	}
	if (nextSystemState != SYSTEM_STATE_TEMPO)		// Detect transitions out of state
	{
		LED_Blink_Status(0);	// Turn off LED
		initialized = 0;		// Reinitialize next time state is entered
	}
	return nextSystemState;
}

