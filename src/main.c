#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

// Konfiguracja pinow LCD
#define LCD_DDR DDRC
#define LCD_PORT PORTC
#define RS PC4
#define EN PC5

// Konfiguracja pinow klawiatury
#define ROWS 4
#define COLS 4
char keys[ROWS][COLS] = {
    {'1', '2', '3', '4'},
    {'5', '6', '7', '8'},
    {'9', '0', 'A', 'B'},
    {'C', 'D', 'E', 'F'}
};
uint8_t rowPins[ROWS] = {PD3, PD2, PD1, PD0}; // Piny wierszy
uint8_t colPins[COLS] = {PD4, PD5, PD6, PD7}; // Piny kolumn

// Konfiguracja pinu buzzera
#define BUZZER_PIN PB0

volatile unsigned long timePlayer1; // Czas gracza 1
volatile unsigned long timePlayer2; // Czas gracza 2
volatile bool isPlayer1Turn = true; // Flaga tury gracza 1
volatile bool gameStarted = false; // Flaga rozpoczecia gry

ISR(TIMER1_COMPA_vect) {
  if (gameStarted) {
    if (isPlayer1Turn) {
      if (timePlayer1 > 0) {
        timePlayer1 -= 1000; // Zmniejsz czas gracza 1 o 1 sekundę
      }
    } else {
      if (timePlayer2 > 0) {
        timePlayer2 -= 1000; // Zmniejsz czas gracza 2 o 1 sekundę
      }
    }

    displayTime(); // Wyswietl zaktualizowane czasy

    if (timePlayer1 <= 0 || timePlayer2 <= 0) {
      // Koniec gry
      PORTB |= (1 << BUZZER_PIN); // Włącz buzzer
      _delay_ms(1000);
      PORTB &= ~(1 << BUZZER_PIN); // Wyłącz buzzer
      gameStarted = false;
      lcdClear();
      lcdSetCursor(0, 0);
      lcdPrint("Game Over!");
    }
  }
}

int main(void) {
  DDRD = 0x0F; // Konfiguracja dolnych 4 bitow portu C jako wyjscia (wiersze klawiatury), gornych 4 jako wejscia (kolumny klawiatury)
  PORTD = 0xF0; // Wlaczenie pull-up dla kolumn
  DDRB |= (1 << BUZZER_PIN); // Konfiguracja pinu buzzera jako wyjscie

  lcdInit(); // Inicjalizacja LCD
  selectMode(); // Wybor trybu gry
  initTimer(); // Inicjalizacja timera
  sei(); // Wlaczenie przerwan

  while (1) {
    char key = getKey();
    if (key == 'C') {
      isPlayer1Turn = false; // Przelaczenie tury na gracza 2
    } else if (key == 'F') {
      isPlayer1Turn = true; // Przelaczenie tury na gracza 1
    }
  }
}

void lcdCommand(uint8_t cmd) {
  // Wyslij gorne 4 bity
  LCD_PORT &= ~(1 << RS); // RS = 0 dla komendy
  LCD_PORT |= (1 << EN);  // EN = 1
  LCD_PORT = (LCD_PORT & 0xF0) | ((cmd >> 4) & 0x0F);           
  LCD_PORT &= ~(1 << EN); // EN = 0
  // Wyslij dolne 4 bity
  LCD_PORT |= (1 << EN);  // EN = 1
  LCD_PORT = (LCD_PORT & 0xF0) | (cmd & 0x0F);              
  LCD_PORT &= ~(1 << EN); // EN = 0
  _delay_us(50);                
}

void lcdData(uint8_t data) {
  // Wyslij gorne 4 bity
  LCD_PORT |= (1 << RS);  // RS = 1 dla danych
  LCD_PORT |= (1 << EN);  // EN = 1
  LCD_PORT = (LCD_PORT & 0xF0) | ((data >> 4) & 0x0F);              
  LCD_PORT &= ~(1 << EN); // EN = 0
  // Wyslij dolne 4 bity
  LCD_PORT |= (1 << EN);  // EN = 1
  LCD_PORT = (LCD_PORT & 0xF0) | (data & 0x0F);              
  LCD_PORT &= ~(1 << EN); // EN = 0
  _delay_us(50);                
}

