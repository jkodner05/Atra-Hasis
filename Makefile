objects = Jarasandha.o Geshtu.o AtraHasis.o

AtraHasis: $(objects)
	cc -g -Wall -Werror -pedantic -o AtraHasis $(objects)

Jarasandha.o: Jarasandha.c Jarasandha.h
	cc -g -Wall -pedantic -c Jarasandha.c

Geshtu.o: Geshtu.c Geshtu.h
	cc -g -Wall -pedantic -c Geshtu.c

AtraHasis.o: AtraHasis.c AtraHasis.h
	cc -g -Wall -Werror -pedantic -c AtraHasis.c

clean:
	rm AtraHasis $(objects)

