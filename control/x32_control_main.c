#include <stdio.h>
#include <stdlib.h>

#include "x32_common.h"
#include "comm.h"
#include "x32_modes.h"
#include "kalman.h"

/*int kb_lift, kb_yaw, kb_pitch, kb_roll;*/
/*int js_lift, js_yaw, js_pitch, js_roll;*/
int lift, roll, pitch, yaw;
int oldoo1, oldoo2, oldoo3, oldoo4;
int lift_error, roll_error, pitch_error, yaw_error;
comm_type mode, type;
volatile int finished,terminate;

int flag_data_loggin;
int phi_kalman_roll_loggin, p_kalman_roll_loggin, s0_loggin, s3_loggin, roll_loggin;

unsigned int time_last_packet;
unsigned int isr_qr_time;
unsigned int start_time;

int oo1, oo2, oo3, oo4;
int s0, s1, s2, s3, s4, s5;//, timestamp;
int s0_bias, s1_bias, s2_bias, s3_bias, s4_bias, s5_bias;
unsigned int p_yaw;
unsigned int p1_full, p2_full;

unsigned char* data;
int len;

int main(void) {

	init_state();
	 
	SET_INTERRUPT_VECTOR(INTERRUPT_XUFO, &isr_qr_link);
	SET_INTERRUPT_PRIORITY(INTERRUPT_XUFO, 21);

	// prepare timer 1 interrupt for 1000 Hz engine control (i.e., 1 ms)
	X32_timer1_per = 1 * CLOCKS_PER_MS;         
	SET_INTERRUPT_VECTOR(INTERRUPT_TIMER1, &isr_timer1);
	SET_INTERRUPT_PRIORITY(INTERRUPT_TIMER1, 30);
	
	// prepare timer 2 interrupt for 10 Hz for checking pc link  (i.e., 100 ms)
	X32_timer2_per = 100 * CLOCKS_PER_MS;
	SET_INTERRUPT_VECTOR(INTERRUPT_TIMER2, &isr_timer2);
	SET_INTERRUPT_PRIORITY(INTERRUPT_TIMER2, 30);
	
	SET_INTERRUPT_VECTOR(INTERRUPT_OVERFLOW, &isr_overflow);
	SET_INTERRUPT_PRIORITY(INTERRUPT_OVERFLOW, 40);
	
	SET_INTERRUPT_VECTOR(INTERRUPT_DIVISION_BY_ZERO, &isr_division_by_zero);
	SET_INTERRUPT_PRIORITY(INTERRUPT_DIVISION_BY_ZERO, 40);
		
	SET_INTERRUPT_VECTOR(INTERRUPT_OUT_OF_MEMORY, &isr_out_of_memory);
	SET_INTERRUPT_PRIORITY(INTERRUPT_OUT_OF_MEMORY, 40);
	
	SET_INTERRUPT_VECTOR(INTERRUPT_TRAP, &isr_trap);
	SET_INTERRUPT_PRIORITY(INTERRUPT_TRAP, 40);

	
	//Initialise communication
	if (0 != comm_init())
		return -1;

	ENABLE_INTERRUPT(INTERRUPT_XUFO);
	ENABLE_INTERRUPT(INTERRUPT_TIMER1);
	ENABLE_INTERRUPT(INTERRUPT_TIMER2);
	ENABLE_INTERRUPT(INTERRUPT_OVERFLOW);
	ENABLE_INTERRUPT(INTERRUPT_DIVISION_BY_ZERO);
	ENABLE_INTERRUPT(INTERRUPT_OUT_OF_MEMORY);
	ENABLE_INTERRUPT(INTERRUPT_TRAP);
	ENABLE_INTERRUPT(INTERRUPT_GLOBAL);

	terminate = 0;
	finished = 0;
	

	//Wait until data can be received
	while (!finished)
	{	
		if ( recv_data(&type, &data, &len) == 1) {
			time_last_packet = X32_ms_clock;
			handleInput();
			free(data);
		}
		if (flag_data_loggin == 1){
			//printf("%d\t%d\t%d\t%d\t%d\n",phi_kalman_roll_loggin, p_kalman_roll_loggin, s0_loggin, s3_loggin, roll_loggin);
			//printf("%d\t%d\t%d\t%d\n\n",oo1, oo2, oo3, oo4);
			//printf("%d\t%d\t%d\t%d\t%d\t%d\n",s0, s1, s2, s3, s4, s5);
			flag_data_loggin=0;
		}
	}
	
	if (0 != send_data(KEYESC, 0, 0))
	{
		printf("Error sending ESC\n");		
	}
	//Uninitialise
	X32_leds=0;
	X32_display=0;
	comm_uninit();
	return 0;
}

