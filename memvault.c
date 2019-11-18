/*
 *
 * memvault - Ten no Koe Bank modernization, not requiring battery
 *
 *            Erases/reprograms sectors on connected flash memory
 *            (critical sections must be run from RAM)
 *
 *            Implemented using a simple menu system; not as polished
 *            as the original Ten no Koe Bank
 *
 */

/*
 * Note: Compiled with HuC version 3.21, with a bugfix to huc_gfx.asm
 *
 *       Not sure whether it will work if compiled on newer
 *       version of HuC
 *
 */

/* TODOs:
 *
 * - Increase # of banks by using 4K sectors in Flash, not just 8K blocks
 *
 * - Perhaps increase size of stored comment (although display and management
 *   could be challenging for the UI)
 *
 */

#include "huc.h"



#asm


flashaccess:

flashid:
	SEI
	TMA	#2
	PHA
	LDA	_flashbank	; map flash write bank to memory $4000-$5FFF
	TAM	#2

	TMA	#3
	PHA
	LDA	#1	; map flash offset $2000 to memory $6000-$7FFF
	TAM	#3

	TMA	#4
	PHA
	LDA	#2	; map flash offset $4000 to memory $8000-$9FFF
	TAM	#4

; set of MMR5 is just a placeholder
;
;	TMA	#5
;	PHA
;	LDA	#$F7	; map Backup RAM bank to memory $A000-$BFFF
;	TAM	#5

	; write the "Software Entry ID" code
	LDA	#$AA
	STA	$9555
	LDA	#$55
	STA	$6AAA
	LDA	#$90
	STA	$9555
	NOP		; wait at least 150ns

	; get software ID
	LDA	$E000
	STA	_id_a
	LDA	$E001
	STA	_id_b

	; write the "Software ID Exit" code
	LDA	#$AA
	STA	$9555
	LDA	#$55
	STA	$6AAA
	LDA	#$F0
	STA	$9555
	NOP		; wait at least 150ns

;	PLA
;	TAM	#5

	PLA
	TAM	#4
	PLA
	TAM	#3
	PLA
	TAM	#2
	CLI
	RTS
flashidend:

erasesec:
	SEI
	TMA	#2
	PHA
	LDA	_flashbank	; map flash write bank to memory $4000-$5FFF
	TAM	#2

	TMA	#3
	PHA
	LDA	#1	; map flash offset $2000 to memory $6000-$7FFF
	TAM	#3

	TMA	#4
	PHA
	LDA	#2	; map flash offset $4000 to memory $8000-$9FFF
	TAM	#4

; set of MMR5 is just a placeholder
;
;	TMA	#5
;	PHA
;	LDA	#$F7	; map Backup RAM bank to memory $A000-$BFFF
;	TAM	#5

	; write the "Erase Sector" code for 1st 4KB sector
	LDA	#$AA	; AA->5555, 55->2AAA, 80->5555, AA->5555, 55->2AAA, 30->sector addr
	STA	$9555
	LDA	#$55
	STA	$6AAA
	LDA	#$80
	STA	$9555
	LDA	#$AA
	STA	$9555
	LDA	#$55
	STA	$6AAA
	LDA	#$30
	STA	$4000   ; *** NOTE: for 4K sectors, choose between $4000 or $5000 !! ***

eraselp:
	NOP
	LDA	$4000   ; *** NOTE: use same address as above ***
	CMP	#$FF
	BNE	eraselp
	
;	PLA
;	TAM	#5

	PLA
	TAM	#4
	PLA
	TAM	#3
	PLA
	TAM	#2
	CLI
	RTS
erasesecend:

flashwrite:
;
; NOTE: this code is fully-relocatable (be careful if adjusting the code)
;
	SEI
	TMA	#2
	PHA
	LDA	_flashbank	; map flash write bank to memory $4000-$5FFF
	TAM	#2

	TMA	#3
	PHA
	LDA	#1	; map flash offset $2000 to memory $6000-$7FFF
	TAM	#3

	TMA	#4
	PHA
	LDA	#2	; map flash offset $4000 to memory $8000-$9FFF
	TAM	#4

	TMA	#5
	PHA
	LDA	#$F7	; map Backup RAM bank to memory $A000-$BFFF
	TAM	#5
	
	CSL
	LDA	#$48
	STA	$1807	; unlock BRAM (step 1)
	LDA	#$75
	STA	$1807	; unlock BRAM (step 2)
	LDA	#$80
	STA	$1807	; unlock BRAM
	TII	$A000,_buffer,2048
	LDA	$1803	;lock BRAM
	CSH


	; Write $0800 bytes from source @ _buffer to flash @ $4000
	;
	stw	#_buffer, <_ax
	stw	#$4000, <_bx   ; *** NOTE: for 4K sectors, choose between $4000 and $5000
	stw	#$0800, <_cx

	BSR	writelp

	; Write 11 bytes from source @ _date to flash @ $4800
	;
	stw	#_date, <_ax
	stw	#$4800, <_bx   ; *** NOTE: for 4K sectors, choose between $4800 and $5800
	stw	#11, <_cx

	BSR	writelp

	; Write 65 bytes from source @ _comment to flash @ $4810
	;
	stw	#_comment, <_ax
	stw	#$4810, <_bx   ; *** NOTE: for 4K sectors, choose between $4810 and $5810
	stw	#65, <_cx

	BSR	writelp

