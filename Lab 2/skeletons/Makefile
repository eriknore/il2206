FILES= cyclic_scheduler executiontimes mixedscheduling overloaddetection overloaddetection2 paralleltasks periodictasks producerconsumer producerconsumer_prot producerconsumer_rndvzs producerconsumer_sem rms workingoverloaddetection extra

all: compile clean_build

compile:
	gnatmake *.adb
	
clean_build:
	rm -f *.o *.ali

clean: clean_build
	rm -f $(FILES)
