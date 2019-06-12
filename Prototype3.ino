#include <Adafruit_NeoPixel.h>
#include <RotaryEncoder.h>

#define PIN_LEDSTRIP 6  //Pin på led-stripen

#define PIN_ENCODER_A 12 //En pin på den fysiske rotary encoder-en
#define PIN_ENCODER_B 13 //Den andre pin-en på den fysiske rotary encoder-en
#define BTN_ENCODER 7 //Knappen på rotary encoder-en. Denne blir brukt som en OK-knapp.

#define BTN_EMDR 5 //Knappen man trykker på for å aktivere tilstanden for EMDR (og avslutte den)
#define BTN_SAVE 4 //Knappen man trykker på for å aktivere tilstanden for å lagre stemningsrapport (og avslutte den)
#define BTN_SEND 3 //Knappen man trykker på for å aktivere tilstanden for å sende en stemningsrapport til ansatte (og avslutte den)
#define BTN_SAMTALE 2 //Knappen man trykker på for å sende en beskjed til ansatte om at man ønsker en samtale

#define NUMBER_OF_LEDS 49  //Antall lys som skal være på led-stripen

//Led-stripen
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMBER_OF_LEDS, PIN_LEDSTRIP, NEO_GRB + NEO_KHZ800);


/**
 * Det er to rotary encoder-e under som begge bruker de samme pins-ene.
 * Dette er fordi selv om de begge bruker den samme fysiske komponenten, så
 * ønsker man at man skal kunne lagre en verdi for EMDR-komponentet og en verdi
 * for stemningsrapport-komponentet som om de var to ulike fysiske komponenter.
 */
//Den virtuelle rotary encoder-en for EMDR
RotaryEncoder encoderEMDR = RotaryEncoder(PIN_ENCODER_A, PIN_ENCODER_B);

//Den virtuelle rotary encoder-en for stemningsrapporten
RotaryEncoder encoderMood = RotaryEncoder(PIN_ENCODER_A, PIN_ENCODER_B);


/**
 * Dette er en enumerator. Det er et heltall som er gjemt inni et navn.
 * I dette tilfellet har vi sju enum-er;
 * 0 = Standby
 * 1 = EMDR
 * 2 = Mood
 * 3 = Alarm
 * 4 = MoodSend
 * 5 = Sending
 * 6 = NotSending
 * 
 * Når brukeren interagerer med enheten på bestemte måter,
 * vil tilstanden på enheten endre seg tilsvarende, og kan gjøre at
 * artefakten oppfører seg på ulike måter.
 */
//Enum for tilstanden på enheten
enum State
{
  Standby,
  EMDR,
  Mood,
  Alarm,
  MoodSend,
  Sending,
  NotSending
};

State state = State::Standby;

uint16_t stripColour = strip.Color(0, 0, 100);


//Enum for tilstanden på EMDR-en
enum StateEMDR
{
  waitingForWheelInput,
  runningLights
};

StateEMDR emdrState = StateEMDR::waitingForWheelInput;


void setup()
{
  //Setter opp knappene som input med en internt pullup-resistor
  pinMode(BTN_EMDR, INPUT_PULLUP);
  pinMode(BTN_SAVE, INPUT_PULLUP);
  pinMode(BTN_ENCODER, INPUT_PULLUP);
  pinMode(BTN_SEND, INPUT_PULLUP);
  pinMode(BTN_SAMTALE, INPUT_PULLUP);

  strip.begin();

  Serial.begin(9600);
  Serial.println("Device started...");
}


/**
 * Dette er løkke-metoden som styrer enheten. Man sjekker tilstander for å 
 * endre oppførselen på artefakten. 
 */
void loop()
{ 
  if (state == State::Standby)
  {
    prynt("State: Standby");
    CheckActivateEMDR();
    CheckPressAlarm();
    CheckPressMood();
    CheckPressMoodSend();
    DoStandby();
  }
  else if (state == State::EMDR)
  {
    prynt("State: EMDR");
    DoEMDRProcedure();
    CheckDeactivateEMDR();
    CheckPressAlarm();
  }
  else if (state == State::Mood)
  {
    prynt("State: Mood");
    CheckPressAlarm();
    CheckFeelWheel();
    CheckSaveMood();
    CheckCancelMood();
  }
  else if (state == State::MoodSend)
  {
    prynt("State: Mood");
    CheckPressAlarm();
    CheckFeelWheel();
    CheckSendMood();
    CheckCancelMood();
  }
  else if (state == State::Sending)
  {
    prynt("State: Sending");
    SendingAnimation();
  }
  else if (state == State::NotSending)
  {
    NotSendAnimation();
  }
  else if (state == State::Alarm)
  {
    prynt("State: Alarm");
    SendAlarm();
    //CheckDeactivateAlarm();
  }
}


