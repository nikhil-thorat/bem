.SILENT:

compile:
	clang ./src/brain.c -o brain
	clang ./src/ear.c -o ear
	clang ./src/mouth.c -o mouth

run:
	./brain 8080 8081

clean:
	rm -r ./system_*
