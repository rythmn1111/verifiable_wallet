# Stub build: no quirc. Add REQUIRES "espressif__quirc" and full qr_decoder.c when enabling camera/QR.
idf_component_register(SRCS "qr_decoder.c"
                    INCLUDE_DIRS "include")
