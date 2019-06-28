.PHONY: clean lint run bochs init

run:
	cd kernel/; make run

init:
	cd kernel/; make init

clean:
	cd kernel/; make clean

lint:
	cd kernel/; make lint

bochs:
	cd kernel/; make bochs