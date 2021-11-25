[global exit]
exit:
    mov     ebx, [esp + 4]  ; ebx = exit code
    mov     eax, 100        ; 100 = system call number (exit)
    int     0x80            ; call the interrupt (0x80 = system calls)
    ret                     ; return

[global printf]
printf:
    mov     esi, [esp + 4]  ; esi = address of the string to be printed out
    mov     eax, 101        ; 101 = system call number (printf)
    int     0x80            ; call the interrupt (0x80 = system calls)
    ret                     ; return

[global read_line]
read_line:
    mov     edi, [esp + 4]  ; edi = address of the buffer
    mov     eax, 102        ; 102 = system call number (read_line)
    int     0x80            ; call the interrupt (0x80 = system calls)
    ret                     ; return

[global malloc]
malloc:
    mov     ebx, [esp + 4]  ; ebx = number of bytes to be allocated
    mov     eax, 103        ; 103 = system call number (malloc)
    int     0x80            ; call the interrupt (0x80 = system calls)
    ret                     ; return

[global free]
free:
    mov     ebx, [esp + 4]  ; ebx = address to the mem we want to free
    mov     eax, 104        ; 104 = system call number (free)
    int     0x80            ; call the interrupt (0x80 = system calls)
    ret                     ; return

[global get_pid]
get_pid:
    mov     eax, 105        ; 105 = system call number (get_pid)
    int     0x80            ; call the interrupt (0x80 = system calls)
    ret                     ; return

[global get_ppid]
get_ppid:
    mov     eax, 106        ; 106 = system call number (get_pid)
    int     0x80            ; call the interrupt (0x80 = system calls)
    ret                     ; return

[global exec]
exec:
    mov     ebx, [esp + 4]  ; ebx = name of the program to be executed
    mov     eax, 107        ; 107 = system call number (exec)
    int     0x80            ; call the interrupt (0x80 = system calls)
    ret                     ; return

[global open]
open:
    mov     ebx, [esp + 4]  ; ebx = name of the program to be executed
    mov     eax, 109        ; 109 = system call number (open)
    int     0x80            ; call the interrupt (0x80 = system calls)
    ret                     ; return

[global close]
close:
    mov     ebx, [esp + 4]  ; ebx = name of the program to be executed
    mov     eax, 110        ; 110 = system call number (close)
    int     0x80            ; call the interrupt (0x80 = system calls)
    ret                     ; return

[global read]
read:
    mov     esi, [esp + 4]   ; filename
    mov     ebx, [esp + 8]   ; buffer
    mov     ecx, [esp + 12]  ; offset
    mov     edx, [esp + 16]  ; len
    mov     eax, 111         ; 111 = system call number (read)
    int     0x80             ; call the interrupt (0x80 = system calls)
    ret                      ; return

[global write]
write:
    mov     esi, [esp + 4]   ; filename
    mov     ebx, [esp + 8]   ; buffer
    mov     ecx, [esp + 12]  ; offset
    mov     edx, [esp + 16]  ; len
    mov     eax, 112         ; 112 = system call number (write)
    int     0x80             ; call the interrupt (0x80 = system calls)
    ret                      ; return

[global touch]
touch:
    mov     ebx, [esp + 4]   ; ebx = name of the file to be touched
    mov     eax, 113         ; 113 = system call number (touch)
    int     0x80             ; call the interrupt (0x80 = system calls)
    ret                      ; return

[global ls]
ls:
    mov     eax, 114         ; 114 = system call number (ls)
    int     0x80             ; call the interrupt (0x80 = system calls)
    ret                      ; return

[global cat]
cat:
    mov     ebx, [esp + 4]   ; ebx = name of the file to be printed out
    mov     eax, 115         ; 115 = system call number (cat)
    int     0x80             ; call the interrupt (0x80 = system calls)
    ret                      ; return

[global cp]
cp:
    mov     ebx, [esp + 4]   ; ebx = filename1 (src)
    mov     ecx, [esp + 8]   ; ecx = filename2 (des)
    mov     eax, 116         ; 116 = system call number (cp)
    int     0x80             ; call the interrupt (0x80 = system calls)
    ret                      ; return

[global rm]
rm:
    mov     ebx, [esp + 4]   ; ebx = name of the file to be deleted
    mov     eax, 117         ; 117 = system call number (rm)
    int     0x80             ; call the interrupt (0x80 = system calls)
    ret                      ; return

[global ps]
ps:
    mov     eax, 118         ; 118 = system call number (ps)
    int     0x80             ; call the interrupt (0x80 = system calls)
    ret                      ; return

[global lp]
lp:
    mov     eax, 119         ; 119 = system call number (lp)
    int     0x80             ; call the interrupt (0x80 = system calls)
    ret                      ; return

[global fork]
fork:
    mov     eax, 120         ; 120 = system call number (fork)
    int     0x80             ; call the interrupt (0x80 = system calls)
    ret                      ; return

[global wait_for_child]
wait_for_child:
    mov     ebx, [esp + 4]   ; ebx = child's pid
    mov     eax, 121         ; 121 = system call number (wait)
    int     0x80             ; call the interrupt (0x80 = system calls)
    ret                      ; return

[global clear_screen_command]
clear_screen_command:
    mov     eax, 122         ; 122 = clear screen command
    int     0x80             ; Call the interrupt (0x80 = system calls)
    ret                      ; return

[global get_last_process_return_value]
get_last_process_return_value:
    mov     eax, 123         ; 123 = return last process exit code
    int     0x80             ; Call the interrupt (0x80 = system calls)
    ret                      ; return

[global file_append]
file_append:
    mov     ebx, [esp + 4]   ; ebx = filename
    mov     ecx, [esp + 8]   ; ecx = buffer to append
    mov     eax, 124         ; 124 = system call number (file_append)
    int     0x80             ; call the interrupt (0x80 = system calls)
    ret                      ; return

[global color_screen_command]
color_screen_command:
    mov     ebx, [esp + 4]   ; ebx = screen foreground color
    mov     ecx, [esp + 8]   ; ecx = screen background color
    mov     eax, 125         ; 125 = system call number (color screen)
    int     0x80             ; call the interrupt (0x80 = system calls)
    ret                      ; return

[global set_cursor_command]
set_cursor_command:
    mov     ebx, [esp + 4]   ; ebx = screen x axis
    mov     ecx, [esp + 8]   ; ecx = screen y axis
    mov     eax, 126         ; 126 = system call number (set cursor)
    int     0x80             ; call the interrupt (0x80 = system calls)
    ret                      ; return