void init_state(void)
{
	lift = roll = pitch = yaw = 0;
	oo1 = oo2 = oo3 = oo4 = 0;
	p_yaw = INITIAL_P_YAW;
	p1_full = INITIAL_P1_FULL;
	p2_full = INITIAL_P2_FULL;		
	s0 = s1 = s2 = s3 = s4 = s5 = 0;
	s0_bias = s1_bias = s2_bias = s3_bias = s4_bias = s5_bias = 0;
	mode = INITIAL_MODE; //starts in INITIAL_STATE mode
	init_kalman();
	time_last_packet=0;
	isr_qr_time=0;
	flag_data_loggin=0;
	calibration_mode();
}

void handleInput (void) {
	if (type == KEY1) {
		mode = PANIC;
		printf("Changing to panic mode\n");
		return;
	}
	
	if (type == RPYL) {
		roll = data[0] - 64;
		pitch = data[1] - 64;
		yaw = data[2] - 64;
		lift = data[3];
		return;
	}
	
	if (type >= KEYY && type <= KEYL){ //Control Parameter changes
		switch (type) {
			case KEYY: ////increase P2PHI
				p2phi += UP_P2PHI;
				if (p2phi > MAX_P2PHI) p2phi = MAX_P2PHI;
				printf("Increase P2PHI\n");
				break;
			case KEYH: //decrease P2PHI
				p2phi -= DOWN_P2PHI;
				if (p2phi < MIN_P2PHI) p2phi = MIN_P2PHI;
				printf("Decrease P2PHI\n");
				break;			
			case KEYU: /*yaw control P up*/
				p_yaw += UP_P_YAW;
				if (p_yaw > MAX_P_YAW) p_yaw = MAX_P_YAW;
				printf("Increase yaw control P\n");
				break;
			case KEYJ: /*yaw control P down*/
				p_yaw -= DOWN_P_YAW;
				if (p_yaw < MIN_P_YAW) p_yaw = MIN_P_YAW;
				printf("Decrease yaw control P\n");
				break;
			case KEYI: //roll/pitch control P1 up
				p1_full += UP_P1_FULL;
				if (p1_full > MAX_P1_FULL) p1_full = MAX_P1_FULL;
				printf("Increase roll pitch control P1\n");
				break;
			case KEYK: //roll/pitch control P1 down
				p1_full -= DOWN_P1_FULL;
				if (p1_full < MIN_P1_FULL) p1_full = MIN_P1_FULL;
				printf("Decrease roll pitch control P1\n");
				break;
			case KEYO: //roll/pitch control P2 up
				p2_full += UP_P2_FULL;
				if (p2_full > MAX_P2_FULL) p2_full = MAX_P2_FULL;				
				printf("Increase roll pitch control P2\n");
				break;
			case KEYL: //roll/pitch control P2 down
				p2_full -= DOWN_P2_FULL;
				if (p2_full < MIN_P2_FULL) p2_full = MIN_P2_FULL;
				printf("Decrease roll pitch control P1\n");
				break;
			default:
				printf("Doesn't know what controller parameter to change\n");
				break;				
		}
		printf("Controller changes:\np_yaw = %d\tp1_full = %d\tp2_full = %d\tp2phi = %d\n", p_yaw,p1_full,p2_full,p2phi);
		return;
	}

	
	//immagine that you are in safe controll lift_error=0 ---->ooi=0, but lift can be !=0, if you switch mode (for example manual) the qr get crazy		
	if ((oo1==0) && (oo2==0) && (oo3==0) && (oo4==0) && (lift==0) && (roll==0) && (pitch==0) && (yaw==0)) //Change control
	{		
		switch (type) {			
			case KEYESC: /* ESC: abort / exit */	
				printf("Exiting...\n");			
				finished=1;				
				break;		
			case KEYRETURN: /*increment control mode */
				if (mode != FULL) {
					mode++;
					printf("Changing to next mode\n");
				}
				else {
					terminate = 1;
					mode=SAFE;				
				}				
				break;		
			case KEY0: /*Safe Mode*/
				terminate = 1; //set flag to finish the program
				mode=SAFE;
				printf("Changing to safe mode\n");
				break;
			case KEY2: /*Manual Mode*/
				mode=MANUAL;	
				printf("Changing to manual mode\n");
				break;										
			case KEY3: /*Calibration Mode*/
				mode=CALIBRATION;
				printf("Changing to calibration mode\n");
				break;					
			case KEY4: /*Yaw control Mode*/
				mode=YAW;
				printf("Changing to yaw mode\n");
				break;
			case KEY5: /*Full control mode*/
				mode=FULL;
				printf("Changing to full mode\n");
				break;
			default:
				printf("Ready to change mode but wrong key.\n");
				break;
		}
		if (mode == FULL)
			reset_kalman();
	}
	else	
		printf("Can't change mode.\n");
}


