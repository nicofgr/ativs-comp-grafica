build: 
	#clang -I./include/ -std=c99 -Wall ./src/*.c -lSDL2 -lGL -lX11 -lpthread -lXrandr -lXi -ldl -lm -o  saida.out
	#clang -I./include/ -std=c99 -Wall -Werror -fsanitize=address ./src/*.c -lSDL2 -lGL -lX11 -lpthread -lXrandr -lXi -ldl -lm -o  saida.out
	clang -I./include/ -std=c99 -Wall -fsanitize=address ./src/*.c -lSDL2 -lGL -lX11 -lpthread -lXrandr -lXi -ldl -lm -o  saida.out
	#Turn -fsanitize off for release build

debug:
	gcc -g -I./include/ -std=c99 -Wall -fsanitize=address ./src/*.c -lSDL2 -lGL -lX11 -lpthread -lXrandr -lXi -ldl -lm -o  saida.out

profile:
	clang -I./include/ -std=c99 -Wall -fsanitize=address ./src/*.c -lSDL2 -lGL -lX11 -lpthread -lXrandr -lXi -ldl -lm -pg -o saida.out

run:
	./saida.out

bresenham:
	./saida.out bresenham

wu:
	./saida.out wu

clean:
	rm ./saida.out
