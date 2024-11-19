    bits 16
    section .text

PROCEDURE fopen*(fname_ofs, fname_seg);NEAR;
   mov   result, cx
END;

PROCEDURE fcreate*(fname_ofs, fname_seg);NEAR;
END;

PROCEDURE fwrite*(fhndl, buf_ofs, buf_seg, buf_len);NEAR;
END;

PROCEDURE fread*(fhndl, buf_ofs, buf_seg, buf_len);NEAR;
END;

PROCEDURE fclose*(file);NEAR;
END;

PROCEDURE kill*(fname_ofs, fname_seg);NEAR;
   mov   result, cx
END;