writeend:
	
	PLA
	TAM	#5
	PLA
	TAM	#4
	PLA
	TAM	#3
	PLA
	TAM	#2
	CLI
	RTS

writelp:
	BSR	flashonebyte

	incw	<_ax
	incw	<_bx
	decw	<_cx

	tstw	<_cx
	BNE	writelp
	RTS


flashonebyte:
	; write the "Write Byte" code
	LDA	#$AA	; AA->5555, 55->2AAA, A0->5555, data->addr
	STA	$9555
	LDA	#$55
	STA	$6AAA
	LDA	#$A0
	STA	$9555

	LDA	[_ax]
	STA	[_bx]
byteloop:
	NOP
	NOP
	NOP
	LDA	[_bx]
	CMP	[_ax]
	BNE	byteloop
	
	; we can either check status, or wait 72 cycles....
	
	RTS

flashwriteend:

flashend:

#endasm

#incbin(kana_font, "kana_font.bin");

#define TITLE_LINE       1
#define INSTRUCT_LINE    3
#define STAT_LINE        5
#define HEX_LINE         9

#define BASE_FLASH_BANK  0x20


char buffer[2048];
char asmbuf[1024];

char pad;
char hex1;
char hex2;

char i;
char j;
char retcode;


int offset;
char char1;


char bank2;
char bank3;
char bank4;
char bank5;
char id_a;
char id_b;
char flashbank;
char page;

char card_date[11];    /* this is latest date stored on card */
char today_date[11];   /* this is date from user input */
char default_date[11]; /* this is the initial/base date on an empty card */

const int days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
const char letter_display[] =
   {"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890.,-[_]'!#%      "};
                              /* "<BCK> <SP> <END>" */

char name[11];
char date[11];
char today_comment[16];
char comment_buf[16];

char comment[65];      /* this is used as 15-byte 'Name', but could be longer */
                       /* (user interface might become more difficult though) */

int year;
int month;
int day;

int end_addr;
int next_free;
int offset_ptr;
int save_size;

char countdown;
char advance;
int menu_A;
int menu_B;
int confirm;
int menu_level;
int date_level;

/* these are read/calculated from the card */
/* at start and after each major operation */
int bram_free;
int banks_in_use;
char bram_formatted;
char flash_formatted[16];


move2buf()
{
#asm
	SEI
	TMA	#2
	PHA
	LDA	_flashbank	; map flash write bank to memory $4000-$5FFF
	TAM	#2

	TII	$4000,_buffer,$0800   ; NOTE: for 4K sectors, choose between $4000 and $5000
	TII	$4800,_date,10        ; NOTE: for 4K sectors, choose between $4800 and $5800
	TII	$4810,_comment,64     ; NOTE: for 4K sectors, choose between $4810 and $5810
	
	PLA
	TAM	#2
	CLI
#endasm
}


bram2buf()
{
#asm
	SEI

	TMA	#2
	PHA
	LDA	#$F7	; map Backup RAM bank to memory $4000-$5FFF
	TAM	#2
	
	CSL
	LDA	#$48
	STA	$1807	; unlock BRAM (step 1)
	LDA	#$75
	STA	$1807	; unlock BRAM (step 2)
	LDA	#$80
	STA	$1807	; unlock BRAM
	TII	$4000,_buffer,$0800
	LDA	$1803	;lock BRAM
	CSH

	PLA
	TAM	#2
	CLI
#endasm
}

buf2bram()
{
#asm
	SEI

	TMA	#2
	PHA
	LDA	#$F7	; map Backup RAM bank to memory $4000-$5FFF
	TAM	#2
	
	CSL
	LDA	#$48
	STA	$1807	; unlock BRAM (step 1)
	LDA	#$75
	STA	$1807	; unlock BRAM (step 2)
	LDA	#$80
	STA	$1807	; unlock BRAM
	TII	_buffer,$4000,$0800
	LDA	$1803	;lock BRAM
	CSH

	PLA
	TAM	#2
	CLI
#endasm
}


/* hexadecimal display  - not currently used */
print_hex(off)
int off;
{
   for (i = 0; i < 16; i++)
   {
      put_hex(offset+(i*8), 3, 0, HEX_LINE+i);
      put_string(": ", 3, HEX_LINE+i);
      for (j = 0; j < 8; j++)
      {
         put_hex(buffer[offset+(i*8)+j], 2, 5+(j*2), HEX_LINE+i);
      }
      for (j = 0; j < 8; j++)
      {
         char1 = buffer[offset+(i*8)+j];
	 if (char1 < 0x20) char1 = '.';
	 if (char1 > 0x7f) char1 = '.';
         put_char(char1, 22+j, HEX_LINE+i);
      }
   }
}

