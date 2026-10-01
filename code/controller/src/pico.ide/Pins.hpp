// Pins.h
//
#pragma once

enum Pin
{
    // --- BUS DATI & TRANSCEIVER 74LVC245 (Pico Pins 1-15) ---
    GP00 = 0, // IDE.DB0 - IDE.DB8  (condiviso da U1(74LVC245) e U2 (74LVC573))
    GP01,     // IDE.DB1 - IDE.DB9  (condiviso da U1(74LVC245) e U2 (74LVC573))
    GP02,     // IDE.DB2 - IDE.DB10 (condiviso da U1(74LVC245) e U2 (74LVC573))
    GP03,     // IDE.DB3 - IDE.DB11 (condiviso da U1(74LVC245) e U2 (74LVC573))
    GP04,     // IDE.DB4 - IDE.DB12 (condiviso da U1(74LVC245) e U2 (74LVC573))
    GP05,     // IDE.DB5 - IDE.DB13 (condiviso da U1(74LVC245) e U2 (74LVC573))
    GP06,     // IDE.DB6 - IDE.DB14 (condiviso da U1(74LVC245) e U2 (74LVC573))
    GP07,     // IDE.DB7 - IDE.DB15 (condiviso da U1(74LVC245) e U2 (74LVC573))

    GP08, // U1 Byte basso DIR (Low = Lettura Amiga->Pico, High = Scrittura Pico->Amiga)
    GP09, // U1 Byte basso OE# (Controllo U1 - HIGH = Disabilitato, LOW = Abilitato)
    GP10, // U2 Byte alto OE# (Controllo U2 - HIGH = Disabilitato, LOW = Abilitato)

    GP11, // U3 LE (Impulso HIGH = blocca il byte alto nel latch)
    GP12, // U3 OE# (Controllo U3 LOW = Spinge il byte alto dal latch al bus IDE)

    GP13, // U4 -> IDE.DIOR# (Segnale di lettura IDE via 74LVC245)
    GP14, // U4 -> IDE.DIOW# (Segnale di scrittura IDE via 74LVC245)
    GP15, // U4 -> IDE.CS0# (Chip Select 0 via transceiver 74LVC245 lato Pico)
    GP16, // U4 -> IDE.CS1# (Chip Select 1 via transceiver 74LVC245 lato Pico)
    GP17, // U4 -> IDE.A0 (Indirizzo A0 via transceiver 74LVC245 lato Pico)
    GP18, // U4 -> IDE.A1 (Indirizzo A1 via transceiver 74LVC245 lato Pico)
    GP19, // U4 -> IDE.A2 (Indirizzo A2 via transceiver 74LVC245 lato Pico)
    GP20, // U4 -> IDE.RESET# (Segnale di reset IDE via 74LVC245)

    GP21, // Pico -> IDE.IORDY (Ready/Busy del device verso l'host)

    GP22, // SDCardReader.CS
    GP23, // -- NC (Power control Wi-Fi su Pico W)
    GP24, // -- NC (VBUS sense su Pico W)
    GP25, // -- NC (LED integrato su Pico standard)
    GP26, // SDCardReader.SCK
    GP27, // SDCardReader.MOSI
    GP28, // SDCardReader.MISO
};