 #ifndef X32_H
#define X32_H
#define ADDR_PERIPHERALS							0x80000000
#define PERIPHERAL_INT_VECT_BASE			0x10000000
#define CLOCKS_PER_MS									(50000000/1000)
#define PERIPHERAL_VERSION						0x4073
	#define INTERRUPT_MAX									(16-1) 
	#define INTERRUPT_GLOBAL							16
	#define PERIPHERAL_INT_ENABLE 				0x20
		#define INTERRUPT_TRAP 								0x00
		#define INTERRUPT_OVERFLOW 						0x02
		#define INTERRUPT_DIVISION_BY_ZERO 		0x01
		#define INTERRUPT_OUT_OF_MEMORY 			0x03
	#define PERIPHERAL_UID								0x00
	#define PERIPHERAL_PROCSTATE					0x09
	#define PERIPHERAL_INSTRCNTR					0x03

		#define PERIPHERAL_SWITCHES 0x06
			#define INTERRUPT_SWITCHES 0x08

		#define PERIPHERAL_BUTTONS 0x08
			#define INTERRUPT_BUTTONS 0x07

		#define PERIPHERAL_LEDS 0x07

		#define PERIPHERAL_PRIMARY_DATA 0x01
		#define PERIPHERAL_PRIMARY_STATUS 0x02
			#define INTERRUPT_PRIMARY_RX 0x09
			#define INTERRUPT_PRIMARY_TX 0x0A

		#define PERIPHERAL_DISPLAY 0x05

		#define PERIPHERAL_MS_CLOCK 0x04

		#define PERIPHERAL_TIMER1_PERIOD 0x26
			#define INTERRUPT_TIMER1 0x05

		#define PERIPHERAL_TIMER2_PERIOD 0x27
			#define INTERRUPT_TIMER2 0x06

			#define INTERRUPT_SOFTINT1 0x04
		#define PERIPHERAL_SOFTINT1 0x21

		#define PERIPHERAL_US_CLOCK 0x32

		#define PERIPHERAL_DPC1_PERIOD 0x40
		#define PERIPHERAL_DPC1_WIDTH 0x41

		#define PERIPHERAL_CUSTOM 0x34

		#define PERIPHERAL_ENGINE_A 0x30
			#define INTERRUPT_ENGINE_A 0x0E
		#define PERIPHERAL_ENGINE_B 0x31
			#define INTERRUPT_ENGINE_B 0x0F
		#define PERIPHERAL_ENGINE_DECODED 0x33
			#define INTERRUPT_ENGINE_ERROR 0x0D

			#define INTERRUPT_XUFO 0x0C
		#define PERIPHERAL_XUFO_COUNT 0x50
		#define PERIPHERAL_XUFO_TIMESTAMP 0x51
		#define PERIPHERAL_XUFO_S0 0x52
		#define PERIPHERAL_XUFO_S1 0x53
		#define PERIPHERAL_XUFO_S2 0x54
		#define PERIPHERAL_XUFO_S3 0x55
		#define PERIPHERAL_XUFO_S4 0x56
		#define PERIPHERAL_XUFO_S5 0x57
		#define PERIPHERAL_XUFO_S6 0x58
//		#define PERIPHERAL_XUFO_A 0x59
		#define PERIPHERAL_XUFO_A0 0x59
		#define PERIPHERAL_XUFO_A1 0x5A
		#define PERIPHERAL_XUFO_A2 0x5B
		#define PERIPHERAL_XUFO_A3 0x5C
extern int* peripherals;
#define THREAD_SAFE_EXECUTION_LEVEL		0xFFFFFFFF
typedef unsigned short INSTRUCTION;
#define COM_CLEAR_TO_SEND(value) (peripherals[value] & 0x01)
#define COM_BYTE_AVAILABLE(value) (peripherals[value] & 0x02)
#define X32_MS_CLOCK (peripherals[0x04])
#define X32_US_CLOCK (peripherals[0x32])
#define X32_STDOUT (peripherals[0x01])
#define X32_STDOUT_STATUS (peripherals[0x02] & 0x01)
#define X32_STDIN (peripherals[0x01])
#define X32_STDIN_STATUS (peripherals[0x02] & 0x02)
	#define INTERRUPT_VECTOR(index) (*(void(**)(void))&peripherals[PERIPHERAL_INT_VECT_BASE+(index<<1)])
	#define INTERRUPT_PRIORITY(index) (peripherals[PERIPHERAL_INT_VECT_BASE+(index<<1)+1])
	#define ENABLE_INTERRUPT(index) enable_interrupt(index)
	#define DISABLE_INTERRUPT(index) disable_interrupt(index)
	#define SET_INTERRUPT_VECTOR(index, value) (INTERRUPT_VECTOR(index) = value)
	#define SET_INTERRUPT_PRIORITY(index, value) (INTERRUPT_PRIORITY(index) = value)
#define STATE_BOOTING (peripherals[PERIPHERAL_PROCSTATE] & 0x01)
#define STATE_DIVISION_BY_ZERO (peripherals[PERIPHERAL_PROCSTATE] & 0x08)
#define STATE_OVERFLOW (peripherals[PERIPHERAL_PROCSTATE] & 0x10)
#define STATE_OUT_OF_MEMORY (peripherals[PERIPHERAL_PROCSTATE] & 0x40)
#define STATE_TRAPPED (peripherals[PERIPHERAL_PROCSTATE] & 0x80)
#define STATE_SIMULATOR (peripherals[PERIPHERAL_PROCSTATE] & 0x02)
	void enable_interrupt(int);
	void disable_interrupt(int);
unsigned get_execution_level();
void set_execution_level(unsigned);
void restore_execution_level();
void combine_stackframe();
void** init_stack(void**, void*, void*);
void context_switch(void**, void***);
typedef int clock_t;
clock_t clock();
int putchar(int);
int getchar();
int getchar_nb();
void sleep(unsigned);
void usleep(unsigned);
typedef int LOCK;
#define lock(arg) _lock(&arg)
int _lock(LOCK*);
#define unlock(arg) _unlock(&arg)
void _unlock(LOCK*);
#endif 
