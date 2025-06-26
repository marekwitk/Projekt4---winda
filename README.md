SPRAWOZDANIE

LOGIKA

1. Dodawanie pasażerów
Kliknięcie w przycisk na danym piętrze dodaje nową instancję ProstokatAnimowany do odpowiedniego piętra w tablicy prostokatikiNaPietrze. Nowy pasażer ma zadany cel (number) i początkową pozycję X (po lewej lub prawej stronie).

2. Sterowanie windą i kierunek jazdy
trybJazdy (W_GORE, W_DOL, IDLE) ustala ogólny kierunek jazdy windy.

Jeśli winda była pusta, pierwszy pasażer ustala kierunek jazdy windy. Następnie winda porusza się zgodnie z jednym z trybów w górę albo w dół, aż nie odwiezie wszystkich w tamtym kierunku i po tym zmienia tryb na drugi i tak w kółko. 
Winda zatrzymuje się zawsze po drodze na piętrze na którym ktoś ma wysiąść, ale również jeśli przejeżdża obok piętra gdzie ktoś stoi, a wszystkie sloty w niej nie są jeszcze zajęte

Jeśli nie ma pasażerów w kabinie ani chętnych do jazdy – po 5 sekundach zjeżdża na 1. piętro.

3. Drzwi
Logikę i animacje działania wsiadania i wysiadania w dużej mierze opieramy na tym w jakim stanie są obecnie drzwi
Zmienna stanDrzwi:
0 = zamknięte, 1 = otwieranie, 2 = otwarte, 3 = zamykanie.

Animacja otwierania/zamykania drzwi odbywa się przez stopniowe zmniejszanie/zwiększanie zmiennych szerokoscLewegoBoku/szerokoscPrawegoBoku (od 1.0 do 0.0 i z powrotem).

Po zatrzymaniu na piętrze i otwarciu drzwi (stanDrzwi==2) uruchamiana jest logika wsiadania/wysiadania.

4. Obsługa pasażerów
Wysiadanie: Pasażerowie, których number jest równe aktualnemu piętru, otrzymują flagę wychodzi=true. Następnie są animowani poza ekran (zmiana X).

Wsiadanie:
a) Tworzona jest kolejka pasażerów przy drzwiach, sortowana wg odległości do wejścia.
b) Jeśli jest wolne miejsce (MAXSLOTS), pierwszy z kolejki wsiada (flaga wWindzie, przydzielany slot).

Sloty w windzie:
Sloty są ponumerowane, jest ich 8 (bo na tyle pasażerów pozwala limit wagowy), po każdej zmianie pasażerów wywoływana jest funkcja CompressSlots(), by nie było "dziur" w windzie, 
podobnie ci którzy stoją w kolejce przysuwają się coraz bliżej wejścia windy, żeby mieć szanse jak najszybciej znaleźć się tuż przy niej, co umożliwiłoby im wejście

5. Animacja
Przemieszczanie windy, otwieranie drzwi, ruch pasażerów do kabiny i wysiadanie – wszędzie używane są interpolacje (stopniowa zmiana pozycji X/Y) oraz stany logiczne, co daje płynność animacji i poprawną synchronizację.

6. Pętla sterowania i decyzyjność
Całość cyklicznie aktualizuje się w WM_TIMER. Po każdej zmianie stanu wywoływane jest odświeżenie okna (InvalidateRect), co uruchamia rysowanie aktualnego stanu sceny.


RYSOWANIE
Część związana z rysowaniem wszystkiego co widzimy na ekranie znajduje się w WM_PAINT
Zdecydowana większość zmiennych których użyliśmy do poprawnego umiejscowania elementów windy na ekranie użytkownika uzależniona jest od rozmiaru wyświetlacza użytkownika który jest każdorazowo pobierany przez program, 
dodatkowo w "stelarzu" naszej windy znajduje się niewidzialny prostokąt który również ułatwił nam umieszczenie pięter i drzwi na odpowiednich wysokościach