void handleMode (void) {

	oldoo1 = oo1;
	oldoo2 = oo2;
	oldoo3 = oo3;
	oldoo4 = oo4;
	
	switch(mode) {
		case SAFE:
			//X32_leds = (X32_leds & 15) | 32;	// (X32_leds & 0000 1111) | 0010 0000
			safe_mode_ctrl();
			break;
		case PANIC:
			//X32_leds = (X32_leds & 15) | 64;	//(X32_leds & 0000 1111) | 0100 0000
			panic_mode_ctrl();
			break;
		case MANUAL:
			//X32_leds =(X32_leds & 15) | 96; //(X32_leds & 0000 1111) | 0110 0000
			manual_mode_ctrl();
			break;
		case CALIBRATION:
			//X32_leds =(X32_leds & 15) | 128; //(X32_leds & 0000 1111) | 1000 0000
			calibration_mode();
			break;
		case YAW:
			//X32_leds =(X32_leds & 15) | 160; //(X32_leds & 0000 1111) | 1010 0000	
			yaw_mode_ctrl();
			break;
		case FULL:
			//X32_leds =(X32_leds & 15) | 192; //(X32_leds & 0000 1111) | 1100 0000
			full_mode_ctrl();
			break;
		default:
			//X32_leds =(X32_leds & 15) | 224; //(X32_leds & 0000 1111) | 1110 0000		
			break;
	}
}


 /*------------------------------------------------------------------
 * isr_qr_link -- QR link rx interrupt handler
 *------------------------------------------------------------------
 */
void isr_qr_link(void)
{
	//save the time of the latest QR link activity
	isr_qr_time = X32_us_clock;
	
	/* get sensor and timestamp values */
	s0 = X32_QR_s0 - s0_bias;
	s1 = -(X32_QR_s1 - s1_bias); 
	s2 = X32_QR_s2 - s2_bias; 
	s3 =-(X32_QR_s3 - s3_bias); 
	s4 =-(X32_QR_s4 - s4_bias); 
	s5 = - (X32_QR_s5 - s5_bias);
}

