all: myShell

myShell: myShell.c
	gcc myShell.c -std=gnu99 -Wpedantic -o myShell

clean:
	rm myShell