flash_getid()
{
#asm
	 TII	flashid,_asmbuf,(flashidend-flashid)
	 JSR	_asmbuf		; get id
#endasm
}


flash_erasesec()
{
#asm
	 TII	erasesec,_asmbuf,(erasesecend-erasesec)
	 JSR	_asmbuf		; erase sector
#endasm
}

flash_write()
{
#asm
	 TII	flashwrite,_asmbuf,(flashwriteend-flashwrite)
	 JSR	_asmbuf		; write BRAM data
#endasm
}


bufint(offset)
int offset;
{
static int retval;

   retval = (buffer[offset+1]*256) + buffer[offset];

   return(retval);
}


is_formatted()
{
static int retval;

   if ((buffer[0] == 'H') && (buffer[1] == 'U') &&
       (buffer[2] == 'B') && (buffer[3] == 'M'))

      retval = 1;
   else
      retval = 0;

   return(retval);
}

clear_panel()
{
   set_font_pal(0);

   for (i = INSTRUCT_LINE; i < HEX_LINE + 17; i++)
   {
      put_string("                                        ", 0, i);
   }
}

buff_listing()
{
   vsync(2);

   clear_panel();

   if (is_formatted())
   {
      set_font_pal(4);
      put_string("Note: Only the first 16 entries", 1, INSTRUCT_LINE + 1);
      put_string("      are displayed", 1, INSTRUCT_LINE + 2);
      
      set_font_pal(5);
      put_string("File", 1, STAT_LINE + 2);
      put_string("----", 1, STAT_LINE + 3);

      put_string("Name", 7, STAT_LINE + 2);
      put_string("----------", 6, STAT_LINE + 3);

      put_string("Size", 18, STAT_LINE + 2);
      put_string("----", 18, STAT_LINE + 3);

      put_string("Free", 28, STAT_LINE + 2);
      put_string("----", 28, STAT_LINE + 3);
      set_font_pal(0);

      end_addr = bufint(4) - 0x8000;
      next_free = bufint(6) - 0x8000;
      offset_ptr = 0x10;

      put_number((0x800 - next_free), 4, 28, HEX_LINE);

      for (i = 0; i < 16; i++)
      {
         if (offset_ptr >= next_free)
         break;

         save_size = bufint(offset_ptr);
         for (j = 0; j < 10; j++)
         {
            name[j] = buffer[offset_ptr + 6 + j];
         }
         put_number(i+1, 2, 2, HEX_LINE + i);
         put_string(name, 6, HEX_LINE + i);
         put_number(save_size, 4, 18, HEX_LINE + i);

         offset_ptr += save_size;
      }
   }
   else
   {
      set_font_pal(3);
      put_string("NOT Formatted", 2, STAT_LINE + 2);
      set_font_pal(0);
   }
}

check_BRAM_status()
{
   banks_in_use = 0;

   for (i = 0; i < 10; i++) {
      card_date[i] = 0x00;
   }

   bram2buf();
   bram_formatted = is_formatted();

   if (bram_formatted)
      bram_free = 0x8800 - bufint(6);
   else
      bram_free = 0;

   for (i = 0; i < 32; i++)
   {
      flashbank = BASE_FLASH_BANK + i;
      move2buf();
      flash_formatted[i] = is_formatted();

      if (is_formatted()) {
         banks_in_use++;
      }

      if (is_formatted() && ((date[0] == '1') || (date[0] == '2')) )
      {
         if ((strcmp(card_date, date) < 0))
         {
            memcpy(card_date, date, 10);
         }
      }
   }
}

clear_errors()
{
   set_font_pal(0);
   put_string("                                       ", 2, INSTRUCT_LINE+1);
   put_string("                                       ", 2, INSTRUCT_LINE+2);
   put_string("                                       ", 2, INSTRUCT_LINE+3);
}

