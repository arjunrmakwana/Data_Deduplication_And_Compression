# Data Deduplication And Compression

This code is a part of the final project I did for the coursework ESE 532: System on a Chip Architecture at Penn.

TL,DR: The project was a pipeline which received packets over ethernet, checked for duplicacy in the received data using Content Defined Chunking (CDC) & SHA256 and compressed non duplicate data using the Lempel–Ziv–Welch (LZW) compression algorithm. The project was implemented on a Avnet Ultra96 board which comprises of 4 ARM A53 cores and an Xilinx SoC.

This code is a prototype of the project where instead of receiving packets over ethernet, I am reading a .txt file and compressing it using the same deduplication and compression algorithms that were employed in the project. 

The project can be divided into 4 major subsections: Breaking the data, checking for duplicacy, compressing the non duplicate data, and finally storing the compressed data.

## Data Chunking

Redundancy in data was identified by first breaking the data in chunks and then checking for repetitive chunks. There can be a few methods to break the data in chunks and the one we went for is Content Defined Chunking (CDC). CDC works by calculating a rolling hash over the data and when the hash meets a certain condition, we call it a chunk. This allows us the consistency that for a given data and fixed parameters of the hash, we'll produce the same chunks. In this case, the chunks were created by rabin fingerprinting the data using a rolling hash. Once the data was broken up in chunks, the task was to check for duplicate chunks.  

## Data Deduplication

A naive way to check for duplicacy can be to store the previous data and compare it with the current data. This would take up a lot of memory and a lot of compputation time. A better way would be to get a sort of unique fingerprint of the data which would be much smaller in size than the data and compare fingerprint of the current data with the history of past fingerprints. This idea was implemented by using the cryptographic hash function SHA256. SHA256 hash function returns 256 bits of information for any string input of arbitrary length. The implementation of the showed SHA256 comparison is not the most efficient. I am maintaining a vector of a structure "chunk" and going through it to see if the chunk was seen before. While this is inefficient, the structure allows to add parameters which help for debugging. A more efficient implementation is using a unordered map of string(SHA 256 output)/8 integers and a 16 bit unsigned int. This allows for faster search through the recorded SHA256 values and was implemented in subsequent version of the code. Once non duplicate data is found, we need to compress it and the duplicate data can be signalled by sending a sort of pointer to when was it seen before.

## Data compression

Data compression was done using LZW compression algorithm which is a lossless compression algorithm. LZW basically keeps a track of the data it has seen and when it encounters the same data again, it uses the knowledge and sends sort of a pointer to the data which was already seen. I know this is an absolutely awful explanation and I highly recommend checking this video to understand LZW algorithm https://youtu.be/j2HSd3HCpDs. 

## Storing the data

LZW compression uses some CODE_LENGTH to store subsections of data transmitted. This code length is usually around 12-14 bits in case of text compression. That is, normally characters are encoded using 8 bits; but using more than 8 bits allows you to encode information more than a single character. Computers usually allow accessing data in multiples of 8 bits. The whole point of compression would not make sense if at the end we had to write the encoded 13 bit substrings as 16 bits. So I use an algorithm which takes in an unsigned 16 bit integer of which relevant data is only stored in the lower 13 bits (or the number of CODE_LENGTH bits) and store it in a buffer to 13 bit precision. That is, the buffer's first 13 bits correspond to the first encoded data, the second 13 bits to the second encoded data and so on.  

##

Once this is all done, we write the output buffer to a bin file with specific header that the decoder can use to decode the information.
