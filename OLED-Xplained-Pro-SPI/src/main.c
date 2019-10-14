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
volatile int terminado_g = false;
volatile int Time_g = 0;


void TC_init(Tc * TC, int ID_TC, int TC_CHANNEL, int freq);
void pin_toggle(Pio *pio, uint32_t mask);

static void SW_Handler(void)
{
	SW_flag = true;
}

static void CLK_Handler(void)
{
	CLK_flag = true;
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
	sprintf(buff, "%02d:%02d:%02d %ds", hora, minuto, segundo, Time_g);
	gfx_mono_draw_string(buff, 2,16, &sysfont);
}

void TimerDec(uint *Time_g) {
	Time_g--;
}

void TimerInc(uint *Time_g) {
	Time_g++;
}

void TC0_Handler(void){
	volatile uint32_t ul_dummy;

	ul_dummy = tc_get_status(TC0, 0);

	UNUSED(ul_dummy);
	
	pin_toggle(LED_PIO, LED_PIN_MASK);
}

void TC1_Handler(void){
	volatile uint32_t ul_dummy;

	ul_dummy = tc_get_status(TC0, 1);

	UNUSED(ul_dummy);

	pin_toggle(LED2_PIO, LED2_PIN_MASK);
}

void RTC_Handler(void)
{
	uint32_t ul_status = rtc_get_status(RTC);

	unsigned int hora;
	unsigned int minuto;
	unsigned int segundo;
	

	rtc_get_time(RTC, &hora, &minuto , &segundo);
	
	
	if ((ul_status & RTC_SR_ALARM) == RTC_SR_ALARM) {
		rtc_clear_status(RTC, RTC_SCCR_ALRCLR);
		SW_flag = false;
	}
	
	if ((ul_status & RTC_SR_SEC) == RTC_SR_SEC) {
		if(SW_flag == true & Time_g == 0) {
			tc_stop(TC0, 0);
		}
		if(SW_flag == true) {
			tc_start(TC0, 1);
			if (Time_g == 0 ) {
				TC_init(TC0, ID_TC0, 0, 6);
				tc_stop(TC0, 1);
				terminado_g = true;
			} else {
				TimerDec(Time_g);
			}
			
		}
		
		
		rtc_clear_status(RTC, RTC_SCCR_SECCLR);
	}
	

	
	rtc_clear_status(RTC, RTC_SCCR_ACKCLR);
	rtc_clear_status(RTC, RTC_SCCR_TIMCLR);
	rtc_clear_status(RTC, RTC_SCCR_CALCLR);
	rtc_clear_status(RTC, RTC_SCCR_TDERRCLR);
	
}

void TC_init(Tc * TC, int ID_TC, int TC_CHANNEL, int freq){
	uint32_t ul_div;
	uint32_t ul_tcclks;
	uint32_t ul_sysclk = sysclk_get_cpu_hz();

	uint32_t channel = 1;

	pmc_enable_periph_clk(ID_TC);

	tc_find_mck_divisor(freq, ul_sysclk, &ul_div, &ul_tcclks, ul_sysclk);
	tc_init(TC, TC_CHANNEL, ul_tcclks | TC_CMR_CPCTRG);
	tc_write_rc(TC, TC_CHANNEL, (ul_sysclk / ul_div) / freq);

	NVIC_EnableIRQ((IRQn_Type) ID_TC);
	tc_enable_interrupt(TC, TC_CHANNEL, TC_IER_CPCS);

	tc_start(TC, TC_CHANNEL);
}

void init(int estado)
{
	board_init();
	sysclk_init();
	delay_init();
	
	
	WDT->WDT_MR = WDT_MR_WDDIS;
	
	pmc_enable_periph_clk(CLK_PIO_ID);
	pio_set_input(CLK_PIO, CLK_PIO_IDX_MASK, PIO_DEBOUNCE);
	
	pio_enable_interrupt(CLK_PIO, CLK_PIO_IDX_MASK);
	pio_handler_set(CLK_PIO, CLK_PIO_ID, CLK_PIO_IDX_MASK, PIO_IT_FALL_EDGE, CLK_Handler);
	
	NVIC_EnableIRQ(CLK_PIO_ID);
	NVIC_SetPriority(CLK_PIO_ID, 2);
	
	//LED
	pmc_enable_periph_clk(LED_PIO_ID);
	pio_set_output(LED_PIO, LED_PIN_MASK, estado, 0, 0 );
	
	pmc_enable_periph_clk(LED2_PIO_ID);
	pio_set_output(LED2_PIO, LED2_PIN_MASK, estado, 0, 0 );
	
	//RTC
	pmc_enable_periph_clk(ID_RTC);
	rtc_set_hour_mode(RTC, 0);
	rtc_set_date(RTC, YEAR, MOUNT, DAY, WEEK);
	rtc_set_time(RTC, HOUR, MINUTE, SECOND);

	NVIC_DisableIRQ(RTC_IRQn);
	NVIC_ClearPendingIRQ(RTC_IRQn);
	NVIC_SetPriority(RTC_IRQn, 0);
	NVIC_EnableIRQ(RTC_IRQn);

	rtc_enable_interrupt(RTC,  RTC_IER_SECEN);
	
	TC_init(TC0, ID_TC1, 1, 1);
	
	pmc_enable_periph_clk(SW_PIO_ID);
	pio_set_input(SW_PIO, SW_PIO_IDX_MASK, PIO_PULLUP | PIO_DEBOUNCE);
	
	pio_enable_interrupt(SW_PIO, SW_PIO_IDX_MASK);
	pio_handler_set(SW_PIO, SW_PIO_ID, SW_PIO_IDX_MASK, PIO_IT_FALL_EDGE, SW_Handler);
	
	NVIC_EnableIRQ(SW_PIO_ID);
	NVIC_SetPriority(SW_PIO_ID, 2);
}

int main (void)
{
	
	init(1);
	tc_stop(TC0, 1);
	
	SW_flag = false;
	CLK_flag = false;
	
	while(1) {
		unsigned int hora;
		unsigned int minuto;
		unsigned int segundo;
		

		rtc_get_time(RTC, &hora, &minuto , &segundo);
		update_screen(hora, minuto, segundo);
		if(CLK_flag == true) {
				TimerInc(Time_g);
			CLK_flag = false;
		}
		if(Time_g > 60) {
			Time_g = 60;
		}
		if(Time_g == -1) {
			Time_g = 0;
			SW_flag = false;
		}
		if(terminado_g == true)
			tc_stop(TC0, 0);
		pmc_sleep(SAM_PM_SMODE_SLEEP_WFI);
	}
}
