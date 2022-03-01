dmc.sif: ../addmc/src/*
	make -C ../addmc clean-libraries
	singularity build -F dmc.sif Singularity

dmc: ../addmc/src/*
	make -C ../addmc clean
	make -C ../addmc dmc opt=-Ofast link=-static
	rm -f dmc
	cp ../addmc/dmc .

.PHONY: clean

clean:
	rm -f dmc dmc.sif
