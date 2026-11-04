# Changelog

## [0.2.5] - 2026-11-04
- SYNC e TEMPO in un'unica finestra scura, come in Dr. Segg-uencer e Multitails: menu DAW/FREE a sinistra, a destra il tempo in uso in grande (quello dell'host in DAW). In FREE si trascina il numero per cambiare il BPM (doppio clic: 120). La finestra sta sul bordo destro, nella stessa posizione degli altri plugin; OUTPUT ha la sua finestra subito a sinistra.
- Titoli delle sezioni senza targa e senza ombra: parola chiave chiara sopra il nome in grassetto, entrambe spaziate e allineate a sinistra, con una linea sottile sotto. A destra il modo della sezione (L C R sull'imager).
- La scheda del sequencer si chiama SEQ.
- S&H e Smooth Rnd mostrano i valori casuali reali generati dall'LFO, che scorrono nel tempo, invece di una forma fissa; anche le anteprime sulle schede.
- L'etichetta MOD MATRIX ha la stessa targa e la stessa ombra del tasto FREEZE.
- FREEZE più in alto: tra la sua ombra e la linea del titolo resta la stessa distanza che c'è tra le scritte e la linea.

## [0.2.4] - 2026-11-03
- Interfaccia molto più fluida: lo sfondo e i controlli fermi vengono conservati in immagine invece di essere ridisegnati a ogni fotogramma. Nelle misure il tempo di disegno scende da 100-180 ms a meno di 10 ms per fotogramma.
- Forma d'onda, scope e schede LFO si aggiornano solo quando hanno qualcosa di nuovo da mostrare; senza audio non si ridisegnano.

## [0.2.3] - 2026-11-03
- macOS: nuovo installer `.pkg` con VST3 e AU. Basta approvare il pacchetto una volta, e i plugin installati non restano bloccati da Gatekeeper.
- macOS: corretta la versione minima del sistema: le build ora partono da macOS 11, come previsto.
- La CI verifica firma ad hoc, versione minima e installer a ogni build.

## [0.2.2] - 2026-11-03
- Finestra ridimensionabile dall'angolo in basso a destra o dall'host, da 800×500 a 2560×1600, sempre in 16:10. La dimensione scelta si salva con la sessione.
- Interfaccia nitida a qualunque dimensione e scala dello schermo: tutto viene disegnato in vettoriale alla risoluzione reale, non più ingrandito come immagine.
- Spaziature uniformi: 16 px dai bordi della finestra e dentro ogni pannello, 12 px tra i pannelli; i controlli sono distribuiti in celle uguali.
- I testi di SYNC, BPM e OUTPUT ora stanno dentro le loro targhe; con SYNC su DAW la targa SYNC si accosta a OUTPUT.
- Screenshot del README a doppia densità (`--snap file.png [pagina] [scala]`).

## [0.2.1] - 2026-11-03
- Nuova grafica: pannelli in vetro su fondo blu, targhe nere inclinate, potenziometri scuri con arco giallo per il valore e valori in carattere pixel.
- LFO 1–4 e METRO SEQ stanno in un'unica area a schede in basso (una pagina alla volta, con anteprima della forma su ogni scheda); la mod matrix resta sempre visibile a destra.
- La modulazione dal vivo si vede ancora su potenziometri e barre: arco verde per l'escursione (giallo sull'area verde), tacca sul valore impostato.
- Font incorporati: Rubik e Silkscreen (SIL Open Font License, in `Resources/fonts`).
- Nessun cambiamento ai parametri: preset e sessioni salvate restano compatibili.

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
