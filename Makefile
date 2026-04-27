# Ορισμός του compiler (για Linux ARM)
CC = arm-linux-gnueabihf-gcc

# Βασικά flags (απενεργοποίηση βελτιστοποιήσεων για debugging)
CFLAGS = -g -O0

# Τα αρχεία μας
TARGET = access_system
OBJS = main.o hash.o

# Ο προεπιλεγμένος στόχος αν γράψεις σκέτο 'make'
all: $(TARGET)

# Πώς να ενώσει τα αρχεία (Link)
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET)

# Πώς να κάνει compile τη C
main.o: main.c
	$(CC) $(CFLAGS) -c main.c -o main.o

# Πώς να κάνει assemble την Assembly
hash.o: hash.s
	$(CC) $(CFLAGS) -c hash.s -o hash.o

# ---- Η ΜΑΓΕΙΑ ΕΙΝΑΙ ΕΔΩ ----
# Γράφοντας 'make run', κάνει compile (αν χρειάζεται) και το τρέχει μέσω QEMU!
run: $(TARGET)
	qemu-arm -L /usr/arm-linux-gnueabihf ./$(TARGET)

# Καθαρισμός
clean:
	rm -f $(OBJS) $(TARGET)