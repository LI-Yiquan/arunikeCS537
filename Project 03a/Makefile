compile:
	gcc -g -Wall -Werror -pthread -O -o psort psort.c

run: compile
	./psort input output

hexDump: input output
	hexdump input > inputHex.txt
	hexdump output > outputHex.txt

clean:
	rm -rf ./*.o
	rm -f output.bin
	rm -f inputHex.txt
	rm -f outputHex.txt