ISR stubs are created using the ISR_NOERROR and ISR_ERROR macros

- ISR_ERROR is for the exceptions that the CPU pushes an error code on the stack for
- ISR_NOERROR is for the exceptions that the CPU doesn't push an error on the stack for *and* Normal interrupts, IRQ and anything other
    - It pushes a dummy error code for a consistent stack



ISR 255 is *always* reserved for APIC Spurious interrupts, *even* if for some reason the APIC driver fails
- Therefore the only thing it does is *immediately* 
        ```
            iretq
        ```
ISR 250 to 254 are *always* reserved for IPIs, *even* if there are no APs