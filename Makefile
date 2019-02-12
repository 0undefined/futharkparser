default:
	gcc -O2 -c -o futhark_parser.o futhark_parser.c
	gcc -O2 -c -o main.o main.c
	gcc -o parser_demo main.o futhark_parser.o
	./parser_demo

debug:
	gcc -g -Og -c -o futhark_parser.o futhark_parser.c
	gcc -g -Og -c -o main.o main.c
	gcc -o parser_demo main.o futhark_parser.o
	gdb parser_demo

clean:
	rm -f *.o parser_demo