top_menu()
{
static int menu_selection;

   vsync(2);

   clear_panel();

   menu_selection = 1;

   set_font_pal(5);
   put_string("PC ENGINE:", 4, HEX_LINE+10);
   set_font_pal(0);

   if (bram_formatted)
   {
      set_font_pal(2);
      put_string("BRAM active", 6, HEX_LINE+11);
      put_number(bram_free, 4, 6, HEX_LINE+12);
      put_string("bytes free", 11, HEX_LINE+12);
      set_font_pal(0);
   }
   else
   {
      set_font_pal(2);
      put_string("BRAM not formatted", 6, HEX_LINE+11);
      set_font_pal(0);
   }

   set_font_pal(5);
   put_string("MEMORY CARD:", 4, HEX_LINE+14);
   set_font_pal(0);

   set_font_pal(2);
   put_string("Banks in use:", 6, HEX_LINE+15);
   put_number(banks_in_use, 2, 22, HEX_LINE+15);
   if (banks_in_use > 0)
   {
      put_string("Last save date: ", 6, HEX_LINE+16);
      put_string(card_date, 22, HEX_LINE+16);
   }
   set_font_pal(0);


   while(1)
   {
      advance = 1;

      clear_errors();

      if (menu_selection == 1)
         set_font_pal(1);
      else
         set_font_pal(0);

      put_string(" VIEW DATA ", 11, STAT_LINE + 4);

      if (menu_selection == 2) {
         if (!bram_formatted) {
            advance = 0;
	    set_font_pal(4);
	    put_string("Can't save unformatted BRAM!", 2, INSTRUCT_LINE+2);
	 }
         set_font_pal(1);
      }
      else
         set_font_pal(0);

      put_string(" SAVE TO CARD ", 11, STAT_LINE + 6);

      if (menu_selection == 3) {
         if (banks_in_use == 0) {
            advance = 0;
	    set_font_pal(3);
	    put_string("Cannot restore. No banks", 4, INSTRUCT_LINE+2);
            put_string("contain any backup data !", 4, INSTRUCT_LINE+3);
	 }
         set_font_pal(1);
      }
      else
         set_font_pal(0);

      put_string(" RESTORE FROM CARD ", 11, STAT_LINE + 8);

      pad = joytrg(0);

      if (pad & JOY_UP) {
         menu_selection--;
	 if (menu_selection == 0)
            menu_selection = 3;
      }

      if (pad & JOY_SLCT) {
         menu_A = -1;
	 break;
      }

      if (pad & JOY_DOWN) {
         menu_selection++;
	 if (menu_selection == 4)
            menu_selection = 1;
      }

      if ((advance == 1) &&
          ((pad & JOY_STRT) || (pad & JOY_A)) )
      {
         menu_A = menu_selection;
         break;
      }

      vsync(0);
   }
}

datestr_to_num(offset, len)
int offset;
int len;
{
static int retval;
static int count;
static int index;

   count = len;
   index = offset;
   retval = 0;
   
   while (count > 0) {
      retval = (retval * 10) + (date[index] - '0');
      index++;
      count--;
   }

   return(retval);
}

num_to_datestr(value, offset, len)
int value;
int offset;
int len;
{
static int count;
static int remainder;
static int tempval;

   count = len;
   remainder = value;

   while (count > 0) {
      tempval = remainder % 10;
      date[offset+count-1] = tempval + '0';
      remainder = (remainder - tempval) / 10;
      count--;
   }
}


normalize_date()
{
   if ((month != 2) && (day > days_in_month[month-1]))
   {
      day = days_in_month[month-1];
   }
   else if ((month == 2) && (day > 28))
   {
      if ((year % 4) != 0) {
         day = 28;
      }
      else if (year == 2100)
         day = 28;
      else
         day = 29;
   }
}


get_date()
{
static char refresh;
static char disp_str[11];

   vsync(2);

   clear_panel();

   /* If we haven't set the date yet during this runtime, */
   /* set it to max found on card */

   if ((today_date[0] != '1') && (today_date[0] != '2'))
   {
      strncpy(today_date, card_date, 10);
   }

   /* If the date is still empty, set to card release date */
   /* as a default/fall-through */

   if ((today_date[0] != '1') && (today_date[0] != '2'))
   {
      strncpy(today_date, default_date, 10);
   }

   strncpy(date, today_date, 10);
   
   set_font_pal(5);
   put_string("Please enter today's date", 6, HEX_LINE);
   put_string("   for filing purposes", 6, HEX_LINE+2);
   set_font_pal(0);

   year  = datestr_to_num(0,4);
   month = datestr_to_num(5,2);
   day   = datestr_to_num(8,2);
   
   refresh = 1;
   date_level = 1;

   while(1)
   {
      if (refresh)
      {
         num_to_datestr(year, 0,4);
         num_to_datestr(month,5,2);
         num_to_datestr(day,  8,2);

         if (date_level == 1)
            set_font_pal(1);
         else
            set_font_pal(0);

         put_string(" ", 11, HEX_LINE+6);
	 strncpy(disp_str, date, 4);
	 disp_str[4] = 0;
         put_string(disp_str, 12, HEX_LINE+6);
         put_string(" ", 16, HEX_LINE+6);

         set_font_pal(0);

	 put_string("-", 17, HEX_LINE+6);

         if (date_level == 2)
            set_font_pal(1);
         else
            set_font_pal(0);

	 put_string(" ", 18, HEX_LINE+6);
	 strncpy(disp_str, &date[5], 2);
	 disp_str[2] = 0;
         put_string(disp_str, 19, HEX_LINE+6);
	 put_string(" ", 21, HEX_LINE+6);

         set_font_pal(0);

	 put_string("-", 22, HEX_LINE+6);

         if (date_level == 3)
            set_font_pal(1);
         else
            set_font_pal(0);

	 put_string(" ", 23, HEX_LINE+6);
	 strncpy(disp_str, &date[8], 2);
	 disp_str[2] = 0;
         put_string(disp_str, 24, HEX_LINE+6);
	 put_string(" ", 26, HEX_LINE+6);

         set_font_pal(0);
	 refresh = 0;
      }

      pad = joytrg(0);

      if (pad & JOY_LEFT) {
         date_level--;
	 if (date_level < 1)
            date_level = 3;

	 /* if (date_level == 3) adjust day */
	 if (date_level == 3)
            normalize_date();

	 refresh = 1;
      }

      if (pad & JOY_RGHT) {
         date_level++;
	 if (date_level > 3)
            date_level = 1;

	 /* if (date_level == 3) adjust day */
	 if (date_level == 3)
            normalize_date();

	 refresh = 1;
      }

      if (pad & JOY_UP) {
         if (date_level == 1) {
	    if (year < 2100) year++;
	 }
	 else if (date_level == 2) {
             if (month < 12) month++;
	 }
	 else if (date_level == 3) {
            day++;
	    normalize_date();
	 }

	 refresh = 1;
      }

      if (pad & JOY_DOWN) {
         if (date_level == 1) {
	    if (year > 1986) year--;
	 }
	 else if (date_level == 2) {
	     if (month > 1) month--;
	 }
	 else if (date_level == 3) {
	     if (day > 1) day--;
	 }

	 refresh = 1;
      }

      if (pad & JOY_B) {
         if (date_level == 1)
	 {
            date_level = -1;
	    strncpy(today_date, date, 10);
	    break;
	 }
	 else
            date_level--;

	 refresh = 1;
      }

      if ((pad & JOY_STRT) || (pad & JOY_A)) {
         if (date_level == 3)
	 {
	    strncpy(today_date, date, 10);
            break;
	 }
	 else
            date_level++;

	 refresh = 1;
      }

      vsync(0);
   }
}

