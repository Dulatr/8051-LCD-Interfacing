  //Tyler Dula
  //DOES: Interfacing HD44780 Controller with 8051 Controller to display on a dot matrix LCD Screen
  //     - A 4x3 numeric keypad is tied to an MM74C922 16-key decoder which has 4 data out bits, and sends an interrupt signal to INT0
  //	 - The high nibble of port 1 is used as the data out bus for the LCD controller in 4-bit mode
  //	 - Keypad characters are decoded onto the low nibble of Port 0 and sent to the LCD
  //	 - Port 3 pins are serviced as the control bus lines to the LCD
  //	 - * from keypad is backspace, # is carriage return

  #include <REG51.h>
  
  unsigned int delay;
  unsigned int position;

  bit q = 0;
  
  unsigned int R4;
  unsigned int R5;

  bit TESTBIT;
  bit BUSYBIT = 1;

  sbit LCD_E = P3^4;
  sbit LCD_RS = P3^0;
  sbit LCD_RW = P3^1;

  sbit LCD_DB4 = P1^4;		 //High data nibble
  sbit LCD_DB5 = P1^5;
  sbit LCD_DB6 = P1^6;
  sbit LCD_DB7 = P1^7;

  unsigned char bdata R0;
  unsigned char bdata cursPos;
  sbit Bit1 = R0^0;
  sbit Bit2 = R0^1;
  sbit Bit3 = R0^2;
  sbit Bit4 = R0^3;
  sbit Bit5 = R0^4;
  sbit Bit6 = R0^5;
  sbit Bit7 = R0^6;
  sbit Bit8 = R0^7;
    
  //system instructions
  unsigned char Config = 0x28;
  unsigned char entryMode = 0x06;	

  //cursor control instructions
  unsigned char offCur = 0x0C;
  unsigned char lineCur = 0x0E;
  unsigned char blinkCur = 0x0D;
  unsigned char combnCur = 0x0F;
  unsigned char homeCur = 0x02;
  unsigned char shLFTCur = 0x10;
  unsigned char shRTCur = 0x14;
  unsigned char Row0 = 0x80;
  unsigned char endRow0 = 0x93;
  unsigned char Row1 = 0xC0;
  unsigned char endRow1 = 0xD3;
  unsigned char Row2 = 0x94;
  unsigned char endRow2 = 0xA7;
  unsigned char Row3 = 0xD4;
  unsigned char endRow3 = 0xE7;

  //Display control instructions
  unsigned char clrDSP = 0x01;
  unsigned char offDSP = 0x08;
  unsigned char onDSP = 0x0C;
  unsigned char shLFDSP = 0x18;
  unsigned char shRTDSP = 0x1C;
   
  //function declarations
  void mdelay(unsigned int time);
  void resetLCD4(void);
  void pulseEwait4(void);
  void wrLCD4(void);
  void LCDoutstr(void);

  void LCDoutchar(void);
  void Newrow(void);
  void readLCD4(void);
  void checkFWDCurs(void);	
  void checkBCKCurs(void);

  //data tables
  unsigned char code table[] = {"GRAD PROJECT BY:$"};
  unsigned char code table2[] = {"Tyler Dula$"};
  unsigned char code table3[] = {"05-14-17$"};

  unsigned char ASCII[] = {0x31,0x32,0x33,0x00,0x34,0x35,0x36,0x00,0x37,0x38,0x39,0x00,0x2A,0x30,0x23,0x00}; //2A,30,23

  unsigned char code * PTR = &table;

  //troubleshooting
  sbit portbit = P0^4;

  main()
  {
  	IT0 = 1;
	EX0 = 1;
	EX1 = 1;
	IT1 = 1;
	EA = 1;

	
	P0 = 0xFF;		 //initialize port before interrupts
	resetLCD4();	 //reset the LCD

	R0 = *PTR;		 //print the first line
	LCDoutstr();

	PTR = &table2[0]; //prepare the next string to write
	R0 = 0xC0;		  //select line next line
	LCD_RS = 0;		  //sending instruction
	wrLCD4();

	R0 = *PTR;
	LCDoutstr();	 //display the string

    PTR = &table3[0];	//3 line to print
	R0 = 0x94;			//select the next row
	LCD_RS = 0;			//sending instruction
	wrLCD4();

	R0 = *PTR;
	LCDoutstr();		//display the string

	position = 0;
		  
	while(1);			//hang up
  }


  //------------------FUNCTIONS------------------------

