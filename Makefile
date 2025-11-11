CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra
TARGET = fap_solver
SOURCE = main.cpp

# Regla principal: compilar el programa
all: $(TARGET)

$(TARGET): $(SOURCE)
	@echo "Compilando $(TARGET)..."
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCE)
	@echo "Compilación exitosa!"

# Limpiar archivos generados
clean:
	@echo "Limpiando archivos de compilación..."
	rm -f $(TARGET)
	@echo "Limpieza completa!"

# Limpiar también los archivos de salida (logs y soluciones)
cleanall: clean
	@echo "Limpiando archivos de salida..."
	rm -f *_log.txt *_solution.txt
	@echo "Todos los archivos eliminados!"

# Ejecutar con instancia por defecto (Tiny.scen)
run: $(TARGET)
	./$(TARGET) Tiny.scen

# Ayuda
help:
	@echo "Makefile para FAP Solver"
	@echo ""
	@echo "Comandos disponibles:"
	@echo "  make          - Compila el programa"
	@echo "  make clean    - Elimina archivos de compilación"
	@echo "  make cleanall - Elimina compilación y resultados"
	@echo "  make run      - Compila y ejecuta con Tiny.scen"
	@echo "  make help     - Muestra esta ayuda"
	@echo ""
	@echo "Ejemplo de uso:"
	@echo "  make"
	@echo "  ./fap_solver Swisscom.scen"

.PHONY: all clean cleanall run help