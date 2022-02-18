// ALL_COMBINED_1.cpp : This file contains the 'main' function. Program execution begins and ends there.
//


/*********************************************************/
/***************** Program flow ****************/

/*
	This prgram first reads a text file in main().
	This text file is stored in an unsigned char buffer which is sent as an argument to top_function().
	The top_function() calls cdc().
	cdc() reads te file, and calculates a rolling hash over the data.
	It checks during hashing if a specific condition is matched.
	When the condition is matched, it defines chunk boundary.
	SHA256 hash of the chunk is calculated and is checked with the past hash values.
	If the exact same hash value is found previously, then the index of that chunk is written as a 32 bit header.
	If the chunk is never seen before, it is sent to lzw_encoding algorithm.
	The algorithm uses a global unordered_map<string, int> to keep track of all the seen substrings.
	For each substring seen before (even 'a' is a substring), it writes 13 bits to a buffer.

	For this, I intend to create a buffer of buffer_len times CODE_LEN size keeping in mind the worse case
	that no substring has been seen before and all the characters will return 13bits.

	Incase if the substring(s) have been seen before, the buffer has to be shortened. For this, I can keep a
	track of how many times am I writing to the buffer and then accordingly clip the buffer short at the end.
	Or I can simply append to the file from the buffer till where the data has been written.
	And simply free the buffer after that.

	I thought that the final challenge was to keep track of buffers in an order so that I can finally write from
	the buffers to a file.

	But this will very much not be needed. I can open a binary file and keep appending data to it.

*/


/*********************************************************/


/*********************************************************/
/***************** DEFINITIONS AND MACROS ****************/


#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <iostream>
#include <stdint.h>
#include <math.h>

#include <map>
#include <unordered_map>
#include <chrono>

#include <fstream>



#define CDC_WIN_SIZE 16
#define CDC_PRIME 3
#define CDC_MODULUS 256
#define CDC_TARGET 0

#define CODE_LENGTH 13


#define uchar unsigned char
#define uint unsigned int

#define DBL_INT_ADD(a,b,c) if (a > 0xffffffff - (c)) ++b; a += c;
#define ROTLEFT(a,b) (((a) << (b)) | ((a) >> (32-(b))))
#define ROTRIGHT(a,b) (((a) >> (b)) | ((a) << (32-(b))))

#define CH(x,y,z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x,y,z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTRIGHT(x,2) ^ ROTRIGHT(x,13) ^ ROTRIGHT(x,22))
#define EP1(x) (ROTRIGHT(x,6) ^ ROTRIGHT(x,11) ^ ROTRIGHT(x,25))
#define SIG0(x) (ROTRIGHT(x,7) ^ ROTRIGHT(x,18) ^ ((x) >> 3))
#define SIG1(x) (ROTRIGHT(x,17) ^ ROTRIGHT(x,19) ^ ((x) >> 10))
/*********************************************************/



class stopwatch
{
public:
	double total_time, calls;
	std::chrono::time_point<std::chrono::high_resolution_clock> start_time, end_time;
	stopwatch() : total_time(0), calls(0) {};

	inline void reset()
	{
		total_time = 0;
		calls = 0;
	}

	inline void start()
	{
		start_time = std::chrono::high_resolution_clock::now();
		calls++;
	};

	inline void stop()
	{
		end_time = std::chrono::high_resolution_clock::now();
		auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
		total_time += static_cast<double>(elapsed);
	};

	// return latency in ns
	inline double latency()
	{
		return total_time;
	};

	// return latency in ns
	inline double avg_latency()
	{
		return (total_time / calls);
	};
};



/********************************************************* /
/***************** FUNCTION DECLARATIONS ****************/

void top_function(unsigned char* buf, int buf_len);
uint64_t hash_func(unsigned char* input, unsigned int pos);
void cdc(unsigned char* buff, unsigned int buff_size);
void print_chunk(unsigned char* data, int len);

