all: veriKul.c
	gcc veriKul.c -o veriKul -pthread

clean:
	rm veriKul

