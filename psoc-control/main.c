/*******************************************************************************
* File Name:   main.c
*
* Description: This is the source code for Hello World Example using PDL APIs
*              for ModusToolbox.
*
* Related Document: See README.md
*
*
*******************************************************************************
* (c) 2024-2026, Infineon Technologies AG, or an affiliate of Infineon
* Technologies AG. All rights reserved.
* This software, associated documentation and materials ("Software") is
* owned by Infineon Technologies AG or one of its affiliates ("Infineon")
* and is protected by and subject to worldwide patent protection, worldwide
* copyright laws, and international treaty provisions. Therefore, you may use
* this Software only as provided in the license agreement accompanying the
* software package from which you obtained this Software. If no license
* agreement applies, then any use, reproduction, modification, translation, or
* compilation of this Software is prohibited without the express written
* permission of Infineon.
*
* Disclaimer: UNLESS OTHERWISE EXPRESSLY AGREED WITH INFINEON, THIS SOFTWARE
* IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
* INCLUDING, BUT NOT LIMITED TO, ALL WARRANTIES OF NON-INFRINGEMENT OF
* THIRD-PARTY RIGHTS AND IMPLIED WARRANTIES SUCH AS WARRANTIES OF FITNESS FOR A
* SPECIFIC USE/PURPOSE OR MERCHANTABILITY.
* Infineon reserves the right to make changes to the Software without notice.
* You are responsible for properly designing, programming, and testing the
* functionality and safety of your intended application of the Software, as
* well as complying with any legal requirements related to its use. Infineon
* does not guarantee that the Software will be free from intrusion, data theft
* or loss, or other breaches ("Security Breaches"), and Infineon shall have
* no liability arising out of any Security Breaches. Unless otherwise
* explicitly approved by Infineon, the Software may not be used in any
* application where a failure of the Product or any consequences of the use
* thereof can reasonably be expected to result in personal injury.
*******************************************************************************/


/*******************************************************************************
* Header Files
*******************************************************************************/
#include "cy_pdl.h"
#include "cybsp.h"
#include "cy_retarget_io.h"
#include "mtb_hal.h"

/*******************************************************************************
* Macros
*******************************************************************************/

/*******************************************************************************
* Global Variables
*******************************************************************************/
const cy_stc_sysint_t intrCfg1 =
{
          .intrSrc = TCPWM_COUNTER_IRQ,
          .intrPriority = 7u
};

volatile bool timer_interrupt_flag = false;
bool led_blink_active_flag = true;

/* Variable for storing character read from terminal */
uint8_t uart_read_value;

/* For the Retarget -IO (Debug UART) usage */
static cy_stc_scb_uart_context_t    DEBUG_UART_context;           /** UART context */
static mtb_hal_uart_t               DEBUG_UART_hal_obj;           /** Debug UART HAL object  */

/*******************************************************************************
* Function Prototypes
*******************************************************************************/
void timer_init(void);
void isr_timer(void);
void measure_tcpwm_direct_register_write_read_timing(bool print_results);

/*******************************************************************************
* Function Definitions
*******************************************************************************/

