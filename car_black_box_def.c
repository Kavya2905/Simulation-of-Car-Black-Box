
#include "main.h"
#include "string.h"
#include "clcd.h"
unsigned char clock_reg[3];

char time[7]; //HHMMSS
char log[11];
int pos = -1; ///
int event_count = 0; //event count is 1 by default
extern unsigned char sec;
extern unsigned char return_time;
char *menu[] = {"View Log", "Clear Log", "Change Pass"};

void get_time(void){
    clock_reg[0] = read_ds1307(HOUR_ADDR); // HH -> BCD 
    clock_reg[1] = read_ds1307(MIN_ADDR); // MM -> BCD 
    clock_reg[2] = read_ds1307(SEC_ADDR); // SS -> BCD 
    
    //HH -> BCD -> ascii
    time[0] = ((clock_reg[0] >> 4) & 0x03) + '0';;
    time[1] = (clock_reg[0] & 0x0F) + '0';
    //MM ->BCD -> ascii
    time[2] = ((clock_reg[1] >> 4) & 0x07) + '0';
    time[3] = (clock_reg[1] & 0x0F) + '0';
    //SS-> BCD -> ascii
    time[4] = ((clock_reg[2] >> 4) & 0x07) + '0';
    time[5] = (clock_reg[2] & 0x0F) + '0';
    
    time[6] = '\0';
}

void display_time(void){
    get_time();
    //HH:
    clcd_putch(time[0], LINE2(2)); 
    clcd_putch(time[1], LINE2(3)); 
    clcd_putch(':', LINE2(4)); 
    //MM:
    clcd_putch(time[2], LINE2(5)); 
    clcd_putch(time[3], LINE2(6)); 
    clcd_putch(':', LINE2(7)); 
    //SS
    clcd_putch(time[4], LINE2(8)); 
    clcd_putch(time[5], LINE2(9)); 
   
}

void display_dash_board(char event[],unsigned char speed){
    clcd_print("  TIME     E  SP", LINE1(0));
    
    //Display time
    display_time();
    
    //Display event
    clcd_print(event, LINE2(11));
    
    //Display speed
    //speed 0->99
    clcd_putch((speed/10) + '0', LINE2(14)); //'0' to get ascii value
    clcd_putch((speed%10) + '0', LINE2(15));
    
}

void log_event(){
    unsigned char add;
    add = 5;
    
    pos++;
    if(pos == 10){
        pos = 0;
    }
    add = pos*10 + 5;
    for(int i =0; log[i] != '\0'; i++){
        eeprom_write(add, log[i]);
        add++;
    }
    if(event_count < 9){
        event_count++;
    }
}

void log_car_event(char event[], unsigned char speed){
    
    // log time
    
    get_time(); //HHMMSS -> 6 bytes
    strncpy(log, time, 6);  //strncpy(dest, source, no. of bytes
    strncpy(&log[6], event, 2);
    log[8] = (speed/10) + '0';
    log[9] = (speed%10) + '0';
    
    log[10] = '\0';
    
    log_event();
           
}

void clear_screen(void){
    clcd_write(CLEAR_DISP_SCREEN, INST_MODE);
    __delay_us(100);
}

char login(unsigned char key, unsigned char reset_flag){
    static char npassword[4]; //"storing password
    unsigned char attempt_rem, i;
    char spassword[4];
    
    if(reset_flag == RESET_PASSWORD){
        return_time = 5;
        attempt_rem = '3';
        i = 0;
        npassword[0] = '\0';
        npassword[1] = '\0';
        npassword[2] = '\0';
        npassword[3] = '\0';
        key = ALL_RELEASED;
    }
    if(return_time == 0){
        return RETURN_BACK;
    }
    if(key==SW4 && i<4){   //SW4->'4'
        npassword[i] = '4';
        clcd_putch('*', LINE2(6 + i));
        i++;
        return_time = 5;
    }
    else if(key==SW5 && i<4){ //SW4->'2'
        npassword[i] = '2';
        clcd_putch('*', LINE2(6 + i));
        i++;
        return_time = 5;
    }
    //User entered 4 digits
    if(i == 4){
        // npassword -> 4 digit
        // Compare the password
        for(unsigned char j = 0; j<4; j++){  ///
            spassword[j] = eeprom_read(j); //'2', '4, '2', '4'
        }
    
        if(strncmp(npassword, spassword, 4) == 0){//Correct password{
            //Goes to menu screen
            return TASK_SUCCESS;
        }
        else{ //Wrong password 
            //Wrong password comment
            attempt_rem--;
            if(attempt_rem == '0'){
                clcd_write(DISP_ON_AND_CURSOR_OFF, INST_MODE);
                __delay_us(100);
                clcd_print(" You are blocked", LINE1(0));
                clcd_print(" Wait..for 60sec", LINE2(0));
                sec = 60;
                while(sec != 0){
                    clcd_putch((sec/10)+'0', LINE2(11));
                    clcd_putch((sec%10)+'0', LINE2(12));
                }
                attempt_rem = '3';
            }


            else{
                
                clear_screen();
                    //Prompt wrong message 3 sec
                clcd_print(" WRONG PASSWORD ", LINE1(0));
                clcd_putch(attempt_rem, LINE2(0));
                clcd_print(" attempt remain ", LINE2(1));
                __delay_ms(3000); //3000ms = 3s
            }
            clear_screen();
            clcd_print(" ENTER  PASSWORD ", LINE1(0));
            clcd_write(CURSOR_POS, INST_MODE);
            clcd_write(DISP_ON_AND_CURSOR_ON, INST_MODE);
            __delay_us(100);
            i = 0;
            return_time = 5;
        }
    }
    return 0x10;
}

