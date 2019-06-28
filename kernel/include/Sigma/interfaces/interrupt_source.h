#ifndef SIGMA_KERNEL_INTERFACES_INTERRUPT_SOURCE
#define SIGMA_KERNEL_INTERFACES_INTERRUPT_SOURCE

#include <Sigma/common.h>

class IInterruptController {
    public:
        virtual void init() = 0;
        virtual void set_base_vector(uint8_t base) = 0;
        virtual void deinit() = 0;

        virtual void enable() = 0;
        virtual void disable() = 0;

        virtual void send_eoi() = 0;

    protected:
        ~IInterruptController() { }
};

#endif