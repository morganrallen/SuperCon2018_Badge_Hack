/************************************
 * This is the framework for those
 * who wish to write their own C
 * code for the basic badge
 * 
 * Take a look at user_program_temp.c (not included in project, but
 * available in src directory) to see how to use IIC routines
 ************************************/

#include <xc.h>

#include "badge_user.h"
#include <string.h>

extern uint8_t get_stat,key_buffer_ptr,cmd_line_buff[80], cmd_line_pointer,cmd_line_key_stat_old,prompt;

uint8_t rxBuf[80];
uint8_t rxBufPos;

uint8_t nick[20];

void ircCommand(uint8_t * msg) {
    int len;
    stdio_c('\n');
    if (strncmp("/nick ", msg, 6) == 0) {
        msg += 6;
        len = strlen(msg);
        if (len > 0 && len < 10) {
            strcpy(nick, msg);
            video_set_color(EGA_GREEN,EGA_DGRAY);
            stdio_write("* got it!\n");
        } else {
            video_set_color(EGA_BRED,EGA_DGRAY);
            stdio_write("* Bad command\n");
        }
    }
}

void ircChat(uint8_t * msg) {
    uint8_t *c;
    for (c = nick; *c; c++)
        tx_write(*c);
    tx_write(':');
    for (c = cmd_line_buff; *c; c++)
        tx_write(*c);
    tx_write('\n'); //null terminate packets
    video_gotoxy(0,DISP_BUFFER_HIGH-1);
    video_set_color(EGA_MAGENTA,EGA_BLACK);
    stdio_write(nick);
    stdio_write(":");
    stdio_write(msg);
    stdio_write("\n");
}

void loop_input (void)
	{
    int8_t char_out;
	if (prompt==1)
	    {
        video_set_color(EGA_LGRAY,EGA_BLACK);
        video_gotoxy(0,DISP_BUFFER_HIGH-1);
	    stdio_write(">");	
	    prompt = 0;
	    }
	get_stat = stdio_get(&char_out);
	if (get_stat!=0)
	    {
	    if (char_out==NEWLINE) 
			{
			//stdio_c(char_out);
			cmd_line_buff[cmd_line_pointer] = 0;
            
            //TODO parse irc lite command, or send text
            if (cmd_line_pointer > 1 && cmd_line_buff[0] == '/') {
                ircCommand(cmd_line_buff);
            } else {
                ircChat(cmd_line_buff);
            }
            
			cmd_line_pointer=0;
			cmd_line_buff[0] = 0;
			prompt = 1;			
			}
	    else
			{
			if ((char_out>=' ')&(char_out<0x7F)&(cmd_line_pointer<INPUT_BUFFER_LEN)) 
				{
				stdio_c(char_out);
				cmd_line_buff[cmd_line_pointer++] = char_out;
				}
			else if (char_out==BACKSPACE)
				{
				if (cmd_line_pointer>0) 
					{
					cmd_line_buff[--cmd_line_pointer]=0;
					stdio_c(char_out);
					}
				}

			}
	    }	
	}

void user_program_init(void)
	{
	/* This will run when User Program is first selected form the menu */
    
    strcpy(nick, "Anonymous");
	clr_buffer();
    video_set_color(EGA_LGRAY,EGA_DGRAY);
	video_gotoxy(0,4);
    stdio_write("Welcome Anonymous, to badge hack IRC");
	video_gotoxy(0,5);
	stdio_write("Set your nick with /nick <nick>\nThen chat!");
    
    video_gotoxy(0,DISP_BUFFER_HIGH-1);
    prompt = 1;
    rxBuf[0] = '\0';
    rxBufPos = 0;
    
    EXP_3_OUT = 1;
    EXP_3_T = 0;
//    b1State = 0;
	}