/*******************************************************************************
* Function Name: main
********************************************************************************
* Summary:
* This is the main function. It sets up a timer to trigger a periodic interrupt.
* The main while loop checks for the status of a flag set by the interrupt and
* toggles an LED at 1Hz to create an LED blinky. Will be achieving the 1Hz Blink
* rate. The while loop also checks whether the 'Enter' key was pressed and
* stops/restarts LED blinking.
*
* Parameters:
*  void
*
* Return:
*  int
*
*******************************************************************************/
int main(void)
{
    cy_rslt_t result;

#if defined (CY_DEVICE_SECURE)
    cyhal_wdt_t wdt_obj;

    /* Clear watchdog timer so that it doesn't trigger a reset */
    result = cyhal_wdt_init(&wdt_obj, cyhal_wdt_get_max_timeout_ms());
    CY_ASSERT(CY_RSLT_SUCCESS == result);
    cyhal_wdt_free(&wdt_obj);
#endif

    /* Initialize the device and board peripherals */
    result = cybsp_init();

    /* Board init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* Enable global interrupts */
    __enable_irq();

    /* Debug UART init */
    result = (cy_rslt_t)Cy_SCB_UART_Init(DEBUG_UART_HW, &DEBUG_UART_config, &DEBUG_UART_context);

    /* UART init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    Cy_SCB_UART_Enable(DEBUG_UART_HW);

    /* Setup the HAL UART */
    result = mtb_hal_uart_setup(&DEBUG_UART_hal_obj, &DEBUG_UART_hal_config, &DEBUG_UART_context, NULL);

    /* HAL UART init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    result = cy_retarget_io_init(&DEBUG_UART_hal_obj);

    /* HAL retarget_io init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* \x1b[2J\x1b[;H - ANSI ESC sequence for clear screen */
    printf("\x1b[2J\x1b[;H");

    printf("****************** "
           "PDL: Hello World! Example "
           "****************** \r\n\n");

    printf("Hello World!!!\r\n\n");
    printf("For more projects, "
           "visit our code examples repositories:\r\n\n");

    printf("https://github.com/Infineon/"
           "Code-Examples-for-ModusToolbox-Software\r\n\n");

    /* Initialize timer to toggle the LED */
    timer_init();

    /* Direct-register (no PDL call) 10-register write/read timing test,
     * unbuffered mode only (WRITE_BUFF=0) - real bus access time. A silent
     * warm-up pass runs first to remove first-access cold-start overhead
     * from the printed result. WRITE_BUFF is forced before each call and
     * restored afterward since it is a chip-wide setting. */
    printf("---------------------------------------------------------\r\n");
    printf(" Boy2P (PSC3M6) direct-register (no PDL call) test conditions\r\n");
    printf(" Target: TCPWM0 GRP0_CNT0 (base 0x42A00000), unused/unconfigured\r\n");
    printf(" Registers (10): CC0, CC0_BUFF, CC1, CC1_BUFF, PERIOD_BUFF,\r\n");
    printf("                 LINE_SEL, LINE_SEL_BUFF, TR_IN_SEL0,\r\n");
    printf("                 TR_IN_SEL1, TR_IN_EDGE_SEL\r\n");
    printf(" Write value: 0x%08lX\r\n", (unsigned long)0x1u);
    printf(" CPU clock (SystemCoreClock) = %lu Hz\r\n", (unsigned long)SystemCoreClock);
    {
        uint32_t tcpwm_clk_hf0_hz = Cy_SysClk_ClkHfGetFrequency(0u);
        printf(" TCPWM clock source: CLK_HF0 (shared with CPU, single DPLL0) = %lu Hz (%.1f MHz)\r\n",
               (unsigned long)tcpwm_clk_hf0_hz, (double)tcpwm_clk_hf0_hz / 1000000.0);
    }
    printf(" Timing method: SysTick (CY_SYSTICK_CLOCK_SOURCE_CLK_CPU), read\r\n");
    printf("                immediately before first write and after last read\r\n");
    printf(" Mode: unbuffered (WRITE_BUFF=0) - real bus access time, no CPU\r\n");
    printf("       write-buffer posting\r\n");
    printf("---------------------------------------------------------\r\n\n");

    uint32_t direct_reg_orig_buff_ctl = CPUSS->BUFF_CTL;
    CPUSS->BUFF_CTL = direct_reg_orig_buff_ctl & ~CPUSS_BUFF_CTL_WRITE_BUFF_Msk;
    /* Warm-up pass (silent): primes pipeline/branch-predictor/flash-prefetch so the
     * printed measurement reflects steady-state bus access time, not one-time
     * cold-start overhead from the very first access to this register block. */
    measure_tcpwm_direct_register_write_read_timing(false);
    measure_tcpwm_direct_register_write_read_timing(true);
    CPUSS->BUFF_CTL = direct_reg_orig_buff_ctl;

    printf("Press 'Enter' key to pause or "
           "resume blinking the user LED \r\n\r\n");

    for (;;)
    {
        /* Check if 'Enter' key was pressed */
         if (mtb_hal_uart_get(&DEBUG_UART_hal_obj, &uart_read_value, 1) == CY_RSLT_SUCCESS)
         {

               if (uart_read_value == '\r')
               {
                     /* Pause LED blinking by stopping the timer */
                    if (led_blink_active_flag)
                    {
                          Cy_TCPWM_TriggerStopOrKill_Single(TCPWM_COUNTER_HW, TCPWM_COUNTER_NUM);

                         printf("LED blinking paused \r\n");
                    }
                    else /* Resume LED blinking by starting the timer */
                    {
                          Cy_TCPWM_TriggerStart_Single(TCPWM_COUNTER_HW, TCPWM_COUNTER_NUM);

                          printf("LED blinking resumed\r\n");
                    }

                 /* Move cursor to previous line */
                 printf("\x1b[1F");

                 led_blink_active_flag ^= 1;
             }
         }

         /* Check if timer elapsed (interrupt fired) */
         if (timer_interrupt_flag)
         {
             /* Clear the flag */
             timer_interrupt_flag = false;

             /* CYBSP_USER_LED is now dedicated to the TCPWM timing-test
              * scope marker (see
              * measure_tcpwm_direct_register_write_read_timing()), so the
              * periodic blink uses the second user LED (CYBSP_USER_LED4,
              * P8_5) instead. */
         //    Cy_GPIO_Inv(CYBSP_USER_LED4_PORT, CYBSP_USER_LED4_PIN);
         }

         /* Repeat the TCPWM direct-register timing test continuously so the
          * GPIO scope marker (CYBSP_USER_LED, P8_4) produces a steady train
          * of pulses that can be observed on an oscilloscope. Unbuffered
          * mode only (WRITE_BUFF=0) - this is the real bus access time,
          * with printing suppressed (print_results=false) so the UART stays
          * quiet while the loop runs. */
         CPUSS->BUFF_CTL = direct_reg_orig_buff_ctl & ~CPUSS_BUFF_CTL_WRITE_BUFF_Msk;
         measure_tcpwm_direct_register_write_read_timing(false);
         CPUSS->BUFF_CTL = direct_reg_orig_buff_ctl;
    }
}