get_comment()
{
static char refresh;
static char current_letter;
static char disp_str[11];
static int x_pos;
static int y_pos;
static int comment_index;

   vsync(2);

   clear_panel();

   x_pos = 0;
   y_pos = 0;

   strcpy(today_comment, "               ");
   today_comment[15] = 0x00;
   comment_index = 0;

   put_string(">> _______________ <<", 9, HEX_LINE);

   refresh = 1;

   while (1)
   {
      if (refresh)
      {
         put_string(today_comment, 12, HEX_LINE);
	 
	 set_font_pal(1);
	 put_char(today_comment[comment_index], comment_index+12, HEX_LINE);
	 set_font_pal(0);

         for (i = 0; i < 6; i++)
	 {
            for (j = 0; j < 13; j++)
	    {
               if ((i == y_pos) && (j == x_pos))
                  set_font_pal(1);
	       else
                  set_font_pal(0);


               current_letter = letter_display[(i*13)+j];

	       if ((i == 5) && (j > 6)) {
                  switch(j) {
                     case 8:
                        put_string(" SPC ", 23, HEX_LINE+15);
                        break;
                     case 10:
                        put_string(" BCK ", 29, HEX_LINE+15);
                        break;
                     case 12:
                        put_string(" END ", 35, HEX_LINE+15);
                        break;
		  }
	       }
	       else
	       {
                  put_string(" ", (j*3), HEX_LINE+(i*2)+5);
                  put_char(current_letter, (j*3)+1, HEX_LINE+(i*2)+5);
                  put_string(" ", (j*3)+2, HEX_LINE+(i*2)+5);
	       }
               set_font_pal(0);
	    }
	 }
	 refresh = 0;
      }

      pad = joytrg(0);
      
      if (pad & JOY_LEFT)
      {
	 if (x_pos == 0)
            x_pos = 12;
	 else
            x_pos--;

	 if ((y_pos == 5) && (x_pos > 6) && ((x_pos % 2) == 1) )  
            x_pos--;

	 refresh = 1;
      }

      if (pad & JOY_RGHT)
      {
	 if (x_pos == 12)
            x_pos = 0;
	 else
            x_pos++;

	 if ((y_pos == 5) && (x_pos > 6) && ((x_pos % 2) == 1) )  
            x_pos++;

	 refresh = 1;
      }

      if (pad & JOY_UP)
      {
	 if (y_pos == 0)
            y_pos = 5;
	 else
            y_pos--;

	 if (y_pos == 5) {
            if ((x_pos > 6) && ((x_pos % 2) == 1))
               x_pos--;
	 }

	 refresh = 1;
      }

      if (pad & JOY_DOWN)
      {
	 if (y_pos == 5)
            y_pos = 0;
	 else
            y_pos++;

	 if (y_pos == 5) {
            if ((x_pos > 6) && ((x_pos % 2) == 1))
               x_pos--;
	 }

	 refresh = 1;
      }

      if (pad & JOY_B) {
         date_level = -1;
         strncpy(comment, today_comment, 15);
	 break;
      }

      if (pad & JOY_A) {
         if ((y_pos == 5) && (x_pos == 12))   /* END */
	 {
            strncpy(comment, today_comment, 15);
            break;
	 }
	 else if ((y_pos == 5) && (x_pos == 10))  /* Backspace */
	 {
	    today_comment[comment_index] = ' ';
            if (comment_index > 0) {
               comment_index--;
	    }

	 }
	 else if ((y_pos == 5) && (x_pos == 8))  /* Space */
	 {
	    today_comment[comment_index++] = ' ';
	    if (comment_index == 15)
               comment_index = 14;
	 }
	 else  /* everything else */
	 {
	    today_comment[comment_index++] = letter_display[(y_pos*13)+x_pos];
	    if (comment_index == 15)
               comment_index = 14;
	 }

	 refresh = 1;
      }

      vsync(0);
   }
}


