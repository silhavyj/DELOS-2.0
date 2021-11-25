; https://wiki.osdev.org/Spinlock

[global spinlock_init]
spinlock_init:
    jmp spinlock_release

[global spinlock_acquire]
spinlock_acquire:
    mov eax, [esp + 4]       ; retrieve the address we wanna use as a lock

spin_wait:
    pause                    ; tell the CPU we're spinning
    test dword [eax], 1      ; test if the lock's free
    jnz spin_wait            ; if not, keep testing

spin_acquire:
    lock bts dword [eax], 0  ; attempt to acquire the lock
    jc spin_wait             ; spin if locked
    ret                      ; lock is successfully obtained

[global spinlock_release]
spinlock_release:
    mov eax, [esp + 4]       ; retrieve the address we wanna use as a lock
    mov dword [eax], 0       ; release the lock
    ret                      ; lock is successfully released