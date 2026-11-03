# Changelog

## [0.2.0] - 2026-11-03
- LFO 3 e LFO 4, disponibili come sorgenti nella mod matrix. I quattro pannelli LFO stanno nella stessa finestra 1280×800; in modalità FREE il Rate è una barra invece di un potenziometro.
- Potenziometri e valori numerici seguono la modulazione della matrice in tempo reale, con arco oro per l'escursione e tacca sul valore impostato. Il parametro non viene scritto.
- I valori senza decimali ora sono arrotondati (prima poteva comparire "40.6533 %").
- Le sessioni salvate restano compatibili. Attenzione solo all'automazione del parametro Source: gli host la salvano in forma normalizzata, quindi con 7 sorgenti invece di 5 punta a una sorgente diversa.

## [0.1.0] - 2026-11-03
Prima release pubblica.
- Granulatore live con forma d'onda e finestra Position/Size/Spray evidenziata, Freeze, Cloud, Feedback.
- Glitch a tempo (Slice, Rpt Pitch, Hold) e sezione Colour (Bits, Downsample, Tone, Wow, Flutter, Drive).
- Filtri HPF + Q e LPF + Reso.
- Imager Width 0–200 % + Bass Mono con spettroscopio polare L C R e correlazione.
- Sync DAW / FREE.
- 2 LFO e sequencer METRO a 8 stadi (pulse, ratchet, gate, slide, probabilità).
- Mod matrix a 8 slot su 25 destinazioni.
- 8 preset di fabbrica e preset utente (salva / cancella).
- Interfaccia 1280×800.
- Formati: VST3 (macOS, Windows, Linux), AU, AAX (non firmato).