void isr_timer1(void)
{
	check_qr_link();
	handleMode();
	clip_AE();
	
	if (lift == 0){
		oo1 = 0;
		oo2 = 0;
		oo3 = 0;
		oo4 = 0;
	}			

	X32_QR_a0 = oo1;
	X32_QR_a1 = oo2;
	X32_QR_a2 = oo3;
	X32_QR_a3 = oo4;
	
	if((X32_ms_clock % BLINK_COUNT) == 0)
	{
			toggle_led(0);
	}

/*	if (X32_ms_clock % 100 == 0) {
		X32_display = oo2;//printf("%d %d %d %d \n", oo1, oo2, oo3, oo4);
		if (flag_data_loggin == 0)
			{
				phi_kalman_roll_loggin=phi_kalman_roll;
				p_kalman_roll_loggin=p_kalman_roll;
				s0_loggin=s0;
				s3_loggin=s3;
				roll_loggin=roll;
				flag_data_loggin=1;
			}
	}*/
}

void isr_timer2(void)
{
	check_pc_link();
	toggle_led(6);
}
void isr_overflow(void)
{
	//log and flag error
	toggle_led(7);
	//mode=PANIC;
}

void isr_division_by_zero(void)
{
	//log and flag error
	X32_leds |= 0x40;
	printf("Division by zero\n");	
	mode=PANIC;
}

void isr_out_of_memory(void)
{
	//log and flag error
	X32_leds |= 0x20;
	printf("Out of memory\n");
	mode = PANIC;
}

void isr_trap(void)
{
	//log and flag error
	X32_leds |= 0x10;
	printf("Trap\n");
	mode=PANIC;
}


void check_pc_link(void)
{
	if (time_last_packet!=0) {
		if (X32_ms_clock - time_last_packet > TIME_OUT_PC_LINK) {
			mode=PANIC;
			X32_leds = 253; //1111 1101
			//log error
		}
		else
			X32_leds|=2;
		
	}
}

void check_qr_link(void)
{
	if (isr_qr_time!=0) {
		if (X32_us_clock - isr_qr_time > TIME_OUT_QR_LINK) {
			mode=PANIC;
			X32_leds = 251; //1111 1011
			//log error
		}
		else
			X32_leds|=4;
	}
}

void clip_AE()
{
//absolute clipping
	if (oo1 < MIN_MOTOR1) oo1 = MIN_MOTOR1;
	if (oo1 > MAX_MOTOR1) oo1 = MAX_MOTOR1;
	
	if (oo2 < MIN_MOTOR2) oo2 = MIN_MOTOR2;
	if (oo2 > MAX_MOTOR2) oo2 = MAX_MOTOR2;
	
	if (oo3 < MIN_MOTOR3) oo3 = MIN_MOTOR3;
	if (oo3 > MAX_MOTOR3) oo3 = MAX_MOTOR3;
	
	if (oo4 < MIN_MOTOR4) oo4 = MIN_MOTOR4;
	if (oo4 > MAX_MOTOR4) oo4 = MAX_MOTOR4;					

// rate clipping
	if(oo1 - oldoo1 > LIMIT_RATE)
		oo1 = oldoo1 + LIMIT_RATE;
	else if(oldoo1 - oo1 > LIMIT_RATE)
		oo1 = oldoo1 - LIMIT_RATE;

	if(oo2 - oldoo2 > LIMIT_RATE)
		oo2 = oldoo2 + LIMIT_RATE;
	else if(oldoo2 - oo2 > LIMIT_RATE)
		oo2 = oldoo2 - LIMIT_RATE;
		
	if(oo3 - oldoo3 > LIMIT_RATE)
		oo3 = oldoo3 + LIMIT_RATE;
	else if(oldoo3 - oo3 > LIMIT_RATE)
		oo3 = oldoo3 - LIMIT_RATE;
		
	if(oo4 - oldoo4 > LIMIT_RATE)
		oo4 = oldoo4 + LIMIT_RATE;
	else if(oldoo4 - oo4 > LIMIT_RATE)
		oo4 = oldoo4 - LIMIT_RATE;

	
}

void toggle_led(int i) 
{
	X32_leds ^= 1 << i;
}

