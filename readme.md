# Test Thrill in FAM

Thrill einrichten als Submodule:
https://blog.brakmic.com/thrill-big-data-processing-with-c/

Vorbereitung
1. generieren einer Textdatei mit zufällig erzeugten Worten in 50 Mb und 9.4Gib
2. Variante viele verschiedene Worte: generate_words-many.c
3. Variante zwei verschiedene Worte: generate_words-2.c
4. Daten ins FAM (Path:/lfs) schreiben via mmap(): writer.c

Thrill besitzt eigene Datenstruktur das DIA. Die Daten können mit drei verschiedenen Funktionen eingelesen werden:

1. ReadLines: word_count_simple_timer.cpp
Liest die Daten direkt aus der Textdatei.

2. EqualtoDia: word_count_mmap.ccp
Liest die Daten aus einen Vektor.

3. Generate: word_count_mmap_generate.ccp
Liest die Daten aus erzeugten (generate) Daten.

## Quellen

https://project-thrill.org/docs/master/start_compile.html
https://arxiv.org/pdf/1608.05634.pdf