/**
 * Metodene under er for å sjekke knappetrykk, og endre tilstand
 * på enheten basert på hva som blir trykket.
 */
void CheckActivateEMDR()
{
  if (!digitalRead(BTN_EMDR))
  {
    if (DebounceDelay(300))
    {
      state = State::EMDR;
      emdrState = StateEMDR::waitingForWheelInput;
    }
  }
}

void CheckDeactivateEMDR()
{
  if (!digitalRead(BTN_EMDR))
  {
    if (DebounceDelay(300))
    {
      state = State::Standby;
    }
  }
}

void CheckPressMood()
{
  if (!digitalRead(BTN_SAVE))
  {
    if (DebounceDelay(300))
    {
      state = State::Mood;
    }
  }
}

void CheckPressMoodSend()
{
  if (!digitalRead(BTN_SEND))
  {
    if (DebounceDelay(300))
    {
      state = State::MoodSend;
    }
  }
}

void CheckFeelWheel()
{
  int moodNumber = GetLatestEncoderMoodValue();
  ShowNumberOnLEDStrip(moodNumber);
  prynt("Følelsestall: " + String(moodNumber));
}

void CheckSaveMood()
{
  if (!digitalRead(BTN_ENCODER))
  {
    if (DebounceDelay(300))
    {
      state = State::Sending;
    }
  }
}

void CheckSendMood()
{
  if (!digitalRead(BTN_ENCODER))
  {
    if (DebounceDelay(300))
    {
      state = State::Sending; 
    }
  }
}

void CheckCancelMood()
{
  if (!digitalRead(BTN_SEND) || !digitalRead(BTN_SAVE))
  {
    if (DebounceDelay(300))
    {
      state = State::NotSending;
    }
  }
}

void CheckPressAlarm()
{
  if (!digitalRead(BTN_SAMTALE))
  {
    if (DebounceDelay(300))
    {
      state = State::Alarm;
    }
  }
}

void SendAlarm()
{
  if (DebounceDelay(10000))
  {
    state = State::Standby;
  }
  PulseLights();
}

void CheckDeactivateAlarm()
{
  if (!digitalRead(BTN_SAMTALE))
  {
    if (DebounceDelay(300))
    {
      state = State::Standby;
    }
  }
}


 /**
 * Ser på encoderEMDR og returnerer dens verdi. Metoden clamp-er tallet slik
 * at man ikke kan få verdier over ti, eller verdier under én.
 */
static int GetLatestEncoderEMDRValue()
{
  encoderEMDR.tick(); //Looks at the position of the encoder and if it has changed

  int newPos = encoderEMDR.getPosition();
  
  if (newPos < 1)
  {
    encoderEMDR.setPosition(1);
    return 1;
  }
  
  if (newPos > 10)
  {
    encoderEMDR.setPosition(10);
    return 10;
  }
  
  return newPos;
}


/**
 * Ser på encoderMood og returnerer dens verdi. Metoden clamp-er tallet slik
 * at man ikke kan få verdier over ti, eller verdier under én.
 */
static int GetLatestEncoderMoodValue()
{
  encoderMood.tick(); //Looks at the position of the encoder and if it has changed

  int newPos = encoderMood.getPosition();
  
  if (newPos < 1)
  {
    encoderMood.setPosition(1);
    return 1;
  }
  
  if (newPos > 10)
  {
    encoderMood.setPosition(10);
    return 10;
  }
  
  return newPos;
}


/**
 * Denne metoden tar inn et nummer og viser fram like mange
 * led-lys på led-stripen. I tillegg til dette endrer den fargen på økende 
 * led-lys. De første fem lysene er grønne, de neste tre er gule og de to siste 
 * er røde.
 */
static void ShowNumberOnLEDStrip(int number)
{
  strip.clear();

  for (int i = 0; i < number; i++)
  {
    if (i <= 4)
    {
      strip.setPixelColor(i, strip.Color(0, 100, 0));
    }
    else if (i >= 5 && i <= 7)
    {
      strip.setPixelColor(i, strip.Color(255, 255, 0));
    }
    else if (i >= 8)
    {
      strip.setPixelColor(i, strip.Color(255, 0, 0));
    }
  }

  strip.show();
  
  //prynt("Skrudde på " + String(number) + " antall lys!");
}


