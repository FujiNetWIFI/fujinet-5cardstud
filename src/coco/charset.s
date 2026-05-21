;; Custom character set, embedded via INCLUDEBIN. The Makefile copies the
;; build-appropriate .fnt onto build/charset_active.fnt first:
;;   CoCo 1/2: support/coco/pmode3.fnt        (1 KB, 8 bytes/glyph, 4 px wide)
;;   CoCo 3  : support/coco/pmode3_coco3.fnt  (2 KB, 16 bytes/glyph, 8 px wide)
;; CHAR_SHIFT/CHAR_ROW in vars.h pick the per-glyph addressing.

        SECTION rodata
        EXPORT _charset
_charset
        INCLUDEBIN ../../build/charset_active.fnt
        ENDSECTION