select_bank_menu()
{
static int menu_selection;
static char bottom_limit;
static char refresh;

   vsync(2);

   clear_panel();
      
   set_font_pal(5);
   put_string("Bank", 1, HEX_LINE-2);
   put_string("----", 1, HEX_LINE-1);
   put_string("Save Date", 6, HEX_LINE-2);
   put_string("----------", 6, HEX_LINE-1);
   put_string("Free", 18, HEX_LINE-2);
   put_string("----", 18, HEX_LINE-1);
   put_string("Name", 25, HEX_LINE-2);
   put_string("---------------", 24, HEX_LINE-1);
   set_font_pal(0);

   if (menu_A == 1)        /* view */
   {
      menu_selection = 0;  /* BRAM is eligible for selection */
      bottom_limit = 0;
   }
   else if (menu_A == 2)   /* save - date and comment should  */
   {                       /*        have been entered by now */
      menu_selection = 1;  /* BRAM is not eligible for selection */
      bottom_limit = 1;
      set_font_pal(5);
      put_string(">> Select a bank to SAVE to <<", 5, INSTRUCT_LINE+1);
      set_font_pal(3);
      put_string("SAVE", 25, INSTRUCT_LINE+1);
      set_font_pal(0);
   }
   else if (menu_A == 3)   /* restore */
   {
      menu_selection = 1;  /* BRAM is not eligible for selection */
      bottom_limit = 1;
   }
   refresh = 1;



   while(1)
   {
      if (refresh)     /* don't display everything every cycle */
      {
         advance = 1;  /* unless otherwise stated, allow moving to next screen */

	 page = (menu_selection-1) / 16;

	 if (menu_A != 2)
            clear_errors();

         if (menu_selection == 0)
            set_font_pal(1);
          else
            set_font_pal(0);

         if (bram_formatted) {
            put_string("BRAM             ", 1, HEX_LINE);
            put_number(bram_free, 4, 18, HEX_LINE);
            put_string("                 ", 22, HEX_LINE);
         }
         else
         {
            if (menu_selection == 0) /* if unformatted and current selection, */ 
            {                        /* disallow moving to next screen */
               advance = 0;
               set_font_pal(3);
               put_string("No contents to view.", 5, INSTRUCT_LINE+1);
	       set_font_pal(1);
	    }
            else
               set_font_pal(2);

            put_string("BRAM Unused                           ", 1, HEX_LINE);
            set_font_pal(0);
         }
         set_font_pal(0);


	 /* note that menu selection of banks is 1-relative, */
	 /* but flash index is 0-relative */

         for (i = 0; i < 16; i++)
         {
	    if (menu_selection == ((page*16)+i+1) )
               set_font_pal(1);
	    else
               set_font_pal(0);

            put_string(" ", 1, HEX_LINE+1+i);
            put_number((page*16)+i+1, 2, 2, HEX_LINE+1+i);
            put_string("  ", 4, HEX_LINE+1+i);

            flashbank = BASE_FLASH_BANK + (page*16)+i;
            move2buf();

            if (is_formatted()) {
               put_number((0x8800 - bufint(6)), 4, 18, HEX_LINE+1+i);
               put_string("  ", 22, HEX_LINE+1+i);
               memcpy(comment_buf, comment, 16);
               comment_buf[15] = '\0';
               put_string(comment_buf, 24, HEX_LINE+1+i);

            }
            else
            {
	       /* no contents */
               if (menu_selection != ((page*16)+i+1))
               {
                  set_font_pal(2);
                  put_string("      Not In Use     ", 18, HEX_LINE+1+i);
                  set_font_pal(0);
               }
	       else
               {
                  put_string("      Not In Use     ", 18, HEX_LINE+1+i);

		  if (menu_A == 1)
                  {
                     advance = 0;
                     set_font_pal(3);
                     put_string("No contents to view.", 5, INSTRUCT_LINE+1);
	             set_font_pal(1);
		  }
		  else if (menu_A == 3)
                  {
                     advance = 0;
                     set_font_pal(3);
                     put_string("No contents to restore.", 5, INSTRUCT_LINE+1);
	             set_font_pal(1);
		  }
               }
            }

            if ((date[0] != '1') && (date[0] != '2')) { /* i.e. year = 19xx or 20xx */
               /* no date set */
               if (menu_selection != ((page*16)+i+1))
               {
                  set_font_pal(2);
                  put_string("Not Set     ", 6, HEX_LINE+1+i);
                  set_font_pal(0);
               }
	       else
                  put_string("Not Set     ", 6, HEX_LINE+1+i);
            }
            else {
               put_string(date, 6, HEX_LINE+1+i);
               put_string("  ", 16, HEX_LINE+1+i);
            }

	    set_font_pal(0);
         }
	 refresh = 0;
      }

      pad = joytrg(0);

      if (pad & JOY_UP) {
         menu_selection--;
	 if (menu_selection < bottom_limit)
            menu_selection = 32;

	 refresh = 1;
      }

      if (pad & JOY_DOWN) {
         menu_selection++;
	 if (menu_selection > 32)
            menu_selection = bottom_limit;

	 refresh = 1;
      }

      if (pad & JOY_LEFT) {
         if (menu_selection == 0) {
            menu_selection = 17;
         }
	 else if (menu_selection > 16) {
             menu_selection -= 16;
	 }
	 else {
             menu_selection += 16;
	 }

	 refresh = 1;
      }

      if (pad & JOY_RGHT) {
         if (menu_selection == 0) {
            menu_selection = 17;
         }
	 else if (menu_selection > 16) {
             menu_selection -= 16;
	 }
	 else {
             menu_selection += 16;
	 }

	 refresh = 1;
      }

      if ((advance == 1) &&
          ((pad & JOY_STRT) || (pad & JOY_A)) )
      {
         menu_B = menu_selection;
	 break;
      }

      if (pad & JOY_B)
      {
         menu_B = -1;
	 break;
      }

      vsync(0);
   }
}


