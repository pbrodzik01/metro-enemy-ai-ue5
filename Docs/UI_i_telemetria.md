# Moduł C++ `MetroEnemyAI`: interfejs i telemetria AI

Moduł dodaje do projektu interfejs gracza oraz narzędzia do pomiaru działania AI przeciwników.
Nie zmienia żadnego Blueprintu, mapy ani assetu z paczek. Wszystko uruchamia się samo przy starcie poziomu.

## Co jest w grze

| Element | Opis |
|---|---|
| Menu główne | Rozpocznij / Sterowanie / Wyjście. Gra stoi na pauzie, dopóki nie klikniesz „Rozpocznij”. |
| HUD | Zdrowie gracza (lewy dół), cel misji (lewa góra), liczba wrogów, eliminacje i czas (prawa góra), podpowiedzi klawiszy (prawy dół). |
| Etykiety nad przeciwnikami | Typ przeciwnika, aktualne zachowanie odczytane z klucza `Behavior` w Blackboardzie (kolor zależy od rodzaju zachowania), pasek zdrowia, czerwony „!” gdy celem jest gracz. |
| Komunikaty | „Zauważył cię: …”, „Wyeliminowano: …”, zapis telemetrii itp. |
| Efekty trafień | Znacznik trafienia przy celowniku, czerwona winieta przy otrzymaniu obrażeń. |
| Pauza (P, w grze uruchomionej poza edytorem także Esc) | Wznów / Sterowanie / Zacznij od nowa / Menu główne / Wyjście. |
| Ekran końca | Zwycięstwo (wszyscy wrogowie wyeliminowani) albo porażka (śmierć gracza), statystyki AI. |
| Panel AI (F1) | Tabela przeciwników: zachowanie, HP, dystans, cel, czas w bieżącym zachowaniu; parametry percepcji odczytane z kontrolera; dziennik ostatnich decyzji. |

Klawisze: F1 panel AI, F2 tryb etykiet (wszystkie / tylko widoczne / wyłączone), F3 zapis telemetrii, F5 test obciążenia (+5 przeciwników), P pauza.

Menu główne można pominąć poleceniem konsoli `thesis.SkipMenu 1`.

## Telemetria (dane do rozdziału z testami)

Po zakończeniu gry, przy restarcie, wyjściu lub po F3 zapisuje się folder
`Saved/TelemetriaAI/<data_godzina>/` z plikami CSV (separator `;`, przecinek dziesiętny, otwierają się wprost w polskim Excelu):

| Plik | Zawartość |
|---|---|
| `zdarzenia.csv` | Każda zmiana zachowania (z → na), wykrycie gracza z czasem reakcji, utrata celu, eliminacje, start/koniec. Kolumny: czas, przeciwnik, typ, dystans do gracza, HP obu stron, liczba aktywnych przeciwników, FPS. |
| `przeciwnicy.csv` | Na przeciwnika: zasięg i kąt wzroku, zasięg słuchu, liczba zmian zachowania, liczba oscylacji, średni/min/max czas reakcji, łączny czas w każdym zachowaniu. |
| `wydajnosc.csv` | Co sekundę: liczba aktywnych przeciwników, FPS, czas klatki. Razem z F5 daje wykres wydajności względem liczby NPC. |
| `sesja.csv` | Wynik, czas sesji, eliminacje, średni i minimalny FPS, maksymalna liczba aktywnych przeciwników. |

Jak liczone są wskaźniki:

- **Czas reakcji**: od chwili, gdy gracz znalazł się w zasięgu i stożku wzroku przeciwnika z czystą linią widoczności (parametry wzięte z `AISenseConfig_Sight` kontrolera), do chwili, gdy klucz `Target` w Blackboardzie wskazał gracza. Wykrycia bez kontaktu wzrokowego (słuch, obrażenia) liczone są osobno.
- **Oscylacja**: powrót do poprzedniego zachowania w czasie krótszym niż 1 s (np. Pościg → Atak z dystansu → Pościg). To liczbowa miara „zbyt częstego przełączania gałęzi” opisanego w podrozdziale 9.3 pracy.

## Pliki