// char *menu[] = {"View Log", "Clear Log", "Change Password"};
char login_menu(unsigned char key, unsigned char reset_flag){
    static char menu_pos;
    if(reset_flag == RESET_LOGIN_MENU){
        menu_pos = 0;
    }
    if(key == SW5 && menu_pos < 2){ //Down Key
        menu_pos++;
        clear_screen();
    }
    else if(key == SW4 && menu_pos > 0){
        menu_pos--;
        clear_screen();
    }
    if(menu_pos < 2){
        
        clcd_putch('>', LINE1(0));
        clcd_print(menu[menu_pos], LINE1(2));
        clcd_print(menu[menu_pos+1], LINE2(2));
        
    }
    else if(menu_pos == 2){
        clcd_print(menu[menu_pos - 1], LINE1(2));
        clcd_print(menu[menu_pos], LINE2(2));
        clcd_putch('>', LINE2(0));
    
    }
    
    return menu_pos;
    
}

void view_log(unsigned char key, unsigned char reset_flag){
    char rlog[11];
    unsigned char add;
    static unsigned char rpos;
    if(event_count == -1){ //No log
        clcd_print(" No logs ", LINE2(0));
        
    }
    else{ //Event there
        if(reset_flag == VIEW_LOG_RESET){
        rpos = 0;
        }
        if(key == SW5 && rpos < (event_count - 1)){
            rpos++;
        }
        else if(key == SW4 && rpos>0){
            rpos--;
        }
        for(int i = 0; i<10; i++){
            add= rpos*10 + 5;
            rlog[i] = eeprom_read(add + i);
        }
        //rpos -> "HHMMSSEESP"
        //Display
        clcd_putch(rpos%10 + '0', LINE2(0));
        //HH:
        clcd_putch(rlog[0],LINE2(2));
        clcd_putch(rlog[1],LINE2(3));
        clcd_putch(':',LINE2(4));
        //MM:
        clcd_putch(rlog[2],LINE2(5));
        clcd_putch(rlog[3],LINE2(6));
        clcd_putch(':',LINE2(7));
        //SS
        clcd_putch(rlog[4],LINE2(8));
        clcd_putch(rlog[5],LINE2(9));

        //Event
        clcd_putch(rlog[6],LINE2(11));
        clcd_putch(rlog[7],LINE2(12));

        //Speed
        clcd_putch(rlog[8],LINE2(14));
        clcd_putch(rlog[9],LINE2(15));


    
    }
        
 }
    
char clear_log(unsigned char reset_flag){
    if(reset_flag == RESET_MEMORY){
        clcd_print(" Logs Cleared ", LINE1(0));
        clcd_print(" Successfully ", LINE2(0));
        
        event_count = -1; //No event
        pos = -1; //for new add
        __delay_ms(5000);
        clear_screen();
        return TASK_SUCCESS;
    }
    return TASK_FAIL;
}
char change_password(unsigned char key, unsigned char reset_flag){
    
    static char pwd[9]; //Single array for both enter and re enter
    static int pos, once;
    if(reset_flag == RESET_CHANGE_PASSWORD){
        pos = 0;
        once=1;
    }
    if(pos < 4 && once){
        once = 0;
        clcd_print("Enter new pass", LINE1(0));
        clcd_write(LINE2(0), INST_MODE);
        clcd_write(DISP_ON_AND_CURSOR_ON, INST_MODE);
    }
    else if(pos >= 4 &&(once == 0)){
        once = 1;
        clear_screen();
        clcd_print("Re-enter pass", LINE1(0));
        clcd_write(LINE2(0), INST_MODE);
        clcd_write(DISP_ON_AND_CURSOR_ON, INST_MODE); 
        
    }
    if(key == SW4){ // '4'
        pwd[pos] = '4';
        clcd_putch('*', LINE2(pos % 4));
        pos++;
        
    }
    else if(key == SW5){ //'5'
         pwd[pos] = '2';
         clcd_putch('*', LINE2(pos % 4));
         pos++;
    }
    if(pos == 8){
        if(strncmp(pwd, &pwd[4], 4) == 0){//Correct password{
            //Matching pass so update
            for(int i = 0; i < 4; i++){
                eeprom_write(i, pwd[i]);
            }
            clear_screen();
            clcd_write(" Pass Change ", LINE1(0));
            clcd_print(" Successfully ", LINE2(0));
            __delay_ms(3000);
            return TASK_SUCCESS;
        }
        else{
            clear_screen();
            clcd_write("Pass Change ", LINE1(0));
            clcd_print(" Failed ", LINE2(0));
            __delay_ms(3000);
            return TASK_SUCCESS;
        }
    }
    return 0x10;
}