void lcdInit(void) {
  LCD_DDR = 0xFF; // Ustawienie portu C jako wyjscia
  _delay_ms(20);  

  // Sekwencja inicjalizacyjna dla 4-bitowego trybu
  lcdCommand(0x33); // Tryb 8-bitowy
  _delay_ms(5);
  lcdCommand(0x32); // Tryb 4-bitowy
  _delay_ms(5);
  lcdCommand(0x28); // 4-bitowy, 2 linie, 5x8 font
  _delay_ms(1);
  lcdCommand(0x0C); // Wlacz LCD, wylacz kursor
  _delay_ms(1);
  lcdCommand(0x01); // Wyczysc ekran
  _delay_ms(2);
  lcdCommand(0x06); // Ustawienia wejsciowe
}

void lcdClear(void) { // Funkcja czyszczaca ekran
    lcdCommand(0x01);
    _delay_ms(2);
}

void lcdSetCursor(uint8_t row, uint8_t col) { // Funkcja do ustawienia kursora na ekranie
  uint8_t addr;
  switch (row) {
    case 0:
      addr = 0x00 + col; // Przypadek dla pierwszego rzedu
      break;
    case 1:
      addr = 0x40 + col; // Przypadek dla drugiego rzedu
      break;
  }
  lcdCommand(0x80 | addr);
}

void lcdPrint(char *str) { // Funkcja wyswietlajaca ciagi znakow na ekranie
  while (*str) {
    lcdData(*str++);
  }
}

void displayTime() { // Funkcja wyswietlajaca zegary obu graczy 
  lcdClear();
  lcdSetCursor(0, 0); // Ustawienie kursora na poczatku pierwszej linii
  lcdPrint("P1: ");
  char buffer[6]; // // Bufor do przechowywania sformatowanego czasu
  snprintf(buffer, sizeof(buffer), "%02lu:%02lu", timePlayer1 / 60000, (timePlayer1 / 1000) % 60); // Formatowanie czasu gracza 1 jako mm:ss
  lcdPrint(buffer);

  lcdSetCursor(1, 0); // Ustawienie kursora na poczatku drugiej linii
  lcdPrint("P2: ");
  snprintf(buffer, sizeof(buffer), "%02lu:%02lu", timePlayer2 / 60000, (timePlayer2 / 1000) % 60); // Formatowanie czasu gracza 2 jako mm:ss
  lcdPrint(buffer);
}

char getKey() { // Funkcja wykrywajaca nacisniecie odpowiedniego przycisku na klawiaturze
  for (uint8_t r = 0; r < ROWS; r++) { // Iteracja przez kazdy wiersz
    PORTD = ~(1 << rowPins[r]); // Ustawienie na pinach wierszy stanu niskiego
    _delay_us(5);
    for (uint8_t c = 0; c < COLS; c++) { // Iteracja przez kazda kolumne
      if (!(PIND & (1 << colPins[c]))) { // Sprawdz, czy kolumna jest niska
        while (!(PIND & (1 << colPins[c]))); // Poczekaj, az klawisz zostanie zwolniony
          PORTD = 0xFF; // Powrot do poczatkowego stanu
          return keys[r][c]; // Zwrot danego klawisza
      }
    }
  }
}

void selectMode() {
  lcdClear();
  lcdSetCursor(0, 0);
  lcdPrint("Select Mode: 1.T");
  lcdSetCursor(1, 0);
  lcdPrint("2:Blitz 3:Rapid");

  while (1) { // Petla ktora na podstawie nacisnietego klawisza wybiera tryb gry
    char key = getKey();
    if (key == '1') {
      setTestMode();
      break;
    } else if (key == '2') {
      setBlitzMode();
      break;
    } else if (key == '3') {
      setRapidMode();
      break;
    }
  }
  gameStarted = true; // Rozpoczecie gry
}

void setTestMode() {
  timePlayer1 = 30000; // 30 sekund w milisekundach
  timePlayer2 = 30000; // 30 sekund w milisekundach
}

void setBlitzMode() {
  timePlayer1 = 300000; // 5 minut w milisekundach
  timePlayer2 = 300000; // 5 minut w milisekundach
}

void setRapidMode() {
  timePlayer1 = 900000; // 15 minut w milisekundach
  timePlayer2 = 900000; // 15 minut w milisekundach
}

void initTimer() {
  TCCR1B |= (1 << WGM12) | (1 << CS12) | (1 << CS10); // CTC mode, prescaler 1024
  OCR1A = 15624; // 1 sekunda przy 16MHz i prescalerze 1024
  TIMSK1 |= (1 << OCIE1A); // Wlaczenie przerwan porownawczych
}