confirm_menu()
{
static int confirm_value;

   confirm_value = 0;

   vsync(2);

   clear_panel();

   if (menu_A == 2)
   {
      set_font_pal(4);
      put_string("Confirm ", 13, HEX_LINE+1);
      set_font_pal(3);
      put_string("SAVE", 21, HEX_LINE+1);
      set_font_pal(4);
      put_string("from Backup Memory", 10, HEX_LINE+3);
      put_string("to BANK #", 13, HEX_LINE+5);
      put_number(menu_B, 2, 22, HEX_LINE+5);
      put_string(" ? ", 24, HEX_LINE+5);
   }
   else if (menu_A == 3)
   {
      set_font_pal(4);
      put_string("Confirm ",11, HEX_LINE+1);
      set_font_pal(3);
      put_string("RESTORE", 19, HEX_LINE+1);
      set_font_pal(4);
      put_string("from BANK #",12, HEX_LINE+3);
      put_number(menu_B, 2, 23, HEX_LINE+3);
      put_string("to Backup Memory ?",10, HEX_LINE+5);
   }
   set_font_pal(0);
   
   while (1)
   {
      if (confirm_value == 1)
         set_font_pal(1);

      put_string(" YES ", 13, HEX_LINE+9);
      set_font_pal(0);

      put_string(" / ",   18, HEX_LINE+9);
      
      if (confirm_value == 0)
         set_font_pal(1);

      put_string(" NO ", 21, HEX_LINE+9);
      set_font_pal(0);

      pad = joytrg(0);

      if (pad & JOY_LEFT)
      {
         confirm_value = (confirm_value == 0) ? 1 : 0;
      }

      if (pad & JOY_RGHT)
      {
         confirm_value = (confirm_value == 0) ? 1 : 0;
      }


      if (pad & JOY_B)
      {
         confirm = 0;
	 break;
      }

      if ((pad & JOY_STRT)|| (pad & JOY_A))
      {
         confirm = confirm_value;
	 break;
      }

      vsync(0);
   }
}


credits()
{
   vsync(2);

   clear_panel();

   put_string("Megavault stores and manages your", 2, HEX_LINE-1);
   put_string("PC Engine game save data.", 2, HEX_LINE);

   put_string("Using modern Flash memory, you can", 2, HEX_LINE+2);
   put_string("now save and index up to 32 backup", 2, HEX_LINE+3);
   put_string("memory compartments for future use.", 2, HEX_LINE+4);
   
   put_string("This card is a proof-of-concept", 2, HEX_LINE+6);
   put_string("and future versions may have more", 2, HEX_LINE+7);
   put_string("capabilities.", 2, HEX_LINE+8);
   
   put_string("(c) 2019 by David Shadoff", 8, HEX_LINE+13);

   while (1)
   {
      pad = joytrg(0);

      if ((pad & JOY_STRT)|| (pad & JOY_A))
      {
	 break;
      }

      vsync(0);
   }
}



