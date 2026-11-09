.SILENT:

compile:
	clang ./src/brain.c -o brain
	clang ./src/ear.c -o ear
	clang ./src/mouth.c -o mouth
	clang ./src/ui.c -o ui

run:
	./brain 8080 8081

clean:
	rm -f ./brain ./ear ./mouth ./ui
	rm -rf ./system_*