std::string SHA256(unsigned char* data, int len);
//void SHA256Final(SHA256_CTX* ctx, uchar hash[]);
//void SHA256Update(SHA256_CTX* ctx, uchar data[], uint len);
//void SHA256Init(SHA256_CTX* ctx);
//void SHA256Transform(SHA256_CTX* ctx, uchar data[]);

void print_chunk(unsigned char* data, int len);
int HadHash_Val(std::string hash);


//std::vector<int> encoding(char* ip, int len);
//void decoding(std::vector<int> op);

/*********************************************************/


/*********************************************************/
/***************** STRUCT DECLARATIONS ****************/

typedef struct {
	//uint _number; //chunks start at 0
	//uint dup;
	uint chunk_start_idx;
	uint chunk_len;
	std::string chunk_hash;
} chunk;

typedef struct {
	uchar data[64];
	uint datalen;
	uint bitlen[2];
	uint state[8];
} SHA256_CTX;

/*********************************************************/




/*********************************************************/
/***************** GLOBAL VARIABLES ****************/

uint64_t hash_val = 0;
std::vector<chunk> chunk_vec;
int chunk_vec_count = 0;
//unsigned int prev_val = 0;
//uint32_t len;


//std::vector<int> lzw_output_code;
//std::unordered_map<std::string, int> lzw_table;
//int code = 256;


uint16_t to_cl_written; // = 0;
uint16_t to_cl_to_be_written; // = CODE_LENGTH;
uint16_t to_cl_capacity; // = 8;
uint16_t to_cl_idx;// = 0;  //initializing the index to 4 as the 1st 4 bytes will be used to write the header



int chunk_number = 0;

std::ofstream outfile;

/*********************************************************/



/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



void top_function(unsigned char* buf, int buf_len) {

	//need to call init_table() somewhere as well

	//Calling cdc. This function takes the buffer as input and uses a rolling hash to write chunk boundaries and sends the chunks to SHA256.
	//unsigned char final_op_buffer[] = {0};
	outfile.open("output_file.bin", std::ios_base::binary | std::ios_base::app);
	stopwatch timer;
	timer.start();
	cdc(buf, buf_len);
	timer.stop();
	std::cout << "Latency = " << timer.latency() / 1000000 << "ms." << std::endl;
	outfile.close();


}



