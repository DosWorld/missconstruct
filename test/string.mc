    bits 16
    section .text
PROCEDURE strlen*(str_ofs, str_seg);
    push ds
    lds  si, [str_ofs]
    cld
    xor  cx, cx
    WHILE cx NE 0 DO
         lodsb
         IF AL E 0 THEN
           BREAK;
         END;
         inc  cx
    END;
   pop   ds
   mov   result, cx
END;
