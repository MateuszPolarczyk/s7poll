# s7poll
Narzędzie do odczytu danych ze sterownika z wykorzystaniem protokołu SNAP7.

## Pomoc

Uruchomienie narzędzia
```
s7poll <ADRES IP> <PARAMETRY>
```
Przykład
```
s7poll 127.0.0.1 -db 107 -r 0 -s 2 -p 0 -i 10 -int -1
```

Parametr | Opis | Domyslnie
--- | --- | ---
adres IP | Adres IP PLC | ---
-h | Wyświetli pomoc | ---
-r | Rack | 0
-s | Slot | 1
-db | Numer bloku danych | 1
-p | Początkowy adres danych | 0
-i | Ilość danych do wyświetlenia | 5
-time | Czas miedzy zapytaniami | 1000 (ms)
-1 | Odczyta dane z serwera tylko raz | ---
-int | Dane w postaci liczb całkowitych | ---
-float | Dane w postaci liczb zmiennoprzecinkowych | ---
-hex | Dane w systemie szesnastkowym | ---
-bin | Dane w systemie dwojkowym | ---