/**
 * Hva Arduino-en gjør når man er i Standby-tilstanden.
 */
static void DoStandby()
{
  strip.clear();
  strip.show();
}


/**
 * Hoved-metoden som skrur på hele EMDR-prosedyren, inkludert å la brukeren
 * stille inn deres følelser på en skala fra en til ti, og å kjøre
 * behandlingen i et minutt av gangen.
 */
static void DoEMDRProcedure()
{
  //Hvis EMDR-tilstanden er i waitingForWheelInput-tilstand vil man 
  // vente på bruker-input.
  if (emdrState == waitingForWheelInput)
  {
    int feelingNumber = GetLatestEncoderEMDRValue();
    //prynt("Skala fra 1 til 10: " + String(feelingNumber));
    ShowNumberOnLEDStrip(feelingNumber);
    if (!digitalRead(BTN_ENCODER))
    {
      if (DebounceDelay(300))
      {
        strip.clear();
        strip.show();
        emdrState = StateEMDR::runningLights;
        //prynt("Gjør nå EMDR-lys!");
      }
    }
  }
  else if (emdrState == runningLights)
  {
    if (DebounceDelayEMDR(60000)) //60000 is one minute
    {
      emdrState = StateEMDR::waitingForWheelInput;
      //prynt("Spør bruker om følelser!");
    }
    DoEMDR(20, stripColour);
    prynt(String(emdrState));
  }
}


/**
 * Metoden DoEMDR(long, uint32_t) er en funksjon som krevet durationBetweenLights og en 
 * farge for å kjøre. Funksjonen bruker DebounceDelay(long)-metoden og DoNextLight(uint32_t).
 * Metoden kjøres kontunuerlig i en loop, og den skal da kjøre på egen hånd og legge til
 * faste intervaller når lys skrur seg på.
 */
static void DoEMDR(long durationBetweenLights, uint32_t colour)
{
  if(DebounceDelayLights(durationBetweenLights))
  {
    //prynt("Neste lys!");
    DoNextLight(colour);
  }
}



/**
 * Metoden DoNextLight(uint32_t) tar in lysfargen som lyset som skal skrus på skal
 * være. Funskjonen er en slags iterator som husker hvilket lys den skrudde på forrige runde,
 * og skrur så på det neste lyset i rekkefølgen når den blir kalt. Funksjonen vet når den skal
 * stige (lys-punktet går vekk fra tilkoblingen) eller synke (lys-punktet får mot tilkoblingen)
 */
static void DoNextLight(uint32_t colour)
{
  static int teller = 0;
  static bool isLightsRising = true;

  if (teller >= 48 && isLightsRising)
  {
    isLightsRising = false;
  }
  else if (teller <= 0 && !isLightsRising)
  {
    isLightsRising = true;
  }

  if (isLightsRising)
  {
    teller++;
    strip.setPixelColor(teller, colour);
    strip.setPixelColor(teller - 1, 0);
    strip.show();
    //prynt("Lysene stiger!");
  }
  else
  {
    teller--;
    strip.setPixelColor(teller, colour);
    strip.setPixelColor(teller + 1, 0);
    strip.show();
    //prynt("Lysene synker!");
  }
}

/**
 * Metode som gjør at led-stripen pulserer når den er kalt kontinuerlig
 */
static void PulseLights()
{
  if(DebounceDelayLights(20))
  {
    DoPulse();
  }
}


/**
 * Metode som viser pulserende lys på led-stripen. Den brukker en innebygd teller, så den vet på hvilket
 * punkt i pulseringen man er. Dette betyr at hvert kall man gjøre på dette endrer lysstyrken på lysene.
 * Når denne er kalt kontunierlig, men med litt mellomrom mellom hvert kall, får man en puls.
 */
static void DoPulse()
{
  static int tellerPulse = 1;
  static bool isLightsRisingPulse = true;

  if (tellerPulse > 50 && isLightsRisingPulse)
  {
    isLightsRisingPulse = false;
  }
  else if (tellerPulse < 1 && !isLightsRisingPulse)
  {
    isLightsRisingPulse = true;
  }

  uint16_t pulseColour = strip.Color(0, 0, tellerPulse);

  if (isLightsRisingPulse)
  {
    tellerPulse++;
  }
  else
  {
    tellerPulse--;
  }

  //skrur på lysene
  for (int i = 0; i < 50; i++)
  {
    strip.setPixelColor(i, pulseColour);
  }
  strip.show();
}