void checkRx(void) {
    int j, i;
    int8_t c;
    int8_t color;
    if (U3STAbits.OERR) U3STAbits.OERR = 0;

    while (rx_sta()) {
        uint8_t c = rx_read();
        if (rxBufPos < 80)
            rxBuf[rxBufPos++] = c;
        if (c == '\n') {
            //TODO write line, mark ready, whatever
            //scroll up to make room
            for (j=1; j<(DISP_BUFFER_HIGH-1); j++){
                for (i=0; i<DISP_BUFFER_WIDE; i++) {
                    disp_buffer[j-1][i] = disp_buffer[j][i];
                    color_buffer[j-1][i] = color_buffer[j][i];
                }
            }
            for (i=0; i<DISP_BUFFER_WIDE; i++) {
                c = ' ';
                color = 0;
                if (i < rxBufPos) {
                    c = rxBuf[i];
                    color = 0x87;
                }
                disp_buffer[DISP_BUFFER_HIGH-2][i] = c;
                color_buffer[DISP_BUFFER_HIGH-2][i] = color;
            }
            rxBufPos = 0;
        }
    }
}

static int brkCnt;
static int b1State;
void user_program_loop(void) {
    
    //drain any serial data buffer
    checkRx();
    //check for user input, using cli reader code
    loop_input();
    
//    if (b1State == 1 && KEY_BRK==0) {
//        b1State = 2;
//    }
//    if (b1State == 1 && KEY_BRK!=0) {
//         
//    }
    
}

void example_user_program_loop(void)
	{
	/* This should contain user program (will be run after init function)*/
	
	/* Write some stuff to the screen*/
	video_set_color(3,8);	//Set color before writing
	video_gotoxy(5,7);		//Move cursor
	stdio_write(" Behold, color! And ASCII: \xCC\xCD\xCD\xB9 "); // stdio_write will affect both color and character in a location
	video_set_color(15,0);
	video_gotoxy(5,10);
	stdio_write("Counting seconds:");
	video_gotoxy(5,14);
	stdio_write("Waiting for you to press any key...");
	
	while(1) //Loop forever
		{
		/* Example of a non-blocking delay */
		static uint32_t delay_until = 0;
		static uint8_t count = '0';
		if (millis()>=delay_until)
			{
				/* Writing directly to disp_buffer instead of stdio_write */
				/* Note that Y value is given before X: disp_buffer[Y][X]*/
				disp_buffer[10][24] = count++;
				if (count > '9')
					{
						count = '0';
					}
				delay_until = millis()+1000;
			}

		/* React to user input */
		uint8_t get_stat, char_out;
		get_stat = stdio_get(&char_out);
		if (get_stat!=0)
			{
			/* Show which letter was pressed just as an example: */
			/* Directly address color_buffer: first nibble is BG, second nibble is FG*/
			color_buffer[16][8] = 0x1E;
			if (char_out > 31) // Make sure it was a printable character
				{
				disp_buffer[16][8] = char_out;
				}
			else disp_buffer[16][8] = 0xEC;
			
			video_gotoxy(5,19);
			stdio_write("2s until manual screen writing demo");
			wait_ms(2000);	//Make sure user sees entered character
			
			/* Now do something interesting */
			enable_display_scanning(0); //Shut off auto-scanning of character buffer
			uint16_t x = 0;
			uint8_t y = 0;
			uint32_t colorshift = 0;
			while(1) //Loop forever
				{
				/* Draw a 1x1 box at x,y -- color is using XOR trick but should be format 0x00RRGGBB */
				tft_fill_area (x, y, 1, 1, (x^(uint32_t)y)+colorshift);
				++x; //Increment x
				
				/* Reset x/y as necessary */
				if (x >= 320)
					{
					x = 0;
					++y;
					}
				if (y >= 240)
					{
					y = 0;
					colorshift += 0x00123456;
					}
			
				/* Waste some time but less than 1 ms */
				uint16_t quickdelay = 2000;
				while (quickdelay)
					{
					--quickdelay;
					}
				}
			//Badge will need to be reset to continue
			}
		}
}