/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
void filePutData(const std::string& name, const char* content, bool append = false) {
	std::ofstream outfile;
	if (append) {
		outfile.open(name, std::ios_base::app);
	}
	else {
		outfile.open(name);
	}
	outfile << content;
}
*/


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void to_code_len(uint16_t num, unsigned char* op) {

	to_cl_to_be_written = CODE_LENGTH;
	unsigned char temp;

	while (to_cl_to_be_written != 0) {

		if (to_cl_to_be_written >= 8) {

			temp = num >> (to_cl_to_be_written - to_cl_capacity);
			op[to_cl_idx] |= temp;

			to_cl_written = to_cl_capacity;
			to_cl_to_be_written = to_cl_to_be_written - to_cl_written;
			to_cl_capacity = to_cl_capacity - to_cl_written;

			if (to_cl_capacity <= 0) { to_cl_idx++; to_cl_capacity = 8; }

		}

		if (to_cl_to_be_written < 8) {

			temp = num << (8 - to_cl_to_be_written);
			op[to_cl_idx] |= temp;

			to_cl_written = to_cl_to_be_written;
			to_cl_capacity = 8 - to_cl_written;
			to_cl_to_be_written = 0;

			if (to_cl_capacity <= 0) { to_cl_idx++; to_cl_capacity = 8; }

		}

	}

}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void add_header_and_write(unsigned int flag, uint32_t idx, unsigned char* op, unsigned int len) {

	// if flag = 1; it means that the chunk is duplicate
	// idx is the index where the chunk was seen before

	//otherwise the flag = 0 and char *op is the buffer from where the data is to be written to the file
	//and len is the length of the data in the buffer



	if (flag == 1) {
		//is a duplicate chunk
		// write 1 to bit 0
		//bit 31-0 are used for index

		if (idx >= pow(2, 31)) {
			std::perror("Index bigger than 2^31 and cant be fit in 31 bits.");
		}

		else {
			uint32_t head = 1;
			uint32_t index = idx << 1;
			uint32_t final = head | index;

			//uint8_t head = 1;
			//std::cout << "\nFinal Header to be a ppended in dec: " << final;
			//printf("\nFinal Header to be appended in hex: %X", final);

			unsigned char send_1 = final >> 24;
			unsigned char send_2 = final >> 16;
			unsigned char send_3 = final >> 8;
			unsigned char send_4 = final;


			/*
			unsigned char u_head = final; //typecasted to 8 bits
			unsigned char send_1 = final >> 8;
			unsigned char send_2 = final >> 16;
			unsigned char send_3 = final >> 24;

			std::cout << u_head << std::endl;
			std::cout << send_1 << std::endl;
			std::cout << send_2 << std::endl;
			std::cout << send_3 << std::endl;


			//append to the file here
			std::ofstream outfile;
			outfile.open("output_file.bin", std::ios_base::app);
			//outfile << final;
			outfile << u_head;
			outfile << send_1;
			outfile << send_2;
			outfile << send_3;
			//outfile << u_head;

			outfile.close();
			*/


			//std::ofstream outfile;
			//outfile.open("output_file.bin", std::ios_base::app);
			outfile << send_4;
			outfile << send_3;
			outfile << send_2;
			outfile << send_1;

			//outfile.close();


		}


	}

	else if (flag == 0) {

		unsigned int times; // = unsigned int(ceil((CODE_LENGTH * len) / 8)) + 1;

		if (CODE_LENGTH * len % 8 == 0) {
			times = CODE_LENGTH * len / 8;
		}
		else {
			times = CODE_LENGTH * len / 8 + 1;
		}

		//std::cout << "\n\ntimes = " << times << "\n";


		unsigned char* tempp = (unsigned char*)malloc(times * sizeof(unsigned char));

		for (unsigned int i = 0; i < (times);i++) {
			tempp[i] = op[i];
		}



		//for (int idx = 0; idx < times; idx++) {
			//printf("tempp[%d] is %X\n", idx, tempp[idx]);
		//}


		uint32_t head = 0;
		uint32_t length = times << 1;
		uint32_t final = head | length;

		//std::cout << "\nFinal Header to be appended in dec: " << final;
		//printf("\nFinal Header to be appended in hex: %X", final);



		unsigned char send_1 = final >> 24;
		unsigned char send_2 = final >> 16;
		unsigned char send_3 = final >> 8;
		unsigned char send_4 = final;



		/*
		unsigned char u_head = final;
		unsigned char send_1 = final >> 8;
		unsigned char send_2 = final >> 16;
		unsigned char send_3 = final >> 24;


		std::ofstream outfile;
		outfile.open("output_file.bin", std::ios_base::app);
		outfile << u_head;
		outfile << send_1;
		outfile << send_2;
		outfile << send_3;

		outfile << tempp;

		outfile.close();

		//append to the file here
		*/



		//std::ofstream outfile;
		//outfile.open("output_file.bin", std::ios_base::app);
		outfile << send_4;
		outfile << send_3;
		outfile << send_2;
		outfile << send_1;
		//

		//outfile << tempp;

		for (int idx = 0; idx < times;idx++) {
			outfile << tempp[idx];
		}


		//outfile.close();


		free(tempp);
	}
	return;

}




//bool table_init = false;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
void init_table() {

}
*/

