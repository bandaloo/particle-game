game: main.c
	gcc main.c -o particles -I include -L lib -l SDL2-2.0.0 -l SDL2_image-2.0.0

clean:
	rm particles 
