dmc: ../addmc/src/*
	make -C ../addmc clean
	make -C ../addmc dmc opt=-Ofast link=-static
	rm -f dmc
	cp ../addmc/dmc .

.PHONY: clean

clean:
	rm -f dmc