void encoding(unsigned char* ip, int len) {

	std::unordered_map<std::string, int> lzw_table;
	int code = 256;

	//if (!table_init) {
		//std::cout << "initializing lzw table." << std::endl;
		for (int i = 0; i <= 255; i++) {
			std::string ch = "";
			ch += char(i);
			lzw_table[ch] = i;
		}
		//table_init = true;
	//}

	to_cl_written = 0;
	//to_cl_to_be_written = CODE_LENGTH;
	to_cl_capacity = 8;
	to_cl_idx = 0;

	//std::cout << "Encoding.\n";
	//std::unordered_map<std::string, int> table;


	int how_much_written = 0;


	uint32_t size_required; // = uint32_t(ceil((CODE_LENGTH * len) / 8)) + 1;

	if (CODE_LENGTH * len % 8 == 0) {
		size_required = CODE_LENGTH * len / 8;
	}
	else {
		size_required = (CODE_LENGTH * len / 8) + 1;
	}

	//printf("\nsize required: %d\n", size_required);

	unsigned char* op = (unsigned char*)calloc((size_required), sizeof(unsigned char));

	std::string p = "", c = "";
	p += ip[0];
	//int code = 256;
	//std::vector<int> output_code; // can be declared as a global var. This can be appended everytime. NO need to return.

	int i = 0;

	//I think I also need to add another loop beneath the while loop which processes
	//the substrings in case we are out of capacity of the CODE_LEN.
	//In that case, we need to just check for presence of the same substring in the table
	//and send its idx if present, else send the idx of the samller substring.

	//Basically do everything the loop below does apart from adding to the table.

	//I think maybe the last line that writes to the code should also be removed or aaatleast out inside 
	//an if loop. NOT SURE though. Need to figure that out. But should be fine for now. ^^

	//for now though, i think prince.txt does not exceed the 8191 capacity so should be fine.

	//^ not really needed.
	//the if(code < pow(2,CODE_LENGTH)) inside the else loop does the work.

	while (i < len) {


		if (i != len - 1) {
			c += ip[i + 1];
		}

		//std::unordered_map<std::string, int>::const_iterator got = lzw_table.find(p+c);

		//if (table.find(p+c) != table.end())
		//if (got != lzw_table.end()) {
		if (lzw_table.find(p + c) != lzw_table.end()) {
			p = p + c;
		}
		else {
			//lzw_output_code.push_back(lzw_table[p]);

			to_code_len(lzw_table[p], op);
			//std::cout << "\n\nLZW table index: " << lzw_table[p] << "\nLZW table entry: " << p << "\nLength of it is: " << p.length();
			how_much_written++;

			//if (code < (pow(2, CODE_LENGTH))) {
			lzw_table[p + c] = code;
			code++;
			//}
			p = c;
		}
		c = "";
		i++;
	}
	//lzw_output_code.push_back(lzw_table[p]);
	to_code_len(lzw_table[p], op);
	//std::cout << "\n\nLZW table index: " << lzw_table[p] << "\nLZW table entry: " << p << "\nLength of it is: " << p.length();
	how_much_written++;

	std::cout << "\n number of 13 bits written: " << how_much_written << "\n";

	//how much of the buffer have we written? send the buffer to a function which adds the header and 
	//write to the bin file
	add_header_and_write(0, 0, op, how_much_written);

	free(op);

	return;
	//at this  point we have a vector of integer values

}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

