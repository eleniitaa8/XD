# Compilador
CC = gcc

# Flags de compilación
CFLAGS = -Wall -Wextra -g

# Obtén todos los archivos .c en el directorio actual
SOURCES = $(wildcard *.c)

# Genera nombres de ejecutables a partir de los archivos fuente, sin extensión
TARGETS = $(SOURCES:.c=)

# Regla principal: compila todos los archivos .c como ejecutables separados
all: $(TARGETS)

# Regla para compilar cada archivo .c en su propio ejecutable
%: %.c
	$(CC) $(CFLAGS) -o $@ $<

# Limpieza de archivos compilados
clean:
	rm -f $(TARGETS)
