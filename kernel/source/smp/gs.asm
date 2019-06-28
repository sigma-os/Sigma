global get_current_cpu
get_current_cpu:
    mov rax, qword [gs:0]
    ret