/**
 * Metode som viser en animasjon på leds-stripen.
 * Denne animasjonen er brukt for å vise en animasjon av et grønt lys som starter på
 * høyre side av enheten og sprer seg mot venstre side helt til hele lys-stripen er
 * grønn. Til slutt går man tilbake til standby-modus.
 */
static void SendingAnimation()
{
  if (DoSendAnimation(2, strip.Color(0, 255, 0)))
  {
    prynt("Animasjon ferdig!");
    state = State::Standby;
    strip.clear();
  }
}

/**
 * Metode som viser en animasjon på leds-stripen.
 * Denne animasjonen er brukt for å vise en animasjon av et rødt lys som starter på
 * høyre side av enheten og sprer seg mot venstre side helt til hele lys-stripen er
 * rød. Til slutt går man tilbake til standby-modus.
 */
static void NotSendAnimation()
{
  if (DoSendAnimation(2, strip.Color(255, 0, 0)))
  {
    prynt("Animasjon ferdig!");
    state = State::Standby;
    strip.clear();
  }
}

/**
 * Metode som kalles på kontunierlig for å gjøre at animasjonen vises.
 * Den tar hånd om hvor lang tid det skal være mellom hvert lys lyser opp
 * med variabelen durationBetweenLights. 
 */
static bool DoSendAnimation(long durationBetweenLights, uint32_t colour)
{
  if(DebounceDelayLights(durationBetweenLights))
  {
    return DoNextAnimationLight(colour);
  }
}

/**
 * Metode med innebygd teller som skrur på et nytt lys hver gang den kalles på.
 * Når hele lysstripen er lyst opp returnerer man true, slik at 
 * DoSendAnimation(long durationBetweenLights, uint32_t colour) vet når den er
 * ferdig og man kan gå tilbake til standby.
 */
static bool DoNextAnimationLight(uint32_t colour)
{
  static int tellerAnim = 0;
  
  tellerAnim++;
  strip.setPixelColor(tellerAnim, colour);
  strip.show();

  if (tellerAnim >= 48)
  {
    tellerAnim = 0;
    return true;
  }
  return false;
}

/**
 * Dette er en debounce-metode som kan brukes som man bruker 
 * delay().
 */
bool DebounceDelay(long duration)
{
  static unsigned long nowTime = 0;
  static unsigned long prevTime = 0;
  nowTime = millis();
  
  if((nowTime - prevTime) >= duration)
  { 
    prevTime = nowTime;
    return true;
  }

  return false;
}

/**
 * A DebounceDelay but that is used separately from the one above.
 * This is because the DebounceDelay for the buttons needs to be
 * separate from other DebounceDelays so that everything stays
 * responsive.
 */
/**
 * En DebounceDelay(), men denne blir brukt separat fra den over.
 * Dette er fordi man må separere DebounceDelay() som blir brukt til
 * input, og de som bare blir brukt av koden selv for at alt forblir
 * responsivt.
 */
bool DebounceDelayEMDR(long duration)
{
  static unsigned long nowTimeE = 0;
  static unsigned long prevTimeE = 0;
  nowTimeE = millis();
  
  if((nowTimeE - prevTimeE) >= duration)
  {
    prevTimeE = nowTimeE;
    return true;
  }

  return false;
}

/**
 * En DebounceDelay(), men denne blir brukt separat fra de over.
 */
bool DebounceDelayLights(long duration)
{
  static unsigned long nowTimeL = 0;
  static unsigned long prevTimeL = 0;
  nowTimeL = millis();
  
  if((nowTimeL - prevTimeL) >= duration)
  {
    prevTimeL = nowTimeL;
    return true;
  }

  return false;
}


/**
 * En DebounceDelay(), men denne blir brukt separat fra de over.
 */
bool DebounceDelaySend(long duration)
{
  static unsigned long nowTimeS = 0;
  static unsigned long prevTimeS = 0;
  nowTimeS = millis();
  
  if((nowTimeS - prevTimeS) >= duration)
  {
    prevTimeS = nowTimeS;
    return true;
  }

  return false;
}

/**
 * Denne metoden har blitt brukt for å gjøre print til seriell overvåker
 * enklere å skrive raskt.
 */
void prynt(String str)
{
  Serial.println(str);
}
