// Serial LCD interface for 40x4 lcd.  Also includes LED and
// beeper.

#pragma CLOCK_FREQ 10000000

// All LCD things on port B.  Data bus is lower 4 bits
#define E1  0x10
#define E2  0x20
#define RS  0x40

// Misc port a junk
#define BEEPER 0
#define LED    1

#pragma RS232_RXPORT PORTA
#pragma RS232_RXPIN  2
#pragma RS232_BAUD   9600
#pragma TRUE_RS232   0

char emask;      // Set to E1, E2, or both

void LCDinit(void);
void LCDnibble(char c);
void LCDchar(char c);
void LCDcmd(char c);
void LCDstring(const char *s);
void LCDdelay(void);
void LCDtinydelay(void);

#define clear_lcd         0x01 /* Clear Display                        */ 
#define return_home       0x02 /* Cursor to Home position              */ 
#define entry_mode        0x06 /* Normal entry mode                    */ 
#define entry_mode_shift  0x07 /* - with shift                         */ 
#define system_set_8_bit  0x38 /* 8 bit data mode 2 line ( 5x7 font )  */ 
#define system_set_4_bit  0x28 /* 4 bit data mode 2 line ( 5x7 font )  */ 
#define display_on        0x0c /* Switch ON Display                    */ 
#define display_off       0x08 /* Cursor plus blink                    */ 
#define set_dd_ram        0x80 /* Line 1 position 1                    */ 
#define set_cg_ram        0x40 /* Beginning of CGRAM                   */ 
#define cursor_on         0x0E /* Switch Cursor ON                     */ 
#define cursor_off        0x0C /* Switch Cursor OFF                    */ 

void main(void) {
  char ch;
  
  asm {
    clrf	PORTA
    clrf	PORTB
    bsf         STATUS,RP0
    movlw	0x0C
    movwf	TRISA
    clrf	TRISB
    bcf	        STATUS,RP0
  }

  // Init both LCDs at the same time
  emask = E1 | E2;
  LCDinit();
  
  // Say hi until the serial port says otherwise
  emask = E1;
  LCDstring("LCD interface ready");
  emask = E2;
  LCDstring("Micah Dowty, July 2000");
  emask = E1;

  // Wait for input
  while (1) {
    ch = getchar();

    // Process escape chars
    if (ch == '\\') {
      ch = getchar();
      switch (ch) {

      case '\\':
	LCDchar('\\');
	break;

      case '1':    // First display
	emask = E1;
	break;

      case '2':    // Second display
	emask = E2;
	break;

      case '*':    // All displays
	emask = E1 | E2;
	break;

      case 'c':    // Next char is an LCD command
	ch = getchar();
	LCDcmd(ch);
	break;

      case 's':    // Clear screen
	LCDcmd(clear_lcd);
	break;

      case 'g':    // Go to the beginning of CGRAM
	LCDcmd(set_cg_ram);
	break;

      case 'd':    // Go to the beginning of DDRAM
	LCDcmd(set_dd_ram);
	break;

      case 'b':    // Beep
	output_high_port_a(BEEPER);
	delay_ms(250);
	output_low_port_a(BEEPER);
	break;

      case 'f':    // Flash
	output_high_port_a(LED);
	delay_ms(250);
	output_low_port_a(LED);
	break;

      case 'n':    // Go to line 2
	LCDcmd(set_dd_ram + 40);
	break;

      }
    }
    else {
      LCDchar(ch);
    }
  }
}

void LCDinit(void) {
  delay_ms(40);
  LCDnibble(system_set_4_bit >> 4);
  delay_ms(5);
  LCDnibble(system_set_4_bit >> 4);
  delay_ms(5);
  LCDnibble(system_set_4_bit >> 4);
  delay_ms(5);

  // Reset complete. Start init

  LCDchar(system_set_4_bit);
  delay_ms(2);
  LCDchar(display_off);
  delay_ms(2);
  LCDchar(entry_mode);
  delay_ms(2);
  LCDchar(display_on);
  delay_ms(2);
  LCDchar(set_dd_ram);
  delay_ms(2);

  // Data mode
  PORTB |= RS;
}

void LCDnibble(char c) {
  PORTB &= 0xF0;
  PORTB |= c & 0x0F;
  LCDtinydelay();
  PORTB |= emask;
  LCDtinydelay();
  PORTB &= 0xCF;
  LCDtinydelay();
}

void LCDchar(char c) {
  LCDnibble(c >> 4);
  LCDnibble(c);
}

void LCDcmd(char c) {
  PORTB &= ~RS;
  LCDchar(c);
  PORTB |= RS;  
}

void LCDstring(const char *s) {
  char i;
  char x;
  for (i=0;;i++) {
    x = s[i];
    if (!x) return;
    LCDchar(x);
    LCDdelay();
  }
}

void LCDdelay(void) {
  delay_ms(1);
}

void LCDtinydelay(void) {
  nop();
  nop();
}