```
Source/
  MetroEnemyAI.Target.cs, MetroEnemyAIEditor.Target.cs  cele kompilacji gry i edytora
  MetroEnemyAI/MetroEnemyAI.Build.cs                  zależności modułu
  MetroEnemyAI/Public/
    ThesisGameSubsystem.h    podsystem świata: stan gry, śledzenie przeciwników, telemetria
    ThesisSessionSubsystem.h ustawienia zachowywane między restartami poziomu
    ThesisTelemetry.h        zapis CSV
    ThesisReflection.h       odczyt zmiennych Blueprintów (CurrentHealth, TargetTags, IsAlive?)
    ThesisText.h             polskie nazwy zachowań i formatowanie liczb
    ThesisTypes.h            struktury danych
  MetroEnemyAI/Private/UI/
    SThesisOverlay           warstwa UI dodawana do viewportu
    SThesisHudPainter        HUD, etykiety nad przeciwnikami, panel AI
    SThesisMenu              menu główne, pauza, sterowanie, ekran końca
    ThesisStyle              kolory, czcionki, style przycisków
```

UI jest napisane w Slate (C++), więc nie wymaga żadnych Widget Blueprintów. Stary pasek zdrowia z `WB_Player` jest ukrywany w trakcie gry, celownik z `WB_Player` zostaje.

## Rozbudowana mapa i postać

| Element | Gdzie | Opis |
|---|---|---|
| `Metro_Rozbudowana` | `Content/Mapy` | Kopia Metro1 plus skrzydło techniczne: korytarz 21 m z bocznego pomieszczenia stacji i hala 28 × 35 m (filary, rampy, murki, ogrodzenia, tor, oświetlenie jak na stacji). Teraz to domyślna mapa gry i edytora. Metro1 zostało bez zmian. |
| Przeciwnicy | w nowej mapie | 13 przeciwników (7 dystansowych, 6 kontaktowych). Kontaktowi `BP_AI_SoldierMelee_TeamA` z Metro1 zamienieni na `BP_AI_SoldierMelee`, bo wariant TeamA polował tylko na drużynę B i ignorował gracza. 2 przeciwników patroluje pętlę `Hala_Patrol`, 2 wędruje (Roaming). |
| NavMesh | w nowej mapie | Objętość nawigacji poszerzona na nowe skrzydło, generowanie w trakcie gry (Dynamic). |
| Postać gracza | `Content/Ocalaly` | `BP_Ocalaly` (dziedziczy po `BP_CharacterExample`) z materiałem `M_Ocalaly`: matowa ciemna oliwka z teksturą brudu. Tryb gry `BP_TrybGry_Ocalaly` ustawiony jako domyślny w `Config/DefaultEngine.ini`. |
| Przeciwnicy | `Content/Ocalaly/Wrogowie` | `BP_Wrog_SoldierRange` (szaroniebieski) i `BP_Wrog_SoldierMelee` (rdzawobrązowy), pochodne klas z paczki, zmieniają tylko materiały. Wszyscy przeciwnicy w `Metro_Rozbudowana` i fale testu obciążenia używają tych klas. Skrypt: `Scripts/build_wrogowie_skin.py`. |

Mapę i postać generują skrypty w `Scripts/`, uruchamiane w edytorze bez okna:

```
UnrealEditor-Cmd.exe MetroEnemyAI.uproject -run=pythonscript -script=Scripts/build_ocalaly_skin.py -unattended
copy Content\MetroMaintenanceStation\Maps\Metro1.umap Content\Mapy\Metro_Rozbudowana.umap
UnrealEditor-Cmd.exe MetroEnemyAI.uproject -run=pythonscript -script=Scripts/build_metro_rozbudowana.py -unattended
```

## Tryby automatyczne (do testów i zrzutów)

| Parametr uruchomienia | Działanie |
|---|---|
| `-ThesisTour` | Gra sama przechodzi przez menu, sterowanie, HUD, panel AI, test obciążenia, pauzę i ekran końca, zapisuje zrzuty do `Saved/Screenshots/ThesisTour` i się zamyka. |
| `-ThesisSpot=x,y,z,yaw` | Razem z `-ThesisTour`: przenosi gracza w podane miejsce przed zrzutami (np. `-ThesisSpot=-4100,1910,120,180` to wejście do hali). |
| `-ThesisBenchmark=240` | Sesja pomiarowa: gracz nieśmiertelny, co 20 s fala 5 przeciwników (do 40), po 240 s zapis CSV i wyjście. |

Przykład: `UnrealEditor.exe MetroEnemyAI.uproject -game -windowed -ResX=1600 -ResY=900 -ThesisBenchmark=240`