/*******************************************************************************
 * Function Name: timer_init
 ********************************************************************************
 * Summary:
 * This function creates and configures a Timer object. The timer ticks
 * continuously and produces a periodic interrupt on every terminal count
 * event. The period is defined by the 'period' and 'compare_value' of the
 * timer configuration structure 'led_blink_timer_cfg'. Without any changes,
 * this application is designed to produce an interrupt every 1 second.
 *
 * Parameters:
 *  none
 *
 *******************************************************************************/
void timer_init(void)
{
     /* Enable interrupts */
     __enable_irq();

     /*TCPWM Counter Mode initial*/
     if (CY_TCPWM_SUCCESS != Cy_TCPWM_Counter_Init(TCPWM_COUNTER_HW, TCPWM_COUNTER_NUM, &TCPWM_COUNTER_config))
     {
          CY_ASSERT(0);
     }

     /* Enable the initialized counter */
     Cy_TCPWM_Counter_Enable(TCPWM_COUNTER_HW, TCPWM_COUNTER_NUM);

     /* Configure GPIO interrupt */
     Cy_TCPWM_SetInterruptMask(TCPWM_COUNTER_HW, TCPWM_COUNTER_NUM, CY_GPIO_INTR_EN_MASK);

     /* Configure CM4+ CPU GPIO interrupt vector for Port 0 */
     Cy_SysInt_Init(&intrCfg1, isr_timer);
     NVIC_EnableIRQ(TCPWM_COUNTER_IRQ);

     /* Start the counter */
     Cy_TCPWM_TriggerStart_Single(TCPWM_COUNTER_HW, TCPWM_COUNTER_NUM);
}

