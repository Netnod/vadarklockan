# Bakgrund

Välkommen till "Vad är klockan?"

(Det här är ungefär samma information som på huvudsidan, men från
innan översättningen till engelska gjordes och kan därmed vara
daterad.  För de senaste uppdateringarna se den <a href='./'>engelska
varianten</a>.

För att kunna kommunicera säkert på internet så är det viktigt att
kunna svara på frågan "vad är klockan", alltså säga vad aktuell
tidpunkt är, både datum och klockslag på dygnet.  Utan korrekt tid är
det flera säkerhetskritiska protokoll som antingen helt slutar fungera
eller som tappar mycket av sin säkerhet.

* DNSSEC, som behövs för att kunna slå upp namn på internet, fungerar
  inte alls om klockan i en klient går mer än fem minuter fel.  Det
  går att falla tillbaka till äldre DNS utan säkerhet.  Men då tappar
  man också säkerhet: någon som kan påverka kommunikationen mellan
  klient och server kan ge fel svar på namnuppslagningar.  När man
  tror att man pratar med "example.com" så pratar man egentligen med
  någon helt annan.

* TLS (tidigare känt som SSL) behöver korrekt tid för att verifiera
  giltigheten på de certifikat som används.  TLS ligger till grund för
  många andra protokoll som används på internet.

* HTTPS används för att säkra upp WWW och är baserat på TLS.  Utan
  säker TLS så kan man inte heller få säker HTTPS.  Det finns HTTP
  utan säkerhet, men då kan någon som kan påverka trafiken mellan en
  klient och en server avlyssna och påverka trafiken.

* Samma gäller protkollen SMTPS, IMAPS och POP3S som används för
  e-mail, alla de är baserade på TLS.  Det finns osäkra varianter,
  SMTP, IMAP och POP3 men de är också känsliga för avlyssning och
  påverkan.

* Tid kan även vara viktiga för själva applikationen.  Det är inte bra
  om ett styrsystem som ska låsa upp en dörr mellan 07:00-17:00 på
  vardagar får fel tid så att dörren står öppen mitt i natten eller på
  en helg.

Många internet-anslutna enheter vet inte vad klockan är när de startar
upp.  En del enheter är konstruerade på det viset, för att hålla ned
priset så har de inget batteri och när de förlorar strömmen så glömmer
den vad klockan är.  Även enheter med batteri kan glömma vad klockan
är om man byter batteri.  Andra enheter har en "shipping mode" där de
för att spara batteri medans de ligger i lager inte startar sin
interna klocka förrän de får ström första gången.  Det gör att den
första gången de startas kommer de inte veta vad klockan är.

Målet med projektet "vad är klockan" är att låta en internet-ansluten
enhet hämta tid på ett säkert sätt under uppstart.  Det handlar till
att börja med om principer, att hitta på en metod att göra detta.
Utöver det så ska projektet ta fram ett bibliotk som implementerar
denna metod som ska kunna vara till nytta för så många som möjligt.

## Metod

Metoden kan konceptuellt delas upp i två delar:

* Att på ett säkert sätt få svar på frågan "vad är klockan?" genom att
  prata med en tids-server.  Det finns en IETF draft av ett protokoll
  som heter "Roughtime" som gör detta och som vi valt att använda.

* Även om man kan ställa säkra frågor så är det inte säkert att
  tids-servern man pratar med ger korrekt tid.  En tids-server kan ha
  gått sönder så att den ger felaktig tid av misstag.  En tids-server
  kan också ha tagits över av någon annan och medvetet ge fel tid.
  För att vara någorlunda säker på att den tid man får är korrekt så
  bör man ställa frågor till flera olika tids-servrar och baserat på
  de svar man får tillbaka välja ut de svar som verkar rimliga,
  t.ex. genom att kräva att en majoritet av svaren ligger
  överensstämmer.  Vi har valt att utgå från den "selection and
  clustering algorithm" som beskrivs i RFC5906.

Genom att separera protokollet för att prata med tids-servrar från
algoritmen för att välja ut vilka man ska lita på så är det relativt
lätt att byta ut någon av delarna i framtiden.  Framför allt så kommer
med stor sannolikhet Roughtime-protokollet att få inkompatibla
ändringar innan det nått fram till att bli en IETF RFC.  Det kan även
hända att något annat protokoll för att ställa frågor till
tids-servrar blir mer populärt i framtiden.

## Implementation

För implementations-delen av har vi valt att göra två implementationer.

* En implementation i Python vars mål är att vara lätt att förstå.  Vi
  har utgått från en befintlig implementation av Roughtime-protokollet
  som heter "pyroughtime" av Marcus Dansarie.  Algoritmen för att
  välja ut vilka tids-servrar man skall lita på är nyskriven från
  grunden.  En av fördelarna med Python är att det har varit relativt
  lätt att simulera och visualisera de algoritmer som används.

* En implementation i C vars mål är att vara liten, kompakt, säker och
  som ska gå att använda på så många internet-anslutna enheter som
  möjligt.  De flesta plattformar, allt från små IoT-enheter baserade
  på Arduino, Raspberry Pi, eller en riktig dator som kör Linux,
  Windows eller MacOS kan köra kod skriven i C.  Stödet för
  Roughtime-protokollet baseras på en implementation som heter
  "vroughtime" av Oscar Reparaz.  Algoritmen för att välja ut vilka
  tids-servrar som ska lita på är en port av Python-koden ovan.

## Referensplattformar

Dessa implementationer behöver sedan lite klisterkod för att köra på
den målplattform man har valt.  Vi har valt två målplattformar till
att börja med, men det bör vara enkelt att porta koden till andra
plattformar.

* Utvecklingen av både Python- och C-implementationerna har skett på
  en dator körandes Linux.  Den mesta koden är plattformsoberoende,
  men det har behövs lite plattformsspecifik klisterkod.  Samma kod
  borde fungera på även Windows och Unix-baserade operativysstem som
  MacOS men har inte testats på de plattformarna.

* Utöver det har vi valt att porta implementationen tll en liten
  ESP32-baserad enhet med WiFi för att visa att det även fungerar på
  mindre IoT-enheter.  Den ESP32-baserade plattformen kan köra en
  bantad version av Python-implementationen i en MicroPython-miljö och
  C-implementationen i en Arduino-miljö.  Den ESP32-enheten heter
  "TTGO" men koden borde gå att köra på de flesta ESP32-enheter och
  eventuellt även på ESP8266-enheter.

<p align="center">
  <img src="_images/ttgo.jpg" width="600" height="400" title="TTGO ESP32" />
</p>

## Implementationsdetaljer Python

"pyroughtime" har från början stöd både för draft 06 och äldre
versioner av Roughtime.

### Python på Linux

Python-implementationen för Linux finns i examples/python.

Endast några smärre ändringar behövde göras för att fungera med "vad
är klockan".

### MicroPython på ESP32

Python-implementationen för TTGO ESP32 finns i examples/micropython-esp32-ttgo.

MicroPython på ESP32 har mycket ont om minne och skiljer sig ganska
mycket från mainline Python.  För att få plats har "pyroughtime"
behövt bantas och anpassas rejält.

MicroPython för ESP32 har utökats med stöd för de
krypterings-primitiver som behövs för Roughtime (ed25519, sha512) och
för att kunna ställa klockan från Python-kod (settimeofday).

## Implementationsdetaljer C

"vroughtime" hade från början inte stöd för draft 06 så koden har
modifierats så att den stödjer både draft 06 och äldre versioner av
Roughtime.  Samma kod används både på Linux och på ESP32.

### C på Linux

C-implementationen för Linux finns i examples/C.

Lite klisterkod behövde skrivas för att köra på Linux och för
UDP-kommunikation via BSD sockets API.  Koden stödjer både IPv4 och
IPv6.

### C på ESP32.

C-implementationen för Linux finns i examples/esp32..

Lite klisterkod behövde skrivas för att använda WiFi och UDP på ESP32.
Koden klarar än så länge bara IPv4.

## Roughtime-protokollet

Eftersom Roughtime-protokollet än så länge bara är en "draft" så finns
det inte så många användsbara tids-servar.  Framför allt finns det
bara en enda sever-implementation som fullt stödjer [IETF draft
version 06 av
Roughtime](https://datatracker.ietf.org/doc/html/draft-ietf-ntp-roughtime-06).
Även denna implementation, skriven i C, är gjord av Marcus Dansarie.

Netnod har valt att sätta upp flera Roughtime-servrar med denna
implementation (sth1.roughtime.netnod.se och
sth2.roughtime.netnod.se).  För att underlätta test av hela "vad är
klockan"-projektet, inklusive delarna med att välja ut servrar som
svarar med orrekt tid, så har Netnod även satt upp flera
Roughtime-srvrar som svarar med inkorrekt tid
(falseticker.roughtime.netnod.se).

## Genomförande

Projektet "Vad är klockan?" har genomförts under 2022 av Calle
Lindkvist (C-implementation), Filip Eriksson (Python-implementation)
under handledning av Christer Weinigel (C/Python och
severinfrastruktur hos Netnod samt paketering och dokumentation).
