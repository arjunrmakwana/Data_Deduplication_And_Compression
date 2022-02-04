# Data Deduplication And Compression

This was the final project I did for the coursework ESE 532: System on a Chip Architecture at Penn.

TL,DR: The project was a pipeline which received packets over ethernet, checked for duplicy in the received data using Content Defoned Chunking (CDC) & SHA256, and compressed non duplicate data using the Lempel–Ziv–Welch (LZW) compression algorithm. The project was implemented on a Avnet Ultra96 board which comprises of 4 ARM A53 cores and an Xilinx SoC.

###DATA DEDUPLICATION

Duplicacy in data was checked by implementing content defined chunking (CDC) followed by calculating SHA256 hash of the chunks. This stage was pipelined and implemented on 4 ARM cores of the Avnet ultra 96 board.