uint64_t hash_func(unsigned char* input, unsigned int pos)
{
	// put your hash function implementation here
	if (hash_val == 0)
		for (int i = 0;i < CDC_WIN_SIZE;i++)
			hash_val += (int)input[pos + CDC_WIN_SIZE - 1 - i] * (pow(CDC_PRIME, i + 1));
	else

		hash_val = hash_val * CDC_PRIME - input[pos - 1] * pow(CDC_PRIME, CDC_WIN_SIZE + 1) + input[pos - 1 + CDC_WIN_SIZE] * CDC_PRIME;

	return hash_val;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void cdc(unsigned char* buff, unsigned int buff_size)
{
	unsigned int prev_val = 0;
	uint32_t len;

	for (unsigned int i = CDC_WIN_SIZE;i < (buff_size - CDC_WIN_SIZE);i++) {
		if (((hash_func(buff, i) % CDC_MODULUS)) == CDC_TARGET) {
			std::printf("%d to %d \nCHUNK:\n", prev_val, i - 1);
			
			len = i - prev_val; // removed the -1
			
			print_chunk(&buff[prev_val], len);
			std::printf("\nlen = %d\n", len);
			std::string ss = SHA256(&buff[prev_val], len);

			int status = HadHash_Val(ss);

			if (status != -1)
			{
				uint32_t u_status = uint32_t(status); //changeed to unsgigned int
				std::cout << "\nThis chunk was seen before at index" << u_status << std::endl;
				//call the header function here.

				add_header_and_write(1, u_status, NULL, 0);

			}
			else {
				std::cout << "\nThis is a new chunk.\n\n" << std::endl;

				chunk_vec.push_back(chunk());

				chunk_vec[chunk_vec_count].chunk_start_idx = prev_val;
				chunk_vec[chunk_vec_count].chunk_len = len;
				chunk_vec[chunk_vec_count].chunk_hash = ss;
				//mem_compress(&buff[prev_val],&len)
				chunk_vec_count++;



				//call LZW_encoding here
				//also the LZW encoding will write to the buffer in 13 bits and add a header to it
				encoding(&buff[prev_val], len);

			}

			//call LZW here

			//encoding(&buff[prev_val], len);

			//change the encoding function

			// LZW_encode(&buff[prev_val], len);
			//chunk_vec.emplace_back(prev_val,len,ss);

			std::cout << "\nHASH VALUE:\n" << ss << "\n\n" << std::endl;
			prev_val = i;
		}
	}
	std::printf("%d to %d \n", prev_val, buff_size);
	len = buff_size - prev_val;
	std::printf("\nlen = %d\n", len);
	print_chunk(&buff[prev_val], len);
	std::string ss = SHA256(&buff[prev_val], len);
	int status = HadHash_Val(ss);

	if (status != -1)
	{
		uint32_t u_status = uint32_t(status); //changeed to unsgigned int
		std::cout << "\nThis chunk was seen before at index" << u_status << std::endl;
		//call the header function here.

		add_header_and_write(1, u_status, NULL, 0);
	}
	else {
		std::cout << "\nThis is a new chunk." << std::endl;
		chunk_vec.push_back(chunk());

		chunk_vec[chunk_vec_count].chunk_start_idx = prev_val;
		chunk_vec[chunk_vec_count].chunk_len = len;
		chunk_vec[chunk_vec_count].chunk_hash = ss;

		chunk_vec_count++;


		//call LZW here
		//also the LZW encoding will write to the buffer in 13 bits and add a header to it
		encoding(&buff[prev_val], len);
	}

	hash_val = 0;
	std::cout << "\nHASH VALUE:\n" << ss << "\n\n" << std::endl;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void print_chunk(unsigned char* data, int len) {
	for (int i = 0; i < len; i++) { // removed the = sign and made it just <
		std::cout << data[i];
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int HadHash_Val(std::string hash) {
	for (int n = 0; n < int(chunk_vec.size()); n++) {
		if (hash == chunk_vec[n].chunk_hash) {
			return n;
		}
	}
	return -1;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



uint k[64] = {
	0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
	0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
	0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
	0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
	0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
	0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
	0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
	0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

void SHA256Transform(SHA256_CTX* ctx, uchar data[])
{
	uint a, b, c, d, e, f, g, h, i, j, t1, t2, m[64];

	for (i = 0, j = 0; i < 16; ++i, j += 4)
		m[i] = (data[j] << 24) | (data[j + 1] << 16) | (data[j + 2] << 8) | (data[j + 3]);
	for (; i < 64; ++i)
		m[i] = SIG1(m[i - 2]) + m[i - 7] + SIG0(m[i - 15]) + m[i - 16];

	a = ctx->state[0];
	b = ctx->state[1];
	c = ctx->state[2];
	d = ctx->state[3];
	e = ctx->state[4];
	f = ctx->state[5];
	g = ctx->state[6];
	h = ctx->state[7];

	for (i = 0; i < 64; ++i) {
		t1 = h + EP1(e) + CH(e, f, g) + k[i] + m[i];
		t2 = EP0(a) + MAJ(a, b, c);
		h = g;
		g = f;
		f = e;
		e = d + t1;
		d = c;
		c = b;
		b = a;
		a = t1 + t2;
	}

	ctx->state[0] += a;
	ctx->state[1] += b;
	ctx->state[2] += c;
	ctx->state[3] += d;
	ctx->state[4] += e;
	ctx->state[5] += f;
	ctx->state[6] += g;
	ctx->state[7] += h;
}

void SHA256Init(SHA256_CTX* ctx)
{
	ctx->datalen = 0;
	ctx->bitlen[0] = 0;
	ctx->bitlen[1] = 0;
	ctx->state[0] = 0x6a09e667;
	ctx->state[1] = 0xbb67ae85;
	ctx->state[2] = 0x3c6ef372;
	ctx->state[3] = 0xa54ff53a;
	ctx->state[4] = 0x510e527f;
	ctx->state[5] = 0x9b05688c;
	ctx->state[6] = 0x1f83d9ab;
	ctx->state[7] = 0x5be0cd19;
}

void SHA256Update(SHA256_CTX* ctx, uchar data[], uint len)
{
	for (uint i = 0; i < len; ++i) {
		ctx->data[ctx->datalen] = data[i];
		ctx->datalen++;
		if (ctx->datalen == 64) {
			SHA256Transform(ctx, ctx->data);
			DBL_INT_ADD(ctx->bitlen[0], ctx->bitlen[1], 512);
			ctx->datalen = 0;
		}
	}
}

void SHA256Final(SHA256_CTX* ctx, uchar hash[])
{
	uint i = ctx->datalen;

	if (ctx->datalen < 56) {
		ctx->data[i++] = 0x80;

		while (i < 56)
			ctx->data[i++] = 0x00;
	}
	else {
		ctx->data[i++] = 0x80;

		while (i < 64)
			ctx->data[i++] = 0x00;

		SHA256Transform(ctx, ctx->data);
		memset(ctx->data, 0, 56);
	}

	DBL_INT_ADD(ctx->bitlen[0], ctx->bitlen[1], ctx->datalen * 8);
	ctx->data[63] = ctx->bitlen[0];
	ctx->data[62] = ctx->bitlen[0] >> 8;
	ctx->data[61] = ctx->bitlen[0] >> 16;
	ctx->data[60] = ctx->bitlen[0] >> 24;
	ctx->data[59] = ctx->bitlen[1];
	ctx->data[58] = ctx->bitlen[1] >> 8;
	ctx->data[57] = ctx->bitlen[1] >> 16;
	ctx->data[56] = ctx->bitlen[1] >> 24;
	SHA256Transform(ctx, ctx->data);

	for (i = 0; i < 4; ++i) {
		hash[i] = (ctx->state[0] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 4] = (ctx->state[1] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 8] = (ctx->state[2] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 12] = (ctx->state[3] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 16] = (ctx->state[4] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 20] = (ctx->state[5] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 24] = (ctx->state[6] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 28] = (ctx->state[7] >> (24 - i * 8)) & 0x000000ff;
	}
}

std::string SHA256(unsigned char* data, int len) {
	//int strLen = strlen(data);
	int strLen = len;
	SHA256_CTX ctx;
	unsigned char hash[32];
	std::string hashStr = "";

	SHA256Init(&ctx);
	SHA256Update(&ctx, (unsigned char*)data, strLen);
	SHA256Final(&ctx, hash);

	char s[3];
	for (int i = 0; i < 32; i++) {
		sprintf(s, "%02x", hash[i]);
		hashStr += s;
	}
	//std::cout << "hashstr length = " << hashStr.length() << std::endl;
	return hashStr;
}


void test_cdc(const char* file)
{
	FILE* fp = fopen(file, "r");
	if (fp == NULL) {
		perror("fopen error");
		return;
	}

	fseek(fp, 0, SEEK_END); // seek to end of file
	int file_size = ftell(fp); // get current file pointer
	fseek(fp, 0, SEEK_SET); // seek back to beginning of file

	unsigned char* buff = (unsigned char*)malloc((sizeof(unsigned char) * file_size));
	if (buff == NULL)
	{
		perror("not enough space");
		fclose(fp);
		return;
	}

	//std::cout << buff;

	int bytes_read = fread(&buff[0], sizeof(unsigned char), file_size, fp);

	top_function(buff, file_size);

	free(buff);
	return;
}


int main() {
	test_cdc("LittlePrince.txt");
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
