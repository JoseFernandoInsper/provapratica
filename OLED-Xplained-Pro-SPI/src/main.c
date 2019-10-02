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
#define CLK_PIO_IDX			3u
#define CLK_PIO_IDX_MASK	(1u << CLK_PIO_IDX)

#define SW_PIO				PIOA
#define SW_PIO_ID			ID_PIOA
#define SW_PIO_IDX			4u
#define SW_PIO_IDX_MASK		(1u << SW_PIO_IDX)

void RTC_init(void);

void update_screen(int hora, int minuto, int segundo) {
	gfx_mono_ssd1306_init();
	char time[64];
	sprintf(time, "%02d:%02d:%02d", hora, minuto, segundo);
	gfx_mono_draw_string(time, 2,16, &sysfont);
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

	
	if ((ul_status & RTC_SR_SEC) == RTC_SR_SEC) {
		
		update_screen(hora, minuto, segundo);
		rtc_clear_status(RTC, RTC_SCCR_SECCLR);
	}
	

	
	rtc_clear_status(RTC, RTC_SCCR_ACKCLR);
	rtc_clear_status(RTC, RTC_SCCR_TIMCLR);
	rtc_clear_status(RTC, RTC_SCCR_CALCLR);
	rtc_clear_status(RTC, RTC_SCCR_TDERRCLR);
	
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



int main (void)
{
	/* Insert system clock initialization code here (sysclk_init()). */

	board_init();
	sysclk_init();
	
	
	delay_init();
	
	RTC_init();
	
/*	gfx_mono_ssd1306_init();*/
// 	gfx_mono_draw_filled_circle(20, 16, 16, GFX_PIXEL_SET, GFX_WHOLE);
//    gfx_mono_draw_string("mundo", 50,16, &sysfont);


  /* Insert application code here, after the board has been initialized. */
	while(1) {
		pmc_sleep(SAM_PM_SMODE_SLEEP_WFI);
	}
}