// ====================================================
// subroutine resetLCD4 - reset the LCD
// software version of the power on reset operation
// see the Optrex 4-bit reset flow chart!
//
// Originally written in Assembly, Rewritten in C
// RESET WORKS FINE
// ----------------------------------------------------

void resetLCD4(void)
{

	mdelay(40);

	LCD_E = 0;		  //begin enable pulse
	LCD_RW = 0;		  //0 means write
	LCD_RS = 0;		  //Select instruction register

	LCD_DB7 = 0;
	LCD_DB6 = 0;	  //Function Set instruction 30h
	LCD_DB5 = 1;
	LCD_DB4 = 1;	  //make 8 bit to initialize

	LCD_E = 1;		 //enable pulse
	TESTBIT = TESTBIT;
	LCD_E = 0;
	mdelay(10);

	LCD_E = 1;
	TESTBIT = TESTBIT;	 //so it recieves the Function set 3 times 
	LCD_E = 0;
	mdelay(5);

	LCD_E = 1;
	TESTBIT = TESTBIT;
	LCD_E = 0;
	mdelay(5);

	LCD_DB7 = 0;
	LCD_DB6 = 0;	  //Function Set instruction 20h
	LCD_DB5 = 1;	  
	LCD_DB4 = 0;	  //set to 4bit

	LCD_E = 1;
	TESTBIT = TESTBIT;
	LCD_E = 0;
	mdelay(5);

	R0 = Config;	  //set configuration to: 4bit data, 2 lines, 5x7 font
	LCD_RS = 0;
	wrLCD4();

	R0 = offDSP;
	LCD_RS = 0;
	wrLCD4();

	R0 = clrDSP;
	LCD_RS = 0;
	wrLCD4();

	R0 = entryMode;
	LCD_RS = 0;
	wrLCD4();

	R0 = onDSP;
	LCD_RS = 0;
	wrLCD4();
}








// ====================================================
// subroutine mdelay revised by Tyler Dula
// use for LCD initialization routine delays
// if using 12 MHZ crystal, 
// delays ABOUT m milliseconds without using a timer
// (each millisecond delay is about 9 us too long)
// the # milliseconds should be put in acc
//
//Inputs: unsigned integer
//Outputs: None
//
//Rewritten in C
// ----------------------------------------------------
void mdelay(unsigned int time)
{
	delay = time;

	while (delay > 0)
	{
	   for (R4 = 1;R4 > 0;R4--)
	   {
			for (R5 = 110;R5 > 0;R5--);			
	   }
	   delay = delay - 1;
	} 

}






//; ====================================================
//; subroutine pulseEwait4
//; generates a positive pulse on the LCD enable line.
//; waits for the Busy Flag to clear before returning.
//; input    : none
//; output   : none
//;
//; Rewritten for C
//; ----------------------------------------------------
void pulseEwait4(void)
{
	BUSYBIT = 1;

	LCD_E = 1;
	TESTBIT = TESTBIT;
	LCD_E = 0;

	LCD_DB7 = 1;
	LCD_RS = 0;

	LCD_RW = 1;

	while(BUSYBIT == 1)
	{
		LCD_E = 1;
		BUSYBIT = LCD_DB7;
		LCD_E = 0;
		
		LCD_E = 1;
		TESTBIT = TESTBIT;
		LCD_E = 0;
	}
}








//; ====================================================
//; subroutine wrLCD4
//; writes 8-bit INSTRUCTION or 8-bit DATA to LCD controller
//; (writes the 8 bits to the LCD 4 bits at a time)
//; The 8-bits must be placed in r0 by calling program
//; LCD_RS MUST be set in main program BEFORE calling 
//; this subroutine: (LCD_RS=0 INSTRUCTION , LCD_RS=1 DATA).
//; ----------------------------------------------------
void wrLCD4(void)
{

  LCD_E = 0;		  //disable during transmitting
  LCD_RW = 0;		  //set screen to read in data

  LCD_DB4 = Bit5;
  LCD_DB5 = Bit6;	  //send high nibble of data
  LCD_DB6 = Bit7;
  LCD_DB7 = Bit8;

  LCD_E = 1;		  //Pulse enable
  TESTBIT = TESTBIT;
  LCD_E = 0;

  LCD_DB4 = Bit1;	  //then send the low nibble
  LCD_DB5 = Bit2;
  LCD_DB6 = Bit3;
  LCD_DB7 = Bit4;

  pulseEwait4();

}


