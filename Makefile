objects = Jarasandha.o Geshtu.o AtraHasis.o

AtraHasis: $(objects)
	cc -o AtraHasis $(objects)

Jarasandha.o: Jarasandha.c Jarasandha.h
	cc -c Jarasandha.c

Geshtu.o: Geshtu.c Geshtu.h
	cc -c Geshtu.c

AtraHasis.o: AtraHasis.c AtraHasis.h
	cc -c AtraHasis.c

clean:
	rm AtraHasis $(objects)

