all:
	cd library_files; make
	cd init_process; make all
	cd user_processes; make all
clean:
	cd library_files; make clean
	cd init_process; make clean
	cd user_processes; make clean