/*******************************************************************************
 * Function Name: measure_tcpwm_direct_register_write_read_timing
 ********************************************************************************
 * Summary:
 * Simple direct-register (no PDL call) write+read-back timing test.
 * Writes 10 TCPWM registers, reads them all back, and times the whole burst
 * with SysTick reads immediately before and after.
 *
 * Registers used: CC0, CC0_BUFF, CC1, CC1_BUFF, PERIOD_BUFF, LINE_SEL,
 * LINE_SEL_BUFF, TR_IN_SEL0, TR_IN_SEL1, TR_IN_EDGE_SEL - all on TCPWM0
 * GRP0_CNT0 (base 0x42A00000). This is NOT the app's actual TCPWM_COUNTER
 * instance (GRP1_CNT0/index 256) - GRP1_CNT0 does not document these 5
 * registers (LINE_SEL/LINE_SEL_BUFF/TR_IN_SEL0/TR_IN_SEL1/TR_IN_EDGE_SEL) in
 * the register reference manual. GRP0_CNT0 documents all 10 and is
 * completely unused/disabled by this app, so no restore is needed.
 *
 * WRITE_BUFF is set by the caller (main()) before each call, not inside
 * this routine - it just reports the current CPUSS_BUFF_CTL.WRITE_BUFF
 * state that was in effect for the burst it just measured.
 *
 * Parameters:
 *  print_results - when true, prints the full results block over UART
 *                  (used for the one-shot run before the main loop). When
 *                  false, the timed write/read/verify/GPIO-marker work still
 *                  runs every call (so the scope marker keeps pulsing), but
 *                  no printf output is produced (used inside the main loop).
 *
 *******************************************************************************/
