default:
	gcc -c -o futhark_parser.o futhark_parser.c
	gcc -c -o main.o main.c
	gcc -o parser_demo main.o futhark_parser.o
	./parser_demo

clean:
	rm -f *.o parser_demo
