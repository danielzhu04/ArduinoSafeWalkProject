//Watchdog code... credit to lab 4 my beautiful TAs- shout out to fuka, and alex 


void initWDT() {

  uint16_t wdtcr = 0; // read some where that it only allows one rewrite

  wdtcr |= (0b1000 << R_WDT_WDTCR_CKS_Pos); // PCKLB / 8192

  wdtcr |= (0b10 << R_WDT_WDTCR_TOPS_Pos); // 8192, ~2.5s timeout

  wdtcr |= (0b11 << R_WDT_WDTCR_RPSS_Pos); // pet the watchdog at any point of the timeout window
  wdtcr |= (0b11 << R_WDT_WDTCR_RPES_Pos);

  R_WDT->WDTCR = wdtcr;
  R_DEBUG->DBGSTOPCR_b.DBGSTOP_WDT = 0;
  R_WDT->WDTSR = 0;
  R_WDT->WDTRCR_b.RSTIRQS = 0; // changes here interrupt NOT a reset

 
  // configure interrupt
  R_ICU->IELSR[WDT_INT] = (0x25 << R_ICU_IELSR_IELS_Pos); //event
  NVIC_SetVector((IRQn_Type) WDT_INT, (uint32_t)&wdtISR);
  NVIC_SetPriority((IRQn_Type) WDT_INT, 15);
  NVIC_EnableIRQ((IRQn_Type) WDT_INT);//lab 4

  petWDT(); // do the first pet

}


//pet pet pet pet
void petWDT() {
  R_WDT->WDTRR = 0x00;   
  R_WDT->WDTRR = 0xFF;
}

//bark bark bark
void wdtISR() { // doing a manual reset so we can see the barking
  Serial.println("==========");
  Serial.println("WOOF!!! (Watchdog Interrupt)");
  Serial.println("==========");
  Serial.flush(); // make sure it is sent

  delay(100);

  R_WDT->WDTSR = 0;

  NVIC_SystemReset(); // manual reset instead of watchdog RSTIRQS bit
}
