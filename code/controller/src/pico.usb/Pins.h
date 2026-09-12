// Pins.h
//

enum Pin
{
    // --- BUS DATI & TRANSCEIVER 74LVC245 (Pico Pins 1-15) ---
    GP00 = 0, // IDE.DB0 (Lato A0 condiviso dai due 74LVC245)
    GP01,     // IDE.DB1 (Lato A1 condiviso dai due 74LVC245)
    GP02,     // IDE.DB2 (Lato A2 condiviso dai due 74LVC245)
    GP03,     // IDE.DB3 (Lato A3 condiviso dai two 74LVC245)
    GP04,     // IDE.DB4 (Lato A4 condiviso dai due 74LVC245)
    GP05,     // IDE.DB5 (Lato A5 condiviso dai due 74LVC245)
    GP06,     // IDE.DB6 (Lato A6 condiviso dai due 74LVC245)
    GP07,     // IDE.DB7 (Lato A7 condiviso dai due 74LVC245)

    GP08, // Transceiver 1 OE_LOW# (Abilita Byte Basso D0-D7)
    GP09, // Transceiver 2 OE_HIGH# (Abilita Byte Alto D8-D15)
    GP10, // Transceiver DIR (Direzione: 0 = Lettura Amiga->Pico, 1 = Scrittura Pico->Amiga)

    // --- BUS CONTROLLO IDE via YF08E / TXS0108E (Pico Pins 15-24) ---
    GP11, // IDE.CS0# (via YF08E Ch 1)
    GP12, // IDE.CS1# (via YF08E Ch 2)
    GP13, // IDE.A0 (via YF08E Ch 3)
    GP14, // IDE.A1 (via YF08E Ch 4)
    GP15, // IDE.A2 (via YF08E Ch 5)
    GP16, // IDE.DIOR# (via YF08E Ch 6)
    GP17, // IDE.DIOW# (via YF08E Ch 7)
    GP18, // IDE.RESET#(via YF08E Ch 8)

    // --- AUDIO I2S PCM5102A (Pico Pins 25-27) ---
    GP19, // PCMAudio.BCK (Bit Clock)
    GP20, // PCMAudio.LCK (L/R Word Select)
    GP21, // PCMAudio.DIN (Data In)

    // --- LETTORE SD CARD & INTERFACCIA (Pico Pins 29-34) ---
    GP22, // SDCardReader.miso
    GP23, // -- NC (Power control Wi-Fi su Pico W)
    GP24, // -- NC (VBUS sense su Pico W)
    GP25, // -- NC (LED integrato su Pico standard)
    GP26, // SDCardReader.clk/sck
    GP27, // SDCardReader.mosi
    GP28, // SDCardReader.cs
};