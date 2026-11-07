# Jagged Intelligence

**KI hat Kanten. Wir bauen Brücken.** Die neue MorphDemo-Produktion hat eine eigene Seite unter
[Dreambau](https://dreambau.com/homepage-assets/jagged-intelligence/). Sie wählt beim Laden genau eine Fassung.
„Andere Fassung“ startet eine zufällig gewählte andere Fassung. Für direkte Links und die Prüfung bleiben
`?anim=4k`, `?anim=16k`, `?anim=64k` und `?anim=60k` verfügbar.

| Fassung | Obergrenze (Bytes) | Bild und Musik |
|---|---:|---|
| Kante | 4.096 | Ein gezacktes Signal wird zum Geländeschnitt für eine Brücke. Tiefe Pulse und ein trockenes Dreieckssignal. |
| Tragwerk | 16.384 | Neun Felder setzen sich zu einem Tragwerk zusammen. Der Takt gibt den Einsätzen der Bauteile ihre Reihenfolge. |
| Schlucht | 65.536 | Eine Hängebrücke schließt sich über einer Schlucht im Abendlicht. Die Musik wächst von einzelnen Akkorden zu einem weiten Raum. |
| Signal | 60.000 | Drei Prüfstationen bündeln Spuren zu einer klaren Linie. Ein gelber Bogen zeigt den Weg für unsichere Fälle. Kurze elektronische Töne finden zu einem gemeinsamen Puls. |

Alle vier laufen 60 Sekunden und verwenden dieselbe Abfolge: Kante, Brückenbau ab 16,8 s, Übergang ab 33,6 s.
Drei Musikereignisse bei 45,6, 48 und 50,4 s bringen nacheinander die drei Zeilen des Schlussbilds.
Kontext, Tests und menschliches Urteil sind die drei Teile der Brücke. Das 95/5-Bild stammt aus dem
[Gespräch mit Diogo Almeida](https://www.mattturck.com/podcast/95-einstein-5-mr-bean-jevs-creator-on-why-ai-cant-automate-anything-reliably-dio)
und ist eine Metapher. Die Forschung zur [Jagged Technological Frontier](https://doi.org/10.1287/orsc.2025.21838)
beschreibt ungleiche KI-Leistung über Aufgaben hinweg; sie liefert keine 95/5-Quote.

## Bauen und ansehen

Aus `dreambau-landing/`:

```sh
npm ci
npm run build:jagged
CSP=apex npm run serve
```

Dann [die lokale Vorschau](http://localhost:8080/jagged-intelligence/) öffnen. Die Quellfassungen liegen in
`src/jagged/`, der fertige statische Ordner in `site/jagged-intelligence/`. Der Build minifiziert die gemeinsame
Laufzeit und die Fassungen einzeln. Jede Fassung enthält den eigenen Shader, die Komposition und den eingebauten
gemeinsamen Zeichencode; die Größenprüfung zählt diese Datei unkomprimiert. Die Seite lädt nur die ausgewählte
Fassung. Sie benötigt keine Bilder, Tonaufnahmen, Webfonts, Dienste oder Inline-Skripte.

`M` schaltet den Ton, `Esc` springt zum Schlussbild. Wenn das Betriebssystem reduzierte Bewegung anfordert, zeigt
die Seite das Schlussbild stumm. Ohne WebGL oder JavaScript bleiben die Zeichnung, Texte und Links sichtbar.

## Prüfen und veröffentlichen

```sh
npx playwright install chromium
npm run check:jagged -- --complete
```

Alternativ kann `CHROMIUM_EXECUTABLE` auf ein lokal vorhandenes Chromium zeigen. Der Test prüft die Bytes über HTTP,
alle Fassungen mit Klang, den Aufbau der drei Schlusszeilen, Tastatur und Ton, responsive Ansichten, reduzierte Bewegung
und den statischen Rückfall. `--complete` prüft zusätzlich den vollständigen 60-Sekunden-Durchlauf aller vier Fassungen.
Der Report und Browserbilder entstehen unter `verification/jagged/tmp/`. Für eine Tonanalyse mit NumPy und Pillow:
`BASE_URL=http://localhost:8080/jagged-intelligence/ node tools/audio.mjs 60k --out verification/jagged/tmp/audio-60k`.

Nur `site/jagged-intelligence/` wird veröffentlicht. Die Demo läuft im vorhandenen Bereich
`/homepage-assets/jagged-intelligence/` mit derselben strengen Content Security Policy wie die Startseite.
Der vollständige Ordner wird abseits der öffentlichen Auslieferung bereitgestellt, anhand von SHA-256 geprüft
und mit einem atomaren Verzeichniswechsel freigeschaltet. Bei einer ersten Veröffentlichung besteht der Rückweg
darin, genau den neuen Ordner wieder aus der Auslieferung zu verschieben. Root-Startseite, Proxy und Webserver
sind für diese Veröffentlichung keine Änderungsziele.

```sh
BASE_URL=https://dreambau.com/homepage-assets/jagged-intelligence/ npm run check:jagged
```
