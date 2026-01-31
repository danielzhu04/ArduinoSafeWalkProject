#include "controller.h"

int noteFrequencies[100];
int noteDurations[100];
int songLen = 0;
volatile int intcount = 0;

// copied from lab4
void initGPT() {
  R_MSTP->MSTPCRD_b.MSTPD6 = 0; // Enable GPT peripheral
  // Make sure the count isn't started
  R_GPT2->GTCR_b.CST = 0;
  // Make sure nobody else can start the count (see 22.2.5 and 22.2.6)
  R_GPT2->GTSSR = (1 << R_GPT0_GTSSR_CSTRT_Pos); // only started w/ software
  R_GPT2->GTPSR = (1 << R_GPT0_GTPSR_CSTOP_Pos); // only stopped w/ software

  // TODO step 1 (Prelab Q5.1): Divide the GPT2 clock
  // IMPORTANT: because of a versioning issue between Arduino and Renesas, use the actual bit position *number* from the datasheet
  // instead of the field name macro defined in the header file
  // See https://github.com/arduino/ArduinoCore-renesas/issues/354 for more info
  R_GPT2->GTCR = (0b010 << 24);

  // Disable GPT interrupt on ICU for now
  R_ICU->IELSR[TIMER_INT] = 0;
  // Use the Arm CMSIS API to enable CPU interrupts
  // (overall GPT interrupt is enabled only when notes play; see playNote)
  NVIC_SetVector((IRQn_Type) TIMER_INT, (uint32_t) &gptISR); // set vector entry to our handler
  NVIC_SetPriority((IRQn_Type) TIMER_INT, 15); // Priority lower than Serial (12). Make it 15 so that wifi packets arent affected
  NVIC_EnableIRQ((IRQn_Type) TIMER_INT);

  // TODO step 4:
  initNoteGPT();
  
  // for our sanity
  Serial.println("We were able to init the GPT");
}


// copied from lab4
/*
 * Sets configures GPT2 interrupt to play note at desired frequency
 * Non-blocking: note can play while program executes
 */
void playNote(int freq) {
   if (freq == 0) {
    stopPlay();
    return;
   }
  // Turn off counting while configuring
  R_GPT2->GTCR_b.CST = 0;

  // TODO step 1: Configure GPT2 to count to correct n
  // Reference prelab Q3.1 for calculation of n
  // Reference prelab Q5.2 for the GPT2 register
  // Remember that the clock frequency is stored in the constant CLOCKFREQ
  R_GPT2->GTPR = CLOCKFREQUENCY / (2.0 * freq);

  // TODO step 1 (Prelab Q5.4):
  // Configure the ICU to connect the GPT interrupt to the CPU interrupt to enable interrupt
  R_ICU->IELSR[TIMER_INT] = (0x06D << R_ICU_IELSR_IELS_Pos);

  // TODO step 1: re-enable the GPT count
  R_GPT2->GTCR_b.CST = 1;

}


/* Stop playing a note */
void stopPlay() {
  // TODO step 1: Turn off GPT counter and disable CPU interrupt via ICU
  R_GPT2->GTCR_b.CST = 0;
  R_ICU->IELSR[TIMER_INT] = 0;
  
  // TODO step 2: Turn off pin
  R_PFS->PORT[PIEZO_PORT].PIN[PIEZO_PIN_NUM].PmnPFS_b.PODR = 0;
}

/* ISR that toggles a GPIO pin low/high at a configured frequency */
void gptISR() {
  intcount++; // USED FOR TESTING: DO NOT REMOVE

  // Toggle output pin
  R_PFS->PORT[PIEZO_PORT].PIN[PIEZO_PIN_NUM].PmnPFS_b.PODR ^= 1;
  
  // Restart count
  R_GPT2->GTCR_b.CST = 1;

  // Clear interrupt on MCU and CPU side
  R_ICU->IELSR_b[TIMER_INT].IR = 0;
  NVIC_ClearPendingIRQ((IRQn_Type) TIMER_INT);
}

/* TODO step 4: Enable interrupt to control which notes are played when */
void initNoteGPT() {
  // Setp up GPT3 (similarly to what initGPT did for GPT2)
  // kick off first noteISR by setting a counter value and turning on the counter

// dont have to do this because done n initGPT
  // R_MSTP->MSTPCRD_b.MSTPD6 = 0; // Enable GPT peripheral
  // Make sure the count isn't started
  R_GPT3->GTCR_b.CST = 0;
  // Make sure nobody else can start the count (see 22.2.5 and 22.2.6)
  R_GPT3->GTSSR = (1 << R_GPT0_GTSSR_CSTRT_Pos); // only started w/ software
  R_GPT3->GTPSR = (1 << R_GPT0_GTPSR_CSTOP_Pos); // only stopped w/ software

  // TODO step 1 (Prelab Q5.1): Divide the GPT2 clock
  // IMPORTANT: because of a versioning issue between Arduino and Renesas, use the actual bit position *number* from the datasheet
  // instead of the field name macro defined in the header file
  // See https://github.com/arduino/ArduinoCore-renesas/issues/354 for more info
  R_GPT3->GTCR = (0b101 << 24);

  // Disable GPT interrupt on ICU for now
  // dont use _b here because we are not shifting into a bit
  R_ICU->IELSR[NOTE_INT] = (0x075 << R_ICU_IELSR_IELS_Pos);
  // Use the Arm CMSIS API to enable CPU interrupts
  // (overall GPT interrupt is enabled only when notes play; see playNote)
  NVIC_SetVector((IRQn_Type) NOTE_INT, (uint32_t) &noteISR); // set vector entry to our handler
  NVIC_SetPriority((IRQn_Type) NOTE_INT, 15); // Priority lower than Serial (12)
  NVIC_EnableIRQ((IRQn_Type) NOTE_INT);

  // find largesrt divisor that can fit 750ms

  // R_GPT3->GTPR = (2^16) / (48*1000000 / 1024) / 1.864135111;
  // // R_GPT3->GTPR = ((3000000/64) * noteDurations[0]) / 1000.0;
  // R_GPT3->GTCR_b.CST=1;
}

/* ISR to control the song sequence */
void noteISR() {
  static int idx = 0;
  
  // stop the GPT3 counter
  R_GPT3->GTCR_b.CST = 0;
  
  // stop current note and play the next note (just like loop/playNoteDuration did)
  stopPlay();

  // set up the next noteISR by setting a counter value and turning on GPT3 counter
  if (idx < songLen) {
    if (noteFrequencies[idx] != 0) {
      playNote(noteFrequencies[idx]);
    }
    // Set duration for this note (46875 = 48MHz / 1024)
    R_GPT3->GTPR = (46875 * noteDurations[idx]) / 1000.0;
    
    idx++;
    
    // start gp3 counter again
    R_GPT3->GTCR_b.CST = 1;
  } else {
    // Sequence finished, reset index
    idx = 0;
  }
  
  // clear pending flags
  R_ICU->IELSR_b[NOTE_INT].IR = 0;
  NVIC_ClearPendingIRQ((IRQn_Type) NOTE_INT);
}

// replace blocking playTone(). Uses noteISR to play tones for speakers
void playToneSequenceISR(String song) {
  songLen = rtttlToBuffers(song, noteFrequencies, noteDurations);
  
  if (songLen < 0) {
    Serial.println("RTTTL parse error");
    return;
  }
  
  // stop current sound
  R_GPT3->GTCR_b.CST = 0;
  stopPlay();
  
  // call noteisr
  noteISR();
}