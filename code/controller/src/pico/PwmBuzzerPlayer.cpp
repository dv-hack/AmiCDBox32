// PwmBuzzerPlayer.cpp
//
#include "PwmBuzzerPlayer.hpp"

#include "Disc.hpp"
#include "Sector.hpp"

#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include "hardware/dma.h"
#include "hardware/clocks.h"
#include "pico/time.h"

static inline void initialize( uint32_t pin, int &dmaChannel )
{
    gpio_set_function(pin, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(pin);

    // 1. Configurazione PWM (Diventa il nostro metronomo a 44.1kHz)
    pwm_config config = pwm_get_default_config();
    
    // Frequenza PWM = SysClock / (clkdiv * (wrap + 1))
    // Vogliamo 44100 Hz esatti con un wrap di 255 (risoluzione 8-bit).
    float sys_clk = static_cast<float>(clock_get_hz(clk_sys));
    float clkdiv = sys_clk / (44100.0f * 256.0f); 
    
    pwm_config_set_clkdiv(&config, clkdiv);
    pwm_config_set_wrap(&config, 255);    
    pwm_init(slice_num, &config, true);
    pwm_set_gpio_level(pin, 128); // Segnale centrale (silenzio)

    // 2. Configurazione DMA
    dmaChannel = dma_claim_unused_channel(true);
    dma_channel_config dma_config = dma_channel_get_default_config(dmaChannel);
    
    channel_config_set_transfer_data_size(&dma_config, DMA_SIZE_32); // Scriviamo 32 bit per aggiornare il registro CC
    channel_config_set_read_increment(&dma_config, true);            // Avanza nel nostro _sampleBuffer
    channel_config_set_write_increment(&dma_config, false);          // Scrivi sempre nello stesso registro del PWM
    channel_config_set_dreq(&dma_config, pwm_get_dreq(slice_num));   // Sincronizza il trasferimento con il wrap del PWM

    dma_channel_configure(
        dmaChannel,
        &dma_config,
        &pwm_hw->slice[slice_num].cc, // Indirizzo di destinazione: il registro Counter Compare del PWM
        nullptr,                      // Indirizzo sorgente (verrà impostato in feedSector)
        588,                          // Numero di trasferimenti (i campioni in un settore)
        false                         // Non avviare ancora
    );
}

static inline void deinitialize( uint32_t pin, int &dmaChannel )
{
    if (dmaChannel >= 0)
    {
        dma_channel_abort(dmaChannel);
        dma_channel_unclaim(dmaChannel);
        dmaChannel = -1;
    }

    uint slice_num = pwm_gpio_to_slice_num(pin);
    pwm_set_enabled(slice_num, false);
    
    gpio_set_function(pin, GPIO_FUNC_SIO);
    gpio_set_dir(pin, GPIO_OUT);
    gpio_put(pin, 0); // Evita ronzii a riposo
}


// ------------------------------------------------------------------------------------------------
PwmBuzzerPlayer::PwmBuzzerPlayer(uint32_t dacPin)
: _dacPin(dacPin) 
{
    initialize( _dacPin, _dmaChannel );
}

// ------------------------------------------------------------------------------------------------
PwmBuzzerPlayer::~PwmBuzzerPlayer()
{
    deinitialize(_dacPin, _dmaChannel );
}

// ------------------------------------------------------------------------------------------------
void PwmBuzzerPlayer::startPlayback( Disc *disc, uint32_t startLBA, uint32_t endLBA )
{
    if( !disc )
        return;

    _disc = disc;
    _currentBuffer = 0;
    _currentAudioLba = startLBA;
    _targetAudioLba = endLBA;   
}

// ------------------------------------------------------------------------------------------------
void PwmBuzzerPlayer::stopPlayback()
{
    _disc = nullptr;
    _currentAudioLba = 0;
    _targetAudioLba = 0;
    _currentBuffer  = 0;

    deinitialize( _dacPin, _dmaChannel );
}

// ------------------------------------------------------------------------------------------------
bool PwmBuzzerPlayer::playFrame()
{
    if( !_disc)
        return false;

    if (_currentAudioLba >= _targetAudioLba) 
    {
        stopPlayback();
        return false;
    }

    uint8_t rawSector[SectorSize::RAW];
    if ( !_disc->readSectors(_currentAudioLba, 1, SectorSize::RAW,rawSector, SectorSize::RAW) ) 
    {
        stopPlayback();
        return false;
    }

    play(rawSector);
    _currentAudioLba++;
    return true;
}

// ------------------------------------------------------------------------------------------------
void PwmBuzzerPlayer::play(const uint8_t *sectorData)
{
    uint chan = pwm_gpio_to_channel(_dacPin);
    uint32_t* currentBuf = _sampleBuffer[_currentBuffer];

    // 1. Processa i bytes nel buffer attualmente "libero"
    int sampleIdx = 0;
    for (int i = 0; i < SectorSize::RAW; i += 4)
    {
        const int16_t left = static_cast<int16_t>(sectorData[i] | (sectorData[i + 1] << 8));
        const int16_t right = static_cast<int16_t>(sectorData[i + 2] | (sectorData[i + 3] << 8));

        const int32_t mono = (left + right) / 2;
        const uint8_t sample8 = static_cast<uint8_t>((mono >> 8) + 128);

        // Il registro 'cc' a 32-bit controlla il Canale A (bit 0-15) e il Canale B (bit 16-31).
        // Mettiamo il byte nella posizione giusta in base a quale canale è assegnato il pin.
        if (chan == PWM_CHAN_B) {
            currentBuf[sampleIdx] = static_cast<uint32_t>(sample8) << 16;
        } else {
            currentBuf[sampleIdx] = static_cast<uint32_t>(sample8);
        }
        sampleIdx++;
    }

    // 2. Sincronizzazione hardware-software
    // Aspettiamo che il DMA finisca di suonare il buffer precedente.
    // Questo allinea magicamente il nostro ciclo a 75Hz (che legge FatFs) 
    // con il tempo reale di riproduzione (13.33ms), evitando sia blocchi CPU sia desincronizzazioni.
    dma_channel_wait_for_finish_blocking(_dmaChannel);

    // 3. Spara il nuovo buffer verso il DAC in background
    dma_channel_transfer_from_buffer_now(_dmaChannel, currentBuf, 588);

    // 4. Cambia buffer per il prossimo settore (Ping-Pong)
    _currentBuffer = 1 - _currentBuffer;
}

// ------------------------------------------------------------------------------------------------
uint32_t PwmBuzzerPlayer::currentLBA() const { return _currentAudioLba; }
bool     PwmBuzzerPlayer::isPlaying() const { return _disc != nullptr; }