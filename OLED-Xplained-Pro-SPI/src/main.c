/**
 * \file
 *
 * \brief Empty user application template
 *
 */

/**
 * \mainpage User Application template doxygen documentation
 *
 * \par Empty user application template
 *
 * Bare minimum empty user application template
 *
 * \par Content
 *
 * -# Include the ASF header files (through asf.h)
 * -# "Insert system clock initialization code here" comment
 * -# Minimal main function that starts with a call to board_init()
 * -# "Insert application code here" comment
 *
 */

/*
 * Include header files for all drivers that have been imported from
 * Atmel Software Framework (ASF).
 */
/*
 * Support and FAQ: visit <a href="http://www.atmel.com/design-support/">Atmel Support</a>
 */
#include <asf.h>

#include "gfx_mono_ug_2832hsweg04.h"
#include "gfx_mono_text.h"
#include "sysfont.h"

#define YEAR        2019
#define MOUNT       9
#define DAY         30
#define WEEK        40
#define HOUR        16
#define MINUTE      52
#define SECOND      45

#define CLK_PIO				PIOA
#define CLK_PIO_ID			ID_PIOA
#define CLK_PIO_IDX			2
#define CLK_PIO_IDX_MASK	(1u << CLK_PIO_IDX)

#define SW_PIO				PIOA
#define SW_PIO_ID			ID_PIOA
#define SW_PIO_IDX			24
#define SW_PIO_IDX_MASK		(1u << SW_PIO_IDX)

#define LED_PIO_ID	   ID_PIOA
#define LED_PIO        PIOA
#define LED_PIN		   0
#define LED_PIN_MASK   (1<<LED_PIN)

#define LED2_PIO_ID	    ID_PIOC
#define LED2_PIO        PIOC
#define LED2_PIN		30
#define LED2_PIN_MASK   (1<<LED2_PIN)

volatile Bool SW_flag = false;
volatile Bool CLK_flag = false;
volatile int Time = 0;
volatile int terminado = 0;

void SW_init(void);
void RTC_init(void);
void LED_init(int estado);
void TC_init(Tc * TC, int ID_TC, int TC_CHANNEL, int freq);
void pin_toggle(Pio *pio, uint32_t mask);

static void SW_Handler(void)
{
	SW_flag = !SW_flag;
	if(terminado == 1)
		tc_stop(TC0, 0);
}

static void CLK_Handler(void)
{
	CLK_flag = 1;
}

void pin_toggle(Pio *pio, uint32_t mask){
	if(pio_get_output_data_status(pio, mask))
	pio_clear(pio, mask);
	else
	pio_set(pio,mask);
}

void update_screen(int hora, int minuto, int segundo) {
	gfx_mono_ssd1306_init();
	char buff[64];
	sprintf(buff, "%02d:%02d:%02d %ds", hora, minuto, segundo, Time);
	gfx_mono_draw_string(buff, 2,16, &sysfont);
}

void TimerDec(void) {
	Time--;
}

void TC0_Handler(void){
	volatile uint32_t ul_dummy;

	/****************************************************************
	* Devemos indicar ao TC que a interrupção foi satisfeita.
	******************************************************************/
	ul_dummy = tc_get_status(TC0, 0);

	/* Avoid compiler warning */
	UNUSED(ul_dummy);

	/* pin_toggle(BUZZER_PIO, BUZZER_PIO_IDX_MASK); */
	
	pin_toggle(LED_PIO, LED_PIN_MASK);
}

void TC1_Handler(void){
	volatile uint32_t ul_dummy;

	/****************************************************************
	* Devemos indicar ao TC que a interrupção foi satisfeita.
	******************************************************************/
	ul_dummy = tc_get_status(TC0, 1);

	/* Avoid compiler warning */
	UNUSED(ul_dummy);

	/** Muda o estado do LED */
	pin_toggle(LED2_PIO, LED2_PIN_MASK);
}

void RTC_Handler(void)
{
	uint32_t ul_status = rtc_get_status(RTC);

	/*
	*  Verifica por qual motivo entrou
	*  na interrupcao, se foi por segundo
	*  ou Alarm
	*/
	unsigned int hora;
	unsigned int minuto;
	unsigned int segundo;
	

	rtc_get_time(RTC, &hora, &minuto , &segundo);
	
	
	if ((ul_status & RTC_SR_ALARM) == RTC_SR_ALARM) {
	//	TC_init(TC0, ID_TC0, 0, 6);
	//	tc_stop(TC0, 1);
		rtc_clear_status(RTC, RTC_SCCR_ALRCLR);
		SW_flag = false;
	}
	
	if ((ul_status & RTC_SR_SEC) == RTC_SR_SEC) {
		if(SW_flag == true & Time == 0) {
			tc_stop(TC0, 0);
		}
		if(SW_flag == true) {
			tc_start(TC0, 1);
			if (Time == 0 ) {
				TC_init(TC0, ID_TC0, 0, 6);
				tc_stop(TC0, 1);
				terminado = 1;
				//rtc_set_time_alarm(RTC, 1, hora, 1, minuto+1, 1, segundo);
			} else {
				TimerDec();
				//rtc_set_time_alarm(RTC, 1, hora, 1, minuto, 1, segundo+Time);
			}
			
		}
		
		update_screen(hora, minuto, segundo);
		rtc_clear_status(RTC, RTC_SCCR_SECCLR);
	}
	

	
	rtc_clear_status(RTC, RTC_SCCR_ACKCLR);
	rtc_clear_status(RTC, RTC_SCCR_TIMCLR);
	rtc_clear_status(RTC, RTC_SCCR_CALCLR);
	rtc_clear_status(RTC, RTC_SCCR_TDERRCLR);
	
}

