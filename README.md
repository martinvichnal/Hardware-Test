# laughing-doodle

# Table of Content

-   [laughing-doodle](#laughing-doodle)
-   [Table of Content](#table-of-content)
-   [Kapcsolási rajz](#kapcsolási-rajz)
    -   [GPIO](#gpio)
    -   [I2C](#i2c)
-   [Firmware](#firmware)
    -   [Működése](#működése)
        -   [Változók](#változók)
    -   [Fényérzékelő](#fényérzékelő)
    -   [Ultrahangos érzékelő](#ultrahangos-érzékelő)
        -   [Class használata](#class-használata)
    -   [Kommunikáció](#kommunikáció)
        -   [I2C](#i2c-1)
        -   [UART](#uart)
-   [Program](#program)

# Kapcsolási rajz

A kapcsolási rajzot a KiCad 8.0.2 programmal hoztam létre. Középpontjában egy Arduino Nano Socket áll.

A kapcsolási rajzon a J1-es jellel ellátott foglalaton kivezettem az Arduino UART lábait ezzel könnyedén debuggerelni lehet a kommunikációs rését a rendszernek.

## GPIO

| Neve          | Jele | GPIO    |
| ------------- | ---- | ------- |
| Power         | PW1  | -       |
| UART          | J1   | D0/D1   |
| Photoresistor | J2   | A0      |
| SRF-04        | J3   | D5/D4   |
| 1604 LCD      | J4   | SDA/SCL |
| 1604 LCD      | J5   | SDA/SCL |
| LED1          | J6   | D8      |
| LED2          | J7   | D7      |
| LED3          | J8   | D6      |

## I2C

2 darab egységünk van a kapcsoláson ami I2C-vel kommunikál. A J4 és J5 foglalatba tudjuk ezeket az LCD-ket bekötni.

Felhúzó ellenállások (R1, R2) is megtalálhatóak az SDA és SCL lábakon mivel az I2C protokoll egy Open-Drain összeköttetésen alapúl.

A rendszeren belül 2 I2C cím jelenik meg, ezek a _0x70_ és _0x7E_ címek. Ezeket a címeket fizikailag be lehet állítani a PCF8574A GPIO extender IC segítségével mivel ezen keresztűl kommunikálunk az LCD kijelzőkkel.

```
A2 A1 A0 | READ WRITE
-----------------------
J4: L L L | 0x71 0x70
J5: H H H | 0x7F 0x7E
```

A2-A0 ig ellátott jelölések fizikai réz lapokat jelölnek amikkel be lehet állítani hogy az adott IC-nek milyen fizikai I2C címe legyen. Én esetemben kettőt válaszottam ahol mind a 3 pad nincs összekötve és ahol mind a 3 pad össze van kötve.

# Firmware

## Működése

A program 500ms-ként lekérdezi az összes szenzorról a kívánt adatokat majd feldolgozza az adatokat és elküldi az üzenetet az UART-on keresztűl. A program folyamatosan frissíti a LED-ek állapotát amit a `handleLEDs()` függvény kezel. Ebben a függvényben szimplán komparálja a távolságot az előre definiált konstans változókkal `ledUpperLimit`, `ledBottomLimit`.
A programban az 500ms-kénti lekérdezést úgy váltottam valóra, hogy a saját könyvtáramat hozzáadtam a programhoz. Ez akönyvtár az `AntiDelay`, amivel _NON-BLOCKING_ delay-eket hozhatunk létre.

```C++
AntiDelay sensorReadings(500);
```

Ezt az értéket a késöbbiekben könnyedén megváltoztathatjuk a

```C++
void AntiDelay::setInterval(unsigned long interval)
```

belső függvény segítségével.

### Változók

## Fényérzékelő

Mivel a fényérzékelő egy analóg GPIO-pinre megy rá (A0) ezért könnyedén lekérdezhetjük az ADC értékét a beépített Arduino könyvtár

## Ultrahangos érzékelő

Az ultrahangos érzékelőnek csináltam egy Classt ezzel a kódot letisztultabbá és rendszerezhetőbbé tettem. A classon belül 1 függvény van ami szimplán megadja, hogy egy objektum milyen messze van amit egy float típusú változóként küld vissza. 2 belső változója van ami szimplán eltározza a GPIO lábak értékét.

### Class használata

Deklarálás:

```C++
Sonic sonicSensor(TRIGGER_PIN, ECHO_PIN);
```

Távolság lekérése:

```C++
float sonicDistance = sonicSensor.getDistance();
```

## Kommunikáció

### I2C

Az I2C kommunikációs protokollt a két 1604-es LCD kijelző futtatására használom.

### UART

A kommunikáció a PC-n lévő porgram és az Arduino között aszinkron adó-vevő protokollon (UART) keresztűl jut 1 bájtos adatcsomagokkal továbbításra.

Üzenetkeret:

```C++
typedef struct
{
	uint8_t start;		  // 1 byte - const 0x55
	uint8_t sonicData[4]; // 4 bytes - Sonic sensor data (float)
	uint8_t photoData[4]; // 2 bytes - Photo cell data (int)
	uint8_t cs;			  // 1 byte - Check sum error handling
	uint8_t end;		  // 1 byte - const 0xAA
} Message;
```

Az üzenetkeretet a programokon belül egy struct-ban tárolom, tartalma a következő:

-   start: 1 bájt előre definiált konstans 0x55 érték ezzel jelezve a csomag kezdetét
-   sonicData: 4 bájtos adat amiben az Ultrahangos érzékelő távolságát tartalmazza ami FLOAT típusú
-   photoData: 4 bájtos adat amiben a Fényérzékelő ADC értékét tartalmazza ami INT típusú (sok rendszerben változó az int típus nagysága ezért a biztonság kedvéért 4 bájt)
-   cs: 1 bájtnyi Check Sum ami a kód integritás vizsgálására használatos.
-   end: 1 bájt előre definiált konstans 0xAA érték ezzel jelezve a csomag végét

Az UART kommunikációhoz fűződve 4 belső függvényt csináltam.

```C++
bool sendUARTMessage(Message *msg);
void convertToMessage(float fSonicData, int iPhotoData, Message *buffer);
bool decodeMessage(Message *buffer, float *fSonicData, int *iPhotoData);
uint8_t calculateCheckSum(Message *msg);
```

Ezek segítségével a nyers adatokat át tudom konvertálni a Message struct bufferba és fordítva. A sendUARTMessage függvény segítségével lehet kiküldeni az adattömböt az UARTra. Egy biztonsági réteget is beleiktattam az adatcsomagba ami egy egyszerű check sum funkció, ezzel ki lehet kerülni az esetlegesen megroncsolt adatok feldolgozását.

# Program
