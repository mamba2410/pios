#include <stdint.h>
#include <addresses/irq.h>
#include <mmio/interrupts.h>
#include <mmio/mmio.h>
#include <serial/printf.h>
#include <system/timer.h>

char* const IRQ_NAMES[16] = {
	"Synchronous Exception EL1t",
	"Interrupt Request EL1t", 
	"Fast Interrupt Request EL1t",
	"System Error EL1t",
	"Synchronous Exception EL1h",
	"Interrupt Request EL1h", 
	"Fast Interrupt Request EL1h",
	"System Error EL1h",
	"Synchronous Exception EL0_32",
	"Interrupt Request EL0_32", 
	"Fast Interrupt Request EL0_32",
	"System Error EL0_32",
	"Synchronous Exception EL0_64",
	"Interrupt Request EL0_64", 
	"Fast Interrupt Request EL0_64",
	"System Error EL0_64"
};

/*
 * Handles known interrupts in EL1h
 */
void handle_irq_el1h(){
	//uint32_t irq = mmio_get32(IRQ_PENDING_1);	// Get the IRQ
	uint32_t irq;

	while( (irq = mmio_get32(IRQ_PENDING_1)) ){		// While there is an IRQ pending
		switch(irq){								// Which IRQ is pending?
			case SYSTEM_TIMER_IRQ_1:				// If its system timer 1...
				handle_system_timer();				// ...call a separate function
				break;	
			default:								// Else
				printf("Unknown pending irq: %x\n", irq);	// The IRQ is unrecognised and ignored
		}
	}	
}

/*
 * Prints information about the unhandled interrupt
 */
void show_invalid_entry_message(uint8_t exception_type, uint64_t esr, uint64_t *elr){
	printf("Exception type: %s (0x%x); esr: %x; elr: %x\n", IRQ_NAMES[exception_type], exception_type, esr, elr);
}


/*
 * Enables the interrupt controller
 */
void enable_interrupt_controller(){
	mmio_put32(ENABLE_IRQS_1, SYSTEM_TIMER_IRQ_1);
}
