# Table of Content

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
    -   [1602 LCD](#1602-lcd)
    -   [Kommunikáció](#kommunikáció)
        -   [I2C](#i2c-1)
        -   [UART](#uart)
    -   [Könyvtárak](#könyvtárak)
-   [Program](#program)
    -   [Python](#python)
        -   [Könyvtárak](#könyvtárak-1)
    -   [C++](#c)
        -   [Portkezelés](#portkezelés)
            -   [API](#api)
        -   [Könyvtárak](#könyvtárak-2)
-   [Gyakorlati megvalósítás](#gyakorlati-megvalósítás)

# Kapcsolási rajz

A kapcsolási rajzot a KiCad 8.0.2 programmal hoztam létre. Középpontjában egy `Arduino Nano` Socket áll.

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

2 darab egységünk van a kapcsoláson, ami I2C-vel kommunikál. A J4 és J5 foglalatba tudjuk ezeket az LCD-ket bekötni.

Felhúzó ellenállások (R1, R2) is megtalálhatóak az SDA és SCL lábakon, mivel az I2C protokoll egy Open-Drain buszrendszeren alapúl.

A rendszeren belül 2 I2C cím jelenik meg, ezek a _0x70_ és _0x7E_ címek. Ezeket a címeket fizikailag be lehet állítani a PCF8574A GPIO extender IC segítségével, mivel ezen keresztűl kommunikálunk az LCD kijelzőkkel.

```
A2 A1 A0 | READ WRITE
-----------------------
J4: L L L | 0x71 0x70
J5: H H H | 0x7F 0x7E
```

A2-A0 ig ellátott jelölések fizikai réz lapokat jelölnek amikkel be lehet állítani, hogy az adott IC-nek milyen fizikai I2C címe legyen. Én esetemben kettőt válaszottam, ahol mind a 3 pad nincs összekötve és ahol mind a 3 pad össze van kötve.

![Address Table](/Docs/PCF8574A%20I2C%20Address%20Reference%20table.png)

# Firmware

## Működése

A program 500ms-ként lekérdezi az összes szenzorról a kívánt adatokat, majd feldolgozza, és elküldi az üzenetet UART-on keresztűl. Eközben az LCD-re való kiírás is megtörténik a `writeLCD()` függvény segítségével.

A program folyamatosan frissíti a LED-ek állapotát, amit a `handleLEDs()` függvény kezel. Ebben a függvényben szimplán komparálja a távolságot az előre definiált konstans változókkal `ledUpperLimit`, `ledBottomLimit`.
A programban az 500ms-kénti lekérdezést úgy váltottam valóra, hogy a saját könyvtáramat hozzáadtam a programhoz. Ez akönyvtár az `AntiDelay`, amivel _NON-BLOCKING_ delay-eket hozhatunk létre.

```C++
AntiDelay sensorReadings(500);
```

Ezt az értéket a késöbbiekben könnyedén megváltoztathatjuk a `void setInterval(unsigned long interval)` belső függvény segítségével.

A program futtatása során lehetőség van belső debuggerelésre, ami szimplán kiírja a soros portra az értékeket, amiket a szenzorról olvas le. Ezt a funkciót `#define DEBUG 1/0`-val lehet ki és bekapcsolni. Az üzenetek kiküldése ugyan azon a porton keresztül történik meg amelyiken a Message adatcsomagot kiküldjük ezért érdemes kikapcsolva hagyni.

### Változók

## Fényérzékelő

Mivel a fényérzékelő egy analóg GPIO-pinre (A0) megy rá, ezért könnyedén lekérdezhetjük az ADC értékét a beépített Arduino könyvtár segítségével.

```C++
photoCellValue = analogRead(PHOTOCELL);
```

## Ultrahangos érzékelő

Az ultrahangos érzékelőnek csináltam egy class-t, ezzel a kódot letisztultabbá és rendszerezhetőbbé tettem. A classon belül 1 függvény van, ami szimplán megadja, hogy egy objektum milyen messze van, amit egy float típusú változóként küld vissza. 2 belső változója van, ami szimplán eltározza a GPIO lábak értékét.

### Class használata

Deklarálás:

```C++
Sonic sonicSensor(TRIGGER_PIN, ECHO_PIN);
```

Távolság lekérése:

```C++
float sonicDistance = sonicSensor.getDistance();
```

## 1602 LCD

Az LCD vezérléséhez 2 függvényt hoztam létre. Az `void initLCD()` csupán inicializálja az I2C kommunikációt és beállítja az LCD alapbeállításait.

Adatok kiírása a void `writeLCD()` függvény segítségével történik meg.

## Kommunikáció

### I2C

Az I2C kommunikációs protokollt a két 1604-es LCD kijelző futtatására használom. A kommunikáció leegyszerűsítése kedvéért `LiquidCrystal_I2C` könyvtárat használtam.

### UART

A kommunikáció a PC-n lévő porgram és az Arduino között aszinkron adó-vevő (UART) protokollon keresztűl jut 1 bájtos adatcsomagokkal továbbításra.

Az üzenetkeret:

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

-   `start`: 1 bájt előre definiált konstans 0x55 érték ezzel jelezve a csomag kezdetét
-   `sonicData`: 4 bájtos adat amiben az Ultrahangos érzékelő távolságát tartalmazza ami FLOAT típusú
-   `photoData`: 4 bájtos adat amiben a Fényérzékelő ADC értékét tartalmazza ami INT típusú (sok rendszerben változó az int típus nagysága ezért a biztonság kedvéért 4 bájt)
-   `cs`: 1 bájtnyi Check Sum ami a kód integritás vizsgálására használatos.
-   `end`: 1 bájt előre definiált konstans 0xAA érték ezzel jelezve a csomag végét

![UART Logic Analysator](/Docs/UART%20data%20transfer.png)
(A fájl megtalálható: Docs/UART data transfer.sal és megnyitható a [Saleae Logic 2.4.14](https://discuss.saleae.com/t/logic-2-4-14/2746)-es programmal)


Az UART kommunikációhoz fűződve 4 belső függvényt csináltam.

```C++
bool sendUARTMessage(Message *msg);
void convertToMessage(float fSonicData, int iPhotoData, Message *buffer);
bool decodeMessage(Message *buffer, float *fSonicData, int *iPhotoData);
uint8_t calculateCheckSum(Message *msg);
```

Ezek segítségével a nyers adatokat át tudom konvertálni a Message struct bufferba, és fordítva. A sendUARTMessage függvény segítségével lehet kiküldeni az adattömböt az UART-ra. Egy biztonsági réteget is beleiktattam az adatcsomagba, ami egy egyszerű check sum funkció, ezzel ki lehet kerülni az esetlegesen megroncsolt adatok feldolgozását.

## Könyvtárak

[johnrickman/LiquidCrystal_I2C](https://github.com/johnrickman/LiquidCrystal_I2C/tree/master)

[martinvichnal/AntiDelay](https://github.com/martinvichnal/AntiDelay)

# Program

A program megvalósítása 2 féle programozási környezetben lett megvalósítva. A C++ alkalmazásban a felhasználó csak a terminálon keresztül tudja venni az Arduinótól jövő adatokat, amit kiír nyers adatként és feldolgozott Sonic és Photo adatként.

A python alkalmazás nem csak fogadja hanem ki is rajzolja (plotolja) a képernyőre.

## Python

Az adatok fogadása a `def reciveData(ser):` függvény felelős ami bytonként olvassa, majd a `struct` könyvtár segítségével megfelelő típusra konvertálja őket. A függvény visszatérési értékei a `return sonicData, photoData`. A program megvárja míg a start bit (0x55) megérkezik, majd csak akkor kerül az adatok további feldolgozásra.

Az értékek kirajzolása `update_sonic_plot` és `update_photo_plot` függvényeken keresztül történnek meg, amit itt hívunk meg:

```Python
fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(8, 6))
ani1 = FuncAnimation(fig, update_sonic_plot, interval=20)
# ani2 = FuncAnimation(fig, update_photo_plot, interval=20)
plt.show()
```

Sajnos ismeretlen okok miatt egyszerre a kettő érték kirajzolása nem megvalósítható, mivel a program nagyon belassul ilyenkor, ezért csak az egyiket lehet futtatni, a másikat ki kell kommentezni.

![Fényszenzor plot](/Docs/Fényszenzor%20plot.png)

![Ultrahang plot](/Docs/Ultrahang%20plot.png)

### Könyvtárak

Az alaklamazásban két könyvtárat használtam. Az UART kommunikációért felelős: `pip install pyserial` és az értékek kirajzolásáért: `pip install matplotlib`

## C++

A C++-os megvalósításban mind terminálra való kiíratás és grafikus módon is ki kerül iratásra az adatok. Mivel a két megvalósítás (csak terminálra való kiíratés és a grafikus módon való kiíratás) teljesen más struktúrát igényelt ezért, ezért két mappa található meg a projektben: `Program cpp v1` (csak terminál) és a `Program cpp v2` (terminál és grafikus). Az alap funkciók, tehát a portkezelés teljesen megegyezik, csak a `v2` projektben egy `Draw` classba gyűjtöttem minden egyes olcPixelGameEngine-hez szükséges funkciókat. Mivel a Draw classon belül használom a portkezelést, ezért azok is átkerültek oda mint belső (privát) függvények.

A funkciók és kódrészek nagy része átvehető volt az Arduino Firmware kódrészéből ezért a gyakorlati működése megegyezik az Arduinoéval.
A kódban csak akkor történik `bufferbe` adatok írása amikor a start bit (0x55) megérkezik, valamint hiba esetén (azaz a check sumok nem egyeznek meg) a `parseMessage` funkció 1-et küld vissza.

![Kiíratás terminálra](/Docs/Kiíratás%20terminálra.png)

![Grafikus megjelenítés](/Docs/Cpp%20grafikus.png)

### Portkezelés

Portkezelésre írtam egy külön könyvtárat, ahol a `windows.h` beépített könyvtár tulajdonságait használom fel.

#### API

A portkezelés API formája nagyon megegyező az Arduinoóéhoz ezért könnyedén lehet használni.

```C++
int begin(const char* portName);
void close();
int read(const char* buffer, unsigned int bufferSize);
bool write(const char* buffer, unsigned int bufferSize);
bool isConnected();
```

Ezek a függvények dokumentációja, értelmezése megtalálható a projekten belüli `SerialHandler.cpp` fájlban.

### Könyvtárak

Grafikus megjelenítésért felelős könyvtár: [OneLoneCoder/olcPixelGameEngine](https://github.com/OneLoneCoder/olcPixelGameEngine)

# Gyakorlati megvalósítás

A gyakorlatban is megépítettem a rendszert, ugyan azokkal a szenzor elemekkel, amik a feltételek között is szerepeltek. A rendszer minden elemét teszteltem, kivéve az LCD kijelzőt, mivel erre sajnos nem volt megfelelő elemem.

![Gyakorlati megvalósítás](/Docs/Gyakorlati%20megvalósítás.jpg)
