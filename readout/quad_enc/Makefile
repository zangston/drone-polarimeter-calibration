CC = gcc
CFLAGS = -I/usr/include/python3.10
LIBS = -lpython3.10
SRC = declan-python-pickle.c
OUT = declan-python-pickle.out

all: $(OUT)

$(OUT): $(SRC)
	$(CC) -o $(OUT) $(SRC) $(CFLAGS) $(LIBS)

clean:
	rm -f $(OUT)