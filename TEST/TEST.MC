    bits 16
    section .data

%define EOF* -1

struc FILE*
.fp        resb 2
.buf        resb 512
.ptr        resb 2
.inbuf        resb 2
.pos        resb                        4
endstruc

    section .text
PROCEDURE fopen*(name_ofs, name_seg, attr);
    IF ZFLAG THEN
        xor ax,ax
    ELSE
        xor ax,ax
        inc ax
    END;
END;

PROCEDURE fclose*(file_ofs, file_seg);
    mov cx, 5
    WHILE ZERO cx DO
        nop
        dec cx
    END;
END;


