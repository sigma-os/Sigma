#pragma once

#include <sys/mman.h>
#include <libsigma/sys.h>
#include <libsigma/virt.h>
#include <stdint.h>
#include <stdio.h>

struct rm_regs {
    uint32_t eflags;
    uint32_t ebp;
    uint32_t edi;
    uint32_t esi;
    uint32_t edx;
    uint32_t ecx;
    uint32_t ebx;
    uint32_t eax;
};

struct rm_cpu {
    public:
    rm_cpu(){
        vspace = vctl(vCtlCreateVspace, 0, 0, 0, 0);

        vctl(vCtlMapVspacePhys, vspace, 0x0, 0x0, 0xFFFFFFFF); // Map 4GiB of ram into the VM

        auto* ram = libsigma_vm_map(0x8000, NULL, NULL, PROT_READ | PROT_WRITE, MAP_ANON);

        payload = ((uint8_t*)ram + 0x2C00);

        data_section_addr = 0x7d00;
        data_section = ((uint8_t*)ram + 0x2D00);

        vctl(vCtlMapVspace, vspace, 0x5000, (uint64_t)ram, 0x8000);

        vcpu = vctl(vCtlCreateVcpu, 0, 0, 0, 0);
    }

    void intn(int n, rm_regs& regs){
        /*
            int n
            vmmcall
        */

        payload[0] = 0xCD;
        payload[1] = n;

        payload[2] = 0x0F;
        payload[3] = 0x01;
        payload[4] = 0xd9;

        vregs cpu_regs{};

        vctl(vCtlGetRegs, vcpu, (uint64_t)&cpu_regs, 0, 0);

        cpu_regs.rax = regs.eax;
        cpu_regs.rbx = regs.ebx;
        cpu_regs.rcx = regs.ecx;
        cpu_regs.rdx = regs.edx;
        cpu_regs.rsi = regs.esi;
        cpu_regs.rdi = regs.edi;

        cpu_regs.rflags = regs.eflags;

        cpu_regs.ds.limit = 0xFFFFFFFF;
        cpu_regs.es.limit = 0xFFFFFFFF;
        cpu_regs.fs.limit = 0xFFFFFFFF;
        cpu_regs.gs.limit = 0xFFFFFFFF;
        cpu_regs.ss.limit = 0xFFFFFFFF;

        cpu_regs.cs.selector = 0;
        cpu_regs.cs.base = 0;
        cpu_regs.rip = 0x7C00;

        cpu_regs.rsp = 0x7C00;

        vctl(vCtlSetRegs, vcpu, (uint64_t)&cpu_regs, 0, 0);

        // Setup state, now run

        vexit exit = {};
        while(1){
            vctl(vCtlRunVcpu, vcpu, (uint64_t)&exit, 0, 0);

            /*printf("Exit reason: %lx\nOpcode: ", exit.reason);
        
            for(int i = 0; i < exit.opcode_length; i++)
                printf("0x%02X ", exit.opcode[i]);

            printf("\n");*/

            if(exit.reason == vCtlExitReasonInterrupt){
                printf("Interrupt: vector: %02x\n", exit.interrupt_number);
                break;
            } else if(exit.reason == vCtlExitReasonPortRead){
                switch(exit.opcode[0]){
                    default:
                        printf("Unknown PIO IN opcode: %x\n", exit.opcode[0]);
                        while(1)
                            asm("pause");
                }
            } else if(exit.reason == vCtlExitReasonPortWrite){
                switch(exit.opcode[0]){
                    default:
                        printf("Unknown PIO OUT opcode: %x\n", exit.opcode[0]);
                        while(1)
                            asm("pause");
                }
            } else if(exit.reason == vCtlExitReasonHypercall)
                break;
        }

        vctl(vCtlGetRegs, vcpu, (uint64_t)&cpu_regs, 0, 0);

        regs.eax = cpu_regs.rax;
        regs.ebx = cpu_regs.rbx;
        regs.ecx = cpu_regs.rcx;
        regs.edx = cpu_regs.rdx;
        regs.esi = cpu_regs.rsi;
        regs.edi = cpu_regs.rdi;

        regs.eflags = cpu_regs.rflags;
    }
    uint8_t* data_section = nullptr;
    uint32_t data_section_addr = 0;

    private:

    uint64_t vspace, vcpu;

    uint8_t* payload;
};