//; ====================================================
//; subroutine readLCD4
//; reads 8-bit cursor position to LCD controller
//; (reads the 8 bits to the LCD 4 bits at a time)
//; The 8-bits must be placed in r0 by calling program
//; LCD_RS MUST be LOW in main program BEFORE calling 
//; this subroutine: (LCD_RS=0 INSTRUCTION , LCD_RS=1 DATA).
//; ----------------------------------------------------
void readLCD4(void)
{
  P1 = 0xFF;

  LCD_E = 1;		  //disable during transmitting
  LCD_RW = 1;		  //set screen to write out data

  Bit5 = LCD_DB4;
  Bit6 = LCD_DB5;	  //retriev high nibble of data
  Bit7 = LCD_DB6;
  Bit8 = LCD_DB7;

  LCD_E = 0;		  //Pulse enable
  TESTBIT = TESTBIT;
  LCD_E = 1;

  Bit1 = LCD_DB4;	  //then retrieve the low nibble
  Bit2 = LCD_DB5;
  Bit3 = LCD_DB6;
  Bit4 = LCD_DB7;

  LCD_E = 0;

}




//; ====================================================
//; subroutine LCDoutstr revised by Tyler Dula
//; (modified McKenzies monitor outstr routine)
//; print a STRING of characters to the LCD
//; string MUST end with a $
//
//  Rewritten in C language
//; ----------------------------------------------------
void LCDoutstr(void)
{
   while(R0 != 0x24)	// $ terminated strings, can make null terminated by ' ' 
   {
	  LCD_RS = 1;		//to send the data
	  if (R0 == 0x24)
	  {
	  	R0 = ' ';		//display a blank instead of money sign
 	  }
	  wrLCD4();
	  PTR++;
	  R0 = *PTR;
   }
}






//; ====================================================
//; subroutine LCDoutchar by Tyler Dula
//; print a single character on P0 from the number pad to the LCD
//;	--simple function that calls wrLCD4 and writes RS high for data
//;  Rewritten in C language
//; ----------------------------------------------------
void LCDoutchar(void)
{
   LCD_RS = 1;	 //sending data
  
   wrLCD4();	 //write to screen
}


//; ====================================================
//; PRIMARY SERVICE ROUTINE: ISR
//; DOES: -Handles interrupts incoming from keypad and calls LCDoutchar to display
//; 	  -Leave the cursor on and blinking
//; ----------------------------------------------------


void ISR(void) interrupt 0
{

 if (q == 0)				//first interrupt?
 {
	  resetLCD4();			//clear the screen for new inputs
 	  R0 = ASCII[P0&0x0F];	//grab character to display

		  if (R0 == 0x2A)	//if * then shift cursor to the left
		  {
			checkBCKCurs();  //where is the cursor located? Want to go back to appropriate line
							 // Otherwise, if not at any of the starting locations, move on
			LCD_RS = 0;		 //send instruction
			R0 = shLFTCur;	 //shift the cursor
			wrLCD4();

			LCD_RS = 1;		 //send data
			R0 = ' ';
			wrLCD4();		 //clear character

			LCD_RS = 0;		 //send instruction
			R0 = shLFTCur;
			wrLCD4();
		  } 

	 	  else if (R0 == 0x23)	  //if # then carriage return to next line
		  {
				  checkFWDCurs();
				  Newrow();	  
		  }
	
	      else
		  {
				  LCDoutchar();	 //display character otherwise
				  checkFWDCurs();
		  }

 LCD_RS = 0;  //sending instruction
 R0 = combnCur;
 wrLCD4();	  //turn cursor on and blinking

 q = 1;	  			 //never enter this loop again
 }



 else 						 //every consecutive interrupt
 {
	  R0 = ASCII[P0&0x0F];	 //grab character to display

		  if (R0 == 0x2A)	 //if * shift cursor to the left
		  {
			checkBCKCurs();  //where is the cursor located? Want to go back to appropriate line
							 // Otherwise, if not at any of the starting locations, move on
			LCD_RS = 0;		 //send instruction
			R0 = shLFTCur;	 //shift the cursor
			wrLCD4();

			LCD_RS = 1;		 //send data
			R0 = ' ';
			wrLCD4();		 //clear character

			LCD_RS = 0;		 //send instruction
			R0 = shLFTCur;
			wrLCD4();
		  }
		   
	 	  else if (R0 == 0x23)	  //if # then carriage return to next line
		  {
				    checkFWDCurs();  //grab the position of cursor
					Newrow();	     //adjust row
		  }
	
	   	  else
		  {			
					LCDoutchar();	 //display character otherwise
					checkFWDCurs();
		  }
 }
 
// LCD_RS = 0;  //sending instruction
// R0 = combnCur;
// wrLCD4();	  //turn cursor on and blinking
}


