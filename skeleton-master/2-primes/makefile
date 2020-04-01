all: primes

primes: primes.o
	ld -o primes primes.o

primes.o: primes.asm
	nasm -f elf64 -g primes.asm

clean:
	rm -f primes primes.o