main()
{
   /* set up palette */
   /* Font uses sub-palette #1 for FG, #2 for BG */

   /* palette #0 is default - light green background, bright white foreground */
   set_color_rgb(0, 0,2,0);
   set_color_rgb(1, 7,7,7);
   set_color_rgb(2, 0,2,0);
   
   /* palette #1 is selection/inverse - bright white background, light green foreground */
   set_color_rgb(16, 7,7,7);
   set_color_rgb(17, 0,2,0);
   set_color_rgb(18, 7,7,7);

   /* palette #2 is disabled/dimmed - light green background, dimmed white foreground */
   set_color_rgb(32, 0,2,0);
   set_color_rgb(33, 4,4,4);
   set_color_rgb(34, 0,2,0);

   /* palette #3 is error/red - light green background, bright red foreground */
   set_color_rgb(48, 0,2,0);
   set_color_rgb(49, 7,0,0);
   set_color_rgb(50, 0,2,0);

   /* palette #4 is highlight/yellow - light green background, bright yellow foreground */
   set_color_rgb(64, 0,2,0);
   set_color_rgb(65, 7,7,0);
   set_color_rgb(66, 0,2,0);

   /* palette #5 is highlight/blue-green - light green background, blue-green foreground */
   set_color_rgb(80, 0,2,0);
   set_color_rgb(81, 0,6,7);
   set_color_rgb(82, 0,2,0);

   /* need to reload font with non-zero background color */
   /* (it is preloaded as BG=0 by the bootup process, and 0 is transparent in most palettes) */
   set_font_color(1,2);
   set_font_pal(0);

   /* kana isn't in HuC by default - need to explicitly load */
   load_font(kana_font,128,0xE00);

   /* reload the basic font, same location but different colors */
   set_font_addr(0x800);
   load_default_font();

   /* default values */
   comment_buf[15] = 0;
   name[10] = 0;
   id_a = 0;
   id_b = 0;
   flashbank = BASE_FLASH_BANK;

   set_xres(320);

   /* title of page */
   strcpy(default_date, "2019-10-19");
   strcpy(comment, "               ");
   strcpy(today_comment, "               ");


   set_font_pal(4);
   put_string("Multi-Backup Megavault", 8, TITLE_LINE);
   put_string("v0.99", 34, TITLE_LINE);
   set_font_pal(0);


   /* check whether flash identifies itself as a flash chip which */
   /* works with the programming sequences in use in this program */

   flash_getid();
   if ((id_a != 0xBF) || (id_b != 0xB7))
   {
      put_string("THIS IS NOT BEING RUN", 9, INSTRUCT_LINE + 6);
      put_string("ON ORIGINAL MEDIA !!!", 9, INSTRUCT_LINE + 8);
      put_string("*** ABORT *** ", 13, INSTRUCT_LINE + 12);
      while(1);
   }
   
   menu_level = 1;


   /* determine whether each bank is actually in use, and */
   /* most recent date of save on card */

   while (1)
   {
      check_BRAM_status();

      if (menu_level == 1)
      {
         top_menu();

	 if (menu_A == -1)
	 {
            credits();
            menu_level = 1;
            continue;
	 }
	 else if (menu_A == 2)         /* save - get date, comment */
         {
            /* Get date information */
            get_date();

	    if (date_level == -1)
	    {
               menu_level = 1;
	       continue;
	    }

            /* Get comment information */
            get_comment();

	    if (date_level == -1)
	    {
               menu_level = 1;
	       continue;
	    }
         }
	 menu_level = 2;
      }

      if (menu_level == 2)
      {
         select_bank_menu();

         if (menu_B == -1)
	 {
            menu_level = 1;
            continue;
	 }
	 else
	    menu_level = 3;
      }

      if (menu_level == 3)
      {
         if (menu_A == 1)         /* view */
         {
            if (menu_B == 0)
	    {
               bram2buf();
	    }
	    else
	    {
	       flashbank = BASE_FLASH_BANK + menu_B - 1;
	       move2buf();
	    }

	    buff_listing();

            while(1)
            {
               pad = joytrg(0);

               if ((pad & JOY_STRT) || (pad & JOY_B))
               {
                  menu_level = 2;
                  break;
               }
	    }
         }
         else if (menu_A == 2)    /* save */
         {
            /* need to confirm commit */
            confirm_menu();

	    if (confirm == 0)
	    {
               menu_level = 2;
	       continue;
	    }
	    else
	    {
               strncpy(date, today_date, 10);
               strncpy(comment, today_comment, 15);

	       flashbank = BASE_FLASH_BANK + menu_B - 1;
	       flash_erasesec();
	       flash_write();
	       move2buf();

	       menu_level = 1;
	    }
         }
         else if (menu_A == 3)    /* restore */
         {
            /* need to confirm commit */
            confirm_menu();

	    if (confirm == 0)
	    {
               menu_level = 2;
	       continue;
	    }
	    else
	    {
	       flashbank = BASE_FLASH_BANK + menu_B - 1;
	       move2buf();
	       buf2bram();

	       menu_level = 1;
	    }
         }
      }
   }

   put_string("oops - fatal error", 1, TITLE_LINE);

   while(1);

   return;
}