//////////////////////

void checkFWDCurs(void)
{
 LCD_RS = 0;
 readLCD4();

 cursPos = R0;

 if (cursPos == 0x14) 	   //cursor is in row0
 {
	//load the DDRAM address
	LCD_RS = 0;
	LCD_RW = 0;				 //make row1
	R0 = Row1;
	wrLCD4();
 }
  else if (cursPos == 0x54) 	   //cursor is in row1
 {
	//load the DDRAM address
	LCD_RS = 0;
	LCD_RW = 0;				 //make row2
	R0 = Row2;
	wrLCD4();
 }
  else if (cursPos == 0x40) 	   //cursor is in row2
 {
	//load the DDRAM address
	LCD_RS = 0;
	LCD_RW = 0;				 //make row3
	R0 = Row3;
	wrLCD4();
 }

}

//////////////////////
void checkBCKCurs(void)
{
	 LCD_RS = 0;
	 readLCD4();
	
	 cursPos = R0;
	
	 if (cursPos == 0x40) 	   //cursor is in row1
	 {
		
		LCD_RS = 0;
		LCD_RW = 0;				 //make end of row0
		R0 = endRow0;
		wrLCD4();

		LCD_RS = 1;		 //send data
		R0 = ' ';
		wrLCD4();		 //clear character

	 }
	  else if (cursPos == 0x14) 	   //cursor is in row2
	 {
		
		LCD_RS = 0;
		LCD_RW = 0;				 //make end of row1
		R0 = endRow1;
		wrLCD4();

		LCD_RS = 1;		 //send data
		R0 = ' ';
		wrLCD4();		 //clear character
	 }
	  else if (cursPos == 0x54) 	   //cursor is in row3
	 {
		//load the DDRAM address
		LCD_RS = 0;
		LCD_RW = 0;				 //make row3
		R0 = endRow2;
		wrLCD4();

		LCD_RS = 1;		 //send data
		R0 = ' ';
		wrLCD4();		 //clear character
	 }
	  else if (cursPos == 0x00) 	   //cursor is in row3
	 {
		//load the DDRAM address
		LCD_RS = 0;
		LCD_RW = 0;				 //make row3
		R0 = endRow3;
		wrLCD4();

		LCD_RS = 1;		 //send data
		R0 = ' ';
		wrLCD4();		 //clear character
	 }
}

//////////////////////
void Newrow(void)
{
 
   if (cursPos >= 0x00 && cursPos <= 0x13) //in row0 ?
	{	 
		 LCD_RS = 0;	//sending instruction
		
		 R0 = Row1;		 //row1
		
		 wrLCD4();
	 }
   else if (cursPos >= 0x40 && cursPos <= 0x53) //in row1?
	 {
		 LCD_RS = 0;	//sending instruction
		
		 R0 = Row2;		 //row2
		
		 wrLCD4();
	 }
   else if (cursPos >= 0x14 && cursPos <= 0x27) //in row2?
     {
		 LCD_RS = 0;	//sending instruction
		
		 R0 = Row3;		 //row3
		
		 wrLCD4();
	 }
   else if (cursPos >= 0x54 && cursPos <= 0x67)	//in row3?
     {
		 LCD_RS = 0;	//sending instruction
		
		 R0 = Row0;		 //row0
		
		 wrLCD4();
	 }
}


