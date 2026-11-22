CC = gcc
CFLAGS = -Wall -Wextra -pthread

TARGET = hw04
SOURCE = hw04.c

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) $(SOURCE) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) output.txt

rebuild: clean all