void SW_init(void) {
	pmc_enable_periph_clk(SW_PIO_ID);
	pio_set_input(SW_PIO, SW_PIO_IDX_MASK, PIO_PULLUP | PIO_DEBOUNCE);
	
	pio_enable_interrupt(SW_PIO, SW_PIO_IDX_MASK);
	pio_handler_set(SW_PIO, SW_PIO_ID, SW_PIO_IDX_MASK, PIO_IT_FALL_EDGE, SW_Handler);
	
	NVIC_EnableIRQ(SW_PIO_ID);
	NVIC_SetPriority(SW_PIO_ID, 2);
	
}

void TC_init(Tc * TC, int ID_TC, int TC_CHANNEL, int freq){
	uint32_t ul_div;
	uint32_t ul_tcclks;
	uint32_t ul_sysclk = sysclk_get_cpu_hz();

	uint32_t channel = 1;

	/* Configura o PMC */
	/* O TimerCounter é meio confuso
	o uC possui 3 TCs, cada TC possui 3 canais
	TC0 : ID_TC0, ID_TC1, ID_TC2
	TC1 : ID_TC3, ID_TC4, ID_TC5
	TC2 : ID_TC6, ID_TC7, ID_TC8
	*/
	pmc_enable_periph_clk(ID_TC);

	/** Configura o TC para operar em  4Mhz e interrupçcão no RC compare */
	tc_find_mck_divisor(freq, ul_sysclk, &ul_div, &ul_tcclks, ul_sysclk);
	tc_init(TC, TC_CHANNEL, ul_tcclks | TC_CMR_CPCTRG);
	tc_write_rc(TC, TC_CHANNEL, (ul_sysclk / ul_div) / freq);

	/* Configura e ativa interrupçcão no TC canal 0 */
	/* Interrupção no C */
	NVIC_EnableIRQ((IRQn_Type) ID_TC);
	tc_enable_interrupt(TC, TC_CHANNEL, TC_IER_CPCS);

	/* Inicializa o canal 0 do TC */
	tc_start(TC, TC_CHANNEL);
}

void RTC_init(){
	/* Configura o PMC */
	pmc_enable_periph_clk(ID_RTC);

	/* Default RTC configuration, 24-hour mode */
	rtc_set_hour_mode(RTC, 0);

	/* Configura data e hora manualmente */
	rtc_set_date(RTC, YEAR, MOUNT, DAY, WEEK);
	rtc_set_time(RTC, HOUR, MINUTE, SECOND);

	/* Configure RTC interrupts */
	NVIC_DisableIRQ(RTC_IRQn);
	NVIC_ClearPendingIRQ(RTC_IRQn);
	NVIC_SetPriority(RTC_IRQn, 0);
	NVIC_EnableIRQ(RTC_IRQn);

	/* Ativa interrupcao via alarme */
	rtc_enable_interrupt(RTC,  RTC_IER_SECEN);

}

void init(void)
{

	sysclk_init();
	
	WDT->WDT_MR = WDT_MR_WDDIS;
	
	pmc_enable_periph_clk(CLK_PIO_ID);
	pio_set_input(CLK_PIO, CLK_PIO_IDX_MASK, PIO_DEBOUNCE);
	
	pio_enable_interrupt(CLK_PIO, CLK_PIO_IDX_MASK);
	pio_handler_set(CLK_PIO, CLK_PIO_ID, CLK_PIO_IDX_MASK, PIO_IT_FALL_EDGE, CLK_Handler);
	
	NVIC_EnableIRQ(CLK_PIO_ID);
	NVIC_SetPriority(CLK_PIO_ID, 2);
}

void LED_init(int estado) {
	pmc_enable_periph_clk(LED_PIO_ID);
	pio_set_output(LED_PIO, LED_PIN_MASK, estado, 0, 0 );
	
	pmc_enable_periph_clk(LED2_PIO_ID);
	pio_set_output(LED2_PIO, LED2_PIN_MASK, estado, 0, 0 );
}

int main (void)
{
	/* Insert system clock initialization code here (sysclk_init()). */

	board_init();
	sysclk_init();
	
	
	delay_init();
	
	init();
	RTC_init();
	SW_init();
	LED_init(1);
	TC_init(TC0, ID_TC1, 1, 1);
	tc_stop(TC0, 1);
	
 

/*	gfx_mono_ssd1306_init();*/
// 	gfx_mono_draw_filled_circle(20, 16, 16, GFX_PIXEL_SET, GFX_WHOLE);
//    gfx_mono_draw_string("mundo", 50,16, &sysfont);

	SW_flag = false;
	CLK_flag = false;
  /* Insert application code here, after the board has been initialized. */
	while(1) {
		if(CLK_flag == true) {
				Time++;
			CLK_flag = false;
		}
		if(Time > 60) {
			Time = 60;
		}
		if(Time == -1) {
			Time = 0;
			SW_flag = false;
		}
		pmc_sleep(SAM_PM_SMODE_SLEEP_WFI);
	}
}