void measure_tcpwm_direct_register_write_read_timing(bool print_results)
{
    TCPWM_GRP_CNT_Type *cnt = &TCPWM_COUNTER_HW->GRP[0UL].CNT[0UL];   /* TCPWM0 GRP0_CNT0 */

    uint32_t write_val = 0x1u;
    uint32_t rd_cc0, rd_cc0_buff, rd_cc1, rd_cc1_buff, rd_period_buff;
    uint32_t rd_line_sel, rd_line_sel_buff, rd_tr_in_sel0, rd_tr_in_sel1, rd_tr_in_edge_sel;
    uint32_t mismatches = 0u;

    Cy_SysTick_Init(CY_SYSTICK_CLOCK_SOURCE_CLK_CPU, 0x00FFFFFFUL);
    Cy_SysTick_DisableInterrupt();
    Cy_SysTick_Clear();

    /* Scope marker (repurposed CYBSP_USER_LED): set HIGH just before the
     * timed window starts, CLR just after it ends (see matching clear below).
     * Direct OUT_SET register write - outside the timed region, so it does
     * not add to the measured cycle count, but lets the full write+read
     * burst be seen/cross-checked on an oscilloscope/logic analyzer. */
    CYBSP_USER_LED_PORT->OUT_SET = (1UL << CYBSP_USER_LED_PIN);

    /* Read SysTick->VAL directly instead of calling Cy_SysTick_GetValue().
     * The PDL call is a real BL/BX function call - an opaque call boundary
     * that forces the compiler to recompute 'cnt' and 'write_val' AFTER the
     * call returns (since r0-r3 are caller-saved and may be clobbered),
     * pulling that setup work inside the timed window. Reading SysTick->VAL
     * directly removes that call boundary, letting the compiler schedule
     * the pointer/constant setup before the timestamp is taken. */
    uint32_t start_val = SysTick->VAL;

    /* 10 register accesses, each write immediately followed by a read-back
     * of the SAME register (unlike the earlier batched write-all-then-
     * read-all pattern, this exposes any write-buffer-vs-read hazard: with
     * WRITE_BUFF=1, a write is posted/buffered and the CPU can move on, but
     * an immediately-following read to the SAME address may have to stall
     * until that buffered write actually completes on the bus, or may be
     * forwarded/short-circuited by hazard-detection logic - either way this
     * pattern is a more realistic worst case than reading back long after
     * the write has already drained. */
    cnt->CC0 = write_val;            rd_cc0            = cnt->CC0;
    cnt->CC0_BUFF = write_val;       rd_cc0_buff       = cnt->CC0_BUFF;
    cnt->CC1 = write_val;            rd_cc1            = cnt->CC1;
    cnt->CC1_BUFF = write_val;       rd_cc1_buff       = cnt->CC1_BUFF;
    cnt->PERIOD_BUFF = write_val;    rd_period_buff    = cnt->PERIOD_BUFF;
    cnt->LINE_SEL = write_val;       rd_line_sel       = cnt->LINE_SEL;
    cnt->LINE_SEL_BUFF = write_val;  rd_line_sel_buff  = cnt->LINE_SEL_BUFF;
    cnt->TR_IN_SEL0 = write_val;     rd_tr_in_sel0     = cnt->TR_IN_SEL0;
    cnt->TR_IN_SEL1 = write_val;     rd_tr_in_sel1     = cnt->TR_IN_SEL1;
    cnt->TR_IN_EDGE_SEL = write_val; rd_tr_in_edge_sel = cnt->TR_IN_EDGE_SEL;

    uint32_t end_val = SysTick->VAL;

    /* Scope marker: CLR immediately after the timed window ends */
    CYBSP_USER_LED_PORT->OUT_CLR = (1UL << CYBSP_USER_LED_PIN);

    /* Verify after timing so compares don't pollute the measured region */
    if (rd_cc0            != write_val) { mismatches++; }
    if (rd_cc0_buff       != write_val) { mismatches++; }
    if (rd_cc1            != write_val) { mismatches++; }
    if (rd_cc1_buff       != write_val) { mismatches++; }
    if (rd_period_buff    != write_val) { mismatches++; }
    if (rd_line_sel       != write_val) { mismatches++; }
    if (rd_line_sel_buff  != write_val) { mismatches++; }
    if (rd_tr_in_sel0     != write_val) { mismatches++; }
    if (rd_tr_in_sel1     != write_val) { mismatches++; }
    if (rd_tr_in_edge_sel != write_val) { mismatches++; }

    uint32_t elapsed_cycles = start_val - end_val;   /* SysTick counts DOWN */

    uint32_t write_buff = (CPUSS->BUFF_CTL & CPUSS_BUFF_CTL_WRITE_BUFF_Msk);

    if (print_results)
    {
        printf("---------------------------------------------------------\r\n");
        printf(" Boy2P (PSC3M6) direct-register (no PDL call) 10-register write+read\r\n");
        printf(" Pattern: write then immediate read-back of the SAME register\r\n");
        printf(" Registers: CC0, CC0_BUFF, CC1, CC1_BUFF, PERIOD_BUFF, LINE_SEL,\r\n");
        printf("            LINE_SEL_BUFF, TR_IN_SEL0, TR_IN_SEL1, TR_IN_EDGE_SEL\r\n");
        printf(" CPUSS_BUFF_CTL.WRITE_BUFF = %lu (%s)\r\n",
               (unsigned long)write_buff, write_buff ? "buffered" : "unbuffered");
        printf(" 10 writes + 10 reads took %lu CPU cycles, %lu mismatch(es)\r\n",
               (unsigned long)elapsed_cycles, (unsigned long)mismatches);
        printf(" Average: %.2f cycles/access (%.1f ns/access)\r\n",
               (double)elapsed_cycles / 20.0, ((double)elapsed_cycles / 20.0 * 1.0e9) / (double)SystemCoreClock);
        printf("---------------------------------------------------------\r\n\n");
    }
}

/*******************************************************************************
 * Function Name: isr_timer
 ********************************************************************************
 * Summary:
 * This is the interrupt handler function for the timer interrupt.
 *
 * Parameters:
 *  none
 *
 *******************************************************************************/
void isr_timer(void)
{
     uint32_t interrupts = Cy_TCPWM_GetInterruptStatusMasked(TCPWM_COUNTER_HW, TCPWM_COUNTER_NUM);

     /* Clear the interrupt */
     Cy_TCPWM_ClearInterrupt(TCPWM_COUNTER_HW, TCPWM_COUNTER_NUM, interrupts);

     if (0UL != (CY_TCPWM_INT_ON_TC & interrupts))
     {
          /* Set the interrupt flag and process it from the main while(1) loop */
          timer_interrupt_flag = true;
     }
}
/* [] END OF FILE */
